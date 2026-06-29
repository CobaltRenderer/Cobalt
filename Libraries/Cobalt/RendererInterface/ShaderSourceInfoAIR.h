// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "ShaderSourceInfoBaseAIR.h"
namespace cobalt { namespace graphics {

struct ShaderSourceInfoAIR : public ShaderSourceInfoBaseAIR
{
	ShaderSourceInfoAIR(const uint8_t* shaderCode, size_t shaderCodeSizeInBytes)
	{
		code = shaderCode;
		codeSizeInBytes = shaderCodeSizeInBytes;
	}
	ShaderSourceInfoAIR(const uint8_t* shaderCode, size_t shaderCodeSizeInBytes, const char* entryPointName, size_t entryPointNameInChars)
	{
		code = shaderCode;
		codeSizeInBytes = shaderCodeSizeInBytes;
		entryPointFunctionName = entryPointName;
		entryPointFunctionNameSizeInBytes = entryPointNameInChars;
	}
	ShaderSourceInfoAIR(const uint8_t* shaderCode, size_t shaderCodeSizeInBytes, const std::string& entryPointName)
	: bufferedEntryPointFunctionName(entryPointName)
	{
		code = shaderCode;
		codeSizeInBytes = shaderCodeSizeInBytes;
		entryPointFunctionName = bufferedEntryPointFunctionName.c_str();
		entryPointFunctionNameSizeInBytes = bufferedEntryPointFunctionName.size();
	}
	ShaderSourceInfoAIR(const uint8_t* shaderCode, size_t shaderCodeSizeInBytes, std::string&& entryPointName)
	: bufferedEntryPointFunctionName(std::move(entryPointName))
	{
		code = shaderCode;
		codeSizeInBytes = shaderCodeSizeInBytes;
		entryPointFunctionName = bufferedEntryPointFunctionName.c_str();
		entryPointFunctionNameSizeInBytes = bufferedEntryPointFunctionName.size();
	}

private:
	std::string bufferedEntryPointFunctionName;
};

}} // namespace cobalt::graphics
