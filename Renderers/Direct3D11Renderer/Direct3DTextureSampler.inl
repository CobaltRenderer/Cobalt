// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "Direct3DRenderer.h"
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
template<class T>
Direct3DTextureSampler<T>::Direct3DTextureSampler(cobalt::logging::ILogger* log, Direct3DRenderer* renderer)
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
	_buildState.maxLevel = D3D11_FLOAT32_MAX;
	_buildState.maxAnisotropy = 1;
	_buildState.anisotropyEnabled = false;
	_buildState.nativeObjectsCurrent = false;
}

//----------------------------------------------------------------------------------------
// Format methods
//----------------------------------------------------------------------------------------
template<class T>
void Direct3DTextureSampler<T>::SetTextureFilterMode(typename T::FilterMode filterModeShrink, typename T::FilterMode filterModeExpand)
{
	_buildState.filterModeShrink = filterModeShrink;
	_buildState.filterModeExpand = filterModeExpand;
	_buildState.nativeObjectsCurrent = false;
	FlagBuildStateModified();
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DTextureSampler<T>::SetTextureMipmapMode(typename T::MipmapMode mipmapMode)
{
	_buildState.mipmapMode = mipmapMode;
	_buildState.nativeObjectsCurrent = false;
	FlagBuildStateModified();
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DTextureSampler<T>::SetMipmapLevelMapping(float minLevel, float maxLevel, float levelBias)
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
void Direct3DTextureSampler<T>::MigrateBuildStateToDrawState()
{
	_drawState = _buildState;
	_buildState.nativeObjectsCurrent = true;
	_stateModified.clear(std::memory_order_relaxed);
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DTextureSampler<T>::FlagBuildStateModified()
{
	if (!_stateModified.test_and_set(std::memory_order_acquire))
	{
		FlagObjectModified();
	}
}

//----------------------------------------------------------------------------------------
template<class T>
ID3D11SamplerState* Direct3DTextureSampler<T>::GetSamplerState()
{
	// Update the native sampler object if required
	if (!_drawState.nativeObjectsCurrent)
	{
		if (!BuildSamplerObject())
		{
			return nullptr;
		}
		_drawState.nativeObjectsCurrent = true;
	}

	// Return the sampler object to the caller
	return _samplerState.Get();
}

//----------------------------------------------------------------------------------------
template<class T>
bool Direct3DTextureSampler<T>::BuildSamplerObject()
{
	bool mipmapDisabled = (_drawState.mipmapMode == ITextureSampler::MipmapMode::None);
	D3D11_FILTER filterMode = GetFilterModeNative(_drawState.filterModeShrink, _drawState.filterModeExpand, _drawState.mipmapMode, _drawState.anisotropyEnabled);
	D3D11_TEXTURE_ADDRESS_MODE wrapModeU = GetWrapModeNative(_drawState.wrapModeHorizontal);
	D3D11_TEXTURE_ADDRESS_MODE wrapModeV = GetWrapModeNative(_drawState.wrapModeVertical);
	D3D11_TEXTURE_ADDRESS_MODE wrapModeW = GetWrapModeNative(_drawState.wrapModeDepth);
	D3D11_SAMPLER_DESC desc = {};
	desc.Filter = filterMode;
	desc.AddressU = wrapModeU;
	desc.AddressV = wrapModeV;
	desc.AddressW = wrapModeW;
	desc.MinLOD = (mipmapDisabled ? 0 : _drawState.minLevel);
	desc.MaxLOD = (mipmapDisabled ? 0 : _drawState.maxLevel);
	desc.MipLODBias = _drawState.levelBias;
	if (_drawState.anisotropyEnabled)
	{
		desc.MaxAnisotropy = (_drawState.maxAnisotropy != 0 ? _drawState.maxAnisotropy : 16);
	}
	else
	{
		desc.MaxAnisotropy = 1;
	}
	desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	HRESULT createSamplerStateReturn = _renderer->GetDevice()->CreateSamplerState(&desc, &_samplerState);
	if (FAILED(createSamplerStateReturn))
	{
		_log->Error("CreateSamplerState failed with error code {0}", createSamplerStateReturn);
		return false;
	}
	return true;
}

//----------------------------------------------------------------------------------------
template<class T>
Direct3DRenderer* Direct3DTextureSampler<T>::Renderer() const
{
	return _renderer;
}

//----------------------------------------------------------------------------------------
// Format methods
//----------------------------------------------------------------------------------------
template<class T>
void Direct3DTextureSampler<T>::SetTextureWrapModeInternal(typename T::WrapMode wrapModeHorizontal, typename T::WrapMode wrapModeVertical, typename T::WrapMode wrapModeDepth)
{
	_buildState.wrapModeHorizontal = wrapModeHorizontal;
	_buildState.wrapModeVertical = wrapModeVertical;
	_buildState.wrapModeDepth = wrapModeDepth;
	_buildState.nativeObjectsCurrent = false;
	FlagBuildStateModified();
}

//----------------------------------------------------------------------------------------
template<class T>
void Direct3DTextureSampler<T>::SetAnisotropicFilterModeInternal(bool enabled, int maxAnisotropy)
{
	_buildState.anisotropyEnabled = enabled;
	_buildState.maxAnisotropy = maxAnisotropy;
	_buildState.nativeObjectsCurrent = false;
	FlagBuildStateModified();
}

//----------------------------------------------------------------------------------------
template<class T>
constexpr D3D11_FILTER Direct3DTextureSampler<T>::GetFilterModeNative(typename T::FilterMode filterModeShrink, typename T::FilterMode filterModeExpand, typename T::MipmapMode mipmapMode, bool anisotropyEnabled)
{
	if (anisotropyEnabled)
	{
		return D3D11_FILTER_ANISOTROPIC;
	}
	if ((filterModeShrink == T::FilterMode::Nearest) && (filterModeExpand == T::FilterMode::Nearest))
	{
		if (mipmapMode == T::MipmapMode::Linear)
		{
			return D3D11_FILTER::D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
		}
		return D3D11_FILTER::D3D11_FILTER_MIN_MAG_MIP_POINT;
	}
	if ((filterModeShrink == T::FilterMode::Nearest) && (filterModeExpand == T::FilterMode::Linear))
	{
		if (mipmapMode == T::MipmapMode::Linear)
		{
			return D3D11_FILTER::D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR;
		}
		return D3D11_FILTER::D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
	}
	if ((filterModeShrink == T::FilterMode::Linear) && (filterModeExpand == T::FilterMode::Nearest))
	{
		if (mipmapMode == T::MipmapMode::Linear)
		{
			return D3D11_FILTER::D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
		}
		return D3D11_FILTER::D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
	}
	if ((filterModeShrink == T::FilterMode::Linear) && (filterModeExpand == T::FilterMode::Linear))
	{
		if (mipmapMode == T::MipmapMode::Linear)
		{
			return D3D11_FILTER::D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		}
		return D3D11_FILTER::D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	}
	UNREACHABLE();
	return {};
}

//----------------------------------------------------------------------------------------
template<class T>
constexpr D3D11_TEXTURE_ADDRESS_MODE Direct3DTextureSampler<T>::GetWrapModeNative(typename T::WrapMode wrapMode)
{
	switch (wrapMode)
	{
	case T::WrapMode::ClampToEdge:
		return D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_CLAMP;
	case T::WrapMode::Repeat:
		return D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_WRAP;
	case T::WrapMode::RepeatMirrored:
		return D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_MIRROR;
	}
	UNREACHABLE();
	return {};
}

} // namespace cobalt::graphics
