// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "TextureBuffer2DArray.h"
#include <Cobalt/RendererInterface/RendererInterface.pkg>

using namespace cobalt::graphics;

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_TextureBuffer2DArray_AllocateMemory(Cobalt_TextureBuffer2DArray texture)
{
	auto _this = reinterpret_cast<ITextureBuffer2DArray*>(texture);

	return _this->AllocateMemory() ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
// Format methods
//----------------------------------------------------------------------------------------
void Cobalt_TextureBuffer2DArray_SetTextureFormat(Cobalt_TextureBuffer2DArray texture, Cobalt_ImageFormat imageFormat, Cobalt_DataFormat dataFormat)
{
	auto _this = reinterpret_cast<ITextureBuffer2DArray*>(texture);

	_this->SetTextureFormat((ITextureBuffer::ImageFormat)imageFormat, (ITextureBuffer::DataFormat)dataFormat);
}

//----------------------------------------------------------------------------------------
void Cobalt_TextureBuffer2DArray_SetTextureDimensions(Cobalt_TextureBuffer2DArray texture, const uint32_t imageDimensions[2], size_t arraySize, int mipmapLevelCount)
{
	auto _this = reinterpret_cast<ITextureBuffer2DArray*>(texture);

	_this->SetTextureDimensions(V2UInt32(imageDimensions[0], imageDimensions[1]), arraySize, mipmapLevelCount);
}

//----------------------------------------------------------------------------------------
Cobalt_ImageFormat Cobalt_TextureBuffer2DArray_AllocatedImageFormat(Cobalt_TextureBuffer2DArray texture)
{
	auto _this = reinterpret_cast<ITextureBuffer2DArray*>(texture);

	return (Cobalt_ImageFormat)_this->AllocatedImageFormat();
}

//----------------------------------------------------------------------------------------
Cobalt_DataFormat Cobalt_TextureBuffer2DArray_AllocatedDataFormat(Cobalt_TextureBuffer2DArray texture)
{
	auto _this = reinterpret_cast<ITextureBuffer2DArray*>(texture);

	return (Cobalt_DataFormat)_this->AllocatedDataFormat();
}

//----------------------------------------------------------------------------------------
int Cobalt_TextureBuffer2DArray_MipmapLevelCount(Cobalt_TextureBuffer2DArray texture)
{
	auto _this = reinterpret_cast<ITextureBuffer2DArray*>(texture);

	return _this->MipmapLevelCount();
}

//----------------------------------------------------------------------------------------
void Cobalt_TextureBuffer2DArray_MipmapLevelDimensions(Cobalt_TextureBuffer2DArray texture, int mipmapLevel, uint32_t dimensions[2])
{
	auto _this = reinterpret_cast<ITextureBuffer2DArray*>(texture);

	auto _dimensions = _this->MipmapLevelDimensions(mipmapLevel);
	dimensions[0] = _dimensions.X();
	dimensions[1] = _dimensions.Y();
}

//----------------------------------------------------------------------------------------
// Data methods
//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_TextureBuffer2DArray_SetInitialData(Cobalt_TextureBuffer2DArray texture, const void* sourceBuffer, size_t sourceBufferSizeInBytes, Cobalt_SourceImageFormat imageFormat, Cobalt_SourceDataFormat dataFormat, size_t arrayIndex, int mipmapLevel)
{
	auto _this = reinterpret_cast<ITextureBuffer2DArray*>(texture);

	return _this->SetInitialData(sourceBuffer, sourceBufferSizeInBytes, (ITextureBuffer::SourceImageFormat)imageFormat, (ITextureBuffer::SourceDataFormat)dataFormat, arrayIndex, mipmapLevel) ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_TextureBuffer2DArray_QueueDataUpdate(Cobalt_TextureBuffer2DArray texture, const void* sourceBuffer, size_t sourceBufferSizeInBytes, Cobalt_SourceImageFormat imageFormat, Cobalt_SourceDataFormat dataFormat, size_t arrayIndex, int mipmapLevel, const uint32_t imageOffsetInPixels[2], const uint32_t imageRegionInPixels[2], Cobalt_TransferBatch transferBatch)
{
	auto _this = reinterpret_cast<ITextureBuffer2DArray*>(texture);

	return _this->QueueDataUpdate(sourceBuffer, sourceBufferSizeInBytes, (ITextureBuffer::SourceImageFormat)imageFormat, (ITextureBuffer::SourceDataFormat)dataFormat, arrayIndex, mipmapLevel, V2UInt32(imageOffsetInPixels[0], imageOffsetInPixels[1]), V2UInt32(imageRegionInPixels[0], imageRegionInPixels[1]), reinterpret_cast<ITransferBatch*>(transferBatch)) ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
void Cobalt_TextureBuffer2DArray_Delete(Cobalt_TextureBuffer2DArray texture)
{
	auto _this = reinterpret_cast<ITextureBuffer2DArray*>(texture);

	_this->Delete();
}
