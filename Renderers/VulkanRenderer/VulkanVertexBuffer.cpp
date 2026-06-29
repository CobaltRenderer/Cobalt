// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "VulkanVertexBuffer.h"
#include "VulkanRenderer.h"
#include "VulkanTexelArray.h"
#include "VulkanTransferBatch.h"
#include <Internal/RendererSupport/KnownDynamicCast.h>
#include <cstring>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------------------
VulkanVertexBuffer::VulkanVertexBuffer(cobalt::logging::ILogger* log, VulkanRenderer* renderer)
: _log(log), _renderer(renderer), _buildIndex(0), _drawIndex(1)
{}

//----------------------------------------------------------------------------------------
VulkanVertexBuffer::~VulkanVertexBuffer()
{
	ReleaseMemory();
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void VulkanVertexBuffer::Delete()
{
	_renderer->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
SuccessToken VulkanVertexBuffer::AllocateMemory()
{
	return AllocateMemoryInternal(nullptr);
}

//----------------------------------------------------------------------------------------
SuccessToken VulkanVertexBuffer::AllocateMemoryWithAlias(ITexelArray* texelArray)
{
	auto* texelArrayResolved = KnownDynamicCast<VulkanTexelArray*>(texelArray);
	if (texelArrayResolved == nullptr)
	{
		_log->Error("Attempted to allocate memory for a vertex buffer with an alias, where no alias object was supplied.");
		return false;
	}
	return AllocateMemoryInternal(texelArrayResolved);
}

//----------------------------------------------------------------------------------------
bool VulkanVertexBuffer::AllocateMemoryInternal(VulkanTexelArray* texelArray)
{
	// Ensure the array hasn't already been created, and that at least one vertex attribute has been defined.
	if (_vertexBufferCreated)
	{
		_log->Error("Attempted to allocate memory for a vertex buffer that has already been allocated.");
		return false;
	}
	if (_vertexAttributeCount <= 0)
	{
		_log->Error("Attempted to allocate memory for a vertex buffer that contains no vertex attributes.");
		return false;
	}

	// Ensure that a manual memory layout has been defined if an alias is being used
	bool aliasAsTexelArray = (texelArray != nullptr);
	if (aliasAsTexelArray && !_manualBufferLayout)
	{
		_log->Error("Attempted to allocate memory for a vertex buffer with an alias, when a manual buffer layout was not defined. A manual layout must be used if an alias is present, or the memory structure cannot be known, and therefore cannot be shared in a well defined manner.");
		return false;
	}

	// Calculate the starting positions of each vertex attribute data block in the vertex buffer
	_bufferInterleaved = false;
	size_t minimumComponentAlignment = _renderer->GetMinVertexElementStride();
	const size_t minimumStartingAttributeAlignment = 4;
	size_t totalBufferSizeInBytes = 0;
	size_t totalPaddedBufferSizeInBytes = 0;
	if (_manualBufferLayout)
	{
		for (size_t i = 0; i < _vertexAttributeCount; ++i)
		{
			VertexAttributeInfo& attributeInfo = _vertexAttributeInfo[i];
			size_t entrySizeInBytes = attributeInfo.dataTypeByteSize * attributeInfo.elementCount;
			size_t bufferStartPosInBytes = attributeInfo.bufferStartPosInBytes;
			size_t bufferEndPosInBytes = attributeInfo.bufferStartPosInBytes + ((attributeInfo.vertexCount - 1) * attributeInfo.bufferStrideInBytes) + entrySizeInBytes;
			size_t bufferPaddingEndPosInBytes = attributeInfo.bufferStartPosInBytes + (attributeInfo.vertexCount * attributeInfo.bufferStrideInBytes);
			totalBufferSizeInBytes = std::max(totalBufferSizeInBytes, bufferEndPosInBytes);
			totalPaddedBufferSizeInBytes = std::max(totalPaddedBufferSizeInBytes, bufferPaddingEndPosInBytes);
			for (size_t j = i + 1; j < _vertexAttributeCount; ++j)
			{
				_bufferInterleaved |= (_vertexAttributeInfo[j].bufferStartPosInBytes >= bufferStartPosInBytes) && (_vertexAttributeInfo[j].bufferStartPosInBytes <= bufferEndPosInBytes);
			}
		}
		totalBufferSizeInBytes = ((totalBufferSizeInBytes + (minimumStartingAttributeAlignment - 1)) / minimumStartingAttributeAlignment) * minimumStartingAttributeAlignment;
		totalPaddedBufferSizeInBytes = ((totalPaddedBufferSizeInBytes + (minimumStartingAttributeAlignment - 1)) / minimumStartingAttributeAlignment) * minimumStartingAttributeAlignment;
	}
	else
	{
		// Set the offset for each vertex attribute in the sequential buffer
		size_t offset = 0;
		for (size_t i = 0; i < _vertexAttributeCount; ++i)
		{
			// Calculate the padding for this attribute at the start and end of each vertex entry, taking into account
			// alignment requirements. Note that all three component vertex attributes have a minimum four byte
			// alignment.
			VertexAttributeInfo& attributeInfo = _vertexAttributeInfo[i];
			auto alignmentRequirement = std::max((attributeInfo.elementCount == 3 ? minimumStartingAttributeAlignment : minimumComponentAlignment), attributeInfo.dataTypeByteSize);
			attributeInfo.vertexPaddingAtStart = 0;
			attributeInfo.vertexPaddingAtEnd = (alignmentRequirement - ((attributeInfo.elementCount * attributeInfo.dataTypeByteSize) % alignmentRequirement)) % alignmentRequirement;

			// Store information about the start position in the buffer for this attribute
			offset = offset + ((alignmentRequirement - (offset % alignmentRequirement)) % alignmentRequirement);
			attributeInfo.bufferOffsetInBytes = offset;
			attributeInfo.bufferStartPosInBytes = attributeInfo.bufferOffsetInBytes + attributeInfo.vertexPaddingAtStart;

			// Calculate the starting position of the next attribute in the linear vertex buffer, taking into account
			// any padding for this attribute. Note that we also align to a minimum of four bytes between sequential
			// attributes, as required by hardware.
			offset += (attributeInfo.vertexPaddingAtStart + (attributeInfo.elementCount * attributeInfo.dataTypeByteSize) + attributeInfo.vertexPaddingAtEnd) * attributeInfo.vertexCount;
			offset = ((offset + (minimumStartingAttributeAlignment - 1)) / minimumStartingAttributeAlignment) * minimumStartingAttributeAlignment;
		}

		// Calculate the stride between buffer entries for each vertex attribute
		for (size_t i = 0; i < _vertexAttributeCount; ++i)
		{
			VertexAttributeInfo& attributeInfo = _vertexAttributeInfo[i];
			size_t entrySizeInBytes = attributeInfo.dataTypeByteSize * attributeInfo.elementCount;
			size_t attributeVertexSizeInBytes = attributeInfo.vertexPaddingAtStart + entrySizeInBytes + attributeInfo.vertexPaddingAtEnd;
			attributeInfo.bufferStrideInBytes = attributeVertexSizeInBytes;
		}

		// Calculate the total size of the required buffer
		totalBufferSizeInBytes = offset;
		totalPaddedBufferSizeInBytes = offset;
	}
	size_t nonPaddedBufferSizeInBytes = totalBufferSizeInBytes;
	_totalBufferSizeInBytes = totalPaddedBufferSizeInBytes;

	// Check if any initial data has been supplied for the buffer. Note that we deliberately check against the
	// non-padded buffer size here. The caller shouldn't know or care that padding exists.
	bool initialDataProvided = !_initialDataBuffer.empty();
	if (!_initialDataBuffer.empty() && (_initialDataBuffer.size() != nonPaddedBufferSizeInBytes))
	{
		_log->Error("Raw initial vertex data of size {0} was provided, but {1} bytes are needed for the buffer.", _initialDataBuffer.size(), nonPaddedBufferSizeInBytes);
		return false;
	}
	for (size_t i = 0; i < _vertexAttributeCount; ++i)
	{
		VertexAttributeInfo& attributeInfo = _vertexAttributeInfo[i];
		initialDataProvided |= attributeInfo.initialDataSet;
	}

	// Create the vertex buffer
	VulkanMemoryManager* memoryManager = _renderer->GetMemoryManager();
	VkBufferUsageFlags usageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	if (aliasAsTexelArray)
	{
		usageFlags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
	}
	memoryManager->CreateBuffer(_totalBufferSizeInBytes, usageFlags, VMA_MEMORY_USAGE_GPU_ONLY, 0, _vertexBuffer, _vertexBufferAllocation);

	// If initial data has been provided, stage an upload of that data into the buffer.
	if (initialDataProvided)
	{
		// Create an upload buffer for the data
		VkBuffer uploadBuffer;
		VmaAllocation uploadBufferAllocation;
		_renderer->CreateTemporaryUploadBuffer(_totalBufferSizeInBytes, uploadBuffer, uploadBufferAllocation, false);

		// Map the upload buffer into the CPU address space
		uint8_t* uploadBufferMapped;
		if (!memoryManager->MapBufferMemory(uploadBufferAllocation, uploadBufferMapped))
		{
			_log->Error("Failed to map upload buffer for vertex buffer");
			return false;
		}

		// Write the initial data into the upload buffer. Note that in the case of a raw write, we use the non-padded
		// buffer size as the transfer length, which we validated above. This will leave the padding bytes, if any,
		// uninitialized, but we don't care what value they're set to.
		if (!_initialDataBuffer.empty())
		{
			PendingWrite pendingWrite(_vertexAttributeInfo.front(), 0, nonPaddedBufferSizeInBytes);
			WriteDataToMappedBuffer(pendingWrite, _initialDataBuffer.data(), uploadBufferMapped);
		}
		else
		{
			for (size_t i = 0; i < _vertexAttributeCount; ++i)
			{
				VertexAttributeInfo& attributeInfo = _vertexAttributeInfo[i];
				if (!attributeInfo.initialDataSet)
				{
					continue;
				}
				PendingWrite pendingWrite(attributeInfo, attributeInfo.vertexCount, 0, attributeInfo.initialDataEntryStrideInBytes);
				WriteDataToMappedBuffer(pendingWrite, attributeInfo.initialData, (uploadBufferMapped + attributeInfo.bufferStartPosInBytes));
			}
		}

		// Unmap the upload buffer
		memoryManager->UnmapBufferMemory(uploadBufferAllocation);

		// Allocate a new command buffer on the build queue to handle the initial data transfer
		VkCommandBuffer commandBuffer = _renderer->GetBuildCommandBuffer();

		// Schedule a data transfer from the upload buffer to the target buffer
		VkBufferCopy copyRegion = {};
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = 0;
		copyRegion.size = _totalBufferSizeInBytes;
		vkCmdCopyBuffer(commandBuffer, uploadBuffer, _vertexBuffer, 1, &copyRegion);

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
			barrier.buffer = _vertexBuffer;
			barrier.offset = 0;
			barrier.size = _totalBufferSizeInBytes;
			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);

			// Schedule an acquire operation on the graphics queue for the buffer
			_renderer->ScheduleGraphicsQueueAcquireOperation(this);
		}

		// Submit the command buffer
		_renderer->SubmitBuildCommandBuffer(commandBuffer);
	}

	// Add an alias for this vertex buffer if requested
	if (aliasAsTexelArray && !texelArray->AllocateAsAliasForBuffer(_vertexBuffer, _totalBufferSizeInBytes))
	{
		_log->Error("Failed to allocate alias for vertex buffer.");
		return false;
	}

	// Flag that the buffer has been created
	_vertexBufferCreated = true;
	return true;
}

