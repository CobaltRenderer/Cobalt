// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "OpenGLTexelArray.h"
#include "OpenGLDebug.h"
#include "OpenGLIndexBuffer.h"
#include "OpenGLRenderer.h"
#include "OpenGLTexelArrayOutput.h"
#include "OpenGLTransferBatch.h"
#include "OpenGLVertexBuffer.h"
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <Internal/TextureFormatConversion/TextureFormatConversion.pkg>
#include <Internal/RendererSupport/KnownDynamicCast.h>
#include <algorithm>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
OpenGLTexelArray::OpenGLTexelArray(cobalt::logging::ILogger* log, OpenGLRenderer* renderer)
: _log(log), _renderer(renderer), _buildIndex(0), _drawIndex(1)
{}

//----------------------------------------------------------------------------------------
OpenGLTexelArray::~OpenGLTexelArray()
{
	ReleaseMemory();
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void OpenGLTexelArray::Delete()
{
	_renderer->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
SuccessToken OpenGLTexelArray::AllocateMemory()
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
	ImageFormat actualImageFormat;
	DataFormat actualDataFormat;
	if (!GetFormatNative(_imageFormat, _dataFormat, actualImageFormat, actualDataFormat, _nativeFormat))
	{
		_log->Error("Failed to allocate memory for texel array. The combination of image format {0} and data format {1} is not supported by this renderer.", _imageFormat, _dataFormat);
		return false;
	}
	_imageFormat = actualImageFormat;
	_dataFormat = actualDataFormat;

	// Determine the usage flag to use when defining the OpenGL buffer
	PerformanceHint performanceHintCpu = _performanceHintCpu;
	GLenum usageFlag;
	switch (performanceHintCpu & PerformanceHint::WriteFlagsMask)
	{
	case PerformanceHint::WriteNever:
	case PerformanceHint::WriteRarely:
		usageFlag = GL_STATIC_DRAW;
		break;
	default:
	case PerformanceHint::Default:
		usageFlag = GL_DYNAMIC_DRAW;
		break;
	case PerformanceHint::WriteOften:
		usageFlag = GL_STREAM_DRAW;
		break;
	}
	_usageFlag = usageFlag;

	// Obtain any initial data for the buffer, performing image format conversion if required.
	if (_initialDataSet && (_initialDataSizeInBytes != _totalBufferSizeInBytes))
	{
		_log->Error("Initial texel array data of size {0} was provided, but {1} bytes are needed for the buffer.", _initialDataSizeInBytes, _totalBufferSizeInBytes);
		return false;
	}
	if (_initialDataSet)
	{
		if (!ConvertDataFormat(_initialData, _initialDataSizeInBytes, _initialDataImageFormat, _initialDataDataFormat, _imageFormat, _dataFormat, _initialDataBuffer))
		{
			_log->Error("AllocateMemory failed to convert initial data for texel array");
			return false;
		}
		_initialDataSet = false;
		_initialData = nullptr;
	}

	// Flag that the buffer has been created
	_bufferCreated = true;
	_nativeBufferCreationPending = true;
	FlagBuildStateModified();
	return true;
}

//----------------------------------------------------------------------------------------
void OpenGLTexelArray::ReleaseMemory()
{
	// Delete our created buffer object
	if (_bufferCreated && !_nativeBufferCreationPending)
	{
		glDeleteBuffers(1, &_openGLBufferNo);
		CheckGLError(_log);
	}
	_bufferCreated = false;
}

//----------------------------------------------------------------------------------------
void OpenGLTexelArray::SetBufferLayout(ImageFormat imageFormat, DataFormat dataFormat, size_t entryCount)
{
	_imageFormat = imageFormat;
	_dataFormat = dataFormat;
	_structureEntryCount = entryCount;
	_structureStrideInBytes = ByteSizePerElementFromFormat(dataFormat) * ElementCountPerPixelFromFormat(imageFormat);
	_bufferLayoutSet = true;
}

//----------------------------------------------------------------------------------------
bool OpenGLTexelArray::AllocateAsAliasForBuffer(OpenGLIndexBuffer* buffer, size_t bufferSizeInBytes)
{
	if (!AllocateAsAliasForBufferInternal(bufferSizeInBytes))
	{
		return false;
	}
	_bufferAliasSourceIndex = buffer;
	return true;
}

//----------------------------------------------------------------------------------------
bool OpenGLTexelArray::AllocateAsAliasForBuffer(OpenGLVertexBuffer* buffer, size_t bufferSizeInBytes)
{
	if (!AllocateAsAliasForBufferInternal(bufferSizeInBytes))
	{
		return false;
	}
	_bufferAliasSourceVertex = buffer;
	return true;
}

//----------------------------------------------------------------------------------------
bool OpenGLTexelArray::AllocateAsAliasForBufferInternal(size_t bufferSizeInBytes)
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

	// Calculate the internal data format to use
	ImageFormat actualImageFormat;
	DataFormat actualDataFormat;
	if (!GetFormatNative(_imageFormat, _dataFormat, actualImageFormat, actualDataFormat, _nativeFormat))
	{
		_log->Error("Failed to allocate texel array as alias. The combination of image format {0} and data format {1} is not supported by this renderer.", _imageFormat, _dataFormat);
		return false;
	}
	_imageFormat = actualImageFormat;
	_dataFormat = actualDataFormat;

	// Flag that the buffer has been created
	_bufferCreated = true;
	_nativeBufferCreationPending = true;
	_bufferIsAlias = true;
	FlagBuildStateModified();
	return true;
}

