// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "OpenGLTextureSampler.h"
namespace cobalt::graphics {

class OpenGLTextureSampler3D : public OpenGLTextureSampler<ITextureSampler3D>
{
public:
	// Constructors
	inline OpenGLTextureSampler3D(cobalt::logging::ILogger* log, OpenGLRenderer* renderer);

	// Initialization methods
	inline void Delete() final;

	// Format methods
	inline void SetTextureWrapMode(WrapMode wrapModeHorizontal, WrapMode wrapModeVertical, WrapMode wrapModeDepth) final;

	// Build state methods
	inline void FlagObjectModified() final;
};

} // namespace cobalt::graphics
#include "OpenGLTextureSampler3D.inl"
