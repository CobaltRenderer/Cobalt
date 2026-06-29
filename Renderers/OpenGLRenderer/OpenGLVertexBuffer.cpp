// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "OpenGLVertexBuffer.h"
#include "OpenGLDebug.h"
#include "OpenGLRenderer.h"
#include "OpenGLTransferBatch.h"
#ifdef GL_VERSION_4_3
#include "OpenGLTexelArray.h"
#endif
#include <Internal/RendererSupport/KnownDynamicCast.h>
#include <algorithm>
#include <cstring>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
OpenGLVertexBuffer::OpenGLVertexBuffer(cobalt::logging::ILogger* log, OpenGLRenderer* renderer)
: _log(log), _renderer(renderer), _vertexBufferCreated(false), _nativeBufferCreationPending(false), _vertexAttributeCount(0), _interleavedVertexSizeInBytes(0), _buildIndex(0), _drawIndex(1)
{}

//----------------------------------------------------------------------------------------
OpenGLVertexBuffer::~OpenGLVertexBuffer()
{
	ReleaseMemory();
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void OpenGLVertexBuffer::Delete()
{
	_renderer->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
SuccessToken OpenGLVertexBuffer::AllocateMemory()
{
	return AllocateMemoryInternal(nullptr);
}

//----------------------------------------------------------------------------------------
SuccessToken OpenGLVertexBuffer::AllocateMemoryWithAlias(ITexelArray* texelArray)
{
#ifdef GL_VERSION_4_3
	auto* texelArrayResolved = KnownDynamicCast<OpenGLTexelArray*>(texelArray);
	if (texelArrayResolved == nullptr)
	{
		_log->Error("Attempted to allocate memory for a vertex buffer with an alias, where no alias object was supplied.");
		return false;
	}
	return AllocateMemoryInternal(texelArrayResolved);
#else
	return false;
#endif
}

//----------------------------------------------------------------------------------------
bool OpenGLVertexBuffer::AllocateMemoryInternal(OpenGLTexelArray* texelArray)
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
#ifdef GL_VERSION_4_3
	bool aliasAsTexelArray = (texelArray != nullptr);
	if (aliasAsTexelArray && !_manualBufferLayout)
	{
		_log->Error("Attempted to allocate memory for a vertex buffer with an alias, when a manual buffer layout was not defined. A manual layout must be used if an alias is present, or the memory structure cannot be known, and therefore cannot be shared in a well defined manner.");
		return false;
	}
#endif

	// Determine the vertex count to use if an interleaved buffer is created
	//bool sameVertexCountForAllAttributes = true;
	auto interleavedBufferVertexCount = _vertexAttributeInfo[0].vertexCount;
	for (size_t i = 1; i < _vertexAttributeCount; ++i)
	{
		size_t attributeVertexCount = _vertexAttributeInfo[i].vertexCount;
		//sameVertexCountForAllAttributes &= interleavedBufferVertexCount == attributeVertexCount;
		interleavedBufferVertexCount = (attributeVertexCount > interleavedBufferVertexCount) ? attributeVertexCount : interleavedBufferVertexCount;
	}

	// Combine the performance hint flags that are of interest between the bound attributes
	bool cpuWritesFrequent = false;
	bool gpuWritesFrequent = false;
	bool invalidateDataAfterDraw = false;
	for (size_t i = 0; i < _vertexAttributeCount; ++i)
	{
		bool cpuWritesFrequentForAttribute = ((_vertexAttributeInfo[i].performanceHintCpu & IVertexAttribute::PerformanceHint::WriteFlagsMask) == IVertexAttribute::PerformanceHint::WriteOften);
		bool gpuWritesFrequentForAttribute = ((_vertexAttributeInfo[i].performanceHintGpu & IVertexAttribute::PerformanceHint::WriteFlagsMask) == IVertexAttribute::PerformanceHint::WriteOften);
		bool invalidateDataAfterDrawForAttribute = ((_vertexAttributeInfo[i].persistenceFlags & IVertexAttribute::DataPersistenceFlags::InvalidateExistingDataAfterDrawComplete) != IVertexAttribute::DataPersistenceFlags(0));
		cpuWritesFrequent |= cpuWritesFrequentForAttribute;
		gpuWritesFrequent |= gpuWritesFrequentForAttribute;
		invalidateDataAfterDraw |= invalidateDataAfterDrawForAttribute;
	}

	// Determine the usage flag to use when defining the OpenGL buffer
	GLenum usageFlag;
	if (invalidateDataAfterDraw)
	{
		// If we're not preserving the data in this attribute after use, by definition its contents must be updated
		// before each usage, so we should be using STREAM_DRAW.
		usageFlag = GL_STREAM_DRAW;
	}
	else if (cpuWritesFrequent || gpuWritesFrequent)
	{
		// If we're frequently modifying this buffer from either the CPU or the GPU, use GL_DYNAMIC_DRAW.
		usageFlag = GL_DYNAMIC_DRAW;
	}
	else
	{
		// Seldom or never modifying the buffer. Note that modifying a static buffer is explicitly allowed in the OpenGL
		// spec, and profiling shows it is the best setting for "almost never" updated buffers. We also use this setting
		// for buffers which specify the default write performance hints, because typically geometry is expected to be
		// redrawn without changing between frames more often than not.
		usageFlag = GL_STATIC_DRAW;
	}
	_usageFlag = usageFlag;

	// Determine if an interleaved memory buffer should be used for the vertex attributes
	//##TODO## Interleaved buffers are painfully slow to update when we don't map the buffer, and the gains from
	// interleaving are questionable to start with. It's been suggested interleaving is either not helpful or is
	// actually harmful on modern hardware. As we couldn't measure benefits from doing it, and it hurts update
	// performance for non-static data, we've disabled it here. Determine the future of interleaved buffer support, such
	// as whether it should be retained, and if so, under what circumstances it should be employed.
	//_bufferInterleaved = (_vertexAttributeCount > 1) && sameVertexCountForAllAttributes && samePerformanceHintForAllAttributes;
	_bufferInterleaved = false;

	// Calculate the starting positions of each vertex attribute data block in the vertex buffer. We ensure a minimum
	// alignment of 4 bytes between vertex attributes, as per the defined OpenGL Vertex Specification Best Practices:
	// https://www.khronos.org/opengl/wiki/Vertex_Specification_Best_Practices
	const size_t minimumComponentAlignment = (_bufferInterleaved ? 4 : 1);
	const size_t minimumStartingAttributeAlignment = 4;
	size_t totalBufferSizeInBytes = 0;
	if (_manualBufferLayout)
	{
		for (size_t i = 0; i < _vertexAttributeCount; ++i)
		{
			VertexAttributeInfo& attributeInfo = _vertexAttributeInfo[i];
			size_t entrySizeInBytes = attributeInfo.dataTypeByteSize * attributeInfo.elementCount;
			size_t bufferStartPosInBytes = attributeInfo.bufferStartPosInBytes;
			size_t bufferEndPosInBytes = attributeInfo.bufferStartPosInBytes + ((attributeInfo.vertexCount - 1) * attributeInfo.bufferStrideInBytes) + entrySizeInBytes;
			totalBufferSizeInBytes = std::max(totalBufferSizeInBytes, bufferEndPosInBytes);
			for (size_t j = i + 1; j < _vertexAttributeCount; ++j)
			{
				_bufferInterleaved |= (_vertexAttributeInfo[j].bufferStartPosInBytes >= bufferStartPosInBytes) && (_vertexAttributeInfo[j].bufferStartPosInBytes <= bufferEndPosInBytes);
			}
		}
		totalBufferSizeInBytes = ((totalBufferSizeInBytes + (minimumStartingAttributeAlignment - 1)) / minimumStartingAttributeAlignment) * minimumStartingAttributeAlignment;
	}
	else if (_bufferInterleaved)
	{
		// Set the offset for each vertex attribute in the interleaved buffer
		size_t offset = 0;
		for (size_t i = 0; i < _vertexAttributeCount; ++i)
		{
			// Calculate the padding for this attribute at the start and end of each vertex entry, taking into account
			// alignment requirements. Note that all three component vertex attributes have a minimum four byte
			// alignment.
			VertexAttributeInfo& attributeInfo = _vertexAttributeInfo[i];
			auto alignmentRequirement = std::max((attributeInfo.elementCount == 3 ? minimumStartingAttributeAlignment : minimumComponentAlignment), attributeInfo.dataTypeByteSize);
			attributeInfo.vertexPaddingAtStart = (alignmentRequirement - (offset % alignmentRequirement)) % alignmentRequirement;
			attributeInfo.vertexPaddingAtEnd = (alignmentRequirement - ((attributeInfo.elementCount * attributeInfo.dataTypeByteSize) % alignmentRequirement)) % alignmentRequirement;

			// Store information about the start position in the buffer for this attribute
			attributeInfo.bufferOffsetInBytes = offset;
			attributeInfo.bufferStartPosInBytes = attributeInfo.bufferOffsetInBytes + attributeInfo.vertexPaddingAtStart;

			// Calculate the starting position of the next attribute in the interleaved vertex buffer, taking into
			// account any padding for this attribute.
			offset += attributeInfo.vertexPaddingAtStart + (attributeInfo.elementCount * attributeInfo.dataTypeByteSize) + attributeInfo.vertexPaddingAtEnd;
		}

		// Calculate and store the total size of a single interleaved vertex entry, taking into account alignment
		// requirements.
		_interleavedVertexSizeInBytes = offset + ((minimumComponentAlignment - (offset % minimumComponentAlignment)) % minimumComponentAlignment);

		// Calculate the stride between buffer entries for each vertex attribute
		for (size_t i = 0; i < _vertexAttributeCount; ++i)
		{
			VertexAttributeInfo& attributeInfo = _vertexAttributeInfo[i];
			attributeInfo.bufferStrideInBytes = _interleavedVertexSizeInBytes;
		}

		// Calculate the total size of the required buffer
		totalBufferSizeInBytes = _interleavedVertexSizeInBytes * interleavedBufferVertexCount;
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
			auto entrySizeInBytes = attributeInfo.dataTypeByteSize * attributeInfo.elementCount;
			auto attributeVertexSizeInBytes = attributeInfo.vertexPaddingAtStart + entrySizeInBytes + attributeInfo.vertexPaddingAtEnd;
			attributeInfo.bufferStrideInBytes = attributeVertexSizeInBytes;
		}

		// Calculate the total size of the required buffer
		totalBufferSizeInBytes = offset;
	}
	_totalBufferSizeInBytes = totalBufferSizeInBytes;

	// Obtain any initial data for the buffer
	if (_rawInitialDataSet && (_initialDataBuffer.size() != _totalBufferSizeInBytes))
	{
		_log->Error("Raw initial vertex data of size {0} was provided, but {1} bytes are needed for the buffer.", _initialDataBuffer.size(), _totalBufferSizeInBytes);
		return false;
	}
	if (!_rawInitialDataSet)
	{
		bool initialDataProvided = false;
		for (size_t i = 0; i < _vertexAttributeCount; ++i)
		{
			VertexAttributeInfo& attributeInfo = _vertexAttributeInfo[i];
			initialDataProvided |= attributeInfo.initialDataSet;
		}
		if (initialDataProvided)
		{
			// Build a combined buffer with the initial data to load into the memory region
			_initialDataBuffer.resize(totalBufferSizeInBytes);
			uint8_t* allocatedBuffer = _initialDataBuffer.data();
			for (size_t i = 0; i < _vertexAttributeCount; ++i)
			{
				VertexAttributeInfo& attributeInfo = _vertexAttributeInfo[i];
				if (!attributeInfo.initialDataSet)
				{
					continue;
				}

				size_t entrySizeInBytes = attributeInfo.dataTypeByteSize * attributeInfo.elementCount;
				size_t bufferStrideInBytes = attributeInfo.bufferStrideInBytes;
				const uint8_t* sourceEntryPos = attributeInfo.initialData;
				uint8_t* targetEntryPos = allocatedBuffer + attributeInfo.bufferStartPosInBytes;
				size_t entryCount = attributeInfo.vertexCount;
				if (!_bufferInterleaved && (attributeInfo.initialDataEntryStrideInBytes == bufferStrideInBytes))
				{
					std::memcpy(targetEntryPos, sourceEntryPos, bufferStrideInBytes * entryCount);
				}
				else
				{
					while (entryCount > 0)
					{
						std::memcpy(targetEntryPos, sourceEntryPos, entrySizeInBytes);
						targetEntryPos += attributeInfo.bufferStrideInBytes;
						sourceEntryPos += attributeInfo.initialDataEntryStrideInBytes;
						--entryCount;
					}
				}
			}
		}
	}

	// Release any resources related to the initial data
	for (size_t i = 0; i < _vertexAttributeCount; ++i)
	{
		VertexAttributeInfo& attributeInfo = _vertexAttributeInfo[i];
		attributeInfo.initialDataSet = false;
		attributeInfo.initialData = nullptr;
	}

#ifdef GL_VERSION_4_3
	// Add an alias for this vertex buffer if requested
	if (aliasAsTexelArray && !texelArray->AllocateAsAliasForBuffer(this, _totalBufferSizeInBytes))
	{
		_log->Error("Failed to allocate alias for vertex buffer.");
		return false;
	}
#endif

	// Flag that the vertex buffer has been created
	_vertexBufferCreated = true;
	_nativeBufferCreationPending = true;
	FlagBuildStateModified();
	return true;
}

