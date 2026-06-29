// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "TextureSamplerCube.h"
#include <Cobalt/RendererInterface/RendererInterface.pkg>

using namespace cobalt::graphics;

//----------------------------------------------------------------------------------------
// Format methods
//----------------------------------------------------------------------------------------
void Cobalt_TextureSamplerCube_SetTextureWrapMode(Cobalt_TextureSamplerCube sampler, Cobalt_WrapMode wrapModeHorizontal, Cobalt_WrapMode wrapModeVertical)
{
	auto _this = reinterpret_cast<ITextureSamplerCube*>(sampler);

	_this->SetTextureWrapMode((ITextureSampler::WrapMode)wrapModeHorizontal, (ITextureSampler::WrapMode)wrapModeVertical);
}

//----------------------------------------------------------------------------------------
void Cobalt_TextureSamplerCube_SetTextureFilterMode(Cobalt_TextureSamplerCube sampler, Cobalt_FilterMode filterModeShrink, Cobalt_FilterMode filterModeExpand)
{
	auto _this = reinterpret_cast<ITextureSamplerCube*>(sampler);

	_this->SetTextureFilterMode((ITextureSampler::FilterMode)filterModeShrink, (ITextureSampler::FilterMode)filterModeExpand);
}

//----------------------------------------------------------------------------------------
void Cobalt_TextureSamplerCube_SetTextureMipmapMode(Cobalt_TextureSamplerCube sampler, Cobalt_MipmapMode mipmapMode)
{
	auto _this = reinterpret_cast<ITextureSamplerCube*>(sampler);

	_this->SetTextureMipmapMode((ITextureSampler::MipmapMode)mipmapMode);
}

//----------------------------------------------------------------------------------------
void Cobalt_TextureSamplerCube_SetMipmapLevelMapping(Cobalt_TextureSamplerCube sampler, float minLevel, float maxLevel, float levelBias)
{
	auto _this = reinterpret_cast<ITextureSamplerCube*>(sampler);

	_this->SetMipmapLevelMapping(minLevel, maxLevel, levelBias);
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void Cobalt_TextureSamplerCube_Delete(Cobalt_TextureSamplerCube sampler)
{
	auto _this = reinterpret_cast<ITextureSamplerCube*>(sampler);

	_this->Delete();
}
