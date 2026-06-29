// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Direct3DTextureBuffer.h"
namespace cobalt::graphics {

class Direct3DTextureBuffer2D : public Direct3DTextureBuffer<ITextureBuffer2D, V2UInt32>
{
public:
	// Constructors
	inline Direct3DTextureBuffer2D(cobalt::logging::ILogger* log, Direct3DRenderer* renderer);

	// Initialization methods
	inline void Delete() final;

protected:
	// Build state methods
	inline void FlagObjectModified() final;
	inline void PopulateTextureResourceDescription(CD3DX12_RESOURCE_DESC& resourceDescription, D3D12_RESOURCE_FLAGS resourceFlags, DXGI_FORMAT internalFormat) final;
	inline void PopulateShaderResourceViewDescription(D3D12_SHADER_RESOURCE_VIEW_DESC& viewDescription, DXGI_FORMAT internalFormat) final;
};

} // namespace cobalt::graphics
#include "Direct3DTextureBuffer2D.inl"
