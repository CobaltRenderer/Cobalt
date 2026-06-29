// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Macros.h"
#include "Result.h"
#include "TexelArray.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Cobalt_TexelArrayOutput_Internal* Cobalt_TexelArrayOutput;

// Configuration methods
COBALT_FUNCTION_EXPORT void Cobalt_TexelArrayOutput_SetDetachAfterCapture(Cobalt_TexelArrayOutput output, char state);
COBALT_FUNCTION_EXPORT void Cobalt_TexelArrayOutput_SetArrayCaptureRegion(Cobalt_TexelArrayOutput output, size_t captureEntryCount, size_t bufferOffset);

// Data methods
COBALT_FUNCTION_EXPORT char Cobalt_TexelArrayOutput_HasCapturedOutput(Cobalt_TexelArrayOutput output);
COBALT_FUNCTION_EXPORT void Cobalt_TexelArrayOutput_ClearCapturedOutput(Cobalt_TexelArrayOutput output);
COBALT_FUNCTION_EXPORT size_t Cobalt_TexelArrayOutput_GetEntryCount(Cobalt_TexelArrayOutput output);
COBALT_FUNCTION_EXPORT Cobalt_TexelArraySourceImageFormat Cobalt_TexelArrayOutput_GetOptimalImageFormat(Cobalt_TexelArrayOutput output);
COBALT_FUNCTION_EXPORT Cobalt_TexelArraySourceDataFormat Cobalt_TexelArrayOutput_GetOptimalDataFormat(Cobalt_TexelArrayOutput output);

COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_TexelArrayOutput_ReadBufferData(Cobalt_TexelArrayOutput output, void* targetBuffer, size_t targetBufferSizeInBytes, Cobalt_TexelArraySourceImageFormat imageFormat, Cobalt_TexelArraySourceDataFormat dataFormat);

COBALT_FUNCTION_EXPORT void Cobalt_TexelArrayOutput_Delete(Cobalt_TexelArrayOutput output);

#ifdef __cplusplus
}
#endif
