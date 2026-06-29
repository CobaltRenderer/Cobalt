// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "VulkanIndexBuffer.h"
#include "VulkanRenderer.h"
#include "VulkanTexelArray.h"
#include "VulkanTransferBatch.h"
#include <Internal/RendererSupport/KnownDynamicCast.h>
#include <cstring>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
VulkanIndexBuffer::VulkanIndexBuffer(cobalt::logging::ILogger* log, VulkanRenderer* renderer)
{
	_log = log;
	_renderer = renderer;
	_indexBufferCreated = false;
	_initialDataSet = false;
	_initialData = nullptr;
	_buildIndex = 0;
	_drawIndex = 1;
}

//----------------------------------------------------------------------------------------
VulkanIndexBuffer::~VulkanIndexBuffer()
{
	ReleaseMemory();
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void VulkanIndexBuffer::Delete()
{
	_renderer->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
SuccessToken VulkanIndexBuffer::AllocateMemory()
{
	return AllocateMemoryInternal(nullptr);
}

//----------------------------------------------------------------------------------------
SuccessToken VulkanIndexBuffer::AllocateMemoryWithAlias(ITexelArray* texelArray)
{
	auto* texelArrayResolved = KnownDynamicCast<VulkanTexelArray*>(texelArray);
	if (texelArrayResolved == nullptr)
	{
		_log->Error("Attempted to allocate memory for an index buffer with an alias, where no alias object was supplied.");
		return false;
	}
	return AllocateMemoryInternal(texelArrayResolved);
}

//----------------------------------------------------------------------------------------
bool VulkanIndexBuffer::AllocateMemoryInternal(VulkanTexelArray* texelArray)
{
	// Ensure the array hasn't already been created
	if (_indexBufferCreated)
	{
		_log->Error("Attempted to allocate memory for an index buffer that has already been allocated.");
		return false;
	}
	if (!_indexAttributeAdded)
	{
		_log->Error("Attempted to allocate memory for an index buffer that contains no index attributes.");
		return false;
	}

	// Ensure that a manual memory layout has been defined if an alias is being used
	bool aliasAsTexelArray = (texelArray != nullptr);
	if (aliasAsTexelArray && !_manualBufferLayout)
	{
		_log->Error("Attempted to allocate memory for an index buffer with an alias, when a manual buffer layout was not defined. A manual layout must be used if an alias is present, or the memory structure cannot be known, and therefore cannot be shared in a well defined manner.");
		return false;
	}

	// Calculate the total size of the required buffer
	_totalBufferSizeInBytes = _indexAttributeInfo.dataTypeByteSize * _indexAttributeInfo.indexCount;

	//##TODO## Use the usage flags to control the way buffers are allocated and modified

	// Create the index buffer
	VulkanMemoryManager* memoryManager = _renderer->GetMemoryManager();
	VkBufferUsageFlags usageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	if (aliasAsTexelArray)
	{
		usageFlags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
	}
	memoryManager->CreateBuffer(_totalBufferSizeInBytes, usageFlags, VMA_MEMORY_USAGE_GPU_ONLY, 0, _indexBuffer, _indexBufferAllocation);

	// If initial data has been provided, stage an upload of that data into the buffer.
	if (_initialDataSet)
	{
		// Ensure the provided initial data is of the correct size
		if ((_initialDataEntryStrideInBytes == 1) && (_initialDataEntryCount != _totalBufferSizeInBytes))
		{
			_log->Error("Raw initial index data of size {0} was provided, but {1} bytes are needed for the buffer.", _initialDataEntryCount, _totalBufferSizeInBytes);
			return false;
		}

		// Create an upload buffer for the data
		VkBuffer uploadBuffer;
		VmaAllocation uploadBufferAllocation;
		_renderer->CreateTemporaryUploadBuffer(_totalBufferSizeInBytes, uploadBuffer, uploadBufferAllocation, false);

		// Map the upload buffer into the CPU address space
		uint8_t* uploadBufferMapped;
		if (!memoryManager->MapBufferMemory(uploadBufferAllocation, uploadBufferMapped))
		{
			_log->Error("Failed to map upload buffer for index buffer");
			return false;
		}

		// Write the initial data into the upload buffer
		WriteDataToMappedBuffer(_initialDataEntryCount, _initialDataEntrySizeInBytes, _initialDataEntryStrideInBytes, _initialData, uploadBufferMapped);

		// Unmap the upload buffer
		memoryManager->UnmapBufferMemory(uploadBufferAllocation);

		// Allocate a new command buffer on the build queue to handle the initial data transfer
		VkCommandBuffer commandBuffer = _renderer->GetBuildCommandBuffer();

		// Schedule a data transfer from the upload buffer to the target buffer
		VkBufferCopy copyRegion = {};
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = 0;
		copyRegion.size = _totalBufferSizeInBytes;
		vkCmdCopyBuffer(commandBuffer, uploadBuffer, _indexBuffer, 1, &copyRegion);

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
			barrier.buffer = _indexBuffer;
			barrier.offset = 0;
			barrier.size = _totalBufferSizeInBytes;
			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);

			// Schedule an acquire operation on the graphics queue for the buffer
			_renderer->ScheduleGraphicsQueueAcquireOperation(this);
		}

		// Submit the command buffer
		_renderer->SubmitBuildCommandBuffer(commandBuffer);
	}

	// Release any resources related to the initial data
	_initialDataSet = false;
	_initialData = nullptr;

	// Add an alias for this index buffer if requested
	if (aliasAsTexelArray && !texelArray->AllocateAsAliasForBuffer(_indexBuffer, _totalBufferSizeInBytes))
	{
		_log->Error("Failed to allocate alias for index buffer.");
		return false;
	}

	// Flag that the buffer has been created
	_indexBufferCreated = true;
	return true;
}

