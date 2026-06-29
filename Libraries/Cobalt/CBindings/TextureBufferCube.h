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

typedef struct Cobalt_TextureBufferCube_Internal* Cobalt_TextureBufferCube;

// Initialization methods
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_TextureBufferCube_AllocateMemory(Cobalt_TextureBufferCube texture);

// Format methods
COBALT_FUNCTION_EXPORT void Cobalt_TextureBufferCube_SetTextureFormat(Cobalt_TextureBufferCube texture, Cobalt_ImageFormat imageFormat, Cobalt_DataFormat dataFormat);
COBALT_FUNCTION_EXPORT void Cobalt_TextureBufferCube_SetTextureDimensions(Cobalt_TextureBufferCube texture, uint32_t faceLength, int mipmapLevelCount);
COBALT_FUNCTION_EXPORT Cobalt_ImageFormat Cobalt_TextureBufferCube_AllocatedImageFormat(Cobalt_TextureBufferCube texture);
COBALT_FUNCTION_EXPORT Cobalt_DataFormat Cobalt_TextureBufferCube_AllocatedDataFormat(Cobalt_TextureBufferCube texture);
int Cobalt_TextureBufferCube_MipmapLevelCount(Cobalt_TextureBufferCube texture);
COBALT_FUNCTION_EXPORT void Cobalt_TextureBufferCube_MipmapLevelDimensions(Cobalt_TextureBufferCube texture, int mipmapLevel, uint32_t dimensions[2]);

// Data methods
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_TextureBufferCube_SetInitialData(Cobalt_TextureBufferCube texture, const void* sourceBuffer, size_t sourceBufferSizeInBytes, Cobalt_SourceImageFormat imageFormat, Cobalt_SourceDataFormat dataFormat, Cobalt_CubeMapFace targetFace, int mipmapLevel);
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_TextureBufferCube_QueueDataUpdate(Cobalt_TextureBufferCube texture, const void* sourceBuffer, size_t sourceBufferSizeInBytes, Cobalt_SourceImageFormat imageFormat, Cobalt_SourceDataFormat dataFormat, Cobalt_CubeMapFace targetFace, int mipmapLevel, const uint32_t imageOffsetInPixels[2], const uint32_t imageRegionInPixels[2], Cobalt_TransferBatch transferBatch);

COBALT_FUNCTION_EXPORT void Cobalt_TextureBufferCube_Delete(Cobalt_TextureBufferCube texture);

#ifdef __cplusplus
}
#endif
