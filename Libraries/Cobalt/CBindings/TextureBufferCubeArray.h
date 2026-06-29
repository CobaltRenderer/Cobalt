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

typedef struct Cobalt_TextureBufferCubeArray_Internal* Cobalt_TextureBufferCubeArray;

// Initialization methods
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_TextureBufferCubeArray_AllocateMemory(Cobalt_TextureBufferCubeArray texture);

// Format methods
COBALT_FUNCTION_EXPORT void Cobalt_TextureBufferCubeArray_SetTextureFormat(Cobalt_TextureBufferCubeArray texture, Cobalt_ImageFormat imageFormat, Cobalt_DataFormat dataFormat);
COBALT_FUNCTION_EXPORT void Cobalt_TextureBufferCubeArray_SetTextureDimensions(Cobalt_TextureBufferCubeArray texture, uint32_t faceLength, size_t arraySize, int mipmapLevelCount);
COBALT_FUNCTION_EXPORT Cobalt_ImageFormat Cobalt_TextureBufferCubeArray_AllocatedImageFormat(Cobalt_TextureBufferCubeArray texture);
COBALT_FUNCTION_EXPORT Cobalt_DataFormat Cobalt_TextureBufferCubeArray_AllocatedDataFormat(Cobalt_TextureBufferCubeArray texture);
int Cobalt_TextureBufferCubeArray_MipmapLevelCount(Cobalt_TextureBufferCubeArray texture);
COBALT_FUNCTION_EXPORT void Cobalt_TextureBufferCubeArray_MipmapLevelDimensions(Cobalt_TextureBufferCubeArray texture, int mipmapLevel, uint32_t dimensions[2]);

// Data methods
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_TextureBufferCubeArray_SetInitialData(Cobalt_TextureBufferCubeArray texture, const void* sourceBuffer, size_t sourceBufferSizeInBytes, Cobalt_SourceImageFormat imageFormat, Cobalt_SourceDataFormat dataFormat, Cobalt_CubeMapFace targetFace, size_t arrayIndex, int mipmapLevel);
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_TextureBufferCubeArray_QueueDataUpdate(Cobalt_TextureBufferCubeArray texture, const void* sourceBuffer, size_t sourceBufferSizeInBytes, Cobalt_SourceImageFormat imageFormat, Cobalt_SourceDataFormat dataFormat, Cobalt_CubeMapFace targetFace, size_t arrayIndex, int mipmapLevel, const uint32_t imageOffsetInPixels[2], const uint32_t imageRegionInPixels[2], Cobalt_TransferBatch transferBatch);

COBALT_FUNCTION_EXPORT void Cobalt_TextureBufferCubeArray_Delete(Cobalt_TextureBufferCubeArray texture);

#ifdef __cplusplus
}
#endif
