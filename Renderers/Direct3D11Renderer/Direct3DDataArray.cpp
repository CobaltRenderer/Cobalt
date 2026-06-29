// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "Direct3DDataArray.h"
#include "Direct3DDataArrayOutput.h"
#include "Direct3DRenderer.h"
#include "Direct3DTransferBatch.h"
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <Internal/RendererSupport/KnownDynamicCast.h>
#include <algorithm>
#include <cstring>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
Direct3DDataArray::Direct3DDataArray(cobalt::logging::ILogger* log, Direct3DRenderer* renderer)
: _log(log), _renderer(renderer), _buildIndex(0), _drawIndex(1)
{}

//----------------------------------------------------------------------------------------
Direct3DDataArray::~Direct3DDataArray()
{
	ReleaseMemory();
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void Direct3DDataArray::Delete()
{
	_renderer->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
SuccessToken Direct3DDataArray::AllocateMemory()
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

	// Determine if we should use deferred buffer creation
	bool deferBufferCreation = _renderer->UseDeferredBufferCreation();

	// Obtain any initial data for the buffer
	const uint8_t* initialData = nullptr;
	if (_initialDataSet && (_initialDataSizeInBytes != _totalBufferSizeInBytes))
	{
		_log->Error("Initial data array data of size {0} was provided, but {1} bytes are needed for the buffer.", _initialDataSizeInBytes, _totalBufferSizeInBytes);
		return false;
	}
	if (_initialDataSet)
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

	// Determine the usage flag to use when defining the Direct3D buffer. Note that as per the documentation, the
	// D3D11_RESOURCE_MISC_FLAG cannot be used when creating resources with D3D11_CPU_ACCESS flags. Since we need to
	// create the buffer using D3D11_RESOURCE_MISC_BUFFER_STRUCTURED and possibly D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS,
	// we therefore must keep cpu flags to 0 here.
	PerformanceHint performanceHintCpu = _performanceHintCpu;
	UINT cpuFlags;
	D3D11_USAGE usageType;
	switch (performanceHintCpu & PerformanceHint::WriteFlagsMask)
	{
	case PerformanceHint::WriteNever:
		usageType = D3D11_USAGE_IMMUTABLE;
		cpuFlags = 0;
		break;
	default:
	case PerformanceHint::Default:
	case PerformanceHint::WriteRarely:
	case PerformanceHint::WriteOften:
		usageType = D3D11_USAGE_DEFAULT;
		cpuFlags = 0;
		break;
	}
	_cpuFlags = cpuFlags;
	_usageType = usageType;

	// Create the buffer immediately if requested
	_nativeBufferCreationPending = deferBufferCreation;
	if (!deferBufferCreation)
	{
		if (!CreateNativeBuffer(initialData))
		{
			_log->Error("Failed to create native objects for data array.");
			return false;
		}
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
void Direct3DDataArray::ReleaseMemory()
{
	// Delete our created buffer object
	if (_bufferCreated)
	{
		_buffer.Reset();
		_bufferCreated = false;
	}
}

//----------------------------------------------------------------------------------------
void Direct3DDataArray::SetBufferLayout(size_t entryStrideInBytes, size_t entryCount, bool hasCounter, uint32_t counterResetValue)
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
void Direct3DDataArray::SetUsageFlags(UsageFlags usageFlags)
{
	_usageFlags = usageFlags;
}

//----------------------------------------------------------------------------------------
void Direct3DDataArray::SetPerformanceHints(PerformanceHint performanceHintCpu, PerformanceHint performanceHintGpu)
{
	_performanceHintCpu = performanceHintCpu;
	_performanceHintGpu = performanceHintGpu;
}

//----------------------------------------------------------------------------------------
void Direct3DDataArray::SetDataPersistenceFlags(DataPersistenceFlags dataPersistenceFlags)
{
	_dataPersistenceFlags = dataPersistenceFlags;
}

//----------------------------------------------------------------------------------------
// Initial data methods
//----------------------------------------------------------------------------------------
SuccessToken Direct3DDataArray::SetInitialData(const void* sourceBuffer, size_t sourceBufferSizeInBytes)
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
SuccessToken Direct3DDataArray::QueueDataUpdate(const void* sourceBuffer, size_t sourceBufferSizeInBytes, size_t targetBufferOffsetInBytes, ITransferBatch* transferBatch)
{
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

	// If a transfer batch has been supplied, ensure it hasn't already been submitted.
	auto* transferBatchResolved = KnownDynamicCast<Direct3DTransferBatch*>(transferBatch);
	if ((transferBatchResolved != nullptr) && transferBatchResolved->IsSubmitted())
	{
		_log->Error("Attempted to queue a transfer using a transfer batch that has already been submitted");
		return false;
	}

	// Capture the supplied update settings and data
	const auto* sourceBufferAsByteArray = reinterpret_cast<const uint8_t*>(sourceBuffer);
	PendingWrite pendingWrite(transferBatchResolved);
	pendingWrite.targetBufferPos = targetBufferOffsetInBytes;
	pendingWrite.data.assign(sourceBufferAsByteArray, sourceBufferAsByteArray + sourceBufferSizeInBytes);

	// If a transfer batch has been supplied, increment the usage count.
	if (transferBatchResolved != nullptr)
	{
		transferBatchResolved->IncrementUsageCount();
	}

	// Queue a task to update the buffer with the supplied data
	std::unique_lock<std::mutex> lock(_buildStateMutex);
	_state[_buildIndex].pendingWrites.push_back(pendingWrite);
	lock.unlock();
	FlagBuildStateModified();
	return true;
}

//----------------------------------------------------------------------------------------
void Direct3DDataArray::UpdateCounterResetValue(uint32_t counterResetValue)
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
SuccessToken Direct3DDataArray::QueueDataTransfer(IDataArray* targetBuffer, size_t transferCountInBytes, size_t sourceBufferOffsetInBytes, size_t targetBufferOffsetInBytes, ITransferBatch* transferBatch)
{
	// Ensure the source and target buffers allow a transfer
	auto* targetBufferResolved = KnownDynamicCast<Direct3DDataArray*>(targetBuffer);
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

	// If a transfer batch has been supplied, ensure it hasn't already been submitted.
	auto* transferBatchResolved = KnownDynamicCast<Direct3DTransferBatch*>(transferBatch);
	if ((transferBatchResolved != nullptr) && transferBatchResolved->IsSubmitted())
	{
		_log->Error("Attempted to queue a transfer using a transfer batch that has already been submitted");
		return false;
	}

	// Capture the supplied update settings and data
	PendingTransfer pendingTransfer(targetBufferResolved, transferBatchResolved);
	pendingTransfer.transferCountInBytes = transferCountInBytes;
	pendingTransfer.sourceBufferPosInBytes = sourceBufferOffsetInBytes;
	pendingTransfer.targetBufferPosInBytes = targetBufferOffsetInBytes;

	// If a transfer batch has been supplied, increment the usage count.
	if (transferBatchResolved != nullptr)
	{
		transferBatchResolved->IncrementUsageCount();
	}

	// Queue a task to perform the data transfer
	std::unique_lock<std::mutex> lock(_buildStateMutex);
	_state[_buildIndex].pendingTransfers.push_back(pendingTransfer);
	lock.unlock();
	FlagBuildStateModified();
	return true;
}

//----------------------------------------------------------------------------------------
// Output capture methods
//----------------------------------------------------------------------------------------
void Direct3DDataArray::AddOutputCaptureTarget(IDataArrayOutput* captureTarget)
{
	std::unique_lock<std::mutex> lock(_accessMutex);
	_state[_buildIndex].captureTargets.push_back(KnownDynamicCast<Direct3DDataArrayOutput*>(captureTarget));
	lock.unlock();
	FlagBuildStateModified();
}

//----------------------------------------------------------------------------------------
void Direct3DDataArray::RemoveOutputCaptureTarget(IDataArrayOutput* captureTarget)
{
	std::unique_lock<std::mutex> lock(_accessMutex);
	auto* captureTargetResolved = KnownDynamicCast<Direct3DDataArrayOutput*>(captureTarget);
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
void Direct3DDataArray::CaptureDataBufferOutput(ID3D11Device1* device, ID3D11DeviceContext1* context)
{
	// Reset our flag indicating this buffer has been added as a current resource buffer
	_addedAsCurrent = false;

	// If no capture targets are defined, abort any further processing.
	if (_state[_drawIndex].captureTargets.empty())
	{
		return;
	}

	// Retrieve or create a staging buffer for the captured data
	ID3D11Buffer* dataStagingBuffer = _captureDataStagingBuffer.Get();
	if (dataStagingBuffer == nullptr)
	{
		D3D11_BUFFER_DESC bufferDescription;
		bufferDescription.ByteWidth = (UINT)_totalBufferSizeInBytes;
		bufferDescription.Usage = D3D11_USAGE_STAGING;
		bufferDescription.BindFlags = 0;
		bufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		bufferDescription.MiscFlags = 0;
		bufferDescription.StructureByteStride = 0;
		HRESULT createDataArrayCaptureStagingBufferReturn = device->CreateBuffer(&bufferDescription, nullptr, &_captureDataStagingBuffer);
		if (FAILED(createDataArrayCaptureStagingBufferReturn))
		{
			_log->Error("Failed to create staging buffer for data array capture with error code {0}", createDataArrayCaptureStagingBufferReturn);
			return;
		}
		dataStagingBuffer = _captureDataStagingBuffer.Get();
	}

	// Retrieve or create a staging buffer for the counter value
	ID3D11Buffer* counterStagingBuffer = _captureCounterStagingBuffer.Get();
	UINT counterValue = 0;
	if (_hasCounter && (counterStagingBuffer == nullptr))
	{
		D3D11_BUFFER_DESC bufferDescription;
		bufferDescription.ByteWidth = sizeof(UINT);
		bufferDescription.Usage = D3D11_USAGE_STAGING;
		bufferDescription.BindFlags = 0;
		bufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		bufferDescription.MiscFlags = 0;
		bufferDescription.StructureByteStride = 0;
		HRESULT createDataArrayCounterCaptureStagingBufferReturn = device->CreateBuffer(&bufferDescription, nullptr, &_captureCounterStagingBuffer);
		if (FAILED(createDataArrayCounterCaptureStagingBufferReturn))
		{
			_log->Error("Failed to create staging buffer for data array counter capture with error code {0}", createDataArrayCounterCaptureStagingBufferReturn);
			return;
		}
		counterStagingBuffer = _captureCounterStagingBuffer.Get();
	}

	// Read the counter value if a counter is present
	auto* readWriteView = (_createdReadWriteViewWithCounter ? _readWriteViewWithCounter.Get() : _readWriteViewAppend.Get());
	if (_hasCounter && (counterStagingBuffer != nullptr) && (readWriteView != nullptr))
	{
		// Transfer the counter value to the counter staging buffer
		context->CopyStructureCount(counterStagingBuffer, 0, readWriteView);

		// Map the staging buffer
		const void* mappedBuffer = nullptr;
		D3D11_MAPPED_SUBRESOURCE mappedSubresource;
		if (FAILED(context->Map(_captureCounterStagingBuffer.Get(), 0, D3D11_MAP_READ, 0, &mappedSubresource)))
		{
			_log->Error("Failed to map staging buffer when attempting to capture data array counter value");
			return;
		}
		mappedBuffer = mappedSubresource.pData;

		// Read the counter value from the buffer. Note that we use memcpy here rather than a simple cast to avoid
		// potentially misaligned memory access. While this should be legal on all platforms we care about, it's
		// technically undefined behaviour so we avoid it.
		std::memcpy(&counterValue, mappedBuffer, sizeof(counterValue));

		// Unmap the staging buffer
		context->Unmap(_captureCounterStagingBuffer.Get(), 0);
	}

	// Transfer each requested capture region into the data staging buffer
	for (auto* captureTarget : _state[_drawIndex].captureTargets)
	{
		// Retrieve the requested capture settings from our framebuffer output object
		size_t bufferStartPosInBytes = captureTarget->GetRequestedBufferOffset() * _structureStrideInBytes;
		size_t dataSizeInBytes = captureTarget->GetRequestedEntryCount() * _structureStrideInBytes;
		dataSizeInBytes = (dataSizeInBytes > 0 ? dataSizeInBytes : _totalBufferSizeInBytes);

		// Transfer the requested capture region into the staging buffer
		D3D11_BOX box;
		box.front = 0;
		box.back = 1;
		box.top = 0;
		box.bottom = 1;
		box.left = (UINT)bufferStartPosInBytes;
		box.right = box.left + (UINT)dataSizeInBytes;
		context->CopySubresourceRegion(_captureDataStagingBuffer.Get(), 0, (UINT)bufferStartPosInBytes, 0, 0, _buffer.Get(), 0, &box);
	}

	// Map the data staging buffer
	const void* mappedBuffer = nullptr;
	D3D11_MAPPED_SUBRESOURCE mappedSubresource;
	if (FAILED(context->Map(_captureDataStagingBuffer.Get(), 0, D3D11_MAP_READ, 0, &mappedSubresource)))
	{
		_log->Error("Failed to map staging buffer when attempting to capture data array output");
		return;
	}
	mappedBuffer = mappedSubresource.pData;

	// Store the buffer data and counter values into each attached capture target
	const auto* mappedMemory = reinterpret_cast<const uint8_t*>(mappedBuffer);
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
	context->Unmap(_captureDataStagingBuffer.Get(), 0);
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
void Direct3DDataArray::MigrateBuildStateToDrawState()
{
	std::swap(_buildIndex, _drawIndex);
	_stateModified.clear(std::memory_order_relaxed);

	// Ensure any updated capture targets are carried over to the new build state
	_state[_buildIndex].captureTargets = _state[_drawIndex].captureTargets;

	// If a new reset value has been supplied for the counter, update it now.
	if (_state[_drawIndex].updatedCounterResetValue)
	{
		_state[_drawIndex].updatedCounterResetValue = false;
		_counterResetValue = _state[_drawIndex].newCounterResetValue;
	}
}

//----------------------------------------------------------------------------------------
bool Direct3DDataArray::CreateNativeBuffer(const uint8_t* initialData)
{
	// Retrieve the initial data buffer if required
	if ((initialData == nullptr) && !_initialDataBuffer.empty())
	{
		initialData = _initialDataBuffer.data();
	}

	// Allocate the data array
	bool rawBuffer = false;
	D3D11_BUFFER_DESC bufferDescription = {};
	bufferDescription.Usage = _usageType;
	bufferDescription.ByteWidth = (UINT)_totalBufferSizeInBytes;
	//##FIX## Should we be combining with D3D11_BIND_SHADER_RESOURCE here, and for other resources where they are used
	//from shaders?
	bufferDescription.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	bufferDescription.CPUAccessFlags = _cpuFlags;
	if ((_usageFlags & UsageFlags::IndirectDrawSource) == UsageFlags::IndirectDrawSource)
	{
		// Note that D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS cannot be combined with D3D11_RESOURCE_MISC_BUFFER_STRUCTURED
		// as stated in the documentation here:
		// https://docs.microsoft.com/en-us/windows/win32/direct3d11/overviews-direct3d-11-resources-intro
		// This is a bit of a pain, but it means buffers requested as indirect draw buffers must always created as raw
		// buffers.
		bufferDescription.MiscFlags = D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS | D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
		rawBuffer = true;
	}
	else if ((_usageFlags & UsageFlags::AtomicOperations) == UsageFlags::AtomicOperations)
	{
		bufferDescription.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
		rawBuffer = true;
	}
	else
	{
		bufferDescription.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	}
	bufferDescription.StructureByteStride = (!rawBuffer ? (UINT)_structureStrideInBytes : 0);
	D3D11_SUBRESOURCE_DATA subresourceData = {};
	D3D11_SUBRESOURCE_DATA* subresourceDataPointer = nullptr;
	if (initialData != nullptr)
	{
		subresourceData.pSysMem = initialData;
		subresourceData.SysMemPitch = 0;
		subresourceData.SysMemSlicePitch = 0;
		subresourceDataPointer = &subresourceData;
	}
	HRESULT createDataArrayBufferReturn = _renderer->GetDevice()->CreateBuffer(&bufferDescription, subresourceDataPointer, &_buffer);
	if (FAILED(createDataArrayBufferReturn))
	{
		_log->Error("Failed to create data array with error code {0}", createDataArrayBufferReturn);
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
void Direct3DDataArray::CompletePendingDataWrites(ID3D11Device1* device, ID3D11DeviceContext1* context)
{
	// Create the native buffer if its creation is pending
	if (_nativeBufferCreationPending)
	{
		CreateNativeBuffer(nullptr);
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

	// Complete any pending writes we need to perform in the buffer
	for (const PendingWrite& write : readyWrites)
	{
		CompletePendingDataWrite(write, device, context);
	}
}

//----------------------------------------------------------------------------------------
void Direct3DDataArray::CompletePendingDataWrite(const PendingWrite& pendingWrite, ID3D11Device1* device, ID3D11DeviceContext1* context)
{
	// Calculate the region of the buffer being modified
	const uint8_t* data = pendingWrite.data.data();
	auto dataSizeInBytes = pendingWrite.data.size();
	auto bufferStartPosInBytes = pendingWrite.targetBufferPos;

	// Perform the data write
	D3D11_BOX box;
	box.front = 0;
	box.back = 1;
	box.top = 0;
	box.bottom = 1;
	box.left = (UINT)bufferStartPosInBytes;
	box.right = box.left + (UINT)dataSizeInBytes;
	context->UpdateSubresource(_buffer.Get(), 0, &box, reinterpret_cast<const void*>(data), 0, 0);

	// If a transfer batch has been supplied, decrement the usage count.
	if (pendingWrite.transferBatch != nullptr)
	{
		pendingWrite.transferBatch->DecrementUsageCount();
	}
}

//----------------------------------------------------------------------------------------
void Direct3DDataArray::CompletePendingDataTransfers(ID3D11Device1* device, ID3D11DeviceContext1* context)
{
	// Create the native buffer if its creation is pending
	if (_nativeBufferCreationPending)
	{
		CreateNativeBuffer(nullptr);
		_nativeBufferCreationPending = false;
	}

	// Obtain the set of pending data transfers. If no transfers are pending, abort any further processing.
	std::vector<PendingTransfer>& pendingTransfers = _state[_drawIndex].pendingTransfers;
	if (pendingTransfers.empty())
	{
		return;
	}

	// Split pending transfers into those that are ready to run now, and those that must remain queued until their
	// batch has been submitted.
	std::vector<PendingTransfer> readyTransfers;
	std::vector<PendingTransfer> deferredTransfers;
	readyTransfers.reserve(pendingTransfers.size());
	deferredTransfers.reserve(pendingTransfers.size());
	for (PendingTransfer& transfer : pendingTransfers)
	{
		if ((transfer.transferBatch != nullptr) && !transfer.transferBatch->IsSubmitted())
		{
			deferredTransfers.push_back(transfer);
			continue;
		}
		readyTransfers.push_back(transfer);
	}
	pendingTransfers.clear();

	// Re-queue any deferred transfers onto the build state so they remain pending for a later frame.
	if (!deferredTransfers.empty())
	{
		std::unique_lock<std::mutex> lock(_buildStateMutex);
		auto& buildPendingTransfers = _state[_buildIndex].pendingTransfers;
		for (PendingTransfer& transfer : deferredTransfers)
		{
			buildPendingTransfers.push_back(transfer);
		}
		lock.unlock();
		FlagBuildStateModified();
	}
	if (readyTransfers.empty())
	{
		return;
	}

	// Complete any pending transfers we need to perform from the buffer
	for (const PendingTransfer& transfer : readyTransfers)
	{
		CompletePendingDataTransfer(transfer, device, context);
	}
}

//----------------------------------------------------------------------------------------
void Direct3DDataArray::CompletePendingDataTransfer(const PendingTransfer& pendingTransfer, ID3D11Device1* device, ID3D11DeviceContext1* context)
{
	// Perform the data transfer
	D3D11_BOX box;
	box.front = 0;
	box.back = 1;
	box.top = 0;
	box.bottom = 1;
	box.left = (UINT)pendingTransfer.sourceBufferPosInBytes;
	box.right = box.left + (UINT)pendingTransfer.transferCountInBytes;
	context->CopySubresourceRegion(pendingTransfer.targetBuffer->_buffer.Get(), 0, (UINT)pendingTransfer.targetBufferPosInBytes, 0, 0, _buffer.Get(), 0, &box);

	// If a transfer batch has been supplied, decrement the usage count.
	if (pendingTransfer.transferBatch != nullptr)
	{
		pendingTransfer.transferBatch->DecrementUsageCount();
	}
}

//----------------------------------------------------------------------------------------
ID3D11Buffer* Direct3DDataArray::GetNativeBuffer() const
{
	return _buffer.Get();
}

//----------------------------------------------------------------------------------------
ID3D11ShaderResourceView* Direct3DDataArray::GetReadOnlyView(D3D_SHADER_INPUT_TYPE type)
{
	// Create the view if required
	if (!_createdReadOnlyView)
	{
		if (type == D3D_SIT_BYTEADDRESS)
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC viewDescription = {};
			viewDescription.Format = DXGI_FORMAT_UNKNOWN;
			viewDescription.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
			viewDescription.BufferEx.FirstElement = 0;
			viewDescription.BufferEx.NumElements = (UINT)_totalBufferSizeInBytes >> 2;
			viewDescription.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
			if (FAILED(_renderer->GetDevice()->CreateShaderResourceView(_buffer.Get(), &viewDescription, &_readOnlyView)))
			{
				_log->Error("Failed to create shader resource view for data array");
			}
		}
		else
		{
			if (FAILED(_renderer->GetDevice()->CreateShaderResourceView(_buffer.Get(), nullptr, &_readOnlyView)))
			{
				_log->Error("Failed to create shader resource view for data array");
			}
		}
		_createdReadOnlyView = true;
	}

	// Return the buffer view to the caller
	return _readOnlyView.Get();
}

//----------------------------------------------------------------------------------------
ID3D11UnorderedAccessView* Direct3DDataArray::GetReadWriteView(D3D_SHADER_INPUT_TYPE type)
{
	// Select the target UAV based on the usage flags from the shader for this binding
	bool* createdTargetReadWriteView;
	ID3D11UnorderedAccessView* targetReadWriteView = nullptr;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>* targetReadWriteViewPointer = nullptr;
	bool createCounterUAV = false;
	bool createAppendUAV = false;
	bool createRawUAV = false;
	switch (type)
	{
	default:
		createdTargetReadWriteView = &_createdReadWriteView;
		targetReadWriteView = _readWriteView.Get();
		if (!*createdTargetReadWriteView)
		{
			targetReadWriteViewPointer = &_readWriteView;
		}
		break;
	case D3D_SIT_UAV_RWBYTEADDRESS:
		createdTargetReadWriteView = &_createdReadWriteView;
		targetReadWriteView = _readWriteView.Get();
		if (!*createdTargetReadWriteView)
		{
			targetReadWriteViewPointer = &_readWriteView;
			createRawUAV = true;
		}
		break;
	case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
		createdTargetReadWriteView = &_createdReadWriteViewWithCounter;
		targetReadWriteView = _readWriteViewWithCounter.Get();
		if (!*createdTargetReadWriteView)
		{
			targetReadWriteViewPointer = &_readWriteViewWithCounter;
			createCounterUAV = true;
		}
		break;
	case D3D_SIT_UAV_APPEND_STRUCTURED:
	case D3D_SIT_UAV_CONSUME_STRUCTURED:
		createdTargetReadWriteView = &_createdReadWriteViewAppend;
		targetReadWriteView = _readWriteViewAppend.Get();
		if (!*createdTargetReadWriteView)
		{
			targetReadWriteViewPointer = &_readWriteViewAppend;
			createAppendUAV = true;
		}
		break;
	}

	// Create the view if required
	if (!*createdTargetReadWriteView)
	{
		// Select the flags to use when creating the target UAV
		UINT flags = 0;
		DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
		UINT numElements = (UINT)_structureEntryCount;
		if (createCounterUAV)
		{
			flags = D3D11_BUFFER_UAV_FLAG_COUNTER;
		}
		else if (createAppendUAV)
		{
			flags = D3D11_BUFFER_UAV_FLAG_APPEND;
		}
		else if (createRawUAV)
		{
			flags = D3D11_BUFFER_UAV_FLAG_RAW;
			format = DXGI_FORMAT_R32_TYPELESS;
			numElements = (UINT)_totalBufferSizeInBytes >> 2;
		}

		// Create the target UAV
		D3D11_UNORDERED_ACCESS_VIEW_DESC viewDescription = {};
		viewDescription.Format = format;
		viewDescription.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		viewDescription.Buffer.FirstElement = 0;
		viewDescription.Buffer.NumElements = numElements;
		viewDescription.Buffer.Flags = flags;
		if (FAILED(_renderer->GetDevice()->CreateUnorderedAccessView(_buffer.Get(), &viewDescription, &(*targetReadWriteViewPointer))))
		{
			_log->Error("Failed to create unordered access view for data array");
		}
		*createdTargetReadWriteView = true;

		// Retrieve the allocated buffer view
		targetReadWriteView = targetReadWriteViewPointer->Get();
	}

	// Return the buffer view to the caller
	return targetReadWriteView;
}

//----------------------------------------------------------------------------------------
bool Direct3DDataArray::HasCounter() const
{
	return _hasCounter;
}

//----------------------------------------------------------------------------------------
UINT Direct3DDataArray::GetCounterResetValue() const
{
	return _counterResetValue;
}

//----------------------------------------------------------------------------------------
void Direct3DDataArray::FlagBuildStateModified()
{
	if (!_stateModified.test_and_set(std::memory_order_acquire))
	{
		_renderer->FlagObjectModified(this);
	}
}

//----------------------------------------------------------------------------------------
void Direct3DDataArray::AddAsCurrentBuffer()
{
	if (!_addedAsCurrent)
	{
		_renderer->AddCurrentDataArray(this);
		_addedAsCurrent = true;
	}
}

//----------------------------------------------------------------------------------------
UINT Direct3DDataArray::GetCurrentCounterValue(ID3D11Device1* device, ID3D11DeviceContext1* context)
{
	// Retrieve or create a staging buffer for the counter value
	ID3D11Buffer* counterStagingBuffer = _captureCounterStagingBuffer.Get();
	UINT counterValue = 0;
	if (_hasCounter && (counterStagingBuffer == nullptr))
	{
		D3D11_BUFFER_DESC bufferDescription;
		bufferDescription.ByteWidth = sizeof(UINT);
		bufferDescription.Usage = D3D11_USAGE_STAGING;
		bufferDescription.BindFlags = 0;
		bufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		bufferDescription.MiscFlags = 0;
		bufferDescription.StructureByteStride = 0;
		HRESULT createDataArrayCounterCaptureStagingBufferReturn = device->CreateBuffer(&bufferDescription, nullptr, &_captureCounterStagingBuffer);
		if (FAILED(createDataArrayCounterCaptureStagingBufferReturn))
		{
			_log->Error("Failed to create staging buffer for data array counter capture with error code {0}", createDataArrayCounterCaptureStagingBufferReturn);
			return 0;
		}
		counterStagingBuffer = _captureCounterStagingBuffer.Get();
	}

	// Read the counter value if a counter is present
	auto* readWriteView = (_createdReadWriteViewWithCounter ? _readWriteViewWithCounter.Get() : _readWriteViewAppend.Get());
	if (_hasCounter && (counterStagingBuffer != nullptr) && (readWriteView != nullptr))
	{
		// Transfer the counter value to the counter staging buffer
		context->CopyStructureCount(counterStagingBuffer, 0, readWriteView);

		// Map the staging buffer
		const void* mappedBuffer = nullptr;
		D3D11_MAPPED_SUBRESOURCE mappedSubresource;
		if (FAILED(context->Map(_captureCounterStagingBuffer.Get(), 0, D3D11_MAP_READ, 0, &mappedSubresource)))
		{
			_log->Error("Failed to map staging buffer when attempting to capture data array counter value");
			return 0;
		}
		mappedBuffer = mappedSubresource.pData;

		// Read the counter value from the buffer. Note that we use memcpy here rather than a simple cast to avoid
		// potentially misaligned memory access. While this should be legal on all platforms we care about, it's
		// technically undefined behaviour so we avoid it.
		std::memcpy(&counterValue, mappedBuffer, sizeof(counterValue));

		// Unmap the staging buffer
		context->Unmap(_captureCounterStagingBuffer.Get(), 0);
	}

	// Return the retrieved counter value to the caller
	return counterValue;
}

//----------------------------------------------------------------------------------------
void Direct3DDataArray::GetCurrentBufferData(ID3D11Device1* device, ID3D11DeviceContext1* context, size_t bufferOffsetInBytes, void* targetBuffer, size_t byteCount)
{
	// Retrieve or create a staging buffer for the captured data
	ID3D11Buffer* dataStagingBuffer = _captureDataStagingBuffer.Get();
	if (dataStagingBuffer == nullptr)
	{
		D3D11_BUFFER_DESC bufferDescription;
		bufferDescription.ByteWidth = (UINT)_totalBufferSizeInBytes;
		bufferDescription.Usage = D3D11_USAGE_STAGING;
		bufferDescription.BindFlags = 0;
		bufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		bufferDescription.MiscFlags = 0;
		bufferDescription.StructureByteStride = 0;
		HRESULT createDataArrayCaptureStagingBufferReturn = device->CreateBuffer(&bufferDescription, nullptr, &_captureDataStagingBuffer);
		if (FAILED(createDataArrayCaptureStagingBufferReturn))
		{
			_log->Error("Failed to create staging buffer for data array capture with error code {0}", createDataArrayCaptureStagingBufferReturn);
			return;
		}
		dataStagingBuffer = _captureDataStagingBuffer.Get();
	}

	// Transfer the requested capture region into the staging buffer
	D3D11_BOX box;
	box.front = 0;
	box.back = 1;
	box.top = 0;
	box.bottom = 1;
	box.left = (UINT)bufferOffsetInBytes;
	box.right = box.left + (UINT)byteCount;
	context->CopySubresourceRegion(_captureDataStagingBuffer.Get(), 0, (UINT)bufferOffsetInBytes, 0, 0, _buffer.Get(), 0, &box);

	// Map the data staging buffer
	const void* mappedBuffer = nullptr;
	D3D11_MAPPED_SUBRESOURCE mappedSubresource;
	if (FAILED(context->Map(_captureDataStagingBuffer.Get(), 0, D3D11_MAP_READ, 0, &mappedSubresource)))
	{
		_log->Error("Failed to map staging buffer when attempting to capture data array output");
		return;
	}
	mappedBuffer = mappedSubresource.pData;

	// Copy the requested buffer data into the target memory buffer
	const auto* mappedMemory = reinterpret_cast<const uint8_t*>(mappedBuffer);
	std::memcpy(targetBuffer, mappedMemory + bufferOffsetInBytes, byteCount);

	// Unmap the data staging buffer
	context->Unmap(_captureDataStagingBuffer.Get(), 0);
}

} // namespace cobalt::graphics
