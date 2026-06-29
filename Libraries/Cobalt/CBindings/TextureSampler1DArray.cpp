// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "TextureSampler1DArray.h"
#include <Cobalt/RendererInterface/RendererInterface.pkg>

using namespace cobalt::graphics;

//----------------------------------------------------------------------------------------
// Format methods
//----------------------------------------------------------------------------------------
void Cobalt_TextureSampler1DArray_SetTextureWrapMode(Cobalt_TextureSampler1DArray sampler, Cobalt_WrapMode wrapModeHorizontal)
{
	auto _this = reinterpret_cast<ITextureSampler1DArray*>(sampler);

	_this->SetTextureWrapMode((ITextureSampler::WrapMode)wrapModeHorizontal);
}

//----------------------------------------------------------------------------------------
void Cobalt_TextureSampler1DArray_SetTextureFilterMode(Cobalt_TextureSampler1DArray sampler, Cobalt_FilterMode filterModeShrink, Cobalt_FilterMode filterModeExpand)
{
	auto _this = reinterpret_cast<ITextureSampler1DArray*>(sampler);

	_this->SetTextureFilterMode((ITextureSampler::FilterMode)filterModeShrink, (ITextureSampler::FilterMode)filterModeExpand);
}

//----------------------------------------------------------------------------------------
void Cobalt_TextureSampler1DArray_SetTextureMipmapMode(Cobalt_TextureSampler1DArray sampler, Cobalt_MipmapMode mipmapMode)
{
	auto _this = reinterpret_cast<ITextureSampler1DArray*>(sampler);

	_this->SetTextureMipmapMode((ITextureSampler::MipmapMode)mipmapMode);
}

//----------------------------------------------------------------------------------------
void Cobalt_TextureSampler1DArray_SetMipmapLevelMapping(Cobalt_TextureSampler1DArray sampler, float minLevel, float maxLevel, float levelBias)
{
	auto _this = reinterpret_cast<ITextureSampler1DArray*>(sampler);

	_this->SetMipmapLevelMapping(minLevel, maxLevel, levelBias);
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void Cobalt_TextureSampler1DArray_Delete(Cobalt_TextureSampler1DArray sampler)
{
	auto _this = reinterpret_cast<ITextureSampler1DArray*>(sampler);

	_this->Delete();
}
