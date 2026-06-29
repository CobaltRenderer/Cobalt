// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "TextureSampler2D.h"
#include <Cobalt/RendererInterface/RendererInterface.pkg>

using namespace cobalt::graphics;

//----------------------------------------------------------------------------------------
// Format methods
//----------------------------------------------------------------------------------------
void Cobalt_TextureSampler2D_SetTextureWrapMode(Cobalt_TextureSampler2D sampler, Cobalt_WrapMode wrapModeHorizontal, Cobalt_WrapMode wrapModeVertical)
{
	auto _this = reinterpret_cast<ITextureSampler2D*>(sampler);

	_this->SetTextureWrapMode((ITextureSampler::WrapMode)wrapModeHorizontal, (ITextureSampler::WrapMode)wrapModeVertical);
}

//----------------------------------------------------------------------------------------
void Cobalt_TextureSampler2D_SetTextureFilterMode(Cobalt_TextureSampler2D sampler, Cobalt_FilterMode filterModeShrink, Cobalt_FilterMode filterModeExpand)
{
	auto _this = reinterpret_cast<ITextureSampler2D*>(sampler);

	_this->SetTextureFilterMode((ITextureSampler::FilterMode)filterModeShrink, (ITextureSampler::FilterMode)filterModeExpand);
}

//----------------------------------------------------------------------------------------
void Cobalt_TextureSampler2D_SetTextureMipmapMode(Cobalt_TextureSampler2D sampler, Cobalt_MipmapMode mipmapMode)
{
	auto _this = reinterpret_cast<ITextureSampler2D*>(sampler);

	_this->SetTextureMipmapMode((ITextureSampler::MipmapMode)mipmapMode);
}

//----------------------------------------------------------------------------------------
void Cobalt_TextureSampler2D_SetMipmapLevelMapping(Cobalt_TextureSampler2D sampler, float minLevel, float maxLevel, float levelBias)
{
	auto _this = reinterpret_cast<ITextureSampler2D*>(sampler);

	_this->SetMipmapLevelMapping(minLevel, maxLevel, levelBias);
}

//----------------------------------------------------------------------------------------
void Cobalt_TextureSampler2D_SetAnisotropicFilterMode(Cobalt_TextureSampler2D sampler, char enabled, int maxAnisotropy)
{
	auto _this = reinterpret_cast<ITextureSampler2D*>(sampler);

	_this->SetAnisotropicFilterMode(enabled != 0, maxAnisotropy);
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void Cobalt_TextureSampler2D_Delete(Cobalt_TextureSampler2D sampler)
{
	auto _this = reinterpret_cast<ITextureSampler2D*>(sampler);

	_this->Delete();
}
