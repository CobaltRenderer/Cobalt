// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "IndexAttribute.h"
#include "Macros.h"
#include "Result.h"
#include "TexelArray.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Cobalt_IndexBuffer_Internal* Cobalt_IndexBuffer;

COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_IndexBuffer_AllocateMemory(Cobalt_IndexBuffer indexBuffer);
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_IndexBuffer_AllocateMemoryWithAlias(Cobalt_IndexBuffer indexBuffer, Cobalt_TexelArray texelArray);

// Binding methods
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_IndexBuffer_BindIndexAttribute(Cobalt_IndexBuffer indexBuffer, Cobalt_IndexAttribute attribute);
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_IndexBuffer_BindIndexAttributeManualLayout(Cobalt_IndexBuffer indexBuffer, Cobalt_IndexAttribute attribute, size_t bufferOffsetInBytes, size_t bufferStrideInBytes);

// Data methods
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_IndexBuffer_SetRawInitialData(Cobalt_IndexBuffer indexBuffer, const uint8_t* data, size_t dataSizeInBytes);
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_IndexBuffer_QueueRawDataUpdate(Cobalt_IndexBuffer indexBuffer, const uint8_t* data, size_t dataSizeInBytes, size_t bufferOffsetInBytes, Cobalt_TransferBatch transferBatch);

// Initialization methods
COBALT_FUNCTION_EXPORT void Cobalt_IndexBuffer_Delete(Cobalt_IndexBuffer indexBuffer);

#ifdef __cplusplus
}
#endif
