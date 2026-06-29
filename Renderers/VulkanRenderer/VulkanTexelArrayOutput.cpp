// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "VulkanTexelArrayOutput.h"
#include "VulkanRenderer.h"
#include <Internal/TextureFormatConversion/TextureFormatConversion.pkg>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
VulkanTexelArrayOutput::VulkanTexelArrayOutput(cobalt::logging::ILogger* log, VulkanRenderer* renderer)
: _log(log), _renderer(renderer), _buildIndex(0), _drawIndex(1), _readFromCurrentDrawBufferEnabled(false)
{
	_state[_drawIndex].hasCapturedOutput = false;
	_state[_buildIndex].hasCapturedOutput = false;
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void VulkanTexelArrayOutput::Delete()
{
	_renderer->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
// Configuration methods
//----------------------------------------------------------------------------------------
void VulkanTexelArrayOutput::SetDetachAfterCapture(bool state)
{
	_state[_buildIndex].detachAfterCapture = state;
	_renderer->FlagObjectModified(this);
}

//----------------------------------------------------------------------------------------
void VulkanTexelArrayOutput::SetArrayCaptureRegion(size_t captureEntryCount, size_t bufferOffset)
{
	_state[_buildIndex].requestedCaptureEntryCount = captureEntryCount;
	_state[_buildIndex].requestedBufferOffset = bufferOffset;
	_renderer->FlagObjectModified(this);
}

//----------------------------------------------------------------------------------------
// Data methods
//----------------------------------------------------------------------------------------
bool VulkanTexelArrayOutput::HasCapturedOutput() const
{
	return _state[ReadIndex()].hasCapturedOutput;
}

//----------------------------------------------------------------------------------------
void VulkanTexelArrayOutput::ClearCapturedOutput()
{
	_state[ReadIndex()].dataBuffer.clear();
	_state[ReadIndex()].hasCapturedOutput = false;
}

//----------------------------------------------------------------------------------------
size_t VulkanTexelArrayOutput::GetEntryCount() const
{
	if (!_state[ReadIndex()].hasCapturedOutput)
	{
		return 0;
	}
	return _state[ReadIndex()].actualCaptureEntryCount;
}

//----------------------------------------------------------------------------------------
ITexelArray::SourceImageFormat VulkanTexelArrayOutput::GetOptimalImageFormat() const
{
	switch (_state[ReadIndex()].imageFormat)
	{
	case ITexelArray::ImageFormat::R:
		return ITexelArray::SourceImageFormat::R;
	case ITexelArray::ImageFormat::RG:
		return ITexelArray::SourceImageFormat::RG;
	case ITexelArray::ImageFormat::RGBA:
		return ITexelArray::SourceImageFormat::RGBA;
	case ITexelArray::ImageFormat::X:
		return ITexelArray::SourceImageFormat::X;
	case ITexelArray::ImageFormat::XY:
		return ITexelArray::SourceImageFormat::XY;
	case ITexelArray::ImageFormat::XYZW:
		return ITexelArray::SourceImageFormat::XYZW;
	}
	return ITexelArray::SourceImageFormat::RGBA;
}

//----------------------------------------------------------------------------------------
ITexelArray::SourceDataFormat VulkanTexelArrayOutput::GetOptimalDataFormat() const
{
	switch (_state[ReadIndex()].dataFormat)
	{
	case ITexelArray::DataFormat::Int8:
		return ITexelArray::SourceDataFormat::Int8;
	case ITexelArray::DataFormat::Int16:
		return ITexelArray::SourceDataFormat::Int16;
	case ITexelArray::DataFormat::Int32:
		return ITexelArray::SourceDataFormat::Int32;
	case ITexelArray::DataFormat::UInt8:
		return ITexelArray::SourceDataFormat::UInt8;
	case ITexelArray::DataFormat::UInt16:
		return ITexelArray::SourceDataFormat::UInt16;
	case ITexelArray::DataFormat::UInt32:
		return ITexelArray::SourceDataFormat::UInt32;
	case ITexelArray::DataFormat::Norm8:
		return ITexelArray::SourceDataFormat::Norm8;
	case ITexelArray::DataFormat::UNorm8:
		return ITexelArray::SourceDataFormat::UNorm8;
	case ITexelArray::DataFormat::Float16:
		return ITexelArray::SourceDataFormat::Float16;
	case ITexelArray::DataFormat::Float32:
		return ITexelArray::SourceDataFormat::Float32;
	}
	return ITexelArray::SourceDataFormat::Float32;
}

//----------------------------------------------------------------------------------------
SuccessToken VulkanTexelArrayOutput::ReadBufferData(void* targetBuffer, size_t targetBufferSizeInBytes, ITexelArray::SourceImageFormat imageFormat, ITexelArray::SourceDataFormat dataFormat) const
{
	// Ensure we have a captured output to work with
	if (!HasCapturedOutput())
	{
		_log->Error("Failed to read texel array output. A read was attempted when no captured data exists in the buffer.");
		return false;
	}

	// Convert the data to the required format
	auto readIndex = ReadIndex();
	size_t capturedDataSizeInBytes = _state[readIndex].dataBuffer.size();
	bool sourceDataFormatError = false;
	bool targetDataFormatError = false;
	bool result = TextureFormatConversion::ConvertTexelArrayOutputData(_state[readIndex].dataBuffer.data(), capturedDataSizeInBytes, _state[readIndex].imageFormat, _state[readIndex].dataFormat, imageFormat, dataFormat, targetBuffer, sourceDataFormatError, targetDataFormatError);
	if (targetDataFormatError)
	{
		_log->Critical("Data conversion wasn't handled for texel array with image format {0} and data format {1}", imageFormat, dataFormat);
	}
	if (sourceDataFormatError)
	{
		_log->Error("Attempted to read from texel array with incompatible or unsupported image format {0} and data format {1}", imageFormat, dataFormat);
	}
	return result;
}

//----------------------------------------------------------------------------------------
// Capture methods
//----------------------------------------------------------------------------------------
bool VulkanTexelArrayOutput::IsDetachingAfterCapture() const
{
	return _state[_drawIndex].detachAfterCapture;
}

//----------------------------------------------------------------------------------------
size_t VulkanTexelArrayOutput::GetRequestedEntryCount() const
{
	return _state[_drawIndex].requestedCaptureEntryCount;
}

//----------------------------------------------------------------------------------------
size_t VulkanTexelArrayOutput::GetRequestedBufferOffset() const
{
	return _state[_drawIndex].requestedBufferOffset;
}

//----------------------------------------------------------------------------------------
void VulkanTexelArrayOutput::StoreCaptureData(const unsigned char* bufferData, size_t bufferEntryCount, size_t bufferEntrySizeInBytes, ITexelArray::ImageFormat imageFormat, ITexelArray::DataFormat dataFormat)
{
	// Store the provided capture data
	_state[_drawIndex].dataBuffer.assign(bufferData, bufferData + (bufferEntryCount * bufferEntrySizeInBytes));
	_state[_drawIndex].actualCaptureEntryCount = bufferEntryCount;
	_state[_drawIndex].actualCaptureEntrySizeInBytes = bufferEntrySizeInBytes;
	_state[_drawIndex].imageFormat = imageFormat;
	_state[_drawIndex].dataFormat = dataFormat;
	_state[_drawIndex].hasCapturedOutput = true;
	_renderer->FlagObjectModified(this);
}

//----------------------------------------------------------------------------------------
void VulkanTexelArrayOutput::EnableCaptureReadFromCurrentDrawBuffer()
{
	_readFromCurrentDrawBufferEnabled = true;
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
void VulkanTexelArrayOutput::MigrateBuildStateToDrawState()
{
	std::swap(_buildIndex, _drawIndex);
	_state[_drawIndex].hasCapturedOutput = false;
	_state[_buildIndex].detachAfterCapture = _state[_drawIndex].detachAfterCapture;
	_state[_buildIndex].requestedBufferOffset = _state[_drawIndex].requestedBufferOffset;
	_state[_buildIndex].requestedCaptureEntryCount = _state[_drawIndex].requestedCaptureEntryCount;
	_readFromCurrentDrawBufferEnabled = false;
}

//----------------------------------------------------------------------------------------
uint32_t VulkanTexelArrayOutput::ReadIndex() const
{
	return (_readFromCurrentDrawBufferEnabled ? _drawIndex : _buildIndex);
}

} // namespace cobalt::graphics
