// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "VulkanDataArray.h"
#include "VulkanDataArrayOutput.h"
#include "VulkanRenderer.h"
#include "VulkanTransferBatch.h"
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <Internal/RendererSupport/KnownDynamicCast.h>
#include <algorithm>
#include <cstring>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
VulkanDataArray::VulkanDataArray(cobalt::logging::ILogger* log, VulkanRenderer* renderer)
: _log(log), _renderer(renderer), _buildIndex(0), _drawIndex(1)
{}

//----------------------------------------------------------------------------------------
VulkanDataArray::~VulkanDataArray()
{
	ReleaseMemory();
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void VulkanDataArray::Delete()
{
	_renderer->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
SuccessToken VulkanDataArray::AllocateMemory()
{
	// Ensure the array hasn't already been created
	if (_bufferCreated)
	{
		_log->Error("Attempted to allocate memory for a data array that has already been allocated.");
		return false;
	}

	// Ensure the buffer layout has been set
	if (!_bufferLayoutSet)
	{
		_log->Error("Attempted to allocate memory for a data array when the buffer layout has not been defined.");
		return false;
	}

	// Calculate the total size of the required buffer
	_totalBufferSizeInBytes = _structureEntryCount * _structureStrideInBytes;

	// Ensure that the buffer size is not zero
	if (_totalBufferSizeInBytes == 0)
	{
		_log->Error("Attempted to allocate an empty data array.");
		return false;
	}

	// Set the VMA usage and allocation flags appropriately
	VkBufferUsageFlags usageFlagsNative = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	if (((_usageFlags & UsageFlags::IndirectDrawSource) == UsageFlags::IndirectDrawSource) || ((_usageFlags & UsageFlags::IndirectDrawCountSource) == UsageFlags::IndirectDrawCountSource))
	{
		usageFlagsNative |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
	}
	VmaMemoryUsage memoryUsageFlags = VMA_MEMORY_USAGE_GPU_ONLY;
	VmaAllocationCreateFlags memoryAllocationFlags = 0;
	if (((_usageFlags & UsageFlags::IndirectDrawCountSource) == UsageFlags::IndirectDrawCountSource) && !_renderer->GetExtensionInfo().extensionLoaded_VK_KHR_draw_indirect_count)
	{
		// Fall back to slower CPU-accessible memory if we don't support indirect draw counts from GPU buffers, and this
		// buffer has been set as an indirect draw count source. In this case the IndirectMultiDrawNative feature won't
		// have been advertised, so the caller has been made aware of the performance hit.
		memoryUsageFlags = VMA_MEMORY_USAGE_AUTO;
		memoryAllocationFlags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
	}

	// Create the data array
	VulkanMemoryManager* memoryManager = _renderer->GetMemoryManager();
	memoryManager->CreateBuffer(_totalBufferSizeInBytes, usageFlagsNative, memoryUsageFlags, memoryAllocationFlags, _resourceBuffer, _resourceBufferAllocation);

	// If initial data has been provided, stage an upload of that data into the buffer.
	if (_initialDataSet && (_initialDataSizeInBytes != _totalBufferSizeInBytes))
	{
		_log->Error("Initial data array data of size {0} was provided, but {1} bytes are needed for the buffer.", _initialDataSizeInBytes, _totalBufferSizeInBytes);
		return false;
	}
	if (_initialDataSet)
	{
		// Create an upload buffer for the data
		VkBuffer uploadBuffer;
		VmaAllocation uploadBufferAllocation;
		_renderer->CreateTemporaryUploadBuffer(_initialDataSizeInBytes, uploadBuffer, uploadBufferAllocation, false);

		// Map the upload buffer into the CPU address space
		uint8_t* uploadBufferMapped;
		if (!memoryManager->MapBufferMemory(uploadBufferAllocation, uploadBufferMapped))
		{
			_log->Error("Failed to map upload buffer for data array");
		}

		// Write the initial data into the upload buffer
		std::memcpy(uploadBufferMapped, _initialData, _initialDataSizeInBytes);

		// Unmap the upload buffer
		memoryManager->UnmapBufferMemory(uploadBufferAllocation);

		// Allocate a new command buffer on the build queue to handle the initial data transfer
		VkCommandBuffer commandBuffer = _renderer->GetBuildCommandBuffer();

		// Schedule a data transfer from the upload buffer to the target buffer
		VkBufferCopy copyRegion = {};
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = 0;
		copyRegion.size = _initialDataSizeInBytes;
		vkCmdCopyBuffer(commandBuffer, uploadBuffer, _resourceBuffer, 1, &copyRegion);

		// If the initial upload ran on a dedicated transfer queue, release queue ownership back to graphics now.
		if (!_renderer->IsTransferQueueSharedWithGraphics())
		{
			// Perform a release operation on the transfer queue for the buffer
			VkBufferMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = 0;
			barrier.srcQueueFamilyIndex = _renderer->GetTransferQueueFamily();
			barrier.dstQueueFamilyIndex = _renderer->GetGraphicsQueueFamily();
			barrier.buffer = _resourceBuffer;
			barrier.offset = 0;
			barrier.size = _totalBufferSizeInBytes;
			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);

			// Schedule an acquire operation on the graphics queue for the buffer
			_renderer->ScheduleGraphicsQueueAcquireOperation(this);
		}

		// Submit the command buffer
		_renderer->SubmitBuildCommandBuffer(commandBuffer);
	}

	// Allocate the counter buffer if required
	if (_hasCounter)
	{
		// Set the VMA usage and allocation flags appropriately
		VkBufferUsageFlags counterBufferUsageFlagsNative = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		if ((_usageFlags & UsageFlags::IndirectDrawCountSource) == UsageFlags::IndirectDrawCountSource)
		{
			counterBufferUsageFlagsNative |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
		}
		VmaMemoryUsage counterBufferMemoryUsageFlags = VMA_MEMORY_USAGE_GPU_ONLY;
		VmaAllocationCreateFlags counterBufferMemoryAllocationFlags = 0;
		if (((_usageFlags & UsageFlags::IndirectDrawCountSource) == UsageFlags::IndirectDrawCountSource) && !_renderer->GetExtensionInfo().extensionLoaded_VK_KHR_draw_indirect_count)
		{
			// Fall back to slower CPU-accessible memory if we don't support indirect draw counts from GPU buffers, and
			// this buffer has been set as an indirect draw count source. In this case the IndirectMultiDrawNative
			// feature won't have been advertised, so the caller has been made aware of the performance hit.
			counterBufferMemoryUsageFlags = VMA_MEMORY_USAGE_AUTO;
			counterBufferMemoryAllocationFlags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
		}

		// Create the counter buffer
		memoryManager->CreateBuffer(sizeof(_counterResetValue), counterBufferUsageFlagsNative, counterBufferMemoryUsageFlags, counterBufferMemoryAllocationFlags, _counterBuffer, _counterBufferAllocation);

		// Create an upload buffer for the data
		_renderer->CreatePersistentUploadBuffer(_totalBufferSizeInBytes, _counterUploadBuffer, _counterUploadBufferAllocation);

		// Map the upload buffer into the CPU address space
		uint8_t* uploadBufferMapped;
		if (!memoryManager->MapBufferMemory(_counterUploadBufferAllocation, uploadBufferMapped))
		{
			_log->Error("Failed to map upload buffer for data array counter");
		}

		// Write the initial data into the upload buffer
		std::memcpy(uploadBufferMapped, &_counterResetValue, sizeof(_counterResetValue));

		// Unmap the upload buffer
		memoryManager->UnmapBufferMemory(_counterUploadBufferAllocation);
	}

	// Release any resources related to the initial data
	_initialDataSet = false;
	_initialData = nullptr;

	// Flag that the buffer has been created
	_bufferCreated = true;
	FlagBuildStateModified();
	return true;
}

//----------------------------------------------------------------------------------------
void VulkanDataArray::ReleaseMemory()
{
	auto* memoryManager = _renderer->GetMemoryManager();
	if (_bufferCreated)
	{
		memoryManager->DestroyBuffer(_resourceBuffer, _resourceBufferAllocation);
		if (_hasCounter)
		{
			memoryManager->DestroyBuffer(_counterBuffer, _counterBufferAllocation);
			memoryManager->DestroyBuffer(_counterUploadBuffer, _counterUploadBufferAllocation);
		}
	}
	if (_captureDataStagingBuffer != VK_NULL_HANDLE)
	{
		memoryManager->DestroyBuffer(_captureDataStagingBuffer, _captureDataStagingBufferAllocation);
	}
	if (_captureCounterStagingBuffer != VK_NULL_HANDLE)
	{
		memoryManager->DestroyBuffer(_captureCounterStagingBuffer, _captureCounterStagingBufferAllocation);
	}
}

//----------------------------------------------------------------------------------------
void VulkanDataArray::SetBufferLayout(size_t entryStrideInBytes, size_t entryCount, bool hasCounter, uint32_t counterResetValue)
{
	_structureEntryCount = entryCount;
	_structureStrideInBytes = entryStrideInBytes;
	_hasCounter = hasCounter;
	_counterResetValue = counterResetValue;
	_bufferLayoutSet = true;
}

//----------------------------------------------------------------------------------------
// Usage methods
//----------------------------------------------------------------------------------------
void VulkanDataArray::SetUsageFlags(UsageFlags usageFlags)
{
	_usageFlags = usageFlags;
}

//----------------------------------------------------------------------------------------
void VulkanDataArray::SetPerformanceHints(PerformanceHint performanceHintCpu, PerformanceHint performanceHintGpu)
{
	_performanceHintCpu = performanceHintCpu;
	_performanceHintGpu = performanceHintGpu;
}

//----------------------------------------------------------------------------------------
void VulkanDataArray::SetDataPersistenceFlags(DataPersistenceFlags dataPersistenceFlags)
{
	_dataPersistenceFlags = dataPersistenceFlags;
}

//----------------------------------------------------------------------------------------
// Initial data methods
//----------------------------------------------------------------------------------------
SuccessToken VulkanDataArray::SetInitialData(const void* sourceBuffer, size_t sourceBufferSizeInBytes)
{
	// Validate the current state of the buffer
	if (_bufferCreated)
	{
		_log->Error("Attempted to set initial data array data after the buffer has already been created.");
		return false;
	}
	if (_initialDataSet)
	{
		_log->Error("Attempted to set initial data array data when initial data has already been provided.");
		return false;
	}

	// Store the initial data for use when the buffer is allocated
	_initialData = reinterpret_cast<const uint8_t*>(sourceBuffer);
	_initialDataSizeInBytes = sourceBufferSizeInBytes;
	_initialDataSet = true;
	return true;
}

//----------------------------------------------------------------------------------------
// Data update methods
//----------------------------------------------------------------------------------------
SuccessToken VulkanDataArray::QueueDataUpdate(const void* sourceBuffer, size_t sourceBufferSizeInBytes, size_t targetBufferOffsetInBytes, ITransferBatch* transferBatch)
{
	// If we're targeting a transfer batch, make sure it's not already submitted.
	if ((transferBatch != nullptr) && transferBatch->IsSubmitted())
	{
		_log->Error("Attempted to queue a transfer using a transfer batch that has already been submitted");
		return false;
	}

	// Ensure the buffer is allowed to be modified
	if ((_performanceHintCpu & PerformanceHint::WriteFlagsMask) == PerformanceHint::WriteNever)
	{
		_log->Error("Attempted to update a data array that can't be modified.");
		return false;
	}

	// Verify that the requested write is within the bounds of the buffer
	if ((targetBufferOffsetInBytes > _totalBufferSizeInBytes) || ((targetBufferOffsetInBytes + sourceBufferSizeInBytes) > _totalBufferSizeInBytes))
	{
		_log->Error("Attempted to perform a write outside the bounds of a data array, with write location {0}, byte size {1}, against buffer size of {2}.", targetBufferOffsetInBytes, sourceBufferSizeInBytes, _totalBufferSizeInBytes);
		return false;
	}

	// Capture the supplied update settings
	auto* transferBatchResolved = KnownDynamicCast<VulkanTransferBatch*>(transferBatch);
	PendingWrite pendingWrite(transferBatchResolved);

	// Create an upload buffer for the data
	auto uploadBufferSizeInBytes = sourceBufferSizeInBytes;
	VkBuffer uploadBuffer;
	VmaAllocation uploadBufferAllocation;
	if (transferBatchResolved != nullptr)
	{
		transferBatchResolved->CreateTemporaryTransferBuffer(uploadBufferSizeInBytes, uploadBuffer, uploadBufferAllocation);
	}
	else
	{
		_renderer->CreateTemporaryUploadBuffer(uploadBufferSizeInBytes, uploadBuffer, uploadBufferAllocation, true);
	}

	// Map the upload buffer into the CPU address space
	uint8_t* uploadBufferMapped;
	VulkanMemoryManager* memoryManager = _renderer->GetMemoryManager();
	if (!memoryManager->MapBufferMemory(uploadBufferAllocation, uploadBufferMapped))
	{
		_log->Error("Failed to map upload buffer for data array");
		return false;
	}

	// Write the data into the upload buffer
	std::memcpy(uploadBufferMapped, sourceBuffer, sourceBufferSizeInBytes);
	pendingWrite.uploadBuffer = uploadBuffer;
	pendingWrite.targetBufferPos = targetBufferOffsetInBytes;
	pendingWrite.uploadBufferSizeInBytes = uploadBufferSizeInBytes;

	// Unmap the upload buffer
	memoryManager->UnmapBufferMemory(uploadBufferAllocation);

	// Queue a task to update the buffer with the supplied data
	if (transferBatchResolved != nullptr)
	{
		bool separateTransferQueue = _renderer->IsTransferQueueSharedWithGraphics();
		if (separateTransferQueue)
		{
			transferBatchResolved->AddInitializeOperation(this);
		}
		transferBatchResolved->AddTransferOperation([this, pendingWrite](VkCommandBuffer commandBuffer) { CompletePendingDataWrite(commandBuffer, pendingWrite); });
		if (separateTransferQueue)
		{
			transferBatchResolved->AddFinalizeOperation(this);
		}
	}
	else
	{
		std::unique_lock<std::mutex> lock(_buildStateMutex);
		_state[_buildIndex].pendingWrites.push_back(pendingWrite);
		lock.unlock();
		FlagBuildStateModified();
	}
	return true;
}

//----------------------------------------------------------------------------------------
void VulkanDataArray::UpdateCounterResetValue(uint32_t counterResetValue)
{
	// Ensure this data array has a counter attached
	if (!_hasCounter)
	{
		_log->Error("Attempted to update the counter reset value for a data array with no counter attached");
		return;
	}

	// Update the counter reset value to use next frame
	_state[_buildIndex].newCounterResetValue = counterResetValue;
	_state[_buildIndex].updatedCounterResetValue = true;
	FlagBuildStateModified();
}

//----------------------------------------------------------------------------------------
// Data transfer methods
//----------------------------------------------------------------------------------------
SuccessToken VulkanDataArray::QueueDataTransfer(IDataArray* targetBuffer, size_t transferCountInBytes, size_t sourceBufferOffsetInBytes, size_t targetBufferOffsetInBytes, ITransferBatch* transferBatch)
{
	// If we're targeting a transfer batch, make sure it's not already submitted.
	if ((transferBatch != nullptr) && transferBatch->IsSubmitted())
	{
		_log->Error("Attempted to queue a transfer using a transfer batch that has already been submitted");
		return false;
	}

	// Ensure the source and target buffers allow a transfer
	auto* targetBufferResolved = KnownDynamicCast<VulkanDataArray*>(targetBuffer);
	if ((_performanceHintGpu & PerformanceHint::ReadFlagsMask) == PerformanceHint::ReadNever)
	{
		_log->Error("Attempted to transfer from a data array that can't be read on the GPU.");
		return false;
	}
	if ((targetBufferResolved->_performanceHintGpu & PerformanceHint::WriteFlagsMask) == PerformanceHint::WriteNever)
	{
		_log->Error("Attempted to transfer to a data array that can't be modified on the GPU.");
		return false;
	}
	if ((_usageFlags & UsageFlags::TransferSource) != UsageFlags::TransferSource)
	{
		_log->Error("Attempted to transfer from a data array that hasn't specified the TransferSource usage type.");
		return false;
	}
	if ((targetBufferResolved->_usageFlags & UsageFlags::TransferDestination) != UsageFlags::TransferDestination)
	{
		_log->Error("Attempted to transfer to a data array that hasn't specified the TransferDestination usage type.");
		return false;
	}

	// Capture the supplied update settings and data
	auto* transferBatchResolved = KnownDynamicCast<VulkanTransferBatch*>(transferBatch);
	PendingTransfer pendingTransfer(targetBufferResolved, transferBatchResolved);
	pendingTransfer.transferCountInBytes = transferCountInBytes;
	pendingTransfer.sourceBufferPosInBytes = sourceBufferOffsetInBytes;
	pendingTransfer.targetBufferPosInBytes = targetBufferOffsetInBytes;

	// Queue a task to perform the data transfer
	if (transferBatchResolved != nullptr)
	{
		bool separateTransferQueue = _renderer->IsTransferQueueSharedWithGraphics();
		if (separateTransferQueue)
		{
			transferBatchResolved->AddInitializeOperation(this);
			transferBatchResolved->AddInitializeOperation(targetBufferResolved);
		}
		transferBatchResolved->AddTransferOperation([this, pendingTransfer](VkCommandBuffer commandBuffer) { CompletePendingDataTransfer(commandBuffer, pendingTransfer); });
		if (separateTransferQueue)
		{
			transferBatchResolved->AddFinalizeOperation(targetBufferResolved);
			transferBatchResolved->AddFinalizeOperation(this);
		}
	}
	else
	{
		std::unique_lock<std::mutex> lock(_buildStateMutex);
		_state[_buildIndex].pendingTransfers.push_back(pendingTransfer);
		lock.unlock();
		FlagBuildStateModified();
	}
	return true;
}

//----------------------------------------------------------------------------------------
// Output capture methods
//----------------------------------------------------------------------------------------
bool VulkanDataArray::HasCaptureTargets() const
{
	return !_state[_drawIndex].captureTargets.empty();
}

//----------------------------------------------------------------------------------------
void VulkanDataArray::AddOutputCaptureTarget(IDataArrayOutput* captureTarget)
{
	std::unique_lock<std::mutex> lock(_accessMutex);
	_state[_buildIndex].captureTargets.push_back(KnownDynamicCast<VulkanDataArrayOutput*>(captureTarget));
	lock.unlock();
	FlagBuildStateModified();
}

//----------------------------------------------------------------------------------------
void VulkanDataArray::RemoveOutputCaptureTarget(IDataArrayOutput* captureTarget)
{
	std::unique_lock<std::mutex> lock(_accessMutex);
	auto* captureTargetResolved = KnownDynamicCast<VulkanDataArrayOutput*>(captureTarget);
	for (auto captureTargetsIterator = _state[_buildIndex].captureTargets.begin(); captureTargetsIterator != _state[_buildIndex].captureTargets.end(); ++captureTargetsIterator)
	{
		if (*captureTargetsIterator == captureTargetResolved)
		{
			_state[_buildIndex].captureTargets.erase(captureTargetsIterator);
			lock.unlock();
			FlagBuildStateModified();
			return;
		}
	}
}

//----------------------------------------------------------------------------------------
void VulkanDataArray::CaptureDataBufferOutput(VkCommandBuffer commandBuffer)
{
	// If no capture targets are defined, abort any further processing.
	if (_state[_drawIndex].captureTargets.empty())
	{
		return;
	}

	// Retrieve or create a staging buffer for the captured data
	if (_captureDataStagingBuffer == VK_NULL_HANDLE)
	{
		_renderer->CreatePersistentReadbackBuffer(_totalBufferSizeInBytes, _captureDataStagingBuffer, _captureDataStagingBufferAllocation);
	}

	// Retrieve or create a staging buffer for the counter value
	if (_captureCounterStagingBuffer == VK_NULL_HANDLE)
	{
		_renderer->CreatePersistentReadbackBuffer(sizeof(uint32_t), _captureCounterStagingBuffer, _captureCounterStagingBufferAllocation);
	}

	// Read the counter value if a counter is present
	if (_hasCounter)
	{
		VkBufferCopy copyRegion = {};
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = 0;
		copyRegion.size = sizeof(_counterResetValue);
		vkCmdCopyBuffer(commandBuffer, _counterBuffer, _captureCounterStagingBuffer, 1, &copyRegion);
	}

	// Transfer each requested capture region into the data staging buffer
	for (auto* captureTarget : _state[_drawIndex].captureTargets)
	{
		// Retrieve the requested capture settings from our framebuffer output object
		size_t bufferStartPosInBytes = captureTarget->GetRequestedBufferOffset() * _structureStrideInBytes;
		size_t dataSizeInBytes = captureTarget->GetRequestedEntryCount() * _structureStrideInBytes;
		dataSizeInBytes = (dataSizeInBytes > 0 ? dataSizeInBytes : _totalBufferSizeInBytes);

		// Transfer the requested capture region into the staging buffer
		VkBufferCopy copyRegion = {};
		copyRegion.srcOffset = bufferStartPosInBytes;
		copyRegion.dstOffset = bufferStartPosInBytes;
		copyRegion.size = dataSizeInBytes;
		vkCmdCopyBuffer(commandBuffer, _resourceBuffer, _captureDataStagingBuffer, 1, &copyRegion);
	}
}

//----------------------------------------------------------------------------------------
void VulkanDataArray::CompleteCaptureDataBufferOutput()
{
	// Reset our flag indicating this buffer has been added as a current resource buffer
	_addedAsCurrent = false;

	// If no capture targets are defined, abort any further processing.
	if (_state[_drawIndex].captureTargets.empty())
	{
		return;
	}

	// Read the counter value if a counter is present
	uint32_t counterValue = 0;
	if (_hasCounter)
	{
		// Map the staging buffer
		VulkanMemoryManager* memoryManager = _renderer->GetMemoryManager();
		uint8_t* mappedBuffer;
		if (!memoryManager->MapBufferMemory(_captureCounterStagingBufferAllocation, mappedBuffer))
		{
			_log->Error("Failed to map staging buffer when attempting to capture data array counter value");
			return;
		}

		// Read the counter value from the buffer. Note that we use memcpy here rather than a simple cast to avoid
		// potentially misaligned memory access. While this should be legal on all platforms we care about, it's
		// technically undefined behaviour so we avoid it.
		std::memcpy(&counterValue, mappedBuffer, sizeof(counterValue));

		// Unmap the staging buffer
		memoryManager->UnmapBufferMemory(_captureCounterStagingBufferAllocation);
	}

	// Map the data staging buffer
	VulkanMemoryManager* memoryManager = _renderer->GetMemoryManager();
	uint8_t* mappedBuffer;
	if (!memoryManager->MapBufferMemory(_captureDataStagingBufferAllocation, mappedBuffer))
	{
		_log->Error("Failed to map staging buffer when attempting to capture data array output");
		return;
	}

	// Store the buffer data and counter values into each attached capture target
	const auto* mappedMemory = static_cast<const uint8_t*>(mappedBuffer);
	for (auto* captureTarget : _state[_drawIndex].captureTargets)
	{
		// Determine the position and size of the buffer region to capture
		size_t entryOffset = captureTarget->GetRequestedBufferOffset();
		size_t entryCountToCapture = captureTarget->GetRequestedEntryCount();
		entryCountToCapture = (entryCountToCapture == 0 ? _structureEntryCount : entryCountToCapture);
		entryCountToCapture = (((entryCountToCapture + entryOffset) > _structureEntryCount) ? (_structureEntryCount - entryOffset) : entryCountToCapture);
		size_t bufferStartPosInBytes = entryOffset * _structureStrideInBytes;

		// Store the requested capture data in the capture target
		bool detachAfterCapture = captureTarget->IsDetachingAfterCapture();
		captureTarget->StoreCaptureData(mappedMemory + bufferStartPosInBytes, entryCountToCapture, _structureStrideInBytes, _hasCounter, counterValue);

		// Record this captured framebuffer output with the renderer
		_renderer->AddCurrentDataArrayOutput(captureTarget);

		// Now that we've captured a frame, detach the output capture target if requested.
		if (detachAfterCapture)
		{
			RemoveOutputCaptureTarget(captureTarget);
		}
	}

	// Unmap the data staging buffer
	memoryManager->UnmapBufferMemory(_captureDataStagingBufferAllocation);
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
void VulkanDataArray::MigrateBuildStateToDrawState()
{
	std::swap(_buildIndex, _drawIndex);
	_stateModified.clear(std::memory_order_relaxed);
}

//----------------------------------------------------------------------------------------
void VulkanDataArray::CompletePendingDataWrites(VkCommandBuffer commandBuffer)
{
	// If a new reset value has been supplied for the counter, update it now.
	if (_state[_drawIndex].updatedCounterResetValue)
	{
		// Migrate the new reset value over to the draw state
		_state[_drawIndex].updatedCounterResetValue = false;
		_counterResetValue = _state[_drawIndex].newCounterResetValue;

		// Map the upload buffer into the CPU address space
		VulkanMemoryManager* memoryManager = _renderer->GetMemoryManager();
		uint8_t* uploadBufferMapped;
		if (!memoryManager->MapBufferMemory(_counterUploadBufferAllocation, uploadBufferMapped))
		{
			_log->Error("Failed to map upload buffer for data array counter");
		}

		// Write the reset data into the upload buffer
		std::memcpy(uploadBufferMapped, &_counterResetValue, sizeof(_counterResetValue));

		// Unmap the upload buffer
		memoryManager->UnmapBufferMemory(_counterUploadBufferAllocation);
	}

	// Obtain our set of current pending writes
	std::vector<PendingWrite>& pendingWrites = _state[_drawIndex].pendingWrites;

	// If there are no pending writes, abort any further processing.
	if (pendingWrites.empty())
	{
		return;
	}

	// Perform each data write into the upload buffer
	for (const PendingWrite& write : pendingWrites)
	{
		CompletePendingDataWrite(commandBuffer, write);
	}

	// Free our set of pending writes
	pendingWrites.clear();
}

//----------------------------------------------------------------------------------------
void VulkanDataArray::CompletePendingDataWrite(VkCommandBuffer commandBuffer, const PendingWrite& pendingWrite)
{
	// Schedule a data transfer from the upload buffer to the target buffer
	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = pendingWrite.targetBufferPos;
	copyRegion.size = pendingWrite.uploadBufferSizeInBytes;
	vkCmdCopyBuffer(commandBuffer, pendingWrite.uploadBuffer, _resourceBuffer, 1, &copyRegion);
}

//----------------------------------------------------------------------------------------
void VulkanDataArray::CompletePendingDataTransfers(VkCommandBuffer commandBuffer)
{
	// Obtain the set of pending data transfers. If no transfers are pending, abort any further processing.
	std::vector<PendingTransfer>& pendingTransfers = _state[_drawIndex].pendingTransfers;
	if (pendingTransfers.empty())
	{
		return;
	}

	// Complete each pending data transfer
	for (const PendingTransfer& transfer : pendingTransfers)
	{
		CompletePendingDataTransfer(commandBuffer, transfer);
	}

	// Clear the set of pending transfers now that they've been performed
	pendingTransfers.clear();
}

//----------------------------------------------------------------------------------------
void VulkanDataArray::CompletePendingDataTransfer(VkCommandBuffer commandBuffer, const PendingTransfer& pendingTransfer)
{
	// Schedule a data transfer from this buffer to the target buffer
	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = pendingTransfer.sourceBufferPosInBytes;
	copyRegion.dstOffset = pendingTransfer.targetBufferPosInBytes;
	copyRegion.size = pendingTransfer.transferCountInBytes;
	vkCmdCopyBuffer(commandBuffer, _resourceBuffer, pendingTransfer.targetBuffer->_resourceBuffer, 1, &copyRegion);
}

//----------------------------------------------------------------------------------------
void VulkanDataArray::PerformGraphicsQueueAcquireOperation(VkCommandBuffer commandBuffer)
{
	// Perform an acquire operation on the graphics queue for the buffer
	//##FIX## Probably overly broad. Tighten this up. Remember use for indirect draw operations.
	VkBufferMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
	barrier.srcQueueFamilyIndex = _renderer->GetTransferQueueFamily();
	barrier.dstQueueFamilyIndex = _renderer->GetGraphicsQueueFamily();
	barrier.buffer = _resourceBuffer;
	barrier.offset = 0;
	barrier.size = _totalBufferSizeInBytes;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);
}

//----------------------------------------------------------------------------------------
void VulkanDataArray::PerformGraphicsQueueReleaseOperation(VkCommandBuffer commandBuffer)
{
	// Perform a release operation on the graphics queue for the buffer
	VkBufferMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
	barrier.dstAccessMask = 0;
	barrier.srcQueueFamilyIndex = _renderer->GetGraphicsQueueFamily();
	barrier.dstQueueFamilyIndex = _renderer->GetTransferQueueFamily();
	barrier.buffer = _resourceBuffer;
	barrier.offset = 0;
	barrier.size = _totalBufferSizeInBytes;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);
}

