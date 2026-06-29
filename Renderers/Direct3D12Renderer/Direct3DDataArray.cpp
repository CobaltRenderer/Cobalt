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

	// Create the data array
	if (!CreateNativeBuffer())
	{
		_log->Error("Failed to create native objects for data array.");
		return false;
	}

	// If initial data has been provided, stage an upload of that data into the buffer.
	if (_initialDataSet && (_initialDataSizeInBytes != _totalBufferSizeInBytes))
	{
		_log->Error("Initial data array data of size {0} was provided, but {1} bytes are needed for the buffer.", _initialDataSizeInBytes, _totalBufferSizeInBytes);
		return false;
	}
	if (_initialDataSet)
	{
		// Allocate a new command list on the build queue to handle the initial data transfer
		CommandListHandle commandListHandle = _renderer->GetBuildCommandList();
		ID3D12GraphicsCommandList* commandList = commandListHandle.GetCommandList();

		// Create an upload buffer for the data
		ID3D12Resource* uploadBuffer = nullptr;
		_renderer->CreateTemporaryUploadBuffer(_initialDataSizeInBytes, uploadBuffer, false);
		if (uploadBuffer == nullptr)
		{
			_log->Error("Failed to create upload buffer for data array");
			return false;
		}

		// Map the upload buffer into the CPU address space
		uint8_t* uploadBufferMapped;
		CD3DX12_RANGE readRange(0, 0);
		if (FAILED(uploadBuffer->Map(0, &readRange, reinterpret_cast<void**>(&uploadBufferMapped))))
		{
			_log->Error("Failed to map upload buffer for data array");
		}

		// Write the initial data into the upload buffer
		PendingWrite pendingWrite(nullptr);
		pendingWrite.uploadBufferSizeInBytes = _initialDataSizeInBytes;
		WriteDataToMappedBuffer(pendingWrite, _initialData, uploadBufferMapped);

		// Unmap the upload buffer
		uploadBuffer->Unmap(0, nullptr);

		// Schedule a data transfer from the upload buffer to the target buffer. Note that we rely on implicit promotion
		// from COMMON here to skip a resource barrier. We also rely on the call to CompletePendingDataWrites by the
		// renderer before the first use to transition it into the default state.
		commandList->CopyBufferRegion(_buffer.Get(), 0, uploadBuffer, 0, _initialDataSizeInBytes);
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
		_readOnlyViewHandle.reset();
		_readWriteViewHandle.reset();
		_buffer.Reset();
		_counter.Reset();
		// D3D12MA requires releasing the resource before the allocation.
		// https://gpuopen-librariesandsdks.github.io/D3D12MemoryAllocator/html/quick_start.html#quick_start_resource_reference_counting
		_counterClearTransferBuffer.Reset();
		if (_counterClearTransferBufferAllocation != nullptr)
		{
			_counterClearTransferBufferAllocation->Release();
			_counterClearTransferBufferAllocation = nullptr;
		}
		_captureDataStagingBuffer.Reset();
		if (_captureDataStagingBufferAllocation != nullptr)
		{
			_captureDataStagingBufferAllocation->Release();
			_captureDataStagingBufferAllocation = nullptr;
		}
		_captureCounterStagingBuffer.Reset();
		if (_captureCounterStagingBufferAllocation != nullptr)
		{
			_captureCounterStagingBufferAllocation->Release();
			_captureCounterStagingBufferAllocation = nullptr;
		}
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

	// Capture the supplied update settings
	PendingWrite pendingWrite(transferBatchResolved);

	// If a transfer batch has been supplied, increment the usage count.
	if (transferBatchResolved != nullptr)
	{
		transferBatchResolved->IncrementUsageCount();
	}

	// Create an upload buffer for the data
	ID3D12Resource* uploadBuffer = nullptr;
	_renderer->CreateTemporaryUploadBuffer(sourceBufferSizeInBytes, uploadBuffer, true);
	if (uploadBuffer == nullptr)
	{
		_log->Error("Failed to create upload buffer for data array");
		return false;
	}

	// Map the upload buffer into the CPU address space
	uint8_t* uploadBufferMapped;
	CD3DX12_RANGE readRange(0, 0);
	if (FAILED(uploadBuffer->Map(0, &readRange, reinterpret_cast<void**>(&uploadBufferMapped))))
	{
		_log->Error("Failed to map upload buffer for data array");
		return false;
	}

	// Write the data into the upload buffer
	pendingWrite.uploadBufferSizeInBytes = sourceBufferSizeInBytes;
	WriteDataToMappedBuffer(pendingWrite, reinterpret_cast<const uint8_t*>(sourceBuffer), uploadBufferMapped);
	pendingWrite.uploadBuffer = uploadBuffer;
	pendingWrite.targetBufferPos = targetBufferOffsetInBytes;

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
	CreateCounterClearTransferBuffer((UINT)_state[_buildIndex].newCounterResetValue, _state[_buildIndex].counterClearTransferBuffer, _state[_buildIndex].counterClearTransferBufferAllocation);
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
bool Direct3DDataArray::HasCaptureTargets() const
{
	return !_state[_drawIndex].captureTargets.empty();
}

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
void Direct3DDataArray::CaptureDataBufferOutput(ID3D12GraphicsCommandList* commandList)
{
	// If no capture targets are defined, abort any further processing.
	if (_state[_drawIndex].captureTargets.empty())
	{
		return;
	}

	// Retrieve or create a staging buffer for the captured data
	ID3D12Resource* dataStagingBuffer = _captureDataStagingBuffer.Get();
	if (dataStagingBuffer == nullptr)
	{
		_renderer->CreatePersistentReadbackBuffer(_totalBufferSizeInBytes, _captureDataStagingBuffer, _captureDataStagingBufferAllocation);
		dataStagingBuffer = _captureDataStagingBuffer.Get();
	}

	// Retrieve or create a staging buffer for the counter value
	ID3D12Resource* counterStagingBuffer = _captureCounterStagingBuffer.Get();
	if (_hasCounter && (counterStagingBuffer == nullptr))
	{
		_renderer->CreatePersistentReadbackBuffer(sizeof(UINT), _captureCounterStagingBuffer, _captureCounterStagingBufferAllocation);
		counterStagingBuffer = _captureCounterStagingBuffer.Get();
	}

	// Read the counter value if a counter is present
	if (_hasCounter)
	{
		// Transition the buffer to the required resource state for reading
		auto acquireBarrier = CD3DX12_RESOURCE_BARRIER::Transition(_counter.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
		commandList->ResourceBarrier(1, &acquireBarrier);

		// Transfer the counter value to the counter staging buffer
		commandList->CopyBufferRegion(counterStagingBuffer, 0, _counter.Get(), 0, sizeof(UINT));

		// Transition the buffer to the last resource state
		auto releaseBarrier = CD3DX12_RESOURCE_BARRIER::Transition(_counter.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		commandList->ResourceBarrier(1, &releaseBarrier);
	}

	// Transfer each requested capture region into the data staging buffer
	for (auto* captureTarget : _state[_drawIndex].captureTargets)
	{
		// Retrieve the requested capture settings from our framebuffer output object
		size_t bufferStartPosInBytes = captureTarget->GetRequestedBufferOffset() * _structureStrideInBytes;
		size_t dataSizeInBytes = captureTarget->GetRequestedEntryCount() * _structureStrideInBytes;
		dataSizeInBytes = (dataSizeInBytes > 0 ? dataSizeInBytes : _totalBufferSizeInBytes);

		// Transition the buffer to the required resource state for reading
		auto acquireBarrier = CD3DX12_RESOURCE_BARRIER::Transition(_buffer.Get(), _lastResourceState, D3D12_RESOURCE_STATE_COPY_SOURCE);
		commandList->ResourceBarrier(1, &acquireBarrier);

		// Transfer the requested capture region into the staging buffer
		commandList->CopyBufferRegion(dataStagingBuffer, bufferStartPosInBytes, _buffer.Get(), bufferStartPosInBytes, dataSizeInBytes);

		// Transition the buffer to the last resource state
		auto releaseBarrier = CD3DX12_RESOURCE_BARRIER::Transition(_buffer.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, _lastResourceState);
		commandList->ResourceBarrier(1, &releaseBarrier);
	}
}

//----------------------------------------------------------------------------------------
void Direct3DDataArray::CompleteCaptureDataBufferOutput()
{
	// Reset our flag indicating this buffer has been added as a current resource buffer
	_addedAsCurrent = false;

	// If no capture targets are defined, abort any further processing.
	if (_state[_drawIndex].captureTargets.empty())
	{
		return;
	}

	// Read the counter value if a counter is present
	UINT counterValue = 0;
	if (_hasCounter)
	{
		// Map the staging buffer
		void* mappedBuffer;
		if (FAILED(_captureCounterStagingBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedBuffer))))
		{
			_log->Error("Failed to map staging buffer when attempting to capture data array counter value");
			return;
		}

		// Read the counter value from the buffer. Note that we use memcpy here rather than a simple cast to avoid
		// potentially misaligned memory access. While this should be legal on all platforms we care about, it's
		// technically undefined behaviour so we avoid it.
		std::memcpy(&counterValue, mappedBuffer, sizeof(counterValue));

		// Unmap the staging buffer
		CD3DX12_RANGE writeRange(0, 0);
		_captureCounterStagingBuffer->Unmap(0, &writeRange);
	}

	// Map the data staging buffer
	void* mappedBuffer;
	if (FAILED(_captureDataStagingBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedBuffer))))
	{
		_log->Error("Failed to map staging buffer when attempting to capture data array output");
		return;
	}

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
	CD3DX12_RANGE writeRange(0, 0);
	_captureDataStagingBuffer->Unmap(0, &writeRange);
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
void Direct3DDataArray::MigrateBuildStateToDrawState()
{
	std::swap(_buildIndex, _drawIndex);
	_stateModified.clear(std::memory_order_relaxed);

	// If a new reset value has been supplied for the counter, update it now.
	if (_state[_drawIndex].updatedCounterResetValue)
	{
		_state[_drawIndex].updatedCounterResetValue = false;
		_counterResetValue = _state[_drawIndex].newCounterResetValue;

		// D3D12MA requires releasing the resource before the allocation.
		// https://gpuopen-librariesandsdks.github.io/D3D12MemoryAllocator/html/quick_start.html#quick_start_resource_reference_counting
		_counterClearTransferBuffer.Reset();
		if (_counterClearTransferBufferAllocation != nullptr)
		{
			_counterClearTransferBufferAllocation->Release();
			_counterClearTransferBufferAllocation = nullptr;
		}

		_counterClearTransferBuffer = std::move(_state[_drawIndex].counterClearTransferBuffer);
		_state[_drawIndex].counterClearTransferBuffer.Reset();
		_counterClearTransferBufferAllocation = _state[_drawIndex].counterClearTransferBufferAllocation;
		_state[_drawIndex].counterClearTransferBufferAllocation = nullptr;
	}
}

