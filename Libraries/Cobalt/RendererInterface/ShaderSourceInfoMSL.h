// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "ShaderSourceInfoBaseMSL.h"
namespace cobalt { namespace graphics {

struct ShaderSourceInfoMSL : public ShaderSourceInfoBaseMSL
{
	ShaderSourceInfoMSL(const char* shaderCode, size_t shaderCodeLengthInChars)
	{
		code = shaderCode;
		codeSizeInBytes = shaderCodeLengthInChars;
	}
	explicit ShaderSourceInfoMSL(const std::string& shaderCode)
	: bufferedCode(shaderCode)
	{
		code = bufferedCode.c_str();
		codeSizeInBytes = bufferedCode.size();
	}
	explicit ShaderSourceInfoMSL(std::string&& shaderCode)
	: bufferedCode(std::move(shaderCode))
	{
		code = bufferedCode.c_str();
		codeSizeInBytes = bufferedCode.size();
	}
	ShaderSourceInfoMSL(const char* shaderCode, size_t shaderCodeLengthInChars, const char* entryPointName, size_t entryPointNameInChars)
	{
		code = shaderCode;
		codeSizeInBytes = shaderCodeLengthInChars;
		entryPointFunctionName = entryPointName;
		entryPointFunctionNameSizeInBytes = entryPointNameInChars;
	}
	ShaderSourceInfoMSL(const std::string& shaderCode, const std::string& entryPointName)
	: bufferedCode(shaderCode), bufferedEntryPointFunctionName(entryPointName)
	{
		code = bufferedCode.c_str();
		codeSizeInBytes = bufferedCode.size();
		entryPointFunctionName = bufferedEntryPointFunctionName.c_str();
		entryPointFunctionNameSizeInBytes = bufferedEntryPointFunctionName.size();
	}
	ShaderSourceInfoMSL(std::string&& shaderCode, const std::string& entryPointName)
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
