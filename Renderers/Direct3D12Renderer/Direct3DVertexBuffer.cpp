// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "Direct3DVertexBuffer.h"
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
: _log(log), _renderer(renderer), _buildIndex(0), _drawIndex(1)
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
	if (!AllocateMemoryInternal(texelArrayResolved))
	{
		return false;
	}
	_hasBufferAlias = true;
	return true;
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

	// Combine the performance hint flags that are of interest between the bound attributes
	//bool samePerformanceHintForAllAttributes = true;
	//bool cpuWritesPerformed = false;
	//bool cpuWritesFrequent = false;
	//for (size_t i = 0; i < _vertexAttributeCount; ++i)
	//{
	//	bool cpuWritesPerformedForAttribute = ((_vertexAttributeInfo[i].performanceHintCpu & IVertexAttribute::PerformanceHint::WriteFlagsMask) == IVertexAttribute::PerformanceHint::Default) || ((_vertexAttributeInfo[i].performanceHintCpu & IVertexAttribute::PerformanceHint::WriteFlagsMask) != IVertexAttribute::PerformanceHint::ReadNever);
	//	bool cpuWritesFrequentForAttribute = ((_vertexAttributeInfo[i].performanceHintCpu & IVertexAttribute::PerformanceHint::WriteFlagsMask) == IVertexAttribute::PerformanceHint::WriteOften);
	//	if (i > 0)
	//	{
	//		samePerformanceHintForAllAttributes &= (cpuWritesPerformedForAttribute == cpuWritesPerformed) && (cpuWritesFrequentForAttribute == cpuWritesFrequent);
	//	}
	//	cpuWritesPerformed |= cpuWritesPerformedForAttribute;
	//	cpuWritesFrequent |= cpuWritesFrequentForAttribute;
	//}

	// Calculate the starting positions of each vertex attribute data block in the vertex buffer. We ensure a minimum
	// alignment of 4 bytes between vertex attributes, as per the defined Direct3D Vertex Specification Best Practices:
	// https://www.khronos.org/Direct3D/wiki/Vertex_specification_best_practices
	// Also note that we don't support interleaved vertex buffers in Direct3D 12. Not only was this shown to harm rather
	// than help performance in tests under other renderers, using interleaved buffers sometimes and not at other times
	// would result in renderable objects which would otherwise be able to share the same pipeline state object having
	// different values for the AlignedByteOffset field of the D3D12_INPUT_ELEMENT_DESC structure within their
	// D3D12_INPUT_LAYOUT_DESC entry of the pipeline state, requiring different pipeline state objects to be used for
	// these objects, which would degrade performance.
	//##TODO## Re-evaluate the future of interleaved buffers, if there is any. We now allow the user to manually generate
	//an interleaved buffer through the use of manual layout vertex attribute attachments, and we were able to do it
	//without pipeline state changes by offsetting the base address of the vertex attribute buffer views rather than
	//using the AlignedByteOffset field, however we sometimes need to introduce silent extra padding to the end of the
	//vertex buffer in order to keep the alignment requirements of the bound buffer views within valid memory regions.
	_bufferInterleaved = false;
	const size_t minimumComponentAlignment = 1;
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
	if (!CreateNativeBuffer(aliasAsTexelArray))
	{
		_log->Error("Failed to create native objects for vertex buffer.");
		return false;
	}

	// If initial data has been provided, stage an upload of that data into the buffer.
	if (initialDataProvided)
	{
		// Create an upload buffer for the data
		auto uploadBufferSizeInBytes = _totalBufferSizeInBytes;
		ID3D12Resource* uploadBuffer = nullptr;
		_renderer->CreateTemporaryUploadBuffer(uploadBufferSizeInBytes, uploadBuffer, false);
		if (uploadBuffer == nullptr)
		{
			_log->Error("Failed to create upload buffer for vertex buffer");
			return false;
		}

		// Map the upload buffer into the CPU address space
		uint8_t* uploadBufferMapped;
		CD3DX12_RANGE readRange(0, 0);
		if (FAILED(uploadBuffer->Map(0, &readRange, reinterpret_cast<void**>(&uploadBufferMapped))))
		{
			_log->Error("Failed to map upload buffer for vertex buffer");
			return false;
		}

		// Write the initial data into the upload buffer. Note that in the case of a raw write, we use the non-padded
		// buffer size as the transfer length, which we validated above. This will leave the padding bytes, if any,
		// uninitialized, but we don't care what value they're set to.
		if (!_initialDataBuffer.empty())
		{
			PendingWrite pendingWrite(_vertexAttributeInfo.front(), 0, nonPaddedBufferSizeInBytes, nullptr);
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
				PendingWrite pendingWrite(attributeInfo, attributeInfo.vertexCount, 0, attributeInfo.initialDataEntryStrideInBytes, nullptr);
				WriteDataToMappedBuffer(pendingWrite, attributeInfo.initialData, (uploadBufferMapped + attributeInfo.bufferStartPosInBytes));
			}
		}

		// Unmap the upload buffer
		uploadBuffer->Unmap(0, nullptr);

		// Allocate a new command list on the build queue to handle the initial data transfer
		CommandListHandle commandListHandle = _renderer->GetBuildCommandList();
		ID3D12GraphicsCommandList* commandList = commandListHandle.GetCommandList();
		D3DX12Residency::ResidencySet* residencySet = commandListHandle.GetResidencySet();

		// Add the target buffer to the residency set
		residencySet->Insert(&_residencyObject);

		// Schedule a data transfer from the upload buffer to the target buffer. Note that we rely on implicit promotion
		// from COMMON here to skip a resource barrier. We also rely on the call to CompletePendingDataWrites by the
		// renderer before the first use to transition it into the default state.
		commandList->CopyBufferRegion(_bufferWrapper.buffer, 0, uploadBuffer, 0, uploadBufferSizeInBytes);
	}

	// Release any resources related to the initial data
	for (size_t i = 0; i < _vertexAttributeCount; ++i)
	{
		VertexAttributeInfo& attributeInfo = _vertexAttributeInfo[i];
		attributeInfo.initialDataSet = false;
		attributeInfo.initialData = nullptr;
	}

	// Add an alias for this vertex buffer if requested
	if (aliasAsTexelArray && !texelArray->AllocateAsAliasForBuffer(&_bufferWrapper, _totalBufferSizeInBytes))
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
	// Delete our residency tracking object
	if (_managedObjectTracked)
	{
		_renderer->ResidencyManager().EndTrackingObject(&_residencyObject);
		_managedObjectTracked = false;
	}

	// Delete our created buffer object
	if (_vertexBufferCreated)
	{
		_vertexBuffer.Reset();
		_bufferWrapper = {};
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

	// Capture the supplied update settings
	size_t entrySizeInBytes = info.dataTypeByteSize * info.elementCount;
	size_t uploadBufferSizeInBytes = entrySizeInBytes * entryCount;
	size_t attributeVertexSizeInBytes = info.vertexPaddingAtStart + entrySizeInBytes + info.vertexPaddingAtEnd;
	size_t bufferStartPosInBytes = info.bufferStartPosInBytes + (initialVertexNo * attributeVertexSizeInBytes);
	PendingWrite pendingWrite(info, entryCount, initialVertexNo, entryStrideInBytes, transferBatchResolved);
	return QueueDataUpdateInternal(pendingWrite, data, uploadBufferSizeInBytes, bufferStartPosInBytes, transferBatchResolved);
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

	// Capture the supplied update settings
	auto* transferBatchResolved = KnownDynamicCast<Direct3DTransferBatch*>(transferBatch);
	PendingWrite pendingWrite(_vertexAttributeInfo.front(), bufferOffsetInBytes, dataSizeInBytes, transferBatchResolved);
	return QueueDataUpdateInternal(pendingWrite, data, dataSizeInBytes, bufferOffsetInBytes, transferBatchResolved);
}

