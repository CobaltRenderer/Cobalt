// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "IShaderProgram.h"
namespace cobalt { namespace graphics {

struct ShaderSourceInfoBaseGLSL : public IShaderProgram::ShaderSourceInfoBase
{
	ShaderSourceInfoBaseGLSL()
	: ShaderSourceInfoBase(ShaderType::GLSL), code(nullptr), codeSizeInBytes(0)
	{}

	const char* code;
	size_t codeSizeInBytes;
};

}} // namespace cobalt::graphics
