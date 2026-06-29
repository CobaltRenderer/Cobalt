// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "VulkanTextureSampler.h"
namespace cobalt::graphics {

class VulkanTextureSamplerCubeArray : public VulkanTextureSampler<ITextureSamplerCubeArray>
{
public:
	// Constructors
	inline VulkanTextureSamplerCubeArray(cobalt::logging::ILogger* log, VulkanRenderer* renderer);

	// Initialization methods
	inline void Delete() final;

	// Format methods
	inline void SetTextureWrapMode(WrapMode wrapModeHorizontal, WrapMode wrapModeVertical) final;

	// Build state methods
	inline void FlagObjectModified() final;
};

} // namespace cobalt::graphics
#include "VulkanTextureSamplerCubeArray.inl"
