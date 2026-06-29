// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Macros.h"
#include "Result.h"
#include "TransferBatch.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Cobalt_IndexAttribute_Internal* Cobalt_IndexAttribute;

// Enumerations
typedef enum Cobalt_IndexAttributeType
{
	Cobalt_IndexAttributeType_UInt16,
	Cobalt_IndexAttributeType_UInt32,
} Cobalt_IndexAttributeType;

typedef enum Cobalt_IndexPerformanceHint
{
	Cobalt_IndexPerformanceHint_Default = 0x00000000,
	Cobalt_IndexPerformanceHint_ReadNever = 0x00000001,
	Cobalt_IndexPerformanceHint_ReadRarely = 0x00000002,
	Cobalt_IndexPerformanceHint_ReadOften = 0x00000004,
	Cobalt_IndexPerformanceHint_ReadFlagsMask = 0x000000FF,
	Cobalt_IndexPerformanceHint_WriteNever = 0x00000100,
	Cobalt_IndexPerformanceHint_WriteRarely = 0x00000200,
	Cobalt_IndexPerformanceHint_WriteOften = 0x00000400,
	Cobalt_IndexPerformanceHint_WriteFlagsMask = 0x0000FF00,
} Cobalt_IndexPerformanceHint;

typedef enum Cobalt_IndexDataPersistenceFlags
{
	Cobalt_IndexDataPersistenceFlags_PersistAlways = 0x00000000,
	Cobalt_IndexDataPersistenceFlags_InvalidateExistingDataOnWrite = 0x000000001,
	Cobalt_IndexDataPersistenceFlags_InvalidateExistingDataAfterDrawComplete = 0x000000002,
} Cobalt_IndexDataPersistenceFlags;

// Type methods
COBALT_FUNCTION_EXPORT Cobalt_IndexAttributeType Cobalt_IndexAttribute_GetDataType(Cobalt_IndexAttribute attribute);
COBALT_FUNCTION_EXPORT size_t Cobalt_IndexAttribute_GetIndexCount(Cobalt_IndexAttribute attribute);

// Usage methods
COBALT_FUNCTION_EXPORT Cobalt_IndexPerformanceHint Cobalt_IndexAttribute_GetPerformanceHintCpu(Cobalt_IndexAttribute attribute);
COBALT_FUNCTION_EXPORT Cobalt_IndexPerformanceHint Cobalt_IndexAttribute_GetPerformanceHintGpu(Cobalt_IndexAttribute attribute);
COBALT_FUNCTION_EXPORT Cobalt_IndexDataPersistenceFlags Cobalt_IndexAttribute_GetDataPersistenceFlags(Cobalt_IndexAttribute attribute);

// Binding methods
COBALT_FUNCTION_EXPORT char Cobalt_IndexAttribute_IsBoundToBuffer(Cobalt_IndexAttribute attribute);

// Data methods
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_IndexAttribute_SetInitialData(Cobalt_IndexAttribute attribute, const uint8_t* data, size_t entryCount, size_t entryStrideInBytes);
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_IndexAttribute_QueueDataUpdate(Cobalt_IndexAttribute attribute, const uint8_t* data, size_t entryCount, size_t initialIndexNo, size_t entryStrideInBytes, Cobalt_TransferBatch transferBatch);

COBALT_FUNCTION_EXPORT void Cobalt_IndexAttribute_Delete(Cobalt_IndexAttribute attribute);

#ifdef __cplusplus
}
#endif