//----------------------------------------------------------------------------------------
void VulkanIndexBuffer::ReleaseMemory()
{
	auto* memoryManager = _renderer->GetMemoryManager();
	if (_indexBufferCreated)
	{
		memoryManager->DestroyBuffer(_indexBuffer, _indexBufferAllocation);
	}
}

//----------------------------------------------------------------------------------------
bool VulkanIndexBuffer::IsAllocated()
{
	return _indexBufferCreated;
}

//----------------------------------------------------------------------------------------
// Binding methods
//----------------------------------------------------------------------------------------
SuccessToken VulkanIndexBuffer::BindIndexAttribute(IIndexAttribute& indexAttribute)
{
	// Verify that the array hasn't already been created
	if (_indexBufferCreated)
	{
		_log->Error("Attempted to bind a new index attribute after the index buffer has already been created.");
		return false;
	}

	// Verify that an index attribute hasn't already been bound
	if (_indexAttributeAdded)
	{
		_log->Error("Attempted to bind a new index attribute to a buffer which already has an index attribute defined. Index buffers currently only support a single index attribute.");
		return false;
	}

	// Verify that there's at least one entry defined in the supplied attribute
	if (indexAttribute.GetIndexCount() == 0)
	{
		_log->Error("Attempted to bind an index attribute with an index count of zero.");
		return false;
	}

	// Verify that the supplied attribute hasn't already been bound to a buffer
	if (indexAttribute.IsBoundToBuffer())
	{
		_log->Error("Attempted to bind an index attribute which has already been bound to another buffer.");
		return false;
	}

	// Bind the index attribute to the array
	AttachIndexAttributeToThisArray(indexAttribute, 0);

	// Define the attribute info
	IndexAttributeInfo attributeInfoEntry{};
	attributeInfoEntry.dataType = indexAttribute.GetDataType();
	attributeInfoEntry.dataTypeByteSize = IIndexAttribute::GetDataTypeByteSize(attributeInfoEntry.dataType);
	attributeInfoEntry.indexCount = indexAttribute.GetIndexCount();
	attributeInfoEntry.performanceHintCpu = indexAttribute.GetPerformanceHintCpu();
	attributeInfoEntry.performanceHintGpu = indexAttribute.GetPerformanceHintGpu();
	attributeInfoEntry.bufferOffsetInBytes = 0;
	attributeInfoEntry.bufferStartPosInBytes = 0;
	_indexAttributeInfo = attributeInfoEntry;
	_indexAttributeAdded = true;
	return true;
}

