// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "OpenGLTextureSampler.h"
namespace cobalt::graphics {

class OpenGLTextureSampler2DArray : public OpenGLTextureSampler<ITextureSampler2DArray>
{
public:
	// Constructors
	inline OpenGLTextureSampler2DArray(cobalt::logging::ILogger* log, OpenGLRenderer* renderer);

	// Initialization methods
	inline void Delete() final;

	// Format methods
	inline void SetTextureWrapMode(WrapMode wrapModeHorizontal, WrapMode wrapModeVertical) final;
	inline void SetAnisotropicFilterMode(bool enabled, int maxAnisotropy) final;

	// Build state methods
	inline void FlagObjectModified() final;
};

} // namespace cobalt::graphics
#include "OpenGLTextureSampler2DArray.inl"
