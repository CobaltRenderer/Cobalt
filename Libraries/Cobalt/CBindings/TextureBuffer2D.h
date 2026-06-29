// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Macros.h"
#include "TextureBuffer.h"
#include "TransferBatch.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Cobalt_TextureBuffer2D_Internal* Cobalt_TextureBuffer2D;

// Initialization methods
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_TextureBuffer2D_AllocateMemory(Cobalt_TextureBuffer2D texture);

// Format methods
COBALT_FUNCTION_EXPORT void Cobalt_TextureBuffer2D_SetTextureFormat(Cobalt_TextureBuffer2D texture, Cobalt_ImageFormat imageFormat, Cobalt_DataFormat dataFormat);
COBALT_FUNCTION_EXPORT void Cobalt_TextureBuffer2D_SetTextureDimensions(Cobalt_TextureBuffer2D texture, const uint32_t imageDimensions[2], int mipmapLevelCount);
COBALT_FUNCTION_EXPORT char Cobalt_TextureBuffer2D_IsSampleCountSupported(Cobalt_TextureBuffer2D texture, Cobalt_ImageFormat imageFormat, Cobalt_DataFormat dataFormat, Cobalt_SampleCount sampleCount);
COBALT_FUNCTION_EXPORT void Cobalt_TextureBuffer2D_SetSampleCount(Cobalt_TextureBuffer2D texture, Cobalt_SampleCount sampleCount);
COBALT_FUNCTION_EXPORT Cobalt_ImageFormat Cobalt_TextureBuffer2D_AllocatedImageFormat(Cobalt_TextureBuffer2D texture);
COBALT_FUNCTION_EXPORT Cobalt_DataFormat Cobalt_TextureBuffer2D_AllocatedDataFormat(Cobalt_TextureBuffer2D texture);
int Cobalt_TextureBuffer2D_MipmapLevelCount(Cobalt_TextureBuffer2D texture);
COBALT_FUNCTION_EXPORT void Cobalt_TextureBuffer2D_MipmapLevelDimensions(Cobalt_TextureBuffer2D texture, int mipmapLevel, uint32_t dimensions[2]);

// Data methods
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_TextureBuffer2D_SetInitialData(Cobalt_TextureBuffer2D texture, const void* sourceBuffer, size_t sourceBufferSizeInBytes, Cobalt_SourceImageFormat imageFormat, Cobalt_SourceDataFormat dataFormat, int mipmapLevel);
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_TextureBuffer2D_QueueDataUpdate(Cobalt_TextureBuffer2D texture, const void* sourceBuffer, size_t sourceBufferSizeInBytes, Cobalt_SourceImageFormat imageFormat, Cobalt_SourceDataFormat dataFormat, int mipmapLevel, const uint32_t imageOffsetInPixels[2], const uint32_t imageRegionInPixels[2], Cobalt_TransferBatch transferBatch);

COBALT_FUNCTION_EXPORT void Cobalt_TextureBuffer2D_Delete(Cobalt_TextureBuffer2D texture);

#ifdef __cplusplus
}
#endif
