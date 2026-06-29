// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Direct3DTextureBuffer.h"
namespace cobalt::graphics {

class Direct3DTextureBuffer3D : public Direct3DTextureBuffer<ITextureBuffer3D, V3UInt32>
{
public:
	// Constructors
	inline Direct3DTextureBuffer3D(cobalt::logging::ILogger* log, Direct3DRenderer* renderer);
	inline ~Direct3DTextureBuffer3D();

	// Initialization methods
	inline void Delete() final;

	// Build state methods
	inline ID3D11Resource* GetTextureAsResource() const final;

protected:
	// Initialization methods
	inline void ReleaseMemoryInternal() final;

	// Build state methods
	inline void FlagObjectModified() final;
	inline bool CreateTextureObject(ImageFormat imageFormat, DataFormat dataFormat, DXGI_FORMAT nativeFormat, UINT bindFlags, UINT cpuFlags, D3D11_USAGE usageType, const std::vector<InitialDataEntry>& initialData) final;

	// Data update methods
	inline bool CompletePendingDataWrite(const PendingWrite& pendingWrite, ID3D11Device1* device, ID3D11DeviceContext1* context) final;

private:
	Microsoft::WRL::ComPtr<ID3D11Texture3D> _texture;
};

} // namespace cobalt::graphics
#include "Direct3DTextureBuffer3D.inl"
