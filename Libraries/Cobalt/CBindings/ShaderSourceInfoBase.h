// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Macros.h"
#include <stddef.h>
#include <stdint.h>

typedef enum
{
	Cobalt_ShaderSourceInfoType_HLSL = 0x0100,          // Text
	Cobalt_ShaderSourceInfoType_DXBC = 0x0101,          // Bytecode HLSL (FXC)
	Cobalt_ShaderSourceInfoType_DXIL = 0x0102,          // Bytecode HLSL (DXC)
	Cobalt_ShaderSourceInfoType_SPIRVAssembly = 0x0200, // Text
	Cobalt_ShaderSourceInfoType_SPIRV = 0x0201,         // Bytecode SPIRV
	Cobalt_ShaderSourceInfoType_GLSL = 0x0300,          // Text
	Cobalt_ShaderSourceInfoType_MSL = 0x0400,           // Text
	Cobalt_ShaderSourceInfoType_AIR = 0x0401,           // Bytecode MSL
} Cobalt_ShaderSourceInfoType;

typedef struct Cobalt_ShaderSourceInfoBase
{
	Cobalt_ShaderSourceInfoType type;
} Cobalt_ShaderSourceInfoBase;
