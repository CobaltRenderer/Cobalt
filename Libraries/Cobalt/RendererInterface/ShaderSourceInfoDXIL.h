// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "ShaderSourceInfoBaseDXIL.h"
namespace cobalt { namespace graphics {

struct ShaderSourceInfoDXIL : public ShaderSourceInfoBaseDXIL
{
	ShaderSourceInfoDXIL(const uint8_t* shaderCode, size_t shaderCodeSizeInBytes)
	{
		code = shaderCode;
		codeSizeInBytes = shaderCodeSizeInBytes;
	}
};

}} // namespace cobalt::graphics
