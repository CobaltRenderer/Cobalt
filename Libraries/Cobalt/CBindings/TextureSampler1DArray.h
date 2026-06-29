// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Macros.h"
#include "TextureSampler.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Cobalt_TextureSampler1DArray_Internal* Cobalt_TextureSampler1DArray;

// Format methods
COBALT_FUNCTION_EXPORT void Cobalt_TextureSampler1DArray_SetTextureWrapMode(Cobalt_TextureSampler1DArray sampler, Cobalt_WrapMode wrapModeHorizontal);
COBALT_FUNCTION_EXPORT void Cobalt_TextureSampler1DArray_SetTextureFilterMode(Cobalt_TextureSampler1DArray sampler, Cobalt_FilterMode filterModeShrink, Cobalt_FilterMode filterModeExpand);
COBALT_FUNCTION_EXPORT void Cobalt_TextureSampler1DArray_SetTextureMipmapMode(Cobalt_TextureSampler1DArray sampler, Cobalt_MipmapMode mipmapMode);
COBALT_FUNCTION_EXPORT void Cobalt_TextureSampler1DArray_SetMipmapLevelMapping(Cobalt_TextureSampler1DArray sampler, float minLevel, float maxLevel, float levelBias);

// Initialization methods
COBALT_FUNCTION_EXPORT void Cobalt_TextureSampler1DArray_Delete(Cobalt_TextureSampler1DArray sampler);

#ifdef __cplusplus
}
#endif