//----------------------------------------------------------------------------------------
bool VulkanDataArray::PerformTransferQueueAcquireOperation(VkCommandBuffer commandBuffer, bool canDiscardCurrentContent)
{
	// Increment the transfer queue usage count for this buffer. If there was already an existing usage, abort any
	// further processing.
	uint32_t previousTransferQueueUseCount = _transferQueueUseCount.fetch_add(1, std::memory_order_acq_rel);
	if (previousTransferQueueUseCount != 0)
	{
		return false;
	}

	// Since we're transferring ownership to the transfer queue, attempt to remove this from the list of buffers pending
	// an aquire operation on the graphics queue. This can occur where this same buffer is used in a batch transfer,
	// that transfer completes, then before another frame advances it is added to another batch transfer. If the removal
	// succeeds, we know the buffer is currently still owned by the transfer queue, otherwise we perform a transfer
	// queue acquire operation here.
	if (_renderer->CancelPendingGraphicsQueueAcquireOperation(this))
	{
		return false;
	}

	// Perform an acquire operation on the transfer queue for the buffer
	VkBufferMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.srcQueueFamilyIndex = _renderer->GetGraphicsQueueFamily();
	barrier.dstQueueFamilyIndex = _renderer->GetTransferQueueFamily();
	barrier.buffer = _resourceBuffer;
	barrier.offset = 0;
	barrier.size = _totalBufferSizeInBytes;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);
	return true;
}

