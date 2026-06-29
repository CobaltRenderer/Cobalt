// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Macros.h"
#include "TextureSampler.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Cobalt_TextureSampler2D_Internal* Cobalt_TextureSampler2D;

// Format methods
COBALT_FUNCTION_EXPORT void Cobalt_TextureSampler2D_SetTextureWrapMode(Cobalt_TextureSampler2D sampler, Cobalt_WrapMode wrapModeHorizontal, Cobalt_WrapMode wrapModeVertical);
COBALT_FUNCTION_EXPORT void Cobalt_TextureSampler2D_SetTextureFilterMode(Cobalt_TextureSampler2D sampler, Cobalt_FilterMode filterModeShrink, Cobalt_FilterMode filterModeExpand);
COBALT_FUNCTION_EXPORT void Cobalt_TextureSampler2D_SetTextureMipmapMode(Cobalt_TextureSampler2D sampler, Cobalt_MipmapMode mipmapMode);
COBALT_FUNCTION_EXPORT void Cobalt_TextureSampler2D_SetMipmapLevelMapping(Cobalt_TextureSampler2D sampler, float minLevel, float maxLevel, float levelBias);
COBALT_FUNCTION_EXPORT void Cobalt_TextureSampler2D_SetAnisotropicFilterMode(Cobalt_TextureSampler2D sampler, char enabled, int maxAnisotropy);

// Initialization methods
COBALT_FUNCTION_EXPORT void Cobalt_TextureSampler2D_Delete(Cobalt_TextureSampler2D sampler);

#ifdef __cplusplus
}
#endif
