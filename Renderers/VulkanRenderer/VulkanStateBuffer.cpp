// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "VulkanStateBuffer.h"
#include "VulkanRenderer.h"
#include "VulkanStateBufferLayout.h"
#include <Internal/RendererSupport/KnownDynamicCast.h>
#include <cstdlib>
#include <cstring>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
VulkanStateBuffer::VulkanStateBuffer(cobalt::logging::ILogger* log, VulkanRenderer* renderer, bool isolatedBuffer)
: _log(log), _renderer(renderer), _performanceHintCpu(PerformanceHint::Default), _performanceHintGpu(PerformanceHint::Default), _pagesPerPageBlock(1), _pageBlockEntryMask(0x7FFFFFFF), _pageBlockShiftCount(31), _buildIndex(0), _drawIndex(1)
{
	_state[_buildIndex].pageCount = 1;

	if (isolatedBuffer)
	{
		_stateModified.test_and_set(std::memory_order_acquire);
		_drawIndex = _buildIndex;
	}
}

//----------------------------------------------------------------------------------------
VulkanStateBuffer::~VulkanStateBuffer()
{
	ReleaseMemory();
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void VulkanStateBuffer::Delete()
{
	_renderer->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
SuccessToken VulkanStateBuffer::AllocateMemory()
{
	// Validate the current state of the buffer
	if (!_stateBufferSizeSet || _memoryAllocated)
	{
		_log->Error("Attempted to allocate a state buffer which hasn't been sized or has already been allocated");
		return false;
	}

	// Ensure that the buffer is writable if dynamic resize has been requested
	PerformanceHint cpuWriteFlags = _performanceHintCpu & PerformanceHint::WriteFlagsMask;
	if ((cpuWriteFlags == PerformanceHint::WriteNever) && _allowDynamicResize)
	{
		_log->Error("Cannot create a state buffer which doesn't allow CPU writes and supports dynamic resize");
		return false;
	}

	// Flag that memory has now been allocated, and return true to the caller.
	_memoryAllocated = true;
	FlagBuildStateModified();
	return true;
}

//----------------------------------------------------------------------------------------
void VulkanStateBuffer::ReleaseMemory()
{
	auto* memoryManager = _renderer->GetMemoryManager();

	// Delete each page block
	for (auto& pageBlock : _pageBlocks)
	{
		// Free the local memory buffer
#ifdef _WIN32
		_aligned_free(pageBlock.memoryBuffers[0]);
		_aligned_free(pageBlock.memoryBuffers[1]);
#else
		std::free(pageBlock.memoryBuffers[0]); // NOLINT(cppcoreguidelines-no-malloc,hicpp-no-malloc)
		std::free(pageBlock.memoryBuffers[1]); // NOLINT(cppcoreguidelines-no-malloc,hicpp-no-malloc)
#endif

		// Free device memory
		if (pageBlock.nativeBufferAllocated)
		{
			memoryManager->DestroyBuffer(pageBlock.nativeBuffer, pageBlock.nativeBufferAllocation);
			memoryManager->DestroyBuffer(pageBlock.stagingBuffer, pageBlock.stagingBufferAllocation);
		}
	}
	_pageBlocks.clear();

	// Flag that memory is no longer allocated for this buffer
	_memoryAllocated = false;
}

//----------------------------------------------------------------------------------------
// Usage methods
//----------------------------------------------------------------------------------------
IStateBuffer::PerformanceHint VulkanStateBuffer::GetPerformanceHintCpu() const
{
	return _performanceHintCpu;
}

//----------------------------------------------------------------------------------------
IStateBuffer::PerformanceHint VulkanStateBuffer::GetPerformanceHintGpu() const
{
	return _performanceHintGpu;
}

//----------------------------------------------------------------------------------------
void VulkanStateBuffer::SetPerformanceHints(PerformanceHint performanceHintCpu, PerformanceHint performanceHintGpu)
{
	// Validate the current state of the buffer
	if (_memoryAllocated)
	{
		_log->Error("Attempted to modify initial state buffer settings after it has been constructed");
		return;
	}

	// Update the performance hints for the state buffer
	_performanceHintCpu = performanceHintCpu;
	_performanceHintGpu = performanceHintGpu;
}

//----------------------------------------------------------------------------------------
// Format methods
//----------------------------------------------------------------------------------------
void VulkanStateBuffer::SetManualPageSize(size_t pageSizeInBytes)
{
	// Validate the current state of the buffer
	if (_memoryAllocated)
	{
		_log->Error("Attempted to modify initial state buffer settings after it has been constructed");
		return;
	}
	if (_stateBufferLayout != nullptr)
	{
		_log->Error("Attempted to set a state buffer manual page size after a state buffer layout has been bound");
		return;
	}
	if (_manualPageSizeSpecified)
	{
		_log->Error("Attempted to set a state buffer manual page size more than once");
		return;
	}

	// Record the specified manual page size
	_manualPageSizeSpecified = true;
	_manualPageSizeInBytes = pageSizeInBytes;
}

//----------------------------------------------------------------------------------------
SuccessToken VulkanStateBuffer::BindBufferLayout(IStateBufferLayout* stateBufferLayout)
{
	// Validate the current state of the buffer
	if (_memoryAllocated)
	{
		_log->Error("Attempted to modify initial state buffer settings after it has been constructed");
		return false;
	}
	if (_stateBufferLayout != nullptr)
	{
		_log->Error("Attempted to set the state buffer layout mode than once");
		return false;
	}
	if (_manualPageSizeSpecified)
	{
		_log->Error("Attempted to bind a state buffer layout after a manual page size has been defined");
		return false;
	}

	// Bind to the target state buffer layout
	_stateBufferLayout = KnownDynamicCast<VulkanStateBufferLayout*>(stateBufferLayout);
	return true;
}

//----------------------------------------------------------------------------------------
void VulkanStateBuffer::SetPageSettings(uint32_t initialPageCount, bool allowDynamicResize)
{
	// Validate the current state of the buffer
	if (_memoryAllocated)
	{
		_log->Error("Attempted to modify initial state buffer settings after it has been constructed");
		return;
	}
	if (_stateBufferSizeSet)
	{
		_log->Error("Attempted to set the size of a state buffer more than once");
		return;
	}
	if ((_stateBufferLayout == nullptr) && !_manualPageSizeSpecified)
	{
		_log->Error("Attempted to set state buffer page settings before the buffer layout has been defined");
		return;
	}

	// Set the page settings for this uniform buffer
	_state[_buildIndex].pageCount = initialPageCount;
	_allowDynamicResize = allowDynamicResize;

	// Calculate the size of a page in the buffer
	_usedPageSizeInBytes = (_manualPageSizeSpecified ? _manualPageSizeInBytes : _stateBufferLayout->GetTotalLayoutSizeInBytes());
	if (_allowDynamicResize || (_state[_buildIndex].pageCount > 1))
	{
		size_t uniformBufferOffsetAlignment = ConstantBufferOffsetAlignment;
		_pageSizeInBytes = ((_usedPageSizeInBytes + (uniformBufferOffsetAlignment - 1)) / uniformBufferOffsetAlignment) * uniformBufferOffsetAlignment;
	}
	else
	{
		size_t uniformBufferSizeAlignment = ConstantBufferSizeAlignment;
		_pageSizeInBytes = ((_usedPageSizeInBytes + (uniformBufferSizeAlignment - 1)) / uniformBufferSizeAlignment) * uniformBufferSizeAlignment;
	}

	// Some Vulkan driver implementations have no effective limit on state buffer page size, and return extremely large
	// numbers for the maxUniformBufferRange limit. To define a sane maximum, we additionally cap the page size at 256
	// kilobytes.
	size_t maxPageBlockSizeInBytes = _renderer->GetPhysicalDeviceProperties().limits.maxUniformBufferRange;
	maxPageBlockSizeInBytes = std::min(maxPageBlockSizeInBytes, std::max(_pageSizeInBytes, (size_t)(256 * 1024)));

	// Ensure the state buffer page size is less than the maximum
	if (_pageSizeInBytes > maxPageBlockSizeInBytes)
	{
		_log->Error("Required state buffer page size of {0} is greater than the maximum supported size of {1}.", _pageSizeInBytes, maxPageBlockSizeInBytes);
		return;
	}

	// Calculate the size of a page block, and the number of page blocks to allocate.
	uint32_t pageBlocksToAllocate;
	if (!_allowDynamicResize)
	{
		_pagesPerPageBlock = _state[_buildIndex].pageCount;
		_pageBlockEntryMask = 0x7FFFFFFF;
		_pageBlockShiftCount = 31;
		pageBlocksToAllocate = 1;
	}
	else
	{
		// Determine the number of pages to assign per block, keeping below the maximum block size. Note that we round
		// down to the nearest power of two here, to make page lookup more efficient. Knowing that we have a page count
		// per block which is a power of two, we can use simple bit shifts and masks to convert from a page number of a
		// page block number, avoiding an expensive divide operation.
		auto totalPagesPerBlock = maxPageBlockSizeInBytes / _pageSizeInBytes;
		auto highestBitNumber = GetHighestSetBitNumber(totalPagesPerBlock);
		totalPagesPerBlock = (size_t)(1ull << highestBitNumber);

		// Set the page block properties
		_pagesPerPageBlock = (uint32_t)totalPagesPerBlock;
		_pageBlockEntryMask = _pagesPerPageBlock - 1;
		_pageBlockShiftCount = highestBitNumber;
		pageBlocksToAllocate = (_state[_buildIndex].pageCount + (_pagesPerPageBlock - 1)) / _pagesPerPageBlock;
	}
	_pageBlockSizeInBytes = _pageSizeInBytes * _pagesPerPageBlock;

	// If dynamic resize is supported, reserve an appropriate number of pages in the page block array to speed up
	// additions later.
	if (allowDynamicResize)
	{
		_pageBlocks.reserve(((_state[_buildIndex].pageCount * 5) + (_pagesPerPageBlock - 1)) / _pagesPerPageBlock);
	}

	// Create the required number of page blocks
	ResizePageBlockCount(pageBlocksToAllocate);
	_stateBufferSizeSet = true;
}

//----------------------------------------------------------------------------------------
SuccessToken VulkanStateBuffer::ResizePageCount(uint32_t pageCount)
{
	// Validate the current state of the buffer
	if (!_allowDynamicResize || !_memoryAllocated)
	{
		_log->Error("Attempted to resize a state buffer which hasn't been constructed or doesn't allow resize");
		return false;
	}

	// Update the page count, increasing the page block count if required.
	if (pageCount > _state[_buildIndex].pageCount)
	{
		uint32_t pageBlocksToAllocate = (pageCount + (_pagesPerPageBlock - 1)) / _pagesPerPageBlock;
		ResizePageBlockCount(pageBlocksToAllocate);
	}
	_state[_buildIndex].pageCount = pageCount;
	FlagBuildStateModified();
	return true;
}

//----------------------------------------------------------------------------------------
void VulkanStateBuffer::ResizePageBlockCount(uint32_t pageBlockCount)
{
	// Allocate any new page blocks which are required
	for (auto i = (uint32_t)_pageBlocks.size(); i < pageBlockCount; ++i)
	{
		// Allocate a memory buffer to cache the state data. Note that we follow guidelines from NVidia here to align
		// the buffer to a 16 byte boundary, which reportedly can speed up memcpy operations when mapping the buffer.
		// See the following for more information:
		// https://developer.nvidia.com/content/constant-buffers-without-constant-pain-0
		PageBlock pageBlock = {};
		pageBlock.nativeBufferAllocated = true;
		pageBlock.hasUnsavedChanges[_buildIndex] = false;
#ifdef _WIN32
		pageBlock.memoryBuffers[0] = reinterpret_cast<uint8_t*>(_aligned_malloc(_pageBlockSizeInBytes, 16));
		pageBlock.memoryBuffers[1] = reinterpret_cast<uint8_t*>(_aligned_malloc(_pageBlockSizeInBytes, 16));
#else
		(void)posix_memalign(reinterpret_cast<void**>(&pageBlock.memoryBuffers[0]), 16, _pageBlockSizeInBytes);
		(void)posix_memalign(reinterpret_cast<void**>(&pageBlock.memoryBuffers[1]), 16, _pageBlockSizeInBytes);
#endif

		// Initialize the memory buffers to zero. This ensures a consistent initial value for buffer contents, even if
		// they haven't been explicitly set.
		WARNINGS_PUSH_OFF
#ifdef _MSC_VER
#pragma warning(disable : 6387)
#endif
		std::memset(pageBlock.memoryBuffers[0], 0, _pageBlockSizeInBytes);
		std::memset(pageBlock.memoryBuffers[1], 0, _pageBlockSizeInBytes);
		WARNINGS_POP

		// Create a staging buffer and state buffer in GPU memory
		VulkanMemoryManager* memoryManager = _renderer->GetMemoryManager();
		if (!memoryManager->CreateBuffer(_pageBlockSizeInBytes, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY, 0, pageBlock.nativeBuffer, pageBlock.nativeBufferAllocation))
		{
			_log->Error("Failed to allocate state buffer with page block size {0}", _pageBlockSizeInBytes);
			return;
		}
		if (!memoryManager->CreateMappedBuffer(_pageBlockSizeInBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, VMA_ALLOCATION_CREATE_MAPPED_BIT, pageBlock.stagingBuffer, pageBlock.stagingBufferAllocation, pageBlock.stagingBufferPointer))
		{
			_log->Error("Failed to allocate state staging buffer with page block size {0}", _pageBlockSizeInBytes);
			return;
		}
		pageBlock.hasUnsavedChanges[_drawIndex] = true;

		// Add this page block to the set of defined page blocks
		_pageBlocks.push_back(pageBlock);
	}
}

//----------------------------------------------------------------------------------------
uint32_t VulkanStateBuffer::GetHighestSetBitNumber(size_t data)
{
	uint32_t highestBit = 0;
	while ((data >>= 1) > 0)
	{
		++highestBit;
	}
	return highestBit;
}

//----------------------------------------------------------------------------------------
// Binding methods
//----------------------------------------------------------------------------------------
VkBuffer VulkanStateBuffer::GetNativeBuffer(uint32_t pageNo)
{
	if (pageNo >= _state[_drawIndex].pageCount)
	{
		_log->Error("Page number {0} was outside the page count of {1} when attempting to bind state buffer", pageNo, _state[_drawIndex].pageCount);
		return VK_NULL_HANDLE;
	}

	size_t pageBlockIndex = pageNo >> _pageBlockShiftCount;
	return _pageBlocks[pageBlockIndex].nativeBuffer;
}

//----------------------------------------------------------------------------------------
size_t VulkanStateBuffer::GetPageOffset(uint32_t pageNo)
{
	return _pageSizeInBytes * (pageNo & _pageBlockEntryMask);
}

//----------------------------------------------------------------------------------------
size_t VulkanStateBuffer::GetPageSize()
{
	return _pageSizeInBytes;
}

//----------------------------------------------------------------------------------------
// State value methods
//----------------------------------------------------------------------------------------
StateValueId VulkanStateBuffer::GetStateValueId(const Marshal::In<std::string>& name) const
{
	if (_stateBufferLayout == nullptr)
	{
		_log->Error("Attempted to retrieve a state ID from a state buffer when no state buffer layout has been bound");
		return StateValueId::Null;
	}
	return StateValueId(_stateBufferLayout->GetFieldId(name));
}

//----------------------------------------------------------------------------------------
void VulkanStateBuffer::SetRawPageDataWithReturnedPageIndex(uint32_t pageNo, const unsigned char* data, uint32_t& pageIndexInBlock, uint32_t& pageBlockIndex)
{
	// Determine the page block index and page number within the block for the target page number
	pageIndexInBlock = pageNo & _pageBlockEntryMask;
	pageBlockIndex = pageNo >> _pageBlockShiftCount;
	if (pageNo >= _state[_buildIndex].pageCount)
	{
		_log->Error("Page number {0} was outside the page count of {1} when attempting to update state buffer", pageNo, _state[_buildIndex].pageCount);
		return;
	}

	// Perform the write to the target page
	PageBlock& pageBlock = _pageBlocks[pageBlockIndex];
	unsigned char* pageBuffer = pageBlock.memoryBuffers[_buildIndex] + (pageIndexInBlock * _pageSizeInBytes);
	std::memcpy(pageBuffer, data, _usedPageSizeInBytes);

	// Flag that the written page block has unsaved changes
	pageBlock.hasUnsavedChanges[_buildIndex] = true;
}

//----------------------------------------------------------------------------------------
void VulkanStateBuffer::SetRawPageData(uint32_t pageNo, const uint8_t* data, size_t dataSizeInBytes, size_t dataOffsetInBytes)
{
	// Determine the page block index and page number within the block for the target page number
	auto pageIndexInBlock = pageNo & _pageBlockEntryMask;
	auto pageBlockIndex = pageNo >> _pageBlockShiftCount;
	if (pageNo >= _state[_buildIndex].pageCount)
	{
		_log->Error("Page number {0} was outside the page count of {1} when attempting to update state buffer", pageNo, _state[_buildIndex].pageCount);
		return;
	}

	// Verify that the requested write is within the bounds of the buffer
	if ((dataOffsetInBytes > _pageSizeInBytes) || ((dataOffsetInBytes + dataSizeInBytes) > _pageSizeInBytes))
	{
		_log->Error("Attempted to perform a raw write outside the bounds of a state buffer page, with write location {0}, byte size {1}, against buffer size of {2}.", dataOffsetInBytes, dataSizeInBytes, _pageSizeInBytes);
		return;
	}

	// Perform the write to the target page
	PageBlock& pageBlock = _pageBlocks[pageBlockIndex];
	uint8_t* pageBuffer = pageBlock.memoryBuffers[_buildIndex] + (pageIndexInBlock * _pageSizeInBytes);
	uint8_t* writeTarget = pageBuffer + dataOffsetInBytes;
	std::memcpy(writeTarget, data, dataSizeInBytes);

	// Flag that the written page block has unsaved changes
	if (!pageBlock.hasUnsavedChanges[_buildIndex])
	{
		pageBlock.hasUnsavedChanges[_buildIndex] = true;
		FlagBuildStateModified();
	}
}

//----------------------------------------------------------------------------------------
void VulkanStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const size_t* arrayIndices, size_t arrayIndexCount, const uint8_t* data, size_t dataSizeInBytes)
{
	// Attempt to retrieve the target field entry from the state buffer layout
	if (_stateBufferLayout == nullptr)
	{
		_log->Error("Attempted to set a state value in a state buffer when no state buffer layout has been bound");
		return;
	}
	const VulkanStateBufferLayout::FieldEntry* fieldEntry = _stateBufferLayout->GetFieldEntryInfo(stateId);
	if (fieldEntry == nullptr)
	{
		_log->Error("Failed to find uniform entry for ID {0} when attempting to update state buffer", stateId);
		return;
	}

	// Determine the page block index and page number within the block for the target page number
	auto pageIndexInBlock = pageNo & _pageBlockEntryMask;
	auto pageBlockIndex = pageNo >> _pageBlockShiftCount;
	if (pageNo >= _state[_buildIndex].pageCount)
	{
		_log->Error("Page number {0} was outside the page count of {1} when attempting to update state buffer", pageNo, _state[_buildIndex].pageCount);
		return;
	}

	// Validate the array indices, and calculate the array buffer offset.
	size_t arrayOffset = 0;
	size_t bufferArrayDimensions = fieldEntry->arraySizes.size();
	if (arrayIndexCount != bufferArrayDimensions)
	{
		_log->Error("Attempted to modify state buffer variable {0} with {1} array index values, when {2} indices are required.", fieldEntry->name, arrayIndexCount, bufferArrayDimensions);
		return;
	}
	for (size_t i = 0; i < bufferArrayDimensions; ++i)
	{
		size_t arrayIndex = arrayIndices[i];
		if (arrayIndex >= fieldEntry->arraySizes[i])
		{
			_log->Error("Attempted to modify state buffer variable {0} at index {1}, when only {2} entries are present in the array.", fieldEntry->name, arrayIndex, fieldEntry->arraySizes[i]);
			return;
		}
		arrayOffset += arrayIndex * fieldEntry->arrayStridesInBytes[i];
	}

	// Validate that the write being attempted is within the bounds of the page
	size_t writeEndPos = (fieldEntry->bufferOffsetInBytes + arrayOffset + dataSizeInBytes);
	if (writeEndPos > _pageBlockSizeInBytes)
	{
		_log->Error("Write end position {0} is outside the page size of {1} when attempting to update state buffer", writeEndPos, _pageBlockSizeInBytes);
		return;
	}

	// Perform the write to the target page
	PageBlock& pageBlock = _pageBlocks[pageBlockIndex];
	uint8_t* pageBuffer = pageBlock.memoryBuffers[_buildIndex] + (pageIndexInBlock * _pageSizeInBytes);
	uint8_t* writeTarget = pageBuffer + fieldEntry->bufferOffsetInBytes + arrayOffset;
	auto sizeToCopy = std::min(dataSizeInBytes, fieldEntry->totalFieldEntryByteSize);
	std::memcpy(writeTarget, data, sizeToCopy);

	// Flag that the written page block has unsaved changes
	if (!pageBlock.hasUnsavedChanges[_buildIndex])
	{
		pageBlock.hasUnsavedChanges[_buildIndex] = true;
		FlagBuildStateModified();
	}
}

//----------------------------------------------------------------------------------------
void VulkanStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, bool value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	int val = (int)value;
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<unsigned char*>(&val), sizeof(val));
}

