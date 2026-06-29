// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "VulkanTransferBatch.h"
#include "VulkanDataArray.h"
#include "VulkanIndexBuffer.h"
#include "VulkanRenderer.h"
#include "VulkanTexelArray.h"
#include "VulkanTextureBuffer1D.h"
#include "VulkanTextureBuffer1DArray.h"
#include "VulkanTextureBuffer2D.h"
#include "VulkanTextureBuffer2DArray.h"
#include "VulkanTextureBuffer3D.h"
#include "VulkanTextureBufferCube.h"
#include "VulkanTextureBufferCubeArray.h"
#include "VulkanVertexBuffer.h"
#include <thread>
#include <utility>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
VulkanTransferBatch::VulkanTransferBatch(cobalt::logging::ILogger* log, VulkanRenderer* renderer, StartTiming startTiming, EndTiming endTiming)
: _log(log), _renderer(renderer), _startTiming(startTiming), _endTiming(endTiming)
{
	_submitCompleteToken = std::make_shared<VulkanRenderer::CompletionToken>();
}

//----------------------------------------------------------------------------------------
VulkanTransferBatch::~VulkanTransferBatch()
{
	// Delete any allocated transfer buffers
	for (const auto& entry : _transferBufferAllocations)
	{
		_renderer->GetMemoryManager()->DestroyBuffer(entry.buffer, entry.bufferAllocation);
	}

	// Free the command buffer for this transfer batch
	if (_commandBuffer != VK_NULL_HANDLE)
	{
		_renderer->FreeBatchCommandBuffer(_commandBuffer, _commandPool);
	}

	// Delete the allocated fence object
	if (_fence != VK_NULL_HANDLE)
	{
		vkDestroyFence(_renderer->GetDevice(), _fence, nullptr);
	}

	// Notify the renderer that this transfer batch is no longer running in a detached state if required
	if (_batchDetachedFromOwner)
	{
		_renderer->DecrementDetachedTransferBatchCount();
	}
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void VulkanTransferBatch::Delete()
{
	// It's permissible to submit a batch without waiting for it to complete, and allowing the transfer batch object
	// itself to be cleaned up. If we do this though, we need to clean up resources for this pending transfer. In this
	// case, we defer the actual deletion of this object, and spawn a thread here to wait for the batch transfer to
	// complete, cleaning up this object when it is finished.
	if (!IsComplete())
	{
		_renderer->IncrementDetachedTransferBatchCount();
		_batchDetachedFromOwner = true;
		std::thread([=]() {
			bool submitBatchBeforeWaiting = false;
			{
				std::scoped_lock<std::mutex> lock(_accessMutex);
				submitBatchBeforeWaiting = !_batchSubmitted;
			}
			if (submitBatchBeforeWaiting)
			{
				_log->Error("Transfer batch was destroyed without being submitted, but after it has been used for operations.");
				SubmitBatch();
			}
			WaitForComplete();
			delete this;
		})
		  .detach();
		return;
	}

	// Since this batch is completed, delete it immediately.
	delete this;
}

//----------------------------------------------------------------------------------------
// Submission methods
//----------------------------------------------------------------------------------------
SuccessToken VulkanTransferBatch::SubmitBatch()
{
	// Ensure the batch hasn't already been submitted
	std::unique_lock<std::mutex> lock(_accessMutex);
	if (_batchSubmitted)
	{
		_log->Error("Transfer batch has already been submitted.");
		return false;
	}

	// Mark the batch as submitted
	_batchSubmitted = true;
	_submissionStateChanged.notify_all();

	// If there's no work to do in the batch, it's not an error, but we shortcut the process and complete immediately
	// here.
	if (_initializeOperations.empty() && _transferOperations.empty() && _finalizeOperations.empty())
	{
		std::scoped_lock<std::mutex> tokenLock(_submitCompleteToken->mutex);
		_submitCompleteToken->complete = true;
		_submitCompleteToken->notifyOnComplete.notify_all();
		return true;
	}

	// Create the native fence object
	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = 0;
	VkResult createFenceResult = vkCreateFence(_renderer->GetDevice(), &fenceInfo, nullptr, &_fence);
	if (createFenceResult != VK_SUCCESS)
	{
		_log->Error("Failed to create fence object for VulkanTransferBatch with error code {0}", createFenceResult);
		return false;
	}
	lock.unlock();

	// Obtain a command buffer and record all queued transfer operations into it. Resource ownership changes are
	// recorded first and last, while the individual data copy callbacks stay in their original submission order.
	_commandBuffer = _renderer->GetBatchCommandBuffer(_endTiming, _commandPool, _targetQueue);

	// Perform queue family ownership transfers from graphics to transfer queues if required
	bool canDiscardCurrentContent = (_startTiming == StartTiming::Immediately);
	bool queueTransferFromGraphicsRequired = (!_renderer->IsTransferQueueSharedWithGraphics() && !canDiscardCurrentContent);
	for (const auto& operationEntry : _initializeOperations)
	{
		auto renderer = _renderer;
		auto commandBuffer = _commandBuffer;
		std::visit([commandBuffer, renderer, queueTransferFromGraphicsRequired, canDiscardCurrentContent](auto* object) {
			if (object->PerformTransferQueueAcquireOperation(commandBuffer, canDiscardCurrentContent) && queueTransferFromGraphicsRequired)
			{
				renderer->ScheduleGraphicsQueueReleaseOperation(object);
			}
		},
		           operationEntry.object);
	}
	_initializeOperations.clear();

	// Perform the transfer operations
	for (const auto& operationEntry : _transferOperations)
	{
		operationEntry(_commandBuffer);
	}
	_transferOperations.clear();

	// Perform queue family ownership transfers from transfer to graphics queues if required
	for (const auto& operationEntry : _finalizeOperations)
	{
		auto renderer = _renderer;
		auto commandBuffer = _commandBuffer;
		std::visit([commandBuffer, renderer](auto* object) {
			if (object->PerformTransferQueueReleaseOperation(commandBuffer) && !renderer->IsTransferQueueSharedWithGraphics())
			{
				renderer->ScheduleGraphicsQueueAcquireOperation(object);
			}
		},
		           operationEntry.object);
	}
	_finalizeOperations.clear();

	// Submit the batch for processing
	_renderer->SubmitBatchCommandBuffer(_commandBuffer, _fence, _targetQueue, _startTiming, _submitCompleteToken);
	return true;
}

//----------------------------------------------------------------------------------------
bool VulkanTransferBatch::IsSubmitted() const
{
	std::scoped_lock<std::mutex> lock(_accessMutex);
	return _batchSubmitted;
}

//----------------------------------------------------------------------------------------
bool VulkanTransferBatch::IsComplete() const
{
	// Check if the batch has been submitted
	{
		std::scoped_lock<std::mutex> lock(_accessMutex);
		if (!_batchSubmitted)
		{
			return false;
		}
	}

	// Check if the submission process is complete
	{
		std::unique_lock<std::mutex> tokenLock(_submitCompleteToken->mutex);
		if (!_submitCompleteToken->complete)
		{
			return false;
		}
	}

	// Check if the batch transfers have completed
	return ((_fence == VK_NULL_HANDLE) || (vkGetFenceStatus(_renderer->GetDevice(), _fence) == VK_SUCCESS));
}

//----------------------------------------------------------------------------------------
void VulkanTransferBatch::WaitForComplete() const
{
	// Wait for the batch to be submitted
	{
		std::unique_lock<std::mutex> lock(_accessMutex);
		while (!_batchSubmitted)
		{
			_submissionStateChanged.wait(lock);
		}
	}

	// Wait for the submission process to complete
	{
		std::unique_lock<std::mutex> tokenLock(_submitCompleteToken->mutex);
		while (!_submitCompleteToken->complete)
		{
			_submitCompleteToken->notifyOnComplete.wait(tokenLock);
		}
	}

	// Wait for the batch transfers to complete if required
	if (_fence != VK_NULL_HANDLE)
	{
		vkWaitForFences(_renderer->GetDevice(), 1, &_fence, VK_TRUE, std::numeric_limits<uint64_t>::max());
	}
}

//----------------------------------------------------------------------------------------
// Operation methods
//----------------------------------------------------------------------------------------
void VulkanTransferBatch::CreateTemporaryTransferBuffer(size_t bufferSizeInBytes, VkBuffer& buffer, VmaAllocation& bufferAllocation)
{
	// Create the requested transfer buffer
	if (!_renderer->GetMemoryManager()->CreateBuffer(bufferSizeInBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, 0, buffer, bufferAllocation))
	{
		_log->Error("CreateBuffer failed");
		return;
	}

	// Add this buffer to the set of allocated transfer buffers
	TransferBufferAllocation allocationEntry{};
	allocationEntry.bufferSizeInBytes = bufferSizeInBytes;
	allocationEntry.buffer = buffer;
	allocationEntry.bufferAllocation = bufferAllocation;
	std::scoped_lock<std::mutex> lock(_accessMutex);
	_transferBufferAllocations.push_back(allocationEntry);
}

//----------------------------------------------------------------------------------------
void VulkanTransferBatch::AddInitializeOperation(QueueOperationObjectTypes object)
{
	// Ensure the batch hasn't already been submitted
	std::scoped_lock<std::mutex> lock(_accessMutex);
	if (_batchSubmitted)
	{
		_log->Error("Attempted to add transfers to a batch which has already been submitted");
		return;
	}

	// Add the initialize operation
	void* objectPointerRaw = std::visit([](auto* objectResolved) { return static_cast<void*>(objectResolved); }, object);
	auto& operationEntry = _initializeOperations.emplace_back();
	operationEntry.object = object;
	operationEntry.objectPointerRaw = objectPointerRaw;
}

//----------------------------------------------------------------------------------------
void VulkanTransferBatch::AddTransferOperation(std::function<void(VkCommandBuffer)>&& transferOperation)
{
	// Ensure the batch hasn't already been submitted
	std::scoped_lock<std::mutex> lock(_accessMutex);
	if (_batchSubmitted)
	{
		_log->Error("Attempted to add transfers to a batch which has already been submitted");
		return;
	}

	// Add the transfer operation
	_transferOperations.push_back(std::move(transferOperation));
}

//----------------------------------------------------------------------------------------
void VulkanTransferBatch::AddFinalizeOperation(QueueOperationObjectTypes object)
{
	// Ensure the batch hasn't already been submitted
	std::scoped_lock<std::mutex> lock(_accessMutex);
	if (_batchSubmitted)
	{
		_log->Error("Attempted to add transfers to a batch which has already been submitted");
		return;
	}

	// Add the finalize operation
	void* objectPointerRaw = std::visit([](auto* objectResolved) { return static_cast<void*>(objectResolved); }, object);
	auto& operationEntry = _finalizeOperations.emplace_back();
	operationEntry.object = object;
	operationEntry.objectPointerRaw = objectPointerRaw;
}

} // namespace cobalt::graphics
