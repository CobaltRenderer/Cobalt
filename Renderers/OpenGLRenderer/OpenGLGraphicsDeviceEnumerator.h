// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
namespace cobalt::graphics {
class OpenGLGraphicsDevice;

class OpenGLGraphicsDeviceEnumerator : public IGraphicsDeviceEnumerator
{
public:
	// Constructors
	explicit OpenGLGraphicsDeviceEnumerator(cobalt::logging::ILogger::unique_ptr log);

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
	// Device methods
	bool ReadOpenGLGraphicsDeviceInfo(EnumerationFlags flags);

private:
	cobalt::logging::ILogger::unique_ptr _log;
	std::vector<std::unique_ptr<OpenGLGraphicsDevice>> _devices;
	std::vector<IGraphicsDevice*> _filteredDevices;
	bool _readDeviceInfo = false;
};

} // namespace cobalt::graphics
