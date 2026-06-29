// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Macros.h"
#include "Result.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Cobalt_StateBufferLayout_Internal* Cobalt_StateBufferLayout;

// Enumerations
typedef enum
{
	Cobalt_StateBufferDataType_Null = 0,
	Cobalt_StateBufferDataType_Boolean,
	Cobalt_StateBufferDataType_Int32,
	Cobalt_StateBufferDataType_UInt32,
	Cobalt_StateBufferDataType_Float32,
	Cobalt_StateBufferDataType_Float64,
} Cobalt_StateBufferDataType;

// State layout building methods
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_StateBufferLayout_BeginLayoutDefinition(Cobalt_StateBufferLayout layout);
COBALT_FUNCTION_EXPORT void Cobalt_StateBufferLayout_AppendField(Cobalt_StateBufferLayout layout, const char* fieldName, size_t fieldNameLength, Cobalt_StateBufferDataType type, size_t arraySize);
COBALT_FUNCTION_EXPORT void Cobalt_StateBufferLayout_AppendVector(Cobalt_StateBufferLayout layout, const char* fieldName, size_t fieldNameLength, Cobalt_StateBufferDataType type, size_t elementCount, size_t arraySize);
COBALT_FUNCTION_EXPORT void Cobalt_StateBufferLayout_AppendMatrix(Cobalt_StateBufferLayout layout, const char* fieldName, size_t fieldNameLength, Cobalt_StateBufferDataType type, size_t width, size_t height, size_t arraySize);
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_StateBufferLayout_ConstructStateLayout(Cobalt_StateBufferLayout layout);

COBALT_FUNCTION_EXPORT void Cobalt_StateBufferLayout_Delete(Cobalt_StateBufferLayout layout);

#ifdef __cplusplus
}
#endif
