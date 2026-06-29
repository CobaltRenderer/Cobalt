// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "ShaderSourceInfoBaseDXBC.h"
namespace cobalt { namespace graphics {

struct ShaderSourceInfoDXBC : public ShaderSourceInfoBaseDXBC
{
	ShaderSourceInfoDXBC(const uint8_t* shaderCode, size_t shaderCodeSizeInBytes)
	{
		code = shaderCode;
		codeSizeInBytes = shaderCodeSizeInBytes;
	}
};

}} // namespace cobalt::graphics
