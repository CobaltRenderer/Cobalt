// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "Direct3DTexelArray.h"
#include "Direct3DRenderer.h"
#include "Direct3DTexelArrayOutput.h"
#include "Direct3DTransferBatch.h"
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <Internal/TextureFormatConversion/TextureFormatConversion.pkg>
#include <Internal/RendererSupport/KnownDynamicCast.h>
#include <algorithm>
#include <cstring>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
Direct3DTexelArray::Direct3DTexelArray(cobalt::logging::ILogger* log, Direct3DRenderer* renderer)
: _log(log), _renderer(renderer), _buildIndex(0), _drawIndex(1)
{}

//----------------------------------------------------------------------------------------
Direct3DTexelArray::~Direct3DTexelArray()
{
	ReleaseMemory();
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void Direct3DTexelArray::Delete()
{
	_renderer->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
SuccessToken Direct3DTexelArray::AllocateMemory()
{
	// Ensure the array hasn't already been created
	if (_bufferCreated)
	{
		_log->Error("Attempted to allocate memory for a texel array that has already been allocated.");
		return false;
	}

	// Ensure the buffer layout has been set
	if (!_bufferLayoutSet)
	{
		_log->Error("Attempted to allocate memory for a texel array when the buffer layout has not been defined.");
		return false;
	}

	// Calculate the total size of the required buffer
	_totalBufferSizeInBytes = _structureEntryCount * _structureStrideInBytes;

	// Ensure that the buffer size is not zero
	if (_totalBufferSizeInBytes == 0)
	{
		_log->Error("Attempted to allocate an empty texel array.");
		return false;
	}

	// Calculate the internal data format to use
	if (!GetFormatNative(_imageFormat, _dataFormat, _nativeFormat))
	{
		_log->Error("Failed to allocate memory for texel array. The combination of image format {0} and data format {1} is not supported by this renderer.", _imageFormat, _dataFormat);
		return false;
	}

	// Create the data array
	if (!CreateNativeBuffer())
	{
		_log->Error("Failed to create native objects for texel array.");
		return false;
	}

	// If initial data has been provided, stage an upload of that data into the buffer.
	if (_initialDataSet && (_initialDataSizeInBytes != _totalBufferSizeInBytes))
	{
		_log->Error("Initial texel array data of size {0} was provided, but {1} bytes are needed for the buffer.", _initialDataSizeInBytes, _totalBufferSizeInBytes);
		return false;
	}
	if (_initialDataSet)
	{
		// Obtain any initial data for the buffer, performing image format conversion if required.
		bool convertInitialData = !ImageFormatsAreBinaryEquivalent(_initialDataImageFormat, _imageFormat) || !DataFormatsAreBinaryEquivalent(_initialDataDataFormat, _dataFormat);
		if (convertInitialData && !ConvertDataFormat(_initialData, _initialDataSizeInBytes, _initialDataImageFormat, _initialDataDataFormat, _imageFormat, _dataFormat, _initialDataBuffer))
		{
			_log->Error("AllocateMemory failed to convert initial data for texel array");
			return false;
		}
		const uint8_t* initialData = (convertInitialData ? _initialDataBuffer.data() : _initialData);
		size_t initialDataSizeInBytes = (convertInitialData ? _initialDataBuffer.size() : _initialDataSizeInBytes);

		// Allocate a new command list on the build queue to handle the initial data transfer
		CommandListHandle commandListHandle = _renderer->GetBuildCommandList();
		ID3D12GraphicsCommandList* commandList = commandListHandle.GetCommandList();

		// Create an upload buffer for the data
		ID3D12Resource* uploadBuffer = nullptr;
		_renderer->CreateTemporaryUploadBuffer(initialDataSizeInBytes, uploadBuffer, false);
		if (uploadBuffer == nullptr)
		{
			_log->Error("Failed to create upload buffer for texel array");
			return false;
		}

		// Map the upload buffer into the CPU address space
		uint8_t* uploadBufferMapped;
		CD3DX12_RANGE readRange(0, 0);
		if (FAILED(uploadBuffer->Map(0, &readRange, reinterpret_cast<void**>(&uploadBufferMapped))))
		{
			_log->Error("Failed to map upload buffer for texel array");
		}

		// Write the initial data into the upload buffer
		PendingWrite pendingWrite(nullptr);
		pendingWrite.uploadBufferSizeInBytes = initialDataSizeInBytes;
		WriteDataToMappedBuffer(pendingWrite, initialData, uploadBufferMapped);

		// Unmap the upload buffer
		uploadBuffer->Unmap(0, nullptr);

		// Schedule a data transfer from the upload buffer to the target buffer. Note that we rely on implicit promotion
		// from COMMON here to skip a resource barrier. We also rely on the call to CompletePendingDataWrites by the
		// renderer before the first use to transition it into the default state.
		commandList->CopyBufferRegion(_bufferWrapper->buffer, 0, uploadBuffer, 0, initialDataSizeInBytes);
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
bool Direct3DTexelArray::AllocateAsAliasForBuffer(BufferWrapper* bufferWrapper, size_t bufferSizeInBytes)
{
	// Ensure the array hasn't already been created
	if (_bufferCreated)
	{
		_log->Error("Attempted to allocate a texel array as an alias that has already been allocated.");
		return false;
	}

	// Ensure the buffer layout has been set
	if (!_bufferLayoutSet)
	{
		_log->Error("Attempted to allocate a texel array as an alias when the buffer layout has not been defined.");
		return false;
	}

	// Ensure initial data has not been provided
	if (_initialDataSet)
	{
		_log->Error("Failed to allocate texel array as alias because initial data has been set. Initial data cannot be provided for a texel array being used as an alias.");
		return false;
	}

	// Calculate the total size of the required buffer
	_totalBufferSizeInBytes = _structureEntryCount * _structureStrideInBytes;
	if (_totalBufferSizeInBytes > bufferSizeInBytes)
	{
		_log->Error("Failed to allocate texel array as alias because the layout requires {0} bytes of memory, but only {1} bytes are available in the buffer.", _totalBufferSizeInBytes, bufferSizeInBytes);
		return false;
	}

	// Calculate the internal data format to use
	if (!GetFormatNative(_imageFormat, _dataFormat, _nativeFormat))
	{
		_log->Error("Failed to allocate texel array as alias. The combination of image format {0} and data format {1} is not supported by this renderer.", _imageFormat, _dataFormat);
		return false;
	}

	// Since we lazily allocate resource views for this object on the heap, ensure we've pre-allocated the heaps we'll
	// be creating them on, so we don't get heap binding problems trying to create a new heap and use a resource from it
	// mid-frame.
	_renderer->EnsureHeapExists(Direct3DHeapManager::ResourceType::ShaderResourceView);
	_renderer->EnsureHeapExists(Direct3DHeapManager::ResourceType::UnorderedAccessView);

	// Flag that the buffer has been created
	_bufferCreated = true;
	_bufferWrapper = bufferWrapper;
	FlagBuildStateModified();
	return true;
}

//----------------------------------------------------------------------------------------
void Direct3DTexelArray::ReleaseMemory()
{
	// Delete our created buffer object
	if (_bufferCreated)
	{
		_readOnlyViewHandle.reset();
		_readWriteViewHandle.reset();
		// D3D12MA requires releasing the resource before the allocation.
		// https://gpuopen-librariesandsdks.github.io/D3D12MemoryAllocator/html/quick_start.html#quick_start_resource_reference_counting
		_captureDataStagingBuffer.Reset();
		if (_captureDataStagingBufferAllocation != nullptr)
		{
			_captureDataStagingBufferAllocation->Release();
			_captureDataStagingBufferAllocation = nullptr;
		}
		_buffer.Reset();
		_bufferWrapper = nullptr;
		_bufferCreated = false;
	}
}

//----------------------------------------------------------------------------------------
void Direct3DTexelArray::SetBufferLayout(ImageFormat imageFormat, DataFormat dataFormat, size_t entryCount)
{
	_imageFormat = imageFormat;
	_dataFormat = dataFormat;
	_structureEntryCount = entryCount;
	_structureStrideInBytes = ByteSizePerElementFromFormat(dataFormat) * ElementCountPerPixelFromFormat(imageFormat);
	_bufferLayoutSet = true;
}

//----------------------------------------------------------------------------------------
// Usage methods
//----------------------------------------------------------------------------------------
void Direct3DTexelArray::SetUsageFlags(UsageFlags usageFlags)
{
	_usageFlags = usageFlags;
}

//----------------------------------------------------------------------------------------
void Direct3DTexelArray::SetPerformanceHints(PerformanceHint performanceHintCpu, PerformanceHint performanceHintGpu)
{
	_performanceHintCpu = performanceHintCpu;
	_performanceHintGpu = performanceHintGpu;
}

//----------------------------------------------------------------------------------------
void Direct3DTexelArray::SetDataPersistenceFlags(DataPersistenceFlags dataPersistenceFlags)
{
	_dataPersistenceFlags = dataPersistenceFlags;
}

//----------------------------------------------------------------------------------------
// Initial data methods
//----------------------------------------------------------------------------------------
SuccessToken Direct3DTexelArray::SetInitialData(const void* sourceBuffer, size_t sourceBufferSizeInBytes, SourceImageFormat imageFormat, SourceDataFormat dataFormat)
{
	// Validate the current state of the buffer
	if (_bufferCreated)
	{
		_log->Error("Attempted to set initial texel array data after the buffer has already been created.");
		return false;
	}
	if (_initialDataSet)
	{
		_log->Error("Attempted to set initial texel array data when initial data has already been provided.");
		return false;
	}

	// Store the initial data for use when the buffer is allocated
	_initialData = reinterpret_cast<const uint8_t*>(sourceBuffer);
	_initialDataSizeInBytes = sourceBufferSizeInBytes;
	_initialDataImageFormat = imageFormat;
	_initialDataDataFormat = dataFormat;
	_initialDataSet = true;
	return true;
}

//----------------------------------------------------------------------------------------
// Data update methods
//----------------------------------------------------------------------------------------
SuccessToken Direct3DTexelArray::QueueDataUpdate(const void* sourceBuffer, size_t sourceBufferSizeInBytes, SourceImageFormat imageFormat, SourceDataFormat dataFormat, size_t targetBufferOffset, ITransferBatch* transferBatch)
{
	// Ensure the buffer is allowed to be modified
	if ((_performanceHintCpu & PerformanceHint::WriteFlagsMask) == PerformanceHint::WriteNever)
	{
		_log->Error("Attempted to update a texel array that can't be modified.");
		return false;
	}

	// Verify that the requested write is within the bounds of the buffer
	auto targetBufferOffsetInBytes = targetBufferOffset * _structureStrideInBytes;
	if ((targetBufferOffsetInBytes > _totalBufferSizeInBytes) || ((targetBufferOffsetInBytes + sourceBufferSizeInBytes) > _totalBufferSizeInBytes))
	{
		_log->Error("Attempted to perform a write outside the bounds of a texel array, with write location {0}, byte size {1}, against buffer size of {2}.", targetBufferOffsetInBytes, sourceBufferSizeInBytes, _totalBufferSizeInBytes);
		return false;
	}

	// If a transfer batch has been supplied, ensure it hasn't already been submitted.
	auto* transferBatchResolved = KnownDynamicCast<Direct3DTransferBatch*>(transferBatch);
	if ((transferBatchResolved != nullptr) && transferBatchResolved->IsSubmitted())
	{
		_log->Error("Attempted to queue a transfer using a transfer batch that has already been submitted");
		return false;
	}

	// Capture the supplied update settings and data, performing format conversion if required.
	PendingWrite pendingWrite(transferBatchResolved);
	pendingWrite.targetBufferPosInBytes = targetBufferOffsetInBytes;
	if (!ConvertDataFormat(sourceBuffer, sourceBufferSizeInBytes, imageFormat, dataFormat, _imageFormat, _dataFormat, pendingWrite.data))
	{
		_log->Error("QueueDataUpdate failed to convert source data");
		return false;
	}

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
		_log->Error("Failed to create upload buffer for texel array");
		return false;
	}

	// Map the upload buffer into the CPU address space
	uint8_t* uploadBufferMapped;
	CD3DX12_RANGE readRange(0, 0);
	if (FAILED(uploadBuffer->Map(0, &readRange, reinterpret_cast<void**>(&uploadBufferMapped))))
	{
		_log->Error("Failed to map upload buffer for texel array");
		return false;
	}

	// Write the data into the upload buffer
	pendingWrite.uploadBufferSizeInBytes = sourceBufferSizeInBytes;
	WriteDataToMappedBuffer(pendingWrite, reinterpret_cast<const uint8_t*>(sourceBuffer), uploadBufferMapped);
	pendingWrite.uploadBuffer = uploadBuffer;
	pendingWrite.targetBufferPosInBytes = targetBufferOffsetInBytes;

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
// Data transfer methods
//----------------------------------------------------------------------------------------
SuccessToken Direct3DTexelArray::QueueDataTransfer(ITexelArray* targetBuffer, size_t transferCount, size_t sourceBufferOffset, size_t targetBufferOffset, ITransferBatch* transferBatch)
{
	// Ensure the source and target buffers allow a transfer
	auto* targetBufferResolved = KnownDynamicCast<Direct3DTexelArray*>(targetBuffer);
	if ((_performanceHintGpu & PerformanceHint::ReadFlagsMask) == PerformanceHint::ReadNever)
	{
		_log->Error("Attempted to transfer from a texel array that can't be read on the GPU.");
		return false;
	}
	if ((targetBufferResolved->_performanceHintGpu & PerformanceHint::WriteFlagsMask) == PerformanceHint::WriteNever)
	{
		_log->Error("Attempted to transfer to a texel array that can't be modified on the GPU.");
		return false;
	}
	if ((_usageFlags & UsageFlags::TransferSource) != UsageFlags::TransferSource)
	{
		_log->Error("Attempted to transfer from a texel array that hasn't specified the TransferSource usage type.");
		return false;
	}
	if ((targetBufferResolved->_usageFlags & UsageFlags::TransferDestination) != UsageFlags::TransferDestination)
	{
		_log->Error("Attempted to transfer to a texel array that hasn't specified the TransferDestination usage type.");
		return false;
	}
	if (!ImageFormatsAreBinaryEquivalent(_imageFormat, targetBufferResolved->_imageFormat) || !DataFormatsAreBinaryEquivalent(_dataFormat, targetBufferResolved->_dataFormat))
	{
		_log->Error("Attempted to transfer between texel arrays with incompatible formats. Formats between buffers must be binary compatible to perform transfers.");
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
	auto sourceBufferOffsetInBytes = sourceBufferOffset * _structureStrideInBytes;
	auto targetBufferOffsetInBytes = targetBufferOffset * _structureStrideInBytes;
	auto transferCountInBytes = transferCount * _structureStrideInBytes;
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
// Data conversion methods
//----------------------------------------------------------------------------------------
bool Direct3DTexelArray::ConvertDataFormat(const void* sourceBuffer, size_t sourceBufferSizeInBytes, SourceImageFormat sourceImageFormat, SourceDataFormat sourceDataFormat, ImageFormat targetImageFormat, DataFormat targetDataFormat, std::vector<uint8_t>& targetBuffer) const
{
	// If the source and target image and data formats match, directly copy the source data to the target buffer.
	if (ImageFormatsAreBinaryEquivalent(sourceImageFormat, targetImageFormat) && DataFormatsAreBinaryEquivalent(sourceDataFormat, targetDataFormat))
	{
		targetBuffer.assign(reinterpret_cast<const uint8_t*>(sourceBuffer), reinterpret_cast<const uint8_t*>(sourceBuffer) + sourceBufferSizeInBytes);
		return true;
	}

	// Convert the data to the required format
	bool sourceDataFormatError = false;
	bool targetDataFormatError = false;
	bool result = TextureFormatConversion::ConvertTexelArrayInputData(sourceBuffer, sourceBufferSizeInBytes, sourceImageFormat, sourceDataFormat, targetImageFormat, targetDataFormat, targetBuffer, sourceDataFormatError, targetDataFormatError);
	if (targetDataFormatError)
	{
		_log->Critical("Data conversion wasn't handled for texel array with image format {0} and data format {1}", targetImageFormat, targetDataFormat);
	}
	if (sourceDataFormatError)
	{
		_log->Error("Attempted to write to texel array with incompatible or unsupported image format {0} and data format {1}", sourceImageFormat, sourceDataFormat);
	}
	return result;
}

//----------------------------------------------------------------------------------------
// Format methods
//----------------------------------------------------------------------------------------
constexpr bool Direct3DTexelArray::GetFormatNative(ImageFormat requestedImageFormat, DataFormat requestedDataFormat, DXGI_FORMAT& nativeFormat)
{
	switch (requestedDataFormat)
	{
	case DataFormat::Int8:
		switch (requestedImageFormat)
		{
		case ImageFormat::R:
		case ImageFormat::X:
			nativeFormat = DXGI_FORMAT_R8_SINT;
			return true;
		case ImageFormat::RG:
		case ImageFormat::XY:
			nativeFormat = DXGI_FORMAT_R8G8_SINT;
			return true;
		case ImageFormat::RGBA:
		case ImageFormat::XYZW:
			nativeFormat = DXGI_FORMAT_R8G8B8A8_SINT;
			return true;
		}
		break;
	case DataFormat::Int16:
		switch (requestedImageFormat)
		{
		case ImageFormat::R:
		case ImageFormat::X:
			nativeFormat = DXGI_FORMAT_R16_SINT;
			return true;
		case ImageFormat::RG:
		case ImageFormat::XY:
			nativeFormat = DXGI_FORMAT_R16G16_SINT;
			return true;
		case ImageFormat::RGBA:
		case ImageFormat::XYZW:
			nativeFormat = DXGI_FORMAT_R16G16B16A16_SINT;
			return true;
		}
		break;
	case DataFormat::Int32:
		switch (requestedImageFormat)
		{
		case ImageFormat::R:
		case ImageFormat::X:
			nativeFormat = DXGI_FORMAT_R32_SINT;
			return true;
		case ImageFormat::RG:
		case ImageFormat::XY:
			nativeFormat = DXGI_FORMAT_R32G32_SINT;
			return true;
		case ImageFormat::RGBA:
		case ImageFormat::XYZW:
			nativeFormat = DXGI_FORMAT_R32G32B32A32_SINT;
			return true;
		}
		break;
	case DataFormat::UInt8:
		switch (requestedImageFormat)
		{
		case ImageFormat::R:
		case ImageFormat::X:
			nativeFormat = DXGI_FORMAT_R8_UINT;
			return true;
		case ImageFormat::RG:
		case ImageFormat::XY:
			nativeFormat = DXGI_FORMAT_R8G8_UINT;
			return true;
		case ImageFormat::RGBA:
		case ImageFormat::XYZW:
			nativeFormat = DXGI_FORMAT_R8G8B8A8_UINT;
			return true;
		}
		break;
	case DataFormat::UInt16:
		switch (requestedImageFormat)
		{
		case ImageFormat::R:
		case ImageFormat::X:
			nativeFormat = DXGI_FORMAT_R16_UINT;
			return true;
		case ImageFormat::RG:
		case ImageFormat::XY:
			nativeFormat = DXGI_FORMAT_R16G16_UINT;
			return true;
		case ImageFormat::RGBA:
		case ImageFormat::XYZW:
			nativeFormat = DXGI_FORMAT_R16G16B16A16_UINT;
			return true;
		}
		break;
	case DataFormat::UInt32:
		switch (requestedImageFormat)
		{
		case ImageFormat::R:
		case ImageFormat::X:
			nativeFormat = DXGI_FORMAT_R32_UINT;
			return true;
		case ImageFormat::RG:
		case ImageFormat::XY:
			nativeFormat = DXGI_FORMAT_R32G32_UINT;
			return true;
		case ImageFormat::RGBA:
		case ImageFormat::XYZW:
			nativeFormat = DXGI_FORMAT_R32G32B32A32_UINT;
			return true;
		}
		break;
	case DataFormat::Norm8:
		switch (requestedImageFormat)
		{
		case ImageFormat::R:
		case ImageFormat::X:
			nativeFormat = DXGI_FORMAT_R8_SNORM;
			return true;
		case ImageFormat::RG:
		case ImageFormat::XY:
			nativeFormat = DXGI_FORMAT_R8G8_SNORM;
			return true;
		case ImageFormat::RGBA:
		case ImageFormat::XYZW:
			nativeFormat = DXGI_FORMAT_R8G8B8A8_SNORM;
			return true;
		}
		break;
	case DataFormat::UNorm8:
		switch (requestedImageFormat)
		{
		case ImageFormat::R:
		case ImageFormat::X:
			nativeFormat = DXGI_FORMAT_R8_UNORM;
			return true;
		case ImageFormat::RG:
		case ImageFormat::XY:
			nativeFormat = DXGI_FORMAT_R8G8_UNORM;
			return true;
		case ImageFormat::RGBA:
		case ImageFormat::XYZW:
			nativeFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
			return true;
		}
		break;
	case DataFormat::Float16:
		switch (requestedImageFormat)
		{
		case ImageFormat::R:
		case ImageFormat::X:
			nativeFormat = DXGI_FORMAT_R16_FLOAT;
			return true;
		case ImageFormat::RG:
		case ImageFormat::XY:
			nativeFormat = DXGI_FORMAT_R16G16_FLOAT;
			return true;
		case ImageFormat::RGBA:
		case ImageFormat::XYZW:
			nativeFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
			return true;
		}
		break;
	case DataFormat::Float32:
		switch (requestedImageFormat)
		{
		case ImageFormat::R:
		case ImageFormat::X:
			nativeFormat = DXGI_FORMAT_R32_FLOAT;
			return true;
		case ImageFormat::RG:
		case ImageFormat::XY:
			nativeFormat = DXGI_FORMAT_R32G32_FLOAT;
			return true;
		case ImageFormat::RGBA:
		case ImageFormat::XYZW:
			nativeFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
			return true;
		}
		break;
	}
	return false;
}

//----------------------------------------------------------------------------------------
// Output capture methods
//----------------------------------------------------------------------------------------
bool Direct3DTexelArray::HasCaptureTargets() const
{
	return !_state[_drawIndex].captureTargets.empty();
}

//----------------------------------------------------------------------------------------
void Direct3DTexelArray::AddOutputCaptureTarget(ITexelArrayOutput* captureTarget)
{
	std::unique_lock<std::mutex> lock(_accessMutex);
	_state[_buildIndex].captureTargets.push_back(KnownDynamicCast<Direct3DTexelArrayOutput*>(captureTarget));
	lock.unlock();
	FlagBuildStateModified();
}

//----------------------------------------------------------------------------------------
void Direct3DTexelArray::RemoveOutputCaptureTarget(ITexelArrayOutput* captureTarget)
{
	std::unique_lock<std::mutex> lock(_accessMutex);
	auto* captureTargetResolved = KnownDynamicCast<Direct3DTexelArrayOutput*>(captureTarget);
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
void Direct3DTexelArray::CaptureDataBufferOutput(ID3D12GraphicsCommandList* commandList)
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

	// Transfer each requested capture region into the data staging buffer
	for (auto* captureTarget : _state[_drawIndex].captureTargets)
	{
		// Retrieve the requested capture settings from our framebuffer output object
		size_t bufferStartPosInBytes = captureTarget->GetRequestedBufferOffset() * _structureStrideInBytes;
		size_t dataSizeInBytes = captureTarget->GetRequestedEntryCount() * _structureStrideInBytes;
		dataSizeInBytes = (dataSizeInBytes > 0 ? dataSizeInBytes : _totalBufferSizeInBytes);

		// Transition the buffer to the required resource state for reading
		auto acquireBarrier = CD3DX12_RESOURCE_BARRIER::Transition(_bufferWrapper->buffer, _bufferWrapper->lastResourceState, D3D12_RESOURCE_STATE_COPY_SOURCE);
		commandList->ResourceBarrier(1, &acquireBarrier);

		// Transfer the requested capture region into the staging buffer
		commandList->CopyBufferRegion(dataStagingBuffer, bufferStartPosInBytes, _bufferWrapper->buffer, bufferStartPosInBytes, dataSizeInBytes);

		// Transition the buffer to the last resource state
		auto releaseBarrier = CD3DX12_RESOURCE_BARRIER::Transition(_bufferWrapper->buffer, D3D12_RESOURCE_STATE_COPY_SOURCE, _bufferWrapper->lastResourceState);
		commandList->ResourceBarrier(1, &releaseBarrier);
	}
}

//----------------------------------------------------------------------------------------
void Direct3DTexelArray::CompleteCaptureDataBufferOutput()
{
	// Reset our flag indicating this buffer has been added as a current resource buffer
	_addedAsCurrent = false;

	// If no capture targets are defined, abort any further processing.
	if (_state[_drawIndex].captureTargets.empty())
	{
		return;
	}

	// Map the data staging buffer
	void* mappedBuffer;
	if (FAILED(_captureDataStagingBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedBuffer))))
	{
		_log->Error("Failed to map staging buffer when attempting to capture texel array output");
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
		captureTarget->StoreCaptureData(mappedMemory + bufferStartPosInBytes, entryCountToCapture, _structureStrideInBytes, _imageFormat, _dataFormat);

		// Record this captured framebuffer output with the renderer
		_renderer->AddCurrentTexelArrayOutput(captureTarget);

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
void Direct3DTexelArray::MigrateBuildStateToDrawState()
{
	std::swap(_buildIndex, _drawIndex);
	_stateModified.clear(std::memory_order_relaxed);

	// Ensure any updated capture targets are carried over to the new build state
	_state[_buildIndex].captureTargets = _state[_drawIndex].captureTargets;
}

//----------------------------------------------------------------------------------------
bool Direct3DTexelArray::CreateNativeBuffer()
{
	// Allocate the data array. Note that we need to create the buffer in the COMMON resource state as we're using the
	// default heap.
	D3D12_RESOURCE_STATES finalResourceState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_COMMON;
	auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto resourceDescription = CD3DX12_RESOURCE_DESC::Buffer(_totalBufferSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	HRESULT createTexelArrayReturn = _renderer->GetDevice()->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDescription, initialResourceState, nullptr, IID_PPV_ARGS(&_buffer));
	if (FAILED(createTexelArrayReturn))
	{
		_log->Error("Failed to create texel array with error code {0}", createTexelArrayReturn);
		return false;
	}
	_bufferWrapper = &_bufferWrapperLocal;
	_bufferWrapper->buffer = _buffer.Get();

	// Record the resource state for the buffer
	_bufferWrapper->lastResourceState = initialResourceState;
	_bufferWrapper->defaultResourceState = finalResourceState;

	// Since we lazily allocate resource views for this object on the heap, ensure we've pre-allocated the heaps we'll
	// be creating them on, so we don't get heap binding problems trying to create a new heap and use a resource from it
	// mid-frame.
	_renderer->EnsureHeapExists(Direct3DHeapManager::ResourceType::ShaderResourceView);
	_renderer->EnsureHeapExists(Direct3DHeapManager::ResourceType::UnorderedAccessView);
	return true;
}

//----------------------------------------------------------------------------------------
size_t Direct3DTexelArray::CompletePendingDataWrites(ID3D12GraphicsCommandList* commandList, D3DX12Residency::ResidencySet* residencySet)
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
			_renderer->ExtendTransferBufferLifetimeToNextFrame(write.uploadBuffer);
			buildPendingWrites.push_back(std::move(write));
		}
		lock.unlock();
		FlagBuildStateModified();
	}

	// If there are no pending writes, transition the buffer to the required resource state for drawing if required, and
	// abort any further processing.
	if (readyWrites.empty())
	{
		if (_bufferWrapper->lastResourceState != _bufferWrapper->defaultResourceState)
		{
			auto releaseBarrier = CD3DX12_RESOURCE_BARRIER::Transition(_bufferWrapper->buffer, _bufferWrapper->lastResourceState, _bufferWrapper->defaultResourceState);
			commandList->ResourceBarrier(1, &releaseBarrier);
			_bufferWrapper->lastResourceState = _bufferWrapper->defaultResourceState;
		}
		return 0;
	}

	// Perform each pending write operation
	return CompletePendingDataWritesInternal(readyWrites, commandList, residencySet, true);
}

//----------------------------------------------------------------------------------------
size_t Direct3DTexelArray::CompletePendingDataWritesInternal(std::vector<PendingWrite>& pendingWrites, ID3D12GraphicsCommandList* commandList, D3DX12Residency::ResidencySet* residencySet, bool performDrawStateTransition)
{
	// Transition the buffer to the required resource state for writing
	if (_bufferWrapper->lastResourceState != D3D12_RESOURCE_STATE_COPY_DEST)
	{
		auto acquireBarrier = CD3DX12_RESOURCE_BARRIER::Transition(_bufferWrapper->buffer, _bufferWrapper->lastResourceState, D3D12_RESOURCE_STATE_COPY_DEST);
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
		auto releaseBarrier = CD3DX12_RESOURCE_BARRIER::Transition(_bufferWrapper->buffer, D3D12_RESOURCE_STATE_COPY_DEST, _bufferWrapper->defaultResourceState);
		commandList->ResourceBarrier(1, &releaseBarrier);
		_bufferWrapper->lastResourceState = _bufferWrapper->defaultResourceState;
	}
	return 0;
}

//----------------------------------------------------------------------------------------
void Direct3DTexelArray::WriteDataToMappedBuffer(const PendingWrite& pendingWrite, const uint8_t* data, uint8_t* mappedMemory)
{
	// Copy our data into the memory buffer
	std::memcpy(mappedMemory, data, pendingWrite.uploadBufferSizeInBytes);
}

//----------------------------------------------------------------------------------------
void Direct3DTexelArray::CompletePendingDataWrite(ID3D12GraphicsCommandList* commandList, D3DX12Residency::ResidencySet* residencySet, const PendingWrite& pendingWrite)
{
	// Schedule a data transfer from the upload buffer to the target buffer
	commandList->CopyBufferRegion(_bufferWrapper->buffer, pendingWrite.targetBufferPosInBytes, pendingWrite.uploadBuffer, 0, pendingWrite.uploadBufferSizeInBytes);

	// If a transfer batch has been supplied, decrement the usage count.
	if (pendingWrite.transferBatch != nullptr)
	{
		pendingWrite.transferBatch->DecrementUsageCount();
	}
}

//----------------------------------------------------------------------------------------
size_t Direct3DTexelArray::CompletePendingDataTransfers(ID3D12GraphicsCommandList* commandList, D3DX12Residency::ResidencySet* residencySet)
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
	if (_bufferWrapper->lastResourceState != D3D12_RESOURCE_STATE_COPY_SOURCE)
	{
		auto acquireBarrier = CD3DX12_RESOURCE_BARRIER::Transition(_bufferWrapper->buffer, _bufferWrapper->lastResourceState, D3D12_RESOURCE_STATE_COPY_SOURCE);
		_bufferWrapper->lastResourceState = D3D12_RESOURCE_STATE_COPY_SOURCE;
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
void Direct3DTexelArray::CompletePendingDataTransfer(ID3D12GraphicsCommandList* commandList, D3DX12Residency::ResidencySet* residencySet, const PendingTransfer& pendingTransfer)
{
	// Transition the target buffer to the required resource state for writing
	if (pendingTransfer.targetBuffer->_bufferWrapper->lastResourceState != D3D12_RESOURCE_STATE_COPY_DEST)
	{
		auto acquireBarrier = CD3DX12_RESOURCE_BARRIER::Transition(pendingTransfer.targetBuffer->_bufferWrapper->buffer, pendingTransfer.targetBuffer->_bufferWrapper->lastResourceState, D3D12_RESOURCE_STATE_COPY_DEST);
		commandList->ResourceBarrier(1, &acquireBarrier);
		pendingTransfer.targetBuffer->_bufferWrapper->lastResourceState = D3D12_RESOURCE_STATE_COPY_DEST;
	}

	// Schedule a data transfer from this buffer to the target buffer
	commandList->CopyBufferRegion(pendingTransfer.targetBuffer->_bufferWrapper->buffer, pendingTransfer.targetBufferPosInBytes, _bufferWrapper->buffer, pendingTransfer.sourceBufferPosInBytes, pendingTransfer.transferCountInBytes);

	// If a transfer batch has been supplied, decrement the usage count.
	if (pendingTransfer.transferBatch != nullptr)
	{
		pendingTransfer.transferBatch->DecrementUsageCount();
	}
}

//----------------------------------------------------------------------------------------
const CD3DX12_GPU_DESCRIPTOR_HANDLE& Direct3DTexelArray::GetReadOnlyGPUDescriptorHandle(ID3D12GraphicsCommandList* commandList, D3D_SHADER_INPUT_TYPE type)
{
	// Create the view if required
	if (!_createdReadOnlyView)
	{
		_readOnlyViewHandle = _renderer->AllocateDescriptor(Direct3DHeapManager::ResourceType::ShaderResourceView);
		D3D12_SHADER_RESOURCE_VIEW_DESC viewDescription{};
		viewDescription.Format = _nativeFormat;
		viewDescription.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		viewDescription.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		viewDescription.Buffer.FirstElement = 0;
		viewDescription.Buffer.NumElements = (UINT)_structureEntryCount;
		viewDescription.Buffer.StructureByteStride = 0;
		viewDescription.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		_renderer->GetDevice()->CreateShaderResourceView(_bufferWrapper->buffer, &viewDescription, _readOnlyViewHandle->GetNativeCPUHandle());
		_createdReadOnlyView = true;
	}

	// Transition the buffer to the required resource state
	if (_bufferWrapper->lastResourceState != (D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE))
	{
		auto resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(_bufferWrapper->buffer, _bufferWrapper->lastResourceState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList->ResourceBarrier(1, &resourceBarrier);
		_bufferWrapper->lastResourceState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	}

	// Return the descriptor handle to the caller
	return _readOnlyViewHandle->GetNativeGPUHandle();
}

//----------------------------------------------------------------------------------------
const CD3DX12_GPU_DESCRIPTOR_HANDLE& Direct3DTexelArray::GetReadWriteGPUDescriptorHandle(ID3D12GraphicsCommandList* commandList, D3D_SHADER_INPUT_TYPE type)
{
	// Create the view if required
	if (!_createdReadWriteView)
	{
		_readWriteViewHandle = _renderer->AllocateDescriptor(Direct3DHeapManager::ResourceType::UnorderedAccessView);
		D3D12_UNORDERED_ACCESS_VIEW_DESC resourceBufferViewDescription = {};
		resourceBufferViewDescription.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		resourceBufferViewDescription.Format = _nativeFormat;
		resourceBufferViewDescription.Buffer.FirstElement = 0;
		resourceBufferViewDescription.Buffer.NumElements = (UINT)_structureEntryCount;
		resourceBufferViewDescription.Buffer.StructureByteStride = 0;
		resourceBufferViewDescription.Buffer.CounterOffsetInBytes = 0;
		resourceBufferViewDescription.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
		_renderer->GetDevice()->CreateUnorderedAccessView(_bufferWrapper->buffer, nullptr, &resourceBufferViewDescription, _readWriteViewHandle->GetNativeCPUHandle());
		_createdReadWriteView = true;
	}

	// Transition the buffer to the required resource state
	if (_bufferWrapper->lastResourceState != D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
	{
		auto resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(_bufferWrapper->buffer, _bufferWrapper->lastResourceState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		commandList->ResourceBarrier(1, &resourceBarrier);
		_bufferWrapper->lastResourceState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	}

	// Return the descriptor handle to the caller
	return _readWriteViewHandle->GetNativeGPUHandle();
}

//----------------------------------------------------------------------------------------
void Direct3DTexelArray::FlagBuildStateModified()
{
	if (!_stateModified.test_and_set(std::memory_order_acquire))
	{
		_renderer->FlagObjectModified(this);
	}
}

//----------------------------------------------------------------------------------------
void Direct3DTexelArray::AddAsCurrentBuffer()
{
	if (!_addedAsCurrent)
	{
		_renderer->AddCurrentTexelArray(this);
		_addedAsCurrent = true;
	}
}

} // namespace cobalt::graphics
