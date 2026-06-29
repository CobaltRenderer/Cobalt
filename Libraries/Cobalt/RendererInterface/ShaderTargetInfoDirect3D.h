// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "IShaderProgram.h"
namespace cobalt { namespace graphics {

struct ShaderTargetInfoDirect3D : public IShaderProgram::ShaderTargetInfoBase
{
	// Enumerations
	enum class Flags
	{
		None = 0x00,
		ForceFXC = 0x01,
		SkipOptimization = 0x02,
		EnableDebugInfo = 0x04,
	};

	// Constructors
	explicit ShaderTargetInfoDirect3D(Flags flags = Flags::None)
	: ShaderTargetInfoBase(ShaderTarget::Direct3D), targetShaderModelMajor(0), targetShaderModelMinor(0), flags(flags)
	{}
	ShaderTargetInfoDirect3D(unsigned int targetShaderModelMajor, unsigned int targetShaderModelMinor, Flags flags = Flags::None)
	: ShaderTargetInfoBase(ShaderTarget::Direct3D), targetShaderModelMajor(targetShaderModelMajor), targetShaderModelMinor(targetShaderModelMinor), flags(flags)
	{}

	unsigned int targetShaderModelMajor;
	unsigned int targetShaderModelMinor;
	Flags flags;
};

}} // namespace cobalt::graphics
#include "ShaderTargetInfoDirect3D.inl"
