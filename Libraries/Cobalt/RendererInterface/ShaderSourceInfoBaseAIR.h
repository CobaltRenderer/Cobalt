// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "IShaderProgram.h"
namespace cobalt { namespace graphics {

struct ShaderSourceInfoBaseAIR : public IShaderProgram::ShaderSourceInfoBase
{
	ShaderSourceInfoBaseAIR()
	: ShaderSourceInfoBase(ShaderType::AIR), code(nullptr), codeSizeInBytes(0), entryPointFunctionName(nullptr), entryPointFunctionNameSizeInBytes(0)
	{}

	const uint8_t* code;
	size_t codeSizeInBytes;
	const char* entryPointFunctionName;
	size_t entryPointFunctionNameSizeInBytes;
};

}} // namespace cobalt::graphics
