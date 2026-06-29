// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Macros.h"
#include "Result.h"
#include "TexelArray.h"
#include "VertexAttribute.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Cobalt_VertexBuffer_Internal* Cobalt_VertexBuffer;

COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_VertexBuffer_AllocateMemory(Cobalt_VertexBuffer vertexBuffer);
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_VertexBuffer_AllocateMemoryWithAlias(Cobalt_VertexBuffer vertexBuffer, Cobalt_TexelArray texelArray);

// Binding methods
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_VertexBuffer_BindVertexAttribute(Cobalt_VertexBuffer vertexBuffer, Cobalt_VertexAttribute attribute);
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_VertexBuffer_BindVertexAttributeManualLayout(Cobalt_VertexBuffer vertexBuffer, Cobalt_VertexAttribute attribute, size_t bufferOffsetInBytes, size_t bufferStrideInBytes);

// Data methods
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_VertexBuffer_SetRawInitialData(Cobalt_VertexBuffer vertexBuffer, const uint8_t* data, size_t dataSizeInBytes);
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_VertexBuffer_QueueRawDataUpdate(Cobalt_VertexBuffer vertexBuffer, const uint8_t* data, size_t dataSizeInBytes, size_t bufferOffsetInBytes, Cobalt_TransferBatch transferBatch);

// Initialization methods
COBALT_FUNCTION_EXPORT void Cobalt_VertexBuffer_Delete(Cobalt_VertexBuffer vertexBuffer);

#ifdef __cplusplus
}
#endif