//----------------------------------------------------------------------------------------
void OpenGLVertexBuffer::ReleaseMemory()
{
	if (_vertexBufferCreated && !_nativeBufferCreationPending)
	{
		glDeleteBuffers(1, &_openGLBufferNo);
		CheckGLError(_log);
	}
	_vertexBufferCreated = false;
}

//----------------------------------------------------------------------------------------
bool OpenGLVertexBuffer::IsAllocated() const
{
	return _vertexBufferCreated;
}

//----------------------------------------------------------------------------------------
// Binding methods
//----------------------------------------------------------------------------------------
SuccessToken OpenGLVertexBuffer::BindVertexAttribute(IVertexAttribute& vertexAttribute)
{
	return BindVertexAttributeInternal(vertexAttribute, false, 0, 0);
}

//----------------------------------------------------------------------------------------
SuccessToken OpenGLVertexBuffer::BindVertexAttributeManualLayout(IVertexAttribute& vertexAttribute, size_t bufferOffsetInBytes, size_t bufferStrideInBytes)
{
	return BindVertexAttributeInternal(vertexAttribute, true, bufferOffsetInBytes, bufferStrideInBytes);
}

//----------------------------------------------------------------------------------------
bool OpenGLVertexBuffer::BindVertexAttributeInternal(IVertexAttribute& vertexAttribute, bool manualLayout, size_t bufferOffsetInBytes, size_t bufferStrideInBytes)
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
	VertexAttributeInfo attributeInfoEntry = {};
	attributeInfoEntry.dataType = vertexAttribute.GetDataType();
	attributeInfoEntry.dataTypeByteSize = IVertexAttribute::GetDataTypeByteSize(attributeInfoEntry.dataType);
	attributeInfoEntry.elementCount = vertexAttribute.GetAttributeElementCount();
	attributeInfoEntry.vertexCount = vertexAttribute.GetVertexCount();
	attributeInfoEntry.performanceHintCpu = vertexAttribute.GetPerformanceHintCpu();
	attributeInfoEntry.performanceHintGpu = vertexAttribute.GetPerformanceHintGpu();
	attributeInfoEntry.persistenceFlags = vertexAttribute.GetDataPersistenceFlags();
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
	++_vertexAttributeCount;
	_manualBufferLayout = manualLayout;
	return true;
}

