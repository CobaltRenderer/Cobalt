// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "TextureBufferCube.h"
#include <Cobalt/RendererInterface/RendererInterface.pkg>

using namespace cobalt::graphics;

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_TextureBufferCube_AllocateMemory(Cobalt_TextureBufferCube texture)
{
	auto _this = reinterpret_cast<ITextureBufferCube*>(texture);

	return _this->AllocateMemory() ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
// Format methods
//----------------------------------------------------------------------------------------
void Cobalt_TextureBufferCube_SetTextureFormat(Cobalt_TextureBufferCube texture, Cobalt_ImageFormat imageFormat, Cobalt_DataFormat dataFormat)
{
	auto _this = reinterpret_cast<ITextureBufferCube*>(texture);

	_this->SetTextureFormat((ITextureBuffer::ImageFormat)imageFormat, (ITextureBuffer::DataFormat)dataFormat);
}

//----------------------------------------------------------------------------------------
void Cobalt_TextureBufferCube_SetTextureDimensions(Cobalt_TextureBufferCube texture, uint32_t faceLength, int mipmapLevelCount)
{
	auto _this = reinterpret_cast<ITextureBufferCube*>(texture);

	_this->SetTextureDimensions(faceLength, mipmapLevelCount);
}

//----------------------------------------------------------------------------------------
Cobalt_ImageFormat Cobalt_TextureBufferCube_AllocatedImageFormat(Cobalt_TextureBufferCube texture)
{
	auto _this = reinterpret_cast<ITextureBufferCube*>(texture);

	return (Cobalt_ImageFormat)_this->AllocatedImageFormat();
}

//----------------------------------------------------------------------------------------
Cobalt_DataFormat Cobalt_TextureBufferCube_AllocatedDataFormat(Cobalt_TextureBufferCube texture)
{
	auto _this = reinterpret_cast<ITextureBufferCube*>(texture);

	return (Cobalt_DataFormat)_this->AllocatedDataFormat();
}

//----------------------------------------------------------------------------------------
int Cobalt_TextureBufferCube_MipmapLevelCount(Cobalt_TextureBufferCube texture)
{
	auto _this = reinterpret_cast<ITextureBufferCube*>(texture);

	return _this->MipmapLevelCount();
}

//----------------------------------------------------------------------------------------
void Cobalt_TextureBufferCube_MipmapLevelDimensions(Cobalt_TextureBufferCube texture, int mipmapLevel, uint32_t dimensions[2])
{
	auto _this = reinterpret_cast<ITextureBufferCube*>(texture);

	auto _dimensions = _this->MipmapLevelDimensions(mipmapLevel);
	dimensions[0] = _dimensions.X();
	dimensions[1] = _dimensions.Y();
}

//----------------------------------------------------------------------------------------
// Data methods
//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_TextureBufferCube_SetInitialData(Cobalt_TextureBufferCube texture, const void* sourceBuffer, size_t sourceBufferSizeInBytes, Cobalt_SourceImageFormat imageFormat, Cobalt_SourceDataFormat dataFormat, Cobalt_CubeMapFace targetFace, int mipmapLevel)
{
	auto _this = reinterpret_cast<ITextureBufferCube*>(texture);

	return _this->SetInitialData(sourceBuffer, sourceBufferSizeInBytes, (ITextureBuffer::SourceImageFormat)imageFormat, (ITextureBuffer::SourceDataFormat)dataFormat, (ITextureBuffer::CubeMapFace)targetFace, mipmapLevel) ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_TextureBufferCube_QueueDataUpdate(Cobalt_TextureBufferCube texture, const void* sourceBuffer, size_t sourceBufferSizeInBytes, Cobalt_SourceImageFormat imageFormat, Cobalt_SourceDataFormat dataFormat, Cobalt_CubeMapFace targetFace, int mipmapLevel, const uint32_t imageOffsetInPixels[2], const uint32_t imageRegionInPixels[2], Cobalt_TransferBatch transferBatch)
{
	auto _this = reinterpret_cast<ITextureBufferCube*>(texture);

	return _this->QueueDataUpdate(sourceBuffer, sourceBufferSizeInBytes, (ITextureBuffer::SourceImageFormat)imageFormat, (ITextureBuffer::SourceDataFormat)dataFormat, (ITextureBuffer::CubeMapFace)targetFace, mipmapLevel, V2UInt32(imageOffsetInPixels[0], imageOffsetInPixels[1]), V2UInt32(imageRegionInPixels[0], imageRegionInPixels[1]), reinterpret_cast<ITransferBatch*>(transferBatch)) ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
void Cobalt_TextureBufferCube_Delete(Cobalt_TextureBufferCube texture)
{
	auto _this = reinterpret_cast<ITextureBufferCube*>(texture);

	_this->Delete();
}