//----------------------------------------------------------------------------------------
void VulkanVertexBuffer::ReleaseMemory()
{
	auto* memoryManager = _renderer->GetMemoryManager();
	if (_vertexBufferCreated)
	{
		memoryManager->DestroyBuffer(_vertexBuffer, _vertexBufferAllocation);
	}
}

//----------------------------------------------------------------------------------------
// Binding methods
//----------------------------------------------------------------------------------------
SuccessToken VulkanVertexBuffer::BindVertexAttribute(IVertexAttribute& vertexAttribute)
{
	return BindVertexAttributeInternal(vertexAttribute, false, 0, 0);
}

//----------------------------------------------------------------------------------------
SuccessToken VulkanVertexBuffer::BindVertexAttributeManualLayout(IVertexAttribute& vertexAttribute, size_t bufferOffsetInBytes, size_t bufferStrideInBytes)
{
	return BindVertexAttributeInternal(vertexAttribute, true, bufferOffsetInBytes, bufferStrideInBytes);
}

//----------------------------------------------------------------------------------------
bool VulkanVertexBuffer::BindVertexAttributeInternal(IVertexAttribute& vertexAttribute, bool manualLayout, size_t bufferOffsetInBytes, size_t bufferStrideInBytes)
{
	// Verify that the buffer hasn't already been created
	if (_vertexBufferCreated)
	{
		_log->Error("Attempted to bind a new vertex attribute after the vertex buffer has already been created.");
		return false;
	}

	// Verify that there's at least one entry defined in the supplied attribute
	if (vertexAttribute.GetVertexCount() == 0)
	{
		_log->Error("Attempted to bind a vertex attribute with a vertex count of zero.");
		return false;
	}

	// Verify that the supplied attribute hasn't already been bound to a buffer
	if (vertexAttribute.IsBoundToBuffer())
	{
		_log->Error("Attempted to bind a vertex attribute which has already been bound to another buffer.");
		return false;
	}

	// Ensure that manual and auto layout vertex attributes aren't being mixed on the one buffer
	if ((_vertexAttributeCount > 0) && (_manualBufferLayout != manualLayout))
	{
		_log->Error("Attempted to bind a mix of auto and manual vertex attributes to the one buffer.");
		return false;
	}

	// Bind the vertex attribute to the array
	AttachVertexAttributeToThisArray(vertexAttribute, _vertexAttributeCount);

	// Define the attribute info
	VertexAttributeInfo attributeInfoEntry{};
	attributeInfoEntry.dataType = vertexAttribute.GetDataType();
	attributeInfoEntry.dataTypeByteSize = IVertexAttribute::GetDataTypeByteSize(attributeInfoEntry.dataType);
	attributeInfoEntry.elementCount = vertexAttribute.GetAttributeElementCount();
	attributeInfoEntry.vertexCount = vertexAttribute.GetVertexCount();
	attributeInfoEntry.performanceHintCpu = vertexAttribute.GetPerformanceHintCpu();
	attributeInfoEntry.performanceHintGpu = vertexAttribute.GetPerformanceHintGpu();
	attributeInfoEntry.initialDataSet = false;
	attributeInfoEntry.initialData = nullptr;
	attributeInfoEntry.bufferOffsetInBytes = bufferOffsetInBytes;
	attributeInfoEntry.bufferStrideInBytes = bufferStrideInBytes;
	if (manualLayout)
	{
		size_t entrySizeInBytes = attributeInfoEntry.dataTypeByteSize * attributeInfoEntry.elementCount;
		attributeInfoEntry.vertexPaddingAtStart = 0;
		attributeInfoEntry.vertexPaddingAtEnd = std::max(bufferStrideInBytes, entrySizeInBytes) - std::min(bufferStrideInBytes, entrySizeInBytes);
		attributeInfoEntry.bufferStartPosInBytes = bufferOffsetInBytes;
	}
	_vertexAttributeInfo.push_back(attributeInfoEntry);

	// Update the attribute count
	_vertexAttributeCount++;
	_manualBufferLayout = manualLayout;
	return true;
}