//----------------------------------------------------------------------------------------
bool VulkanDataArray::PerformTransferQueueReleaseOperation(VkCommandBuffer commandBuffer)
{
	// Decrement the transfer queue usage count for this buffer. If there is at least one user remaining, abort any
	// further processing.
	uint32_t previousTransferQueueUseCount = _transferQueueUseCount.fetch_sub(1, std::memory_order_acq_rel);
	if (previousTransferQueueUseCount != 1)
	{
		return false;
	}

	// Perform a release operation on the transfer queue for the buffer
	VkBufferMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = 0;
	barrier.srcQueueFamilyIndex = _renderer->GetTransferQueueFamily();
	barrier.dstQueueFamilyIndex = _renderer->GetGraphicsQueueFamily();
	barrier.buffer = _resourceBuffer;
	barrier.offset = 0;
	barrier.size = _totalBufferSizeInBytes;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);
	return true;
}

//----------------------------------------------------------------------------------------
void VulkanDataArray::FlagBuildStateModified()
{
	if (!_stateModified.test_and_set(std::memory_order_acquire))
	{
		_renderer->FlagObjectModified(this);
	}
}

//----------------------------------------------------------------------------------------
bool VulkanDataArray::HasCounter() const
{
	return _hasCounter;
}

//----------------------------------------------------------------------------------------
void VulkanDataArray::ResetCounter(VkCommandBuffer commandBuffer)
{
	// Schedule a data transfer from the upload buffer to the target buffer
	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = sizeof(_counterResetValue);
	vkCmdCopyBuffer(commandBuffer, _counterUploadBuffer, _counterBuffer, 1, &copyRegion);
}

//----------------------------------------------------------------------------------------
VkBuffer VulkanDataArray::GetNativeBuffer() const
{
	return _resourceBuffer;
}

//----------------------------------------------------------------------------------------
VkBuffer VulkanDataArray::GetNativeCounterBuffer() const
{
	return _counterBuffer;
}

//----------------------------------------------------------------------------------------
VmaAllocation VulkanDataArray::GetNativeAllocation() const
{
	return _resourceBufferAllocation;
}

//----------------------------------------------------------------------------------------
VmaAllocation VulkanDataArray::GetNativeCounterAllocation() const
{
	return _counterBufferAllocation;
}

//----------------------------------------------------------------------------------------
size_t VulkanDataArray::GetBufferSizeInBytes() const
{
	return _totalBufferSizeInBytes;
}

//----------------------------------------------------------------------------------------
void VulkanDataArray::AddAsCurrentBuffer()
{
	if (!_addedAsCurrent)
	{
		_renderer->AddCurrentDataArray(this);
		_addedAsCurrent = true;
	}
}

} // namespace cobalt::graphics
