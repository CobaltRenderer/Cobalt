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

typedef struct Cobalt_TextureBuffer1D_Internal* Cobalt_TextureBuffer1D;

// Initialization methods
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_TextureBuffer1D_AllocateMemory(Cobalt_TextureBuffer1D texture);

// Format methods
COBALT_FUNCTION_EXPORT void Cobalt_TextureBuffer1D_SetTextureFormat(Cobalt_TextureBuffer1D texture, Cobalt_ImageFormat imageFormat, Cobalt_DataFormat dataFormat);
COBALT_FUNCTION_EXPORT void Cobalt_TextureBuffer1D_SetTextureDimensions(Cobalt_TextureBuffer1D texture, uint32_t imageDimensions, int mipmapLevelCount);
COBALT_FUNCTION_EXPORT Cobalt_ImageFormat Cobalt_TextureBuffer1D_AllocatedImageFormat(Cobalt_TextureBuffer1D texture);
COBALT_FUNCTION_EXPORT Cobalt_DataFormat Cobalt_TextureBuffer1D_AllocatedDataFormat(Cobalt_TextureBuffer1D texture);
int Cobalt_TextureBuffer1D_MipmapLevelCount(Cobalt_TextureBuffer1D texture);
COBALT_FUNCTION_EXPORT void Cobalt_TextureBuffer1D_MipmapLevelDimensions(Cobalt_TextureBuffer1D texture, int mipmapLevel, uint32_t* dimensions);

// Data methods
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_TextureBuffer1D_SetInitialData(Cobalt_TextureBuffer1D texture, const void* sourceBuffer, size_t sourceBufferSizeInBytes, Cobalt_SourceImageFormat imageFormat, Cobalt_SourceDataFormat dataFormat, int mipmapLevel);
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_TextureBuffer1D_QueueDataUpdate(Cobalt_TextureBuffer1D texture, const void* sourceBuffer, size_t sourceBufferSizeInBytes, Cobalt_SourceImageFormat imageFormat, Cobalt_SourceDataFormat dataFormat, int mipmapLevel, uint32_t imageOffsetInPixels, uint32_t imageRegionInPixels, Cobalt_TransferBatch transferBatch);

COBALT_FUNCTION_EXPORT void Cobalt_TextureBuffer1D_Delete(Cobalt_TextureBuffer1D texture);

#ifdef __cplusplus
}
#endif
