// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Macros.h"
#include "TextureSampler.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Cobalt_TextureSampler3D_Internal* Cobalt_TextureSampler3D;

// Format methods
COBALT_FUNCTION_EXPORT void Cobalt_TextureSampler3D_SetTextureWrapMode(Cobalt_TextureSampler3D sampler, Cobalt_WrapMode wrapModeHorizontal, Cobalt_WrapMode wrapModeVertical, Cobalt_WrapMode wrapModeDepth);
COBALT_FUNCTION_EXPORT void Cobalt_TextureSampler3D_SetTextureFilterMode(Cobalt_TextureSampler3D sampler, Cobalt_FilterMode filterModeShrink, Cobalt_FilterMode filterModeExpand);
COBALT_FUNCTION_EXPORT void Cobalt_TextureSampler3D_SetTextureMipmapMode(Cobalt_TextureSampler3D sampler, Cobalt_MipmapMode mipmapMode);
COBALT_FUNCTION_EXPORT void Cobalt_TextureSampler3D_SetMipmapLevelMapping(Cobalt_TextureSampler3D sampler, float minLevel, float maxLevel, float levelBias);

// Initialization methods
COBALT_FUNCTION_EXPORT void Cobalt_TextureSampler3D_Delete(Cobalt_TextureSampler3D sampler);

#ifdef __cplusplus
}
#endif