//----------------------------------------------------------------------------------------
void VulkanStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V1Int8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const unsigned char*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void VulkanStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V1Int16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const unsigned char*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void VulkanStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V1Int32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const unsigned char*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void VulkanStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V1UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const unsigned char*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void VulkanStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V1UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const unsigned char*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void VulkanStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V1UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const unsigned char*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void VulkanStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V1Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const unsigned char*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void VulkanStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V1Float64& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const unsigned char*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void VulkanStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V2Int8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const unsigned char*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void VulkanStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V2Int16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const unsigned char*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void VulkanStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V2Int32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const unsigned char*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void VulkanStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V2UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const unsigned char*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void VulkanStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V2UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const unsigned char*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void VulkanStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V2UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const unsigned char*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void VulkanStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V2Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const unsigned char*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void VulkanStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V2Float64& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const unsigned char*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void VulkanStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V3Int8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const unsigned char*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void VulkanStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V3Int16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const unsigned char*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void VulkanStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V3Int32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const unsigned char*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void VulkanStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V3UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const unsigned char*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void VulkanStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V3UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const unsigned char*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void VulkanStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V3UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const unsigned char*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void VulkanStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V3Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const unsigned char*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void VulkanStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V3Float64& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const unsigned char*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void VulkanStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V4Int8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const unsigned char*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void VulkanStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V4Int16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const unsigned char*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void VulkanStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V4Int32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const unsigned char*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void VulkanStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V4UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const unsigned char*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void VulkanStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V4UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const unsigned char*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void VulkanStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V4UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const unsigned char*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void VulkanStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V4Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const unsigned char*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void VulkanStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V4Float64& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const unsigned char*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void VulkanStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const M2Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const unsigned char*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void VulkanStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const M3Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const unsigned char*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void VulkanStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const M4Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const unsigned char*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
void VulkanStateBuffer::MigrateBuildStateToDrawState()
{
	_state[_drawIndex] = _state[_buildIndex];
	std::swap(_buildIndex, _drawIndex);
	_stateModified.clear(std::memory_order_relaxed);

	// Copy page block data with new changes back into the new build buffers
	for (auto& pageBlock : _pageBlocks)
	{
		if (pageBlock.hasUnsavedChanges[_drawIndex])
		{
			std::memcpy(pageBlock.memoryBuffers[_buildIndex], pageBlock.memoryBuffers[_drawIndex], _pageBlockSizeInBytes);
			pageBlock.hasUnsavedChanges[_buildIndex] = false;
		}
	}
}

