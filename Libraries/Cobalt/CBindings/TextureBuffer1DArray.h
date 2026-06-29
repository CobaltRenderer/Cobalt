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

typedef struct Cobalt_TextureBuffer1DArray_Internal* Cobalt_TextureBuffer1DArray;

// Initialization methods
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_TextureBuffer1DArray_AllocateMemory(Cobalt_TextureBuffer1DArray texture);

// Format methods
COBALT_FUNCTION_EXPORT void Cobalt_TextureBuffer1DArray_SetTextureFormat(Cobalt_TextureBuffer1DArray texture, Cobalt_ImageFormat imageFormat, Cobalt_DataFormat dataFormat);
COBALT_FUNCTION_EXPORT void Cobalt_TextureBuffer1DArray_SetTextureDimensions(Cobalt_TextureBuffer1DArray texture, uint32_t imageDimensions, size_t arraySize, int mipmapLevelCount);
COBALT_FUNCTION_EXPORT Cobalt_ImageFormat Cobalt_TextureBuffer1DArray_AllocatedImageFormat(Cobalt_TextureBuffer1DArray texture);
COBALT_FUNCTION_EXPORT Cobalt_DataFormat Cobalt_TextureBuffer1DArray_AllocatedDataFormat(Cobalt_TextureBuffer1DArray texture);
int Cobalt_TextureBuffer1DArray_MipmapLevelCount(Cobalt_TextureBuffer1DArray texture);
COBALT_FUNCTION_EXPORT void Cobalt_TextureBuffer1DArray_MipmapLevelDimensions(Cobalt_TextureBuffer1DArray texture, int mipmapLevel, uint32_t* dimensions);

// Data methods
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_TextureBuffer1DArray_SetInitialData(Cobalt_TextureBuffer1DArray texture, const void* sourceBuffer, size_t sourceBufferSizeInBytes, Cobalt_SourceImageFormat imageFormat, Cobalt_SourceDataFormat dataFormat, size_t arrayIndex, int mipmapLevel);
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_TextureBuffer1DArray_QueueDataUpdate(Cobalt_TextureBuffer1DArray texture, const void* sourceBuffer, size_t sourceBufferSizeInBytes, Cobalt_SourceImageFormat imageFormat, Cobalt_SourceDataFormat dataFormat, size_t arrayIndex, int mipmapLevel, uint32_t imageOffsetInPixels, uint32_t imageRegionInPixels, Cobalt_TransferBatch transferBatch);

COBALT_FUNCTION_EXPORT void Cobalt_TextureBuffer1DArray_Delete(Cobalt_TextureBuffer1DArray texture);

#ifdef __cplusplus
}
#endif
