// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "FrameBufferOutput.h"
#include <Cobalt/RendererInterface/RendererInterface.pkg>

using namespace cobalt::graphics;

//----------------------------------------------------------------------------------------
// Configuration methods
//----------------------------------------------------------------------------------------
void Cobalt_FrameBufferOutput_SetDetachAfterCapture(Cobalt_FrameBufferOutput output, char state)
{
	auto _this = reinterpret_cast<IFrameBufferOutput*>(output);

	_this->SetDetachAfterCapture(state != 0);
}

//----------------------------------------------------------------------------------------
void Cobalt_FrameBufferOutput_SetFrameBufferCaptureRegion(Cobalt_FrameBufferOutput output, const uint32_t imageOffsetInPixels[2], const uint32_t imageRegionInPixels[2])
{
	auto _this = reinterpret_cast<IFrameBufferOutput*>(output);

	_this->SetFrameBufferCaptureRegion(V2UInt32(imageOffsetInPixels[0], imageOffsetInPixels[1]), V2UInt32(imageRegionInPixels[0], imageRegionInPixels[1]));
}

//----------------------------------------------------------------------------------------
// Data methods
//----------------------------------------------------------------------------------------
char Cobalt_FrameBufferOutput_HasCapturedOutput(Cobalt_FrameBufferOutput output)
{
	auto _this = reinterpret_cast<IFrameBufferOutput*>(output);

	return _this->HasCapturedOutput() ? 1 : 0;
}

//----------------------------------------------------------------------------------------
void Cobalt_FrameBufferOutput_ClearCapturedOutput(Cobalt_FrameBufferOutput output)
{
	auto _this = reinterpret_cast<IFrameBufferOutput*>(output);

	return _this->ClearCapturedOutput();
}

//----------------------------------------------------------------------------------------
void Cobalt_FrameBufferOutput_GetImageDimensions(Cobalt_FrameBufferOutput output, uint32_t dimensions[2])
{
	auto _this = reinterpret_cast<IFrameBufferOutput*>(output);

	auto _dimensions = _this->GetImageDimensions();
	dimensions[0] = _dimensions.X();
	dimensions[1] = _dimensions.Y();
}

//----------------------------------------------------------------------------------------
void Cobalt_FrameBufferOutput_GetCroppedImageDimensions(Cobalt_FrameBufferOutput output, const uint32_t imageOffsetInPixels[2], const uint32_t imageRegionInPixels[2], uint32_t dimensions[2])
{
	auto _this = reinterpret_cast<IFrameBufferOutput*>(output);

	auto _dimensions = _this->GetCroppedImageDimensions(V2UInt32(imageOffsetInPixels[0], imageOffsetInPixels[1]), V2UInt32(imageRegionInPixels[0], imageRegionInPixels[1]));
	dimensions[0] = _dimensions.X();
	dimensions[1] = _dimensions.Y();
}

//----------------------------------------------------------------------------------------
Cobalt_SourceImageFormat Cobalt_FrameBufferOutput_GetOptimalImageFormat(Cobalt_FrameBufferOutput output)
{
	auto _this = reinterpret_cast<IFrameBufferOutput*>(output);

	return (Cobalt_SourceImageFormat)_this->GetOptimalImageFormat();
}

//----------------------------------------------------------------------------------------
Cobalt_SourceDataFormat Cobalt_FrameBufferOutput_GetOptimalDataFormat(Cobalt_FrameBufferOutput output)
{
	auto _this = reinterpret_cast<IFrameBufferOutput*>(output);

	return (Cobalt_SourceDataFormat)_this->GetOptimalDataFormat();
}

//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_FrameBufferOutput_ReadBufferData(Cobalt_FrameBufferOutput output, void* targetBuffer, size_t targetBufferSizeInBytes, Cobalt_SourceImageFormat imageFormat, Cobalt_SourceDataFormat dataFormat, const uint32_t imageOffsetInPixels[2], const uint32_t imageRegionInPixels[2])
{
	auto _this = reinterpret_cast<IFrameBufferOutput*>(output);

	return _this->ReadBufferData(targetBuffer, targetBufferSizeInBytes, (ITextureBuffer::SourceImageFormat)imageFormat, (ITextureBuffer::SourceDataFormat)dataFormat, V2UInt32(imageOffsetInPixels[0], imageOffsetInPixels[1]), V2UInt32(imageRegionInPixels[0], imageRegionInPixels[1])) ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
void Cobalt_FrameBufferOutput_Delete(Cobalt_FrameBufferOutput output)
{
	auto _this = reinterpret_cast<IFrameBufferOutput*>(output);

	_this->Delete();
}
