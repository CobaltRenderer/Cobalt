// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Deleter.h"
#include "ITextureSampler.h"
#include <memory>
namespace cobalt { namespace graphics {

class ITextureSampler3D : public ITextureSampler
{
public:
	// Typedefs
	typedef std::unique_ptr<ITextureSampler3D, Deleter<ITextureSampler3D>> unique_ptr;

public:
	// Initialization methods
	virtual void Delete() = 0;

	// Format methods
	virtual void SetTextureWrapMode(WrapMode wrapModeHorizontal, WrapMode wrapModeVertical, WrapMode wrapModeDepth) = 0;
	virtual void SetTextureFilterMode(FilterMode filterModeShrink, FilterMode filterModeExpand) = 0;
	virtual void SetTextureMipmapMode(MipmapMode mipmapMode) = 0;
	virtual void SetMipmapLevelMapping(float minLevel = 0, float maxLevel = 0, float levelBias = 0) = 0;

protected:
	// Constructors
	~ITextureSampler3D() = default;
};

}} // namespace cobalt::graphics
