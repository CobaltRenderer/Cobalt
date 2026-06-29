// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Macros.h"
#include "Result.h"
#include "ShaderProgram.h"
#include "StateGroupNode.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Cobalt_ProgramNode_Internal* Cobalt_ProgramNode;

// Child node methods
COBALT_FUNCTION_EXPORT void Cobalt_ProgramNode_AddChildNode(Cobalt_ProgramNode programNode, Cobalt_StateGroupNode childNode);
COBALT_FUNCTION_EXPORT void Cobalt_ProgramNode_AddChildNodes(Cobalt_ProgramNode programNode, const Cobalt_StateGroupNode* childNodes, size_t childNodeCount);
COBALT_FUNCTION_EXPORT void Cobalt_ProgramNode_RemoveChildNode(Cobalt_ProgramNode programNode, Cobalt_StateGroupNode childNode);
COBALT_FUNCTION_EXPORT void Cobalt_ProgramNode_RemoveChildNodes(Cobalt_ProgramNode programNode, const Cobalt_StateGroupNode* childNodes, size_t childNodeCount);
COBALT_FUNCTION_EXPORT void Cobalt_ProgramNode_RemoveAllChildNodes(Cobalt_ProgramNode programNode);
COBALT_FUNCTION_EXPORT void Cobalt_ProgramNode_SetChildNodes(Cobalt_ProgramNode programNode, const Cobalt_StateGroupNode* childNodes, size_t childNodeCount);

// Shader program methods
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_ProgramNode_BindShaderProgram(Cobalt_ProgramNode programNode, Cobalt_ShaderProgram shaderProgram);

// Constant state value methods
COBALT_FUNCTION_EXPORT void Cobalt_ProgramNode_SetConstantStateValueBool(Cobalt_ProgramNode programNode, uint32_t stateId, char value, const size_t* arrayIndices, size_t arrayIndexCount);

