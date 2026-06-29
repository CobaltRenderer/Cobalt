// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Macros.h"
#include "Result.h"
#include "ShaderSourceInfoAIR.h"
#include "ShaderSourceInfoBase.h"
#include "ShaderSourceInfoDXBC.h"
#include "ShaderSourceInfoDXIL.h"
#include "ShaderSourceInfoGLSL.h"
#include "ShaderSourceInfoHLSL.h"
#include "ShaderSourceInfoMSL.h"
#include "ShaderSourceInfoSPIRV.h"
#include "ShaderSourceInfoSPIRVAssembly.h"
#include "ShaderTargetInfoBase.h"
#include "ShaderTargetInfoDirect3D.h"
#include "ShaderTargetInfoOpenGL.h"
#include "StateBufferLayout.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Cobalt_ShaderProgram_Internal* Cobalt_ShaderProgram;

// Enumerations
typedef enum
{
	Cobalt_ShaderStage_Vertex = 0x01,
	Cobalt_ShaderStage_Fragment = 0x02,
	Cobalt_ShaderStage_Geometry = 0x04,
	Cobalt_ShaderStage_Compute = 0x08,
} Cobalt_ShaderStage;

typedef enum
{
	Cobalt_CodeFormat_HLSL = Cobalt_ShaderSourceInfoType_HLSL,
	Cobalt_CodeFormat_DXBC = Cobalt_ShaderSourceInfoType_DXBC,
	Cobalt_CodeFormat_DXIL = Cobalt_ShaderSourceInfoType_DXIL,
	Cobalt_CodeFormat_SPIRVAssembly = Cobalt_ShaderSourceInfoType_SPIRVAssembly,
	Cobalt_CodeFormat_SPIRV = Cobalt_ShaderSourceInfoType_SPIRV,
	Cobalt_CodeFormat_GLSL = Cobalt_ShaderSourceInfoType_GLSL,
	Cobalt_CodeFormat_MSL = Cobalt_ShaderSourceInfoType_MSL,
	Cobalt_CodeFormat_AIR = Cobalt_ShaderSourceInfoType_AIR,
} Cobalt_CodeFormat;

// Code format methods
COBALT_FUNCTION_EXPORT char Cobalt_ShaderProgram_IsCodeFormatSupported(Cobalt_ShaderProgram shaderProgram, Cobalt_CodeFormat format);
COBALT_FUNCTION_EXPORT Cobalt_CodeFormat Cobalt_ShaderProgram_PreferredCodeFormat(Cobalt_ShaderProgram shaderProgram);

// Compilation methods
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_ShaderProgram_ConfigureShaderTarget(Cobalt_ShaderProgram shaderProgram, const Cobalt_ShaderTargetInfoBase* shaderTargetInfo);
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_ShaderProgram_LoadShaderStage(Cobalt_ShaderProgram shaderProgram, Cobalt_ShaderStage stage, const Cobalt_ShaderSourceInfoBase* shaderSourceInfo);
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_ShaderProgram_CompileProgram(Cobalt_ShaderProgram shaderProgram);

// Shader input methods
COBALT_FUNCTION_EXPORT char Cobalt_ShaderProgram_VertexAttributeExists(Cobalt_ShaderProgram shaderProgram, const char* name, size_t nameLength);
COBALT_FUNCTION_EXPORT char Cobalt_ShaderProgram_StateValueExists(Cobalt_ShaderProgram shaderProgram, const char* name, size_t nameLength);
COBALT_FUNCTION_EXPORT char Cobalt_ShaderProgram_TextureExists(Cobalt_ShaderProgram shaderProgram, const char* name, size_t nameLength);
COBALT_FUNCTION_EXPORT char Cobalt_ShaderProgram_SamplerExists(Cobalt_ShaderProgram shaderProgram, const char* name, size_t nameLength);
COBALT_FUNCTION_EXPORT char Cobalt_ShaderProgram_StateBufferExists(Cobalt_ShaderProgram shaderProgram, const char* name, size_t nameLength);
COBALT_FUNCTION_EXPORT char Cobalt_ShaderProgram_ResourceArrayExists(Cobalt_ShaderProgram shaderProgram, const char* name, size_t nameLength);
COBALT_FUNCTION_EXPORT uint32_t Cobalt_ShaderProgram_GetVertexAttributeId(Cobalt_ShaderProgram shaderProgram, const char* name, size_t nameLength);
COBALT_FUNCTION_EXPORT uint32_t Cobalt_ShaderProgram_GetStateValueId(Cobalt_ShaderProgram shaderProgram, const char* name, size_t nameLength);
COBALT_FUNCTION_EXPORT uint32_t Cobalt_ShaderProgram_GetTextureId(Cobalt_ShaderProgram shaderProgram, const char* name, size_t nameLength);
COBALT_FUNCTION_EXPORT uint32_t Cobalt_ShaderProgram_GetSamplerId(Cobalt_ShaderProgram shaderProgram, const char* name, size_t nameLength);
COBALT_FUNCTION_EXPORT uint32_t Cobalt_ShaderProgram_GetStateBufferId(Cobalt_ShaderProgram shaderProgram, const char* name, size_t nameLength);
COBALT_FUNCTION_EXPORT uint32_t Cobalt_ShaderProgram_GetResourceArrayId(Cobalt_ShaderProgram shaderProgram, const char* name, size_t nameLength);

// State buffer methods
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_ShaderProgram_LoadStateBufferLayoutFromShader(Cobalt_ShaderProgram shaderProgram, uint32_t stateBufferId, Cobalt_StateBufferLayout stateBufferLayout);

// Initialization methods
COBALT_FUNCTION_EXPORT void Cobalt_ShaderProgram_Delete(Cobalt_ShaderProgram shaderProgram);

#ifdef __cplusplus
}
#endif
