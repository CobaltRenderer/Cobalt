// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Macros.h"
#include "ResourceArray.h"
#include "Result.h"
#include "TransferBatch.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Cobalt_DataArray_Internal* Cobalt_DataArray;
typedef struct Cobalt_DataArrayOutput_Internal* Cobalt_DataArrayOutput;

// Enumerations
typedef enum
{
	Cobalt_DataArrayUsageFlags_Default = 0x00000000,
	Cobalt_DataArrayUsageFlags_ShaderInput = 0x00000001,
	Cobalt_DataArrayUsageFlags_ShaderOutput = 0x00000002,
	Cobalt_DataArrayUsageFlags_TransferSource = 0x00000004,
	Cobalt_DataArrayUsageFlags_TransferDestination = 0x00000008,
	Cobalt_DataArrayUsageFlags_IndirectDrawSource = 0x00000010,
	Cobalt_DataArrayUsageFlags_IndirectDrawCountSource = 0x00000020,
	Cobalt_DataArrayUsageFlags_AtomicOperations = 0x00000040,
} Cobalt_DataArrayUsageFlags;

// Initialization methods
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_DataArray_AllocateMemory(Cobalt_DataArray array);
COBALT_FUNCTION_EXPORT void Cobalt_DataArray_SetBufferLayout(Cobalt_DataArray array, size_t entryStrideInBytes, size_t entryCount, char hasCounter, uint32_t counterResetValue);

// Usage methods
COBALT_FUNCTION_EXPORT void Cobalt_DataArray_SetUsageFlags(Cobalt_DataArray array, Cobalt_DataArrayUsageFlags usageFlags);

// Initial data methods
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_DataArray_SetInitialData(Cobalt_DataArray array, const void* sourceBuffer, size_t sourceBufferSizeInBytes);

// Data update methods
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_DataArray_QueueDataUpdate(Cobalt_DataArray array, const void* sourceBuffer, size_t sourceBufferSizeInBytes, size_t targetBufferOffsetInBytes, Cobalt_TransferBatch transferBatch);
COBALT_FUNCTION_EXPORT void Cobalt_DataArray_UpdateCounterResetValue(Cobalt_DataArray array, uint32_t counterResetValue);

// Data transfer methods
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_DataArray_QueueDataTransfer(Cobalt_DataArray array, Cobalt_DataArray targetBuffer, size_t transferCountInBytes, size_t sourceBufferOffsetInBytes, size_t targetBufferOffsetInBytes, Cobalt_TransferBatch transferBatch);

// Output capture methods
COBALT_FUNCTION_EXPORT void Cobalt_DataArray_AddOutputCaptureTarget(Cobalt_DataArray array, Cobalt_DataArrayOutput captureTarget);
COBALT_FUNCTION_EXPORT void Cobalt_DataArray_RemoveOutputCaptureTarget(Cobalt_DataArray array, Cobalt_DataArrayOutput captureTarget);

COBALT_FUNCTION_EXPORT void Cobalt_DataArray_Delete(Cobalt_DataArray array);

#ifdef __cplusplus
}
#endif
