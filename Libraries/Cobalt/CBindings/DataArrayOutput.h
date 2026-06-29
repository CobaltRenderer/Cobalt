// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "DataArray.h"
#include "Macros.h"
#include "Result.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Cobalt_DataArrayOutput_Internal* Cobalt_DataArrayOutput;

// Configuration methods
COBALT_FUNCTION_EXPORT void Cobalt_DataArrayOutput_SetDetachAfterCapture(Cobalt_DataArrayOutput output, char state);
COBALT_FUNCTION_EXPORT void Cobalt_DataArrayOutput_SetArrayCaptureRegion(Cobalt_DataArrayOutput output, size_t captureEntryCount, size_t bufferOffset, char captureCounterValue);

// Data methods
COBALT_FUNCTION_EXPORT char Cobalt_DataArrayOutput_HasCapturedOutput(Cobalt_DataArrayOutput output);
COBALT_FUNCTION_EXPORT char Cobalt_DataArrayOutput_HasCapturedCounterValue(Cobalt_DataArrayOutput output);
COBALT_FUNCTION_EXPORT void Cobalt_DataArrayOutput_ClearCapturedOutput(Cobalt_DataArrayOutput output);
COBALT_FUNCTION_EXPORT size_t Cobalt_DataArrayOutput_GetEntryCount(Cobalt_DataArrayOutput output);
COBALT_FUNCTION_EXPORT size_t Cobalt_DataArrayOutput_GetEntrySizeInBytes(Cobalt_DataArrayOutput output);
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_DataArrayOutput_ReadBufferData(Cobalt_DataArrayOutput output, void* targetBuffer, size_t targetBufferSizeInBytes);
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_DataArrayOutput_ReadCounterValue(Cobalt_DataArrayOutput output, uint32_t* counterValue);

COBALT_FUNCTION_EXPORT void Cobalt_DataArrayOutput_Delete(Cobalt_DataArrayOutput output);

#ifdef __cplusplus
}
#endif
