// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "ShaderSourceInfoBaseHLSL.h"
namespace cobalt { namespace graphics {

struct ShaderSourceInfoHLSL : public ShaderSourceInfoBaseHLSL
{
	ShaderSourceInfoHLSL(const char* shaderCode, size_t shaderCodeLengthInChars)
	{
		code = shaderCode;
		codeSizeInBytes = shaderCodeLengthInChars;
	}
	explicit ShaderSourceInfoHLSL(const std::string& shaderCode)
	: bufferedCode(shaderCode)
	{
		code = bufferedCode.c_str();
		codeSizeInBytes = bufferedCode.size();
	}
	explicit ShaderSourceInfoHLSL(std::string&& shaderCode)
	: bufferedCode(std::move(shaderCode))
	{
		code = bufferedCode.c_str();
		codeSizeInBytes = bufferedCode.size();
	}
	ShaderSourceInfoHLSL(const char* shaderCode, size_t shaderCodeLengthInChars, const char* entryPointName, size_t entryPointNameInChars)
	{
		code = shaderCode;
		codeSizeInBytes = shaderCodeLengthInChars;
		entryPointFunctionName = entryPointName;
		entryPointFunctionNameSizeInBytes = entryPointNameInChars;
	}
	ShaderSourceInfoHLSL(const std::string& shaderCode, const std::string& entryPointName)
	: bufferedCode(shaderCode), bufferedEntryPointFunctionName(entryPointName)
	{
		code = bufferedCode.c_str();
		codeSizeInBytes = bufferedCode.size();
		entryPointFunctionName = bufferedEntryPointFunctionName.c_str();
		entryPointFunctionNameSizeInBytes = bufferedEntryPointFunctionName.size();
	}
	ShaderSourceInfoHLSL(std::string&& shaderCode, const std::string& entryPointName)
	: bufferedCode(std::move(shaderCode)), bufferedEntryPointFunctionName(entryPointName)
	{
		code = bufferedCode.c_str();
		codeSizeInBytes = bufferedCode.size();
		entryPointFunctionName = bufferedEntryPointFunctionName.c_str();
		entryPointFunctionNameSizeInBytes = bufferedEntryPointFunctionName.size();
	}

private:
	std::string bufferedCode;
	std::string bufferedEntryPointFunctionName;
};

}} // namespace cobalt::graphics
