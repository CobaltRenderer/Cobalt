// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Macros.h"
#include "Result.h"
#include "TextureBuffer.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Cobalt_FrameBufferOutput_Internal* Cobalt_FrameBufferOutput;

// Configuration methods
COBALT_FUNCTION_EXPORT void Cobalt_FrameBufferOutput_SetDetachAfterCapture(Cobalt_FrameBufferOutput output, char state);
COBALT_FUNCTION_EXPORT void Cobalt_FrameBufferOutput_SetFrameBufferCaptureRegion(Cobalt_FrameBufferOutput output, const uint32_t imageOffsetInPixels[2], const uint32_t imageRegionInPixels[2]);

// Data methods
COBALT_FUNCTION_EXPORT char Cobalt_FrameBufferOutput_HasCapturedOutput(Cobalt_FrameBufferOutput output);
COBALT_FUNCTION_EXPORT void Cobalt_FrameBufferOutput_ClearCapturedOutput(Cobalt_FrameBufferOutput output);
COBALT_FUNCTION_EXPORT void Cobalt_FrameBufferOutput_GetImageDimensions(Cobalt_FrameBufferOutput output, uint32_t dimensions[2]);
COBALT_FUNCTION_EXPORT void Cobalt_FrameBufferOutput_GetCroppedImageDimensions(Cobalt_FrameBufferOutput output, const uint32_t imageOffsetInPixels[2], const uint32_t imageRegionInPixels[2], uint32_t dimensions[2]);
COBALT_FUNCTION_EXPORT Cobalt_SourceImageFormat Cobalt_FrameBufferOutput_GetOptimalImageFormat(Cobalt_FrameBufferOutput output);
COBALT_FUNCTION_EXPORT Cobalt_SourceDataFormat Cobalt_FrameBufferOutput_GetOptimalDataFormat(Cobalt_FrameBufferOutput output);
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_FrameBufferOutput_ReadBufferData(Cobalt_FrameBufferOutput output, void* targetBuffer, size_t targetBufferSizeInBytes, Cobalt_SourceImageFormat imageFormat, Cobalt_SourceDataFormat dataFormat, const uint32_t imageOffsetInPixels[2], const uint32_t imageRegionInPixels[2]);

COBALT_FUNCTION_EXPORT void Cobalt_FrameBufferOutput_Delete(Cobalt_FrameBufferOutput output);

#ifdef __cplusplus
}
#endif
