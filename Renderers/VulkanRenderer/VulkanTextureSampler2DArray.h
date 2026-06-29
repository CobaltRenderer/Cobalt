// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "VulkanTextureSampler.h"
namespace cobalt::graphics {

class VulkanTextureSampler2DArray : public VulkanTextureSampler<ITextureSampler2DArray>
{
public:
	// Constructors
	inline VulkanTextureSampler2DArray(cobalt::logging::ILogger* log, VulkanRenderer* renderer);

	// Initialization methods
	inline void Delete() final;

	// Format methods
	inline void SetTextureWrapMode(WrapMode wrapModeHorizontal, WrapMode wrapModeVertical) final;
	inline void SetAnisotropicFilterMode(bool enabled, int maxAnisotropy) final;

	// Build state methods
	inline void FlagObjectModified() final;
};

} // namespace cobalt::graphics
#include "VulkanTextureSampler2DArray.inl"
