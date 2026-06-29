// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "VulkanRenderer.h"
#include <limits>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
template<class T>
VulkanTextureSampler<T>::VulkanTextureSampler(cobalt::logging::ILogger* log, VulkanRenderer* renderer)
: _log(log), _renderer(renderer)
{
	_buildState.wrapModeHorizontal = T::WrapMode::ClampToEdge;
	_buildState.wrapModeVertical = T::WrapMode::ClampToEdge;
	_buildState.wrapModeDepth = T::WrapMode::ClampToEdge;
	_buildState.filterModeShrink = T::FilterMode::Linear;
	_buildState.filterModeExpand = T::FilterMode::Linear;
	_buildState.mipmapMode = T::MipmapMode::None;
	_buildState.levelBias = 0.0f;
	_buildState.minLevel = 0.0f;
	_buildState.maxLevel = std::numeric_limits<decltype(_buildState.maxLevel)>::max();
	_buildState.maxAnisotropy = 1;
	_buildState.anisotropyEnabled = false;
	_buildState.nativeObjectsCurrent = false;
}

//----------------------------------------------------------------------------------------
template<class T>
VulkanTextureSampler<T>::~VulkanTextureSampler()
{
	DestroySamplerObject();
}

//----------------------------------------------------------------------------------------
// Format methods
//----------------------------------------------------------------------------------------
template<class T>
void VulkanTextureSampler<T>::SetTextureFilterMode(typename T::FilterMode filterModeShrink, typename T::FilterMode filterModeExpand)
{
	_buildState.filterModeShrink = filterModeShrink;
	_buildState.filterModeExpand = filterModeExpand;
	_buildState.nativeObjectsCurrent = false;
	FlagBuildStateModified();
}

//----------------------------------------------------------------------------------------
template<class T>
void VulkanTextureSampler<T>::SetTextureMipmapMode(typename T::MipmapMode mipmapMode)
{
	_buildState.mipmapMode = mipmapMode;
	_buildState.nativeObjectsCurrent = false;
	FlagBuildStateModified();
}