//----------------------------------------------------------------------------------------
bool VulkanVertexBuffer::IsAllocated()
{
	return _vertexBufferCreated;
}

//----------------------------------------------------------------------------------------
const VulkanVertexBuffer::VertexAttributeInfo* VulkanVertexBuffer::GetVertexAttributeInfo(size_t attributeIndex)
{
	// Note that the caller is responsible here for verifying the buffer has been allocated prior to calling this
	// method. If this is the case we can get away with taking this array member pointer here, as the attribute set
	// can't be changed after the allocation process.
	ASSERT(attributeIndex < _vertexAttributeCount);
	return &_vertexAttributeInfo[attributeIndex];
}

//----------------------------------------------------------------------------------------
// Data methods
//----------------------------------------------------------------------------------------
SuccessToken VulkanVertexBuffer::SetInitialData(size_t attributeIndex, const uint8_t* data, size_t entryCount, size_t entryStrideInBytes)
{
	// Validate the current state of the buffer
	VertexAttributeInfo& info = _vertexAttributeInfo[attributeIndex];
	if (_vertexBufferCreated)
	{
		_log->Error("Attempted to set initial vertex data after the buffer has already been created.");
		return false;
	}
	if (info.vertexCount != entryCount)
	{
		_log->Error("Attempted to set initial vertex data with vertex count {0}, which is different to the defined vertex count of {1}.", entryCount, info.vertexCount);
		return false;
	}
	if (info.initialDataSet || _rawInitialDataSet)
	{
		_log->Error("Attempted to set initial vertex data when initial data has already been provided.");
		return false;
	}

	// Store the initial data for use when the buffer is allocated
	info.initialData = data;
	info.initialDataEntryStrideInBytes = entryStrideInBytes;
	info.initialDataSet = true;
	return true;
}

