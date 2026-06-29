// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Macros.h"
#include <stdint.h>

typedef enum
{
	Cobalt_ShaderTargetInfoType_Direct3D = 0x01,
	Cobalt_ShaderTargetInfoType_OpenGL = 0x02,
	Cobalt_ShaderTargetInfoType_Vulkan = 0x03,
	Cobalt_ShaderTargetInfoType_Metal = 0x04,
} Cobalt_ShaderTargetInfoType;

typedef struct Cobalt_ShaderTargetInfoBase
{
	Cobalt_ShaderTargetInfoType type;
} Cobalt_ShaderTargetInfoBase;
