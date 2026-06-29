// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "Direct3DIndexBuffer.h"
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
	return AllocateMemoryInternal(texelArrayResolved);
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

	// Determine if we should use deferred buffer creation
	bool deferBufferCreation = (_renderer->UseDeferredBufferCreation() && !aliasAsTexelArray);

	// Obtain any initial data for the buffer
	const uint8_t* initialData = nullptr;
	if (_initialDataSet)
	{
		if (_initialDataEntrySizeInBytes == 1)
		{
			if (_initialDataEntryCount != _totalBufferSizeInBytes)
			{
				_log->Error("Raw initial index data of size {0} was provided, but {1} bytes are needed for the buffer.", _initialDataEntryCount, _totalBufferSizeInBytes);
				return false;
			}
			initialData = _initialData;
		}
		else if (_initialDataEntryStrideInBytes == _initialDataEntrySizeInBytes)
		{
			if (deferBufferCreation)
			{
				_initialDataBuffer.resize(_totalBufferSizeInBytes);
				uint8_t* targetEntryPos = _initialDataBuffer.data();
				std::memcpy(targetEntryPos, _initialData, _totalBufferSizeInBytes);
			}
			else
			{
				initialData = _initialData;
			}
		}
		else
		{
			_initialDataBuffer.resize(_totalBufferSizeInBytes);
			size_t entryCount = _initialDataEntryCount;
			const uint8_t* sourceEntryPos = _initialData;
			uint8_t* targetEntryPos = _initialDataBuffer.data();
			while (entryCount > 0)
			{
				std::memcpy(targetEntryPos, sourceEntryPos, _initialDataEntrySizeInBytes);
				targetEntryPos += _initialDataEntrySizeInBytes;
				sourceEntryPos += _initialDataEntryStrideInBytes;
				--entryCount;
			}
		}
	}

	// Combine the performance hint flags that are of interest between the bound attributes
	bool cpuWritesPerformed = ((_indexAttributeInfo.performanceHintCpu & IIndexAttribute::PerformanceHint::WriteFlagsMask) == IIndexAttribute::PerformanceHint::Default) || ((_indexAttributeInfo.performanceHintCpu & IIndexAttribute::PerformanceHint::WriteFlagsMask) != IIndexAttribute::PerformanceHint::ReadNever);
	bool cpuWritesFrequent = ((_indexAttributeInfo.performanceHintCpu & IIndexAttribute::PerformanceHint::WriteFlagsMask) == IIndexAttribute::PerformanceHint::WriteOften);
	bool allowDiscardOnPartialWrite = ((_indexAttributeInfo.dataPersistenceFlags & IIndexAttribute::DataPersistenceFlags::InvalidateExistingDataOnWrite) == IIndexAttribute::DataPersistenceFlags::InvalidateExistingDataOnWrite);

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

	// Create the buffer immediately if requested
	_nativeBufferCreationPending = deferBufferCreation;
	if (!deferBufferCreation)
	{
		if (!CreateNativeBuffer(initialData, aliasAsTexelArray))
		{
			_log->Error("Failed to create native objects for index buffer.");
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
	_initialDataSet = false;
	_initialData = nullptr;

	// Add an alias for this index buffer if requested
	if (aliasAsTexelArray && !texelArray->AllocateAsAliasForBuffer(_indexBuffer.Get(), _totalBufferSizeInBytes))
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
	attributeInfoEntry.dataPersistenceFlags = indexAttribute.GetDataPersistenceFlags();
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

	// Capture the supplied update settings and data
	auto entrySizeInBytes = info.dataTypeByteSize;
	auto totalDataSize = (entryCount > 0) ? entrySizeInBytes + ((entryCount - 1) * entryStrideInBytes) : 0;
	PendingWrite pendingWrite(info, entryCount, initialIndexNo, entryStrideInBytes, transferBatchResolved);
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

	// If a transfer batch has been supplied, ensure it hasn't already been submitted.
	auto* transferBatchResolved = KnownDynamicCast<Direct3DTransferBatch*>(transferBatch);
	if ((transferBatchResolved != nullptr) && transferBatchResolved->IsSubmitted())
	{
		_log->Error("Attempted to queue a transfer using a transfer batch that has already been submitted");
		return false;
	}

	// Capture the supplied update settings and data
	PendingWrite pendingWrite(_indexAttributeInfo, bufferOffsetInBytes, dataSizeInBytes, transferBatchResolved);
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
void Direct3DIndexBuffer::MigrateBuildStateToDrawState()
{
	std::swap(_buildIndex, _drawIndex);
	_stateModified.clear(std::memory_order_relaxed);
}

//----------------------------------------------------------------------------------------
bool Direct3DIndexBuffer::CreateNativeBuffer(const uint8_t* initialData, bool aliasAsTexelArray)
{
	// Calculate the total size of the required buffer
	auto totalBufferSize = _indexAttributeInfo.dataTypeByteSize * _indexAttributeInfo.indexCount;

	// Retrieve the initial data buffer if required
	if ((initialData == nullptr) && !_initialDataBuffer.empty())
	{
		initialData = _initialDataBuffer.data();
	}

	// Allocate the index buffer
	D3D11_BUFFER_DESC bufferDescription = {};
	bufferDescription.Usage = _usageType;
	bufferDescription.ByteWidth = (UINT)totalBufferSize;
	bufferDescription.BindFlags = D3D11_BIND_INDEX_BUFFER;
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
	HRESULT createIndexBufferReturn = _renderer->GetDevice()->CreateBuffer(&bufferDescription, subresourceDataPointer, &_indexBuffer);
	if (FAILED(createIndexBufferReturn))
	{
		_log->Error("Failed to create index buffer with error code {0}", createIndexBufferReturn);
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
void Direct3DIndexBuffer::CompletePendingDataWrites(ID3D11Device1* device, ID3D11DeviceContext1* context)
{
	// Create the native buffer if its creation is pending
	if (_nativeBufferCreationPending)
	{
		CreateNativeBuffer(nullptr, false);
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
		HRESULT mapReturn = context->Map(_indexBuffer.Get(), 0, (_allowDiscardOnPartialWrite ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE), 0, &mappedSubresource);
		if (FAILED(mapReturn))
		{
			_log->Error("Map operation failed when completing write operations for index buffer with error code {0}", mapReturn);
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
		context->Unmap(_indexBuffer.Get(), 0);
	}
}

//----------------------------------------------------------------------------------------
bool Direct3DIndexBuffer::CompletePendingDataWrite(const PendingWrite& pendingWrite, ID3D11Device1* device, ID3D11DeviceContext1* context, void* mappedBuffer)
{
	// Update the target region of the buffer
	const uint8_t* data = pendingWrite.data.data();
	size_t entryCount = pendingWrite.entryCount;
	size_t initialIndexNo = pendingWrite.initialIndexNo;
	size_t entryStrideInBytes = pendingWrite.entryStrideInBytes;
	if (pendingWrite.rawDataWrite)
	{
		if (_usingDynamicBuffer)
		{
			auto* mappedMemory = reinterpret_cast<uint8_t*>(mappedBuffer);
			mappedMemory += initialIndexNo;
			std::memcpy(mappedMemory, data, entryCount);
		}
		else
		{
			D3D11_BOX box;
			box.front = 0;
			box.back = 1;
			box.top = 0;
			box.bottom = 1;
			box.left = (UINT)initialIndexNo;
			box.right = box.left + (UINT)entryCount;
			context->UpdateSubresource(_indexBuffer.Get(), 0, &box, reinterpret_cast<const void*>(data), 0, 0);
		}
	}
	else
	{
		const IndexAttributeInfo& info = pendingWrite.attributeInfo;
		auto entrySizeInBytes = info.dataTypeByteSize;
		auto bufferStartPosInBytes = info.bufferOffsetInBytes + (initialIndexNo * entrySizeInBytes);
		if (_usingDynamicBuffer)
		{
			auto* mappedMemory = reinterpret_cast<uint8_t*>(mappedBuffer);
			mappedMemory += bufferStartPosInBytes;
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
		else if (entryStrideInBytes == entrySizeInBytes)
		{
			D3D11_BOX box;
			box.front = 0;
			box.back = 1;
			box.top = 0;
			box.bottom = 1;
			box.left = (UINT)bufferStartPosInBytes;
			box.right = box.left + (UINT)(entryCount * entrySizeInBytes);
			context->UpdateSubresource(_indexBuffer.Get(), 0, &box, reinterpret_cast<const void*>(data), 0, 0);
		}
		else
		{
			UINT currentBufferPos = (UINT)bufferStartPosInBytes;
			while (entryCount > 0)
			{
				D3D11_BOX box;
				box.front = 0;
				box.back = 1;
				box.top = 0;
				box.bottom = 1;
				box.left = currentBufferPos;
				box.right = box.left + (UINT)entrySizeInBytes;
				context->UpdateSubresource(_indexBuffer.Get(), 0, &box, reinterpret_cast<const void*>(data), 0, 0);

				currentBufferPos += (UINT)entrySizeInBytes;
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
void Direct3DIndexBuffer::FlagBuildStateModified()
{
	if (!_stateModified.test_and_set(std::memory_order_acquire))
	{
		_renderer->FlagObjectModified(this);
	}
}

//----------------------------------------------------------------------------------------
ID3D11Buffer* Direct3DIndexBuffer::GetNativeBuffer() const
{
	return _indexBuffer.Get();
}

} // namespace cobalt::graphics
