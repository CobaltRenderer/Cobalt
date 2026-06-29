// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "TextureBuffer3D.h"
#include <Cobalt/RendererInterface/RendererInterface.pkg>

using namespace cobalt::graphics;

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_TextureBuffer3D_AllocateMemory(Cobalt_TextureBuffer3D texture)
{
	auto _this = reinterpret_cast<ITextureBuffer3D*>(texture);

	return _this->AllocateMemory() ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
// Format methods
//----------------------------------------------------------------------------------------
void Cobalt_TextureBuffer3D_SetTextureFormat(Cobalt_TextureBuffer3D texture, Cobalt_ImageFormat imageFormat, Cobalt_DataFormat dataFormat)
{
	auto _this = reinterpret_cast<ITextureBuffer3D*>(texture);

	_this->SetTextureFormat((ITextureBuffer::ImageFormat)imageFormat, (ITextureBuffer::DataFormat)dataFormat);
}

//----------------------------------------------------------------------------------------
void Cobalt_TextureBuffer3D_SetTextureDimensions(Cobalt_TextureBuffer3D texture, const uint32_t imageDimensions[3], int mipmapLevelCount)
{
	auto _this = reinterpret_cast<ITextureBuffer3D*>(texture);

	_this->SetTextureDimensions(V3UInt32(imageDimensions[0], imageDimensions[1], imageDimensions[2]), mipmapLevelCount);
}

//----------------------------------------------------------------------------------------
Cobalt_ImageFormat Cobalt_TextureBuffer3D_AllocatedImageFormat(Cobalt_TextureBuffer3D texture)
{
	auto _this = reinterpret_cast<ITextureBuffer3D*>(texture);

	return (Cobalt_ImageFormat)_this->AllocatedImageFormat();
}

//----------------------------------------------------------------------------------------
Cobalt_DataFormat Cobalt_TextureBuffer3D_AllocatedDataFormat(Cobalt_TextureBuffer3D texture)
{
	auto _this = reinterpret_cast<ITextureBuffer3D*>(texture);

	return (Cobalt_DataFormat)_this->AllocatedDataFormat();
}

//----------------------------------------------------------------------------------------
int Cobalt_TextureBuffer3D_MipmapLevelCount(Cobalt_TextureBuffer3D texture)
{
	auto _this = reinterpret_cast<ITextureBuffer3D*>(texture);

	return _this->MipmapLevelCount();
}

//----------------------------------------------------------------------------------------
void Cobalt_TextureBuffer3D_MipmapLevelDimensions(Cobalt_TextureBuffer3D texture, int mipmapLevel, uint32_t dimensions[3])
{
	auto _this = reinterpret_cast<ITextureBuffer3D*>(texture);

	auto _dimensions = _this->MipmapLevelDimensions(mipmapLevel);
	dimensions[0] = _dimensions.X();
	dimensions[1] = _dimensions.Y();
	dimensions[2] = _dimensions.Z();
}

//----------------------------------------------------------------------------------------
// Data methods
//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_TextureBuffer3D_SetInitialData(Cobalt_TextureBuffer3D texture, const void* sourceBuffer, size_t sourceBufferSizeInBytes, Cobalt_SourceImageFormat imageFormat, Cobalt_SourceDataFormat dataFormat, int mipmapLevel)
{
	auto _this = reinterpret_cast<ITextureBuffer3D*>(texture);

	return _this->SetInitialData(sourceBuffer, sourceBufferSizeInBytes, (ITextureBuffer::SourceImageFormat)imageFormat, (ITextureBuffer::SourceDataFormat)dataFormat, mipmapLevel) ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_TextureBuffer3D_QueueDataUpdate(Cobalt_TextureBuffer3D texture, const void* sourceBuffer, size_t sourceBufferSizeInBytes, Cobalt_SourceImageFormat imageFormat, Cobalt_SourceDataFormat dataFormat, int mipmapLevel, const uint32_t imageOffsetInPixels[3], const uint32_t imageRegionInPixels[3], Cobalt_TransferBatch transferBatch)
{
	auto _this = reinterpret_cast<ITextureBuffer3D*>(texture);

	return _this->QueueDataUpdate(sourceBuffer, sourceBufferSizeInBytes, (ITextureBuffer::SourceImageFormat)imageFormat, (ITextureBuffer::SourceDataFormat)dataFormat, mipmapLevel, V3UInt32(imageOffsetInPixels[0], imageOffsetInPixels[1], imageOffsetInPixels[2]), V3UInt32(imageRegionInPixels[0], imageRegionInPixels[1], imageRegionInPixels[2]), reinterpret_cast<ITransferBatch*>(transferBatch)) ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
void Cobalt_TextureBuffer3D_Delete(Cobalt_TextureBuffer3D texture)
{
	auto _this = reinterpret_cast<ITextureBuffer3D*>(texture);

	_this->Delete();
}
