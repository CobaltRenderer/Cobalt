// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "VulkanTextureSampler.h"
namespace cobalt::graphics {

class VulkanTextureSampler3D : public VulkanTextureSampler<ITextureSampler3D>
{
public:
	// Constructors
	inline VulkanTextureSampler3D(cobalt::logging::ILogger* log, VulkanRenderer* renderer);

	// Initialization methods
	inline void Delete() final;

	// Format methods
	inline void SetTextureWrapMode(WrapMode wrapModeHorizontal, WrapMode wrapModeVertical, WrapMode wrapModeDepth) final;

	// Build state methods
	inline void FlagObjectModified() final;
};

} // namespace cobalt::graphics
#include "VulkanTextureSampler3D.inl"
