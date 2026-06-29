// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "IShaderProgram.h"
namespace cobalt { namespace graphics {

struct ShaderSourceInfoBaseSPIRV : public IShaderProgram::ShaderSourceInfoBase
{
	ShaderSourceInfoBaseSPIRV()
	: ShaderSourceInfoBase(ShaderType::SPIRV), code(nullptr), codeSizeInUnits(0), entryPointFunctionName(nullptr), entryPointFunctionNameSizeInBytes(0)
	{}

	const uint32_t* code;
	size_t codeSizeInUnits;
	const char* entryPointFunctionName;
	size_t entryPointFunctionNameSizeInBytes;
};

}} // namespace cobalt::graphics
