// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "TextureSampler2DArray.h"
#include <Cobalt/RendererInterface/RendererInterface.pkg>

using namespace cobalt::graphics;

//----------------------------------------------------------------------------------------
// Format methods
//----------------------------------------------------------------------------------------
void Cobalt_TextureSampler2DArray_SetTextureWrapMode(Cobalt_TextureSampler2DArray sampler, Cobalt_WrapMode wrapModeHorizontal, Cobalt_WrapMode wrapModeVertical)
{
	auto _this = reinterpret_cast<ITextureSampler2DArray*>(sampler);

	_this->SetTextureWrapMode((ITextureSampler::WrapMode)wrapModeHorizontal, (ITextureSampler::WrapMode)wrapModeVertical);
}

//----------------------------------------------------------------------------------------
void Cobalt_TextureSampler2DArray_SetTextureFilterMode(Cobalt_TextureSampler2DArray sampler, Cobalt_FilterMode filterModeShrink, Cobalt_FilterMode filterModeExpand)
{
	auto _this = reinterpret_cast<ITextureSampler2DArray*>(sampler);

	_this->SetTextureFilterMode((ITextureSampler::FilterMode)filterModeShrink, (ITextureSampler::FilterMode)filterModeExpand);
}

//----------------------------------------------------------------------------------------
void Cobalt_TextureSampler2DArray_SetTextureMipmapMode(Cobalt_TextureSampler2DArray sampler, Cobalt_MipmapMode mipmapMode)
{
	auto _this = reinterpret_cast<ITextureSampler2DArray*>(sampler);

	_this->SetTextureMipmapMode((ITextureSampler::MipmapMode)mipmapMode);
}

//----------------------------------------------------------------------------------------
void Cobalt_TextureSampler2DArray_SetMipmapLevelMapping(Cobalt_TextureSampler2DArray sampler, float minLevel, float maxLevel, float levelBias)
{
	auto _this = reinterpret_cast<ITextureSampler2DArray*>(sampler);

	_this->SetMipmapLevelMapping(minLevel, maxLevel, levelBias);
}

//----------------------------------------------------------------------------------------
void Cobalt_TextureSampler2DArray_SetAnisotropicFilterMode(Cobalt_TextureSampler2DArray sampler, char enabled, int maxAnisotropy)
{
	auto _this = reinterpret_cast<ITextureSampler2DArray*>(sampler);

	_this->SetAnisotropicFilterMode(enabled != 0, maxAnisotropy);
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void Cobalt_TextureSampler2DArray_Delete(Cobalt_TextureSampler2DArray sampler)
{
	auto _this = reinterpret_cast<ITextureSampler2DArray*>(sampler);

	_this->Delete();
}
