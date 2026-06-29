// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Macros.h"
#include "ShaderTargetInfoBase.h"

typedef enum
{
	Cobalt_ShaderTargetInfoDirect3D_Flags_None = 0x00,
	Cobalt_ShaderTargetInfoDirect3D_Flags_ForceFXC = 0x01,
	Cobalt_ShaderTargetInfoDirect3D_Flags_SkipOptimization = 0x02,
	Cobalt_ShaderTargetInfoDirect3D_Flags_EnableDebugInfo = 0x04,
} Cobalt_ShaderTargetInfoDirect3D_Flags;

typedef struct Cobalt_ShaderTargetInfoDirect3D
{
	Cobalt_ShaderTargetInfoBase base;
	unsigned int targetShaderModelMajor;
	unsigned int targetShaderModelMinor;
	Cobalt_ShaderTargetInfoDirect3D_Flags flags;
} Cobalt_ShaderTargetInfoDirect3D;
