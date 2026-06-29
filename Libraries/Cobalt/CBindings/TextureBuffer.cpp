// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "TextureBuffer.h"
#include <Cobalt/RendererInterface/RendererInterface.pkg>

using namespace cobalt::graphics;

//----------------------------------------------------------------------------------------
// Usage methods
//----------------------------------------------------------------------------------------
void Cobalt_TextureBuffer_SetUsageFlags(Cobalt_TextureBuffer texture, Cobalt_TextureUsageFlags usageFlags)
{
	auto _this = reinterpret_cast<ITextureBuffer*>(texture);

	_this->SetUsageFlags((ITextureBuffer::UsageFlags)usageFlags);
}

//----------------------------------------------------------------------------------------
void Cobalt_TextureBuffer_SetPerformanceHints(Cobalt_TextureBuffer texture, Cobalt_TexturePerformanceHint performanceHintCpu, Cobalt_TexturePerformanceHint performanceHintGpu)
{
	auto _this = reinterpret_cast<ITextureBuffer*>(texture);

	_this->SetPerformanceHints((ITextureBuffer::PerformanceHint)performanceHintCpu, (ITextureBuffer::PerformanceHint)performanceHintGpu);
}

//----------------------------------------------------------------------------------------
void Cobalt_TextureBuffer_SetDataPersistenceFlags(Cobalt_TextureBuffer texture, Cobalt_TextureDataPersistenceFlags dataPersistenceFlags)
{
	auto _this = reinterpret_cast<ITextureBuffer*>(texture);

	_this->SetDataPersistenceFlags((ITextureBuffer::DataPersistenceFlags)dataPersistenceFlags);
}
