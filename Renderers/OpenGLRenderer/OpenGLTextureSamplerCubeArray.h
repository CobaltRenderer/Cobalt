// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "OpenGLTextureSampler.h"
namespace cobalt::graphics {

class OpenGLTextureSamplerCubeArray : public OpenGLTextureSampler<ITextureSamplerCubeArray>
{
public:
	// Constructors
	inline OpenGLTextureSamplerCubeArray(cobalt::logging::ILogger* log, OpenGLRenderer* renderer);

	// Initialization methods
	inline void Delete() final;

	// Format methods
	inline void SetTextureWrapMode(WrapMode wrapModeHorizontal, WrapMode wrapModeVertical) final;

	// Build state methods
	inline void FlagObjectModified() final;
};

} // namespace cobalt::graphics
#include "OpenGLTextureSamplerCubeArray.inl"
