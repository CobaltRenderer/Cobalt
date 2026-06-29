// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#ifdef __APPLE__
// For VkPhysicalDevicePortabilitySubsetPropertiesKHRs
#define VK_ENABLE_BETA_EXTENSIONS
#endif
#include "VulkanGraphicsDevice.h"
#include "VulkanRenderer.h"
#include <Internal/RendererSupport/VendorName.h>
#include <algorithm>
#include <iomanip>
#include <limits>
#include <sstream>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
VulkanGraphicsDevice::VulkanGraphicsDevice(cobalt::logging::ILogger* log, std::shared_ptr<VulkanInstanceData> instanceData, VkPhysicalDevice physicalDevice)
: _log(log), _physicalDevice(physicalDevice), _instanceData(std::move(instanceData))
{
	// Retrieve information on the target device
	_driverProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES;
	_deviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	_deviceProperties.pNext = &_driverProperties;
	vkGetPhysicalDeviceProperties2(_physicalDevice, &_deviceProperties);
	vkGetPhysicalDeviceFeatures(_physicalDevice, &_deviceFeatures);

	// Retrieve the supported set of device extensions
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(_physicalDevice, nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> deviceExtensionsVector(extensionCount);
	vkEnumerateDeviceExtensionProperties(_physicalDevice, nullptr, &extensionCount, deviceExtensionsVector.data());
	for (const auto& deviceExtension : deviceExtensionsVector)
	{
		_deviceExtensions[std::string(&deviceExtension.extensionName[0])] = deviceExtension;
		_deviceExtensionsSet.insert(&deviceExtension.extensionName[0]);
	}

	// Print available device extensions
#ifdef _DEBUG
	// std::string deviceExtensionsLogMessage;
	// for (const auto& deviceExtensionEntry : _deviceExtensions)
	// {
	// 	deviceExtensionsLogMessage += "  " + deviceExtensionEntry.first + ":" + std::to_string(deviceExtensionEntry.second.specVersion) + "\n";
	// }
	// _log->Debug("Available device extensions ({0}):\n{1}", _deviceProperties.deviceName, deviceExtensionsLogMessage);
#endif

	// Add our base required features to the supported feature set
	_supportedFeatureSet.insert(Feature::ComputeShaders);
	_supportedFeatureSet.insert(Feature::IndirectDraw);
	_supportedFeatureSet.insert(Feature::InstanceOffset);
	_supportedFeatureSet.insert(Feature::ResourceArrays);
	_supportedFeatureSet.insert(Feature::ShaderArraysOfArrays);
	_supportedFeatureSet.insert(Feature::SeparateTextureSamplers);
	_supportedFeatureSet.insert(Feature::TextureCubeArray);

	// Add our available optional features to the supported feature set
	if (_deviceFeatures.geometryShader == VK_TRUE)
	{
		// Not necessarily present - MoltenVK on macOS running under the portability subset doesn't support this.
		_supportedFeatureSet.insert(Feature::GeometryShaders);
	}
	if (_deviceFeatures.independentBlend == VK_TRUE)
	{
		_supportedFeatureSet.insert(Feature::SeparateBlendModePerTarget);
	}
	if (_deviceFeatures.samplerAnisotropy == VK_TRUE)
	{
		_supportedFeatureSet.insert(Feature::AnisotropicFiltering);
	}
	if (_deviceFeatures.fillModeNonSolid == VK_TRUE)
	{
		_supportedFeatureSet.insert(Feature::PolygonWireframeFillMode);
	}
	if (_deviceFeatures.depthBiasClamp == VK_TRUE)
	{
		_supportedFeatureSet.insert(Feature::DepthBiasClamp);
	}
	bool mipmapLevelBiasSupported = (_deviceProperties.properties.limits.maxSamplerLodBias > 0.0f);
#ifdef __APPLE__
	if (_deviceExtensions.find("VK_KHR_portability_subset") != _deviceExtensions.end())
	{
		VkPhysicalDevicePortabilitySubsetFeaturesKHR devicePortabilityFeatures{};
		devicePortabilityFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PORTABILITY_SUBSET_FEATURES_KHR;
		VkPhysicalDeviceFeatures2 deviceFeatures2{};
		deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		deviceFeatures2.pNext = &devicePortabilityFeatures;
		vkGetPhysicalDeviceFeatures2(_physicalDevice, &deviceFeatures2);

		// MoltenVK reports non-zero VkPhysicalDeviceLimits::maxSamplerLodBias even when mipmap LOD bias is unavailable.
		// Under VK_KHR_portability_subset, VkSamplerCreateInfo::mipLodBias can only be non-zero when this feature is true.
		mipmapLevelBiasSupported = mipmapLevelBiasSupported && (devicePortabilityFeatures.samplerMipLodBias == VK_TRUE);
	}
#endif
	if (mipmapLevelBiasSupported)
	{
		_supportedFeatureSet.insert(Feature::MipmapLevelBias);
	}
	if ((_deviceFeatures.multiDrawIndirect == VK_TRUE) && (_deviceExtensions.find("VK_KHR_draw_indirect_count") != _deviceExtensions.end()))
	{
		_supportedFeatureSet.insert(Feature::IndirectMultiDrawNative);
	}

	// On MoltenVK, we're forced to a 4-byte vertex stride, so for example a V1UInt8 vertex attribute requires a 4 byte
	// stride between each element. This is not the case for "normal" compliant Vulkan, but it comes from the underlying
	// Metal API, which has a 4-byte minimum stride, which MoltenVK must comply with. We extract the actual required
	// stride here programmatically on macOS, while leaving it set to the minimum of 1 on other platforms.
#ifdef __APPLE__
	VkPhysicalDevicePortabilitySubsetPropertiesKHR devicePortabilityProperties{};
	devicePortabilityProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PORTABILITY_SUBSET_PROPERTIES_KHR;
	devicePortabilityProperties.pNext = nullptr;
	VkPhysicalDeviceProperties2 deviceProperties2{};
	deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	deviceProperties2.pNext = &devicePortabilityProperties;
	vkGetPhysicalDeviceProperties2(_physicalDevice, &deviceProperties2);
	_minVertexInputBindingStrideAlignment = devicePortabilityProperties.minVertexInputBindingStrideAlignment;
#else
	_minVertexInputBindingStrideAlignment = 1;
#endif
}

//----------------------------------------------------------------------------------------
// Info methods
//----------------------------------------------------------------------------------------
VulkanGraphicsDevice::DeviceType VulkanGraphicsDevice::GetDeviceType() const
{
	// Note that we consider a "Virtual" GPU to be an alias for physical hardware, or a "slice" of physical hardware,
	// such as when a VM is using NVidia GRID technology. If there are specific exceptions, we'd need to detect and
	// handle them here. The Cobalt API doesn't intend to distinguish between "Virtual" adapaters running under VMs,
	// rather we only really care to separate devices into these type categories based on assumptions about their
	// relative performance characteristics, namely we expect the most capable devices are discrete, followed by
	// integrated, followed by software. Detecting software devices is also useful for testing purposes.
	switch (_deviceProperties.properties.deviceType)
	{
	case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
	case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
		return DeviceType::Discrete;
	case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
		return DeviceType::Integrated;
	case VK_PHYSICAL_DEVICE_TYPE_CPU:
		return DeviceType::Software;
	}
	return DeviceType::Unknown;
}

//----------------------------------------------------------------------------------------
IGraphicsDevice::Vendor VulkanGraphicsDevice::GetVendor() const
{
	return VendorCodeToVendor(_deviceProperties.properties.vendorID);
}

//----------------------------------------------------------------------------------------
Marshal::Ret<std::string> VulkanGraphicsDevice::GetVendorName() const
{
	return VendorToVendorName(GetVendor());
}

//----------------------------------------------------------------------------------------
Marshal::Ret<std::string> VulkanGraphicsDevice::GetDeviceName() const
{
	return std::string(&_deviceProperties.properties.deviceName[0]);
}

//----------------------------------------------------------------------------------------
Marshal::Ret<std::string> VulkanGraphicsDevice::GetDriverInfo() const
{
	std::stringstream stringBuilder;
	stringBuilder << &_driverProperties.driverName[0] << " - " << &_driverProperties.driverInfo[0] << "[" << std::hex << _deviceProperties.properties.driverVersion << "]";
	return stringBuilder.str();
}

//----------------------------------------------------------------------------------------
size_t VulkanGraphicsDevice::GetMemorySizeInBytes(MemoryType memoryType) const
{
	// Get memory properties and find size for memory type
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(_physicalDevice, &memProperties);

	// Check each memory heap and sum the total size by ownership class. Vulkan exposes unified memory on integrated
	// devices as DEVICE_LOCAL, so we only treat host-visible device-local heaps as dedicated memory on discrete-style
	// devices where the heap is still physically owned by the graphics device.
	size_t dedicatedMemSize = 0;
	size_t sharedMemSize = 0;
	bool deviceLocalMemoryIsShared = (_deviceProperties.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) || (_deviceProperties.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU);
	for (uint32_t i = 0; i < memProperties.memoryHeapCount; ++i)
	{
		// Check if the memory in this heap is local to the device or visible to the host
		const VkMemoryHeap& heapEntry = memProperties.memoryHeaps[i];
		bool heapIsDeviceLocal = ((heapEntry.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != 0);
		bool heapHasHostVisibleMemoryType = false;
		for (uint32_t memoryTypeIndex = 0; memoryTypeIndex < memProperties.memoryTypeCount; ++memoryTypeIndex)
		{
			const VkMemoryType& memoryTypeEntry = memProperties.memoryTypes[memoryTypeIndex];
			if ((memoryTypeEntry.heapIndex == i) && ((memoryTypeEntry.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0))
			{
				heapHasHostVisibleMemoryType = true;
				break;
			}
		}

		// Add the heap memory size to the dedicated pool if it's device local, and either not shared with or not
		// visible to the host.
		if (heapIsDeviceLocal && (!deviceLocalMemoryIsShared || !heapHasHostVisibleMemoryType))
		{
			dedicatedMemSize += (size_t)heapEntry.size;
		}
		else
		{
			sharedMemSize += (size_t)heapEntry.size;
		}
	}

	if (memoryType == MemoryType::Dedicated)
	{
		return dedicatedMemSize;
	}

	return sharedMemSize;
}

//----------------------------------------------------------------------------------------
bool VulkanGraphicsDevice::SupportsDeviceExtension(const std::string& extensionName) const
{
	return (_deviceExtensionsSet.find(extensionName) != _deviceExtensionsSet.end());
}

//----------------------------------------------------------------------------------------
// Limit methods
//----------------------------------------------------------------------------------------
VulkanGraphicsDevice::ImageLimits VulkanGraphicsDevice::GetImageLimits() const
{
	const auto& deviceLimits = _deviceProperties.properties.limits;
	ImageLimits imageLimits = {};
	imageLimits.maxImageDimensionTexture1D = deviceLimits.maxImageDimension1D;
	imageLimits.maxImageDimensionTexture2D = deviceLimits.maxImageDimension2D;
	imageLimits.maxImageDimensionTexture3D = deviceLimits.maxImageDimension3D;
	imageLimits.maxImageDimensionTextureCube = deviceLimits.maxImageDimension1D;
	imageLimits.maxImageArraySizeTexture1D = deviceLimits.maxImageArrayLayers;
	imageLimits.maxImageArraySizeTexture2D = deviceLimits.maxImageArrayLayers;
	imageLimits.maxSamplerAnisotropicFilteringLevel = (int)deviceLimits.maxSamplerAnisotropy;
	return imageLimits;
}

//----------------------------------------------------------------------------------------
VulkanGraphicsDevice::ShaderLimits VulkanGraphicsDevice::GetShaderLimits() const
{
	const auto& deviceLimits = _deviceProperties.properties.limits;
	ShaderLimits shaderLimits = {};
	shaderLimits.maxVertexShaderInputAttributes = deviceLimits.maxVertexInputAttributes;
	shaderLimits.maxVertexShaderOutputComponents = deviceLimits.maxVertexOutputComponents;
	shaderLimits.maxGeometryShaderInputComponents = deviceLimits.maxGeometryInputComponents;
	shaderLimits.maxGeometryShaderOutputComponents = deviceLimits.maxGeometryOutputComponents;
	shaderLimits.maxGeometryShaderOutputVertices = deviceLimits.maxGeometryOutputVertices;
	shaderLimits.maxGeometryShaderTotalOutputComponents = deviceLimits.maxGeometryTotalOutputComponents;
	shaderLimits.maxFragmentShaderInputComponents = deviceLimits.maxFragmentInputComponents;
	return shaderLimits;
}

//----------------------------------------------------------------------------------------
VulkanGraphicsDevice::DrawLimits VulkanGraphicsDevice::GetDrawLimits() const
{
	const auto& deviceLimits = _deviceProperties.properties.limits;
	DrawLimits drawLimits = {};
	drawLimits.maxVertexCountPerDraw = std::numeric_limits<uint32_t>::max(); // No exposed max value
	drawLimits.maxIndexValue = deviceLimits.maxDrawIndexedIndexValue;
	drawLimits.maxResourcesPerDraw = deviceLimits.maxPerStageResources;
	drawLimits.maxTextureResourcesPerDraw = deviceLimits.maxPerStageDescriptorSampledImages;
	return drawLimits;
}

//----------------------------------------------------------------------------------------
VulkanGraphicsDevice::FrameBufferLimits VulkanGraphicsDevice::GetFrameBufferLimits() const
{
	const auto& deviceLimits = _deviceProperties.properties.limits;
	FrameBufferLimits frameBufferLimits = {};
	frameBufferLimits.maxFrameBufferWidth = deviceLimits.maxFramebufferWidth;
	frameBufferLimits.maxFrameBufferHeight = deviceLimits.maxFramebufferHeight;
	frameBufferLimits.maxFrameBufferColorAttachments = deviceLimits.maxColorAttachments;
	frameBufferLimits.depthRange = IGraphicsDevice::DepthRange::ZeroToOne;
	return frameBufferLimits;
}

//----------------------------------------------------------------------------------------
VulkanGraphicsDevice::DataBufferLimits VulkanGraphicsDevice::GetDataBufferLimits() const
{
	DataBufferLimits limits = {};
	const auto& deviceLimits = _deviceProperties.properties.limits;

	limits.maxStateBufferPageSize = deviceLimits.maxUniformBufferRange;

	// See
	// https://www.khronos.org/registry/vulkan/specs/1.0/html/vkspec.html#interfaces-resources-layout

	// Note in SPIR-V you must specify offsets for every variable in your OpStruct declarations, and
	// your own array stride information, so these are less important. Vulkan has some special
	// vec2 and mat2 packing rules that are exceptions to these rules, but if you align to these
	// limits and specify the offsets explicitly, you'll be fine:
	limits.stateBufferAlignmentFloatOrInt = 4;
	limits.stateBufferAlignmentMatrix4f = 16;
	limits.stateBufferAlignmentVector4f = 16;

	// Note that state buffer means uniform buffers, and the Vulkan standard has a significant postscript
	// to 15.5.4 regarding the structure of uniform buffers - they're
	// std140 (SSBOs and other allocations are std430). So these are all 16.
	limits.stateBufferAlignmentStruct = 16;
	limits.stateBufferAlignmentArrayStride = 16;
	limits.stateBufferAlignmentArrayWhole = 16;

	// SSBO alignment (which is equal to its base alignment - std430) isn't exposed by the api
	// yet.

	return limits;
}

//----------------------------------------------------------------------------------------
// Feature methods
//----------------------------------------------------------------------------------------
bool VulkanGraphicsDevice::IsFeatureSupported(Feature feature) const
{
	return (_supportedFeatureSet.find(feature) != _supportedFeatureSet.end());
}

//----------------------------------------------------------------------------------------
bool VulkanGraphicsDevice::AreAllFeaturesSupported(const Marshal::In<std::set<Feature>>& featureSet) const
{
	auto featureSetResolved = featureSet.Get();
	return std::includes(_supportedFeatureSet.begin(), _supportedFeatureSet.end(), featureSetResolved.begin(), featureSetResolved.end());
}

//----------------------------------------------------------------------------------------
Marshal::Ret<std::set<VulkanGraphicsDevice::Feature>> VulkanGraphicsDevice::GetAllSupportedFeatures() const
{
	return _supportedFeatureSet;
}

//----------------------------------------------------------------------------------------
bool VulkanGraphicsDevice::IsTextureFormatSupported(ITextureBuffer::ImageFormat imageFormat, ITextureBuffer::DataFormat dataFormat) const
{
	// All non-compressed formats are supported either natively or via conversion, so return true for all of them.
	if (!ITextureBuffer::IsCompressedTextureFormat(dataFormat))
	{
		return true;
	}

	// If this is a compressed texture format, retrieve its native format if it might be supported.
	VkFormat nativeFormat = VK_FORMAT_UNDEFINED;
	switch (dataFormat)
	{
	case ITextureBuffer::DataFormat::DXT1:
		if (_deviceFeatures.textureCompressionBC != VK_TRUE)
		{
			return false;
		}
		if (imageFormat == ITextureBuffer::ImageFormat::RGB)
		{
			nativeFormat = VK_FORMAT_BC1_RGB_UNORM_BLOCK;
		}
		else if (imageFormat == ITextureBuffer::ImageFormat::RGBA)
		{
			nativeFormat = VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
		}
		else
		{
			return false;
		}
		break;
	case ITextureBuffer::DataFormat::DXT3:
		if ((_deviceFeatures.textureCompressionBC != VK_TRUE) || (imageFormat != ITextureBuffer::ImageFormat::RGBA))
		{
			return false;
		}
		nativeFormat = VK_FORMAT_BC2_UNORM_BLOCK;
		break;
	case ITextureBuffer::DataFormat::DXT5:
		if ((_deviceFeatures.textureCompressionBC != VK_TRUE) || (imageFormat != ITextureBuffer::ImageFormat::RGBA))
		{
			return false;
		}
		nativeFormat = VK_FORMAT_BC3_UNORM_BLOCK;
		break;
	case ITextureBuffer::DataFormat::BPTC:
		if ((_deviceFeatures.textureCompressionBC != VK_TRUE) || (imageFormat != ITextureBuffer::ImageFormat::RGBA))
		{
			return false;
		}
		nativeFormat = VK_FORMAT_BC7_UNORM_BLOCK;
		break;
	case ITextureBuffer::DataFormat::ASTC4x4:
	case ITextureBuffer::DataFormat::ASTC5x5:
	case ITextureBuffer::DataFormat::ASTC6x6:
	case ITextureBuffer::DataFormat::ASTC8x8:
		if ((_deviceFeatures.textureCompressionASTC_LDR != VK_TRUE) || (imageFormat != ITextureBuffer::ImageFormat::RGBA))
		{
			return false;
		}
		switch (dataFormat)
		{
		case ITextureBuffer::DataFormat::ASTC4x4:
			nativeFormat = VK_FORMAT_ASTC_4x4_UNORM_BLOCK;
			break;
		case ITextureBuffer::DataFormat::ASTC5x5:
			nativeFormat = VK_FORMAT_ASTC_5x5_UNORM_BLOCK;
			break;
		case ITextureBuffer::DataFormat::ASTC6x6:
			nativeFormat = VK_FORMAT_ASTC_6x6_UNORM_BLOCK;
			break;
		case ITextureBuffer::DataFormat::ASTC8x8:
			nativeFormat = VK_FORMAT_ASTC_8x8_UNORM_BLOCK;
			break;
		default:
			return false;
		}
		break;
	default:
		return false;
	}
	if (nativeFormat == VK_FORMAT_UNDEFINED)
	{
		return false;
	}

	// Confirm the compressed texture format is supported for images and sampling
	VkImageFormatProperties imageFormatProperties = {};
	VkResult result = vkGetPhysicalDeviceImageFormatProperties(_physicalDevice, nativeFormat, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT, 0, &imageFormatProperties);
	return (result == VK_SUCCESS);
}

//----------------------------------------------------------------------------------------
// Render methods
//----------------------------------------------------------------------------------------
IRenderer::unique_ptr VulkanGraphicsDevice::CreateRenderer(const Marshal::In<std::set<Feature>>& enabledFeatures, const Marshal::In<std::set<IRenderer::Options>>& enabledOptions)
{
	// Our renderer API supports shaders with unbound resources, such as when certain vertex attributes are optional, or
	// texture resources that may or may not be present. This is easy to support on most graphics APIs, however this
	// functionality on Vulkan requires the "nullDescriptor" provided by the "VK_EXT_robustness2" extension. This
	// functionality is required by Direct3D 12, so it should be universally available, however drivers prior to January
	// 2021 may not have support for this extension implemented. We also have issues with older NVidia driver versions
	// on Windows having broken support, and MoltenVK not providing support for this feature. In order to give universal
	// support, we detect when support is missing or broken, and fallback to binding valid "dummy" buffers in place of
	// actual resource buffers, to make this feature work universally.
	bool nullDescriptorFeatureMissingOrBroken = false;
#ifdef _WIN32
	// Note that NVidia Vulkan drivers on Windows currently (as of 2021-04-17) have a bad implementation of the null
	// descriptor feature. If VK_NULL_HANDLE is used for an input slot, that input slot ends up permanently disabled
	// for the lifetime of the VkDevice object. Subsequent attempts to rebind to a buffer are ignored, even under
	// different pipeline states with different shader programs bound. As of 2025-12-14, this NVidia driver bug appears
	// fixed in newer versions, however it has still been observed on older devices/driver versions, in particular
	// driver version 30.0.14.7514 for a GeForce GTX 660 Ti. It is proven fixed in driver version 32.0.15.8092 on an
	// NVidia RTX 5000 Ada. We therefore still trigger this fallback for older NVidia driver versions. The marketing
	// version numbers appear to be taken from the build number combined with the lower digit of the patch version, so
	// 32.0.15.8092 for example is 580.92.
	//##TODO## Confirm which driver version this was fixed in
	uint32_t majorVersion = (_deviceProperties.properties.driverVersion >> 22);
	//uint32_t minorVersion = ((_deviceProperties.properties.driverVersion >> 14) & 0xFF);
	//uint32_t patchVersion = ((_deviceProperties.properties.driverVersion >> 6) & 0xFF);
	//uint32_t buildVersion = (_deviceProperties.properties.driverVersion & 0x3F);
	nullDescriptorFeatureMissingOrBroken = (GetVendor() == Vendor::Nvidia) && (majorVersion < 500);
#elif defined(__APPLE__)
	// MoltekVK (as of 2026-06-16) still doesn't have support for the null descriptor feature yet:
	// https://github.com/KhronosGroup/MoltenVK/issues/2650
	nullDescriptorFeatureMissingOrBroken = true;
#endif
	IRenderer* renderer = new VulkanRenderer(_log->CloneLogger(), _instanceData, _physicalDevice, _deviceFeatures, _deviceExtensionsSet, enabledFeatures.Get(), enabledOptions.Get(), nullDescriptorFeatureMissingOrBroken, _minVertexInputBindingStrideAlignment);
	return IRenderer::unique_ptr(renderer);
}

} // namespace cobalt::graphics
