// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Macros.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Cobalt_TextureSampler_Internal* Cobalt_TextureSampler;

// Enumerations
typedef enum
{
	Cobalt_WrapMode_ClampToEdge,
	Cobalt_WrapMode_Repeat,
	Cobalt_WrapMode_RepeatMirrored,
} Cobalt_WrapMode;

typedef enum
{
	Cobalt_FilterMode_Nearest,
	Cobalt_FilterMode_Linear,
} Cobalt_FilterMode;

typedef enum
{
	Cobalt_MipmapMode_None,
	Cobalt_MipmapMode_Nearest,
	Cobalt_MipmapMode_Linear,
} Cobalt_MipmapMode;

#ifdef __cplusplus
}
#endif