//----------------------------------------------------------------------------------------
bool Direct3DVertexBuffer::QueueDataUpdateInternal(PendingWrite& pendingWrite, const uint8_t* data, size_t uploadBufferSizeInBytes, size_t bufferStartPosInBytes, Direct3DTransferBatch* transferBatch)
{
	// If a transfer batch has been supplied, increment the usage count.
	if (transferBatch != nullptr)
	{
		transferBatch->IncrementUsageCount();
	}

	// Create an upload buffer for the data
	ID3D12Resource* uploadBuffer = nullptr;
	_renderer->CreateTemporaryUploadBuffer(uploadBufferSizeInBytes, uploadBuffer, true);
	if (uploadBuffer == nullptr)
	{
		_log->Error("Failed to create upload buffer for vertex buffer");
		return false;
	}

	// Map the upload buffer into the CPU address space
	uint8_t* uploadBufferMapped;
	CD3DX12_RANGE readRange(0, 0);
	if (FAILED(uploadBuffer->Map(0, &readRange, reinterpret_cast<void**>(&uploadBufferMapped))))
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
	uploadBuffer->Unmap(0, nullptr);

	// Queue a task to update the buffer with the supplied data
	std::unique_lock<std::mutex> lock(_buildStateMutex);
	_state[_buildIndex].pendingWrites.push_back(pendingWrite);
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
	// Allocate the vertex buffer. Note that we need to create the buffer in the COMMON resource state as we're using
	// the default heap.
	D3D12_RESOURCE_STATES finalResourceState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_COMMON;
	D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE;
	if (aliasAsTexelArray)
	{
		resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}
	auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto resourceDescription = CD3DX12_RESOURCE_DESC::Buffer(_totalBufferSizeInBytes, resourceFlags);
	HRESULT createVertexBufferReturn = _renderer->GetDevice()->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDescription, initialResourceState, nullptr, IID_PPV_ARGS(&_vertexBuffer));
	if (FAILED(createVertexBufferReturn))
	{
		_log->Error("Failed to create vertex buffer with error code {0}", createVertexBufferReturn);
		return false;
	}
	_bufferWrapper.buffer = _vertexBuffer.Get();

	// Register the native object with the residency manager
	_residencyObject.Initialize(_bufferWrapper.buffer, _totalBufferSizeInBytes);
	_renderer->ResidencyManager().BeginTrackingObject(&_residencyObject);
	_managedObjectTracked = true;

	// Record the resource state for the buffer
	_bufferWrapper.lastResourceState = initialResourceState;
	_bufferWrapper.defaultResourceState = finalResourceState;
	return true;
}

