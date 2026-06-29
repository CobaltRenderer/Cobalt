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

typedef struct Cobalt_TextureBuffer3D_Internal* Cobalt_TextureBuffer3D;

// Initialization methods
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_TextureBuffer3D_AllocateMemory(Cobalt_TextureBuffer3D texture);

// Format methods
COBALT_FUNCTION_EXPORT void Cobalt_TextureBuffer3D_SetTextureFormat(Cobalt_TextureBuffer3D texture, Cobalt_ImageFormat imageFormat, Cobalt_DataFormat dataFormat);
COBALT_FUNCTION_EXPORT void Cobalt_TextureBuffer3D_SetTextureDimensions(Cobalt_TextureBuffer3D texture, const uint32_t imageDimensions[3], int mipmapLevelCount);
COBALT_FUNCTION_EXPORT Cobalt_ImageFormat Cobalt_TextureBuffer3D_AllocatedImageFormat(Cobalt_TextureBuffer3D texture);
COBALT_FUNCTION_EXPORT Cobalt_DataFormat Cobalt_TextureBuffer3D_AllocatedDataFormat(Cobalt_TextureBuffer3D texture);
int Cobalt_TextureBuffer3D_MipmapLevelCount(Cobalt_TextureBuffer3D texture);
COBALT_FUNCTION_EXPORT void Cobalt_TextureBuffer3D_MipmapLevelDimensions(Cobalt_TextureBuffer3D texture, int mipmapLevel, uint32_t dimensions[3]);

// Data methods
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_TextureBuffer3D_SetInitialData(Cobalt_TextureBuffer3D texture, const void* sourceBuffer, size_t sourceBufferSizeInBytes, Cobalt_SourceImageFormat imageFormat, Cobalt_SourceDataFormat dataFormat, int mipmapLevel);
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_TextureBuffer3D_QueueDataUpdate(Cobalt_TextureBuffer3D texture, const void* sourceBuffer, size_t sourceBufferSizeInBytes, Cobalt_SourceImageFormat imageFormat, Cobalt_SourceDataFormat dataFormat, int mipmapLevel, const uint32_t imageOffsetInPixels[3], const uint32_t imageRegionInPixels[3], Cobalt_TransferBatch transferBatch);

COBALT_FUNCTION_EXPORT void Cobalt_TextureBuffer3D_Delete(Cobalt_TextureBuffer3D texture);

#ifdef __cplusplus
}
#endif
