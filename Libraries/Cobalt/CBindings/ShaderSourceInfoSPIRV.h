// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Macros.h"
#include "ShaderSourceInfoBase.h"

typedef struct Cobalt_ShaderSourceInfoSPIRV
{
	Cobalt_ShaderSourceInfoBase base;
	const uint32_t* code;
	size_t codeSizeInUnits;
	const char* entryPointFunctionName;
	size_t entryPointFunctionNameSizeInBytes;
} Cobalt_ShaderSourceInfoSPIRV;