//----------------------------------------------------------------------------------------
const OpenGLVertexBuffer::VertexAttributeInfo* OpenGLVertexBuffer::GetVertexAttributeInfo(size_t attributeIndex) const
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
SuccessToken OpenGLVertexBuffer::SetInitialData(size_t attributeIndex, const uint8_t* data, size_t entryCount, size_t entryStrideInBytes)
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
SuccessToken OpenGLVertexBuffer::SetRawInitialData(const uint8_t* data, size_t dataSizeInBytes)
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
SuccessToken OpenGLVertexBuffer::QueueDataUpdate(size_t attributeIndex, const uint8_t* data, size_t entryCount, size_t initialVertexNo, size_t entryStrideInBytes, ITransferBatch* transferBatch)
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

	// If a transfer batch has been supplied, ensure it hasn't already been submitted.
	auto* transferBatchResolved = KnownDynamicCast<OpenGLTransferBatch*>(transferBatch);
	if ((transferBatchResolved != nullptr) && transferBatchResolved->IsSubmitted())
	{
		_log->Error("Attempted to queue a transfer using a transfer batch that has already been submitted");
		return false;
	}

	// Capture the supplied update settings and data
	size_t entrySizeInBytes = info.dataTypeByteSize * info.elementCount;
	size_t totalDataSize = (entryCount > 0) ? entrySizeInBytes + ((entryCount - 1) * entryStrideInBytes) : 0;
	PendingWrite pendingWrite(info, entryCount, initialVertexNo, entryStrideInBytes, transferBatchResolved);
	pendingWrite.data.assign(data, data + totalDataSize);

	// If a transfer batch has been supplied, increment the usage count.
	if (transferBatchResolved != nullptr)
	{
		transferBatchResolved->IncrementUsageCount();
	}

	// Queue a task to update the buffer with the supplied data
	std::unique_lock<std::mutex> lock(_buildStateMutex);
	_state[_buildIndex].pendingWrites.push_back(std::move(pendingWrite));
	lock.unlock();
	FlagBuildStateModified();
	return true;
}

