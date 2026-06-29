// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "ShaderProgram.h"
#include <Cobalt/RendererInterface/RendererInterface.pkg>

using namespace cobalt::graphics;

//----------------------------------------------------------------------------------------
// Code format methods
//----------------------------------------------------------------------------------------
char Cobalt_ShaderProgram_IsCodeFormatSupported(Cobalt_ShaderProgram shaderProgram, Cobalt_CodeFormat format)
{
	auto _this = reinterpret_cast<IShaderProgram*>(shaderProgram);
	auto _format = (IShaderProgram::CodeFormat)format;

	return _this->IsCodeFormatSupported(_format) ? 1 : 0;
}

//----------------------------------------------------------------------------------------
Cobalt_CodeFormat Cobalt_ShaderProgram_PreferredCodeFormat(Cobalt_ShaderProgram shaderProgram)
{
	auto _this = reinterpret_cast<IShaderProgram*>(shaderProgram);

	return (Cobalt_CodeFormat)_this->PreferredCodeFormat();
}

//----------------------------------------------------------------------------------------
// Compilation methods
//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_ShaderProgram_ConfigureShaderTarget(Cobalt_ShaderProgram shaderProgram, const Cobalt_ShaderTargetInfoBase* shaderTargetInfo)
{
	auto _this = reinterpret_cast<IShaderProgram*>(shaderProgram);

	bool result = false;
	switch (shaderTargetInfo->type)
	{
	case Cobalt_ShaderTargetInfoType_Direct3D:
	{
		auto shaderTargetInfoResolved = reinterpret_cast<const Cobalt_ShaderTargetInfoDirect3D*>(shaderTargetInfo);
		result = _this->ConfigureShaderTarget(ShaderTargetInfoDirect3D(shaderTargetInfoResolved->targetShaderModelMajor, shaderTargetInfoResolved->targetShaderModelMinor, (ShaderTargetInfoDirect3D::Flags)shaderTargetInfoResolved->flags));
		break;
	}
	case Cobalt_ShaderTargetInfoType_OpenGL:
	{
		auto shaderTargetInfoResolved = reinterpret_cast<const Cobalt_ShaderTargetInfoOpenGL*>(shaderTargetInfo);
		result = _this->ConfigureShaderTarget(ShaderTargetInfoOpenGL((ShaderTargetInfoOpenGL::Flags)shaderTargetInfoResolved->flags));
		break;
	}
	}
	return result ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_ShaderProgram_LoadShaderStage(Cobalt_ShaderProgram shaderProgram, Cobalt_ShaderStage stage, const Cobalt_ShaderSourceInfoBase* shaderSourceInfo)
{
	auto _this = reinterpret_cast<IShaderProgram*>(shaderProgram);
	auto _stage = (IShaderProgram::ShaderStage)stage;

	bool result = false;
	switch (shaderSourceInfo->type)
	{
	case Cobalt_ShaderSourceInfoType_HLSL:
	{
		auto shaderSourceInfoResolved = reinterpret_cast<const Cobalt_ShaderSourceInfoHLSL*>(shaderSourceInfo);
		result = _this->LoadShaderStage(_stage, ShaderSourceInfoHLSL(shaderSourceInfoResolved->code, shaderSourceInfoResolved->codeSizeInBytes, shaderSourceInfoResolved->entryPointFunctionName, shaderSourceInfoResolved->entryPointFunctionNameSizeInBytes));
		break;
	}
	case Cobalt_ShaderSourceInfoType_DXBC:
	{
		auto shaderSourceInfoResolved = reinterpret_cast<const Cobalt_ShaderSourceInfoDXBC*>(shaderSourceInfo);
		result = _this->LoadShaderStage(_stage, ShaderSourceInfoDXBC(shaderSourceInfoResolved->code, shaderSourceInfoResolved->codeSizeInBytes));
		break;
	}
	case Cobalt_ShaderSourceInfoType_DXIL:
	{
		auto shaderSourceInfoResolved = reinterpret_cast<const Cobalt_ShaderSourceInfoDXIL*>(shaderSourceInfo);
		result = _this->LoadShaderStage(_stage, ShaderSourceInfoDXIL(shaderSourceInfoResolved->code, shaderSourceInfoResolved->codeSizeInBytes));
		break;
	}
	case Cobalt_ShaderSourceInfoType_SPIRVAssembly:
	{
		auto shaderSourceInfoResolved = reinterpret_cast<const Cobalt_ShaderSourceInfoSPIRVAssembly*>(shaderSourceInfo);
		result = _this->LoadShaderStage(_stage, ShaderSourceInfoSPIRVAssembly(shaderSourceInfoResolved->code, shaderSourceInfoResolved->codeSizeInBytes, shaderSourceInfoResolved->entryPointFunctionName, shaderSourceInfoResolved->entryPointFunctionNameSizeInBytes));
		break;
	}
	case Cobalt_ShaderSourceInfoType_SPIRV:
	{
		auto shaderSourceInfoResolved = reinterpret_cast<const Cobalt_ShaderSourceInfoSPIRV*>(shaderSourceInfo);
		result = _this->LoadShaderStage(_stage, ShaderSourceInfoSPIRV(shaderSourceInfoResolved->code, shaderSourceInfoResolved->codeSizeInUnits, shaderSourceInfoResolved->entryPointFunctionName, shaderSourceInfoResolved->entryPointFunctionNameSizeInBytes));
		break;
	}
	case Cobalt_ShaderSourceInfoType_GLSL:
	{
		auto shaderSourceInfoResolved = reinterpret_cast<const Cobalt_ShaderSourceInfoGLSL*>(shaderSourceInfo);
		result = _this->LoadShaderStage(_stage, ShaderSourceInfoGLSL(shaderSourceInfoResolved->code, shaderSourceInfoResolved->codeSizeInBytes));
		break;
	}
	case Cobalt_ShaderSourceInfoType_MSL:
	{
		auto shaderSourceInfoResolved = reinterpret_cast<const Cobalt_ShaderSourceInfoMSL*>(shaderSourceInfo);
		result = _this->LoadShaderStage(_stage, ShaderSourceInfoMSL(shaderSourceInfoResolved->code, shaderSourceInfoResolved->codeSizeInBytes, shaderSourceInfoResolved->entryPointFunctionName, shaderSourceInfoResolved->entryPointFunctionNameSizeInBytes));
		break;
	}
	case Cobalt_ShaderSourceInfoType_AIR:
	{
		auto shaderSourceInfoResolved = reinterpret_cast<const Cobalt_ShaderSourceInfoAIR*>(shaderSourceInfo);
		result = _this->LoadShaderStage(_stage, ShaderSourceInfoAIR(shaderSourceInfoResolved->code, shaderSourceInfoResolved->codeSizeInBytes, shaderSourceInfoResolved->entryPointFunctionName, shaderSourceInfoResolved->entryPointFunctionNameSizeInBytes));
		break;
	}
	}
	return result ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_ShaderProgram_CompileProgram(Cobalt_ShaderProgram shaderProgram)
{
	auto _this = reinterpret_cast<IShaderProgram*>(shaderProgram);

	return _this->CompileProgram() ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
// Shader input methods
//----------------------------------------------------------------------------------------
char Cobalt_ShaderProgram_VertexAttributeExists(Cobalt_ShaderProgram shaderProgram, const char* name, size_t nameLength)
{
	auto _this = reinterpret_cast<IShaderProgram*>(shaderProgram);
	auto _name = std::string(name, nameLength);

	return _this->VertexAttributeExists(_name) ? 1 : 0;
}

//----------------------------------------------------------------------------------------
char Cobalt_ShaderProgram_StateValueExists(Cobalt_ShaderProgram shaderProgram, const char* name, size_t nameLength)
{
	auto _this = reinterpret_cast<IShaderProgram*>(shaderProgram);
	auto _name = std::string(name, nameLength);

	return _this->StateValueExists(_name) ? 1 : 0;
}

//----------------------------------------------------------------------------------------
char Cobalt_ShaderProgram_TextureExists(Cobalt_ShaderProgram shaderProgram, const char* name, size_t nameLength)
{
	auto _this = reinterpret_cast<IShaderProgram*>(shaderProgram);
	auto _name = std::string(name, nameLength);

	return _this->TextureExists(_name) ? 1 : 0;
}

//----------------------------------------------------------------------------------------
char Cobalt_ShaderProgram_SamplerExists(Cobalt_ShaderProgram shaderProgram, const char* name, size_t nameLength)
{
	auto _this = reinterpret_cast<IShaderProgram*>(shaderProgram);
	auto _name = std::string(name, nameLength);

	return _this->SamplerExists(_name) ? 1 : 0;
}

//----------------------------------------------------------------------------------------
char Cobalt_ShaderProgram_StateBufferExists(Cobalt_ShaderProgram shaderProgram, const char* name, size_t nameLength)
{
	auto _this = reinterpret_cast<IShaderProgram*>(shaderProgram);
	auto _name = std::string(name, nameLength);

	return _this->StateBufferExists(_name) ? 1 : 0;
}

//----------------------------------------------------------------------------------------
char Cobalt_ShaderProgram_ResourceArrayExists(Cobalt_ShaderProgram shaderProgram, const char* name, size_t nameLength)
{
	auto _this = reinterpret_cast<IShaderProgram*>(shaderProgram);
	auto _name = std::string(name, nameLength);

	return _this->ResourceArrayExists(_name) ? 1 : 0;
}

//----------------------------------------------------------------------------------------
uint32_t Cobalt_ShaderProgram_GetVertexAttributeId(Cobalt_ShaderProgram shaderProgram, const char* name, size_t nameLength)
{
	auto _this = reinterpret_cast<IShaderProgram*>(shaderProgram);
	auto _name = std::string(name, nameLength);

	return (uint32_t)_this->GetVertexAttributeId(_name);
}

//----------------------------------------------------------------------------------------
uint32_t Cobalt_ShaderProgram_GetStateValueId(Cobalt_ShaderProgram shaderProgram, const char* name, size_t nameLength)
{
	auto _this = reinterpret_cast<IShaderProgram*>(shaderProgram);
	auto _name = std::string(name, nameLength);

	return (uint32_t)_this->GetStateValueId(_name);
}

//----------------------------------------------------------------------------------------
uint32_t Cobalt_ShaderProgram_GetTextureId(Cobalt_ShaderProgram shaderProgram, const char* name, size_t nameLength)
{
	auto _this = reinterpret_cast<IShaderProgram*>(shaderProgram);
	auto _name = std::string(name, nameLength);

	return (uint32_t)_this->GetTextureId(_name);
}

//----------------------------------------------------------------------------------------
uint32_t Cobalt_ShaderProgram_GetSamplerId(Cobalt_ShaderProgram shaderProgram, const char* name, size_t nameLength)
{
	auto _this = reinterpret_cast<IShaderProgram*>(shaderProgram);
	auto _name = std::string(name, nameLength);

	return (uint32_t)_this->GetSamplerId(_name);
}

//----------------------------------------------------------------------------------------
uint32_t Cobalt_ShaderProgram_GetStateBufferId(Cobalt_ShaderProgram shaderProgram, const char* name, size_t nameLength)
{
	auto _this = reinterpret_cast<IShaderProgram*>(shaderProgram);
	auto _name = std::string(name, nameLength);

	return (uint32_t)_this->GetStateBufferId(_name);
}

//----------------------------------------------------------------------------------------
uint32_t Cobalt_ShaderProgram_GetResourceArrayId(Cobalt_ShaderProgram shaderProgram, const char* name, size_t nameLength)
{
	auto _this = reinterpret_cast<IShaderProgram*>(shaderProgram);
	auto _name = std::string(name, nameLength);

	return (uint32_t)_this->GetResourceArrayId(_name);
}

//----------------------------------------------------------------------------------------
// State buffer methods
//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_ShaderProgram_LoadStateBufferLayoutFromShader(Cobalt_ShaderProgram shaderProgram, uint32_t stateBufferId, Cobalt_StateBufferLayout stateBufferLayout)
{
	auto _this = reinterpret_cast<IShaderProgram*>(shaderProgram);

	return _this->LoadStateBufferLayoutFromShader((StateBufferId)stateBufferId, reinterpret_cast<IStateBufferLayout*>(stateBufferLayout)) ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void Cobalt_ShaderProgram_Delete(Cobalt_ShaderProgram shaderProgram)
{
	auto _this = reinterpret_cast<IShaderProgram*>(shaderProgram);

	_this->Delete();
}
