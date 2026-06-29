// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "TextureBuffer2D.h"
#include <Cobalt/RendererInterface/RendererInterface.pkg>

using namespace cobalt::graphics;

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_TextureBuffer2D_AllocateMemory(Cobalt_TextureBuffer2D texture)
{
	auto _this = reinterpret_cast<ITextureBuffer2D*>(texture);

	return _this->AllocateMemory() ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
// Format methods
//----------------------------------------------------------------------------------------
void Cobalt_TextureBuffer2D_SetTextureFormat(Cobalt_TextureBuffer2D texture, Cobalt_ImageFormat imageFormat, Cobalt_DataFormat dataFormat)
{
	auto _this = reinterpret_cast<ITextureBuffer2D*>(texture);

	_this->SetTextureFormat((ITextureBuffer::ImageFormat)imageFormat, (ITextureBuffer::DataFormat)dataFormat);
}

//----------------------------------------------------------------------------------------
void Cobalt_TextureBuffer2D_SetTextureDimensions(Cobalt_TextureBuffer2D texture, const uint32_t imageDimensions[2], int mipmapLevelCount)
{
	auto _this = reinterpret_cast<ITextureBuffer2D*>(texture);

	_this->SetTextureDimensions(V2UInt32(imageDimensions[0], imageDimensions[1]), mipmapLevelCount);
}

//----------------------------------------------------------------------------------------
char Cobalt_TextureBuffer2D_IsSampleCountSupported(Cobalt_TextureBuffer2D texture, Cobalt_ImageFormat imageFormat, Cobalt_DataFormat dataFormat, Cobalt_SampleCount sampleCount)
{
	auto _this = reinterpret_cast<ITextureBuffer2D*>(texture);

	return _this->IsSampleCountSupported((ITextureBuffer::ImageFormat)imageFormat, (ITextureBuffer::DataFormat)dataFormat, (ITextureBuffer::SampleCount)sampleCount) ? 1 : 0;
}

//----------------------------------------------------------------------------------------
void Cobalt_TextureBuffer2D_SetSampleCount(Cobalt_TextureBuffer2D texture, Cobalt_SampleCount sampleCount)
{
	auto _this = reinterpret_cast<ITextureBuffer2D*>(texture);

	_this->SetSampleCount((ITextureBuffer::SampleCount)sampleCount);
}

//----------------------------------------------------------------------------------------
Cobalt_ImageFormat Cobalt_TextureBuffer2D_AllocatedImageFormat(Cobalt_TextureBuffer2D texture)
{
	auto _this = reinterpret_cast<ITextureBuffer2D*>(texture);

	return (Cobalt_ImageFormat)_this->AllocatedImageFormat();
}

//----------------------------------------------------------------------------------------
Cobalt_DataFormat Cobalt_TextureBuffer2D_AllocatedDataFormat(Cobalt_TextureBuffer2D texture)
{
	auto _this = reinterpret_cast<ITextureBuffer2D*>(texture);

	return (Cobalt_DataFormat)_this->AllocatedDataFormat();
}

//----------------------------------------------------------------------------------------
int Cobalt_TextureBuffer2D_MipmapLevelCount(Cobalt_TextureBuffer2D texture)
{
	auto _this = reinterpret_cast<ITextureBuffer2D*>(texture);

	return _this->MipmapLevelCount();
}

//----------------------------------------------------------------------------------------
void Cobalt_TextureBuffer2D_MipmapLevelDimensions(Cobalt_TextureBuffer2D texture, int mipmapLevel, uint32_t dimensions[2])
{
	auto _this = reinterpret_cast<ITextureBuffer2D*>(texture);

	auto _dimensions = _this->MipmapLevelDimensions(mipmapLevel);
	dimensions[0] = _dimensions.X();
	dimensions[1] = _dimensions.Y();
}

//----------------------------------------------------------------------------------------
// Data methods
//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_TextureBuffer2D_SetInitialData(Cobalt_TextureBuffer2D texture, const void* sourceBuffer, size_t sourceBufferSizeInBytes, Cobalt_SourceImageFormat imageFormat, Cobalt_SourceDataFormat dataFormat, int mipmapLevel)
{
	auto _this = reinterpret_cast<ITextureBuffer2D*>(texture);

	return _this->SetInitialData(sourceBuffer, sourceBufferSizeInBytes, (ITextureBuffer::SourceImageFormat)imageFormat, (ITextureBuffer::SourceDataFormat)dataFormat, mipmapLevel) ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_TextureBuffer2D_QueueDataUpdate(Cobalt_TextureBuffer2D texture, const void* sourceBuffer, size_t sourceBufferSizeInBytes, Cobalt_SourceImageFormat imageFormat, Cobalt_SourceDataFormat dataFormat, int mipmapLevel, const uint32_t imageOffsetInPixels[2], const uint32_t imageRegionInPixels[2], Cobalt_TransferBatch transferBatch)
{
	auto _this = reinterpret_cast<ITextureBuffer2D*>(texture);

	return _this->QueueDataUpdate(sourceBuffer, sourceBufferSizeInBytes, (ITextureBuffer::SourceImageFormat)imageFormat, (ITextureBuffer::SourceDataFormat)dataFormat, mipmapLevel, V2UInt32(imageOffsetInPixels[0], imageOffsetInPixels[1]), V2UInt32(imageRegionInPixels[0], imageRegionInPixels[1]), reinterpret_cast<ITransferBatch*>(transferBatch)) ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
void Cobalt_TextureBuffer2D_Delete(Cobalt_TextureBuffer2D texture)
{
	auto _this = reinterpret_cast<ITextureBuffer2D*>(texture);

	_this->Delete();
}