//----------------------------------------------------------------------------------------
bool Direct3DDataArray::CreateNativeBuffer()
{
	// Determine the initial and final resource states to use for this data array. Note that we need to create the
	// buffer in the COMMON resource state as we're using the default heap.
	D3D12_RESOURCE_STATES finalResourceState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	if (((_performanceHintGpu & (PerformanceHint::ReadRarely | PerformanceHint::ReadNever)) != PerformanceHint(0)) && ((_performanceHintGpu & (PerformanceHint::WriteRarely | PerformanceHint::WriteNever)) != PerformanceHint(0)))
	{
		// If this buffer is indicated to be neither read from or written to frequently by the GPU, we consider this may
		// be a special purpose buffer, and take a closer look at the usage flags to determine appropriate resource
		// states to place the buffer in by default, to attempt to reduce stalls when transitioning the buffer.
		//##TODO## Further improvements could be made here. With buffers, it may be more appropriate to keep the buffer
		//in the last used state. This would have fixed the performance bottlenecks originally observed with indirect
		//drawing that prompted this change. We could also potentially populate a ring buffer with heuristics to
		//predict the next required resource state. In practice, most buffers likely remain in the one state, or
		//ping-pong back and forth between two states, so keeping a two-entry ring buffer of the actual last requested
		//states and using that to set the next resource state could be very effective. We could get "smarter" and
		//dynamically resize a vector, pushing back if we reach the end and the new requested state isn't the first
		//entry, or clearing the buffer if we reach a fixed maximum depth (say, 6). If we average a miss rate over a
		//certain threshold after a certain number of repeats, we could then give up and retain the last used state
		//until a new one is requested. This seems like a logical system, and we could wrap this concept into a generic
		//helper type which is shared between renderers in the common library.
		if (((_usageFlags & UsageFlags::IndirectDrawSource) != UsageFlags(0)) || ((_usageFlags & UsageFlags::IndirectDrawCountSource) != UsageFlags(0)))
		{
			finalResourceState = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
		}
		else if (((_usageFlags & UsageFlags::TransferSource) != UsageFlags(0)) && ((_usageFlags & UsageFlags::TransferDestination) != UsageFlags(0)))
		{
			finalResourceState = D3D12_RESOURCE_STATE_COPY_SOURCE | D3D12_RESOURCE_STATE_COPY_DEST;
		}
		else if ((_usageFlags & UsageFlags::TransferSource) != UsageFlags(0))
		{
			finalResourceState = D3D12_RESOURCE_STATE_COPY_SOURCE;
		}
		else if ((_usageFlags & UsageFlags::TransferDestination) != UsageFlags(0))
		{
			finalResourceState = D3D12_RESOURCE_STATE_COPY_DEST;
		}
	}
	D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_COMMON;

	// Allocate the data array
	auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto resourceDescription = CD3DX12_RESOURCE_DESC::Buffer(_totalBufferSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	HRESULT createDataArrayReturn = _renderer->GetDevice()->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDescription, initialResourceState, nullptr, IID_PPV_ARGS(&_buffer));
	if (FAILED(createDataArrayReturn))
	{
		_log->Error("Failed to create data array with error code {0}", createDataArrayReturn);
		return false;
	}

	// Allocate the counter for this data array. We need a counter for the very common pattern of pushing or
	// popping data from a stack. Without a counter attached, the HLSL methods IncrementCounter, DecrementCounter,
	// Append, and Consume will trigger a runtime error if they are used. In D3D11 a counter was attached as part of the
	// buffer using D3D11_BUFFER_UAV_FLAG_COUNTER, while in D3D12 we need to separately create a buffer and attach it if
	// we want one.
	if (_hasCounter)
	{
		// Create the counter buffer
		auto counterResourceDescription = CD3DX12_RESOURCE_DESC::Buffer(sizeof(UINT), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		HRESULT createDataArrayCounterReturn = _renderer->GetDevice()->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &counterResourceDescription, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&_counter));
		if (FAILED(createDataArrayCounterReturn))
		{
			_log->Error("Failed to create data array counter with error code {0}", createDataArrayCounterReturn);
			return false;
		}
		_counterResourceState = D3D12_RESOURCE_STATE_COMMON;

		// Create an upload buffer to reset the counter buffer efficiently during the draw process
		CreateCounterClearTransferBuffer((UINT)_counterResetValue, _counterClearTransferBuffer, _counterClearTransferBufferAllocation);
	}

	// Record the resource state for the buffer
	_lastResourceState = initialResourceState;
	_defaultResourceState = finalResourceState;

	// Since we lazily allocate resource views for this object on the heap, ensure we've pre-allocated the heaps we'll
	// be creating them on, so we don't get heap binding problems trying to create a new heap and use a resource from it
	// mid-frame.
	_renderer->EnsureHeapExists(Direct3DHeapManager::ResourceType::ShaderResourceView);
	_renderer->EnsureHeapExists(Direct3DHeapManager::ResourceType::UnorderedAccessView);
	return true;
}

