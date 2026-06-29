// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "TextureSampler1D.h"
#include <Cobalt/RendererInterface/RendererInterface.pkg>

using namespace cobalt::graphics;

//----------------------------------------------------------------------------------------
// Format methods
//----------------------------------------------------------------------------------------
void Cobalt_TextureSampler1D_SetTextureWrapMode(Cobalt_TextureSampler1D sampler, Cobalt_WrapMode wrapModeHorizontal)
{
	auto _this = reinterpret_cast<ITextureSampler1D*>(sampler);

	_this->SetTextureWrapMode((ITextureSampler::WrapMode)wrapModeHorizontal);
}

//----------------------------------------------------------------------------------------
void Cobalt_TextureSampler1D_SetTextureFilterMode(Cobalt_TextureSampler1D sampler, Cobalt_FilterMode filterModeShrink, Cobalt_FilterMode filterModeExpand)
{
	auto _this = reinterpret_cast<ITextureSampler1D*>(sampler);

	_this->SetTextureFilterMode((ITextureSampler::FilterMode)filterModeShrink, (ITextureSampler::FilterMode)filterModeExpand);
}

//----------------------------------------------------------------------------------------
void Cobalt_TextureSampler1D_SetTextureMipmapMode(Cobalt_TextureSampler1D sampler, Cobalt_MipmapMode mipmapMode)
{
	auto _this = reinterpret_cast<ITextureSampler1D*>(sampler);

	_this->SetTextureMipmapMode((ITextureSampler::MipmapMode)mipmapMode);
}

//----------------------------------------------------------------------------------------
void Cobalt_TextureSampler1D_SetMipmapLevelMapping(Cobalt_TextureSampler1D sampler, float minLevel, float maxLevel, float levelBias)
{
	auto _this = reinterpret_cast<ITextureSampler1D*>(sampler);

	_this->SetMipmapLevelMapping(minLevel, maxLevel, levelBias);
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void Cobalt_TextureSampler1D_Delete(Cobalt_TextureSampler1D sampler)
{
	auto _this = reinterpret_cast<ITextureSampler1D*>(sampler);

	_this->Delete();
}