//----------------------------------------------------------------------------------------
size_t Direct3DVertexBuffer::CompletePendingDataWrites(ID3D12GraphicsCommandList* commandList, D3DX12Residency::ResidencySet* residencySet)
{
	// Obtain our set of current pending writes
	std::vector<PendingWrite>& pendingWrites = _state[_drawIndex].pendingWrites;

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
			deferredWrites.push_back(write);
			continue;
		}
		readyWrites.push_back(write);
	}
	pendingWrites.clear();

	// Re-queue any deferred writes onto the build state so they remain pending for a later frame.
	if (!deferredWrites.empty())
	{
		std::unique_lock<std::mutex> lock(_buildStateMutex);
		auto& buildPendingWrites = _state[_buildIndex].pendingWrites;
		for (PendingWrite& write : deferredWrites)
		{
			_renderer->ExtendTransferBufferLifetimeToNextFrame(write.uploadBuffer);
			buildPendingWrites.push_back(write);
		}
		lock.unlock();
		FlagBuildStateModified();
	}

	// If there are no pending writes, transition the buffer to the required resource state for drawing if required, and
	// abort any further processing.
	if (readyWrites.empty())
	{
		if (_bufferWrapper.lastResourceState != _bufferWrapper.defaultResourceState)
		{
			auto releaseBarrier = CD3DX12_RESOURCE_BARRIER::Transition(_bufferWrapper.buffer, _bufferWrapper.lastResourceState, _bufferWrapper.defaultResourceState);
			commandList->ResourceBarrier(1, &releaseBarrier);
			_bufferWrapper.lastResourceState = _bufferWrapper.defaultResourceState;
		}
		return 0;
	}

	// Perform each pending write operation
	return CompletePendingDataWritesInternal(readyWrites, commandList, residencySet, true);
}

