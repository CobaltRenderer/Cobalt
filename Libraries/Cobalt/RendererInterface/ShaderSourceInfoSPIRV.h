// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "ShaderSourceInfoBaseSPIRV.h"
namespace cobalt { namespace graphics {

struct ShaderSourceInfoSPIRV : public ShaderSourceInfoBaseSPIRV
{
	ShaderSourceInfoSPIRV(const uint32_t* shaderCode, size_t shaderCodeSizeInUnits)
	{
		code = shaderCode;
		codeSizeInUnits = shaderCodeSizeInUnits;
	}
	ShaderSourceInfoSPIRV(const uint32_t* shaderCode, size_t shaderCodeSizeInUnits, const char* entryPointName, size_t entryPointNameInChars)
	{
		code = shaderCode;
		codeSizeInUnits = shaderCodeSizeInUnits;
		entryPointFunctionName = entryPointName;
		entryPointFunctionNameSizeInBytes = entryPointNameInChars;
	}
	ShaderSourceInfoSPIRV(const uint32_t* shaderCode, size_t shaderCodeSizeInUnits, const std::string& entryPointName)
	: bufferedEntryPointFunctionName(entryPointName)
	{
		code = shaderCode;
		codeSizeInUnits = shaderCodeSizeInUnits;
		entryPointFunctionName = bufferedEntryPointFunctionName.c_str();
		entryPointFunctionNameSizeInBytes = bufferedEntryPointFunctionName.size();
	}
	ShaderSourceInfoSPIRV(const uint32_t* shaderCode, size_t shaderCodeSizeInUnits, std::string&& entryPointName)
	: bufferedEntryPointFunctionName(std::move(entryPointName))
	{
		code = shaderCode;
		codeSizeInUnits = shaderCodeSizeInUnits;
		entryPointFunctionName = bufferedEntryPointFunctionName.c_str();
		entryPointFunctionNameSizeInBytes = bufferedEntryPointFunctionName.size();
	}

private:
	std::string bufferedEntryPointFunctionName;
};

}} // namespace cobalt::graphics
