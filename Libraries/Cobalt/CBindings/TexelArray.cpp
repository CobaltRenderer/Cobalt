// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "TexelArray.h"
#include <Cobalt/RendererInterface/RendererInterface.pkg>

using namespace cobalt::graphics;

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_TexelArray_AllocateMemory(Cobalt_TexelArray array)
{
	auto _this = reinterpret_cast<ITexelArray*>(array);

	return _this->AllocateMemory() ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
void Cobalt_TexelArray_SetBufferLayout(Cobalt_TexelArray array, Cobalt_TexelArrayImageFormat imageFormat, Cobalt_TexelArrayDataFormat dataFormat, size_t entryCount)
{
	auto _this = reinterpret_cast<ITexelArray*>(array);

	_this->SetBufferLayout((ITexelArray::ImageFormat)imageFormat, (ITexelArray::DataFormat)dataFormat, entryCount);
}

//----------------------------------------------------------------------------------------
// Usage methods
//----------------------------------------------------------------------------------------
void Cobalt_TexelArray_SetUsageFlags(Cobalt_TexelArray array, Cobalt_TexelArrayUsageFlags usageFlags)
{
	auto _this = reinterpret_cast<ITexelArray*>(array);

	_this->SetUsageFlags((ITexelArray::UsageFlags)usageFlags);
}

//----------------------------------------------------------------------------------------
// Initial data methods
//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_TexelArray_SetInitialData(Cobalt_TexelArray array, const void* sourceBuffer, size_t sourceBufferSizeInBytes, Cobalt_TexelArraySourceImageFormat imageFormat, Cobalt_TexelArraySourceDataFormat dataFormat)
{
	auto _this = reinterpret_cast<ITexelArray*>(array);

	return _this->SetInitialData(sourceBuffer, sourceBufferSizeInBytes, (ITexelArray::SourceImageFormat)imageFormat, (ITexelArray::SourceDataFormat)dataFormat) ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
// Data update methods
//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_TexelArray_QueueDataUpdate(Cobalt_TexelArray array, const void* sourceBuffer, size_t sourceBufferSizeInBytes, Cobalt_TexelArraySourceImageFormat imageFormat, Cobalt_TexelArraySourceDataFormat dataFormat, size_t targetBufferOffset, Cobalt_TransferBatch transferBatch)
{
	auto _this = reinterpret_cast<ITexelArray*>(array);

	return _this->QueueDataUpdate(sourceBuffer, sourceBufferSizeInBytes, (ITexelArray::SourceImageFormat)imageFormat, (ITexelArray::SourceDataFormat)dataFormat, targetBufferOffset, reinterpret_cast<ITransferBatch*>(transferBatch)) ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
// Data transfer methods
//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_TexelArray_QueueDataTransfer(Cobalt_TexelArray array, Cobalt_TexelArray targetBuffer, size_t transferCount, size_t sourceBufferOffset, size_t targetBufferOffset, Cobalt_TransferBatch transferBatch)
{
	auto _this = reinterpret_cast<ITexelArray*>(array);

	return _this->QueueDataTransfer(reinterpret_cast<ITexelArray*>(targetBuffer), transferCount, sourceBufferOffset, targetBufferOffset, reinterpret_cast<ITransferBatch*>(transferBatch)) ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
// Output capture methods
//----------------------------------------------------------------------------------------
void Cobalt_TexelArray_AddOutputCaptureTarget(Cobalt_TexelArray array, Cobalt_TexelArrayOutput captureTarget)
{
	auto _this = reinterpret_cast<ITexelArray*>(array);

	_this->AddOutputCaptureTarget(reinterpret_cast<ITexelArrayOutput*>(captureTarget));
}

//----------------------------------------------------------------------------------------
void Cobalt_TexelArray_RemoveOutputCaptureTarget(Cobalt_TexelArray array, Cobalt_TexelArrayOutput captureTarget)
{
	auto _this = reinterpret_cast<ITexelArray*>(array);

	_this->RemoveOutputCaptureTarget(reinterpret_cast<ITexelArrayOutput*>(captureTarget));
}

//----------------------------------------------------------------------------------------
void Cobalt_TexelArray_Delete(Cobalt_TexelArray array)
{
	auto _this = reinterpret_cast<ITexelArray*>(array);

	_this->Delete();
}
