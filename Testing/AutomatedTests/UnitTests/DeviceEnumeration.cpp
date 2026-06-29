// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../UnitTestBase.h"
#include <algorithm>
#include <array>
#include <set>
#include <vector>
namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

namespace {

const std::array<IGraphicsDevice::DeviceType, 4> DeviceTypes = {{
  IGraphicsDevice::DeviceType::Discrete,
  IGraphicsDevice::DeviceType::Integrated,
  IGraphicsDevice::DeviceType::Software,
  IGraphicsDevice::DeviceType::Unknown,
}};

const std::array<IGraphicsDevice::Feature, 15> DeviceFeatures = {{
  IGraphicsDevice::Feature::AnisotropicFiltering,
  IGraphicsDevice::Feature::GeometryShaders,
  IGraphicsDevice::Feature::ComputeShaders,
  IGraphicsDevice::Feature::MeshShaders,
  IGraphicsDevice::Feature::DepthBiasClamp,
  IGraphicsDevice::Feature::IndirectDraw,
  IGraphicsDevice::Feature::IndirectMultiDrawNative,
  IGraphicsDevice::Feature::InstanceOffset,
  IGraphicsDevice::Feature::PolygonWireframeFillMode,
  IGraphicsDevice::Feature::ResourceArrays,
  IGraphicsDevice::Feature::ShaderArraysOfArrays,
  IGraphicsDevice::Feature::SeparateBlendModePerTarget,
  IGraphicsDevice::Feature::SeparateTextureSamplers,
  IGraphicsDevice::Feature::TextureCubeArray,
  IGraphicsDevice::Feature::MipmapLevelBias,
}};

bool ContainsDevice(const std::vector<IGraphicsDevice*>& devices, IGraphicsDevice* targetDevice)
{
	return std::find(devices.begin(), devices.end(), targetDevice) != devices.end();
}

bool ContainsEquivalentDevice(const std::vector<IGraphicsDevice*>& devices, const IGraphicsDevice& targetDevice)
{
	const auto targetVendorName = targetDevice.GetVendorName().Get();
	const auto targetDeviceName = targetDevice.GetDeviceName().Get();
	const auto targetDeviceType = targetDevice.GetDeviceType();
	const auto targetVendor = targetDevice.GetVendor();
	for (const auto* device : devices)
	{
		if ((device->GetVendorName().Get() == targetVendorName) &&
		    (device->GetDeviceName().Get() == targetDeviceName) &&
		    (device->GetDeviceType() == targetDeviceType) &&
		    (device->GetVendor() == targetVendor))
		{
			return true;
		}
	}
	return false;
}

template<class Predicate>
std::vector<IGraphicsDevice*> FilterDevices(const std::vector<IGraphicsDevice*>& devices, Predicate predicate)
{
	std::vector<IGraphicsDevice*> filteredDevices;
	for (auto* device : devices)
	{
		if (predicate(device))
		{
			filteredDevices.push_back(device);
		}
	}
	return filteredDevices;
}

} // namespace

