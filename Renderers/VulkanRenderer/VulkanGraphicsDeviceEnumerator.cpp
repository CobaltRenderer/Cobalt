// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "VulkanGraphicsDeviceEnumerator.h"
#include "AssemblyVersionInfo.h"
#include "SolutionVersionInfo.h"
#include "VulkanGraphicsDevice.h"
#include "VulkanHeaders.h"
#include "VulkanInstanceData.h"
#include <Cobalt/RendererInterface/PlatformBindings.pkg>
#include <Internal/RendererSupport/KnownDynamicCast.h>
#include <cstdlib>
#include <cstring>
#include <memory>
#ifdef __APPLE__
#include <dlfcn.h>
#include <filesystem>
#endif
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
VulkanGraphicsDeviceEnumerator::VulkanGraphicsDeviceEnumerator(cobalt::logging::ILogger::unique_ptr log)
{
	_log = (log != nullptr ? std::move(log) : cobalt::logging::ILogger::unique_ptr(new cobalt::logging::NullLogger()));
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void VulkanGraphicsDeviceEnumerator::Delete()
{
	delete this;
}

//----------------------------------------------------------------------------------------
// Device methods
//----------------------------------------------------------------------------------------
#ifdef __APPLE__
namespace {
[[gnu::noinline]]
std::filesystem::path GetCurrentModulePath()
{
	Dl_info moduleInfo = {};
	if (dladdr(reinterpret_cast<const void*>(&GetCurrentModulePath), &moduleInfo) == 0)
	{
		return {};
	}
	return std::filesystem::path(moduleInfo.dli_fname);
}
} // namespace
#endif

//----------------------------------------------------------------------------------------
SuccessToken VulkanGraphicsDeviceEnumerator::EnumerateDevices(EnumerationFlags flags)
{
	// Clear any existing device entries
	_devices.clear();
	_filteredDevices.clear();

	// Extract our enumeration flags
	bool enableValidationLayers = ((flags & EnumerationFlags::NativeApiValidation) != EnumerationFlags::None);
	bool headlessRendering = ((flags & EnumerationFlags::HeadlessRendering) != EnumerationFlags::None);

	// On macOS we have no native Vulkan support, and rely on a MoltenVK distribution and Vulkan loader being
	// distributed alongside our renderer. In this case, we need to prime the loader with VK_ICD_FILENAMES so that it
	// can find MoltenVK.
#ifdef __APPLE__
	if (std::getenv("VK_ICD_FILENAMES") == nullptr)
	{
		auto modulePath = GetCurrentModulePath();
		if (!modulePath.empty())
		{
			auto icdFilePath = modulePath.parent_path() / "MoltenVK_icd.json";
			setenv("VK_ICD_FILENAMES", icdFilePath.string().c_str(), 0);
		}
		else
		{
			_log->Warning("Failed to determine Vulkan renderer module path for macOS Vulkan loader fallback.");
		}
	}
#endif

	// Build a list of all available instance extensions. Note that it's explicitly documented that this list may change
	// between calls, so we need to be thorough in retrieving this list to ensure it's complete and fully populated.
	std::vector<VkExtensionProperties> availableInstanceExtensions;
	VkResult vkEnumerateInstanceExtensionPropertiesReturn;
	do
	{
		uint32_t availableInstanceExtensionCount;
		vkEnumerateInstanceExtensionPropertiesReturn = vkEnumerateInstanceExtensionProperties(nullptr, &availableInstanceExtensionCount, nullptr);
		if (vkEnumerateInstanceExtensionPropertiesReturn != VK_SUCCESS)
		{
			continue;
		}
		availableInstanceExtensions.resize(availableInstanceExtensionCount);
		vkEnumerateInstanceExtensionPropertiesReturn = vkEnumerateInstanceExtensionProperties(nullptr, &availableInstanceExtensionCount, availableInstanceExtensions.data());
		availableInstanceExtensions.resize(availableInstanceExtensionCount);
	} while (vkEnumerateInstanceExtensionPropertiesReturn == VK_INCOMPLETE);
	if (vkEnumerateInstanceExtensionPropertiesReturn != VK_SUCCESS)
	{
		_log->Error("vkEnumerateInstanceExtensionProperties failed with error code {0}", vkEnumerateInstanceExtensionPropertiesReturn);
		return false;
	}

	// Build a list of all available instance layers. Note that it's explicitly documented that this list may change
	// between calls, so we need to be thorough in retrieving this list to ensure it's complete and fully populated.
	std::vector<VkLayerProperties> availableInstanceLayers;
	VkResult vkEnumerateInstanceLayerPropertiesReturn;
	do
	{
		uint32_t availableInstanceLayerCount;
		vkEnumerateInstanceLayerPropertiesReturn = vkEnumerateInstanceLayerProperties(&availableInstanceLayerCount, nullptr);
		if (vkEnumerateInstanceLayerPropertiesReturn != VK_SUCCESS)
		{
			continue;
		}
		availableInstanceLayers.resize(availableInstanceLayerCount);
		vkEnumerateInstanceLayerPropertiesReturn = vkEnumerateInstanceLayerProperties(&availableInstanceLayerCount, availableInstanceLayers.data());
		availableInstanceLayers.resize(availableInstanceLayerCount);
	} while (vkEnumerateInstanceLayerPropertiesReturn == VK_INCOMPLETE);
	if (vkEnumerateInstanceLayerPropertiesReturn != VK_SUCCESS)
	{
		_log->Error("vkEnumerateInstanceLayerProperties failed with error code {0}", vkEnumerateInstanceLayerPropertiesReturn);
		return false;
	}

	// Print available instance extensions
	// #ifdef _DEBUG
	// 	_log->Debug("Available instance extensions");
	// 	for (auto& extension : extensions)
	// 	{
	// 		_log->Debug("\t{0}", extension.extensionName);
	// 	}
	// #endif

	// Print available instance layers
	// #ifdef _DEBUG
	// 	_log->Debug("Available instance layers");
	// 	for (auto& layer : layers)
	// 	{
	// 		_log->Debug("\t{0}", layer.layerName);
	// 	}
	// #endif

	// Check on the avaiability of optional instance extensions we might want
	bool debugUtilsAvailable = false;
	for (const auto& extension : availableInstanceExtensions)
	{
		if (std::strcmp(&extension.extensionName[0], VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0)
		{
			debugUtilsAvailable = true;
			break;
		}
	}

	// Check on the avaiability of optional instance layers we might want. Note that for validation layers, we assume
	// here that the validation layer is discoverable by the loader, meaning it has either been correctly registered
	// system-wide, or that paths to it have been defined externally through VK_LAYER_PATH and VK_ICD_FILENAMES where
	// necessary.
	bool validationLayerAvailable = false;
	std::string validationLayerName = "VK_LAYER_KHRONOS_validation";
	if (enableValidationLayers)
	{
		for (const auto& layer : availableInstanceLayers)
		{
			if (validationLayerName == &layer.layerName[0])
			{
				validationLayerAvailable = true;
				break;
			}
		}
	}

	// Disable validation layers if required extensions/layers aren't available
	if (enableValidationLayers && (!debugUtilsAvailable || !validationLayerAvailable))
	{
		if (!debugUtilsAvailable)
		{
			_log->Warning("Validation layers are being disabled because instance extension '{0}' is not available", VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
		else
		{
			_log->Warning("Validation layers are being disabled because layer '{0}' is not available", validationLayerName);
		}
		enableValidationLayers = false;
	}

	// Build the list of required instance extensions
	std::vector<const char*> requiredInstanceExtensions;
	if (!headlessRendering)
	{
		requiredInstanceExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
#ifdef COBALT_RENDERER_WIN32_SUPPORT
		requiredInstanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif
#ifdef COBALT_RENDERER_XLIB_SUPPORT
		requiredInstanceExtensions.push_back(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
#endif
#ifdef COBALT_RENDERER_XCB_SUPPORT
		requiredInstanceExtensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#endif
#ifdef COBALT_RENDERER_WAYLAND_SUPPORT
		requiredInstanceExtensions.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
#endif
#ifdef COBALT_RENDERER_APPKIT_SUPPORT
		requiredInstanceExtensions.push_back(VK_EXT_METAL_SURFACE_EXTENSION_NAME);
#endif
	}
	if (debugUtilsAvailable)
	{
		// We activate debug utils by default for the extension here, regardless of whether validation layers have been
		// requested, so we can opt-in to render marker support later. Testing has shown no noticeable performance
		// impact from doing this, and that appears to be supported by documentation. Enabling an instance extension
		// merely makes new endpoints available, but if we don't end up using them, there's no further performance cost.
		requiredInstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}
#ifdef __APPLE__
	// Enable extra required instance extensions when running MoltekVK on macOS
	requiredInstanceExtensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
	requiredInstanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#endif

	// Build the list of required instance layers
	std::vector<const char*> requiredLayers;
	if (enableValidationLayers)
	{
		requiredLayers.push_back(validationLayerName.c_str());
	}

	// Populate the Vulkan instance creation information structure
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = nullptr;
	appInfo.applicationVersion = 0;
	appInfo.pEngineName = SolutionVersionInfo_ProductName;
	appInfo.engineVersion = VK_MAKE_VERSION(AssemblyVersionInfo_VersionDigit_Major, AssemblyVersionInfo_VersionDigit_Minor, AssemblyVersionInfo_VersionDigit_Revision);
	appInfo.apiVersion = VK_API_VERSION_1_1;
	VkInstanceCreateInfo instanceInfo = {};
	instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
#ifdef __APPLE__
	// We need to enumerate portability devices on macOS for MoltenVK support
	instanceInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#else
	instanceInfo.flags = 0;
#endif
	instanceInfo.pApplicationInfo = &appInfo;
	instanceInfo.enabledExtensionCount = (uint32_t)requiredInstanceExtensions.size();
	instanceInfo.ppEnabledExtensionNames = requiredInstanceExtensions.data();
	instanceInfo.enabledLayerCount = (uint32_t)requiredLayers.size();
	instanceInfo.ppEnabledLayerNames = requiredLayers.data();

	// Create the Vulkan instance
	//##FIX## On Linux at least, this likes to leak memory. Not a lot, about 1-2kb, but calling vkCreateInstance
	//immediately followed by vkDestroyInstance results in address sanitizer detecting 1337 (yes really) bytes leaked
	//in 6 allocations on close, at least when testing on Ubuntu 22.04. The amount of memory and number of leaked
	//allocations varies slightly after actual use of the API, but there's always a few kb of memory leaked by this
	//operation we don't get back. This is apparently a common, well known problem.
	VkInstance vulkanInstance;
	VkResult vkCreateInstanceReturn = vkCreateInstance(&instanceInfo, nullptr, &vulkanInstance);
	if (vkCreateInstanceReturn == VK_ERROR_INCOMPATIBLE_DRIVER)
	{
		_log->Warning("vkCreateInstance reported no compatible Vulkan driver. No Vulkan devices will be enumerated.");
		return true;
	}
	if (vkCreateInstanceReturn != VK_SUCCESS)
	{
		_log->Error("vkCreateInstance failed with error code {0}. Please ensure you have at least one Vulkan capable graphics device installed and your graphics card drivers are up to date.", vkCreateInstanceReturn);
		return false;
	}

	// Create a shared instance structure to pass to each VulkanGraphicsDevice object. This gives us a way to manage the
	// lifetime of the shared Vulkan instance object, and any other instance-related objects or state that hangs off it,
	// such as the debug message logging.
	std::shared_ptr<VulkanInstanceData> instanceData = std::make_shared<VulkanInstanceData>(_log->CloneLogger(), vulkanInstance);
	instanceData->enableValidationLayers = enableValidationLayers;
	instanceData->debugUtilsAvailable = debugUtilsAvailable;

	// If validation layers are enabled, turn on debug logging with the necessary verbosity.
	if (enableValidationLayers)
	{
		// Configure debug message filtering settings
		std::vector<std::string> ignoredMessageNames;
		ignoredMessageNames.emplace_back("UNASSIGNED-CoreValidation-DrawState-VtxIndexOutOfBounds");
		ignoredMessageNames.emplace_back("UNASSIGNED-CoreValidation-DrawState-DescriptorSetNotUpdated");
		ignoredMessageNames.emplace_back("VUID-vkCmdDrawIndexed-None-04007");
		ignoredMessageNames.emplace_back("VUID-vkCmdDrawIndexed-None-04008");
		if (!instanceData->RegisterDebugMessengerCallback(VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT, ignoredMessageNames))
		{
			_log->Warning("RegisterDebugMessengerCallback failed. Debug logging will not be available.");
		}
	}

	// Retrieve the set of available physical devices. It's not explicitly stated if this list is able to change
	// dynamically between calls like it is for vkEnumerateInstanceExtensionProperties, which presumably means it can't.
	// Most likely the list of devices is fixed and static, being determined when vkCreateInstance is called. This is
	// further supported by the fact these returned device handles do not need to be explicitly freed. None the less, to
	// be defensive against errors here, we take care here to ensure the list is complete and fully populated, even if
	// it is possible for the device list to change between successive calls.
	std::vector<VkPhysicalDevice> physicalDevices;
	VkResult vkEnumeratePhysicalDevicesReturn;
	do
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevicesReturn = vkEnumeratePhysicalDevices(vulkanInstance, &deviceCount, nullptr);
		if (vkEnumeratePhysicalDevicesReturn != VK_SUCCESS)
		{
			continue;
		}
		physicalDevices.resize(deviceCount);
		vkEnumeratePhysicalDevicesReturn = vkEnumeratePhysicalDevices(vulkanInstance, &deviceCount, physicalDevices.data());
		physicalDevices.resize(deviceCount);
	} while (vkEnumeratePhysicalDevicesReturn == VK_INCOMPLETE);
	if (vkEnumeratePhysicalDevicesReturn != VK_SUCCESS)
	{
		_log->Error("vkEnumeratePhysicalDevices failed with error code {0}", vkEnumeratePhysicalDevicesReturn);
		return false;
	}

	// Build the list of device extensions to request. Note that this is just an initial list passed into the graphics
	// device itself. It can be extended by subsequent options passed in before it needs to be used for actual device
	// creation.
	std::vector<const char*> requiredDeviceExtensions;
	// We need this to allow the viewport to be flipped to match Direct3D/Metal.
	requiredDeviceExtensions.push_back(VK_KHR_MAINTENANCE1_EXTENSION_NAME);
	// We need this for the nullDescriptor feature. Note that although currently this is not supported on MoltenVk:
	// https://github.com/KhronosGroup/MoltenVK/issues/2650
	// https://github.com/KhronosGroup/MoltenVK/issues/214#issuecomment-725221517
	// The "VK_EXT_robustness2" extension itself is supported, just not nullDescriptor, so we request it here anyway.
	requiredDeviceExtensions.push_back("VK_EXT_robustness2");
	if (!headlessRendering)
	{
		requiredDeviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	}

	// Generate VulkanGraphicDevice objects for each physical device
	for (VkPhysicalDevice device : physicalDevices)
	{
		bool requiredExtensionMissing = false;
		auto graphicsDevice = std::make_unique<VulkanGraphicsDevice>(_log.get(), instanceData, device);
		for (auto requiredExtension : requiredDeviceExtensions)
		{
			if (!graphicsDevice->SupportsDeviceExtension(requiredExtension))
			{
				_log->Info(R"(Excluded device with name "{0}" from vendor "{1}" because it doesn't support the required device extension "{2}")", graphicsDevice->GetDeviceName().Get(), graphicsDevice->GetVendorName().Get(), requiredExtension);
				requiredExtensionMissing = true;
			}
		}
		if (requiredExtensionMissing)
		{
			continue;
		}
		_devices.push_back(std::move(graphicsDevice));
	}

	// Initialize the filtered device set
	ClearDeviceFilters();

	// Return true to the caller
	return true;
}

//----------------------------------------------------------------------------------------
bool VulkanGraphicsDeviceEnumerator::FoundDevice() const
{
	return !_devices.empty();
}

//----------------------------------------------------------------------------------------
Marshal::Ret<std::vector<IGraphicsDevice*>> VulkanGraphicsDeviceEnumerator::GetAllDevices() const
{
	std::vector<IGraphicsDevice*> deviceSet;
	deviceSet.reserve(_devices.size());
	for (const auto& device : _devices)
	{
		deviceSet.push_back(device.get());
	}
	return deviceSet;
}

//----------------------------------------------------------------------------------------
Marshal::Ret<std::vector<IGraphicsDevice*>> VulkanGraphicsDeviceEnumerator::GetFilteredDevices() const
{
	return _filteredDevices;
}

//----------------------------------------------------------------------------------------
IGraphicsDevice* VulkanGraphicsDeviceEnumerator::GetPreferredDevice() const
{
	// Find the best devices of each type, based on their reported dedicated memory availability.
	IGraphicsDevice* bestDedicated = nullptr;
	IGraphicsDevice* bestIntegrated = nullptr;
	IGraphicsDevice* bestSoftware = nullptr;
	for (IGraphicsDevice* graphicsDevice : _filteredDevices)
	{
		auto deviceType = graphicsDevice->GetDeviceType();
		if (deviceType == IGraphicsDevice::DeviceType::Discrete)
		{
			if ((bestDedicated == nullptr) || (bestDedicated->GetMemorySizeInBytes(IGraphicsDevice::MemoryType::Dedicated) < graphicsDevice->GetMemorySizeInBytes(IGraphicsDevice::MemoryType::Dedicated)))
			{
				bestDedicated = graphicsDevice;
			}
		}
		else if (deviceType == IGraphicsDevice::DeviceType::Integrated)
		{
			if ((bestIntegrated == nullptr) || (bestIntegrated->GetMemorySizeInBytes(IGraphicsDevice::MemoryType::Dedicated) < graphicsDevice->GetMemorySizeInBytes(IGraphicsDevice::MemoryType::Dedicated)))
			{
				bestIntegrated = graphicsDevice;
			}
		}
		else
		{
			if ((bestSoftware == nullptr) || (bestSoftware->GetMemorySizeInBytes(IGraphicsDevice::MemoryType::Dedicated) < graphicsDevice->GetMemorySizeInBytes(IGraphicsDevice::MemoryType::Dedicated)))
			{
				bestSoftware = graphicsDevice;
			}
		}
	}

	// Prefer dedicated, then integrated, and finally software.
	if (bestDedicated != nullptr)
	{
		return bestDedicated;
	}
	if (bestIntegrated != nullptr)
	{
		return bestIntegrated;
	}
	return bestSoftware;
}

//----------------------------------------------------------------------------------------
// Filtering methods
//----------------------------------------------------------------------------------------
void VulkanGraphicsDeviceEnumerator::FilterDevice(IGraphicsDevice* targetDevice)
{
	// Iterate all remaining filtered devices, and remove any devices that don't match the filter requirement.
	auto deviceIterator = _filteredDevices.begin();
	while (deviceIterator != _filteredDevices.end())
	{
		// Determine if we should filter this device
		auto* device = *deviceIterator;
		bool filterOutDevice = (device == targetDevice);

		// Erase or retain this device in the filtered device list as required
		if (filterOutDevice)
		{
			deviceIterator = _filteredDevices.erase(deviceIterator);
		}
		else
		{
			++deviceIterator;
		}
	}
}

//----------------------------------------------------------------------------------------
void VulkanGraphicsDeviceEnumerator::FilterDevicesOfType(IGraphicsDevice::DeviceType type)
{
	// Iterate all remaining filtered devices, and remove any devices that don't match the filter requirement.
	auto deviceIterator = _filteredDevices.begin();
	while (deviceIterator != _filteredDevices.end())
	{
		// Determine if we should filter this device
		auto* device = *deviceIterator;
		bool filterOutDevice = (device->GetDeviceType() == type);

		// Erase or retain this device in the filtered device list as required
		if (filterOutDevice)
		{
			deviceIterator = _filteredDevices.erase(deviceIterator);
		}
		else
		{
			++deviceIterator;
		}
	}
}

//----------------------------------------------------------------------------------------
void VulkanGraphicsDeviceEnumerator::FilterDevicesNotOfType(IGraphicsDevice::DeviceType type)
{
	// Iterate all remaining filtered devices, and remove any devices that don't match the filter requirement.
	auto deviceIterator = _filteredDevices.begin();
	while (deviceIterator != _filteredDevices.end())
	{
		// Determine if we should filter this device
		auto* device = *deviceIterator;
		bool filterOutDevice = (device->GetDeviceType() != type);

		// Erase or retain this device in the filtered device list as required
		if (filterOutDevice)
		{
			deviceIterator = _filteredDevices.erase(deviceIterator);
		}
		else
		{
			++deviceIterator;
		}
	}
}

//----------------------------------------------------------------------------------------
void VulkanGraphicsDeviceEnumerator::FilterDevicesWithoutFeature(IGraphicsDevice::Feature feature)
{
	// Iterate all remaining filtered devices, and remove any devices that don't match the filter requirement.
	auto deviceIterator = _filteredDevices.begin();
	while (deviceIterator != _filteredDevices.end())
	{
		// Determine if we should filter this device
		auto* device = *deviceIterator;
		bool filterOutDevice = !device->IsFeatureSupported(feature);

		// Erase or retain this device in the filtered device list as required
		if (filterOutDevice)
		{
			deviceIterator = _filteredDevices.erase(deviceIterator);
		}
		else
		{
			++deviceIterator;
		}
	}
}

//----------------------------------------------------------------------------------------
void VulkanGraphicsDeviceEnumerator::FilterDevicesWithoutAllFeatures(const Marshal::In<std::set<IGraphicsDevice::Feature>>& featureSet)
{
	// Iterate all remaining filtered devices, and remove any devices that don't match the filter requirement.
	auto deviceIterator = _filteredDevices.begin();
	while (deviceIterator != _filteredDevices.end())
	{
		// Determine if we should filter this device
		auto* device = *deviceIterator;
		bool filterOutDevice = !device->AreAllFeaturesSupported(featureSet);

		// Erase or retain this device in the filtered device list as required
		if (filterOutDevice)
		{
			deviceIterator = _filteredDevices.erase(deviceIterator);
		}
		else
		{
			++deviceIterator;
		}
	}
}

//----------------------------------------------------------------------------------------
void VulkanGraphicsDeviceEnumerator::ClearDeviceFilters()
{
	_filteredDevices.clear();
	for (const auto& device : _devices)
	{
		_filteredDevices.push_back(device.get());
	}
}

} // namespace cobalt::graphics
