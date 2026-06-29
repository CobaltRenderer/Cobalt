// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "DescriptorHandle.h"
#include "Direct3DHeaders.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <atomic>
namespace cobalt::graphics {
class Direct3DRenderer;

template<class T>
class Direct3DTextureSampler : public T
{
public:
	// Constructors
	Direct3DTextureSampler(cobalt::logging::ILogger* log, Direct3DRenderer* renderer);

	// Format methods
	void SetTextureFilterMode(typename T::FilterMode filterModeShrink, typename T::FilterMode filterModeExpand) final;
	void SetTextureMipmapMode(typename T::MipmapMode mipmapMode) final;
	void SetMipmapLevelMapping(float minLevel, float maxLevel, float levelBias) final;

	// Build state methods
	void MigrateBuildStateToDrawState();
	const CD3DX12_GPU_DESCRIPTOR_HANDLE& GetGPUDescriptorHandle() const;

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
	Direct3DRenderer* Renderer() const;

	// Format methods
	void SetTextureWrapModeInternal(typename T::WrapMode wrapModeHorizontal, typename T::WrapMode wrapModeVertical = T::WrapMode::ClampToEdge, typename T::WrapMode wrapModeDepth = T::WrapMode::ClampToEdge);
	void SetAnisotropicFilterModeInternal(bool enabled, int maxAnisotropy);

private:
	// Build state methods
	void BuildSamplerObject();

	// Format methods
	constexpr static D3D12_FILTER GetFilterModeNative(typename T::FilterMode filterModeShrink, typename T::FilterMode filterModeExpand, typename T::MipmapMode mipmapMode, bool anisotropyEnabled);
	constexpr static D3D12_TEXTURE_ADDRESS_MODE GetWrapModeNative(typename T::WrapMode wrapMode);

private:
	cobalt::logging::ILogger* _log;
	Direct3DRenderer* _renderer;
	std::unique_ptr<DescriptorHandle> _samplerHandle;
	std::atomic_flag _stateModified = ATOMIC_FLAG_INIT;
	MutableState _drawState = {};
	MutableState _buildState = {};
};

} // namespace cobalt::graphics
#include "Direct3DTextureSampler.inl"
