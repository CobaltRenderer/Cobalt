// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "TextureBufferCubeArray.h"
#include <Cobalt/RendererInterface/RendererInterface.pkg>

using namespace cobalt::graphics;

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_TextureBufferCubeArray_AllocateMemory(Cobalt_TextureBufferCubeArray texture)
{
	auto _this = reinterpret_cast<ITextureBufferCubeArray*>(texture);

	return _this->AllocateMemory() ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
// Format methods
//----------------------------------------------------------------------------------------
void Cobalt_TextureBufferCubeArray_SetTextureFormat(Cobalt_TextureBufferCubeArray texture, Cobalt_ImageFormat imageFormat, Cobalt_DataFormat dataFormat)
{
	auto _this = reinterpret_cast<ITextureBufferCubeArray*>(texture);

	_this->SetTextureFormat((ITextureBuffer::ImageFormat)imageFormat, (ITextureBuffer::DataFormat)dataFormat);
}

//----------------------------------------------------------------------------------------
void Cobalt_TextureBufferCubeArray_SetTextureDimensions(Cobalt_TextureBufferCubeArray texture, uint32_t faceLength, size_t arraySize, int mipmapLevelCount)
{
	auto _this = reinterpret_cast<ITextureBufferCubeArray*>(texture);

	_this->SetTextureDimensions(faceLength, arraySize, mipmapLevelCount);
}

//----------------------------------------------------------------------------------------
Cobalt_ImageFormat Cobalt_TextureBufferCubeArray_AllocatedImageFormat(Cobalt_TextureBufferCubeArray texture)
{
	auto _this = reinterpret_cast<ITextureBufferCubeArray*>(texture);

	return (Cobalt_ImageFormat)_this->AllocatedImageFormat();
}

//----------------------------------------------------------------------------------------
Cobalt_DataFormat Cobalt_TextureBufferCubeArray_AllocatedDataFormat(Cobalt_TextureBufferCubeArray texture)
{
	auto _this = reinterpret_cast<ITextureBufferCubeArray*>(texture);

	return (Cobalt_DataFormat)_this->AllocatedDataFormat();
}

//----------------------------------------------------------------------------------------
int Cobalt_TextureBufferCubeArray_MipmapLevelCount(Cobalt_TextureBufferCubeArray texture)
{
	auto _this = reinterpret_cast<ITextureBufferCubeArray*>(texture);

	return _this->MipmapLevelCount();
}

//----------------------------------------------------------------------------------------
void Cobalt_TextureBufferCubeArray_MipmapLevelDimensions(Cobalt_TextureBufferCubeArray texture, int mipmapLevel, uint32_t dimensions[2])
{
	auto _this = reinterpret_cast<ITextureBufferCubeArray*>(texture);

	auto _dimensions = _this->MipmapLevelDimensions(mipmapLevel);
	dimensions[0] = _dimensions.X();
	dimensions[1] = _dimensions.Y();
}

//----------------------------------------------------------------------------------------
// Data methods
//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_TextureBufferCubeArray_SetInitialData(Cobalt_TextureBufferCubeArray texture, const void* sourceBuffer, size_t sourceBufferSizeInBytes, Cobalt_SourceImageFormat imageFormat, Cobalt_SourceDataFormat dataFormat, Cobalt_CubeMapFace targetFace, size_t arrayIndex, int mipmapLevel)
{
	auto _this = reinterpret_cast<ITextureBufferCubeArray*>(texture);

	return _this->SetInitialData(sourceBuffer, sourceBufferSizeInBytes, (ITextureBuffer::SourceImageFormat)imageFormat, (ITextureBuffer::SourceDataFormat)dataFormat, (ITextureBuffer::CubeMapFace)targetFace, arrayIndex, mipmapLevel) ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_TextureBufferCubeArray_QueueDataUpdate(Cobalt_TextureBufferCubeArray texture, const void* sourceBuffer, size_t sourceBufferSizeInBytes, Cobalt_SourceImageFormat imageFormat, Cobalt_SourceDataFormat dataFormat, Cobalt_CubeMapFace targetFace, size_t arrayIndex, int mipmapLevel, const uint32_t imageOffsetInPixels[2], const uint32_t imageRegionInPixels[2], Cobalt_TransferBatch transferBatch)
{
	auto _this = reinterpret_cast<ITextureBufferCubeArray*>(texture);

	return _this->QueueDataUpdate(sourceBuffer, sourceBufferSizeInBytes, (ITextureBuffer::SourceImageFormat)imageFormat, (ITextureBuffer::SourceDataFormat)dataFormat, (ITextureBuffer::CubeMapFace)targetFace, arrayIndex, mipmapLevel, V2UInt32(imageOffsetInPixels[0], imageOffsetInPixels[1]), V2UInt32(imageRegionInPixels[0], imageRegionInPixels[1]), reinterpret_cast<ITransferBatch*>(transferBatch)) ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
void Cobalt_TextureBufferCubeArray_Delete(Cobalt_TextureBufferCubeArray texture)
{
	auto _this = reinterpret_cast<ITextureBufferCubeArray*>(texture);

	_this->Delete();
}
