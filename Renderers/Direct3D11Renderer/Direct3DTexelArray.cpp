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

	// Determine if we should use deferred buffer creation
	bool deferBufferCreation = _renderer->UseDeferredBufferCreation();

	// Obtain any initial data for the buffer, performing image format conversion if required.
	const uint8_t* initialData = nullptr;
	if (_initialDataSet && (_initialDataSizeInBytes != _totalBufferSizeInBytes))
	{
		_log->Error("Initial texel array data of size {0} was provided, but {1} bytes are needed for the buffer.", _initialDataSizeInBytes, _totalBufferSizeInBytes);
		return false;
	}
	if (_initialDataSet)
	{
		bool convertInitialData = deferBufferCreation || !ImageFormatsAreBinaryEquivalent(_initialDataImageFormat, _imageFormat) || !DataFormatsAreBinaryEquivalent(_initialDataDataFormat, _dataFormat);
		if (convertInitialData && !ConvertDataFormat(_initialData, _initialDataSizeInBytes, _initialDataImageFormat, _initialDataDataFormat, _imageFormat, _dataFormat, _initialDataBuffer))
		{
			_log->Error("AllocateMemory failed to convert initial data for texel array");
			return false;
		}
		initialData = (convertInitialData ? _initialDataBuffer.data() : _initialData);
	}

	// Determine the usage flag to use when defining the Direct3D buffer
	PerformanceHint performanceHintCpu = _performanceHintCpu;
	DataPersistenceFlags dataPersistenceFlags = _dataPersistenceFlags;
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
		usageType = D3D11_USAGE_DEFAULT;
		cpuFlags = 0;
		break;
	case PerformanceHint::WriteOften:
		if ((dataPersistenceFlags & DataPersistenceFlags::InvalidateExistingDataOnWrite) == DataPersistenceFlags::InvalidateExistingDataOnWrite)
		{
			usageType = D3D11_USAGE_DYNAMIC;
			cpuFlags = D3D11_CPU_ACCESS_WRITE;
		}
		else
		{
			// D3D11 dynamic buffers only support WRITE_DISCARD and WRITE_NO_OVERWRITE map modes. If callers require the
			// existing contents to be preserved across partial writes, keep the buffer as DEFAULT and update it with
			// UpdateSubresource instead of trying to map it for plain WRITE access.
			usageType = D3D11_USAGE_DEFAULT;
			cpuFlags = 0;
		}
		break;
	}
	_cpuFlags = cpuFlags;
	_usageType = usageType;
	_usingDynamicBuffer = (usageType == D3D11_USAGE_DYNAMIC);
	_allowDiscardOnPartialWrite = ((dataPersistenceFlags & DataPersistenceFlags::InvalidateExistingDataOnWrite) == DataPersistenceFlags::InvalidateExistingDataOnWrite);

	// Create the buffer immediately if requested
	_nativeBufferCreationPending = deferBufferCreation;
	if (!deferBufferCreation)
	{
		if (!CreateNativeBuffer(initialData))
		{
			_log->Error("Failed to create native objects for texel array.");
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
bool Direct3DTexelArray::AllocateAsAliasForBuffer(ID3D11Buffer* buffer, size_t bufferSizeInBytes)
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

	// Flag that the buffer has been created
	_bufferCreated = true;
	_nativeBufferCreationPending = false;
	_usingDynamicBuffer = false;
	_bufferRawPointer = buffer;
	FlagBuildStateModified();
	return true;
}

//----------------------------------------------------------------------------------------
void Direct3DTexelArray::ReleaseMemory()
{
	// Delete our created buffer object
	if (_bufferCreated)
	{
		_buffer.Reset();
		_bufferRawPointer = nullptr;
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
void Direct3DTexelArray::CaptureDataBufferOutput(ID3D11Device1* device, ID3D11DeviceContext1* context)
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
		if (FAILED(device->CreateBuffer(&bufferDescription, nullptr, &_captureDataStagingBuffer)))
		{
			_log->Error("Failed to create staging buffer for texel array capture");
			return;
		}
		dataStagingBuffer = _captureDataStagingBuffer.Get();
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
		context->CopySubresourceRegion(_captureDataStagingBuffer.Get(), 0, (UINT)bufferStartPosInBytes, 0, 0, _bufferRawPointer, 0, &box);
	}

	// Map the data staging buffer
	const void* mappedBuffer = nullptr;
	D3D11_MAPPED_SUBRESOURCE mappedSubresource;
	if (FAILED(context->Map(_captureDataStagingBuffer.Get(), 0, D3D11_MAP_READ, 0, &mappedSubresource)))
	{
		_log->Error("Failed to map staging buffer when attempting to capture texel array output");
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
	context->Unmap(_captureDataStagingBuffer.Get(), 0);
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
bool Direct3DTexelArray::CreateNativeBuffer(const uint8_t* initialData)
{
	// Retrieve the initial data buffer if required
	if ((initialData == nullptr) && !_initialDataBuffer.empty())
	{
		initialData = _initialDataBuffer.data();
	}

	// Allocate the texel array
	D3D11_BUFFER_DESC bufferDescription = {};
	bufferDescription.Usage = _usageType;
	bufferDescription.ByteWidth = (UINT)_totalBufferSizeInBytes;
	//##FIX## Should we be combining with D3D11_BIND_SHADER_RESOURCE here, and for other resources where they are used
	//from shaders?
	bufferDescription.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	bufferDescription.CPUAccessFlags = _cpuFlags;
	bufferDescription.MiscFlags = 0;
	bufferDescription.StructureByteStride = 0;
	D3D11_SUBRESOURCE_DATA subresourceData = {};
	D3D11_SUBRESOURCE_DATA* subresourceDataPointer = nullptr;
	if (initialData != nullptr)
	{
		subresourceData.pSysMem = initialData;
		subresourceData.SysMemPitch = 0;
		subresourceData.SysMemSlicePitch = 0;
		subresourceDataPointer = &subresourceData;
	}
	if (FAILED(_renderer->GetDevice()->CreateBuffer(&bufferDescription, subresourceDataPointer, &_buffer)))
	{
		_log->Error("Failed to create texel array");
		return false;
	}
	_bufferRawPointer = _buffer.Get();

	// Release any resources related to the initial data. We don't use clear() and shrink_to_fit() here because this
	// data could be very large, and shrink_to_fit() isn't guaranteed to do anything. This approach is guaranteed to do
	// what we want, which is actually release the allocated memory for this buffer, since we won't need it again.
	std::vector<unsigned char>().swap(_initialDataBuffer);
	_initialDataBuffer = std::vector<unsigned char>();
	return true;
}

//----------------------------------------------------------------------------------------
void Direct3DTexelArray::CompletePendingDataWrites(ID3D11Device1* device, ID3D11DeviceContext1* context)
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

	// Map the buffer if required
	void* mappedBuffer = nullptr;
	if (_usingDynamicBuffer)
	{
		D3D11_MAPPED_SUBRESOURCE mappedSubresource;
		HRESULT mapReturn = context->Map(_bufferRawPointer, 0, (_allowDiscardOnPartialWrite ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE), 0, &mappedSubresource);
		if (FAILED(mapReturn))
		{
			_log->Error("Map operation failed when completing write operations for texel array with error code {0}", mapReturn);
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
		context->Unmap(_bufferRawPointer, 0);
	}
}

//----------------------------------------------------------------------------------------
void Direct3DTexelArray::CompletePendingDataWrite(const PendingWrite& pendingWrite, ID3D11Device1* device, ID3D11DeviceContext1* context, void* mappedBuffer)
{
	// Calculate the region of the buffer being modified
	const uint8_t* data = pendingWrite.data.data();
	auto dataSizeInBytes = pendingWrite.data.size();
	auto bufferStartPosInBytes = pendingWrite.targetBufferPosInBytes;

	// Perform the data write
	if (_usingDynamicBuffer)
	{
		auto* mappedMemory = reinterpret_cast<uint8_t*>(mappedBuffer);
		mappedMemory += bufferStartPosInBytes;
		std::memcpy(mappedMemory, data, dataSizeInBytes);
	}
	else
	{
		D3D11_BOX box;
		box.front = 0;
		box.back = 1;
		box.top = 0;
		box.bottom = 1;
		box.left = (UINT)bufferStartPosInBytes;
		box.right = box.left + (UINT)dataSizeInBytes;
		context->UpdateSubresource(_bufferRawPointer, 0, &box, reinterpret_cast<const void*>(data), 0, 0);
	}

	// If a transfer batch has been supplied, decrement the usage count.
	if (pendingWrite.transferBatch != nullptr)
	{
		pendingWrite.transferBatch->DecrementUsageCount();
	}
}

//----------------------------------------------------------------------------------------
void Direct3DTexelArray::CompletePendingDataTransfers(ID3D11Device1* device, ID3D11DeviceContext1* context)
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
void Direct3DTexelArray::CompletePendingDataTransfer(const PendingTransfer& pendingTransfer, ID3D11Device1* device, ID3D11DeviceContext1* context)
{
	// Perform the data transfer
	D3D11_BOX box;
	box.front = 0;
	box.back = 1;
	box.top = 0;
	box.bottom = 1;
	box.left = (UINT)pendingTransfer.sourceBufferPosInBytes;
	box.right = box.left + (UINT)pendingTransfer.transferCountInBytes;
	context->CopySubresourceRegion(pendingTransfer.targetBuffer->_bufferRawPointer, 0, (UINT)pendingTransfer.targetBufferPosInBytes, 0, 0, _bufferRawPointer, 0, &box);

	// If a transfer batch has been supplied, decrement the usage count.
	if (pendingTransfer.transferBatch != nullptr)
	{
		pendingTransfer.transferBatch->DecrementUsageCount();
	}
}

//----------------------------------------------------------------------------------------
ID3D11ShaderResourceView* Direct3DTexelArray::GetReadOnlyView(D3D_SHADER_INPUT_TYPE type)
{
	// Create the view if required
	if (!_createdReadOnlyView)
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC viewDescription = {};
		viewDescription.Format = _nativeFormat;
		viewDescription.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		viewDescription.Buffer.FirstElement = 0;
		viewDescription.Buffer.NumElements = (UINT)_structureEntryCount;
		if (FAILED(_renderer->GetDevice()->CreateShaderResourceView(_bufferRawPointer, &viewDescription, &_readOnlyView)))
		{
			_log->Error("Failed to create shader resource view for texel array");
		}
		_createdReadOnlyView = true;
	}

	// Return the buffer view to the caller
	return _readOnlyView.Get();
}

//----------------------------------------------------------------------------------------
ID3D11UnorderedAccessView* Direct3DTexelArray::GetReadWriteView(D3D_SHADER_INPUT_TYPE type)
{
	// Create the view if required
	if (!_createdReadWriteView)
	{
		// Create the target UAV
		D3D11_UNORDERED_ACCESS_VIEW_DESC viewDescription = {};
		viewDescription.Format = _nativeFormat;
		viewDescription.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		viewDescription.Buffer.FirstElement = 0;
		viewDescription.Buffer.NumElements = (UINT)_structureEntryCount;
		viewDescription.Buffer.Flags = 0;
		if (FAILED(_renderer->GetDevice()->CreateUnorderedAccessView(_bufferRawPointer, &viewDescription, &_readWriteView)))
		{
			_log->Error("Failed to create unordered access view for texel array");
		}
		_createdReadWriteView = true;
	}

	// Return the buffer view to the caller
	return _readWriteView.Get();
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
