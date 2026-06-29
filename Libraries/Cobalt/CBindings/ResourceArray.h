// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Macros.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Cobalt_ResourceArray_Internal* Cobalt_ResourceArray;

// Enumerations
typedef enum
{
	Cobalt_ResourceArrayPerformanceHint_Default = 0x00000000,
	Cobalt_ResourceArrayPerformanceHint_ReadNever = 0x00000001,
	Cobalt_ResourceArrayPerformanceHint_ReadRarely = 0x00000002,
	Cobalt_ResourceArrayPerformanceHint_ReadOften = 0x00000004,
	Cobalt_ResourceArrayPerformanceHint_ReadFlagsMask = 0x000000FF,
	Cobalt_ResourceArrayPerformanceHint_WriteNever = 0x00000100,
	Cobalt_ResourceArrayPerformanceHint_WriteRarely = 0x00000200,
	Cobalt_ResourceArrayPerformanceHint_WriteOften = 0x00000400,
	Cobalt_ResourceArrayPerformanceHint_WriteFlagsMask = 0x0000FF00,
} Cobalt_ResourceArrayPerformanceHint;

typedef enum
{
	Cobalt_ResourceArrayDataPersistenceFlags_PersistAlways = 0x00000000,
	Cobalt_ResourceArrayDataPersistenceFlags_InvalidateExistingDataOnWrite = 0x000000001,
	Cobalt_ResourceArrayDataPersistenceFlags_InvalidateExistingDataAfterDrawComplete = 0x000000002,
} Cobalt_ResourceArrayDataPersistenceFlags;

// Usage methods
COBALT_FUNCTION_EXPORT void Cobalt_ResourceArray_SetPerformanceHints(Cobalt_ResourceArray resourceArray, Cobalt_ResourceArrayPerformanceHint performanceHintCpu, Cobalt_ResourceArrayPerformanceHint performanceHintGpu);
COBALT_FUNCTION_EXPORT void Cobalt_ResourceArray_SetDataPersistenceFlags(Cobalt_ResourceArray resourceArray, Cobalt_ResourceArrayDataPersistenceFlags dataPersistenceFlags);

#ifdef __cplusplus
}
#endif
