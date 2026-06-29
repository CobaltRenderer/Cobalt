// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "OpenGLStateBuffer.h"
#include "OpenGLDebug.h"
#include "OpenGLRenderer.h"
#include "OpenGLStateBufferLayout.h"
#include <Internal/RendererSupport/KnownDynamicCast.h>
#include <algorithm>
#include <cstdlib>
#include <cstring>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
OpenGLStateBuffer::OpenGLStateBuffer(cobalt::logging::ILogger* log, OpenGLRenderer* renderer)
: _log(log), _renderer(renderer), _performanceHintCpu(PerformanceHint::Default), _performanceHintGpu(PerformanceHint::Default), _pagesPerPageBlock(1), _pageBlockEntryMask(0x7FFFFFFF), _pageBlockShiftCount(31), _buildIndex(0), _drawIndex(1)
{
	_state[_buildIndex].pageCount = 1;
}

//----------------------------------------------------------------------------------------
OpenGLStateBuffer::~OpenGLStateBuffer()
{
	ReleaseMemory();
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void OpenGLStateBuffer::Delete()
{
	_renderer->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
SuccessToken OpenGLStateBuffer::AllocateMemory()
{
	// Validate the current state of the buffer
	if (!_stateBufferSizeSet || _memoryAllocated)
	{
		_log->Error("Attempted to allocate a state buffer which hasn't been sized or has already been allocated");
		return false;
	}

	// Determine the usage flag to use when defining the OpenGL buffer
	GLenum usageFlag;
	PerformanceHint cpuWriteFlags = _performanceHintCpu & PerformanceHint::WriteFlagsMask;
	switch (cpuWriteFlags)
	{
	case PerformanceHint::WriteNever:
		usageFlag = GL_STATIC_DRAW;
		break;
	default:
	case PerformanceHint::Default:
	case PerformanceHint::WriteRarely:
		usageFlag = GL_DYNAMIC_DRAW;
		break;
	case PerformanceHint::WriteOften:
		usageFlag = GL_STREAM_DRAW;
		break;
	}
	_usageFlag = usageFlag;

	// Ensure that the buffer is writable if dynamic resize has been requested
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
void OpenGLStateBuffer::ReleaseMemory()
{
	// Delete each page block
	for (auto& pageBlock : _pageBlocks)
	{
		// Delete the assigned buffers
		if (pageBlock.nativeBufferAllocated)
		{
			glDeleteBuffers(1, &pageBlock.bufferNo);
		}

		// Free the local memory buffer
#ifdef _WIN32
		_aligned_free(pageBlock.memoryBuffers[0]);
		_aligned_free(pageBlock.memoryBuffers[1]);
#else
		std::free(pageBlock.memoryBuffers[0]); // NOLINT(cppcoreguidelines-no-malloc,hicpp-no-malloc)
		std::free(pageBlock.memoryBuffers[1]); // NOLINT(cppcoreguidelines-no-malloc,hicpp-no-malloc)
#endif
	}
	_pageBlocks.clear();

	// Flag that memory is no longer allocated for this buffer
	_memoryAllocated = false;
}

//----------------------------------------------------------------------------------------
// Usage methods
//----------------------------------------------------------------------------------------
OpenGLStateBuffer::PerformanceHint OpenGLStateBuffer::GetPerformanceHintCpu() const
{
	return _performanceHintCpu;
}

//----------------------------------------------------------------------------------------
OpenGLStateBuffer::PerformanceHint OpenGLStateBuffer::GetPerformanceHintGpu() const
{
	return _performanceHintGpu;
}

//----------------------------------------------------------------------------------------
void OpenGLStateBuffer::SetPerformanceHints(PerformanceHint performanceHintCpu, PerformanceHint performanceHintGpu)
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
void OpenGLStateBuffer::SetManualPageSize(size_t pageSizeInBytes)
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
SuccessToken OpenGLStateBuffer::BindBufferLayout(IStateBufferLayout* stateBufferLayout)
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
	_stateBufferLayout = KnownDynamicCast<OpenGLStateBufferLayout*>(stateBufferLayout);
	return true;
}

//----------------------------------------------------------------------------------------
void OpenGLStateBuffer::SetPageSettings(uint32_t initialPageCount, bool allowDynamicResize)
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
		uint32_t uniformBufferOffsetAlignment = _renderer->GetUniformBufferOffsetAlignment();
		_pageSizeInBytes = ((_usedPageSizeInBytes + (uniformBufferOffsetAlignment - 1)) / uniformBufferOffsetAlignment) * uniformBufferOffsetAlignment;
	}
	else
	{
		_pageSizeInBytes = _usedPageSizeInBytes;
	}

	// Although not observed so far under OpenGL, some Vulkan driver implementations have no effective limit on state
	// buffer page size, and return extremely large numbers for the maxUniformBufferRange limit. In case some OpenGL
	// drivers do the same, to define a sane maximum, we additionally cap the page size at 256 kilobytes.
	auto maxPageBlockSizeInBytes = (size_t)_renderer->GetMaxUniformBlockSize();
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
		auto totalPagesPerBlock = (uint32_t)(maxPageBlockSizeInBytes / _pageSizeInBytes);
		auto highestBitNumber = GetHighestSetBitNumber(totalPagesPerBlock);
		totalPagesPerBlock = (1u << highestBitNumber);

		// Set the page block properties
		_pagesPerPageBlock = totalPagesPerBlock;
		_pageBlockEntryMask = _pagesPerPageBlock - 1;
		_pageBlockShiftCount = (uint32_t)highestBitNumber;
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
SuccessToken OpenGLStateBuffer::ResizePageCount(uint32_t pageCount)
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
		auto pageBlocksToAllocate = (pageCount + (_pagesPerPageBlock - 1)) / _pagesPerPageBlock;
		ResizePageBlockCount(pageBlocksToAllocate);
	}
	_state[_buildIndex].pageCount = pageCount;
	FlagBuildStateModified();
	return true;
}

