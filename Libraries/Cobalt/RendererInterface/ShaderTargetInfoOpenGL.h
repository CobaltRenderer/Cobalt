// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "IShaderProgram.h"
namespace cobalt { namespace graphics {

struct ShaderTargetInfoOpenGL : public IShaderProgram::ShaderTargetInfoBase
{
	// Enumerations
	enum class Flags
	{
		None = 0x00,
		ForceGLSL = 0x01,
		ForceSPIRVIfAvailable = 0x02,
	};

	// Constructors
	explicit ShaderTargetInfoOpenGL(Flags flags = Flags::None)
	: ShaderTargetInfoBase(ShaderTarget::OpenGL), flags(flags)
	{}

	Flags flags;
};

}} // namespace cobalt::graphics
#include "ShaderTargetInfoOpenGL.inl"
