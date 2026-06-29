// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "Direct3DVertexBuffer.h"
#include "Direct3DHeaders.h"
#include "Direct3DRenderer.h"
#include "Direct3DTexelArray.h"
#include "Direct3DTransferBatch.h"
#include <Internal/RendererSupport/KnownDynamicCast.h>
#include <algorithm>
#include <cstring>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
Direct3DVertexBuffer::Direct3DVertexBuffer(cobalt::logging::ILogger* log, Direct3DRenderer* renderer)
: _log(log), _renderer(renderer), _vertexBufferCreated(false), _nativeBufferCreationPending(false), _vertexAttributeCount(0), _interleavedVertexSizeInBytes(0), _buildIndex(0), _drawIndex(1)
{}

//----------------------------------------------------------------------------------------
Direct3DVertexBuffer::~Direct3DVertexBuffer()
{
	ReleaseMemory();
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void Direct3DVertexBuffer::Delete()
{
	_renderer->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
SuccessToken Direct3DVertexBuffer::AllocateMemory()
{
	return AllocateMemoryInternal(nullptr);
}

//----------------------------------------------------------------------------------------
SuccessToken Direct3DVertexBuffer::AllocateMemoryWithAlias(ITexelArray* texelArray)
{
	auto* texelArrayResolved = KnownDynamicCast<Direct3DTexelArray*>(texelArray);
	if (texelArrayResolved == nullptr)
	{
		_log->Error("Attempted to allocate memory for a vertex buffer with an alias, where no alias object was supplied.");
		return false;
	}
	return AllocateMemoryInternal(texelArrayResolved);
}

//----------------------------------------------------------------------------------------
bool Direct3DVertexBuffer::AllocateMemoryInternal(Direct3DTexelArray* texelArray)
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

	// Determine the vertex count to use if an interleaved buffer is created
	//bool sameVertexCountForAllAttributes = true;
	auto interleavedBufferVertexCount = _vertexAttributeInfo[0].vertexCount;
	for (size_t i = 1; i < _vertexAttributeCount; ++i)
	{
		auto attributeVertexCount = _vertexAttributeInfo[i].vertexCount;
		//sameVertexCountForAllAttributes &= interleavedBufferVertexCount == attributeVertexCount;
		interleavedBufferVertexCount = (attributeVertexCount > interleavedBufferVertexCount) ? attributeVertexCount : interleavedBufferVertexCount;
	}

	// Combine the performance hint flags that are of interest between the bound attributes
	//bool samePerformanceHintForAllAttributes = true;
	bool cpuWritesPerformed = false;
	bool cpuWritesFrequent = false;
	bool allowDiscardOnPartialWrite = true;
	for (size_t i = 0; i < _vertexAttributeCount; ++i)
	{
		bool cpuWritesPerformedForAttribute = ((_vertexAttributeInfo[i].performanceHintCpu & IVertexAttribute::PerformanceHint::WriteFlagsMask) == IVertexAttribute::PerformanceHint::Default) || ((_vertexAttributeInfo[i].performanceHintCpu & IVertexAttribute::PerformanceHint::WriteFlagsMask) != IVertexAttribute::PerformanceHint::ReadNever);
		bool cpuWritesFrequentForAttribute = ((_vertexAttributeInfo[i].performanceHintCpu & IVertexAttribute::PerformanceHint::WriteFlagsMask) == IVertexAttribute::PerformanceHint::WriteOften);
		bool allowDiscardOnPartialWritesForAttribute = ((_vertexAttributeInfo[i].dataPersistenceFlags & IVertexAttribute::DataPersistenceFlags::InvalidateExistingDataOnWrite) == IVertexAttribute::DataPersistenceFlags::InvalidateExistingDataOnWrite);

		if (i > 0)
		{
			//samePerformanceHintForAllAttributes &= (cpuWritesPerformedForAttribute == cpuWritesPerformed) && (cpuWritesFrequentForAttribute == cpuWritesFrequent) && (allowDiscardOnPartialWritesForAttribute == allowDiscardOnPartialWrite);
			allowDiscardOnPartialWrite = false;
		}
		cpuWritesPerformed |= cpuWritesPerformedForAttribute;
		cpuWritesFrequent |= cpuWritesFrequentForAttribute;
		allowDiscardOnPartialWrite &= allowDiscardOnPartialWritesForAttribute;
	}

	// Determine the usage flag to use when defining the Direct3D buffer
	UINT cpuFlags;
	D3D11_USAGE usageType;
	if (aliasAsTexelArray)
	{
		usageType = D3D11_USAGE_DEFAULT;
		cpuFlags = 0;
	}
	else if (!cpuWritesPerformed)
	{
		usageType = D3D11_USAGE_IMMUTABLE;
		cpuFlags = 0;
	}
	else if (!cpuWritesFrequent)
	{
		usageType = D3D11_USAGE_DEFAULT;
		cpuFlags = 0;
	}
	else if (!allowDiscardOnPartialWrite)
	{
		// D3D11 dynamic buffers only support WRITE_DISCARD and WRITE_NO_OVERWRITE map modes. If callers require the
		// existing contents to be preserved across partial writes, keep the buffer as DEFAULT and update it with
		// UpdateSubresource instead of trying to map it for plain WRITE access.
		usageType = D3D11_USAGE_DEFAULT;
		cpuFlags = 0;
	}
	else
	{
		usageType = D3D11_USAGE_DYNAMIC;
		cpuFlags = D3D11_CPU_ACCESS_WRITE;
	}
	_cpuFlags = cpuFlags;
	_usageType = usageType;
	_usingDynamicBuffer = (usageType == D3D11_USAGE_DYNAMIC);
	_allowDiscardOnPartialWrite = allowDiscardOnPartialWrite;

	// Determine if an interleaved memory buffer should be used for the vertex attributes
	//##TODO## Interleaved buffers are painfully slow to update when we don't map the buffer, and the gains from
	// interleaving are questionable to start with. It's been suggested interleaving is either not helpful or is
	// actually harmful on modern hardware. As we couldn't measure benefits from doing it, and it hurts update
	// performance for non-static data, we've disabled it here. Determine the future of interleaved buffer support, such
	// as whether it should be retained, and if so, under what circumstances it should be employed.
	//_bufferInterleaved = (_vertexAttributeCount > 1) && sameVertexCountForAllAttributes && samePerformanceHintForAllAttributes;
	_bufferInterleaved = false;

	// Calculate the total size of the vertex buffer, and the starting positions of each vertex attribute data block in
	// the buffer. We ensure a minimum alignment of 4 bytes between vertex attributes, as per the defined OpenGL Vertex
	// Specification Best Practices:
	// https://www.khronos.org/opengl/wiki/Vertex_Specification_Best_Practices
	//##TODO## Determine the future of minimum vertex alignment. Old information suggested aligning all vertex
	//attributes to a minimum stride of 4 bytes could give a performance boost. Recent testing on current hardware was
	//not able to demonstrate any performance improvements, in fact there was a slight reduction in performance on Intel
	//integrated devices, most likely due to increased shared memory access. Since alignment requirements impact memory
	//usage where smaller types are used, the minimum alignment is currently set to 1 byte, effectively disabling the
	//feature. This issue should be re-visited in the future, with more comprehensive hardware testing and research to
	//determine best practice in this area. Note that if support for generating interleaved buffer layouts is restored
	//however, the minimum alignment will once again need to return to 4 bytes in that case.
	const size_t minimumComponentAlignment = (_bufferInterleaved ? 4 : 1);
	const size_t minimumStartingAttributeAlignment = 4;
	size_t totalBufferSizeInBytes = 0;
	if (_manualBufferLayout)
	{
		for (size_t i = 0; i < _vertexAttributeCount; ++i)
		{
			VertexAttributeInfo& attributeInfo = _vertexAttributeInfo[i];
			size_t bufferStartPosInBytes = attributeInfo.bufferStartPosInBytes;
			size_t entrySizeInBytes = attributeInfo.dataTypeByteSize * attributeInfo.elementCount;
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
			attributeInfo.bufferBaseStartAddress = 0;
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
			attributeInfo.bufferBaseStartAddress = offset;
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
			_initialDataBuffer.resize(_totalBufferSizeInBytes);
			unsigned char* allocatedBuffer = _initialDataBuffer.data();
			for (unsigned int i = 0; i < _vertexAttributeCount; ++i)
			{
				VertexAttributeInfo& attributeInfo = _vertexAttributeInfo[i];
				if (!attributeInfo.initialDataSet)
				{
					continue;
				}

				size_t entrySizeInBytes = attributeInfo.dataTypeByteSize * attributeInfo.elementCount;
				size_t bufferStrideInBytes = attributeInfo.bufferStrideInBytes;
				const unsigned char* sourceEntryPos = attributeInfo.initialData;
				unsigned char* targetEntryPos = allocatedBuffer + attributeInfo.bufferStartPosInBytes;
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

	// Determine if we should use deferred buffer creation
	bool deferBufferCreation = (_renderer->UseDeferredBufferCreation() && !aliasAsTexelArray);

	// Create the buffer immediately if requested
	_nativeBufferCreationPending = deferBufferCreation;
	if (!deferBufferCreation)
	{
		if (!CreateNativeBuffer(aliasAsTexelArray))
		{
			_log->Error("Failed to create native objects for vertex buffer.");
			return false;
		}
	}

	// Release any resources related to the initial data. We don't use clear() and shrink_to_fit() here because this
	// data could be very large, and shrink_to_fit() isn't guaranteed to do anything. This approach is guaranteed to do
	// what we want, which is actually release the allocated memory for this buffer, since we won't need it again.
	if (!deferBufferCreation)
	{
		std::vector<unsigned char>().swap(_initialDataBuffer);
		_initialDataBuffer = std::vector<unsigned char>();
	}
	for (size_t i = 0; i < _vertexAttributeCount; ++i)
	{
		VertexAttributeInfo& attributeInfo = _vertexAttributeInfo[i];
		attributeInfo.initialDataSet = false;
		attributeInfo.initialData = nullptr;
	}

	// Add an alias for this vertex buffer if requested
	if (aliasAsTexelArray && !texelArray->AllocateAsAliasForBuffer(_vertexBuffer.Get(), _totalBufferSizeInBytes))
	{
		_log->Error("Failed to allocate alias for vertex buffer.");
		return false;
	}

	// Flag that the buffer has been created
	_vertexBufferCreated = true;
	FlagBuildStateModified();
	return true;
}

//----------------------------------------------------------------------------------------
void Direct3DVertexBuffer::ReleaseMemory()
{
	if (_vertexBufferCreated)
	{
		_vertexBuffer.Reset();
		_vertexBufferCreated = false;
	}
}

//----------------------------------------------------------------------------------------
bool Direct3DVertexBuffer::IsAllocated() const
{
	return _vertexBufferCreated;
}

//----------------------------------------------------------------------------------------
// Binding methods
//----------------------------------------------------------------------------------------
SuccessToken Direct3DVertexBuffer::BindVertexAttribute(IVertexAttribute& vertexAttribute)
{
	return BindVertexAttributeInternal(vertexAttribute, false, 0, 0);
}

//----------------------------------------------------------------------------------------
SuccessToken Direct3DVertexBuffer::BindVertexAttributeManualLayout(IVertexAttribute& vertexAttribute, size_t bufferOffsetInBytes, size_t bufferStrideInBytes)
{
	return BindVertexAttributeInternal(vertexAttribute, true, bufferOffsetInBytes, bufferStrideInBytes);
}

//----------------------------------------------------------------------------------------
bool Direct3DVertexBuffer::BindVertexAttributeInternal(IVertexAttribute& vertexAttribute, bool manualLayout, size_t bufferOffsetInBytes, size_t bufferStrideInBytes)
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
	attributeInfoEntry.dataPersistenceFlags = vertexAttribute.GetDataPersistenceFlags();
	attributeInfoEntry.initialDataSet = false;
	attributeInfoEntry.initialData = nullptr;
	attributeInfoEntry.bufferOffsetInBytes = bufferOffsetInBytes;
	attributeInfoEntry.bufferStrideInBytes = bufferStrideInBytes;
	if (manualLayout)
	{
		size_t entrySizeInBytes = attributeInfoEntry.dataTypeByteSize * attributeInfoEntry.elementCount;
		attributeInfoEntry.bufferBaseStartAddress = bufferOffsetInBytes;
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
const Direct3DVertexBuffer::VertexAttributeInfo* Direct3DVertexBuffer::GetVertexAttributeInfo(size_t attributeIndex) const
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
SuccessToken Direct3DVertexBuffer::SetInitialData(size_t attributeIndex, const uint8_t* data, size_t entryCount, size_t entryStrideInBytes)
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
SuccessToken Direct3DVertexBuffer::SetRawInitialData(const uint8_t* data, size_t dataSizeInBytes)
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
SuccessToken Direct3DVertexBuffer::QueueDataUpdate(size_t attributeIndex, const uint8_t* data, size_t entryCount, size_t initialVertexNo, size_t entryStrideInBytes, ITransferBatch* transferBatch)
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
	auto* transferBatchResolved = KnownDynamicCast<Direct3DTransferBatch*>(transferBatch);
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
SuccessToken Direct3DVertexBuffer::QueueRawDataUpdate(const uint8_t* data, size_t dataSizeInBytes, size_t bufferOffsetInBytes, ITransferBatch* transferBatch)
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
	auto* transferBatchResolved = KnownDynamicCast<Direct3DTransferBatch*>(transferBatch);
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
void Direct3DVertexBuffer::MigrateBuildStateToDrawState()
{
	std::swap(_buildIndex, _drawIndex);
	_stateModified.clear(std::memory_order_relaxed);
}

//----------------------------------------------------------------------------------------
bool Direct3DVertexBuffer::CreateNativeBuffer(bool aliasAsTexelArray)
{
	// Retrieve the initial data buffer if required
	const uint8_t* initialData = nullptr;
	if (!_initialDataBuffer.empty())
	{
		initialData = _initialDataBuffer.data();
	}

	// Allocate the vertex buffer
	D3D11_BUFFER_DESC bufferDescription = {};
	bufferDescription.Usage = _usageType;
	bufferDescription.ByteWidth = (UINT)_totalBufferSizeInBytes;
	bufferDescription.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	if (aliasAsTexelArray)
	{
		bufferDescription.BindFlags |= D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	}
	bufferDescription.CPUAccessFlags = _cpuFlags;
	D3D11_SUBRESOURCE_DATA subresourceData = {};
	D3D11_SUBRESOURCE_DATA* subresourceDataPointer = nullptr;
	if (initialData != nullptr)
	{
		subresourceData.pSysMem = initialData;
		subresourceData.SysMemPitch = 0;
		subresourceData.SysMemSlicePitch = 0;
		subresourceDataPointer = &subresourceData;
	}
	HRESULT createVertexBufferReturn = _renderer->GetDevice()->CreateBuffer(&bufferDescription, subresourceDataPointer, &_vertexBuffer);
	if (FAILED(createVertexBufferReturn))
	{
		_log->Error("Failed to create vertex buffer with error code {0}", createVertexBufferReturn);
		return false;
	}

	// Release any resources related to the initial data. We don't use clear() and shrink_to_fit() here because this
	// data could be very large, and shrink_to_fit() isn't guaranteed to do anything. This approach is guaranteed to do
	// what we want, which is actually release the allocated memory for this buffer, since we won't need it again.
	std::vector<unsigned char>().swap(_initialDataBuffer);
	_initialDataBuffer = std::vector<unsigned char>();
	return true;
}

//----------------------------------------------------------------------------------------
void Direct3DVertexBuffer::CompletePendingDataWrites(ID3D11Device1* device, ID3D11DeviceContext1* context)
{
	// Create the native buffer if its creation is pending
	if (_nativeBufferCreationPending)
	{
		CreateNativeBuffer(false);
		_nativeBufferCreationPending = false;
	}

	// Obtain the set of pending data writes. If no writes are pending, abort any further processing.
	std::vector<PendingWrite>& pendingWrites = _state[_drawIndex].pendingWrites;
	if (pendingWrites.empty())
	{
		return;
	}

	// Split pending writes into those that are ready to run now, and those that must remain queued until their batch
	// has been submitted.
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

	// Map the buffer if required
	void* mappedBuffer = nullptr;
	if (_usingDynamicBuffer)
	{
		D3D11_MAPPED_SUBRESOURCE mappedSubresource;
		HRESULT mapReturn = context->Map(_vertexBuffer.Get(), 0, (_allowDiscardOnPartialWrite ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE), 0, &mappedSubresource);
		if (FAILED(mapReturn))
		{
			_log->Error("Map operation failed when completing write operations for vertex buffer with error code {0}", mapReturn);
			pendingWrites.clear();
			return;
		}
		mappedBuffer = mappedSubresource.pData;
	}

	// Complete any pending writes we need to perform in the buffer
	for (const PendingWrite& write : readyWrites)
	{
		CompletePendingDataWrite(write, device, context, mappedBuffer);
	}

	// Unmap the buffer if required
	if (_usingDynamicBuffer)
	{
		context->Unmap(_vertexBuffer.Get(), 0);
	}
}

//----------------------------------------------------------------------------------------
bool Direct3DVertexBuffer::CompletePendingDataWrite(const PendingWrite& pendingWrite, ID3D11Device1* device, ID3D11DeviceContext1* context, void* mappedBuffer)
{
	// Update the target region of the buffer
	const uint8_t* data = pendingWrite.data.data();
	size_t entryCount = pendingWrite.entryCount;
	size_t initialVertexNo = pendingWrite.initialVertexNo;
	size_t entryStrideInBytes = pendingWrite.entryStrideInBytes;
	if (pendingWrite.rawDataWrite)
	{
		if (_usingDynamicBuffer)
		{
			auto* mappedMemory = reinterpret_cast<uint8_t*>(mappedBuffer);
			mappedMemory += initialVertexNo;
			std::memcpy(mappedMemory, data, entryCount);
		}
		else
		{
			D3D11_BOX box;
			box.front = 0;
			box.back = 1;
			box.top = 0;
			box.bottom = 1;
			box.left = (UINT)initialVertexNo;
			box.right = box.left + (UINT)entryCount;
			context->UpdateSubresource(_vertexBuffer.Get(), 0, &box, reinterpret_cast<const void*>(data), 0, 0);
		}
	}
	else
	{
		const VertexAttributeInfo& info = pendingWrite.attributeInfo;
		auto bufferStrideInBytes = info.bufferStrideInBytes;
		auto entrySizeInBytes = info.dataTypeByteSize * info.elementCount;
		auto bufferStartPosInBytes = info.bufferOffsetInBytes + info.vertexPaddingAtStart + (initialVertexNo * bufferStrideInBytes);
		if (_usingDynamicBuffer)
		{
			auto* mappedMemory = reinterpret_cast<uint8_t*>(mappedBuffer);
			mappedMemory += bufferStartPosInBytes;
			if (!_bufferInterleaved && (bufferStrideInBytes == entryStrideInBytes))
			{
				std::memcpy(mappedMemory, data, entryCount * bufferStrideInBytes);
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
		else if (!_bufferInterleaved && (bufferStrideInBytes == entryStrideInBytes))
		{
			D3D11_BOX box;
			box.front = 0;
			box.back = 1;
			box.top = 0;
			box.bottom = 1;
			box.left = (UINT)bufferStartPosInBytes;
			box.right = box.left + (UINT)(entryCount * entrySizeInBytes);
			context->UpdateSubresource(_vertexBuffer.Get(), 0, &box, reinterpret_cast<const void*>(data), 0, 0);
		}
		else
		{
			auto currentBufferPos = bufferStartPosInBytes;
			while (entryCount > 0)
			{
				D3D11_BOX box;
				box.front = 0;
				box.back = 1;
				box.top = 0;
				box.bottom = 1;
				box.left = (UINT)currentBufferPos;
				box.right = box.left + (UINT)entrySizeInBytes;
				context->UpdateSubresource(_vertexBuffer.Get(), 0, &box, reinterpret_cast<const void*>(data), 0, 0);

				currentBufferPos += bufferStrideInBytes;
				data += entryStrideInBytes;
				--entryCount;
			}
		}
	}

	// If a transfer batch has been supplied, decrement the usage count.
	if (pendingWrite.transferBatch != nullptr)
	{
		pendingWrite.transferBatch->DecrementUsageCount();
	}
	return true;
}

//----------------------------------------------------------------------------------------
void Direct3DVertexBuffer::FlagBuildStateModified()
{
	if (!_stateModified.test_and_set(std::memory_order_acquire))
	{
		_renderer->FlagObjectModified(this);
	}
}

//----------------------------------------------------------------------------------------
ID3D11Buffer* Direct3DVertexBuffer::GetNativeBuffer() const
{
	return _vertexBuffer.Get();
}

} // namespace cobalt::graphics
