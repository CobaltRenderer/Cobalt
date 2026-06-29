// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "TextureSamplerCubeArray.h"
#include <Cobalt/RendererInterface/RendererInterface.pkg>

using namespace cobalt::graphics;

//----------------------------------------------------------------------------------------
// Format methods
//----------------------------------------------------------------------------------------
void Cobalt_TextureSamplerCubeArray_SetTextureWrapMode(Cobalt_TextureSamplerCubeArray sampler, Cobalt_WrapMode wrapModeHorizontal, Cobalt_WrapMode wrapModeVertical)
{
	auto _this = reinterpret_cast<ITextureSamplerCubeArray*>(sampler);

	_this->SetTextureWrapMode((ITextureSampler::WrapMode)wrapModeHorizontal, (ITextureSampler::WrapMode)wrapModeVertical);
}

//----------------------------------------------------------------------------------------
void Cobalt_TextureSamplerCubeArray_SetTextureFilterMode(Cobalt_TextureSamplerCubeArray sampler, Cobalt_FilterMode filterModeShrink, Cobalt_FilterMode filterModeExpand)
{
	auto _this = reinterpret_cast<ITextureSamplerCubeArray*>(sampler);

	_this->SetTextureFilterMode((ITextureSampler::FilterMode)filterModeShrink, (ITextureSampler::FilterMode)filterModeExpand);
}

//----------------------------------------------------------------------------------------
void Cobalt_TextureSamplerCubeArray_SetTextureMipmapMode(Cobalt_TextureSamplerCubeArray sampler, Cobalt_MipmapMode mipmapMode)
{
	auto _this = reinterpret_cast<ITextureSamplerCubeArray*>(sampler);

	_this->SetTextureMipmapMode((ITextureSampler::MipmapMode)mipmapMode);
}

//----------------------------------------------------------------------------------------
void Cobalt_TextureSamplerCubeArray_SetMipmapLevelMapping(Cobalt_TextureSamplerCubeArray sampler, float minLevel, float maxLevel, float levelBias)
{
	auto _this = reinterpret_cast<ITextureSamplerCubeArray*>(sampler);

	_this->SetMipmapLevelMapping(minLevel, maxLevel, levelBias);
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void Cobalt_TextureSamplerCubeArray_Delete(Cobalt_TextureSamplerCubeArray sampler)
{
	auto _this = reinterpret_cast<ITextureSamplerCubeArray*>(sampler);

	_this->Delete();
}
