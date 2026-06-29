// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Macros.h"
#include "ShaderSourceInfoBase.h"

typedef struct Cobalt_ShaderSourceInfoDXBC
{
	Cobalt_ShaderSourceInfoBase base;
	const uint8_t* code;
	size_t codeSizeInBytes;
} Cobalt_ShaderSourceInfoDXBC;
