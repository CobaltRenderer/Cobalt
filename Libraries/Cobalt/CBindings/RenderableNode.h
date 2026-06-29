// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "DataArray.h"
#include "IndexAttribute.h"
#include "Macros.h"
#include "Result.h"
#include "VertexAttribute.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Cobalt_RenderableNode_Internal* Cobalt_RenderableNode;

typedef enum
{
	Cobalt_PrimitiveMode_Points,
	Cobalt_PrimitiveMode_Lines,
	Cobalt_PrimitiveMode_Triangles,
	Cobalt_PrimitiveMode_LineStrip,
	Cobalt_PrimitiveMode_TriangleStrip,
} Cobalt_PrimitiveMode;

// Structures
typedef struct Cobalt_IndirectDrawParams
{
	uint32_t vertexCount;
	uint32_t instanceCount;
	uint32_t firstVertex;
	uint32_t firstInstance;
} Cobalt_IndirectDrawParams;

typedef struct Cobalt_IndexedIndirectDrawParams
{
	uint32_t indexCount;
	uint32_t instanceCount;
	uint32_t firstIndex;
	int32_t vertexOffset;
	uint32_t firstInstance;
} Cobalt_IndexedIndirectDrawParams;

// Binding methods
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_RenderableNode_BindVertexAttribute(Cobalt_RenderableNode renderableNode, Cobalt_VertexAttribute attribute, uint32_t shaderAttributeID);
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_RenderableNode_BindVertexInstanceAttribute(Cobalt_RenderableNode renderableNode, Cobalt_VertexAttribute attribute, uint32_t shaderAttributeID);
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_RenderableNode_BindIndexAttribute(Cobalt_RenderableNode renderableNode, Cobalt_IndexAttribute attribute);

// Primitive mode methods
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_RenderableNode_SetPrimitiveMode(Cobalt_RenderableNode renderableNode, Cobalt_PrimitiveMode primitiveMode, char primitiveRestartEnabled, char adjacencyEnabled);
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_RenderableNode_SetVertexCount(Cobalt_RenderableNode renderableNode, size_t vertexCount, size_t vertexBufferOffset, size_t indexBufferOffset, ptrdiff_t indexValueOffset);
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_RenderableNode_SetInstanceMode(Cobalt_RenderableNode renderableNode, uint32_t instanceCount, uint32_t instanceOffset);
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_RenderableNode_SetIndirectDraw(Cobalt_RenderableNode renderableNode, size_t drawCount, Cobalt_DataArray sourceDataArray, size_t arrayOffsetInBytes, size_t arrayStrideInBytes);
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_RenderableNode_SetIndirectDrawCounter(Cobalt_RenderableNode renderableNode, size_t maxDrawCount, Cobalt_DataArray drawCountSourceCounter, Cobalt_DataArray sourceDataArray, size_t arrayOffsetInBytes, size_t arrayStrideInBytes);
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_RenderableNode_SetIndirectDrawCounterWithOffset(Cobalt_RenderableNode renderableNode, size_t maxDrawCount, Cobalt_DataArray drawCountSourceDataArray, size_t drawCountArrayOffsetInBytes, Cobalt_DataArray sourceDataArray, size_t arrayOffsetInBytes, size_t arrayStrideInBytes);

// Debug methods
COBALT_FUNCTION_EXPORT void Cobalt_RenderableNode_SetDebugName(Cobalt_RenderableNode renderableNode, const char* name, size_t nameLength);

// Initialization methods
COBALT_FUNCTION_EXPORT void Cobalt_RenderableNode_Delete(Cobalt_RenderableNode renderableNode);

#ifdef __cplusplus
}
#endif
