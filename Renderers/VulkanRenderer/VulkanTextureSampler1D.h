// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "VulkanTextureSampler.h"
namespace cobalt::graphics {

class VulkanTextureSampler1D : public VulkanTextureSampler<ITextureSampler1D>
{
public:
	// Constructors
	inline VulkanTextureSampler1D(cobalt::logging::ILogger* log, VulkanRenderer* renderer);

	// Initialization methods
	inline void Delete() final;

	// Format methods
	inline void SetTextureWrapMode(WrapMode wrapModeHorizontal) final;

	// Build state methods
	inline void FlagObjectModified() final;
};

} // namespace cobalt::graphics
#include "VulkanTextureSampler1D.inl"
