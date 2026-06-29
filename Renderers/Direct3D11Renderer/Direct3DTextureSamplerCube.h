// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Direct3DTextureSampler.h"
namespace cobalt::graphics {

class Direct3DTextureSamplerCube : public Direct3DTextureSampler<ITextureSamplerCube>
{
public:
	// Constructors
	inline Direct3DTextureSamplerCube(cobalt::logging::ILogger* log, Direct3DRenderer* renderer);

	// Initialization methods
	inline void Delete() final;

	// Format methods
	inline void SetTextureWrapMode(WrapMode wrapModeHorizontal, WrapMode wrapModeVertical) final;

	// Build state methods
	inline void FlagObjectModified() final;
};

} // namespace cobalt::graphics
#include "Direct3DTextureSamplerCube.inl"