//----------------------------------------------------------------------------------------
void Direct3DDataArray::CreateCounterClearTransferBuffer(UINT counterResetValue, Microsoft::WRL::ComPtr<ID3D12Resource>& counterClearTransferBuffer, D3D12MA::Allocation*& counterClearTransferBufferAllocation)
{
	// Create an upload buffer to reset the counter buffer efficiently during the draw process
	_renderer->CreatePersistentUploadBuffer(sizeof(UINT), counterClearTransferBuffer, counterClearTransferBufferAllocation);

	// Map the upload buffer into the CPU address space
	uint8_t* uploadBufferMapped;
	CD3DX12_RANGE readRange(0, 0);
	if (FAILED(counterClearTransferBuffer->Map(0, &readRange, reinterpret_cast<void**>(&uploadBufferMapped))))
	{
		_log->Error("Failed to map counter clear transfer buffer for data array");
	}

	// Write the counter reset value into the upload buffer
	std::memcpy(uploadBufferMapped, &counterResetValue, sizeof(UINT));

	// Unmap the upload buffer
	counterClearTransferBuffer->Unmap(0, nullptr);
}

//----------------------------------------------------------------------------------------
bool Direct3DDataArray::HasCounter() const
{
	return _hasCounter;
}

//----------------------------------------------------------------------------------------
void Direct3DDataArray::ResetCounter(ID3D12GraphicsCommandList* commandList)
{
	// Transition the counter buffer to the required resource state for writing
	auto acquireBarrier = CD3DX12_RESOURCE_BARRIER::Transition(_counter.Get(), _counterResourceState, D3D12_RESOURCE_STATE_COPY_DEST);
	commandList->ResourceBarrier(1, &acquireBarrier);

	// Schedule a data transfer from the counter reset upload buffer to the counter buffer
	commandList->CopyBufferRegion(_counter.Get(), 0, _counterClearTransferBuffer.Get(), 0, sizeof(UINT));

	// Transition the counter buffer to the required resource state for drawing
	auto releaseBarrier = CD3DX12_RESOURCE_BARRIER::Transition(_counter.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	commandList->ResourceBarrier(1, &releaseBarrier);
	_counterResourceState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
}

//----------------------------------------------------------------------------------------
size_t Direct3DDataArray::CompletePendingDataWrites(ID3D12GraphicsCommandList* commandList, D3DX12Residency::ResidencySet* residencySet)
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
		if (_lastResourceState != _defaultResourceState)
		{
			auto releaseBarrier = CD3DX12_RESOURCE_BARRIER::Transition(_buffer.Get(), _lastResourceState, _defaultResourceState);
			commandList->ResourceBarrier(1, &releaseBarrier);
			_lastResourceState = _defaultResourceState;
		}
		return 0;
	}

	// Perform each pending write operation
	return CompletePendingDataWritesInternal(readyWrites, commandList, residencySet, true);
}