//----------------------------------------------------------------------------------------
SuccessToken VulkanVertexBuffer::SetRawInitialData(const uint8_t* data, size_t dataSizeInBytes)
{
	// Validate the current state of the buffer
	if (_vertexBufferCreated)
	{
		_log->Error("Attempted to set initial raw vertex buffer data after the buffer has already been created.");
		return false;
	}
	if (_rawInitialDataSet)
	{
		_log->Error("Attempted to set initial raw vertex buffer data when initial data has already been provided.");
		return false;
	}
	if (!_manualBufferLayout)
	{
		_log->Error("Attempted to set initial raw vertex buffer data when manual layout vertex attributes have not been bound.");
		return false;
	}
	for (size_t i = 0; i < _vertexAttributeCount; ++i)
	{
		VertexAttributeInfo& attributeInfo = _vertexAttributeInfo[i];
		if (attributeInfo.initialDataSet)
		{
			_log->Error("Attempted to set initial raw vertex buffer data when initial data has already been provided.");
			return false;
		}
	}

	// Store the initial data for use when the buffer is allocated
	_rawInitialDataSet = true;
	_initialDataBuffer.assign(data, data + dataSizeInBytes);
	return true;
}

//----------------------------------------------------------------------------------------
SuccessToken VulkanVertexBuffer::QueueDataUpdate(size_t attributeIndex, const uint8_t* data, size_t entryCount, size_t initialVertexNo, size_t entryStrideInBytes, ITransferBatch* transferBatch)
{
	// Ensure the target attribute is allowed to be modified
	const VertexAttributeInfo& info = _vertexAttributeInfo[attributeIndex];
	if ((info.performanceHintCpu & IVertexAttribute::PerformanceHint::WriteFlagsMask) == IVertexAttribute::PerformanceHint::WriteNever)
	{
		_log->Error("Attempted to update a vertex attribute that can't be modified.");
		return false;
	}

	// Ensure the write region is within the bounds of the buffer
	if ((initialVertexNo >= info.vertexCount) || ((initialVertexNo + entryCount) > info.vertexCount))
	{
		_log->Error("Attempted vertex attribute write at index {0} for {1} entries, when only {2} entries are present.", initialVertexNo, entryCount, info.vertexCount);
		return false;
	}

	// Capture the supplied update settings
	auto* transferBatchResolved = KnownDynamicCast<VulkanTransferBatch*>(transferBatch);
	size_t entrySizeInBytes = info.dataTypeByteSize * info.elementCount;
	size_t uploadBufferSizeInBytes = entrySizeInBytes * entryCount;
	size_t attributeVertexSizeInBytes = info.vertexPaddingAtStart + entrySizeInBytes + info.vertexPaddingAtEnd;
	size_t bufferStartPosInBytes = info.bufferStartPosInBytes + (initialVertexNo * attributeVertexSizeInBytes);
	PendingWrite pendingWrite(info, entryCount, initialVertexNo, entryStrideInBytes);
	return QueueDataUpdateInternal(pendingWrite, data, uploadBufferSizeInBytes, bufferStartPosInBytes, transferBatchResolved);
}

