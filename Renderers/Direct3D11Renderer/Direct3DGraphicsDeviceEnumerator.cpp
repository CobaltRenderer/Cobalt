// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "Direct3DGraphicsDeviceEnumerator.h"
#include "Direct3DGraphicsDevice.h"
#include <numeric>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
Direct3DGraphicsDeviceEnumerator::Direct3DGraphicsDeviceEnumerator(cobalt::logging::ILogger::unique_ptr log)
{
	_log = (log != nullptr ? std::move(log) : cobalt::logging::ILogger::unique_ptr(new cobalt::logging::NullLogger()));
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void Direct3DGraphicsDeviceEnumerator::Delete()
{
	delete this;
}

//----------------------------------------------------------------------------------------
// Device methods
//----------------------------------------------------------------------------------------
SuccessToken Direct3DGraphicsDeviceEnumerator::EnumerateDevices(EnumerationFlags flags)
{
	// Clear any existing device entries
	_devices.clear();
	_filteredDeviceIndices.clear();

	// Create a DXGI factory
	HRESULT createDxgiFactoryReturn = CreateDXGIFactory1(IID_PPV_ARGS(&_dxgiFactory));
	if (FAILED(createDxgiFactoryReturn))
	{
		_log->Error("CreateDXGIFactory1 failed with error code {0}", createDxgiFactoryReturn);
		return false;
	}

	// Enumerate all available adapters, and record information on each one.
	Microsoft::WRL::ComPtr<IDXGIAdapter1> adapterV1;
	uint32_t adapterNo = 0;
	while (!FAILED(_dxgiFactory->EnumAdapters1(adapterNo, &adapterV1)))
	{
		// Cast the adapter to the required type
		Microsoft::WRL::ComPtr<IDXGIAdapter2> adapter;
		HRESULT castAdapterReturn = adapterV1.As(&adapter);
		if (FAILED(castAdapterReturn))
		{
			_log->Warning("Failed to cast adapter to IDXGIAdapter2 with error code {0}", castAdapterReturn);
			++adapterNo;
			continue;
		}

		// Retrieve information on the adapter
		DXGI_ADAPTER_DESC2 adapterDescription;
		HRESULT getAdapterDescriptionReturn = adapter->GetDesc2(&adapterDescription);
		if (FAILED(getAdapterDescriptionReturn))
		{
			_log->Warning("Failed to retrieve adapter description with error code {0}", getAdapterDescriptionReturn);
			++adapterNo;
			continue;
		}

		// Create a device object to represent this adapter
		std::unique_ptr<Direct3DGraphicsDevice> device = std::make_unique<Direct3DGraphicsDevice>(_log.get(), _dxgiFactory, adapter, adapterDescription);

		// Record information on this adapter in the list of adapters
		DeviceEntry deviceEntry;
		deviceEntry.adapter = adapter;
		deviceEntry.device = std::move(device);
		deviceEntry.recommendedDevice = (adapterNo == 0);
		_devices.push_back(std::move(deviceEntry));
		++adapterNo;
	}

	// If no devices were located, log a warning.
	if (_devices.empty())
	{
		_log->Warning("No compatible Direct3D 11 devices were located during device enumeration.");
	}

	// Initialize the device filters and return true to the caller
	ClearDeviceFilters();
	return true;
}

//----------------------------------------------------------------------------------------
bool Direct3DGraphicsDeviceEnumerator::FoundDevice() const
{
	return !_devices.empty();
}

//----------------------------------------------------------------------------------------
Marshal::Ret<std::vector<IGraphicsDevice*>> Direct3DGraphicsDeviceEnumerator::GetAllDevices() const
{
	std::vector<IGraphicsDevice*> deviceSet;
	deviceSet.reserve(_devices.size());
	for (const DeviceEntry& deviceEntry : _devices)
	{
		deviceSet.push_back(deviceEntry.device.get());
	}
	return deviceSet;
}

//----------------------------------------------------------------------------------------
Marshal::Ret<std::vector<IGraphicsDevice*>> Direct3DGraphicsDeviceEnumerator::GetFilteredDevices() const
{
	std::vector<IGraphicsDevice*> deviceSet;
	deviceSet.reserve(_filteredDeviceIndices.size());
	for (auto deviceIndex : _filteredDeviceIndices)
	{
		deviceSet.push_back(_devices[deviceIndex].device.get());
	}
	return deviceSet;
}

//----------------------------------------------------------------------------------------
IGraphicsDevice* Direct3DGraphicsDeviceEnumerator::GetPreferredDevice() const
{
	// Determine the preferred adapter from the set of available adapters. Note that if two adapters are equivalent in
	// terms of our preferences, the first listed device wins. This is important, as per the documentation for the
	// EnumAdapters1 method, the list of returned devices is already listed in order of significance, at least in terms
	// of which devices are connected to which displays, which are aspects we don't consider. As such, if we consider
	// two devices to be equivalent, we defer to the logic in EnumAdapters1 to break the tie.
	const DeviceEntry* bestDevice = nullptr;
	for (auto deviceIndex : _filteredDeviceIndices)
	{
		// If we haven't latched a device as our best device so far, capture the first one and advance to the next
		// entry.
		const DeviceEntry* targetDevice = &_devices[deviceIndex];
		if (bestDevice == nullptr)
		{
			bestDevice = targetDevice;
			continue;
		}

		// If the best device found so far is an integrated device and the target device is not, set the target device
		// as the new best device, and advance to the next entry.
		bool bestDeviceIsIntegrated = (bestDevice->device->GetDeviceType() == IGraphicsDevice::DeviceType::Integrated);
		bool targetDeviceIsIntegrated = (targetDevice->device->GetDeviceType() == IGraphicsDevice::DeviceType::Integrated);
		if (bestDeviceIsIntegrated && !targetDeviceIsIntegrated)
		{
			bestDevice = targetDevice;
			continue;
		}

		// If the best device isn't integrated and the target device is, skip it.
		if (!bestDeviceIsIntegrated && targetDeviceIsIntegrated)
		{
			continue;
		}

		// If the target device is the recommended device, take it over our best match so far, and advance to the next
		// entry.
		if (targetDevice->recommendedDevice)
		{
			bestDevice = targetDevice;
			continue;
		}

		// Retrieve memory info on each device
		uint64_t bestDeviceDedicatedMemory = bestDevice->device->GetMemorySizeInBytes(IGraphicsDevice::MemoryType::Dedicated);
		uint64_t bestDeviceSharedMemory = bestDevice->device->GetMemorySizeInBytes(IGraphicsDevice::MemoryType::Shared);
		uint64_t targetDeviceDedicatedMemory = targetDevice->device->GetMemorySizeInBytes(IGraphicsDevice::MemoryType::Dedicated);
		uint64_t targetDeviceSharedMemory = targetDevice->device->GetMemorySizeInBytes(IGraphicsDevice::MemoryType::Shared);

		// If the target device has more memory than our best device, set the target device as the new best device, and
		// advance to the next entry. We favour dedicated memory here over shared memory.
		bool devicesHaveNoDedicatedMemory = ((bestDeviceDedicatedMemory == 0) || (targetDeviceDedicatedMemory == 0));
		uint64_t bestDeviceTargetMemorySizeInBytes = (devicesHaveNoDedicatedMemory) ? bestDeviceSharedMemory : bestDeviceDedicatedMemory;
		uint64_t targetDeviceTargetMemorySizeInBytes = (devicesHaveNoDedicatedMemory) ? targetDeviceSharedMemory : targetDeviceDedicatedMemory;
		if (targetDeviceTargetMemorySizeInBytes > bestDeviceTargetMemorySizeInBytes)
		{
			bestDevice = targetDevice;
			continue;
		}
	}

	// Return the best device we found to the caller
	if (bestDevice == nullptr)
	{
		return nullptr;
	}
	return bestDevice->device.get();
}

//----------------------------------------------------------------------------------------
// Filtering methods
//----------------------------------------------------------------------------------------
void Direct3DGraphicsDeviceEnumerator::FilterDevice(IGraphicsDevice* targetDevice)
{
	// Iterate all remaining filtered devices, and remove any devices that don't match the filter requirement.
	auto deviceIterator = _filteredDeviceIndices.begin();
	while (deviceIterator != _filteredDeviceIndices.end())
	{
		// Determine if we should filter this device
		const auto& device = _devices[*deviceIterator];
		bool filterOutDevice = (device.device.get() == targetDevice);

		// Erase or retain this device in the filtered device list as required
		if (filterOutDevice)
		{
			deviceIterator = _filteredDeviceIndices.erase(deviceIterator);
		}
		else
		{
			++deviceIterator;
		}
	}
}

//----------------------------------------------------------------------------------------
void Direct3DGraphicsDeviceEnumerator::FilterDevicesOfType(IGraphicsDevice::DeviceType type)
{
	// Iterate all remaining filtered devices, and remove any devices that don't match the filter requirement.
	auto deviceIterator = _filteredDeviceIndices.begin();
	while (deviceIterator != _filteredDeviceIndices.end())
	{
		// Determine if we should filter this device
		const auto& device = _devices[*deviceIterator];
		bool filterOutDevice = (device.device->GetDeviceType() == type);

		// Erase or retain this device in the filtered device list as required
		if (filterOutDevice)
		{
			deviceIterator = _filteredDeviceIndices.erase(deviceIterator);
		}
		else
		{
			++deviceIterator;
		}
	}
}

//----------------------------------------------------------------------------------------
void Direct3DGraphicsDeviceEnumerator::FilterDevicesNotOfType(IGraphicsDevice::DeviceType type)
{
	// Iterate all remaining filtered devices, and remove any devices that don't match the filter requirement.
	auto deviceIterator = _filteredDeviceIndices.begin();
	while (deviceIterator != _filteredDeviceIndices.end())
	{
		// Determine if we should filter this device
		const auto& device = _devices[*deviceIterator];
		bool filterOutDevice = (device.device->GetDeviceType() != type);

		// Erase or retain this device in the filtered device list as required
		if (filterOutDevice)
		{
			deviceIterator = _filteredDeviceIndices.erase(deviceIterator);
		}
		else
		{
			++deviceIterator;
		}
	}
}

//----------------------------------------------------------------------------------------
void Direct3DGraphicsDeviceEnumerator::FilterDevicesWithoutFeature(IGraphicsDevice::Feature feature)
{
	// Iterate all remaining filtered devices, and remove any devices that don't match the filter requirement.
	auto deviceIterator = _filteredDeviceIndices.begin();
	while (deviceIterator != _filteredDeviceIndices.end())
	{
		// Determine if we should filter this device
		const auto& device = _devices[*deviceIterator];
		bool filterOutDevice = !device.device->IsFeatureSupported(feature);

		// Erase or retain this device in the filtered device list as required
		if (filterOutDevice)
		{
			deviceIterator = _filteredDeviceIndices.erase(deviceIterator);
		}
		else
		{
			++deviceIterator;
		}
	}
}

//----------------------------------------------------------------------------------------
void Direct3DGraphicsDeviceEnumerator::FilterDevicesWithoutAllFeatures(const Marshal::In<std::set<IGraphicsDevice::Feature>>& featureSet)
{
	// Iterate all remaining filtered devices, and remove any devices that don't match the filter requirement.
	auto deviceIterator = _filteredDeviceIndices.begin();
	while (deviceIterator != _filteredDeviceIndices.end())
	{
		// Determine if we should filter this device
		const auto& device = _devices[*deviceIterator];
		bool filterOutDevice = !device.device->AreAllFeaturesSupported(featureSet);

		// Erase or retain this device in the filtered device list as required
		if (filterOutDevice)
		{
			deviceIterator = _filteredDeviceIndices.erase(deviceIterator);
		}
		else
		{
			++deviceIterator;
		}
	}
}

//----------------------------------------------------------------------------------------
void Direct3DGraphicsDeviceEnumerator::ClearDeviceFilters()
{
	_filteredDeviceIndices.resize(_devices.size());
	std::iota(_filteredDeviceIndices.begin(), _filteredDeviceIndices.end(), 0);
}

} // namespace cobalt::graphics