//----------------------------------------------------------------------------------------
uint32_t VulkanStateBuffer::GetCurrentPageBlockCount() const
{
	return (uint32_t)_pageBlocks.size();
}

//----------------------------------------------------------------------------------------
uint32_t VulkanStateBuffer::GetPagesPerPageBlock() const
{
	return _pagesPerPageBlock;
}

//----------------------------------------------------------------------------------------
void VulkanStateBuffer::CompletePendingDataWrites(VkCommandBuffer commandBuffer)
{
	// Update the contents of each dirty page block
	for (auto& pageBlock : _pageBlocks)
	{
		CompletePendingDataWritesForPageBlock(commandBuffer, pageBlock);
	}
}

//----------------------------------------------------------------------------------------
void VulkanStateBuffer::CompletePendingDataWritesForPageBlock(VkCommandBuffer commandBuffer, uint32_t targetPageBlockNo)
{
	// Update the contents of the target page block if required
	CompletePendingDataWritesForPageBlock(commandBuffer, _pageBlocks[targetPageBlockNo]);
}

//----------------------------------------------------------------------------------------
void VulkanStateBuffer::CompletePendingDataWritesForPageBlock(VkCommandBuffer commandBuffer, PageBlock& pageBlock)
{
	// If this page block doesn't have unsaved changes, skip it.
	if (!pageBlock.hasUnsavedChanges[_drawIndex] && pageBlock.nativeBufferAllocated)
	{
		return;
	}

	// Copy to staging buffer and then queue copy to state buffer
	if (pageBlock.hasUnsavedChanges[_drawIndex])
	{
		std::memcpy(pageBlock.stagingBufferPointer, pageBlock.memoryBuffers[_drawIndex], _pageBlockSizeInBytes);
		auto* memoryManager = _renderer->GetMemoryManager();
		memoryManager->RecordCopyBuffer(commandBuffer, pageBlock.stagingBuffer, pageBlock.nativeBuffer, _pageBlockSizeInBytes);
	}

	// Flag that there are no more unsaved changes in this page block
	pageBlock.hasUnsavedChanges[_drawIndex] = false;
}

//----------------------------------------------------------------------------------------
void VulkanStateBuffer::FlagBuildStateModified()
{
	if (!_stateModified.test_and_set(std::memory_order_acquire))
	{
		_renderer->FlagObjectModified(this);
	}
}

} // namespace cobalt::graphics
