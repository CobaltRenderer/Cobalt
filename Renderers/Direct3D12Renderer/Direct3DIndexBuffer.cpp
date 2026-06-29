// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "Direct3DIndexBuffer.h"
#include "Direct3DRenderer.h"
#include "Direct3DTexelArray.h"
#include "Direct3DTransferBatch.h"
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <Internal/RendererSupport/KnownDynamicCast.h>
#include <algorithm>
#include <cstring>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
Direct3DIndexBuffer::Direct3DIndexBuffer(cobalt::logging::ILogger* log, Direct3DRenderer* renderer)
: _log(log), _renderer(renderer), _buildIndex(0), _drawIndex(1)
{}

//----------------------------------------------------------------------------------------
Direct3DIndexBuffer::~Direct3DIndexBuffer()
{
	ReleaseMemory();
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void Direct3DIndexBuffer::Delete()
{
	_renderer->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
SuccessToken Direct3DIndexBuffer::AllocateMemory()
{
	return AllocateMemoryInternal(nullptr);
}

//----------------------------------------------------------------------------------------
SuccessToken Direct3DIndexBuffer::AllocateMemoryWithAlias(ITexelArray* texelArray)
{
	auto* texelArrayResolved = KnownDynamicCast<Direct3DTexelArray*>(texelArray);
	if (texelArrayResolved == nullptr)
	{
		_log->Error("Attempted to allocate memory for an index buffer with an alias, where no alias object was supplied.");
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
bool Direct3DIndexBuffer::AllocateMemoryInternal(Direct3DTexelArray* texelArray)
{
	// Ensure the array hasn't already been created, and that an index attribute has been defined.
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
	if (!CreateNativeBuffer(aliasAsTexelArray))
	{
		_log->Error("Failed to create native objects for index buffer.");
		return false;
	}

	// If initial data has been provided, stage an upload of that data into the buffer.
	if (_initialDataSet)
	{
		// Ensure the provided initial data is of the correct size
		if ((_initialDataEntrySizeInBytes == 1) && (_initialDataEntryCount != _totalBufferSizeInBytes))
		{
			_log->Error("Raw initial index data of size {0} was provided, but {1} bytes are needed for the buffer.", _initialDataEntryCount, _totalBufferSizeInBytes);
			return false;
		}

		// Create an upload buffer for the data
		auto entrySizeInBytes = _indexAttributeInfo.dataTypeByteSize;
		auto uploadBufferSizeInBytes = entrySizeInBytes * _indexAttributeInfo.indexCount;
		ID3D12Resource* uploadBuffer = nullptr;
		_renderer->CreateTemporaryUploadBuffer(uploadBufferSizeInBytes, uploadBuffer, false);
		if (uploadBuffer == nullptr)
		{
			_log->Error("Failed to create upload buffer for index buffer");
			return false;
		}

		// Map the upload buffer into the CPU address space
		uint8_t* uploadBufferMapped;
		CD3DX12_RANGE readRange(0, 0);
		if (FAILED(uploadBuffer->Map(0, &readRange, reinterpret_cast<void**>(&uploadBufferMapped))))
		{
			_log->Error("Failed to map upload buffer for index buffer");
			return false;
		}

		// Write the initial data into the upload buffer
		WriteDataToMappedBuffer(_initialDataEntryCount, _initialDataEntrySizeInBytes, _initialDataEntryStrideInBytes, _initialData, uploadBufferMapped);

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
	_initialDataSet = false;
	_initialData = nullptr;

	// Add an alias for this index buffer if requested
	if (aliasAsTexelArray && !texelArray->AllocateAsAliasForBuffer(&_bufferWrapper, _totalBufferSizeInBytes))
	{
		_log->Error("Failed to allocate alias for index buffer.");
		return false;
	}

	// Flag that the buffer has been created
	_indexBufferCreated = true;
	FlagBuildStateModified();
	return true;
}

//----------------------------------------------------------------------------------------
void Direct3DIndexBuffer::ReleaseMemory()
{
	// Delete our residency tracking object
	if (_managedObjectTracked)
	{
		_renderer->ResidencyManager().EndTrackingObject(&_residencyObject);
		_managedObjectTracked = false;
	}

	// Delete our created buffer object
	if (_indexBufferCreated)
	{
		_indexBuffer.Reset();
		_indexBufferCreated = false;
	}
}

//----------------------------------------------------------------------------------------
bool Direct3DIndexBuffer::IsAllocated() const
{
	return _indexBufferCreated;
}

//----------------------------------------------------------------------------------------
// Binding methods
//----------------------------------------------------------------------------------------
SuccessToken Direct3DIndexBuffer::BindIndexAttribute(IIndexAttribute& indexAttribute)
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
	IndexAttributeInfo attributeInfoEntry = {};
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
SuccessToken Direct3DIndexBuffer::BindIndexAttributeManualLayout(IIndexAttribute& indexAttribute, size_t bufferOffsetInBytes, size_t bufferStrideInBytes)
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
const Direct3DIndexBuffer::IndexAttributeInfo* Direct3DIndexBuffer::GetIndexAttributeInfo(size_t attributeIndex) const
{
	ASSERT(attributeIndex == 0);
	return &_indexAttributeInfo;
}

//----------------------------------------------------------------------------------------
// Data methods
//----------------------------------------------------------------------------------------
SuccessToken Direct3DIndexBuffer::SetInitialData(const uint8_t* data, size_t entryCount, size_t entryStrideInBytes)
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
SuccessToken Direct3DIndexBuffer::SetRawInitialData(const uint8_t* data, size_t dataSizeInBytes)
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
SuccessToken Direct3DIndexBuffer::QueueDataUpdate(const uint8_t* data, size_t entryCount, size_t initialIndexNo, size_t entryStrideInBytes, ITransferBatch* transferBatch)
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

	// If a transfer batch has been supplied, ensure it hasn't already been submitted.
	auto* transferBatchResolved = KnownDynamicCast<Direct3DTransferBatch*>(transferBatch);
	if ((transferBatchResolved != nullptr) && transferBatchResolved->IsSubmitted())
	{
		_log->Error("Attempted to queue a transfer using a transfer batch that has already been submitted");
		return false;
	}

	// Capture the supplied update settings
	size_t entrySizeInBytes = info.dataTypeByteSize;
	size_t uploadBufferSizeInBytes = entrySizeInBytes * entryCount;
	size_t bufferStartPosInBytes = info.bufferStartPosInBytes + (initialIndexNo * entrySizeInBytes);
	PendingWrite pendingWrite(info, entryCount, initialIndexNo, entryStrideInBytes, transferBatchResolved);
	return QueueDataUpdateInternal(pendingWrite, data, uploadBufferSizeInBytes, bufferStartPosInBytes, transferBatchResolved);
}

//----------------------------------------------------------------------------------------
SuccessToken Direct3DIndexBuffer::QueueRawDataUpdate(const uint8_t* data, size_t dataSizeInBytes, size_t bufferOffsetInBytes, ITransferBatch* transferBatch)
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
	auto* transferBatchResolved = KnownDynamicCast<Direct3DTransferBatch*>(transferBatch);
	PendingWrite pendingWrite(_indexAttributeInfo, bufferOffsetInBytes, dataSizeInBytes, transferBatchResolved);
	return QueueDataUpdateInternal(pendingWrite, data, dataSizeInBytes, bufferOffsetInBytes, transferBatchResolved);
}

//----------------------------------------------------------------------------------------
bool Direct3DIndexBuffer::QueueDataUpdateInternal(PendingWrite& pendingWrite, const uint8_t* data, size_t uploadBufferSizeInBytes, size_t bufferStartPosInBytes, Direct3DTransferBatch* transferBatch)
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
		_log->Error("Failed to create upload buffer for index buffer");
		return false;
	}

	// Map the upload buffer into the CPU address space
	uint8_t* uploadBufferMapped;
	CD3DX12_RANGE readRange(0, 0);
	if (FAILED(uploadBuffer->Map(0, &readRange, reinterpret_cast<void**>(&uploadBufferMapped))))
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
void Direct3DIndexBuffer::MigrateBuildStateToDrawState()
{
	std::swap(_buildIndex, _drawIndex);
	_stateModified.clear(std::memory_order_relaxed);
}

//----------------------------------------------------------------------------------------
bool Direct3DIndexBuffer::CreateNativeBuffer(bool aliasAsTexelArray)
{
	// Allocate the index buffer. Note that we need to create the buffer in the COMMON resource state as we're using the
	// default heap.
	D3D12_RESOURCE_STATES finalResourceState = D3D12_RESOURCE_STATE_INDEX_BUFFER;
	D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_COMMON;
	D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE;
	if (aliasAsTexelArray)
	{
		resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}
	auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto resourceDescription = CD3DX12_RESOURCE_DESC::Buffer(_totalBufferSizeInBytes, resourceFlags);
	HRESULT createIndexBufferReturn = _renderer->GetDevice()->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDescription, initialResourceState, nullptr, IID_PPV_ARGS(&_indexBuffer));
	if (FAILED(createIndexBufferReturn))
	{
		_log->Error("Failed to create index buffer with error code {0}", createIndexBufferReturn);
		return false;
	}
	_bufferWrapper.buffer = _indexBuffer.Get();

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
size_t Direct3DIndexBuffer::CompletePendingDataWrites(ID3D12GraphicsCommandList* commandList, D3DX12Residency::ResidencySet* residencySet)
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
size_t Direct3DIndexBuffer::CompletePendingDataWritesInternal(std::vector<PendingWrite>& pendingWrites, ID3D12GraphicsCommandList* commandList, D3DX12Residency::ResidencySet* residencySet, bool performDrawStateTransition)
{
	// Add the buffer to the residency set
	residencySet->Insert(&_residencyObject);

	// Transition the buffer to the required resource state for writing
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
void Direct3DIndexBuffer::WriteDataToMappedBuffer(size_t entryCount, size_t entrySizeInBytes, size_t entryStrideInBytes, const uint8_t* data, uint8_t* mappedMemory)
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
void Direct3DIndexBuffer::CompletePendingDataWrite(ID3D12GraphicsCommandList* commandList, D3DX12Residency::ResidencySet* residencySet, const PendingWrite& pendingWrite)
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
void Direct3DIndexBuffer::FlagBuildStateModified()
{
	if (!_stateModified.test_and_set(std::memory_order_acquire))
	{
		_renderer->FlagObjectModified(this);
	}
}

//----------------------------------------------------------------------------------------
ID3D12Resource* Direct3DIndexBuffer::GetNativeBuffer() const
{
	return _bufferWrapper.buffer;
}

//----------------------------------------------------------------------------------------
D3DX12Residency::ManagedObject* Direct3DIndexBuffer::GetResidencyObject() const
{
	return &_residencyObject;
}

//----------------------------------------------------------------------------------------
bool Direct3DIndexBuffer::HasBufferAlias() const
{
	return _hasBufferAlias;
}

//----------------------------------------------------------------------------------------
BufferWrapper* Direct3DIndexBuffer::GetBufferWrapper()
{
	return &_bufferWrapper;
}

} // namespace cobalt::graphics
