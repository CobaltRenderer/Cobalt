// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Macros.h"
#include "ShaderTargetInfoBase.h"

typedef enum
{
	Cobalt_ShaderTargetInfoOpenGL_Flags_None = 0x00,
	Cobalt_ShaderTargetInfoOpenGL_Flags_ForceGLSL = 0x01,
	Cobalt_ShaderTargetInfoOpenGL_Flags_ForceSPIRVIfAvailable = 0x02,
} Cobalt_ShaderTargetInfoOpenGL_Flags;

typedef struct Cobalt_ShaderTargetInfoOpenGL
{
	Cobalt_ShaderTargetInfoBase base;
	Cobalt_ShaderTargetInfoOpenGL_Flags flags;
} Cobalt_ShaderTargetInfoOpenGL;