//----------------------------------------------------------------------------------------
size_t Direct3DDataArray::CompletePendingDataWritesInternal(std::vector<PendingWrite>& pendingWrites, ID3D12GraphicsCommandList* commandList, D3DX12Residency::ResidencySet* residencySet, bool performDrawStateTransition)
{
	// Transition the buffer to the required resource state for writing
	if (_lastResourceState != D3D12_RESOURCE_STATE_COPY_DEST)
	{
		auto acquireBarrier = CD3DX12_RESOURCE_BARRIER::Transition(_buffer.Get(), _lastResourceState, D3D12_RESOURCE_STATE_COPY_DEST);
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
		auto releaseBarrier = CD3DX12_RESOURCE_BARRIER::Transition(_buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, _defaultResourceState);
		commandList->ResourceBarrier(1, &releaseBarrier);
		_lastResourceState = _defaultResourceState;
	}
	return 0;
}

//----------------------------------------------------------------------------------------
void Direct3DDataArray::WriteDataToMappedBuffer(const PendingWrite& pendingWrite, const uint8_t* data, uint8_t* mappedMemory)
{
	// Copy our data into the memory buffer
	std::memcpy(mappedMemory, data, pendingWrite.uploadBufferSizeInBytes);
}

//----------------------------------------------------------------------------------------
void Direct3DDataArray::CompletePendingDataWrite(ID3D12GraphicsCommandList* commandList, D3DX12Residency::ResidencySet* residencySet, const PendingWrite& pendingWrite)
{
	// Schedule a data transfer from the upload buffer to the target buffer
	commandList->CopyBufferRegion(_buffer.Get(), pendingWrite.targetBufferPos, pendingWrite.uploadBuffer, 0, pendingWrite.uploadBufferSizeInBytes);

	// If a transfer batch has been supplied, decrement the usage count.
	if (pendingWrite.transferBatch != nullptr)
	{
		pendingWrite.transferBatch->DecrementUsageCount();
	}
}

