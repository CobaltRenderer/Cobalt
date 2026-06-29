// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "OpenGLHeaders.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <atomic>
namespace cobalt::graphics {
class OpenGLRenderer;

template<class T>
class OpenGLTextureSampler : public T
{
public:
	// Constructors
	OpenGLTextureSampler(cobalt::logging::ILogger* log, OpenGLRenderer* renderer);
	~OpenGLTextureSampler();

	// Format methods
	void SetTextureFilterMode(typename T::FilterMode filterModeShrink, typename T::FilterMode filterModeExpand) final;
	void SetTextureMipmapMode(typename T::MipmapMode mipmapMode) final;
	void SetMipmapLevelMapping(float minLevel, float maxLevel, float levelBias) final;

	// Build state methods
	void MigrateBuildStateToDrawState();
	GLuint GetSamplerNo();

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
	OpenGLRenderer* Renderer() const;

	// Format methods
	void SetTextureWrapModeInternal(typename T::WrapMode wrapModeHorizontal, typename T::WrapMode wrapModeVertical = T::WrapMode::ClampToEdge, typename T::WrapMode wrapModeDepth = T::WrapMode::ClampToEdge);
	void SetAnisotropicFilterModeInternal(bool enabled, int maxAnisotropy);

private:
	// Build state methods
	void BuildSamplerObject();

	// Format methods
	constexpr static GLenum GetFilterModeNative(typename T::FilterMode filterMode, typename T::MipmapMode mipmapMode);
	constexpr static GLenum GetWrapModeNative(typename T::WrapMode wrapMode);

private:
	cobalt::logging::ILogger* _log;
	OpenGLRenderer* _renderer;
	bool _samplerAllocated = false;
	GLuint _samplerNo = 0;
	std::atomic_flag _stateModified = ATOMIC_FLAG_INIT;
	MutableState _drawState = {};
	MutableState _buildState = {};
};

} // namespace cobalt::graphics
#include "OpenGLTextureSampler.inl"
