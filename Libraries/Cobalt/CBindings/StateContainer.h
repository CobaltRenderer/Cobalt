// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "DataArray.h"
#include "Macros.h"
#include "StateBuffer.h"
#include "TexelArray.h"
#include "TextureBuffer1D.h"
#include "TextureBuffer1DArray.h"
#include "TextureBuffer2D.h"
#include "TextureBuffer2DArray.h"
#include "TextureBuffer3D.h"
#include "TextureBufferCube.h"
#include "TextureBufferCubeArray.h"
#include "TextureSampler1D.h"
#include "TextureSampler1DArray.h"
#include "TextureSampler2D.h"
#include "TextureSampler2DArray.h"
#include "TextureSampler3D.h"
#include "TextureSamplerCube.h"
#include "TextureSamplerCubeArray.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Cobalt_StateContainer_Internal* Cobalt_StateContainer;

// Resource binding methods
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_BindTextureWithCombinedSampler1D(Cobalt_StateContainer container, uint32_t textureId, Cobalt_TextureBuffer1D texture, Cobalt_TextureSampler1D sampler);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_BindTextureWithCombinedSampler2D(Cobalt_StateContainer container, uint32_t textureId, Cobalt_TextureBuffer2D texture, Cobalt_TextureSampler2D sampler);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_BindTextureWithCombinedSampler3D(Cobalt_StateContainer container, uint32_t textureId, Cobalt_TextureBuffer3D texture, Cobalt_TextureSampler3D sampler);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_BindTextureWithCombinedSamplerCube(Cobalt_StateContainer container, uint32_t textureId, Cobalt_TextureBufferCube texture, Cobalt_TextureSamplerCube sampler);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_BindTextureWithCombinedSampler1DArray(Cobalt_StateContainer container, uint32_t textureId, Cobalt_TextureBuffer1DArray texture, Cobalt_TextureSampler1DArray sampler);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_BindTextureWithCombinedSampler2DArray(Cobalt_StateContainer container, uint32_t textureId, Cobalt_TextureBuffer2DArray texture, Cobalt_TextureSampler2DArray sampler);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_BindTextureWithCombinedSamplerCubeArray(Cobalt_StateContainer container, uint32_t textureId, Cobalt_TextureBufferCubeArray texture, Cobalt_TextureSamplerCubeArray sampler);

COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_BindTexture1D(Cobalt_StateContainer container, uint32_t textureId, Cobalt_TextureBuffer1D texture);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_BindTexture2D(Cobalt_StateContainer container, uint32_t textureId, Cobalt_TextureBuffer2D texture);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_BindTexture3D(Cobalt_StateContainer container, uint32_t textureId, Cobalt_TextureBuffer3D texture);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_BindTextureCube(Cobalt_StateContainer container, uint32_t textureId, Cobalt_TextureBufferCube texture);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_BindTexture1DArray(Cobalt_StateContainer container, uint32_t textureId, Cobalt_TextureBuffer1DArray texture);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_BindTexture2DArray(Cobalt_StateContainer container, uint32_t textureId, Cobalt_TextureBuffer2DArray texture);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_BindTextureCubeArray(Cobalt_StateContainer container, uint32_t textureId, Cobalt_TextureBufferCubeArray texture);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_UnbindTexture(Cobalt_StateContainer container, uint32_t textureId);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_BindSampler1D(Cobalt_StateContainer container, uint32_t samplerId, Cobalt_TextureSampler1D sampler);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_BindSampler2D(Cobalt_StateContainer container, uint32_t samplerId, Cobalt_TextureSampler2D sampler);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_BindSampler3D(Cobalt_StateContainer container, uint32_t samplerId, Cobalt_TextureSampler3D sampler);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_BindSamplerCube(Cobalt_StateContainer container, uint32_t samplerId, Cobalt_TextureSamplerCube sampler);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_BindSampler1DArray(Cobalt_StateContainer container, uint32_t samplerId, Cobalt_TextureSampler1DArray sampler);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_BindSampler2DArray(Cobalt_StateContainer container, uint32_t samplerId, Cobalt_TextureSampler2DArray sampler);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_BindSamplerCubeArray(Cobalt_StateContainer container, uint32_t samplerId, Cobalt_TextureSamplerCubeArray sampler);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_UnbindSampler(Cobalt_StateContainer container, uint32_t samplerId);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_BindStateBuffer(Cobalt_StateContainer container, uint32_t stateBufferId, Cobalt_StateBuffer stateBuffer, uint32_t stateBufferPageNo);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_UnbindStateBuffer(Cobalt_StateContainer container, uint32_t stateBufferId);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_BindDataArray(Cobalt_StateContainer container, uint32_t resourceArrayId, Cobalt_DataArray dataArray, char resetCounter);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_BindTexelArray(Cobalt_StateContainer container, uint32_t resourceArrayId, Cobalt_TexelArray texelArray);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_UnbindResourceArray(Cobalt_StateContainer container, uint32_t resourceArrayId);

