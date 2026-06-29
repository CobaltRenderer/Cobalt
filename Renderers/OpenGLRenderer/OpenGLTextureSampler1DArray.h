// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "OpenGLTextureSampler.h"
namespace cobalt::graphics {

class OpenGLTextureSampler1DArray : public OpenGLTextureSampler<ITextureSampler1DArray>
{
public:
	// Constructors
	inline OpenGLTextureSampler1DArray(cobalt::logging::ILogger* log, OpenGLRenderer* renderer);

	// Initialization methods
	inline void Delete() final;

	// Format methods
	inline void SetTextureWrapMode(WrapMode wrapModeHorizontal) final;

	// Build state methods
	inline void FlagObjectModified() final;
};

} // namespace cobalt::graphics
#include "OpenGLTextureSampler1DArray.inl"