//----------------------------------------------------------------------------------------
SuccessToken OpenGLVertexBuffer::QueueRawDataUpdate(const uint8_t* data, size_t dataSizeInBytes, size_t bufferOffsetInBytes, ITransferBatch* transferBatch)
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

	// If a transfer batch has been supplied, ensure it hasn't already been submitted.
	auto* transferBatchResolved = KnownDynamicCast<OpenGLTransferBatch*>(transferBatch);
	if ((transferBatchResolved != nullptr) && transferBatchResolved->IsSubmitted())
	{
		_log->Error("Attempted to queue a transfer using a transfer batch that has already been submitted");
		return false;
	}

	// Capture the supplied update settings and data
	PendingWrite pendingWrite(_vertexAttributeInfo.front(), bufferOffsetInBytes, dataSizeInBytes, transferBatchResolved);
	pendingWrite.data.assign(data, data + dataSizeInBytes);

	// If a transfer batch has been supplied, increment the usage count.
	if (transferBatchResolved != nullptr)
	{
		transferBatchResolved->IncrementUsageCount();
	}

	// Queue a task to update the buffer with the supplied data
	std::unique_lock<std::mutex> lock(_buildStateMutex);
	_state[_buildIndex].pendingWrites.push_back(std::move(pendingWrite));
	lock.unlock();
	FlagBuildStateModified();
	return true;
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
void OpenGLVertexBuffer::MigrateBuildStateToDrawState()
{
	std::swap(_buildIndex, _drawIndex);
	_stateModified.clear(std::memory_order_relaxed);
}

