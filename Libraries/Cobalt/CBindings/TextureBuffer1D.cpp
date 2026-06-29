// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "TextureBuffer1D.h"
#include <Cobalt/RendererInterface/RendererInterface.pkg>

using namespace cobalt::graphics;

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_TextureBuffer1D_AllocateMemory(Cobalt_TextureBuffer1D texture)
{
	auto _this = reinterpret_cast<ITextureBuffer1D*>(texture);

	return _this->AllocateMemory() ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
// Format methods
//----------------------------------------------------------------------------------------
void Cobalt_TextureBuffer1D_SetTextureFormat(Cobalt_TextureBuffer1D texture, Cobalt_ImageFormat imageFormat, Cobalt_DataFormat dataFormat)
{
	auto _this = reinterpret_cast<ITextureBuffer1D*>(texture);

	_this->SetTextureFormat((ITextureBuffer::ImageFormat)imageFormat, (ITextureBuffer::DataFormat)dataFormat);
}

//----------------------------------------------------------------------------------------
void Cobalt_TextureBuffer1D_SetTextureDimensions(Cobalt_TextureBuffer1D texture, uint32_t imageDimensions, int mipmapLevelCount)
{
	auto _this = reinterpret_cast<ITextureBuffer1D*>(texture);

	_this->SetTextureDimensions(V1UInt32(imageDimensions), mipmapLevelCount);
}

//----------------------------------------------------------------------------------------
Cobalt_ImageFormat Cobalt_TextureBuffer1D_AllocatedImageFormat(Cobalt_TextureBuffer1D texture)
{
	auto _this = reinterpret_cast<ITextureBuffer1D*>(texture);

	return (Cobalt_ImageFormat)_this->AllocatedImageFormat();
}

//----------------------------------------------------------------------------------------
Cobalt_DataFormat Cobalt_TextureBuffer1D_AllocatedDataFormat(Cobalt_TextureBuffer1D texture)
{
	auto _this = reinterpret_cast<ITextureBuffer1D*>(texture);

	return (Cobalt_DataFormat)_this->AllocatedDataFormat();
}

//----------------------------------------------------------------------------------------
int Cobalt_TextureBuffer1D_MipmapLevelCount(Cobalt_TextureBuffer1D texture)
{
	auto _this = reinterpret_cast<ITextureBuffer1D*>(texture);

	return _this->MipmapLevelCount();
}

//----------------------------------------------------------------------------------------
void Cobalt_TextureBuffer1D_MipmapLevelDimensions(Cobalt_TextureBuffer1D texture, int mipmapLevel, uint32_t* dimensions)
{
	auto _this = reinterpret_cast<ITextureBuffer1D*>(texture);

	*dimensions = _this->MipmapLevelDimensions(mipmapLevel).X();
}

//----------------------------------------------------------------------------------------
// Data methods
//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_TextureBuffer1D_SetInitialData(Cobalt_TextureBuffer1D texture, const void* sourceBuffer, size_t sourceBufferSizeInBytes, Cobalt_SourceImageFormat imageFormat, Cobalt_SourceDataFormat dataFormat, int mipmapLevel)
{
	auto _this = reinterpret_cast<ITextureBuffer1D*>(texture);

	return _this->SetInitialData(sourceBuffer, sourceBufferSizeInBytes, (ITextureBuffer::SourceImageFormat)imageFormat, (ITextureBuffer::SourceDataFormat)dataFormat, mipmapLevel) ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_TextureBuffer1D_QueueDataUpdate(Cobalt_TextureBuffer1D texture, const void* sourceBuffer, size_t sourceBufferSizeInBytes, Cobalt_SourceImageFormat imageFormat, Cobalt_SourceDataFormat dataFormat, int mipmapLevel, uint32_t imageOffsetInPixels, uint32_t imageRegionInPixels, Cobalt_TransferBatch transferBatch)
{
	auto _this = reinterpret_cast<ITextureBuffer1D*>(texture);

	return _this->QueueDataUpdate(sourceBuffer, sourceBufferSizeInBytes, (ITextureBuffer::SourceImageFormat)imageFormat, (ITextureBuffer::SourceDataFormat)dataFormat, mipmapLevel, V1UInt32(imageOffsetInPixels), V1UInt32(imageRegionInPixels), reinterpret_cast<ITransferBatch*>(transferBatch)) ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
void Cobalt_TextureBuffer1D_Delete(Cobalt_TextureBuffer1D texture)
{
	auto _this = reinterpret_cast<ITextureBuffer1D*>(texture);

	_this->Delete();
}
