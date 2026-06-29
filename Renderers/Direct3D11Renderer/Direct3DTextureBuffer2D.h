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
	inline ~Direct3DTextureBuffer2D();

	// Initialization methods
	inline void Delete() final;

	// Build state methods
	inline ID3D11Resource* GetTextureAsResource() const final;
	inline ID3D11Texture2D* GetTexture() const;

protected:
	// Initialization methods
	inline void ReleaseMemoryInternal() final;

	// Build state methods
	inline void FlagObjectModified() final;
	inline bool CreateTextureObject(ImageFormat imageFormat, DataFormat dataFormat, DXGI_FORMAT nativeFormat, UINT bindFlags, UINT cpuFlags, D3D11_USAGE usageType, const std::vector<InitialDataEntry>& initialData) final;

	// Data update methods
	inline bool CompletePendingDataWrite(const PendingWrite& pendingWrite, ID3D11Device1* device, ID3D11DeviceContext1* context) final;

private:
	Microsoft::WRL::ComPtr<ID3D11Texture2D> _texture;
};

} // namespace cobalt::graphics
#include "Direct3DTextureBuffer2D.inl"