//----------------------------------------------------------------------------------------
// Usage methods
//----------------------------------------------------------------------------------------
void OpenGLTexelArray::SetUsageFlags(UsageFlags usageFlags)
{
	_usageFlags = usageFlags;
}

//----------------------------------------------------------------------------------------
void OpenGLTexelArray::SetPerformanceHints(PerformanceHint performanceHintCpu, PerformanceHint performanceHintGpu)
{
	_performanceHintCpu = performanceHintCpu;
	_performanceHintGpu = performanceHintGpu;
}

//----------------------------------------------------------------------------------------
void OpenGLTexelArray::SetDataPersistenceFlags(DataPersistenceFlags dataPersistenceFlags)
{
	_dataPersistenceFlags = dataPersistenceFlags;
}

//----------------------------------------------------------------------------------------
// Initial data methods
//----------------------------------------------------------------------------------------
SuccessToken OpenGLTexelArray::SetInitialData(const void* sourceBuffer, size_t sourceBufferSizeInBytes, SourceImageFormat imageFormat, SourceDataFormat dataFormat)
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
SuccessToken OpenGLTexelArray::QueueDataUpdate(const void* sourceBuffer, size_t sourceBufferSizeInBytes, SourceImageFormat imageFormat, SourceDataFormat dataFormat, size_t targetBufferOffset, ITransferBatch* transferBatch)
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
	auto* transferBatchResolved = KnownDynamicCast<OpenGLTransferBatch*>(transferBatch);
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
SuccessToken OpenGLTexelArray::QueueDataTransfer(ITexelArray* targetBuffer, size_t transferCount, size_t sourceBufferOffset, size_t targetBufferOffset, ITransferBatch* transferBatch)
{
	// Ensure the source and target buffers allow a transfer
	auto* targetBufferResolved = KnownDynamicCast<OpenGLTexelArray*>(targetBuffer);
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
	auto* transferBatchResolved = KnownDynamicCast<OpenGLTransferBatch*>(transferBatch);
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
bool OpenGLTexelArray::ConvertDataFormat(const void* sourceBuffer, size_t sourceBufferSizeInBytes, SourceImageFormat sourceImageFormat, SourceDataFormat sourceDataFormat, ImageFormat targetImageFormat, DataFormat targetDataFormat, std::vector<uint8_t>& targetBuffer) const
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
constexpr bool OpenGLTexelArray::GetFormatNative(ImageFormat requestedImageFormat, DataFormat requestedDataFormat, ImageFormat& actualImageFormat, DataFormat& actualDataFormat, GLenum& nativeFormat)
{
	actualImageFormat = requestedImageFormat;
	actualDataFormat = requestedDataFormat;
	switch (requestedDataFormat)
	{
	case DataFormat::Int8:
		switch (requestedImageFormat)
		{
		case ImageFormat::R:
		case ImageFormat::X:
			nativeFormat = GL_R8I;
			return true;
		case ImageFormat::RG:
		case ImageFormat::XY:
			nativeFormat = GL_RG8I;
			return true;
		case ImageFormat::RGBA:
		case ImageFormat::XYZW:
			nativeFormat = GL_RGBA8I;
			return true;
		}
		break;
	case DataFormat::Int16:
		switch (requestedImageFormat)
		{
		case ImageFormat::R:
		case ImageFormat::X:
			nativeFormat = GL_R16I;
			return true;
		case ImageFormat::RG:
		case ImageFormat::XY:
			nativeFormat = GL_RG16I;
			return true;
		case ImageFormat::RGBA:
		case ImageFormat::XYZW:
			nativeFormat = GL_RGBA16I;
			return true;
		}
		break;
	case DataFormat::Int32:
		switch (requestedImageFormat)
		{
		case ImageFormat::R:
		case ImageFormat::X:
			nativeFormat = GL_R32I;
			return true;
		case ImageFormat::RG:
		case ImageFormat::XY:
			nativeFormat = GL_RG32I;
			return true;
		case ImageFormat::RGBA:
		case ImageFormat::XYZW:
			nativeFormat = GL_RGBA32I;
			return true;
		}
		break;
	case DataFormat::UInt8:
		switch (requestedImageFormat)
		{
		case ImageFormat::R:
		case ImageFormat::X:
			nativeFormat = GL_R8UI;
			return true;
		case ImageFormat::RG:
		case ImageFormat::XY:
			nativeFormat = GL_RG8UI;
			return true;
		case ImageFormat::RGBA:
		case ImageFormat::XYZW:
			nativeFormat = GL_RGBA8UI;
			return true;
		}
		break;
	case DataFormat::UInt16:
		switch (requestedImageFormat)
		{
		case ImageFormat::R:
		case ImageFormat::X:
			nativeFormat = GL_R16UI;
			return true;
		case ImageFormat::RG:
		case ImageFormat::XY:
			nativeFormat = GL_RG16UI;
			return true;
		case ImageFormat::RGBA:
		case ImageFormat::XYZW:
			nativeFormat = GL_RGBA16UI;
			return true;
		}
		break;
	case DataFormat::UInt32:
		switch (requestedImageFormat)
		{
		case ImageFormat::R:
		case ImageFormat::X:
			nativeFormat = GL_R32UI;
			return true;
		case ImageFormat::RG:
		case ImageFormat::XY:
			nativeFormat = GL_RG32UI;
			return true;
		case ImageFormat::RGBA:
		case ImageFormat::XYZW:
			nativeFormat = GL_RGBA32UI;
			return true;
		}
		break;
	case DataFormat::Norm8:
		actualDataFormat = DataFormat::UNorm8;
		switch (requestedImageFormat)
		{
		case ImageFormat::R:
		case ImageFormat::X:
			nativeFormat = GL_R8;
			return true;
		case ImageFormat::RG:
		case ImageFormat::XY:
			nativeFormat = GL_RG8;
			return true;
		case ImageFormat::RGBA:
		case ImageFormat::XYZW:
			nativeFormat = GL_RGBA8;
			return true;
		}
		break;
	case DataFormat::UNorm8:
		switch (requestedImageFormat)
		{
		case ImageFormat::R:
		case ImageFormat::X:
			nativeFormat = GL_R8;
			return true;
		case ImageFormat::RG:
		case ImageFormat::XY:
			nativeFormat = GL_RG8;
			return true;
		case ImageFormat::RGBA:
		case ImageFormat::XYZW:
			nativeFormat = GL_RGBA8;
			return true;
		}
		break;
	case DataFormat::Float16:
		switch (requestedImageFormat)
		{
		case ImageFormat::R:
		case ImageFormat::X:
			nativeFormat = GL_R16F;
			return true;
		case ImageFormat::RG:
		case ImageFormat::XY:
			nativeFormat = GL_RG16F;
			return true;
		case ImageFormat::RGBA:
		case ImageFormat::XYZW:
			nativeFormat = GL_RGBA16F;
			return true;
		}
		break;
	case DataFormat::Float32:
		switch (requestedImageFormat)
		{
		case ImageFormat::R:
		case ImageFormat::X:
			nativeFormat = GL_R32F;
			return true;
		case ImageFormat::RG:
		case ImageFormat::XY:
			nativeFormat = GL_RG32F;
			return true;
		case ImageFormat::RGBA:
		case ImageFormat::XYZW:
			nativeFormat = GL_RGBA32F;
			return true;
		}
		break;
	}
	return false;
}

//----------------------------------------------------------------------------------------
// Output capture methods
//----------------------------------------------------------------------------------------
void OpenGLTexelArray::AddOutputCaptureTarget(ITexelArrayOutput* captureTarget)
{
	std::unique_lock<std::mutex> lock(_accessMutex);
	_state[_buildIndex].captureTargets.push_back(KnownDynamicCast<OpenGLTexelArrayOutput*>(captureTarget));
	lock.unlock();
	FlagBuildStateModified();
}

//----------------------------------------------------------------------------------------
void OpenGLTexelArray::RemoveOutputCaptureTarget(ITexelArrayOutput* captureTarget)
{
	std::unique_lock<std::mutex> lock(_accessMutex);
	auto* captureTargetResolved = KnownDynamicCast<OpenGLTexelArrayOutput*>(captureTarget);
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
void OpenGLTexelArray::CaptureDataBufferOutput()
{
	// Reset our flag indicating this buffer has been added as a current resource buffer
	_addedAsCurrent = false;

	// If no capture targets are defined, abort any further processing.
	if (_state[_drawIndex].captureTargets.empty())
	{
		return;
	}

	// Store the buffer data and counter values into each attached capture target
	std::vector<unsigned char> bufferData;
	for (auto* captureTarget : _state[_drawIndex].captureTargets)
	{
		// Determine the position and size of the buffer region to capture
		size_t entryOffset = captureTarget->GetRequestedBufferOffset();
		size_t entryCountToCapture = captureTarget->GetRequestedEntryCount();
		entryCountToCapture = (entryCountToCapture == 0 ? _structureEntryCount : entryCountToCapture);
		entryCountToCapture = (((entryCountToCapture + entryOffset) > _structureEntryCount) ? (_structureEntryCount - entryOffset) : entryCountToCapture);
		size_t bufferStartPosInBytes = entryOffset * _structureStrideInBytes;

		// Retrieve the requested data from the buffer
		bufferData.resize(entryCountToCapture * _structureStrideInBytes);
		CheckGLError(_log);
		glBindBuffer(GL_TEXTURE_BUFFER, _openGLBufferNo);
		glGetBufferSubData(GL_TEXTURE_BUFFER, bufferStartPosInBytes, bufferData.size(), bufferData.data());
		glBindBuffer(GL_TEXTURE_BUFFER, 0);
		CheckGLError(_log);

		// Store the requested capture data in the capture target
		bool detachAfterCapture = captureTarget->IsDetachingAfterCapture();
		captureTarget->StoreCaptureData(bufferData.data(), entryCountToCapture, _structureStrideInBytes, _imageFormat, _dataFormat);

		// Record this captured framebuffer output with the renderer
		_renderer->AddCurrentTexelArrayOutput(captureTarget);

		// Now that we've captured a frame, detach the output capture target if requested.
		if (detachAfterCapture)
		{
			RemoveOutputCaptureTarget(captureTarget);
		}
	}
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
void OpenGLTexelArray::MigrateBuildStateToDrawState()
{
	std::swap(_buildIndex, _drawIndex);
	_stateModified.clear(std::memory_order_relaxed);

	// Ensure any updated capture targets are carried over to the new build state
	_state[_buildIndex].captureTargets = _state[_drawIndex].captureTargets;
}

//----------------------------------------------------------------------------------------
void OpenGLTexelArray::CreateNativeBuffer()
{
	// Retrieve the initial data buffer if required
	const uint8_t* initialData = nullptr;
	if (!_initialDataBuffer.empty())
	{
		initialData = _initialDataBuffer.data();
	}

	// Allocate the data array
	CheckGLError(_log);
	glGenBuffers(1, &_openGLBufferNo);
	glBindBuffer(GL_TEXTURE_BUFFER, _openGLBufferNo);
	glBufferData(GL_TEXTURE_BUFFER, _totalBufferSizeInBytes, initialData, _usageFlag);
	glBindBuffer(GL_TEXTURE_BUFFER, 0);
	CheckGLError(_log);

	// Generate the texture alias
	CheckGLError(_log);
	glGenTextures(1, &_textureNo);
	glBindTexture(GL_TEXTURE_BUFFER, _textureNo);
	glTexBuffer(GL_TEXTURE_BUFFER, _nativeFormat, _openGLBufferNo);
	glBindTexture(GL_TEXTURE_BUFFER, 0);
	CheckGLError(_log);

	// Release any resources related to the initial data. We don't use clear() and shrink_to_fit() here because this
	// data could be very large, and shrink_to_fit() isn't guaranteed to do anything. This approach is guaranteed to do
	// what we want, which is actually release the allocated memory for this buffer, since we won't need it again.
	std::vector<unsigned char>().swap(_initialDataBuffer);
	_initialDataBuffer = std::vector<unsigned char>();
}

//----------------------------------------------------------------------------------------
void OpenGLTexelArray::CompletePendingDataWrites()
{
	// Create the native buffer if its creation is pending
	if (_nativeBufferCreationPending)
	{
		if (_bufferIsAlias)
		{
			// If this texel array is an alias, force the source buffer to complete pending data writes first. This will
			// force the source buffer to generate its buffer storage object, allowing us to capture the OpenGL buffer
			// number here.
			if (_bufferAliasSourceIndex != nullptr)
			{
				_bufferAliasSourceIndex->CompletePendingDataWrites();
				_openGLBufferNo = _bufferAliasSourceIndex->GetOpenGLBufferNo();
			}
			else if (_bufferAliasSourceVertex != nullptr)
			{
				_bufferAliasSourceVertex->CompletePendingDataWrites();
				_openGLBufferNo = _bufferAliasSourceVertex->GetOpenGLBufferNo();
			}

			// Generate the texture alias
			CheckGLError(_log);
			glGenTextures(1, &_textureNo);
			glBindTexture(GL_TEXTURE_BUFFER, _textureNo);
			glTexBuffer(GL_TEXTURE_BUFFER, _nativeFormat, _openGLBufferNo);
			glBindTexture(GL_TEXTURE_BUFFER, 0);
			CheckGLError(_log);
		}
		else
		{
			CreateNativeBuffer();
		}
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
	glBindBuffer(GL_TEXTURE_BUFFER, _openGLBufferNo);
	CheckGLError(_log);
	for (const PendingWrite& write : readyWrites)
	{
		CompletePendingDataWrite(write);
	}
	glBindBuffer(GL_TEXTURE_BUFFER, 0);
	CheckGLError(_log);
}

//----------------------------------------------------------------------------------------
void OpenGLTexelArray::CompletePendingDataWrite(const PendingWrite& pendingWrite)
{
	// Calculate the region of the buffer being modified
	const uint8_t* data = pendingWrite.data.data();
	auto dataSizeInBytes = pendingWrite.data.size();
	auto bufferStartPosInBytes = pendingWrite.targetBufferPosInBytes;

	// Update the target region of the buffer
	glBufferSubData(GL_TEXTURE_BUFFER, bufferStartPosInBytes, dataSizeInBytes, reinterpret_cast<const GLvoid*>(data));

	// If a transfer batch has been supplied, decrement the usage count.
	if (pendingWrite.transferBatch != nullptr)
	{
		pendingWrite.transferBatch->DecrementUsageCount();
	}
}

//----------------------------------------------------------------------------------------
void OpenGLTexelArray::CompletePendingDataTransfers()
{
	// Create the native buffer if its creation is pending
	if (_nativeBufferCreationPending)
	{
		if (_bufferIsAlias)
		{
			// If this texel array is an alias, force the source buffer to complete pending data writes first. This will
			// force the source buffer to generate its buffer storage object, allowing us to capture the OpenGL buffer
			// number here.
			if (_bufferAliasSourceIndex != nullptr)
			{
				_bufferAliasSourceIndex->CompletePendingDataWrites();
				_openGLBufferNo = _bufferAliasSourceIndex->GetOpenGLBufferNo();
			}
			else if (_bufferAliasSourceVertex != nullptr)
			{
				_bufferAliasSourceVertex->CompletePendingDataWrites();
				_openGLBufferNo = _bufferAliasSourceVertex->GetOpenGLBufferNo();
			}

			// Generate the texture alias
			CheckGLError(_log);
			glGenTextures(1, &_textureNo);
			glBindTexture(GL_TEXTURE_BUFFER, _textureNo);
			glTexBuffer(GL_TEXTURE_BUFFER, _nativeFormat, _openGLBufferNo);
			glBindTexture(GL_TEXTURE_BUFFER, 0);
			CheckGLError(_log);
		}
		else
		{
			CreateNativeBuffer();
		}
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
	glBindBuffer(GL_TEXTURE_BUFFER, _openGLBufferNo);
	CheckGLError(_log);
	for (const PendingTransfer& transfer : readyTransfers)
	{
		CompletePendingDataTransfer(transfer);
	}
	glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
	glBindBuffer(GL_TEXTURE_BUFFER, 0);
	CheckGLError(_log);
}

//----------------------------------------------------------------------------------------
void OpenGLTexelArray::CompletePendingDataTransfer(const PendingTransfer& pendingTransfer)
{
	// Bind the target buffer to the write target
	glBindBuffer(GL_COPY_WRITE_BUFFER, pendingTransfer.targetBuffer->_openGLBufferNo);

	// Update the target region of the buffer
	glCopyBufferSubData(GL_TEXTURE_BUFFER, GL_COPY_WRITE_BUFFER, (GLintptr)pendingTransfer.sourceBufferPosInBytes, (GLintptr)pendingTransfer.targetBufferPosInBytes, (GLsizeiptr)pendingTransfer.transferCountInBytes);

	// If a transfer batch has been supplied, decrement the usage count.
	if (pendingTransfer.transferBatch != nullptr)
	{
		pendingTransfer.transferBatch->DecrementUsageCount();
	}
}

//----------------------------------------------------------------------------------------
void OpenGLTexelArray::FlagBuildStateModified()
{
	if (!_stateModified.test_and_set(std::memory_order_acquire))
	{
		_renderer->FlagObjectModified(this);
	}
}

//----------------------------------------------------------------------------------------
GLuint OpenGLTexelArray::GetTextureNo() const
{
	return _textureNo;
}

//----------------------------------------------------------------------------------------
GLenum OpenGLTexelArray::GetTextureFormat() const
{
	return _nativeFormat;
}

//----------------------------------------------------------------------------------------
void OpenGLTexelArray::AddAsCurrentBuffer()
{
	if (!_addedAsCurrent)
	{
		_renderer->AddCurrentTexelArray(this);
		_addedAsCurrent = true;
	}
}

} // namespace cobalt::graphics