// State value methods
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_SetStateValueBool(Cobalt_StateContainer container, uint32_t stateId, char value, const size_t* arrayIndices, size_t arrayIndexCount);

COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_SetStateValueV1Int8(Cobalt_StateContainer container, uint32_t stateId, int8_t value, const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_SetStateValueV1Int16(Cobalt_StateContainer container, uint32_t stateId, int16_t value, const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_SetStateValueV1Int32(Cobalt_StateContainer container, uint32_t stateId, int32_t value, const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_SetStateValueV1UInt8(Cobalt_StateContainer container, uint32_t stateId, uint8_t value, const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_SetStateValueV1UInt16(Cobalt_StateContainer container, uint32_t stateId, uint16_t value, const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_SetStateValueV1UInt32(Cobalt_StateContainer container, uint32_t stateId, uint32_t value, const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_SetStateValueV1Float32(Cobalt_StateContainer container, uint32_t stateId, float value, const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_SetStateValueV1Float64(Cobalt_StateContainer container, uint32_t stateId, double value, const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_SetStateValueV2Int8(Cobalt_StateContainer container, uint32_t stateId, const int8_t value[2], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_SetStateValueV2Int16(Cobalt_StateContainer container, uint32_t stateId, const int16_t value[2], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_SetStateValueV2Int32(Cobalt_StateContainer container, uint32_t stateId, const int32_t value[2], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_SetStateValueV2UInt8(Cobalt_StateContainer container, uint32_t stateId, const uint8_t value[2], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_SetStateValueV2UInt16(Cobalt_StateContainer container, uint32_t stateId, const uint16_t value[2], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_SetStateValueV2UInt32(Cobalt_StateContainer container, uint32_t stateId, const uint32_t value[2], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_SetStateValueV2Float32(Cobalt_StateContainer container, uint32_t stateId, const float value[2], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_SetStateValueV2Float64(Cobalt_StateContainer container, uint32_t stateId, const double value[2], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_SetStateValueV3Int8(Cobalt_StateContainer container, uint32_t stateId, const int8_t value[3], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_SetStateValueV3Int16(Cobalt_StateContainer container, uint32_t stateId, const int16_t value[3], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_SetStateValueV3Int32(Cobalt_StateContainer container, uint32_t stateId, const int32_t value[3], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_SetStateValueV3UInt8(Cobalt_StateContainer container, uint32_t stateId, const uint8_t value[3], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_SetStateValueV3UInt16(Cobalt_StateContainer container, uint32_t stateId, const uint16_t value[3], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_SetStateValueV3UInt32(Cobalt_StateContainer container, uint32_t stateId, const uint32_t value[3], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_SetStateValueV3Float32(Cobalt_StateContainer container, uint32_t stateId, const float value[3], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_SetStateValueV3Float64(Cobalt_StateContainer container, uint32_t stateId, const double value[3], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_SetStateValueV4Int8(Cobalt_StateContainer container, uint32_t stateId, const int8_t value[4], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_SetStateValueV4Int16(Cobalt_StateContainer container, uint32_t stateId, const int16_t value[4], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_SetStateValueV4Int32(Cobalt_StateContainer container, uint32_t stateId, const int32_t value[4], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_SetStateValueV4UInt8(Cobalt_StateContainer container, uint32_t stateId, const uint8_t value[4], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_SetStateValueV4UInt16(Cobalt_StateContainer container, uint32_t stateId, const uint16_t value[4], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_SetStateValueV4UInt32(Cobalt_StateContainer container, uint32_t stateId, const uint32_t value[4], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_SetStateValueV4Float32(Cobalt_StateContainer container, uint32_t stateId, const float value[4], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_SetStateValueV4Float64(Cobalt_StateContainer container, uint32_t stateId, const double value[4], const size_t* arrayIndices, size_t arrayIndexCount);

COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_SetStateValueM2Float32(Cobalt_StateContainer container, uint32_t stateId, const float value[4], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_SetStateValueM3Float32(Cobalt_StateContainer container, uint32_t stateId, const float value[9], const size_t* arrayIndices, size_t arrayIndexCount);
COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_SetStateValueM4Float32(Cobalt_StateContainer container, uint32_t stateId, const float value[16], const size_t* arrayIndices, size_t arrayIndexCount);

COBALT_FUNCTION_EXPORT void Cobalt_StateContainer_ResetStateValue(Cobalt_StateContainer container, uint32_t stateId, const size_t* arrayIndices, size_t arrayIndexCount);

#ifdef __cplusplus
}
#endif