//----------------------------------------------------------------------------------------
SuccessToken VulkanIndexBuffer::BindIndexAttributeManualLayout(IIndexAttribute& indexAttribute, size_t bufferOffsetInBytes, size_t bufferStrideInBytes)
{
	// Ensure the supplied index attribute is being bound at offset 0, tightly packed.
	auto dataType = indexAttribute.GetDataType();
	auto calculatedStrideInBytes = IIndexAttribute::GetDataTypeByteSize(dataType);
	if ((bufferOffsetInBytes != 0) || (bufferStrideInBytes != calculatedStrideInBytes))
	{
		_log->Error("Attempted to bind an index attribute with offset {0} and stride {1}, but index buffers only currently support an offset of 0 and a tightly packed stride.", bufferOffsetInBytes, bufferStrideInBytes);
		return false;
	}

	// Bind the index attribute
	if (!BindIndexAttribute(indexAttribute))
	{
		return false;
	}
	_manualBufferLayout = true;
	return true;
}

//----------------------------------------------------------------------------------------
const VulkanIndexBuffer::IndexAttributeInfo* VulkanIndexBuffer::GetIndexAttributeInfo(size_t attributeIndex) const
{
	ASSERT(attributeIndex == 0);
	return &_indexAttributeInfo;
}

//----------------------------------------------------------------------------------------
// Data methods
//----------------------------------------------------------------------------------------
SuccessToken VulkanIndexBuffer::SetInitialData(const uint8_t* data, size_t entryCount, size_t entryStrideInBytes)
{
	// Validate the current state of the buffer
	if (_indexBufferCreated)
	{
		_log->Error("Attempted to set initial index data after the buffer has already been created.");
		return false;
	}
	if (_indexAttributeInfo.indexCount != entryCount)
	{
		_log->Error("Attempted to set initial index data with index count {0}, which is different to the defined index count of {1}.", entryCount, _indexAttributeInfo.indexCount);
		return false;
	}
	if (_initialDataSet)
	{
		_log->Error("Attempted to set initial index data when initial data has already been provided.");
		return false;
	}

	// Store the initial data for use when the buffer is allocated
	_initialData = data;
	_initialDataEntryCount = entryCount;
	_initialDataEntryStrideInBytes = entryStrideInBytes;
	_initialDataEntrySizeInBytes = _indexAttributeInfo.dataTypeByteSize;
	_initialDataSet = true;
	return true;
}

//----------------------------------------------------------------------------------------
SuccessToken VulkanIndexBuffer::SetRawInitialData(const uint8_t* data, size_t dataSizeInBytes)
{
	// Validate the current state of the buffer
	if (_indexBufferCreated)
	{
		_log->Error("Attempted to set initial raw index buffer data after the buffer has already been created.");
		return false;
	}
	if (_initialDataSet)
	{
		_log->Error("Attempted to set initial raw index buffer data when initial data has already been provided.");
		return false;
	}
	if (!_manualBufferLayout)
	{
		_log->Error("Attempted to set initial raw index buffer data when manual layout index attributes have not been bound.");
		return false;
	}

	// Store the initial data for use when the buffer is allocated
	_initialData = data;
	_initialDataEntryCount = dataSizeInBytes;
	_initialDataEntryStrideInBytes = 1;
	_initialDataEntrySizeInBytes = 1;
	_initialDataSet = true;
	return true;
}

//----------------------------------------------------------------------------------------
SuccessToken VulkanIndexBuffer::QueueDataUpdate(const uint8_t* data, size_t entryCount, size_t initialIndexNo, size_t entryStrideInBytes, ITransferBatch* transferBatch)
{
	// Ensure the target attribute is allowed to be modified
	IndexAttributeInfo& info = _indexAttributeInfo;
	if ((info.performanceHintCpu & IIndexAttribute::PerformanceHint::WriteFlagsMask) == IIndexAttribute::PerformanceHint::WriteNever)
	{
		_log->Error("Attempted to update an index attribute that can't be modified.");
		return false;
	}

	// Ensure the write region is within the bounds of the buffer
	if ((initialIndexNo >= info.indexCount) || ((initialIndexNo + entryCount) > info.indexCount))
	{
		_log->Error("Attempted index attribute write at index {0} for {1} entries, when only {2} entries are present.", initialIndexNo, entryCount, info.indexCount);
		return false;
	}

	// Capture the supplied update settings
	auto* transferBatchResolved = KnownDynamicCast<VulkanTransferBatch*>(transferBatch);
	size_t entrySizeInBytes = info.dataTypeByteSize;
	size_t uploadBufferSizeInBytes = entrySizeInBytes * entryCount;
	size_t bufferStartPosInBytes = info.bufferStartPosInBytes + (initialIndexNo * entrySizeInBytes);
	PendingWrite pendingWrite(info, entryCount, initialIndexNo, entryStrideInBytes);
	return QueueDataUpdateInternal(pendingWrite, data, uploadBufferSizeInBytes, bufferStartPosInBytes, transferBatchResolved);
}

