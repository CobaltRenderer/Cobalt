// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "ShaderSourceInfoBaseSPIRVAssembly.h"
namespace cobalt { namespace graphics {

struct ShaderSourceInfoSPIRVAssembly : public ShaderSourceInfoBaseSPIRVAssembly
{
	ShaderSourceInfoSPIRVAssembly(const char* shaderCode, size_t shaderCodeLengthInChars)
	{
		code = shaderCode;
		codeSizeInBytes = shaderCodeLengthInChars;
	}
	explicit ShaderSourceInfoSPIRVAssembly(const std::string& shaderCode)
	: bufferedCode(shaderCode)
	{
		code = bufferedCode.c_str();
		codeSizeInBytes = bufferedCode.size();
	}
	explicit ShaderSourceInfoSPIRVAssembly(std::string&& shaderCode)
	: bufferedCode(std::move(shaderCode))
	{
		code = bufferedCode.c_str();
		codeSizeInBytes = bufferedCode.size();
	}
	ShaderSourceInfoSPIRVAssembly(const char* shaderCode, size_t shaderCodeLengthInChars, const char* entryPointName, size_t entryPointNameInChars)
	{
		code = shaderCode;
		codeSizeInBytes = shaderCodeLengthInChars;
		entryPointFunctionName = entryPointName;
		entryPointFunctionNameSizeInBytes = entryPointNameInChars;
	}
	ShaderSourceInfoSPIRVAssembly(const std::string& shaderCode, const std::string& entryPointName)
	: bufferedCode(shaderCode), bufferedEntryPointFunctionName(entryPointName)
	{
		code = bufferedCode.c_str();
		codeSizeInBytes = bufferedCode.size();
		entryPointFunctionName = bufferedEntryPointFunctionName.c_str();
		entryPointFunctionNameSizeInBytes = bufferedEntryPointFunctionName.size();
	}
	ShaderSourceInfoSPIRVAssembly(std::string&& shaderCode, const std::string& entryPointName)
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
