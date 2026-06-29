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

typedef struct Cobalt_VertexAttribute_Internal* Cobalt_VertexAttribute;

// Enumerations
typedef enum Cobalt_VertexAttributeType
{
	Cobalt_VertexAttributeType_Int8,
	Cobalt_VertexAttributeType_Int16,
	Cobalt_VertexAttributeType_Int32,
	Cobalt_VertexAttributeType_UInt8,
	Cobalt_VertexAttributeType_UInt16,
	Cobalt_VertexAttributeType_UInt32,
	Cobalt_VertexAttributeType_Norm8,
	Cobalt_VertexAttributeType_Norm16,
	Cobalt_VertexAttributeType_UNorm8,
	Cobalt_VertexAttributeType_UNorm16,
	Cobalt_VertexAttributeType_Float16,
	Cobalt_VertexAttributeType_Float32,
	Cobalt_VertexAttributeType_A2B10G10R10UNorm,
} Cobalt_VertexAttributeType;

typedef enum Cobalt_VertexPerformanceHint
{
	Cobalt_VertexPerformanceHint_Default = 0x00000000,
	Cobalt_VertexPerformanceHint_ReadNever = 0x00000001,
	Cobalt_VertexPerformanceHint_ReadRarely = 0x00000002,
	Cobalt_VertexPerformanceHint_ReadOften = 0x00000004,
	Cobalt_VertexPerformanceHint_ReadFlagsMask = 0x000000FF,
	Cobalt_VertexPerformanceHint_WriteNever = 0x00000100,
	Cobalt_VertexPerformanceHint_WriteRarely = 0x00000200,
	Cobalt_VertexPerformanceHint_WriteOften = 0x00000400,
	Cobalt_VertexPerformanceHint_WriteFlagsMask = 0x0000FF00,
} Cobalt_VertexPerformanceHint;

typedef enum Cobalt_VertexDataPersistenceFlags
{
	Cobalt_VertexDataPersistenceFlags_PersistAlways = 0x00000000,
	Cobalt_VertexDataPersistenceFlags_InvalidateExistingDataOnWrite = 0x000000001,
	Cobalt_VertexDataPersistenceFlags_InvalidateExistingDataAfterDrawComplete = 0x000000002,
} Cobalt_VertexDataPersistenceFlags;

// Type methods
COBALT_FUNCTION_EXPORT Cobalt_VertexAttributeType Cobalt_VertexAttribute_GetDataType(Cobalt_VertexAttribute attribute);
COBALT_FUNCTION_EXPORT size_t Cobalt_VertexAttribute_GetVertexCount(Cobalt_VertexAttribute attribute);
COBALT_FUNCTION_EXPORT size_t Cobalt_VertexAttribute_GetAttributeElementCount(Cobalt_VertexAttribute attribute);

// Usage methods
COBALT_FUNCTION_EXPORT Cobalt_VertexPerformanceHint Cobalt_VertexAttribute_GetPerformanceHintCpu(Cobalt_VertexAttribute attribute);
COBALT_FUNCTION_EXPORT Cobalt_VertexPerformanceHint Cobalt_VertexAttribute_GetPerformanceHintGpu(Cobalt_VertexAttribute attribute);
COBALT_FUNCTION_EXPORT Cobalt_VertexDataPersistenceFlags Cobalt_VertexAttribute_GetDataPersistenceFlags(Cobalt_VertexAttribute attribute);

// Binding methods
COBALT_FUNCTION_EXPORT char Cobalt_VertexAttribute_IsBoundToBuffer(Cobalt_VertexAttribute attribute);

// Data methods
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_VertexAttribute_SetInitialData(Cobalt_VertexAttribute attribute, const uint8_t* data, size_t entryCount, size_t entryStrideInBytes);
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_VertexAttribute_QueueDataUpdate(Cobalt_VertexAttribute attribute, const uint8_t* data, size_t entryCount, size_t initialVertexNo, size_t entryStrideInBytes, Cobalt_TransferBatch transferBatch);

COBALT_FUNCTION_EXPORT void Cobalt_VertexAttribute_Delete(Cobalt_VertexAttribute attribute);

#ifdef __cplusplus
}
#endif
