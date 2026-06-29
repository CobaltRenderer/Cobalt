// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Macros.h"
#include "Result.h"
#include "StateBufferLayout.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Cobalt_StateBuffer_Internal* Cobalt_StateBuffer;

// Enumerations
typedef enum
{
	Cobalt_StateBufferPerformanceHint_Default = 0x00000000,
	Cobalt_StateBufferPerformanceHint_ReadNever = 0x00000001,
	Cobalt_StateBufferPerformanceHint_ReadRarely = 0x00000002,
	Cobalt_StateBufferPerformanceHint_ReadOften = 0x00000004,
	Cobalt_StateBufferPerformanceHint_ReadFlagsMask = 0x000000FF,
	Cobalt_StateBufferPerformanceHint_WriteNever = 0x00000100,
	Cobalt_StateBufferPerformanceHint_WriteRarely = 0x00000200,
	Cobalt_StateBufferPerformanceHint_WriteOften = 0x00000400,
	Cobalt_StateBufferPerformanceHint_WriteFlagsMask = 0x0000FF00,
} Cobalt_StateBufferPerformanceHint;

// Initialization methods
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_StateBuffer_AllocateMemory(Cobalt_StateBuffer buffer);

// Usage methods
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetPerformanceHints(Cobalt_StateBuffer buffer, Cobalt_StateBufferPerformanceHint performanceHintCpu, Cobalt_StateBufferPerformanceHint performanceHintGpu);

// Format methods
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetManualPageSize(Cobalt_StateBuffer buffer, size_t pageSizeInBytes);
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_StateBuffer_BindBufferLayout(Cobalt_StateBuffer buffer, Cobalt_StateBufferLayout stateBufferLayout);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetPageSettings(Cobalt_StateBuffer buffer, uint32_t initialPageCount, char allowDynamicResize);
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_StateBuffer_ResizePageCount(Cobalt_StateBuffer buffer, uint32_t pageCount);

// State value methods
COBALT_FUNCTION_EXPORT uint32_t Cobalt_StateBuffer_GetStateValueId(Cobalt_StateBuffer buffer, const char* name, size_t nameLength);

// State value methods
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueBool(Cobalt_StateBuffer buffer, uint32_t stateId, char value, const size_t* arrayIndices, size_t arrayIndexCount);

COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueV1Int8(Cobalt_StateBuffer buffer, uint32_t stateId, int8_t value, const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueV1Int16(Cobalt_StateBuffer buffer, uint32_t stateId, int16_t value, const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueV1Int32(Cobalt_StateBuffer buffer, uint32_t stateId, int32_t value, const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueV1UInt8(Cobalt_StateBuffer buffer, uint32_t stateId, uint8_t value, const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueV1UInt16(Cobalt_StateBuffer buffer, uint32_t stateId, uint16_t value, const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueV1UInt32(Cobalt_StateBuffer buffer, uint32_t stateId, uint32_t value, const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueV1Float32(Cobalt_StateBuffer buffer, uint32_t stateId, float value, const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueV1Float64(Cobalt_StateBuffer buffer, uint32_t stateId, double value, const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueV2Int8(Cobalt_StateBuffer buffer, uint32_t stateId, const int8_t value[2], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueV2Int16(Cobalt_StateBuffer buffer, uint32_t stateId, const int16_t value[2], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueV2Int32(Cobalt_StateBuffer buffer, uint32_t stateId, const int32_t value[2], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueV2UInt8(Cobalt_StateBuffer buffer, uint32_t stateId, const uint8_t value[2], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueV2UInt16(Cobalt_StateBuffer buffer, uint32_t stateId, const uint16_t value[2], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueV2UInt32(Cobalt_StateBuffer buffer, uint32_t stateId, const uint32_t value[2], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueV2Float32(Cobalt_StateBuffer buffer, uint32_t stateId, const float value[2], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueV2Float64(Cobalt_StateBuffer buffer, uint32_t stateId, const double value[2], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueV3Int8(Cobalt_StateBuffer buffer, uint32_t stateId, const int8_t value[3], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueV3Int16(Cobalt_StateBuffer buffer, uint32_t stateId, const int16_t value[3], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueV3Int32(Cobalt_StateBuffer buffer, uint32_t stateId, const int32_t value[3], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueV3UInt8(Cobalt_StateBuffer buffer, uint32_t stateId, const uint8_t value[3], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueV3UInt16(Cobalt_StateBuffer buffer, uint32_t stateId, const uint16_t value[3], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueV3UInt32(Cobalt_StateBuffer buffer, uint32_t stateId, const uint32_t value[3], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueV3Float32(Cobalt_StateBuffer buffer, uint32_t stateId, const float value[3], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueV3Float64(Cobalt_StateBuffer buffer, uint32_t stateId, const double value[3], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueV4Int8(Cobalt_StateBuffer buffer, uint32_t stateId, const int8_t value[4], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueV4Int16(Cobalt_StateBuffer buffer, uint32_t stateId, const int16_t value[4], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueV4Int32(Cobalt_StateBuffer buffer, uint32_t stateId, const int32_t value[4], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueV4UInt8(Cobalt_StateBuffer buffer, uint32_t stateId, const uint8_t value[4], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueV4UInt16(Cobalt_StateBuffer buffer, uint32_t stateId, const uint16_t value[4], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueV4UInt32(Cobalt_StateBuffer buffer, uint32_t stateId, const uint32_t value[4], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueV4Float32(Cobalt_StateBuffer buffer, uint32_t stateId, const float value[4], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueV4Float64(Cobalt_StateBuffer buffer, uint32_t stateId, const double value[4], const size_t* arrayIndices, size_t arrayIndexCount);

COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueM2Float32(Cobalt_StateBuffer buffer, uint32_t stateId, const float value[4], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueM3Float32(Cobalt_StateBuffer buffer, uint32_t stateId, const float value[9], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueM4Float32(Cobalt_StateBuffer buffer, uint32_t stateId, const float value[16], const size_t* arrayIndices, size_t arrayIndexCount);

COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueForPageBool(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, char value, const size_t* arrayIndices, size_t arrayIndexCount);

COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueForPageV1Int8(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, int8_t value, const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueForPageV1Int16(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, int16_t value, const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueForPageV1Int32(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, int32_t value, const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueForPageV1UInt8(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, uint8_t value, const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueForPageV1UInt16(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, uint16_t value, const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueForPageV1UInt32(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, uint32_t value, const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueForPageV1Float32(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, float value, const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueForPageV1Float64(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, double value, const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueForPageV2Int8(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, const int8_t value[2], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueForPageV2Int16(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, const int16_t value[2], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueForPageV2Int32(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, const int32_t value[2], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueForPageV2UInt8(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, const uint8_t value[2], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueForPageV2UInt16(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, const uint16_t value[2], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueForPageV2UInt32(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, const uint32_t value[2], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueForPageV2Float32(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, const float value[2], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueForPageV2Float64(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, const double value[2], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueForPageV3Int8(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, const int8_t value[3], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueForPageV3Int16(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, const int16_t value[3], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueForPageV3Int32(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, const int32_t value[3], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueForPageV3UInt8(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, const uint8_t value[3], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueForPageV3UInt16(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, const uint16_t value[3], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueForPageV3UInt32(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, const uint32_t value[3], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueForPageV3Float32(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, const float value[3], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueForPageV3Float64(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, const double value[3], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueForPageV4Int8(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, const int8_t value[4], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueForPageV4Int16(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, const int16_t value[4], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueForPageV4Int32(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, const int32_t value[4], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueForPageV4UInt8(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, const uint8_t value[4], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueForPageV4UInt16(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, const uint16_t value[4], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueForPageV4UInt32(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, const uint32_t value[4], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueForPageV4Float32(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, const float value[4], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueForPageV4Float64(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, const double value[4], const size_t* arrayIndices, size_t arrayIndexCount);

COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueForPageM2Float32(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, const float value[4], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueForPageM3Float32(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, const float value[9], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetStateValueForPageM4Float32(Cobalt_StateBuffer buffer, uint32_t pageNo, uint32_t stateId, const float value[16], const size_t* arrayIndices, size_t arrayIndexCount);

COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_SetRawPageData(Cobalt_StateBuffer buffer, uint32_t pageNo, const uint8_t* data, size_t dataSizeInBytes, size_t dataOffsetInBytes);

COBALT_FUNCTION_EXPORT void Cobalt_StateBuffer_Delete(Cobalt_StateBuffer buffer);

#ifdef __cplusplus
}
#endif
