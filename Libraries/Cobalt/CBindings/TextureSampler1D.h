// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Macros.h"
#include "TextureSampler.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Cobalt_TextureSampler1D_Internal* Cobalt_TextureSampler1D;

// Format methods
COBALT_FUNCTION_EXPORT void Cobalt_TextureSampler1D_SetTextureWrapMode(Cobalt_TextureSampler1D sampler, Cobalt_WrapMode wrapModeHorizontal);
COBALT_FUNCTION_EXPORT void Cobalt_TextureSampler1D_SetTextureFilterMode(Cobalt_TextureSampler1D sampler, Cobalt_FilterMode filterModeShrink, Cobalt_FilterMode filterModeExpand);
COBALT_FUNCTION_EXPORT void Cobalt_TextureSampler1D_SetTextureMipmapMode(Cobalt_TextureSampler1D sampler, Cobalt_MipmapMode mipmapMode);
COBALT_FUNCTION_EXPORT void Cobalt_TextureSampler1D_SetMipmapLevelMapping(Cobalt_TextureSampler1D sampler, float minLevel, float maxLevel, float levelBias);

// Initialization methods
COBALT_FUNCTION_EXPORT void Cobalt_TextureSampler1D_Delete(Cobalt_TextureSampler1D sampler);

#ifdef __cplusplus
}
#endif
