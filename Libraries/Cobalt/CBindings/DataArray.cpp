// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "DataArray.h"
#include <Cobalt/RendererInterface/RendererInterface.pkg>

using namespace cobalt::graphics;

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_DataArray_AllocateMemory(Cobalt_DataArray array)
{
	auto _this = reinterpret_cast<IDataArray*>(array);

	return _this->AllocateMemory() ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
void Cobalt_DataArray_SetBufferLayout(Cobalt_DataArray array, size_t entryStrideInBytes, size_t entryCount, char hasCounter, uint32_t counterResetValue)
{
	auto _this = reinterpret_cast<IDataArray*>(array);

	_this->SetBufferLayout(entryStrideInBytes, entryCount, hasCounter != 0, counterResetValue);
}

//----------------------------------------------------------------------------------------
// Usage methods
//----------------------------------------------------------------------------------------
void Cobalt_DataArray_SetUsageFlags(Cobalt_DataArray array, Cobalt_DataArrayUsageFlags usageFlags)
{
	auto _this = reinterpret_cast<IDataArray*>(array);

	_this->SetUsageFlags((IDataArray::UsageFlags)usageFlags);
}

//----------------------------------------------------------------------------------------
// Initial data methods
//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_DataArray_SetInitialData(Cobalt_DataArray array, const void* sourceBuffer, size_t sourceBufferSizeInBytes)
{
	auto _this = reinterpret_cast<IDataArray*>(array);

	return _this->SetInitialData(sourceBuffer, sourceBufferSizeInBytes) ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
// Data update methods
//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_DataArray_QueueDataUpdate(Cobalt_DataArray array, const void* sourceBuffer, size_t sourceBufferSizeInBytes, size_t targetBufferOffsetInBytes, Cobalt_TransferBatch transferBatch)
{
	auto _this = reinterpret_cast<IDataArray*>(array);

	return _this->QueueDataUpdate(sourceBuffer, sourceBufferSizeInBytes, targetBufferOffsetInBytes, reinterpret_cast<ITransferBatch*>(transferBatch)) ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
void Cobalt_DataArray_UpdateCounterResetValue(Cobalt_DataArray array, uint32_t counterResetValue)
{
	auto _this = reinterpret_cast<IDataArray*>(array);

	return _this->UpdateCounterResetValue(counterResetValue);
}

//----------------------------------------------------------------------------------------
// Data transfer methods
//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_DataArray_QueueDataTransfer(Cobalt_DataArray array, Cobalt_DataArray targetBuffer, size_t transferCountInBytes, size_t sourceBufferOffsetInBytes, size_t targetBufferOffsetInBytes, Cobalt_TransferBatch transferBatch)
{
	auto _this = reinterpret_cast<IDataArray*>(array);

	return _this->QueueDataTransfer(reinterpret_cast<IDataArray*>(targetBuffer), transferCountInBytes, sourceBufferOffsetInBytes, targetBufferOffsetInBytes, reinterpret_cast<ITransferBatch*>(transferBatch)) ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
// Output capture methods
//----------------------------------------------------------------------------------------
void Cobalt_DataArray_AddOutputCaptureTarget(Cobalt_DataArray array, Cobalt_DataArrayOutput captureTarget)
{
	auto _this = reinterpret_cast<IDataArray*>(array);

	_this->AddOutputCaptureTarget(reinterpret_cast<IDataArrayOutput*>(captureTarget));
}

//----------------------------------------------------------------------------------------;
void Cobalt_DataArray_RemoveOutputCaptureTarget(Cobalt_DataArray array, Cobalt_DataArrayOutput captureTarget)
{
	auto _this = reinterpret_cast<IDataArray*>(array);

	_this->RemoveOutputCaptureTarget(reinterpret_cast<IDataArrayOutput*>(captureTarget));
}

//----------------------------------------------------------------------------------------
void Cobalt_DataArray_Delete(Cobalt_DataArray array)
{
	auto _this = reinterpret_cast<IDataArray*>(array);

	_this->Delete();
}
