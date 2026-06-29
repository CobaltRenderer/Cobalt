// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "IShaderProgram.h"
namespace cobalt { namespace graphics {

struct ShaderSourceInfoBaseDXBC : public IShaderProgram::ShaderSourceInfoBase
{
	ShaderSourceInfoBaseDXBC()
	: ShaderSourceInfoBase(ShaderType::DXBC), code(nullptr), codeSizeInBytes(0)
	{}

	const uint8_t* code;
	size_t codeSizeInBytes;
};

}} // namespace cobalt::graphics