//----------------------------------------------------------------------------------------
SuccessToken VulkanIndexBuffer::QueueRawDataUpdate(const uint8_t* data, size_t dataSizeInBytes, size_t bufferOffsetInBytes, ITransferBatch* transferBatch)
{
	// Validate the current state of the buffer
	if (!_indexBufferCreated)
	{
		_log->Error("Attempted to queue a raw data update before the index buffer has been allocated.");
		return false;
	}
	if (!_manualBufferLayout)
	{
		_log->Error("Attempted to queue a raw data update when manual layout index attributes have not been bound.");
		return false;
	}

	// Verify that the requested write is within the bounds of the buffer
	if ((bufferOffsetInBytes > _totalBufferSizeInBytes) || ((bufferOffsetInBytes + dataSizeInBytes) > _totalBufferSizeInBytes))
	{
		_log->Error("Attempted to perform a raw write outside the bounds of an index buffer, with write location {0}, byte size {1}, against buffer size of {2}.", bufferOffsetInBytes, dataSizeInBytes, _totalBufferSizeInBytes);
		return false;
	}

	// Capture the supplied update settings and data
	auto* transferBatchResolved = KnownDynamicCast<VulkanTransferBatch*>(transferBatch);
	PendingWrite pendingWrite(_indexAttributeInfo, bufferOffsetInBytes, dataSizeInBytes);
	return QueueDataUpdateInternal(pendingWrite, data, dataSizeInBytes, bufferOffsetInBytes, transferBatchResolved);
}

