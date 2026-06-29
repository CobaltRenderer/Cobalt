// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Deleter.h"
#include "SuccessToken.h"
#include "Tokens.h"
#include <Cobalt/Marshalling/Marshalling.pkg>
#include <memory>
#include <string>
namespace cobalt { namespace graphics {
using namespace cobalt::marshalling::operators;
class IStateBufferLayout;

class IShaderProgram
{
public:
	// Enumerations
	enum class ShaderStage
	{
		Vertex = 0x01,
		Fragment = 0x02,
		Geometry = 0x04,
		Compute = 0x08,
	};
	enum class CodeFormat
	{
		HLSL = 0x0100,          // Text
		DXBC = 0x0101,          // Bytecode HLSL (FXC)
		DXIL = 0x0102,          // Bytecode HLSL (DXC)
		SPIRVAssembly = 0x0200, // Text
		SPIRV = 0x0201,         // Bytecode SPIRV
		GLSL = 0x0300,          // Text
		MSL = 0x0400,           // Text
		AIR = 0x0401,           // Bytecode MSL
	};

	// Structures
	struct ShaderSourceInfoBase
	{
		enum class ShaderType
		{
			HLSL = (int)CodeFormat::HLSL,
			DXBC = (int)CodeFormat::DXBC,
			DXIL = (int)CodeFormat::DXIL,
			SPIRVAssembly = (int)CodeFormat::SPIRVAssembly,
			SPIRV = (int)CodeFormat::SPIRV,
			GLSL = (int)CodeFormat::GLSL,
			MSL = (int)CodeFormat::MSL,
			AIR = (int)CodeFormat::AIR,
		};

		ShaderType shaderType;

	protected:
		explicit ShaderSourceInfoBase(ShaderType shaderType)
		: shaderType(shaderType)
		{}
	};
	struct ShaderTargetInfoBase
	{
		enum class ShaderTarget
		{
			Direct3D = 0x01,
			OpenGL = 0x02,
			Vulkan = 0x03,
			Metal = 0x04,
		};

		ShaderTarget shaderTarget;

	protected:
		explicit ShaderTargetInfoBase(ShaderTarget shaderTarget)
		: shaderTarget(shaderTarget)
		{}
	};

	// Typedefs
	typedef std::unique_ptr<IShaderProgram, Deleter<IShaderProgram>> unique_ptr;

public:
	// Initialization methods
	virtual void Delete() = 0;

	// Code format methods
	virtual bool IsCodeFormatSupported(CodeFormat format) const = 0;
	virtual CodeFormat PreferredCodeFormat() const = 0;

	// Compilation methods
	virtual SuccessToken ConfigureShaderTarget(const ShaderTargetInfoBase& shaderTargetInfo) = 0;
	virtual SuccessToken LoadShaderStage(ShaderStage stage, const ShaderSourceInfoBase& shaderSourceInfo) = 0;
	virtual SuccessToken CompileProgram() = 0;

	// Shader input methods
	virtual bool VertexAttributeExists(const Marshal::In<std::string>& name) const = 0;
	virtual bool StateValueExists(const Marshal::In<std::string>& name) const = 0;
	virtual bool TextureExists(const Marshal::In<std::string>& name) const = 0;
	virtual bool SamplerExists(const Marshal::In<std::string>& name) const = 0;
	virtual bool StateBufferExists(const Marshal::In<std::string>& name) const = 0;
	virtual bool ResourceArrayExists(const Marshal::In<std::string>& name) const = 0;
	virtual VertexAttributeId GetVertexAttributeId(const Marshal::In<std::string>& name) const = 0;
	virtual StateValueId GetStateValueId(const Marshal::In<std::string>& name) const = 0;
	virtual TextureId GetTextureId(const Marshal::In<std::string>& name) const = 0;
	virtual SamplerId GetSamplerId(const Marshal::In<std::string>& name) const = 0;
	virtual StateBufferId GetStateBufferId(const Marshal::In<std::string>& name) const = 0;
	virtual ResourceArrayId GetResourceArrayId(const Marshal::In<std::string>& name) const = 0;

	// State buffer methods
	virtual SuccessToken LoadStateBufferLayoutFromShader(StateBufferId stateBufferId, IStateBufferLayout* stateBufferLayout) const = 0;

protected:
	// Constructors
	~IShaderProgram() = default;
};

}} // namespace cobalt::graphics