COBALT_FUNCTION_EXPORT void Cobalt_ProgramNode_SetConstantStateValueV1Int8(Cobalt_ProgramNode programNode, uint32_t stateId, int8_t value, const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_ProgramNode_SetConstantStateValueV1Int16(Cobalt_ProgramNode programNode, uint32_t stateId, int16_t value, const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_ProgramNode_SetConstantStateValueV1Int32(Cobalt_ProgramNode programNode, uint32_t stateId, int32_t value, const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_ProgramNode_SetConstantStateValueV1UInt8(Cobalt_ProgramNode programNode, uint32_t stateId, uint8_t value, const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_ProgramNode_SetConstantStateValueV1UInt16(Cobalt_ProgramNode programNode, uint32_t stateId, uint16_t value, const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_ProgramNode_SetConstantStateValueV1UInt32(Cobalt_ProgramNode programNode, uint32_t stateId, uint32_t value, const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_ProgramNode_SetConstantStateValueV1Float32(Cobalt_ProgramNode programNode, uint32_t stateId, float value, const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_ProgramNode_SetConstantStateValueV1Float64(Cobalt_ProgramNode programNode, uint32_t stateId, double value, const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_ProgramNode_SetConstantStateValueV2Int8(Cobalt_ProgramNode programNode, uint32_t stateId, const int8_t value[2], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_ProgramNode_SetConstantStateValueV2Int16(Cobalt_ProgramNode programNode, uint32_t stateId, const int16_t value[2], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_ProgramNode_SetConstantStateValueV2Int32(Cobalt_ProgramNode programNode, uint32_t stateId, const int32_t value[2], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_ProgramNode_SetConstantStateValueV2UInt8(Cobalt_ProgramNode programNode, uint32_t stateId, const uint8_t value[2], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_ProgramNode_SetConstantStateValueV2UInt16(Cobalt_ProgramNode programNode, uint32_t stateId, const uint16_t value[2], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_ProgramNode_SetConstantStateValueV2UInt32(Cobalt_ProgramNode programNode, uint32_t stateId, const uint32_t value[2], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_ProgramNode_SetConstantStateValueV2Float32(Cobalt_ProgramNode programNode, uint32_t stateId, const float value[2], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_ProgramNode_SetConstantStateValueV2Float64(Cobalt_ProgramNode programNode, uint32_t stateId, const double value[2], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_ProgramNode_SetConstantStateValueV3Int8(Cobalt_ProgramNode programNode, uint32_t stateId, const int8_t value[3], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_ProgramNode_SetConstantStateValueV3Int16(Cobalt_ProgramNode programNode, uint32_t stateId, const int16_t value[3], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_ProgramNode_SetConstantStateValueV3Int32(Cobalt_ProgramNode programNode, uint32_t stateId, const int32_t value[3], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_ProgramNode_SetConstantStateValueV3UInt8(Cobalt_ProgramNode programNode, uint32_t stateId, const uint8_t value[3], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_ProgramNode_SetConstantStateValueV3UInt16(Cobalt_ProgramNode programNode, uint32_t stateId, const uint16_t value[3], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_ProgramNode_SetConstantStateValueV3UInt32(Cobalt_ProgramNode programNode, uint32_t stateId, const uint32_t value[3], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_ProgramNode_SetConstantStateValueV3Float32(Cobalt_ProgramNode programNode, uint32_t stateId, const float value[3], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_ProgramNode_SetConstantStateValueV3Float64(Cobalt_ProgramNode programNode, uint32_t stateId, const double value[3], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_ProgramNode_SetConstantStateValueV4Int8(Cobalt_ProgramNode programNode, uint32_t stateId, const int8_t value[4], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_ProgramNode_SetConstantStateValueV4Int16(Cobalt_ProgramNode programNode, uint32_t stateId, const int16_t value[4], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_ProgramNode_SetConstantStateValueV4Int32(Cobalt_ProgramNode programNode, uint32_t stateId, const int32_t value[4], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_ProgramNode_SetConstantStateValueV4UInt8(Cobalt_ProgramNode programNode, uint32_t stateId, const uint8_t value[4], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_ProgramNode_SetConstantStateValueV4UInt16(Cobalt_ProgramNode programNode, uint32_t stateId, const uint16_t value[4], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_ProgramNode_SetConstantStateValueV4UInt32(Cobalt_ProgramNode programNode, uint32_t stateId, const uint32_t value[4], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_ProgramNode_SetConstantStateValueV4Float32(Cobalt_ProgramNode programNode, uint32_t stateId, const float value[4], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_ProgramNode_SetConstantStateValueV4Float64(Cobalt_ProgramNode programNode, uint32_t stateId, const double value[4], const size_t* arrayIndices, size_t arrayIndexCount);

COBALT_FUNCTION_EXPORT void Cobalt_ProgramNode_SetConstantStateValueM2Float32(Cobalt_ProgramNode programNode, uint32_t stateId, const float value[4], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_ProgramNode_SetConstantStateValueM3Float32(Cobalt_ProgramNode programNode, uint32_t stateId, const float value[9], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_ProgramNode_SetConstantStateValueM4Float32(Cobalt_ProgramNode programNode, uint32_t stateId, const float value[16], const size_t* arrayIndices, size_t arrayIndexCount);

COBALT_FUNCTION_EXPORT void Cobalt_ProgramNode_ResetConstantStateValue(Cobalt_ProgramNode programNode, uint32_t stateId, const size_t* arrayIndices, size_t arrayIndexCount);

// Debug methods
COBALT_FUNCTION_EXPORT void Cobalt_ProgramNode_SetDebugName(Cobalt_ProgramNode programNode, const char* name, size_t nameLength);

// Initialization methods
COBALT_FUNCTION_EXPORT void Cobalt_ProgramNode_Delete(Cobalt_ProgramNode programNode);

#ifdef __cplusplus
}
#endif
