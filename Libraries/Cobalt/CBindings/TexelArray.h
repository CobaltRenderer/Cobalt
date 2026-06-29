// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Macros.h"
#include "Result.h"
#include "TransferBatch.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Cobalt_TexelArray_Internal* Cobalt_TexelArray;
typedef struct Cobalt_TexelArrayOutput_Internal* Cobalt_TexelArrayOutput;

// Enumerations
typedef enum
{
	Cobalt_TexelArrayImageFormat_R,
	Cobalt_TexelArrayImageFormat_RG,
	Cobalt_TexelArrayImageFormat_RGBA,
	Cobalt_TexelArrayImageFormat_X,
	Cobalt_TexelArrayImageFormat_XY,
	Cobalt_TexelArrayImageFormat_XYZW,
} Cobalt_TexelArrayImageFormat;

typedef enum
{
	Cobalt_TexelArrayDataFormat_Int8,
	Cobalt_TexelArrayDataFormat_Int16,
	Cobalt_TexelArrayDataFormat_Int32,
	Cobalt_TexelArrayDataFormat_UInt8,
	Cobalt_TexelArrayDataFormat_UInt16,
	Cobalt_TexelArrayDataFormat_UInt32,
	Cobalt_TexelArrayDataFormat_Norm8,
	Cobalt_TexelArrayDataFormat_UNorm8,
	Cobalt_TexelArrayDataFormat_Float16,
	Cobalt_TexelArrayDataFormat_Float32,
} Cobalt_TexelArrayDataFormat;

typedef enum
{
	Cobalt_TexelArraySourceImageFormat_R,
	Cobalt_TexelArraySourceImageFormat_RG,
	Cobalt_TexelArraySourceImageFormat_RGBA,
	Cobalt_TexelArraySourceImageFormat_X,
	Cobalt_TexelArraySourceImageFormat_XY,
	Cobalt_TexelArraySourceImageFormat_XYZW,
} Cobalt_TexelArraySourceImageFormat;

typedef enum
{
	Cobalt_TexelArraySourceDataFormat_Int8,
	Cobalt_TexelArraySourceDataFormat_Int16,
	Cobalt_TexelArraySourceDataFormat_Int32,
	Cobalt_TexelArraySourceDataFormat_UInt8,
	Cobalt_TexelArraySourceDataFormat_UInt16,
	Cobalt_TexelArraySourceDataFormat_UInt32,
	Cobalt_TexelArraySourceDataFormat_Norm8,
	Cobalt_TexelArraySourceDataFormat_Norm16,
	Cobalt_TexelArraySourceDataFormat_Norm32,
	Cobalt_TexelArraySourceDataFormat_UNorm8,
	Cobalt_TexelArraySourceDataFormat_UNorm16,
	Cobalt_TexelArraySourceDataFormat_UNorm32,
	Cobalt_TexelArraySourceDataFormat_Float16,
	Cobalt_TexelArraySourceDataFormat_Float32,
} Cobalt_TexelArraySourceDataFormat;

typedef enum
{
	Cobalt_TexelArrayUsageFlags_Default = 0x00000000,
	Cobalt_TexelArrayUsageFlags_ShaderInput = 0x00000001,
	Cobalt_TexelArrayUsageFlags_ShaderOutput = 0x00000002,
	Cobalt_TexelArrayUsageFlags_TransferSource = 0x00000004,
	Cobalt_TexelArrayUsageFlags_TransferDestination = 0x00000008,
} Cobalt_TexelArrayUsageFlags;

// Initialization methods
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_TexelArray_AllocateMemory(Cobalt_TexelArray array);
COBALT_FUNCTION_EXPORT void Cobalt_TexelArray_SetBufferLayout(Cobalt_TexelArray array, Cobalt_TexelArrayImageFormat imageFormat, Cobalt_TexelArrayDataFormat dataFormat, size_t entryCount);

// Usage methods
COBALT_FUNCTION_EXPORT void Cobalt_TexelArray_SetUsageFlags(Cobalt_TexelArray array, Cobalt_TexelArrayUsageFlags usageFlags);

// Initial data methods
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_TexelArray_SetInitialData(Cobalt_TexelArray array, const void* sourceBuffer, size_t sourceBufferSizeInBytes, Cobalt_TexelArraySourceImageFormat imageFormat, Cobalt_TexelArraySourceDataFormat dataFormat);

// Data update methods
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_TexelArray_QueueDataUpdate(Cobalt_TexelArray array, const void* sourceBuffer, size_t sourceBufferSizeInBytes, Cobalt_TexelArraySourceImageFormat imageFormat, Cobalt_TexelArraySourceDataFormat dataFormat, size_t targetBufferOffset, Cobalt_TransferBatch transferBatch);

// Data transfer methods
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_TexelArray_QueueDataTransfer(Cobalt_TexelArray array, Cobalt_TexelArray targetBuffer, size_t transferCount, size_t sourceBufferOffset, size_t targetBufferOffset, Cobalt_TransferBatch transferBatch);

// Output capture methods
COBALT_FUNCTION_EXPORT void Cobalt_TexelArray_AddOutputCaptureTarget(Cobalt_TexelArray array, Cobalt_TexelArrayOutput captureTarget);
COBALT_FUNCTION_EXPORT void Cobalt_TexelArray_RemoveOutputCaptureTarget(Cobalt_TexelArray array, Cobalt_TexelArrayOutput captureTarget);

COBALT_FUNCTION_EXPORT void Cobalt_TexelArray_Delete(Cobalt_TexelArray array);

#ifdef __cplusplus
}
#endif
