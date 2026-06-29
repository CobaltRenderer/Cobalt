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

typedef struct Cobalt_TextureBuffer2DArray_Internal* Cobalt_TextureBuffer2DArray;

// Initialization methods
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_TextureBuffer2DArray_AllocateMemory(Cobalt_TextureBuffer2DArray texture);

// Format methods
COBALT_FUNCTION_EXPORT void Cobalt_TextureBuffer2DArray_SetTextureFormat(Cobalt_TextureBuffer2DArray texture, Cobalt_ImageFormat imageFormat, Cobalt_DataFormat dataFormat);
COBALT_FUNCTION_EXPORT void Cobalt_TextureBuffer2DArray_SetTextureDimensions(Cobalt_TextureBuffer2DArray texture, const uint32_t imageDimensions[2], size_t arraySize, int mipmapLevelCount);
COBALT_FUNCTION_EXPORT Cobalt_ImageFormat Cobalt_TextureBuffer2DArray_AllocatedImageFormat(Cobalt_TextureBuffer2DArray texture);
COBALT_FUNCTION_EXPORT Cobalt_DataFormat Cobalt_TextureBuffer2DArray_AllocatedDataFormat(Cobalt_TextureBuffer2DArray texture);
int Cobalt_TextureBuffer2DArray_MipmapLevelCount(Cobalt_TextureBuffer2DArray texture);
COBALT_FUNCTION_EXPORT void Cobalt_TextureBuffer2DArray_MipmapLevelDimensions(Cobalt_TextureBuffer2DArray texture, int mipmapLevel, uint32_t dimensions[2]);

// Data methods
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_TextureBuffer2DArray_SetInitialData(Cobalt_TextureBuffer2DArray texture, const void* sourceBuffer, size_t sourceBufferSizeInBytes, Cobalt_SourceImageFormat imageFormat, Cobalt_SourceDataFormat dataFormat, size_t arrayIndex, int mipmapLevel);
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_TextureBuffer2DArray_QueueDataUpdate(Cobalt_TextureBuffer2DArray texture, const void* sourceBuffer, size_t sourceBufferSizeInBytes, Cobalt_SourceImageFormat imageFormat, Cobalt_SourceDataFormat dataFormat, size_t arrayIndex, int mipmapLevel, const uint32_t imageOffsetInPixels[2], const uint32_t imageRegionInPixels[2], Cobalt_TransferBatch transferBatch);

COBALT_FUNCTION_EXPORT void Cobalt_TextureBuffer2DArray_Delete(Cobalt_TextureBuffer2DArray texture);

#ifdef __cplusplus
}
#endif
