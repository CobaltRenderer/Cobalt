// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "Direct3DFrameBufferOutput.h"
#include "Direct3DRenderer.h"
#include <Internal/TextureFormatConversion/TextureFormatConversion.pkg>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
Direct3DFrameBufferOutput::Direct3DFrameBufferOutput(cobalt::logging::ILogger* log, Direct3DRenderer* renderer)
: _log(log), _renderer(renderer), _buildIndex(0), _drawIndex(1), _readFromCurrentDrawBufferEnabled(false)
{
	_state[_drawIndex].hasCapturedOutput = false;
	_state[_buildIndex].hasCapturedOutput = false;
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void Direct3DFrameBufferOutput::Delete()
{
	_renderer->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
// Configuration methods
//----------------------------------------------------------------------------------------
void Direct3DFrameBufferOutput::SetDetachAfterCapture(bool state)
{
	_state[_buildIndex].detachAfterCapture = state;
	_renderer->FlagObjectModified(this);
}

//----------------------------------------------------------------------------------------
void Direct3DFrameBufferOutput::SetFrameBufferCaptureRegion(const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels)
{
	_state[_buildIndex].requestedImageOffset = imageOffsetInPixels;
	_state[_buildIndex].requestedImageSize = imageRegionInPixels;
	_renderer->FlagObjectModified(this);
}

//----------------------------------------------------------------------------------------
// Data methods
//----------------------------------------------------------------------------------------
bool Direct3DFrameBufferOutput::HasCapturedOutput() const
{
	return _state[ReadIndex()].hasCapturedOutput;
}

//----------------------------------------------------------------------------------------
void Direct3DFrameBufferOutput::ClearCapturedOutput()
{
	_state[ReadIndex()].dataBuffer.clear();
	_state[ReadIndex()].hasCapturedOutput = false;
}

//----------------------------------------------------------------------------------------
V2UInt32 Direct3DFrameBufferOutput::GetImageDimensions() const
{
	return _state[ReadIndex()].actualImageSize;
}

//----------------------------------------------------------------------------------------
V2UInt32 Direct3DFrameBufferOutput::GetCroppedImageDimensions(const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels) const
{
	return CalculateCroppedImageDimensions(GetImageDimensions(), imageOffsetInPixels, imageRegionInPixels);
}

//----------------------------------------------------------------------------------------
V2UInt32 Direct3DFrameBufferOutput::CalculateCroppedImageDimensions(const V2UInt32& imageSizeInPixels, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels)
{
	if ((imageOffsetInPixels.X() >= imageSizeInPixels.X()) || (imageOffsetInPixels.Y() >= imageSizeInPixels.Y()))
	{
		return V2UInt32(0, 0);
	}
	V2UInt32 visibleImageRegion = V2UInt32((imageSizeInPixels.X() - imageOffsetInPixels.X()), (imageSizeInPixels.Y() - imageOffsetInPixels.Y()));
	V2UInt32 intersectingRegion = V2UInt32(((imageRegionInPixels.X() > 0) ? std::min(visibleImageRegion.X(), imageRegionInPixels.X()) : visibleImageRegion.X()), ((imageRegionInPixels.Y() > 0) ? std::min(visibleImageRegion.Y(), imageRegionInPixels.Y()) : visibleImageRegion.Y()));
	return intersectingRegion;
}

//----------------------------------------------------------------------------------------
size_t Direct3DFrameBufferOutput::CalculatePixelCountForRegion(const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels) const
{
	V2UInt32 croppedImageDimensions = GetCroppedImageDimensions(imageOffsetInPixels, imageRegionInPixels);
	size_t pixelCount = (size_t)croppedImageDimensions.X() * (size_t)croppedImageDimensions.Y();
	return pixelCount;
}

//----------------------------------------------------------------------------------------
ITextureBuffer::SourceImageFormat Direct3DFrameBufferOutput::GetOptimalImageFormat() const
{
	switch (_state[ReadIndex()].imageFormat)
	{
	case ITextureBuffer::ImageFormat::R:
		return ITextureBuffer::SourceImageFormat::R;
	case ITextureBuffer::ImageFormat::RG:
		return ITextureBuffer::SourceImageFormat::RG;
	case ITextureBuffer::ImageFormat::RGB:
		return ITextureBuffer::SourceImageFormat::RGB;
	case ITextureBuffer::ImageFormat::RGBA:
		return ITextureBuffer::SourceImageFormat::RGBA;
	case ITextureBuffer::ImageFormat::BGR:
		return ITextureBuffer::SourceImageFormat::BGR;
	case ITextureBuffer::ImageFormat::BGRA:
		return ITextureBuffer::SourceImageFormat::BGRA;
	case ITextureBuffer::ImageFormat::Depth:
	case ITextureBuffer::ImageFormat::DepthAndStencil:
	case ITextureBuffer::ImageFormat::X:
		return ITextureBuffer::SourceImageFormat::X;
	case ITextureBuffer::ImageFormat::XY:
		return ITextureBuffer::SourceImageFormat::XY;
	case ITextureBuffer::ImageFormat::XYZ:
		return ITextureBuffer::SourceImageFormat::XYZ;
	case ITextureBuffer::ImageFormat::XYZW:
		return ITextureBuffer::SourceImageFormat::XYZW;
	}
	return ITextureBuffer::SourceImageFormat::RGBA;
}

//----------------------------------------------------------------------------------------
ITextureBuffer::SourceDataFormat Direct3DFrameBufferOutput::GetOptimalDataFormat() const
{
	switch (_state[ReadIndex()].dataFormat)
	{
	case ITextureBuffer::DataFormat::Int8:
		return ITextureBuffer::SourceDataFormat::Int8;
	case ITextureBuffer::DataFormat::Int16:
		return ITextureBuffer::SourceDataFormat::Int16;
	case ITextureBuffer::DataFormat::Int32:
		return ITextureBuffer::SourceDataFormat::Int32;
	case ITextureBuffer::DataFormat::UInt8:
		return ITextureBuffer::SourceDataFormat::UInt8;
	case ITextureBuffer::DataFormat::UInt16:
		return ITextureBuffer::SourceDataFormat::UInt16;
	case ITextureBuffer::DataFormat::UInt32:
		return ITextureBuffer::SourceDataFormat::UInt32;
	case ITextureBuffer::DataFormat::Norm8:
		return ITextureBuffer::SourceDataFormat::Norm8;
	case ITextureBuffer::DataFormat::Norm16:
		return ITextureBuffer::SourceDataFormat::Norm16;
	case ITextureBuffer::DataFormat::UNorm8:
		return ITextureBuffer::SourceDataFormat::UNorm8;
	case ITextureBuffer::DataFormat::UNorm16:
		return ITextureBuffer::SourceDataFormat::UNorm16;
	case ITextureBuffer::DataFormat::Float16:
		return ITextureBuffer::SourceDataFormat::Float16;
	case ITextureBuffer::DataFormat::Float32:
	case ITextureBuffer::DataFormat::DepthUNorm16:
	case ITextureBuffer::DataFormat::DepthUNorm24:
	case ITextureBuffer::DataFormat::DepthUNorm24StencilUInt8:
	case ITextureBuffer::DataFormat::DepthFloat32:
	case ITextureBuffer::DataFormat::DepthFloat32StencilUInt8:
		return ITextureBuffer::SourceDataFormat::Float32;
	}
	return ITextureBuffer::SourceDataFormat::Float32;
}

//----------------------------------------------------------------------------------------
SuccessToken Direct3DFrameBufferOutput::ReadBufferData(void* targetBuffer, size_t targetBufferSizeInBytes, ITextureBuffer::SourceImageFormat imageFormat, ITextureBuffer::SourceDataFormat dataFormat, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels) const
{
	// Ensure we have a captured output image to work with
	if (!HasCapturedOutput())
	{
		_log->Error("Failed to read framebuffer output. A read was attempted when no captured image exists in the buffer.");
		return false;
	}

	// Determine the size of the image region to copy, and verify the output buffer is large enough to receive it.
	V2UInt32 imageSizeToCopy = GetCroppedImageDimensions(imageOffsetInPixels, imageRegionInPixels);
	size_t sampleCountToCopy = (size_t)imageSizeToCopy.X() * (size_t)imageSizeToCopy.Y();
	size_t outputElementSizeInBytes = ITextureBuffer::ElementCountPerPixelFromFormat(imageFormat) * ITextureBuffer::ByteSizePerElementFromFormat(dataFormat);
	size_t requiredBufferSizeInBytes = sampleCountToCopy * outputElementSizeInBytes;
	if (targetBufferSizeInBytes < requiredBufferSizeInBytes)
	{
		_log->Error("Failed to read framebuffer output. The requested image size requires {0} bytes, but the supplied buffer only has space for {1} bytes.", requiredBufferSizeInBytes, targetBufferSizeInBytes);
		return false;
	}

	// Pack our image info into a structure to pass to our conversion library
	auto readIndex = ReadIndex();
	TextureFormatConversion::FrameBufferInfo frameBufferInfo;
	frameBufferInfo.actualImageSize = _state[readIndex].actualImageSize;
	frameBufferInfo.actualImageOffset = _state[readIndex].actualImageOffset;
	frameBufferInfo.imageFormat = _state[readIndex].imageFormat;
	frameBufferInfo.dataFormat = _state[readIndex].dataFormat;
	frameBufferInfo.isStencilComponent = _state[readIndex].isStencilComponent;
	frameBufferInfo.elementCount = _state[readIndex].elementCount;
	frameBufferInfo.elementSizeInBytes = _state[readIndex].elementSizeInBytes;
	frameBufferInfo.pixelOffsetInBytes = _state[readIndex].pixelOffsetInBytes;
	frameBufferInfo.pixelStrideInBytes = _state[readIndex].pixelStrideInBytes;
	frameBufferInfo.rowStrideInBytes = _state[readIndex].rowStrideInBytes;
	frameBufferInfo.dataBuffer = _state[readIndex].dataBuffer.data();

	// Attempt to transfer the image data to the target buffer, performing any required format conversions.
	bool sourceDataFormatError = false;
	bool targetDataFormatError = false;
	bool result = TextureFormatConversion::ConvertFrameBufferOutputData(targetBuffer, targetBufferSizeInBytes, imageFormat, dataFormat, imageOffsetInPixels, imageSizeToCopy, frameBufferInfo, sourceDataFormatError, targetDataFormatError);
	if (targetDataFormatError)
	{
		_log->Error("Attempted to read framebuffer output with incompatible or unsupported image format {0} and data format {1}", imageFormat, dataFormat);
	}
	if (sourceDataFormatError)
	{
		_log->Critical("Attempted to read framebuffer output, but the internal buffer format wasn't handled with image format {0} and data format {1}.", _state[readIndex].imageFormat, _state[readIndex].dataFormat);
	}
	return result;
}

//----------------------------------------------------------------------------------------
// Capture methods
//----------------------------------------------------------------------------------------
bool Direct3DFrameBufferOutput::IsDetachingAfterCapture() const
{
	return _state[_drawIndex].detachAfterCapture;
}

//----------------------------------------------------------------------------------------
V2UInt32 Direct3DFrameBufferOutput::GetRequestedImageOffset() const
{
	return _state[_drawIndex].requestedImageOffset;
}

//----------------------------------------------------------------------------------------
V2UInt32 Direct3DFrameBufferOutput::GetRequestedImageSize() const
{
	return _state[_drawIndex].requestedImageSize;
}

//----------------------------------------------------------------------------------------
void Direct3DFrameBufferOutput::StoreCaptureData(V2UInt32 imageSize, const unsigned char* imageData, size_t imageDataSizeInBytes, ITextureBuffer::ImageFormat imageFormat, ITextureBuffer::DataFormat dataFormat, size_t elementCount, size_t elementSizeInBytes, size_t pixelOffsetInBytes, size_t pixelStrideInBytes, size_t rowStrideInBytes, bool isStencilComponent)
{
	_state[_drawIndex].actualImageSize = imageSize;
	_state[_drawIndex].actualImageOffset = _state[_drawIndex].requestedImageOffset;
	_state[_drawIndex].imageFormat = imageFormat;
	_state[_drawIndex].dataFormat = dataFormat;
	_state[_drawIndex].elementCount = elementCount;
	_state[_drawIndex].elementSizeInBytes = elementSizeInBytes;
	_state[_drawIndex].pixelOffsetInBytes = pixelOffsetInBytes;
	_state[_drawIndex].pixelStrideInBytes = pixelStrideInBytes;
	_state[_drawIndex].rowStrideInBytes = rowStrideInBytes;
	_state[_drawIndex].dataBuffer.assign(imageData, imageData + imageDataSizeInBytes);
	_state[_drawIndex].isStencilComponent = isStencilComponent;
	_state[_drawIndex].hasCapturedOutput = true;
	_renderer->FlagObjectModified(this);
}

//----------------------------------------------------------------------------------------
void Direct3DFrameBufferOutput::EnableCaptureReadFromCurrentDrawBuffer()
{
	_readFromCurrentDrawBufferEnabled = true;
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
void Direct3DFrameBufferOutput::MigrateBuildStateToDrawState()
{
	std::swap(_buildIndex, _drawIndex);
	_state[_drawIndex].hasCapturedOutput = false;
	_state[_buildIndex].detachAfterCapture = _state[_drawIndex].detachAfterCapture;
	_state[_buildIndex].requestedImageOffset = _state[_drawIndex].requestedImageOffset;
	_state[_buildIndex].requestedImageSize = _state[_drawIndex].requestedImageSize;
	_readFromCurrentDrawBufferEnabled = false;
}

//----------------------------------------------------------------------------------------
uint32_t Direct3DFrameBufferOutput::ReadIndex() const
{
	return (_readFromCurrentDrawBufferEnabled ? _drawIndex : _buildIndex);
}

} // namespace cobalt::graphics
