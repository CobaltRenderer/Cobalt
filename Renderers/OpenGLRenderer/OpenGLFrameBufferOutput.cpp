// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "OpenGLFrameBufferOutput.h"
#include "OpenGLRenderer.h"
#include <Internal/TextureFormatConversion/TextureFormatConversion.pkg>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
OpenGLFrameBufferOutput::OpenGLFrameBufferOutput(cobalt::logging::ILogger* log, OpenGLRenderer* renderer)
: _log(log), _renderer(renderer), _buildIndex(0), _drawIndex(1), _readFromCurrentDrawBufferEnabled(false)
{
	_state[_drawIndex].hasCapturedOutput = false;
	_state[_buildIndex].hasCapturedOutput = false;
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void OpenGLFrameBufferOutput::Delete()
{
	_renderer->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
// Configuration methods
//----------------------------------------------------------------------------------------
void OpenGLFrameBufferOutput::SetDetachAfterCapture(bool state)
{
	_state[_buildIndex].detachAfterCapture = state;
	_renderer->FlagObjectModified(this);
}

//----------------------------------------------------------------------------------------
void OpenGLFrameBufferOutput::SetFrameBufferCaptureRegion(const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels)
{
	_state[_buildIndex].requestedImageOffset = imageOffsetInPixels;
	_state[_buildIndex].requestedImageSize = imageRegionInPixels;
	_renderer->FlagObjectModified(this);
}

//----------------------------------------------------------------------------------------
// Data methods
//----------------------------------------------------------------------------------------
bool OpenGLFrameBufferOutput::HasCapturedOutput() const
{
	return _state[ReadIndex()].hasCapturedOutput;
}

//----------------------------------------------------------------------------------------
void OpenGLFrameBufferOutput::ClearCapturedOutput()
{
	_state[ReadIndex()].dataBuffer.clear();
	_state[ReadIndex()].hasCapturedOutput = false;
}

//----------------------------------------------------------------------------------------
V2UInt32 OpenGLFrameBufferOutput::GetImageDimensions() const
{
	return _state[ReadIndex()].actualImageSize;
}

//----------------------------------------------------------------------------------------
V2UInt32 OpenGLFrameBufferOutput::GetCroppedImageDimensions(const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels) const
{
	return CalculateCroppedImageDimensions(GetImageDimensions(), imageOffsetInPixels, imageRegionInPixels);
}

//----------------------------------------------------------------------------------------
V2UInt32 OpenGLFrameBufferOutput::CalculateCroppedImageDimensions(const V2UInt32& imageSizeInPixels, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels)
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
size_t OpenGLFrameBufferOutput::CalculatePixelCountForRegion(const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels) const
{
	V2UInt32 croppedImageDimensions = GetCroppedImageDimensions(imageOffsetInPixels, imageRegionInPixels);
	size_t pixelCount = (size_t)croppedImageDimensions.X() * (size_t)croppedImageDimensions.Y();
	return pixelCount;
}

//----------------------------------------------------------------------------------------
ITextureBuffer::SourceImageFormat OpenGLFrameBufferOutput::GetOptimalImageFormat() const
{
	return _state[ReadIndex()].optimalSourceImageFormat;
}

//----------------------------------------------------------------------------------------
ITextureBuffer::SourceDataFormat OpenGLFrameBufferOutput::GetOptimalDataFormat() const
{
	return _state[ReadIndex()].optimalSourceDataFormat;
}

//----------------------------------------------------------------------------------------
SuccessToken OpenGLFrameBufferOutput::ReadBufferData(void* targetBuffer, size_t targetBufferSizeInBytes, ITextureBuffer::SourceImageFormat imageFormat, ITextureBuffer::SourceDataFormat dataFormat, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels) const
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
	frameBufferInfo.elementCount = ITextureBuffer::ElementCountPerPixelFromFormat(_state[readIndex].imageFormat);
	frameBufferInfo.elementSizeInBytes = ITextureBuffer::ByteSizePerElementFromFormat(_state[readIndex].dataFormat);
	frameBufferInfo.pixelOffsetInBytes = 0;
	frameBufferInfo.pixelStrideInBytes = frameBufferInfo.elementSizeInBytes * frameBufferInfo.elementCount;
	frameBufferInfo.rowStrideInBytes = frameBufferInfo.pixelStrideInBytes * frameBufferInfo.actualImageSize.X();
	frameBufferInfo.dataBuffer = _state[readIndex].dataBuffer.data();
	frameBufferInfo.rowsAreReversed = true;

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
bool OpenGLFrameBufferOutput::IsDetachingAfterCapture() const
{
	return _state[_drawIndex].detachAfterCapture;
}

//----------------------------------------------------------------------------------------
V2UInt32 OpenGLFrameBufferOutput::GetRequestedImageOffset() const
{
	return _state[_drawIndex].requestedImageOffset;
}

//----------------------------------------------------------------------------------------
V2UInt32 OpenGLFrameBufferOutput::GetRequestedImageSize() const
{
	return _state[_drawIndex].requestedImageSize;
}

//----------------------------------------------------------------------------------------
void OpenGLFrameBufferOutput::StoreCaptureData(const V2UInt32& imageSize, const unsigned char* imageData, size_t imageDataSizeInBytes, ITextureBuffer::ImageFormat imageFormat, ITextureBuffer::DataFormat dataFormat, ITextureBuffer::SourceImageFormat optimalSourceImageFormat, ITextureBuffer::SourceDataFormat optimalSourceDataFormat, bool isStencilComponent)
{
	_state[_drawIndex].actualImageSize = imageSize;
	_state[_drawIndex].actualImageOffset = _state[_drawIndex].requestedImageOffset;
	_state[_drawIndex].imageFormat = imageFormat;
	_state[_drawIndex].dataFormat = dataFormat;
	_state[_drawIndex].optimalSourceImageFormat = optimalSourceImageFormat;
	_state[_drawIndex].optimalSourceDataFormat = optimalSourceDataFormat;
	_state[_drawIndex].dataBuffer.assign(imageData, imageData + imageDataSizeInBytes);
	_state[_drawIndex].isStencilComponent = isStencilComponent;
	_state[_drawIndex].hasCapturedOutput = true;
	_renderer->FlagObjectModified(this);
}

//----------------------------------------------------------------------------------------
void OpenGLFrameBufferOutput::EnableCaptureReadFromCurrentDrawBuffer()
{
	_readFromCurrentDrawBufferEnabled = true;
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
void OpenGLFrameBufferOutput::MigrateBuildStateToDrawState()
{
	std::swap(_buildIndex, _drawIndex);
	_state[_drawIndex].hasCapturedOutput = false;
	_state[_buildIndex].detachAfterCapture = _state[_drawIndex].detachAfterCapture;
	_state[_buildIndex].requestedImageOffset = _state[_drawIndex].requestedImageOffset;
	_state[_buildIndex].requestedImageSize = _state[_drawIndex].requestedImageSize;
	_readFromCurrentDrawBufferEnabled = false;
}

//----------------------------------------------------------------------------------------
uint32_t OpenGLFrameBufferOutput::ReadIndex() const
{
	return (_readFromCurrentDrawBufferEnabled ? _drawIndex : _buildIndex);
}

} // namespace cobalt::graphics
