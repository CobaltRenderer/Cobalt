// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "DataArrayOutput.h"
#include <Cobalt/RendererInterface/RendererInterface.pkg>

using namespace cobalt::graphics;

//----------------------------------------------------------------------------------------
// Configuration methods
//----------------------------------------------------------------------------------------
void Cobalt_DataArrayOutput_SetDetachAfterCapture(Cobalt_DataArrayOutput output, char state)
{
	auto _this = reinterpret_cast<IDataArrayOutput*>(output);

	_this->SetDetachAfterCapture(state != 0);
}

//----------------------------------------------------------------------------------------
void Cobalt_DataArrayOutput_SetArrayCaptureRegion(Cobalt_DataArrayOutput output, size_t captureEntryCount, size_t bufferOffset, char captureCounterValue)
{
	auto _this = reinterpret_cast<IDataArrayOutput*>(output);

	_this->SetArrayCaptureRegion(captureEntryCount, bufferOffset, captureCounterValue != 0);
}

//----------------------------------------------------------------------------------------
// Data methods
//----------------------------------------------------------------------------------------
char Cobalt_DataArrayOutput_HasCapturedOutput(Cobalt_DataArrayOutput output)
{
	auto _this = reinterpret_cast<IDataArrayOutput*>(output);

	return _this->HasCapturedOutput() ? 1 : 0;
}

//----------------------------------------------------------------------------------------
char Cobalt_DataArrayOutput_HasCapturedCounterValue(Cobalt_DataArrayOutput output)
{
	auto _this = reinterpret_cast<IDataArrayOutput*>(output);

	return _this->HasCapturedCounterValue() ? 1 : 0;
}

//----------------------------------------------------------------------------------------
void Cobalt_DataArrayOutput_ClearCapturedOutput(Cobalt_DataArrayOutput output)
{
	auto _this = reinterpret_cast<IDataArrayOutput*>(output);

	_this->ClearCapturedOutput();
}

//----------------------------------------------------------------------------------------
size_t Cobalt_DataArrayOutput_GetEntryCount(Cobalt_DataArrayOutput output)
{
	auto _this = reinterpret_cast<IDataArrayOutput*>(output);

	return _this->GetEntryCount();
}

//----------------------------------------------------------------------------------------
size_t Cobalt_DataArrayOutput_GetEntrySizeInBytes(Cobalt_DataArrayOutput output)
{
	auto _this = reinterpret_cast<IDataArrayOutput*>(output);

	return _this->GetEntrySizeInBytes();
}

//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_DataArrayOutput_ReadBufferData(Cobalt_DataArrayOutput output, void* targetBuffer, size_t targetBufferSizeInBytes)
{
	auto _this = reinterpret_cast<IDataArrayOutput*>(output);

	return _this->ReadBufferData(targetBuffer, targetBufferSizeInBytes) ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_DataArrayOutput_ReadCounterValue(Cobalt_DataArrayOutput output, uint32_t* counterValue)
{
	auto _this = reinterpret_cast<IDataArrayOutput*>(output);

	uint32_t value = 0;
	if (_this->ReadCounterValue(value))
	{
		*counterValue = value;
		return COBALT_SUCCESS;
	}

	return COBALT_FAILURE;
};

//----------------------------------------------------------------------------------------
void Cobalt_DataArrayOutput_Delete(Cobalt_DataArrayOutput output)
{
	auto _this = reinterpret_cast<IDataArrayOutput*>(output);

	_this->Delete();
};
