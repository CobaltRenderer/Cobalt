// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "VulkanHeaders.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <atomic>
namespace cobalt::graphics {
class VulkanRenderer;

template<class T>
class VulkanTextureSampler : public T
{
public:
	// Constructors
	VulkanTextureSampler(cobalt::logging::ILogger* log, VulkanRenderer* renderer);
	~VulkanTextureSampler();

	// Format methods
	void SetTextureFilterMode(typename T::FilterMode filterModeShrink, typename T::FilterMode filterModeExpand) final;
	void SetTextureMipmapMode(typename T::MipmapMode mipmapMode) final;
	void SetMipmapLevelMapping(float minLevel, float maxLevel, float levelBias) final;

	// Build state methods
	void MigrateBuildStateToDrawState();
	VkSampler GetNativeSampler();

protected:
	// Structures
	struct MutableState
	{
		typename T::WrapMode wrapModeHorizontal;
		typename T::WrapMode wrapModeVertical;
		typename T::WrapMode wrapModeDepth;
		typename T::FilterMode filterModeShrink;
		typename T::FilterMode filterModeExpand;
		typename T::MipmapMode mipmapMode;
		float minLevel;
		float maxLevel;
		float levelBias;
		unsigned int maxAnisotropy;
		bool anisotropyEnabled;
		bool nativeObjectsCurrent;
	};

protected:
	// Build state methods
	void FlagBuildStateModified();
	virtual void FlagObjectModified() = 0;
	VulkanRenderer* Renderer() const;

	// Format methods
	void SetTextureWrapModeInternal(typename T::WrapMode wrapModeHorizontal, typename T::WrapMode wrapModeVertical = T::WrapMode::ClampToEdge, typename T::WrapMode wrapModeDepth = T::WrapMode::ClampToEdge);
	void SetAnisotropicFilterModeInternal(bool enabled, int maxAnisotropy);

private:
	// Build state methods
	void BuildSamplerObject();
	void DestroySamplerObject();

	// Format methods
	constexpr static VkFilter GetFilterModeNative(typename T::FilterMode filterMode);
	constexpr static VkSamplerMipmapMode GetMipmapModeNative(typename T::MipmapMode mipmapMode);
	constexpr static VkSamplerAddressMode GetWrapModeNative(typename T::WrapMode wrapMode);

private:
	cobalt::logging::ILogger* _log;
	VulkanRenderer* _renderer;
	VkSampler _sampler = {};
	bool _createdSampler = false;
	std::atomic_flag _stateModified = ATOMIC_FLAG_INIT;
	MutableState _drawState = {};
	MutableState _buildState = {};
};

} // namespace cobalt::graphics
#include "VulkanTextureSampler.inl"
