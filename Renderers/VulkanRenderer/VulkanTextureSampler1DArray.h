// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "VulkanTextureSampler.h"
namespace cobalt::graphics {

class VulkanTextureSampler1DArray : public VulkanTextureSampler<ITextureSampler1DArray>
{
public:
	// Constructors
	inline VulkanTextureSampler1DArray(cobalt::logging::ILogger* log, VulkanRenderer* renderer);

	// Initialization methods
	inline void Delete() final;

	// Format methods
	inline void SetTextureWrapMode(WrapMode wrapModeHorizontal) final;

	// Build state methods
	inline void FlagObjectModified() final;
};

} // namespace cobalt::graphics
#include "VulkanTextureSampler1DArray.inl"