//----------------------------------------------------------------------------------------
SuccessToken VulkanVertexBuffer::QueueRawDataUpdate(const uint8_t* data, size_t dataSizeInBytes, size_t bufferOffsetInBytes, ITransferBatch* transferBatch)
{
	// Validate the current state of the buffer
	if (!_vertexBufferCreated)
	{
		_log->Error("Attempted to queue a raw data update before the vertex buffer has been allocated.");
		return false;
	}
	if (!_manualBufferLayout)
	{
		_log->Error("Attempted to queue a raw data update when manual layout vertex attributes have not been bound.");
		return false;
	}

	// Verify that the requested write is within the bounds of the buffer
	if ((bufferOffsetInBytes > _totalBufferSizeInBytes) || ((bufferOffsetInBytes + dataSizeInBytes) > _totalBufferSizeInBytes))
	{
		_log->Error("Attempted to perform a raw write outside the bounds of a vertex buffer, with write location {0}, byte size {1}, against buffer size of {2}.", bufferOffsetInBytes, dataSizeInBytes, _totalBufferSizeInBytes);
		return false;
	}

	// Capture the supplied update settings
	auto* transferBatchResolved = KnownDynamicCast<VulkanTransferBatch*>(transferBatch);
	PendingWrite pendingWrite(_vertexAttributeInfo.front(), bufferOffsetInBytes, dataSizeInBytes);
	return QueueDataUpdateInternal(pendingWrite, data, dataSizeInBytes, bufferOffsetInBytes, transferBatchResolved);
}