//----------------------------------------------------------------------------------------
void OpenGLVertexBuffer::CreateNativeBuffer()
{
	// Retrieve the initial data buffer if required
	const uint8_t* initialData = nullptr;
	if (!_initialDataBuffer.empty())
	{
		initialData = _initialDataBuffer.data();
	}

	// Allocate the vertex buffer
	glGenBuffers(1, &_openGLBufferNo);
	glBindBuffer(GL_ARRAY_BUFFER, _openGLBufferNo);
	glBufferData(GL_ARRAY_BUFFER, _totalBufferSizeInBytes, initialData, _usageFlag);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	CheckGLError(_log);

	// Release any resources related to the initial data. We don't use clear() and shrink_to_fit() here because this
	// data could be very large, and shrink_to_fit() isn't guaranteed to do anything. This approach is guaranteed to do
	// what we want, which is actually release the allocated memory for this buffer, since we won't need it again.
	std::vector<unsigned char>().swap(_initialDataBuffer);
	_initialDataBuffer = std::vector<unsigned char>();
}

//----------------------------------------------------------------------------------------
void OpenGLVertexBuffer::CompletePendingDataWrites()
{
	// Create the native buffer if its creation is pending
	if (_nativeBufferCreationPending)
	{
		CreateNativeBuffer();
		_nativeBufferCreationPending = false;
	}

	// Complete any pending writes we need to perform in the buffer
	std::vector<PendingWrite>& pendingWrites = _state[_drawIndex].pendingWrites;
	if (!pendingWrites.empty())
	{
		// Split pending writes into those that are ready to run now, and those that must remain queued until their
		// batch has been submitted.
		std::vector<PendingWrite> readyWrites;
		std::vector<PendingWrite> deferredWrites;
		readyWrites.reserve(pendingWrites.size());
		deferredWrites.reserve(pendingWrites.size());
		for (PendingWrite& write : pendingWrites)
		{
			if ((write.transferBatch != nullptr) && !write.transferBatch->IsSubmitted())
			{
				deferredWrites.push_back(std::move(write));
				continue;
			}
			readyWrites.push_back(std::move(write));
		}
		pendingWrites.clear();

		// Re-queue any deferred writes onto the build state so they remain pending for a later frame.
		if (!deferredWrites.empty())
		{
			std::unique_lock<std::mutex> lock(_buildStateMutex);
			auto& buildPendingWrites = _state[_buildIndex].pendingWrites;
			for (PendingWrite& write : deferredWrites)
			{
				buildPendingWrites.push_back(std::move(write));
			}
			lock.unlock();
			FlagBuildStateModified();
		}
		if (readyWrites.empty())
		{
			return;
		}

		glBindBuffer(GL_ARRAY_BUFFER, _openGLBufferNo);
		CheckGLError(_log);

		for (const PendingWrite& write : readyWrites)
		{
			CompletePendingDataWrite(write);
		}

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		CheckGLError(_log);
	}
}

