// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
namespace cobalt::graphics {
class VulkanGraphicsDevice;

class VulkanGraphicsDeviceEnumerator : public IGraphicsDeviceEnumerator
{
public:
	// Constructors
	explicit VulkanGraphicsDeviceEnumerator(cobalt::logging::ILogger::unique_ptr log);

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
	logging::ILogger::unique_ptr _log;
	std::vector<std::unique_ptr<VulkanGraphicsDevice>> _devices;
	std::vector<IGraphicsDevice*> _filteredDevices;
};

} // namespace cobalt::graphics