//----------------------------------------------------------------------------------------
void OpenGLStateBuffer::ResizePageBlockCount(uint32_t pageBlockCount)
{
	// Allocate any new page blocks which are required
	for (auto i = (uint32_t)_pageBlocks.size(); i < pageBlockCount; ++i)
	{
		// Allocate a memory buffer to cache the state data. Note that we follow guidelines from NVidia here to align
		// the buffer to a 16 byte boundary, which reportedly can speed up memcpy operations when mapping the buffer.
		// See the following for more information:
		// https://developer.nvidia.com/content/constant-buffers-without-constant-pain-0
		PageBlock pageBlock = {};
		pageBlock.nativeBufferAllocated = false;
		pageBlock.hasUnsavedChanges[_buildIndex] = true;
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

		// Add this page block to the set of defined page blocks
		_pageBlocks.push_back(pageBlock);
	}
}

//----------------------------------------------------------------------------------------
uint32_t OpenGLStateBuffer::GetHighestSetBitNumber(uint32_t data)
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
bool OpenGLStateBuffer::BindStateBufferPage(GLuint bindingPoint, uint32_t pageNo)
{
	// Determine the page block index and page number within the block for the target page number
	auto pageIndexInBlock = pageNo & _pageBlockEntryMask;
	auto pageBlockIndex = pageNo >> _pageBlockShiftCount;
	if (pageNo >= _state[_drawIndex].pageCount)
	{
		_log->Error("Page number {0} was outside the page count of {1} when attempting to bind state buffer", pageNo, _state[_drawIndex].pageCount);
		return false;
	}

	// Bind the target page of this buffer to the specified binding point
	const PageBlock& pageBlock = _pageBlocks[pageBlockIndex];
	auto pageBlockOffset = pageIndexInBlock * _pageSizeInBytes;
	glBindBufferRange(GL_UNIFORM_BUFFER, bindingPoint, pageBlock.bufferNo, pageBlockOffset, _pageSizeInBytes);
	CheckGLError(_log);
	return true;
}

//----------------------------------------------------------------------------------------
// State value methods
//----------------------------------------------------------------------------------------
StateValueId OpenGLStateBuffer::GetStateValueId(const Marshal::In<std::string>& name) const
{
	if (_stateBufferLayout == nullptr)
	{
		_log->Error("Attempted to retrieve a state ID from a state buffer when no state buffer layout has been bound");
		return StateValueId::Null;
	}
	return _stateBufferLayout->GetFieldId(name);
}

//----------------------------------------------------------------------------------------
void OpenGLStateBuffer::SetRawPageData(uint32_t pageNo, const uint8_t* data, size_t dataSizeInBytes, size_t dataOffsetInBytes)
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
void OpenGLStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const size_t* arrayIndices, size_t arrayIndexCount, const uint8_t* data, size_t dataSizeInBytes)
{
	// Attempt to retrieve the target field entry from the state buffer layout
	if (_stateBufferLayout == nullptr)
	{
		_log->Error("Attempted to set a state value in a state buffer when no state buffer layout has been bound");
		return;
	}
	const OpenGLStateBufferLayout::FieldEntry* fieldEntry = _stateBufferLayout->GetFieldEntryInfo(stateId);
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
void OpenGLStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, bool value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	int val = (int)value;
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const uint8_t*>(&val), sizeof(val));
}

