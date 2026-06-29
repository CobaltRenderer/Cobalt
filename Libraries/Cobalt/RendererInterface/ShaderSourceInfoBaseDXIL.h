// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "IShaderProgram.h"
namespace cobalt { namespace graphics {

struct ShaderSourceInfoBaseDXIL : public IShaderProgram::ShaderSourceInfoBase
{
	ShaderSourceInfoBaseDXIL()
	: ShaderSourceInfoBase(ShaderType::DXIL), code(nullptr), codeSizeInBytes(0)
	{}

	const uint8_t* code;
	size_t codeSizeInBytes;
};

}} // namespace cobalt::graphics