//----------------------------------------------------------------------------------------
bool VulkanVertexBuffer::QueueDataUpdateInternal(PendingWrite& pendingWrite, const uint8_t* data, size_t uploadBufferSizeInBytes, size_t bufferStartPosInBytes, VulkanTransferBatch* transferBatch)
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
		_log->Error("Failed to map upload buffer for vertex buffer");
		return false;
	}

	// Write the data into the upload buffer
	WriteDataToMappedBuffer(pendingWrite, data, uploadBufferMapped);
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
void VulkanVertexBuffer::CompletePendingDataWrites(VkCommandBuffer commandBuffer)
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
void VulkanVertexBuffer::WriteDataToMappedBuffer(const PendingWrite& pendingWrite, const uint8_t* data, uint8_t* mappedMemory)
{
	// Retrieve info on the pending write
	const VertexAttributeInfo& info = pendingWrite.attributeInfo;
	size_t entryCount = pendingWrite.entryCount;
	size_t entryStrideInBytes = pendingWrite.entryStrideInBytes;
	size_t entrySizeInBytes = info.dataTypeByteSize * info.elementCount;
	size_t attributeVertexSizeInBytes = info.vertexPaddingAtStart + entrySizeInBytes + info.vertexPaddingAtEnd;
	size_t bufferStrideInBytes = attributeVertexSizeInBytes;

	// Copy our data into the memory buffer
	if (pendingWrite.rawDataWrite || (!_bufferInterleaved && (bufferStrideInBytes == entryStrideInBytes)))
	{
		std::memcpy(mappedMemory, data, entryCount * entryStrideInBytes);
	}
	else
	{
		while (entryCount > 0)
		{
			std::memcpy(mappedMemory, data, entrySizeInBytes);
			mappedMemory += bufferStrideInBytes;
			data += entryStrideInBytes;
			--entryCount;
		}
	}
}