//----------------------------------------------------------------------------------------
bool OpenGLVertexBuffer::CompletePendingDataWrite(const PendingWrite& pendingWrite)
{
	const VertexAttributeInfo& info = pendingWrite.attributeInfo;
	size_t entryCount = pendingWrite.entryCount;
	size_t initialVertexNo = pendingWrite.initialVertexNo;
	size_t entryStrideInBytes = pendingWrite.entryStrideInBytes;
	const uint8_t* data = pendingWrite.data.data();

	// Calculate the region of the buffer being modified
	size_t entrySizeInBytes = info.dataTypeByteSize * info.elementCount;
	size_t attributeVertexSizeInBytes = (!pendingWrite.rawDataWrite && _bufferInterleaved) ? _interleavedVertexSizeInBytes : (info.vertexPaddingAtStart + entrySizeInBytes + info.vertexPaddingAtEnd);
	size_t bufferStartPosInBytes = info.bufferOffsetInBytes + info.vertexPaddingAtStart + (initialVertexNo * attributeVertexSizeInBytes);
	size_t bufferStrideInBytes = attributeVertexSizeInBytes;
	if (pendingWrite.rawDataWrite)
	{
		glBufferSubData(GL_ARRAY_BUFFER, initialVertexNo, entryCount, reinterpret_cast<const void*>(data));
	}
	else if (!_bufferInterleaved && (bufferStrideInBytes == entryStrideInBytes))
	{
		glBufferSubData(GL_ARRAY_BUFFER, bufferStartPosInBytes, entryCount * bufferStrideInBytes, reinterpret_cast<const void*>(data));
	}
	else
	{
		auto* mappedMemory = reinterpret_cast<uint8_t*>(glMapBufferRange(GL_ARRAY_BUFFER, bufferStartPosInBytes, entryCount * bufferStrideInBytes, GL_MAP_WRITE_BIT));
		if (mappedMemory == nullptr)
		{
			_log->Error("glMapBufferRange failed with error code {0}", glGetError());
			return false;
		}

		while (entryCount > 0)
		{
			std::memcpy(mappedMemory, data, entrySizeInBytes);
			mappedMemory += bufferStrideInBytes;
			data += entryStrideInBytes;
			--entryCount;
		}

		glUnmapBuffer(GL_ARRAY_BUFFER);
	}
	CheckGLError(_log);

	// If a transfer batch has been supplied, decrement the usage count.
	if (pendingWrite.transferBatch != nullptr)
	{
		pendingWrite.transferBatch->DecrementUsageCount();
	}
	return true;
}

//----------------------------------------------------------------------------------------
void OpenGLVertexBuffer::FlagBuildStateModified()
{
	if (!_stateModified.test_and_set(std::memory_order_acquire))
	{
		_renderer->FlagObjectModified(this);
	}
}

//----------------------------------------------------------------------------------------
GLuint OpenGLVertexBuffer::GetOpenGLBufferNo() const
{
	return _openGLBufferNo;
}

} // namespace cobalt::graphics
