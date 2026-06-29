// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "TexelArrayOutput.h"
#include <Cobalt/RendererInterface/RendererInterface.pkg>

using namespace cobalt::graphics;

//----------------------------------------------------------------------------------------
// Configuration methods
//----------------------------------------------------------------------------------------
void Cobalt_TexelArrayOutput_SetDetachAfterCapture(Cobalt_TexelArrayOutput output, char state)
{
	auto _this = reinterpret_cast<ITexelArrayOutput*>(output);

	_this->SetDetachAfterCapture(state != 0);
}

//----------------------------------------------------------------------------------------
void Cobalt_TexelArrayOutput_SetArrayCaptureRegion(Cobalt_TexelArrayOutput output, size_t captureEntryCount, size_t bufferOffset)
{
	auto _this = reinterpret_cast<ITexelArrayOutput*>(output);

	_this->SetArrayCaptureRegion(captureEntryCount, bufferOffset);
}

//----------------------------------------------------------------------------------------
// Data methods
//----------------------------------------------------------------------------------------
char Cobalt_TexelArrayOutput_HasCapturedOutput(Cobalt_TexelArrayOutput output)
{
	auto _this = reinterpret_cast<ITexelArrayOutput*>(output);

	return _this->HasCapturedOutput() ? 1 : 0;
}

//----------------------------------------------------------------------------------------
void Cobalt_TexelArrayOutput_ClearCapturedOutput(Cobalt_TexelArrayOutput output)
{
	auto _this = reinterpret_cast<ITexelArrayOutput*>(output);

	_this->ClearCapturedOutput();
}

//----------------------------------------------------------------------------------------
size_t Cobalt_TexelArrayOutput_GetEntryCount(Cobalt_TexelArrayOutput output)
{
	auto _this = reinterpret_cast<ITexelArrayOutput*>(output);

	return _this->GetEntryCount();
}

//----------------------------------------------------------------------------------------
Cobalt_TexelArraySourceImageFormat Cobalt_TexelArrayOutput_GetOptimalImageFormat(Cobalt_TexelArrayOutput output)
{
	auto _this = reinterpret_cast<ITexelArrayOutput*>(output);

	return (Cobalt_TexelArraySourceImageFormat)_this->GetOptimalImageFormat();
}

//----------------------------------------------------------------------------------------
Cobalt_TexelArraySourceDataFormat Cobalt_TexelArrayOutput_GetOptimalDataFormat(Cobalt_TexelArrayOutput output)
{
	auto _this = reinterpret_cast<ITexelArrayOutput*>(output);

	return (Cobalt_TexelArraySourceDataFormat)_this->GetOptimalDataFormat();
}

//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_TexelArrayOutput_ReadBufferData(Cobalt_TexelArrayOutput output, void* targetBuffer, size_t targetBufferSizeInBytes, Cobalt_TexelArraySourceImageFormat imageFormat, Cobalt_TexelArraySourceDataFormat dataFormat)
{
	auto _this = reinterpret_cast<ITexelArrayOutput*>(output);

	return _this->ReadBufferData(targetBuffer, targetBufferSizeInBytes, (ITexelArray::SourceImageFormat)imageFormat, (ITexelArray::SourceDataFormat)dataFormat) ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
void Cobalt_TexelArrayOutput_Delete(Cobalt_TexelArrayOutput output)
{
	auto _this = reinterpret_cast<ITexelArrayOutput*>(output);

	_this->Delete();
}