DEFINE_UNIT_TEST_WITH_BASE("Device/DeviceEnumeration", UnitTestBase)
{
	// Device enumeration owns the device objects it returns, so use a fresh enumerator for this test
	// and deliberately avoid creating renderers from any of the enumerated devices.
	auto& rendererPlugin = session.RendererPlugin();
	auto deviceEnumerator = rendererPlugin.CreateGraphicsDeviceEnumerator(session.Log().GetLoggerChildScope("DeviceEnumeration"));
	REQUIRE(deviceEnumerator != nullptr);

	auto enumerationFlags = IGraphicsDeviceEnumerator::EnumerationFlags::None;
#ifdef _DEBUG
	enumerationFlags |= IGraphicsDeviceEnumerator::EnumerationFlags::NativeApiValidation;
#endif
	REQUIRE((enumerationFlags & IGraphicsDeviceEnumerator::EnumerationFlags::NativeApiValidation) == IGraphicsDeviceEnumerator::EnumerationFlags::NativeApiValidation ||
	        (enumerationFlags & IGraphicsDeviceEnumerator::EnumerationFlags::NativeApiValidation) == IGraphicsDeviceEnumerator::EnumerationFlags::None);
	REQUIRE(deviceEnumerator->EnumerateDevices(enumerationFlags));

	auto allDevices = deviceEnumerator->GetAllDevices().Get();
	auto filteredDevices = deviceEnumerator->GetFilteredDevices().Get();
	REQUIRE(deviceEnumerator->FoundDevice());
	REQUIRE(!allDevices.empty());
	REQUIRE(filteredDevices == allDevices);
	REQUIRE(ContainsEquivalentDevice(allDevices, session.Device()));

	auto requirePreferredDeviceMatchesFilteredSet = [&] {
		auto currentFilteredDevices = deviceEnumerator->GetFilteredDevices().Get();
		auto* preferredDevice = deviceEnumerator->GetPreferredDevice();
		if (currentFilteredDevices.empty())
		{
			REQUIRE(preferredDevice == nullptr);
		}
		else
		{
			REQUIRE(preferredDevice != nullptr);
			REQUIRE(ContainsDevice(currentFilteredDevices, preferredDevice));
		}
	};
	requirePreferredDeviceMatchesFilteredSet();
	session.AddTestSuccess("InitialEnumeration", "The device enumerator returned a non-empty device list, initialized the filtered list, and exposed a preferred device from that filtered list.");

	// Filtering a single explicit device removes only that device and leaves the full device list intact.
	deviceEnumerator->FilterDevice(nullptr);
	REQUIRE(deviceEnumerator->GetFilteredDevices().Get() == allDevices);
	auto* firstDevice = allDevices.front();
	deviceEnumerator->FilterDevice(firstDevice);
	auto expectedDevices = FilterDevices(allDevices, [firstDevice](IGraphicsDevice* device) { return device != firstDevice; });
	REQUIRE(deviceEnumerator->GetFilteredDevices().Get() == expectedDevices);
	REQUIRE(deviceEnumerator->GetAllDevices().Get() == allDevices);
	requirePreferredDeviceMatchesFilteredSet();
	deviceEnumerator->ClearDeviceFilters();
	REQUIRE(deviceEnumerator->GetFilteredDevices().Get() == allDevices);
	session.AddTestSuccess("ExplicitDeviceFilter", "Filtering a null device was a no-op, filtering an explicit device removed it, and ClearDeviceFilters restored the complete device set.");

	// Type filters are exclusion filters for the named type, and inverse filters retain only that type.
	for (auto deviceType : DeviceTypes)
	{
		deviceEnumerator->ClearDeviceFilters();
		deviceEnumerator->FilterDevicesOfType(deviceType);
		expectedDevices = FilterDevices(allDevices, [deviceType](IGraphicsDevice* device) { return device->GetDeviceType() != deviceType; });
		REQUIRE(deviceEnumerator->GetFilteredDevices().Get() == expectedDevices);
		requirePreferredDeviceMatchesFilteredSet();

		deviceEnumerator->ClearDeviceFilters();
		deviceEnumerator->FilterDevicesNotOfType(deviceType);
		expectedDevices = FilterDevices(allDevices, [deviceType](IGraphicsDevice* device) { return device->GetDeviceType() == deviceType; });
		REQUIRE(deviceEnumerator->GetFilteredDevices().Get() == expectedDevices);
		requirePreferredDeviceMatchesFilteredSet();
	}
	deviceEnumerator->ClearDeviceFilters();
	REQUIRE(deviceEnumerator->GetFilteredDevices().Get() == allDevices);
	session.AddTestSuccess("DeviceTypeFilters", "Device type inclusion and exclusion filters produced the expected filtered device sets.");

	// Feature filters retain only devices that support the requested feature set.
	for (auto feature : DeviceFeatures)
	{
		deviceEnumerator->ClearDeviceFilters();
		deviceEnumerator->FilterDevicesWithoutFeature(feature);
		expectedDevices = FilterDevices(allDevices, [feature](IGraphicsDevice* device) { return device->IsFeatureSupported(feature); });
		REQUIRE(deviceEnumerator->GetFilteredDevices().Get() == expectedDevices);
		requirePreferredDeviceMatchesFilteredSet();

		deviceEnumerator->ClearDeviceFilters();
		std::set<IGraphicsDevice::Feature> singleFeatureSet = {feature};
		deviceEnumerator->FilterDevicesWithoutAllFeatures(singleFeatureSet);
		REQUIRE(deviceEnumerator->GetFilteredDevices().Get() == expectedDevices);
		requirePreferredDeviceMatchesFilteredSet();
	}
	deviceEnumerator->ClearDeviceFilters();
	std::set<IGraphicsDevice::Feature> emptyFeatureSet;
	deviceEnumerator->FilterDevicesWithoutAllFeatures(emptyFeatureSet);
	REQUIRE(deviceEnumerator->GetFilteredDevices().Get() == allDevices);

	for (auto* device : allDevices)
	{
		deviceEnumerator->ClearDeviceFilters();
		auto deviceFeatureSet = device->GetAllSupportedFeatures().Get();
		deviceEnumerator->FilterDevicesWithoutAllFeatures(deviceFeatureSet);
		expectedDevices = FilterDevices(allDevices, [&deviceFeatureSet](IGraphicsDevice* candidateDevice) { return candidateDevice->AreAllFeaturesSupported(deviceFeatureSet); });
		REQUIRE(deviceEnumerator->GetFilteredDevices().Get() == expectedDevices);
		requirePreferredDeviceMatchesFilteredSet();
	}
	session.AddTestSuccess("FeatureFilters", "Single-feature, aggregate-feature, and empty feature-set filters produced the expected filtered device sets.");

	// Filtering out every device should produce an empty filtered set and no preferred device.
	deviceEnumerator->ClearDeviceFilters();
	for (auto* device : allDevices)
	{
		deviceEnumerator->FilterDevice(device);
	}
	REQUIRE(deviceEnumerator->GetFilteredDevices().Get().empty());
	REQUIRE(deviceEnumerator->GetPreferredDevice() == nullptr);
	REQUIRE(deviceEnumerator->FoundDevice());
	deviceEnumerator->ClearDeviceFilters();
	REQUIRE(deviceEnumerator->GetFilteredDevices().Get() == allDevices);
	requirePreferredDeviceMatchesFilteredSet();
	session.AddTestSuccess("EmptyAndRestoredFilters", "Filtering out every device removed the preferred device, and clearing filters restored the full enumerated list.");

	// Re-enumeration must rebuild the device list and reset any active filters.
	deviceEnumerator->FilterDevice(allDevices.front());
	REQUIRE(deviceEnumerator->GetFilteredDevices().Get().size() + 1 == allDevices.size());
	REQUIRE(deviceEnumerator->EnumerateDevices(enumerationFlags));
	allDevices = deviceEnumerator->GetAllDevices().Get();
	filteredDevices = deviceEnumerator->GetFilteredDevices().Get();
	REQUIRE(deviceEnumerator->FoundDevice());
	REQUIRE(!allDevices.empty());
	REQUIRE(filteredDevices == allDevices);
	REQUIRE(ContainsEquivalentDevice(allDevices, session.Device()));
	requirePreferredDeviceMatchesFilteredSet();
	session.AddTestSuccess("ReenumerationResetsFilters", "Enumerating devices again rebuilt the device list and reset the filtered device set.");

	return true;
}

} // namespace cobalt::graphics::testing
