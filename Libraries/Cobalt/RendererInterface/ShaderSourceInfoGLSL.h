// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "ShaderSourceInfoBaseGLSL.h"
namespace cobalt { namespace graphics {

struct ShaderSourceInfoGLSL : public ShaderSourceInfoBaseGLSL
{
	ShaderSourceInfoGLSL(const char* shaderCode, size_t shaderCodeLengthInChars)
	{
		code = shaderCode;
		codeSizeInBytes = shaderCodeLengthInChars;
	}
	explicit ShaderSourceInfoGLSL(const std::string& shaderCode)
	: bufferedCode(shaderCode)
	{
		code = bufferedCode.c_str();
		codeSizeInBytes = bufferedCode.size();
	}
	explicit ShaderSourceInfoGLSL(std::string&& shaderCode)
	: bufferedCode(std::move(shaderCode))
	{
		code = bufferedCode.c_str();
		codeSizeInBytes = bufferedCode.size();
	}

private:
	std::string bufferedCode;
};

}} // namespace cobalt::graphics