//----------------------------------------------------------------------------------------
void VulkanVertexBuffer::CompletePendingDataWrite(VkCommandBuffer commandBuffer, const PendingWrite& pendingWrite)
{
	// Schedule a data transfer from the upload buffer to the target buffer
	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = pendingWrite.targetBufferPos;
	copyRegion.size = pendingWrite.uploadBufferSizeInBytes;
	vkCmdCopyBuffer(commandBuffer, pendingWrite.uploadBuffer, _vertexBuffer, 1, &copyRegion);
}

//----------------------------------------------------------------------------------------
void VulkanVertexBuffer::PerformGraphicsQueueAcquireOperation(VkCommandBuffer commandBuffer)
{
	// Perform an acquire operation on the graphics queue for the buffer
	VkBufferMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
	barrier.srcQueueFamilyIndex = _renderer->GetTransferQueueFamily();
	barrier.dstQueueFamilyIndex = _renderer->GetGraphicsQueueFamily();
	barrier.buffer = _vertexBuffer;
	barrier.offset = 0;
	barrier.size = _totalBufferSizeInBytes;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);
}

//----------------------------------------------------------------------------------------
void VulkanVertexBuffer::PerformGraphicsQueueReleaseOperation(VkCommandBuffer commandBuffer)
{
	// Perform a release operation on the graphics queue for the buffer
	VkBufferMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	barrier.srcAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
	barrier.dstAccessMask = 0;
	barrier.srcQueueFamilyIndex = _renderer->GetGraphicsQueueFamily();
	barrier.dstQueueFamilyIndex = _renderer->GetTransferQueueFamily();
	barrier.buffer = _vertexBuffer;
	barrier.offset = 0;
	barrier.size = _totalBufferSizeInBytes;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);
}

//----------------------------------------------------------------------------------------
bool VulkanVertexBuffer::PerformTransferQueueAcquireOperation(VkCommandBuffer commandBuffer, bool canDiscardCurrentContent)
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
	barrier.buffer = _vertexBuffer;
	barrier.offset = 0;
	barrier.size = _totalBufferSizeInBytes;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);
	return true;
}

//----------------------------------------------------------------------------------------
bool VulkanVertexBuffer::PerformTransferQueueReleaseOperation(VkCommandBuffer commandBuffer)
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
	barrier.buffer = _vertexBuffer;
	barrier.offset = 0;
	barrier.size = _totalBufferSizeInBytes;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);
	return true;
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
void VulkanVertexBuffer::FlagBuildStateModified()
{
	if (!_stateModified.test_and_set(std::memory_order_acquire))
	{
		_renderer->FlagObjectModified(this);
	}
}

//----------------------------------------------------------------------------------------
void VulkanVertexBuffer::MigrateBuildStateToDrawState()
{
	std::swap(_buildIndex, _drawIndex);
	_stateModified.clear(std::memory_order_relaxed);
}

//----------------------------------------------------------------------------------------
VkBuffer VulkanVertexBuffer::GetNativeBuffer()
{
	return _vertexBuffer;
}

} // namespace cobalt::graphics