//----------------------------------------------------------------------------------------
void OpenGLStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V1Int8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const uint8_t*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void OpenGLStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V1Int16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const uint8_t*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void OpenGLStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V1Int32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const uint8_t*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void OpenGLStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V1UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const uint8_t*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void OpenGLStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V1UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const uint8_t*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void OpenGLStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V1UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const uint8_t*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void OpenGLStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V1Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const uint8_t*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void OpenGLStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V1Float64& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const uint8_t*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void OpenGLStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V2Int8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const uint8_t*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void OpenGLStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V2Int16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const uint8_t*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void OpenGLStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V2Int32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const uint8_t*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void OpenGLStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V2UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const uint8_t*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void OpenGLStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V2UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const uint8_t*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void OpenGLStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V2UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const uint8_t*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void OpenGLStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V2Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const uint8_t*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void OpenGLStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V2Float64& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const uint8_t*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void OpenGLStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V3Int8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const uint8_t*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void OpenGLStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V3Int16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const uint8_t*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void OpenGLStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V3Int32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const uint8_t*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void OpenGLStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V3UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const uint8_t*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void OpenGLStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V3UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const uint8_t*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void OpenGLStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V3UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const uint8_t*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void OpenGLStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V3Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const uint8_t*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void OpenGLStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V3Float64& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const uint8_t*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void OpenGLStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V4Int8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const uint8_t*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void OpenGLStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V4Int16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const uint8_t*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void OpenGLStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V4Int32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const uint8_t*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void OpenGLStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V4UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const uint8_t*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void OpenGLStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V4UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const uint8_t*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void OpenGLStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V4UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const uint8_t*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void OpenGLStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V4Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const uint8_t*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void OpenGLStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V4Float64& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const uint8_t*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void OpenGLStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const M2Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const uint8_t*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void OpenGLStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const M3Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const uint8_t*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
void OpenGLStateBuffer::SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const M4Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, arrayIndices, arrayIndexCount, reinterpret_cast<const uint8_t*>(value.data()), sizeof(value));
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
void OpenGLStateBuffer::MigrateBuildStateToDrawState()
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
void OpenGLStateBuffer::CompletePendingDataWrites()
{
	// Update the contents of each dirty page block
	for (auto& pageBlock : _pageBlocks)
	{
		CompletePendingDataWritesForPageBlock(pageBlock);
	}
}

//----------------------------------------------------------------------------------------
void OpenGLStateBuffer::CompletePendingDataWritesForPageBlock(uint32_t targetPageBlockNo)
{
	// Update the contents of the target page block if required
	CompletePendingDataWritesForPageBlock(_pageBlocks[targetPageBlockNo]);
}

//----------------------------------------------------------------------------------------
void OpenGLStateBuffer::CompletePendingDataWritesForPageBlock(PageBlock& pageBlock)
{
	// If this page block doesn't have unsaved changes, skip it.
	if (!pageBlock.hasUnsavedChanges[_drawIndex] && !pageBlock.nativeBufferAllocated)
	{
		return;
	}

	// Update the contents of this page block, allocating the buffer if necessary.
	if (!pageBlock.nativeBufferAllocated)
	{
		// Generate the buffer number
		glGenBuffers(1, &pageBlock.bufferNo);
		glBindBuffer(GL_UNIFORM_BUFFER, pageBlock.bufferNo);

		// Create the buffer
		glBufferData(GL_UNIFORM_BUFFER, _pageBlockSizeInBytes, pageBlock.memoryBuffers[_drawIndex], _usageFlag);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
		pageBlock.nativeBufferAllocated = true;
	}
	else if (pageBlock.hasUnsavedChanges[_drawIndex])
	{
		// Update the contents of this page block
		glBindBuffer(GL_UNIFORM_BUFFER, pageBlock.bufferNo);
		CheckGLError(_log);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, _pageBlockSizeInBytes, pageBlock.memoryBuffers[_drawIndex]);
		CheckGLError(_log);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
		CheckGLError(_log);
	}
	CheckGLError(_log);

	// Flag that there are no more unsaved changes in this page block
	pageBlock.hasUnsavedChanges[_drawIndex] = false;
}

//----------------------------------------------------------------------------------------
void OpenGLStateBuffer::FlagBuildStateModified()
{
	if (!_stateModified.test_and_set(std::memory_order_acquire))
	{
		_renderer->FlagObjectModified(this);
	}
}

} // namespace cobalt::graphics