//----------------------------------------------------------------------------------------
template<class T>
void VulkanTextureSampler<T>::SetMipmapLevelMapping(float minLevel, float maxLevel, float levelBias)
{
	_buildState.minLevel = minLevel;
	_buildState.maxLevel = maxLevel;
	_buildState.levelBias = levelBias;
	_buildState.nativeObjectsCurrent = false;
	FlagBuildStateModified();
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
template<class T>
void VulkanTextureSampler<T>::MigrateBuildStateToDrawState()
{
	_drawState = _buildState;
	_buildState.nativeObjectsCurrent = true;
	if (!_drawState.nativeObjectsCurrent)
	{
		BuildSamplerObject();
		_drawState.nativeObjectsCurrent = true;
	}
	_stateModified.clear(std::memory_order_relaxed);
}

//----------------------------------------------------------------------------------------
template<class T>
void VulkanTextureSampler<T>::FlagBuildStateModified()
{
	if (!_stateModified.test_and_set(std::memory_order_acquire))
	{
		FlagObjectModified();
	}
}

//----------------------------------------------------------------------------------------
template<class T>
VkSampler VulkanTextureSampler<T>::GetNativeSampler()
{
	// Return the sampler handle to the caller
	return _sampler;
}

//----------------------------------------------------------------------------------------
template<class T>
void VulkanTextureSampler<T>::BuildSamplerObject()
{
	// If we've already created a sampler object, delete it now.
	if (_createdSampler)
	{
		DestroySamplerObject();
	}

	// Populate our VkSamplerCreateInfo structure
	bool mipmapDisabled = (_drawState.mipmapMode == ITextureSampler::MipmapMode::None);
	VkFilter filterModeShrink = GetFilterModeNative(_drawState.filterModeShrink);
	VkFilter filterModeExpand = GetFilterModeNative(_drawState.filterModeExpand);
	VkSamplerMipmapMode mipmapMode = GetMipmapModeNative(_drawState.mipmapMode);
	VkSamplerAddressMode wrapModeU = GetWrapModeNative(_drawState.wrapModeHorizontal);
	VkSamplerAddressMode wrapModeV = GetWrapModeNative(_drawState.wrapModeVertical);
	VkSamplerAddressMode wrapModeW = GetWrapModeNative(_drawState.wrapModeDepth);
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.minFilter = filterModeShrink;
	samplerInfo.magFilter = filterModeExpand;
	samplerInfo.mipmapMode = mipmapMode;
	samplerInfo.addressModeU = wrapModeU;
	samplerInfo.addressModeV = wrapModeV;
	samplerInfo.addressModeW = wrapModeW;
	samplerInfo.minLod = (mipmapDisabled ? 0 : _drawState.minLevel);
	samplerInfo.maxLod = (mipmapDisabled ? 0 : _drawState.maxLevel);
	samplerInfo.mipLodBias = _drawState.levelBias;
	if (_drawState.anisotropyEnabled)
	{
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = (_drawState.maxAnisotropy != 0 ? (float)_drawState.maxAnisotropy : _renderer->GetMaxTextureSamplerAnisotropy());
	}
	else
	{
		samplerInfo.anisotropyEnable = VK_FALSE;
		samplerInfo.maxAnisotropy = 1.0f;
	}
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

	// Create the sampler object
	VkResult result = vkCreateSampler(_renderer->GetDevice(), &samplerInfo, nullptr, &_sampler);
	if (result != VK_SUCCESS)
	{
		_log->Error("vkCreateSampler failed with error code {0}", result);
		return;
	}
	_createdSampler = true;
}

//----------------------------------------------------------------------------------------
template<class T>
void VulkanTextureSampler<T>::DestroySamplerObject()
{
	if (_createdSampler)
	{
		vkDestroySampler(_renderer->GetDevice(), _sampler, nullptr);
		_createdSampler = false;
	}
}

//----------------------------------------------------------------------------------------
template<class T>
VulkanRenderer* VulkanTextureSampler<T>::Renderer() const
{
	return _renderer;
}

//----------------------------------------------------------------------------------------
// Format methods
//----------------------------------------------------------------------------------------
template<class T>
void VulkanTextureSampler<T>::SetTextureWrapModeInternal(typename T::WrapMode wrapModeHorizontal, typename T::WrapMode wrapModeVertical, typename T::WrapMode wrapModeDepth)
{
	_buildState.wrapModeHorizontal = wrapModeHorizontal;
	_buildState.wrapModeVertical = wrapModeVertical;
	_buildState.wrapModeDepth = wrapModeDepth;
	_buildState.nativeObjectsCurrent = false;
	FlagBuildStateModified();
}

//----------------------------------------------------------------------------------------
template<class T>
void VulkanTextureSampler<T>::SetAnisotropicFilterModeInternal(bool enabled, int maxAnisotropy)
{
	_buildState.anisotropyEnabled = enabled;
	_buildState.maxAnisotropy = maxAnisotropy;
	_buildState.nativeObjectsCurrent = false;
	FlagBuildStateModified();
}

//----------------------------------------------------------------------------------------
template<class T>
constexpr VkFilter VulkanTextureSampler<T>::GetFilterModeNative(typename T::FilterMode filterMode)
{
	switch (filterMode)
	{
	case T::FilterMode::Nearest:
		return VK_FILTER_NEAREST;
	case T::FilterMode::Linear:
		return VK_FILTER_LINEAR;
	}
	UNREACHABLE();
	return VK_FILTER_MAX_ENUM;
}

//----------------------------------------------------------------------------------------
template<class T>
constexpr VkSamplerMipmapMode VulkanTextureSampler<T>::GetMipmapModeNative(typename T::MipmapMode mipmapMode)
{
	switch (mipmapMode)
	{
	case T::MipmapMode::None:
	case T::MipmapMode::Nearest:
		return VK_SAMPLER_MIPMAP_MODE_NEAREST;
	case T::MipmapMode::Linear:
		return VK_SAMPLER_MIPMAP_MODE_LINEAR;
	}
	UNREACHABLE();
	return VK_SAMPLER_MIPMAP_MODE_MAX_ENUM;
}

//----------------------------------------------------------------------------------------
template<class T>
constexpr VkSamplerAddressMode VulkanTextureSampler<T>::GetWrapModeNative(typename T::WrapMode wrapMode)
{
	switch (wrapMode)
	{
	case T::WrapMode::ClampToEdge:
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	case T::WrapMode::Repeat:
		return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	case T::WrapMode::RepeatMirrored:
		return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	}
	UNREACHABLE();
	return VK_SAMPLER_ADDRESS_MODE_MAX_ENUM;
}

} // namespace cobalt::graphics
