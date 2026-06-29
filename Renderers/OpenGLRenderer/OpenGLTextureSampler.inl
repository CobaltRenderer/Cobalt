// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "OpenGLDebug.h"
#include "OpenGLRenderer.h"
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
template<class T>
OpenGLTextureSampler<T>::OpenGLTextureSampler(cobalt::logging::ILogger* log, OpenGLRenderer* renderer)
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
	_buildState.maxLevel = 1000;
	_buildState.maxAnisotropy = 1;
	_buildState.anisotropyEnabled = false;
	_buildState.nativeObjectsCurrent = false;
}

//----------------------------------------------------------------------------------------
template<class T>
OpenGLTextureSampler<T>::~OpenGLTextureSampler()
{
	if (_samplerAllocated)
	{
		glDeleteSamplers(1, &_samplerNo);
		_samplerNo = 0;
	}
}

//----------------------------------------------------------------------------------------
// Format methods
//----------------------------------------------------------------------------------------
template<class T>
void OpenGLTextureSampler<T>::SetTextureFilterMode(typename T::FilterMode filterModeShrink, typename T::FilterMode filterModeExpand)
{
	_buildState.filterModeShrink = filterModeShrink;
	_buildState.filterModeExpand = filterModeExpand;
	_buildState.nativeObjectsCurrent = false;
	FlagBuildStateModified();
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLTextureSampler<T>::SetTextureMipmapMode(typename T::MipmapMode mipmapMode)
{
	_buildState.mipmapMode = mipmapMode;
	_buildState.nativeObjectsCurrent = false;
	FlagBuildStateModified();
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLTextureSampler<T>::SetMipmapLevelMapping(float minLevel, float maxLevel, float levelBias)
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
void OpenGLTextureSampler<T>::MigrateBuildStateToDrawState()
{
	_drawState = _buildState;
	_buildState.nativeObjectsCurrent = true;
	_stateModified.clear(std::memory_order_relaxed);
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLTextureSampler<T>::FlagBuildStateModified()
{
	if (!_stateModified.test_and_set(std::memory_order_acquire))
	{
		FlagObjectModified();
	}
}

//----------------------------------------------------------------------------------------
template<class T>
GLuint OpenGLTextureSampler<T>::GetSamplerNo()
{
	// Update the native sampler object if required
	if (!_drawState.nativeObjectsCurrent)
	{
		BuildSamplerObject();
		_drawState.nativeObjectsCurrent = true;
	}

	// Return the sampler number to the caller
	return _samplerNo;
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLTextureSampler<T>::BuildSamplerObject()
{
	// Generate the sampler if required
	if (!_samplerAllocated)
	{
		glGenSamplers(1, &_samplerNo);
		CheckGLError(_log);
		_samplerAllocated = true;
	}

	// Update the sampler parameters
	GLenum filterModeShrink = GetFilterModeNative(_drawState.filterModeShrink, _drawState.mipmapMode);
	GLenum filterModeExpand = GetFilterModeNative(_drawState.filterModeExpand, T::MipmapMode::None);
	glSamplerParameteri(_samplerNo, GL_TEXTURE_MIN_FILTER, filterModeShrink);
	glSamplerParameteri(_samplerNo, GL_TEXTURE_MAG_FILTER, filterModeExpand);
	glSamplerParameterf(_samplerNo, GL_TEXTURE_MIN_LOD, _drawState.minLevel);
	glSamplerParameterf(_samplerNo, GL_TEXTURE_MAX_LOD, _drawState.maxLevel);
	glSamplerParameterf(_samplerNo, GL_TEXTURE_LOD_BIAS, _drawState.levelBias);
	GLenum wrapModeU = GetWrapModeNative(_drawState.wrapModeHorizontal);
	GLenum wrapModeV = GetWrapModeNative(_drawState.wrapModeVertical);
	GLenum wrapModeW = GetWrapModeNative(_drawState.wrapModeDepth);
	glSamplerParameteri(_samplerNo, GL_TEXTURE_WRAP_S, wrapModeU);
	glSamplerParameteri(_samplerNo, GL_TEXTURE_WRAP_T, wrapModeV);
	glSamplerParameteri(_samplerNo, GL_TEXTURE_WRAP_R, wrapModeW);
#ifdef GL_EXT_texture_filter_anisotropic
	if (GLAD_GL_EXT_texture_filter_anisotropic != 0)
	{
		if (_drawState.anisotropyEnabled)
		{
			glSamplerParameterf(_samplerNo, GL_TEXTURE_MAX_ANISOTROPY_EXT, (_drawState.maxAnisotropy > 0 ? (float)_drawState.maxAnisotropy : _renderer->GetMaxTextureSamplerAnisotropy()));
		}
		else
		{
			glSamplerParameterf(_samplerNo, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);
		}
	}
#endif
	CheckGLError(_log);
}

//----------------------------------------------------------------------------------------
template<class T>
OpenGLRenderer* OpenGLTextureSampler<T>::Renderer() const
{
	return _renderer;
}

//----------------------------------------------------------------------------------------
// Format methods
//----------------------------------------------------------------------------------------
template<class T>
void OpenGLTextureSampler<T>::SetTextureWrapModeInternal(typename T::WrapMode wrapModeHorizontal, typename T::WrapMode wrapModeVertical, typename T::WrapMode wrapModeDepth)
{
	_buildState.wrapModeHorizontal = wrapModeHorizontal;
	_buildState.wrapModeVertical = wrapModeVertical;
	_buildState.wrapModeDepth = wrapModeDepth;
	_buildState.nativeObjectsCurrent = false;
	FlagBuildStateModified();
}

//----------------------------------------------------------------------------------------
template<class T>
void OpenGLTextureSampler<T>::SetAnisotropicFilterModeInternal(bool enabled, int maxAnisotropy)
{
	_buildState.anisotropyEnabled = enabled;
	_buildState.maxAnisotropy = maxAnisotropy;
	_buildState.nativeObjectsCurrent = false;
	FlagBuildStateModified();
}

//----------------------------------------------------------------------------------------
template<class T>
constexpr GLenum OpenGLTextureSampler<T>::GetFilterModeNative(typename T::FilterMode filterMode, typename T::MipmapMode mipmapMode)
{
	switch (filterMode)
	{
	case T::FilterMode::Nearest:
		switch (mipmapMode)
		{
		case T::MipmapMode::None:
			return GL_NEAREST;
		case T::MipmapMode::Nearest:
			return GL_NEAREST_MIPMAP_NEAREST;
		case T::MipmapMode::Linear:
			return GL_NEAREST_MIPMAP_LINEAR;
		}
		break;
	case T::FilterMode::Linear:
		switch (mipmapMode)
		{
		case T::MipmapMode::None:
			return GL_LINEAR;
		case T::MipmapMode::Nearest:
			return GL_LINEAR_MIPMAP_NEAREST;
		case T::MipmapMode::Linear:
			return GL_LINEAR_MIPMAP_LINEAR;
		}
		break;
	}
	UNREACHABLE();
	return {};
}

//----------------------------------------------------------------------------------------
template<class T>
constexpr GLenum OpenGLTextureSampler<T>::GetWrapModeNative(typename T::WrapMode wrapMode)
{
	switch (wrapMode)
	{
	case T::WrapMode::ClampToEdge:
		return GL_CLAMP_TO_EDGE;
	case T::WrapMode::Repeat:
		return GL_REPEAT;
	case T::WrapMode::RepeatMirrored:
		return GL_MIRRORED_REPEAT;
	}
	UNREACHABLE();
	return {};
}

} // namespace cobalt::graphics