//----------------------------------------------------------------------------------------
size_t Direct3DDataArray::CompletePendingDataTransfers(ID3D12GraphicsCommandList* commandList, D3DX12Residency::ResidencySet* residencySet)
{
	// Obtain the set of pending data transfers. If no transfers are pending, abort any further processing.
	std::vector<PendingTransfer>& pendingTransfers = _state[_drawIndex].pendingTransfers;
	if (pendingTransfers.empty())
	{
		return 0;
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
		return 0;
	}

	// Transition the buffer to the required resource state for reading
	if (_lastResourceState != D3D12_RESOURCE_STATE_COPY_SOURCE)
	{
		auto acquireBarrier = CD3DX12_RESOURCE_BARRIER::Transition(_buffer.Get(), _lastResourceState, D3D12_RESOURCE_STATE_COPY_SOURCE);
		_lastResourceState = D3D12_RESOURCE_STATE_COPY_SOURCE;
		commandList->ResourceBarrier(1, &acquireBarrier);
	}

	// Complete each pending data transfer
	for (const PendingTransfer& transfer : readyTransfers)
	{
		CompletePendingDataTransfer(commandList, residencySet, transfer);
	}
	return 0;
}

//----------------------------------------------------------------------------------------
void Direct3DDataArray::CompletePendingDataTransfer(ID3D12GraphicsCommandList* commandList, D3DX12Residency::ResidencySet* residencySet, const PendingTransfer& pendingTransfer)
{
	// Transition the target buffer to the required resource state for writing
	if (pendingTransfer.targetBuffer->_lastResourceState != D3D12_RESOURCE_STATE_COPY_DEST)
	{
		auto acquireBarrier = CD3DX12_RESOURCE_BARRIER::Transition(pendingTransfer.targetBuffer->_buffer.Get(), pendingTransfer.targetBuffer->_lastResourceState, D3D12_RESOURCE_STATE_COPY_DEST);
		commandList->ResourceBarrier(1, &acquireBarrier);
		pendingTransfer.targetBuffer->_lastResourceState = D3D12_RESOURCE_STATE_COPY_DEST;
	}

	// Schedule a data transfer from this buffer to the target buffer
	commandList->CopyBufferRegion(pendingTransfer.targetBuffer->_buffer.Get(), pendingTransfer.targetBufferPosInBytes, _buffer.Get(), pendingTransfer.sourceBufferPosInBytes, pendingTransfer.transferCountInBytes);

	// If a transfer batch has been supplied, decrement the usage count.
	if (pendingTransfer.transferBatch != nullptr)
	{
		pendingTransfer.transferBatch->DecrementUsageCount();
	}
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
ID3D12Resource* Direct3DDataArray::GetNativeBuffer() const
{
	return _buffer.Get();
}

//----------------------------------------------------------------------------------------
ID3D12Resource* Direct3DDataArray::GetCounterBuffer() const
{
	return _counter.Get();
}

//----------------------------------------------------------------------------------------
D3D12_RESOURCE_STATES Direct3DDataArray::GetLastResourceState() const
{
	return _lastResourceState;
}

//----------------------------------------------------------------------------------------
const CD3DX12_GPU_DESCRIPTOR_HANDLE& Direct3DDataArray::GetReadOnlyGPUDescriptorHandle(ID3D12GraphicsCommandList* commandList, D3D_SHADER_INPUT_TYPE type)
{
	// Create the view if required
	if (!_createdReadOnlyView)
	{
		// Determine the parameters to use when creating this buffer view. We need to know the type being used in the
		// shader here to determine if a raw buffer view needs to be created.
		D3D12_BUFFER_SRV_FLAGS bufferFlags = D3D12_BUFFER_SRV_FLAG_NONE;
		DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
		UINT numElements = (UINT)_structureEntryCount;
		UINT structureByteStride = (UINT)_structureStrideInBytes;
		if (type == D3D_SIT_BYTEADDRESS)
		{
			bufferFlags = D3D12_BUFFER_SRV_FLAG_RAW;
			format = DXGI_FORMAT_R32_TYPELESS;
			numElements = (UINT)(_totalBufferSizeInBytes >> 2);
			structureByteStride = 0;
		}

		// Create the buffer view
		_readOnlyViewHandle = _renderer->AllocateDescriptor(Direct3DHeapManager::ResourceType::ShaderResourceView);
		D3D12_SHADER_RESOURCE_VIEW_DESC viewDescription{};
		viewDescription.Format = format;
		viewDescription.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		viewDescription.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		viewDescription.Buffer.FirstElement = 0;
		viewDescription.Buffer.NumElements = numElements;
		viewDescription.Buffer.StructureByteStride = structureByteStride;
		viewDescription.Buffer.Flags = bufferFlags;
		_renderer->GetDevice()->CreateShaderResourceView(_buffer.Get(), &viewDescription, _readOnlyViewHandle->GetNativeCPUHandle());
		_createdReadOnlyView = true;
	}

	// Transition the buffer to the required resource state for reading
	if (_lastResourceState != (D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE))
	{
		auto resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(_buffer.Get(), _lastResourceState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList->ResourceBarrier(1, &resourceBarrier);
		_lastResourceState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	}

	// Return the descriptor handle to the caller
	return _readOnlyViewHandle->GetNativeGPUHandle();
}

//----------------------------------------------------------------------------------------
const CD3DX12_GPU_DESCRIPTOR_HANDLE& Direct3DDataArray::GetReadWriteGPUDescriptorHandle(ID3D12GraphicsCommandList* commandList, D3D_SHADER_INPUT_TYPE type)
{
	// Create the view if required
	if (!_createdReadWriteView)
	{
		// Determine the parameters to use when creating this buffer view. We need to know the type being used in the
		// shader here to determine if a raw buffer view needs to be created.
		D3D12_BUFFER_UAV_FLAGS bufferFlags = D3D12_BUFFER_UAV_FLAG_NONE;
		DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
		UINT numElements = (UINT)_structureEntryCount;
		UINT structureByteStride = (UINT)_structureStrideInBytes;
		if (type == D3D_SIT_UAV_RWBYTEADDRESS)
		{
			bufferFlags = D3D12_BUFFER_UAV_FLAG_RAW;
			format = DXGI_FORMAT_R32_TYPELESS;
			numElements = (UINT)(_totalBufferSizeInBytes >> 2);
			structureByteStride = 0;
		}

		// Create the buffer view
		_readWriteViewHandle = _renderer->AllocateDescriptor(Direct3DHeapManager::ResourceType::UnorderedAccessView);
		D3D12_UNORDERED_ACCESS_VIEW_DESC resourceBufferViewDescription = {};
		resourceBufferViewDescription.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		resourceBufferViewDescription.Format = format;
		resourceBufferViewDescription.Buffer.FirstElement = 0;
		resourceBufferViewDescription.Buffer.NumElements = numElements;
		resourceBufferViewDescription.Buffer.StructureByteStride = structureByteStride;
		resourceBufferViewDescription.Buffer.CounterOffsetInBytes = 0;
		resourceBufferViewDescription.Buffer.Flags = bufferFlags;
		_renderer->GetDevice()->CreateUnorderedAccessView(_buffer.Get(), (_hasCounter ? _counter.Get() : nullptr), &resourceBufferViewDescription, _readWriteViewHandle->GetNativeCPUHandle());
		_createdReadWriteView = true;
	}

	// Transition the buffer to the required resource state for writing
	if (_lastResourceState != D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
	{
		auto resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(_buffer.Get(), _lastResourceState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		commandList->ResourceBarrier(1, &resourceBarrier);
		_lastResourceState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	}

	// Return the descriptor handle to the caller
	return _readWriteViewHandle->GetNativeGPUHandle();
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

} // namespace cobalt::graphics