//----------------------------------------------------------------------------------------
bool VulkanIndexBuffer::QueueDataUpdateInternal(PendingWrite& pendingWrite, const uint8_t* data, size_t uploadBufferSizeInBytes, size_t bufferStartPosInBytes, VulkanTransferBatch* transferBatch)
{
	// If we're targeting a transfer batch, make sure it's not already submitted.
	if ((transferBatch != nullptr) && transferBatch->IsSubmitted())
	{
		_log->Error("Attempted to queue a transfer using a transfer batch that has already been submitted");
		return false;
	}

	// Create an upload buffer for the data
	VkBuffer uploadBuffer;
	VmaAllocation uploadBufferAllocation;
	if (transferBatch != nullptr)
	{
		transferBatch->CreateTemporaryTransferBuffer(uploadBufferSizeInBytes, uploadBuffer, uploadBufferAllocation);
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
		_log->Error("Failed to map upload buffer for index buffer");
		return false;
	}

	// Write the data into the upload buffer
	const IndexAttributeInfo& info = pendingWrite.attributeInfo;
	size_t entrySizeInBytes = (pendingWrite.rawDataWrite ? 1 : info.dataTypeByteSize);
	WriteDataToMappedBuffer(pendingWrite.entryCount, entrySizeInBytes, pendingWrite.entryStrideInBytes, data, uploadBufferMapped);
	pendingWrite.uploadBuffer = uploadBuffer;
	pendingWrite.targetBufferPos = bufferStartPosInBytes;
	pendingWrite.uploadBufferSizeInBytes = uploadBufferSizeInBytes;

	// Unmap the upload buffer
	memoryManager->UnmapBufferMemory(uploadBufferAllocation);

	// Queue a task to update the buffer with the supplied data
	if (transferBatch != nullptr)
	{
		bool separateTransferQueue = _renderer->IsTransferQueueSharedWithGraphics();
		if (separateTransferQueue)
		{
			transferBatch->AddInitializeOperation(this);
		}
		transferBatch->AddTransferOperation([this, pendingWrite = pendingWrite](VkCommandBuffer commandBuffer) { CompletePendingDataWrite(commandBuffer, pendingWrite); });
		if (separateTransferQueue)
		{
			transferBatch->AddFinalizeOperation(this);
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
void VulkanIndexBuffer::CompletePendingDataWrites(VkCommandBuffer commandBuffer)
{
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
void VulkanIndexBuffer::WriteDataToMappedBuffer(size_t entryCount, size_t entrySizeInBytes, size_t entryStrideInBytes, const uint8_t* data, uint8_t* mappedMemory)
{
	// Copy our data into the memory buffer
	if (entryStrideInBytes == entrySizeInBytes)
	{
		std::memcpy(mappedMemory, data, entryCount * entrySizeInBytes);
	}
	else
	{
		while (entryCount > 0)
		{
			std::memcpy(mappedMemory, data, entrySizeInBytes);
			mappedMemory += entrySizeInBytes;
			data += entryStrideInBytes;
			--entryCount;
		}
	}
}

//----------------------------------------------------------------------------------------
void VulkanIndexBuffer::CompletePendingDataWrite(VkCommandBuffer commandBuffer, const PendingWrite& pendingWrite)
{
	// Schedule a data transfer from the upload buffer to the target buffer
	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = pendingWrite.targetBufferPos;
	copyRegion.size = pendingWrite.uploadBufferSizeInBytes;
	vkCmdCopyBuffer(commandBuffer, pendingWrite.uploadBuffer, _indexBuffer, 1, &copyRegion);
}

//----------------------------------------------------------------------------------------
void VulkanIndexBuffer::PerformGraphicsQueueAcquireOperation(VkCommandBuffer commandBuffer)
{
	// Perform an acquire operation on the graphics queue for the buffer
	VkBufferMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_INDEX_READ_BIT;
	barrier.srcQueueFamilyIndex = _renderer->GetTransferQueueFamily();
	barrier.dstQueueFamilyIndex = _renderer->GetGraphicsQueueFamily();
	barrier.buffer = _indexBuffer;
	barrier.offset = 0;
	barrier.size = _totalBufferSizeInBytes;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);
}

//----------------------------------------------------------------------------------------
void VulkanIndexBuffer::PerformGraphicsQueueReleaseOperation(VkCommandBuffer commandBuffer)
{
	// Perform a release operation on the graphics queue for the buffer
	VkBufferMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	barrier.srcAccessMask = VK_ACCESS_INDEX_READ_BIT;
	barrier.dstAccessMask = 0;
	barrier.srcQueueFamilyIndex = _renderer->GetGraphicsQueueFamily();
	barrier.dstQueueFamilyIndex = _renderer->GetTransferQueueFamily();
	barrier.buffer = _indexBuffer;
	barrier.offset = 0;
	barrier.size = _totalBufferSizeInBytes;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);
}

//----------------------------------------------------------------------------------------
bool VulkanIndexBuffer::PerformTransferQueueAcquireOperation(VkCommandBuffer commandBuffer, bool canDiscardCurrentContent)
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
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.srcQueueFamilyIndex = _renderer->GetGraphicsQueueFamily();
	barrier.dstQueueFamilyIndex = _renderer->GetTransferQueueFamily();
	barrier.buffer = _indexBuffer;
	barrier.offset = 0;
	barrier.size = _totalBufferSizeInBytes;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);
	return true;
}

//----------------------------------------------------------------------------------------
bool VulkanIndexBuffer::PerformTransferQueueReleaseOperation(VkCommandBuffer commandBuffer)
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
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = 0;
	barrier.srcQueueFamilyIndex = _renderer->GetTransferQueueFamily();
	barrier.dstQueueFamilyIndex = _renderer->GetGraphicsQueueFamily();
	barrier.buffer = _indexBuffer;
	barrier.offset = 0;
	barrier.size = _totalBufferSizeInBytes;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);
	return true;
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
void VulkanIndexBuffer::MigrateBuildStateToDrawState()
{
	std::swap(_buildIndex, _drawIndex);
	_stateModified.clear(std::memory_order_relaxed);
}

//----------------------------------------------------------------------------------------
void VulkanIndexBuffer::FlagBuildStateModified()
{
	if (!_stateModified.test_and_set(std::memory_order_acquire))
	{
		_renderer->FlagObjectModified(this);
	}
}

//----------------------------------------------------------------------------------------
VkBuffer VulkanIndexBuffer::GetNativeBuffer() const
{
	return _indexBuffer;
}

} // namespace cobalt::graphics
