// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Direct3DHeaders.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <dxgi1_2.h>
namespace cobalt::graphics {
class Direct3DGraphicsDevice;

class Direct3DGraphicsDeviceEnumerator : public IGraphicsDeviceEnumerator
{
public:
	// Constructors
	explicit Direct3DGraphicsDeviceEnumerator(cobalt::logging::ILogger::unique_ptr log);

	// Initialization methods
	void Delete() override;

	// Device methods
	SuccessToken EnumerateDevices(EnumerationFlags flags) override;
	bool FoundDevice() const override;
	Marshal::Ret<std::vector<IGraphicsDevice*>> GetAllDevices() const override;
	Marshal::Ret<std::vector<IGraphicsDevice*>> GetFilteredDevices() const override;
	IGraphicsDevice* GetPreferredDevice() const override;

	// Filtering methods
	void FilterDevice(IGraphicsDevice* targetDevice) override;
	void FilterDevicesOfType(IGraphicsDevice::DeviceType type) override;
	void FilterDevicesNotOfType(IGraphicsDevice::DeviceType type) override;
	void FilterDevicesWithoutFeature(IGraphicsDevice::Feature feature) override;
	void FilterDevicesWithoutAllFeatures(const Marshal::In<std::set<IGraphicsDevice::Feature>>& featureSet) override;
	void ClearDeviceFilters() override;

private:
	// Structures
	struct DeviceEntry
	{
		bool recommendedDevice = false;
		Microsoft::WRL::ComPtr<IDXGIAdapter2> adapter;
		std::unique_ptr<Direct3DGraphicsDevice> device;
	};

private:
	cobalt::logging::ILogger::unique_ptr _log;
	Microsoft::WRL::ComPtr<IDXGIFactory2> _dxgiFactory;
	std::vector<DeviceEntry> _devices;
	std::vector<size_t> _filteredDeviceIndices;
};

} // namespace cobalt::graphics