//----------------------------------------------------------------------------------------
size_t Direct3DVertexBuffer::CompletePendingDataWritesInternal(std::vector<PendingWrite>& pendingWrites, ID3D12GraphicsCommandList* commandList, D3DX12Residency::ResidencySet* residencySet, bool performDrawStateTransition)
{
	// Add the buffer to the residency set
	residencySet->Insert(&_residencyObject);

	// Transition the buffer to the required resource for writing
	if (_bufferWrapper.lastResourceState != D3D12_RESOURCE_STATE_COPY_DEST)
	{
		auto acquireBarrier = CD3DX12_RESOURCE_BARRIER::Transition(_bufferWrapper.buffer, _bufferWrapper.lastResourceState, D3D12_RESOURCE_STATE_COPY_DEST);
		commandList->ResourceBarrier(1, &acquireBarrier);
	}

	// Complete each pending data write
	for (const PendingWrite& write : pendingWrites)
	{
		CompletePendingDataWrite(commandList, residencySet, write);
	}

	// Free our set of pending writes
	pendingWrites.clear();

	// Transition the buffer to the required resource state for drawing if requested
	if (performDrawStateTransition)
	{
		auto releaseBarrier = CD3DX12_RESOURCE_BARRIER::Transition(_bufferWrapper.buffer, D3D12_RESOURCE_STATE_COPY_DEST, _bufferWrapper.defaultResourceState);
		commandList->ResourceBarrier(1, &releaseBarrier);
		_bufferWrapper.lastResourceState = _bufferWrapper.defaultResourceState;
	}

	// Since we added the buffer to the residency set, return the buffer size in bytes.
	return _totalBufferSizeInBytes;
}

//----------------------------------------------------------------------------------------
void Direct3DVertexBuffer::WriteDataToMappedBuffer(const PendingWrite& pendingWrite, const uint8_t* data, uint8_t* mappedMemory)
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
void Direct3DVertexBuffer::CompletePendingDataWrite(ID3D12GraphicsCommandList* commandList, D3DX12Residency::ResidencySet* residencySet, const PendingWrite& pendingWrite)
{
	// Schedule a data transfer from the upload buffer to the target buffer
	commandList->CopyBufferRegion(_bufferWrapper.buffer, pendingWrite.targetBufferPos, pendingWrite.uploadBuffer, 0, pendingWrite.uploadBufferSizeInBytes);

	// If a transfer batch has been supplied, decrement the usage count.
	if (pendingWrite.transferBatch != nullptr)
	{
		pendingWrite.transferBatch->DecrementUsageCount();
	}
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
ID3D12Resource* Direct3DVertexBuffer::GetNativeBuffer() const
{
	return _bufferWrapper.buffer;
}

//----------------------------------------------------------------------------------------
D3DX12Residency::ManagedObject* Direct3DVertexBuffer::GetResidencyObject() const
{
	return &_residencyObject;
}

//----------------------------------------------------------------------------------------
bool Direct3DVertexBuffer::HasBufferAlias() const
{
	return _hasBufferAlias;
}

//----------------------------------------------------------------------------------------
BufferWrapper* Direct3DVertexBuffer::GetBufferWrapper()
{
	return &_bufferWrapper;
}

} // namespace cobalt::graphics
