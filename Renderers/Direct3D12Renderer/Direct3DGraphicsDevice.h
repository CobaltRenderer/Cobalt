// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Direct3DHeaders.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <dxgi1_4.h>
namespace cobalt::graphics {

class Direct3DGraphicsDevice : public IGraphicsDevice
{
public:
	// Constructors
	Direct3DGraphicsDevice(cobalt::logging::ILogger* log, Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory, Microsoft::WRL::ComPtr<IDXGIAdapter3> adapter, const DXGI_ADAPTER_DESC2& adapterDescription);

	// Info methods
	DeviceType GetDeviceType() const override;
	Vendor GetVendor() const override;
	Marshal::Ret<std::string> GetVendorName() const override;
	Marshal::Ret<std::string> GetDeviceName() const override;
	Marshal::Ret<std::string> GetDriverInfo() const override;
	size_t GetMemorySizeInBytes(MemoryType memoryType) const override;

	// Limit methods
	ImageLimits GetImageLimits() const override;
	ShaderLimits GetShaderLimits() const override;
	DrawLimits GetDrawLimits() const override;
	FrameBufferLimits GetFrameBufferLimits() const override;
	DataBufferLimits GetDataBufferLimits() const override;

	// Feature methods
	bool IsFeatureSupported(Feature feature) const override;
	bool AreAllFeaturesSupported(const Marshal::In<std::set<Feature>>& featureSet) const override;
	Marshal::Ret<std::set<Feature>> GetAllSupportedFeatures() const override;
	bool IsTextureFormatSupported(ITextureBuffer::ImageFormat imageFormat, ITextureBuffer::DataFormat dataFormat) const override;

	// Renderer methods
	IRenderer::unique_ptr CreateRenderer(const Marshal::In<std::set<Feature>>& enabledFeatures, const Marshal::In<std::set<IRenderer::Options>>& enabledOptions) override;

private:
	cobalt::logging::ILogger* _log = nullptr;
	Microsoft::WRL::ComPtr<IDXGIFactory4> _dxgiFactory;
	Microsoft::WRL::ComPtr<IDXGIAdapter3> _adapter;
	DXGI_ADAPTER_DESC2 _adapterDescription;
	std::set<Feature> _supportedFeatureSet;
};

} // namespace cobalt::graphics
