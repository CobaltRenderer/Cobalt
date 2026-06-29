// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Macros.h"
#include "TextureSampler.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Cobalt_TextureSamplerCubeArray_Internal* Cobalt_TextureSamplerCubeArray;

// Format methods
COBALT_FUNCTION_EXPORT void Cobalt_TextureSamplerCubeArray_SetTextureWrapMode(Cobalt_TextureSamplerCubeArray sampler, Cobalt_WrapMode wrapModeHorizontal, Cobalt_WrapMode wrapModeVertical);
COBALT_FUNCTION_EXPORT void Cobalt_TextureSamplerCubeArray_SetTextureFilterMode(Cobalt_TextureSamplerCubeArray sampler, Cobalt_FilterMode filterModeShrink, Cobalt_FilterMode filterModeExpand);
COBALT_FUNCTION_EXPORT void Cobalt_TextureSamplerCubeArray_SetTextureMipmapMode(Cobalt_TextureSamplerCubeArray sampler, Cobalt_MipmapMode mipmapMode);
COBALT_FUNCTION_EXPORT void Cobalt_TextureSamplerCubeArray_SetMipmapLevelMapping(Cobalt_TextureSamplerCubeArray sampler, float minLevel, float maxLevel, float levelBias);

// Initialization methods
COBALT_FUNCTION_EXPORT void Cobalt_TextureSamplerCubeArray_Delete(Cobalt_TextureSamplerCubeArray sampler);

#ifdef __cplusplus
}
#endif
