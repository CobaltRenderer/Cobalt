// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "TextureBuffer1DArray.h"
#include <Cobalt/RendererInterface/RendererInterface.pkg>

using namespace cobalt::graphics;

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_TextureBuffer1DArray_AllocateMemory(Cobalt_TextureBuffer1DArray texture)
{
	auto _this = reinterpret_cast<ITextureBuffer1DArray*>(texture);

	return _this->AllocateMemory() ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
// Format methods
//----------------------------------------------------------------------------------------
void Cobalt_TextureBuffer1DArray_SetTextureFormat(Cobalt_TextureBuffer1DArray texture, Cobalt_ImageFormat imageFormat, Cobalt_DataFormat dataFormat)
{
	auto _this = reinterpret_cast<ITextureBuffer1DArray*>(texture);

	_this->SetTextureFormat((ITextureBuffer::ImageFormat)imageFormat, (ITextureBuffer::DataFormat)dataFormat);
}

//----------------------------------------------------------------------------------------
void Cobalt_TextureBuffer1DArray_SetTextureDimensions(Cobalt_TextureBuffer1DArray texture, uint32_t imageDimensions, size_t arraySize, int mipmapLevelCount)
{
	auto _this = reinterpret_cast<ITextureBuffer1DArray*>(texture);

	_this->SetTextureDimensions(V1UInt32(imageDimensions), arraySize, mipmapLevelCount);
}

//----------------------------------------------------------------------------------------
Cobalt_ImageFormat Cobalt_TextureBuffer1DArray_AllocatedImageFormat(Cobalt_TextureBuffer1DArray texture)
{
	auto _this = reinterpret_cast<ITextureBuffer1DArray*>(texture);

	return (Cobalt_ImageFormat)_this->AllocatedImageFormat();
}

//----------------------------------------------------------------------------------------
Cobalt_DataFormat Cobalt_TextureBuffer1DArray_AllocatedDataFormat(Cobalt_TextureBuffer1DArray texture)
{
	auto _this = reinterpret_cast<ITextureBuffer1DArray*>(texture);

	return (Cobalt_DataFormat)_this->AllocatedDataFormat();
}

//----------------------------------------------------------------------------------------
int Cobalt_TextureBuffer1DArray_MipmapLevelCount(Cobalt_TextureBuffer1DArray texture)
{
	auto _this = reinterpret_cast<ITextureBuffer1DArray*>(texture);

	return _this->MipmapLevelCount();
}

//----------------------------------------------------------------------------------------
void Cobalt_TextureBuffer1DArray_MipmapLevelDimensions(Cobalt_TextureBuffer1DArray texture, int mipmapLevel, uint32_t* dimensions)
{
	auto _this = reinterpret_cast<ITextureBuffer1DArray*>(texture);

	*dimensions = _this->MipmapLevelDimensions(mipmapLevel).X();
}

//----------------------------------------------------------------------------------------
// Data methods
//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_TextureBuffer1DArray_SetInitialData(Cobalt_TextureBuffer1DArray texture, const void* sourceBuffer, size_t sourceBufferSizeInBytes, Cobalt_SourceImageFormat imageFormat, Cobalt_SourceDataFormat dataFormat, size_t arrayIndex, int mipmapLevel)
{
	auto _this = reinterpret_cast<ITextureBuffer1DArray*>(texture);

	return _this->SetInitialData(sourceBuffer, sourceBufferSizeInBytes, (ITextureBuffer::SourceImageFormat)imageFormat, (ITextureBuffer::SourceDataFormat)dataFormat, arrayIndex, mipmapLevel) ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_TextureBuffer1DArray_QueueDataUpdate(Cobalt_TextureBuffer1DArray texture, const void* sourceBuffer, size_t sourceBufferSizeInBytes, Cobalt_SourceImageFormat imageFormat, Cobalt_SourceDataFormat dataFormat, size_t arrayIndex, int mipmapLevel, uint32_t imageOffsetInPixels, uint32_t imageRegionInPixels, Cobalt_TransferBatch transferBatch)
{
	auto _this = reinterpret_cast<ITextureBuffer1DArray*>(texture);

	return _this->QueueDataUpdate(sourceBuffer, sourceBufferSizeInBytes, (ITextureBuffer::SourceImageFormat)imageFormat, (ITextureBuffer::SourceDataFormat)dataFormat, arrayIndex, mipmapLevel, V1UInt32(imageOffsetInPixels), V1UInt32(imageRegionInPixels), reinterpret_cast<ITransferBatch*>(transferBatch)) ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
void Cobalt_TextureBuffer1DArray_Delete(Cobalt_TextureBuffer1DArray texture)
{
	auto _this = reinterpret_cast<ITextureBuffer1DArray*>(texture);

	_this->Delete();
}
