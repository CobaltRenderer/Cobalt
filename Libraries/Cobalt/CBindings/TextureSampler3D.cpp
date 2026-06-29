// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "TextureSampler3D.h"
#include <Cobalt/RendererInterface/RendererInterface.pkg>

using namespace cobalt::graphics;

//----------------------------------------------------------------------------------------
// Format methods
//----------------------------------------------------------------------------------------
void Cobalt_TextureSampler3D_SetTextureWrapMode(Cobalt_TextureSampler3D sampler, Cobalt_WrapMode wrapModeHorizontal, Cobalt_WrapMode wrapModeVertical, Cobalt_WrapMode wrapModeDepth)
{
	auto _this = reinterpret_cast<ITextureSampler3D*>(sampler);

	_this->SetTextureWrapMode((ITextureSampler::WrapMode)wrapModeHorizontal, (ITextureSampler::WrapMode)wrapModeVertical, (ITextureSampler::WrapMode)wrapModeDepth);
}

//----------------------------------------------------------------------------------------
void Cobalt_TextureSampler3D_SetTextureFilterMode(Cobalt_TextureSampler3D sampler, Cobalt_FilterMode filterModeShrink, Cobalt_FilterMode filterModeExpand)
{
	auto _this = reinterpret_cast<ITextureSampler3D*>(sampler);

	_this->SetTextureFilterMode((ITextureSampler::FilterMode)filterModeShrink, (ITextureSampler::FilterMode)filterModeExpand);
}

//----------------------------------------------------------------------------------------
void Cobalt_TextureSampler3D_SetTextureMipmapMode(Cobalt_TextureSampler3D sampler, Cobalt_MipmapMode mipmapMode)
{
	auto _this = reinterpret_cast<ITextureSampler3D*>(sampler);

	_this->SetTextureMipmapMode((ITextureSampler::MipmapMode)mipmapMode);
}

//----------------------------------------------------------------------------------------
void Cobalt_TextureSampler3D_SetMipmapLevelMapping(Cobalt_TextureSampler3D sampler, float minLevel, float maxLevel, float levelBias)
{
	auto _this = reinterpret_cast<ITextureSampler3D*>(sampler);

	_this->SetMipmapLevelMapping(minLevel, maxLevel, levelBias);
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void Cobalt_TextureSampler3D_Delete(Cobalt_TextureSampler3D sampler)
{
	auto _this = reinterpret_cast<ITextureSampler3D*>(sampler);

	_this->Delete();
}
