// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Macros.h"
#include "TextureSampler.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Cobalt_TextureSamplerCube_Internal* Cobalt_TextureSamplerCube;

// Format methods
COBALT_FUNCTION_EXPORT void Cobalt_TextureSamplerCube_SetTextureWrapMode(Cobalt_TextureSamplerCube sampler, Cobalt_WrapMode wrapModeHorizontal, Cobalt_WrapMode wrapModeVertical);
COBALT_FUNCTION_EXPORT void Cobalt_TextureSamplerCube_SetTextureFilterMode(Cobalt_TextureSamplerCube sampler, Cobalt_FilterMode filterModeShrink, Cobalt_FilterMode filterModeExpand);
COBALT_FUNCTION_EXPORT void Cobalt_TextureSamplerCube_SetTextureMipmapMode(Cobalt_TextureSamplerCube sampler, Cobalt_MipmapMode mipmapMode);
COBALT_FUNCTION_EXPORT void Cobalt_TextureSamplerCube_SetMipmapLevelMapping(Cobalt_TextureSamplerCube sampler, float minLevel, float maxLevel, float levelBias);

// Initialization methods
COBALT_FUNCTION_EXPORT void Cobalt_TextureSamplerCube_Delete(Cobalt_TextureSamplerCube sampler);

#ifdef __cplusplus
}
#endif
