// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#ifdef __APPLE__
#ifndef VK_ENABLE_BETA_EXTENSIONS
#define VK_ENABLE_BETA_EXTENSIONS
#endif
#endif
#include "VulkanRenderer.h"
#include "VulkanDataArray.h"
#include "VulkanDataArrayOutput.h"
#include "VulkanDefaultState.h"
#include "VulkanFrameBuffer.h"
#include "VulkanFrameBufferOutput.h"
#include "VulkanHeaders.h"
#include "VulkanIndexBuffer.h"
#include "VulkanProgramNode.h"
#include "VulkanRenderPassNode.h"
#include "VulkanRenderableNode.h"
#include "VulkanShaderProgram.h"
#include "VulkanStateBuffer.h"
#include "VulkanStateBufferLayout.h"
#include "VulkanStateGroupNode.h"
#include "VulkanTexelArray.h"
#include "VulkanTexelArrayOutput.h"
#include "VulkanTextureBuffer1D.h"
#include "VulkanTextureBuffer1DArray.h"
#include "VulkanTextureBuffer2D.h"
#include "VulkanTextureBuffer2DArray.h"
#include "VulkanTextureBuffer3D.h"
#include "VulkanTextureBufferCube.h"
#include "VulkanTextureBufferCubeArray.h"
#include "VulkanTextureSampler1D.h"
#include "VulkanTextureSampler1DArray.h"
#include "VulkanTextureSampler2D.h"
#include "VulkanTextureSampler2DArray.h"
#include "VulkanTextureSampler3D.h"
#include "VulkanTextureSamplerCube.h"
#include "VulkanTextureSamplerCubeArray.h"
#include "VulkanTransferBatch.h"
#include "VulkanVertexBuffer.h"
#include <Internal/RendererSupport/KnownDynamicCast.h>
#include <Internal/RendererSupport/RenderMarkers.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <type_traits>
#include <utility>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
//##FIX## Defined this here rather than updated the entire Vulkan SDK at this time. When we do update, these names will
//clash and should be removed from here, with the definitions from the SDK being used instead.
struct VkPhysicalDeviceRobustness2FeaturesEXT
{
	VkStructureType sType;
	void* pNext;
	VkBool32 robustBufferAccess2;
	VkBool32 robustImageAccess2;
	VkBool32 nullDescriptor;
};
#define VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT 1000286000

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
VulkanRenderer::VulkanRenderer(cobalt::logging::ILogger::unique_ptr log, std::shared_ptr<VulkanInstanceData> instanceData, VkPhysicalDevice physicalDevice, const VkPhysicalDeviceFeatures& deviceFeatures, const std::set<std::string>& deviceExtensions, const std::set<IGraphicsDevice::Feature>& enabledFeatures, const std::set<Options>& enabledOptions, bool nullDescriptorFeatureMissingOrBroken, uint32_t minVertexElementStride)
{
	_log = std::move(log);
	_instanceData = std::move(instanceData);
	_physicalDevice = physicalDevice;
	_deviceFeatures = deviceFeatures;
	_deviceExtensions = deviceExtensions;

	_enabledFeatures = enabledFeatures;
	_enabledOptions = enabledOptions;

	_buildIndex = 0;
	_drawIndex = 1;

	_nullDescriptorFeatureMissingOrBroken = nullDescriptorFeatureMissingOrBroken;
	_minVertexElementStride = minVertexElementStride;
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
SuccessToken VulkanRenderer::Initialize(const WindowSystemInfoBase& windowSystemInfo, InitializationFlags flags)
{
	// Retrieve the physical device properties
	vkGetPhysicalDeviceProperties(_physicalDevice, &_physicalDeviceProperties);

	// Retrieve information on the queues provided by the target graphics device
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(_physicalDevice, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(_physicalDevice, &queueFamilyCount, queueFamilies.data());

	// Select the queues to use for graphics and transfer commands
	bool graphicsQueueFound = false;
	bool transferQueueFound = false;
	bool batchTransferQueueFound = false;
	bool computeQueueFound = false;
	uint32_t graphicsQueueFamilyIndex = 0;
	uint32_t graphicsQueueIndex = 0;
	uint32_t transferQueueFamilyIndex = 0;
	uint32_t transferQueueIndex = 0;
	uint32_t batchTransferQueueFamilyIndex = 0;
	uint32_t batchTransferQueueIndex = 0;
	uint32_t computeQueueFamilyIndex = 0;
	uint32_t computeQueueIndex = 0;
	for (uint32_t i = 0; i < queueFamilyCount; ++i)
	{
		if (!graphicsQueueFound && ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0))
		{
			graphicsQueueFamilyIndex = i;
			graphicsQueueIndex = 0;
			graphicsQueueFound = true;
			if (!computeQueueFound && ((queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0))
			{
				computeQueueFamilyIndex = i;
				computeQueueIndex = 0;
				computeQueueFound = true;
			}
		}
		else if (!transferQueueFound && ((queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT) != 0) && ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) && ((queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
		{
			transferQueueFamilyIndex = i;
			transferQueueIndex = 0;
			transferQueueFound = true;
			if (queueFamilies[i].queueCount > 1)
			{
				batchTransferQueueFamilyIndex = i;
				batchTransferQueueIndex = 1;
				batchTransferQueueFound = true;
			}
		}
		else if (!batchTransferQueueFound && ((queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT) != 0) && ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) && ((queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
		{
			batchTransferQueueFamilyIndex = i;
			batchTransferQueueIndex = 0;
			batchTransferQueueFound = true;
		}
		else if (!computeQueueFound && ((queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0))
		{
			computeQueueFamilyIndex = i;
			computeQueueIndex = 0;
			computeQueueFound = true;
		}
	}
	if (!graphicsQueueFound)
	{
		_log->Error("Failed to locate a graphics queue from {0} queue families", queueFamilyCount);
		return false;
	}
	if (!computeQueueFound)
	{
		_log->Error("Failed to locate a compute queue from {0} queue families", queueFamilyCount);
		return false;
	}
	if (!transferQueueFound)
	{
		transferQueueFamilyIndex = graphicsQueueFamilyIndex;
		transferQueueIndex = (queueFamilies[transferQueueFamilyIndex].queueCount > graphicsQueueIndex + 1) ? graphicsQueueIndex + 1 : graphicsQueueIndex;
	}
	if (!batchTransferQueueFound)
	{
		batchTransferQueueFamilyIndex = transferQueueFamilyIndex;
		batchTransferQueueIndex = (queueFamilies[batchTransferQueueFamilyIndex].queueCount > transferQueueIndex + 1) ? transferQueueIndex + 1 : transferQueueIndex;
	}
	bool separateComputeQueue = ((computeQueueFamilyIndex != graphicsQueueFamilyIndex) || (computeQueueIndex != graphicsQueueIndex));
	bool headlessRendering = ((windowSystemInfo.windowSystemType == WindowSystemInfoBase::WindowSystemType::Headless) && (windowSystemInfo.structureSizeInBytes == sizeof(WindowSystemInfoHeadless)));

	// Select the presentation queue for the configured window system type. We prefer the graphics queue family when
	// possible, then an otherwise-unused queue family, then compute, transfer, and finally the batch transfer queue.
	bool presentQueueFound = false;
	uint32_t presentQueueFamilyIndex = 0;
	uint32_t presentQueueIndex = 0;
	auto selectPresentQueue = [&](const auto& queueFamilySupportsPresent) -> bool {
		auto tryQueueFamily = [&](uint32_t queueFamilyIndex) -> bool {
			if (!queueFamilySupportsPresent(queueFamilyIndex))
			{
				return false;
			}
			presentQueueFound = true;
			presentQueueFamilyIndex = queueFamilyIndex;
			presentQueueIndex = 0;
			return true;
		};

		if (!tryQueueFamily(graphicsQueueFamilyIndex))
		{
			for (uint32_t i = 0; i < queueFamilyCount; ++i)
			{
				bool queueFamilyUnused = (i != graphicsQueueFamilyIndex) && (i != computeQueueFamilyIndex) && (i != transferQueueFamilyIndex) && (i != batchTransferQueueFamilyIndex);
				if (queueFamilyUnused && tryQueueFamily(i))
				{
					break;
				}
			}
		}
		if (!presentQueueFound)
		{
			tryQueueFamily(computeQueueFamilyIndex);
		}
		if (!presentQueueFound)
		{
			tryQueueFamily(transferQueueFamilyIndex);
		}
		if (!presentQueueFound)
		{
			tryQueueFamily(batchTransferQueueFamilyIndex);
		}
		if (!presentQueueFound)
		{
			_log->Error("Failed to locate a presentation queue for window system type {0}", windowSystemInfo.windowSystemType);
			return false;
		}
		return true;
	};
#ifdef COBALT_RENDERER_WIN32_SUPPORT
	if ((windowSystemInfo.windowSystemType == WindowSystemInfoBase::WindowSystemType::Win32) && (windowSystemInfo.structureSizeInBytes == sizeof(WindowSystemInfoWin32)))
	{
		if (!selectPresentQueue([&](uint32_t queueFamilyIndex) {
			    return (vkGetPhysicalDeviceWin32PresentationSupportKHR(_physicalDevice, queueFamilyIndex) == VK_TRUE);
		    }))
		{
			return false;
		}
	}
#endif
#ifdef COBALT_RENDERER_XLIB_SUPPORT
	if ((windowSystemInfo.windowSystemType == WindowSystemInfoBase::WindowSystemType::Xlib) && (windowSystemInfo.structureSizeInBytes == sizeof(WindowSystemInfoXlib)))
	{
		const auto* windowSystemInfoResolved = reinterpret_cast<const WindowSystemInfoXlib*>(&windowSystemInfo);
		auto* defaultVisual = DefaultVisual(windowSystemInfoResolved->display, DefaultScreen(windowSystemInfoResolved->display));
		VisualID defaultVisualID = XVisualIDFromVisual(defaultVisual);
		if (!selectPresentQueue([&](uint32_t queueFamilyIndex) {
			    return (vkGetPhysicalDeviceXlibPresentationSupportKHR(_physicalDevice, queueFamilyIndex, windowSystemInfoResolved->display, defaultVisualID) == VK_TRUE);
		    }))
		{
			return false;
		}
	}
#endif
#ifdef COBALT_RENDERER_XCB_SUPPORT
	if ((windowSystemInfo.windowSystemType == WindowSystemInfoBase::WindowSystemType::XCB) && (windowSystemInfo.structureSizeInBytes == sizeof(WindowSystemInfoXCB)))
	{
		const auto* windowSystemInfoResolved = reinterpret_cast<const WindowSystemInfoXCB*>(&windowSystemInfo);
		xcb_visualid_t defaultVisualID = {};
		const xcb_setup_t* setup = xcb_get_setup(windowSystemInfoResolved->connection);
		auto screenIterator = xcb_setup_roots_iterator(setup);
		if (screenIterator.rem > 0)
		{
			defaultVisualID = screenIterator.data->root_visual;
		}
		if (!selectPresentQueue([&](uint32_t queueFamilyIndex) {
			    return (vkGetPhysicalDeviceXcbPresentationSupportKHR(_physicalDevice, queueFamilyIndex, windowSystemInfoResolved->connection, defaultVisualID) == VK_TRUE);
		    }))
		{
			return false;
		}
	}
#endif
#ifdef COBALT_RENDERER_WAYLAND_SUPPORT
	if ((windowSystemInfo.windowSystemType == WindowSystemInfoBase::WindowSystemType::Wayland) && (windowSystemInfo.structureSizeInBytes == sizeof(WindowSystemInfoWayland)))
	{
		const auto* windowSystemInfoResolved = reinterpret_cast<const WindowSystemInfoWayland*>(&windowSystemInfo);
		if (!selectPresentQueue([&](uint32_t queueFamilyIndex) {
			    return (vkGetPhysicalDeviceWaylandPresentationSupportKHR(_physicalDevice, queueFamilyIndex, windowSystemInfoResolved->display) == VK_TRUE);
		    }))
		{
			return false;
		}
	}
#endif
#ifdef COBALT_RENDERER_APPKIT_SUPPORT
	if ((windowSystemInfo.windowSystemType == WindowSystemInfoBase::WindowSystemType::AppKit) && (windowSystemInfo.structureSizeInBytes == sizeof(WindowSystemInfoAppKit)))
	{
		// There is no platform-specific pre-surface presentation query for Metal/AppKit, so prefer the graphics queue
		// and verify compatibility against the created surface later.
		presentQueueFound = true;
		presentQueueFamilyIndex = graphicsQueueFamilyIndex;
		presentQueueIndex = 0;
	}
#endif
	if (headlessRendering)
	{
		// Headless rendering never presents to a native surface, but the renderer still expects a present queue handle
		// for shared code paths. Reuse the graphics queue rather than requiring swapchain presentation support.
		presentQueueFound = true;
		presentQueueFamilyIndex = graphicsQueueFamilyIndex;
		presentQueueIndex = graphicsQueueIndex;
	}

	// Report the selected queues
	if (presentQueueFound)
	{
		_log->Info("Selected graphics queue {0}:{1}, compute queue {2}:{3}, transfer queue {4}:{5}, batch transfer queue {6}:{7}, and present queue {8}:{9} from {10} queue families.", graphicsQueueFamilyIndex, graphicsQueueIndex, computeQueueFamilyIndex, computeQueueIndex, transferQueueFamilyIndex, transferQueueIndex, batchTransferQueueFamilyIndex, batchTransferQueueIndex, presentQueueFamilyIndex, presentQueueIndex, queueFamilyCount);
	}
	else
	{
		_log->Info("Selected graphics queue {0}:{1}, compute queue {2}:{3}, transfer queue {4}:{5}, and batch transfer queue {6}:{7} from {8} queue families.", graphicsQueueFamilyIndex, graphicsQueueIndex, computeQueueFamilyIndex, computeQueueIndex, transferQueueFamilyIndex, transferQueueIndex, batchTransferQueueFamilyIndex, batchTransferQueueIndex, queueFamilyCount);
	}

	// Build the set of queues to create
	float queuePriorities[] = {1.0f, 0.8f, 0.5f};
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfoSet;
	VkDeviceQueueCreateInfo graphicsQueueCreateInfo = {};
	graphicsQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	graphicsQueueCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
	graphicsQueueCreateInfo.queueCount = ((transferQueueFamilyIndex == graphicsQueueFamilyIndex) && (transferQueueIndex > graphicsQueueIndex) ? ((batchTransferQueueFamilyIndex == transferQueueFamilyIndex) && (batchTransferQueueFamilyIndex > transferQueueIndex) ? 3 : 2) : 1);
	graphicsQueueCreateInfo.pQueuePriorities = &queuePriorities[0];
	queueCreateInfoSet.push_back(graphicsQueueCreateInfo);
	if (transferQueueFamilyIndex != graphicsQueueFamilyIndex)
	{
		VkDeviceQueueCreateInfo transferQueueCreateInfo = {};
		transferQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		transferQueueCreateInfo.queueFamilyIndex = transferQueueFamilyIndex;
		transferQueueCreateInfo.queueCount = ((batchTransferQueueFamilyIndex == transferQueueFamilyIndex) && (batchTransferQueueIndex > transferQueueIndex) ? 2 : 1);
		transferQueueCreateInfo.pQueuePriorities = &queuePriorities[1];
		queueCreateInfoSet.push_back(transferQueueCreateInfo);
	}
	if (batchTransferQueueFamilyIndex != transferQueueFamilyIndex)
	{
		VkDeviceQueueCreateInfo batchTransferQueueCreateInfo = {};
		batchTransferQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		batchTransferQueueCreateInfo.queueFamilyIndex = batchTransferQueueFamilyIndex;
		batchTransferQueueCreateInfo.queueCount = 1;
		batchTransferQueueCreateInfo.pQueuePriorities = &queuePriorities[2];
		queueCreateInfoSet.push_back(batchTransferQueueCreateInfo);
	}
	if (computeQueueFamilyIndex != graphicsQueueFamilyIndex)
	{
		VkDeviceQueueCreateInfo computeQueueCreateInfo = {};
		computeQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		computeQueueCreateInfo.queueFamilyIndex = computeQueueFamilyIndex;
		computeQueueCreateInfo.queueCount = 1;
		computeQueueCreateInfo.pQueuePriorities = &queuePriorities[1];
		queueCreateInfoSet.push_back(computeQueueCreateInfo);
	}
	if (presentQueueFound)
	{
		bool presentQueueFamilyAlreadyCreated = false;
		for (const auto& queueCreateInfo : queueCreateInfoSet)
		{
			if (queueCreateInfo.queueFamilyIndex == presentQueueFamilyIndex)
			{
				presentQueueFamilyAlreadyCreated = true;
				break;
			}
		}
		if (!presentQueueFamilyAlreadyCreated)
		{
			VkDeviceQueueCreateInfo presentQueueCreateInfo = {};
			presentQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			presentQueueCreateInfo.queueFamilyIndex = presentQueueFamilyIndex;
			presentQueueCreateInfo.queueCount = 1;
			presentQueueCreateInfo.pQueuePriorities = &queuePriorities[0];
			queueCreateInfoSet.push_back(presentQueueCreateInfo);
		}
	}

	// Setup logical device create info
	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = (uint32_t)queueCreateInfoSet.size();
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfoSet.data();

	// Enable mandatory, optional, and requested device features.
	void* enabledDeviceFeatureChain = nullptr;
	auto appendEnabledDeviceFeature = [&](auto& featureStruct) {
		featureStruct.pNext = enabledDeviceFeatureChain;
		enabledDeviceFeatureChain = &featureStruct;
	};
	VkPhysicalDeviceFeatures2 enabledDeviceFeatures = {};
	enabledDeviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
#ifdef __APPLE__
	// Still no geometry shader support on MoltenVK as of 2026:
	// https://github.com/KhronosGroup/MoltenVK/issues/1524
	enabledDeviceFeatures.features.geometryShader = VK_FALSE;
#else
	enabledDeviceFeatures.features.geometryShader = (_enabledFeatures.find(IGraphicsDevice::Feature::GeometryShaders) != _enabledFeatures.end() ? VK_TRUE : VK_FALSE);
#endif
	enabledDeviceFeatures.features.textureCompressionBC = _deviceFeatures.textureCompressionBC;
	enabledDeviceFeatures.features.textureCompressionETC2 = _deviceFeatures.textureCompressionETC2;
	enabledDeviceFeatures.features.textureCompressionASTC_LDR = _deviceFeatures.textureCompressionASTC_LDR;
	enabledDeviceFeatures.features.imageCubeArray = _deviceFeatures.imageCubeArray;
	enabledDeviceFeatures.features.fillModeNonSolid = _deviceFeatures.fillModeNonSolid;
	enabledDeviceFeatures.features.shaderStorageImageArrayDynamicIndexing = _deviceFeatures.shaderStorageImageArrayDynamicIndexing;
	enabledDeviceFeatures.features.shaderStorageBufferArrayDynamicIndexing = _deviceFeatures.shaderStorageBufferArrayDynamicIndexing;
	enabledDeviceFeatures.features.shaderSampledImageArrayDynamicIndexing = _deviceFeatures.shaderSampledImageArrayDynamicIndexing;
	enabledDeviceFeatures.features.shaderUniformBufferArrayDynamicIndexing = _deviceFeatures.shaderUniformBufferArrayDynamicIndexing;
	enabledDeviceFeatures.features.samplerAnisotropy = (_enabledFeatures.find(IGraphicsDevice::Feature::AnisotropicFiltering) != _enabledFeatures.end() ? VK_TRUE : VK_FALSE);
	enabledDeviceFeatures.features.depthBiasClamp = (_enabledFeatures.find(IGraphicsDevice::Feature::DepthBiasClamp) != _enabledFeatures.end() ? VK_TRUE : VK_FALSE);
	enabledDeviceFeatures.features.fillModeNonSolid = (_enabledFeatures.find(IGraphicsDevice::Feature::PolygonWireframeFillMode) != _enabledFeatures.end() ? VK_TRUE : VK_FALSE);
	enabledDeviceFeatures.features.independentBlend = (_enabledFeatures.find(IGraphicsDevice::Feature::SeparateBlendModePerTarget) != _enabledFeatures.end() ? VK_TRUE : VK_FALSE);
	enabledDeviceFeatures.features.fragmentStoresAndAtomics = ((_deviceFeatures.fragmentStoresAndAtomics == VK_TRUE) && (_enabledFeatures.find(IGraphicsDevice::Feature::ResourceArrays) != _enabledFeatures.end()) ? VK_TRUE : VK_FALSE);
	enabledDeviceFeatures.features.vertexPipelineStoresAndAtomics = ((_deviceFeatures.vertexPipelineStoresAndAtomics == VK_TRUE) && (_enabledFeatures.find(IGraphicsDevice::Feature::ResourceArrays) != _enabledFeatures.end()) ? VK_TRUE : VK_FALSE);
	enabledDeviceFeatures.features.multiDrawIndirect = ((_deviceFeatures.multiDrawIndirect == VK_TRUE) && (_enabledFeatures.find(IGraphicsDevice::Feature::IndirectDraw) != _enabledFeatures.end()) ? VK_TRUE : VK_FALSE);
	enabledDeviceFeatures.features.drawIndirectFirstInstance = ((enabledDeviceFeatures.features.multiDrawIndirect == VK_TRUE) && (_deviceFeatures.drawIndirectFirstInstance == VK_TRUE) ? VK_TRUE : VK_FALSE);
	enabledDeviceFeatures.features.robustBufferAccess = ((_nullDescriptorFeatureMissingOrBroken && (_deviceFeatures.robustBufferAccess == VK_TRUE)) ? VK_TRUE : VK_FALSE);
	deviceCreateInfo.pEnabledFeatures = nullptr;
	deviceCreateInfo.pNext = &enabledDeviceFeatures;

	// Enable the primitive restart feature. In reality, this feature is supported on everything, but on macOS under
	// MoltenVK for Apple silicon, the feature is always enabled and cannot be turned off. This extention is therefore
	// not supported, not because the feature is not supported, but because it can't be disabled. We detect if the
	// extension is supported here and skip asking for it if it isn't, as this will cause device creation to fail.
	_primitiveRestartSupported = (_deviceExtensions.find(VK_EXT_PRIMITIVE_TOPOLOGY_LIST_RESTART_EXTENSION_NAME) != _deviceExtensions.end());
	VkPhysicalDevicePrimitiveTopologyListRestartFeaturesEXT primitiveTopologyRestartFeatures = {};
	if (_primitiveRestartSupported)
	{
		primitiveTopologyRestartFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRIMITIVE_TOPOLOGY_LIST_RESTART_FEATURES_EXT;
		primitiveTopologyRestartFeatures.primitiveTopologyListRestart = VK_TRUE;
		primitiveTopologyRestartFeatures.primitiveTopologyPatchListRestart = VK_FALSE;
		appendEnabledDeviceFeature(primitiveTopologyRestartFeatures);
	}

#ifdef __APPLE__
	// MoltenVK exposes sampler LOD bias through the portability subset feature, not through maxSamplerLodBias alone.
	// If this feature is false, VkSamplerCreateInfo::mipLodBias must remain zero.
	VkPhysicalDevicePortabilitySubsetFeaturesKHR enabledPortabilityFeatures{};
	if (_deviceExtensions.find("VK_KHR_portability_subset") != _deviceExtensions.end())
	{
		VkPhysicalDevicePortabilitySubsetFeaturesKHR supportedPortabilityFeatures{};
		supportedPortabilityFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PORTABILITY_SUBSET_FEATURES_KHR;
		VkPhysicalDeviceFeatures2 supportedDeviceFeatures{};
		supportedDeviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		supportedDeviceFeatures.pNext = &supportedPortabilityFeatures;
		vkGetPhysicalDeviceFeatures2(_physicalDevice, &supportedDeviceFeatures);

		enabledPortabilityFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PORTABILITY_SUBSET_FEATURES_KHR;
		enabledPortabilityFeatures.samplerMipLodBias = ((_enabledFeatures.find(IGraphicsDevice::Feature::MipmapLevelBias) != _enabledFeatures.end()) && (supportedPortabilityFeatures.samplerMipLodBias == VK_TRUE) ? VK_TRUE : VK_FALSE);
		appendEnabledDeviceFeature(enabledPortabilityFeatures);
	}
#endif

#ifndef __APPLE__
	// Enable the null descriptor feature where it exists. Some devices still need the fallback path, so we also keep
	// track of whether the feature is missing or known-broken and patch in dummy bindings later.
	//##FIX## This is still not supported in MoltenVK as of 2026:
	//https://github.com/KhronosGroup/MoltenVK/issues/2650
	VkPhysicalDeviceRobustness2FeaturesEXT deviceFeaturesExtensionPhysicalDeviceRebustness2 = {};
	deviceFeaturesExtensionPhysicalDeviceRebustness2.sType = VkStructureType(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT);
	deviceFeaturesExtensionPhysicalDeviceRebustness2.nullDescriptor = VK_TRUE;
	deviceFeaturesExtensionPhysicalDeviceRebustness2.robustBufferAccess2 = VK_FALSE;
	deviceFeaturesExtensionPhysicalDeviceRebustness2.robustImageAccess2 = VK_FALSE;
	appendEnabledDeviceFeature(deviceFeaturesExtensionPhysicalDeviceRebustness2);
#endif
	enabledDeviceFeatures.pNext = enabledDeviceFeatureChain;

	// Determine which optional device extensions will be loaded
	bool indirectDrawRequested = _enabledFeatures.find(IGraphicsDevice::Feature::IndirectDraw) != _enabledFeatures.end();
	_extensionInfo.extensionLoaded_VK_KHR_draw_indirect_count = indirectDrawRequested && (_deviceExtensions.find("VK_KHR_draw_indirect_count") != _deviceExtensions.end());
	_indirectDrawCountFromBufferFallbackActive = indirectDrawRequested && !_extensionInfo.extensionLoaded_VK_KHR_draw_indirect_count;

	// Requested device extensions
	std::vector<const char*> deviceExtensions = {VK_KHR_MAINTENANCE1_EXTENSION_NAME, "VK_EXT_robustness2"};
	if (!headlessRendering)
	{
		deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	}
	if (_extensionInfo.extensionLoaded_VK_KHR_draw_indirect_count)
	{
		deviceExtensions.push_back("VK_KHR_draw_indirect_count");
	}
#ifdef __APPLE__
	// Required for MoltenVK on macOS
	deviceExtensions.push_back("VK_KHR_portability_subset");
#endif
	deviceCreateInfo.enabledExtensionCount = (uint32_t)deviceExtensions.size();
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

	// Include validation layers if requested
	std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};
	if (_instanceData->enableValidationLayers)
	{
		deviceCreateInfo.enabledLayerCount = (uint32_t)validationLayers.size();
		deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
	{
		deviceCreateInfo.enabledLayerCount = 0;
	}

	// Check if debug utils is available for render markers and find related functions for them
	if (_instanceData->debugUtilsAvailable && _enabledOptions.find(IRenderer::Options::EnableRenderMarkers) != _enabledOptions.end())
	{
		_pfnCmdBeginDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(vkGetInstanceProcAddr(_instanceData->instance, "vkCmdBeginDebugUtilsLabelEXT"));
		_pfnCmdEndDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(vkGetInstanceProcAddr(_instanceData->instance, "vkCmdEndDebugUtilsLabelEXT"));
		_pfnCmdInsertDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdInsertDebugUtilsLabelEXT>(vkGetInstanceProcAddr(_instanceData->instance, "vkCmdInsertDebugUtilsLabelEXT"));
		_useRenderMarkers = (_pfnCmdBeginDebugUtilsLabelEXT != nullptr) && (_pfnCmdEndDebugUtilsLabelEXT != nullptr) && (_pfnCmdInsertDebugUtilsLabelEXT != nullptr);
	}

	// Create the device
	VkResult createDeviceReturn = vkCreateDevice(_physicalDevice, &deviceCreateInfo, nullptr, &_device);
	if (createDeviceReturn != VK_SUCCESS)
	{
		_log.get()->Critical("vkCreateDevice failed with error code {0}", createDeviceReturn);
		return false;
	}

	// Retrieve any required endpoints for the set of loaded extensions
	if (_extensionInfo.extensionLoaded_VK_KHR_draw_indirect_count)
	{
		_extensionInfo.vkCmdDrawIndexedIndirectCountKHR = PFN_vkCmdDrawIndexedIndirectCountKHR(vkGetDeviceProcAddr(_device, "vkCmdDrawIndexedIndirectCountKHR"));
		_extensionInfo.vkCmdDrawIndirectCountKHR = PFN_vkCmdDrawIndirectCountKHR(vkGetDeviceProcAddr(_device, "vkCmdDrawIndirectCountKHR"));
	}

	// Retrieve our requested command queues
	vkGetDeviceQueue(_device, graphicsQueueFamilyIndex, graphicsQueueIndex, &_graphicsQueue);
	vkGetDeviceQueue(_device, transferQueueFamilyIndex, transferQueueIndex, &_transferQueue);
	vkGetDeviceQueue(_device, batchTransferQueueFamilyIndex, batchTransferQueueIndex, &_batchTransferQueue);
	if (separateComputeQueue)
	{
		vkGetDeviceQueue(_device, computeQueueFamilyIndex, computeQueueIndex, &_computeQueue);
	}
	_graphicsQueueFamilyIndex = graphicsQueueFamilyIndex;
	_transferQueueFamilyIndex = transferQueueFamilyIndex;
	_batchTransferQueueFamilyIndex = batchTransferQueueFamilyIndex;
	_computeQueueFamilyIndex = computeQueueFamilyIndex;
	_separateComputeQueue = separateComputeQueue;
	_presentQueueFamilyIndex = presentQueueFamilyIndex;
	if (_presentQueueFamilyIndex == _graphicsQueueFamilyIndex)
	{
		_presentQueue = _graphicsQueue;
	}
	else if ((_presentQueueFamilyIndex == _transferQueueFamilyIndex) && (transferQueueIndex == 0))
	{
		_presentQueue = _transferQueue;
	}
	else if ((_presentQueueFamilyIndex == _batchTransferQueueFamilyIndex) && (batchTransferQueueIndex == 0))
	{
		_presentQueue = _batchTransferQueue;
	}
	else if ((_presentQueueFamilyIndex == _computeQueueFamilyIndex) && (computeQueueIndex == 0))
	{
		_presentQueue = (separateComputeQueue ? _computeQueue : _graphicsQueue);
	}
	else
	{
		vkGetDeviceQueue(_device, _presentQueueFamilyIndex, presentQueueIndex, &_presentQueue);
	}

	// Create memory manager
	_memoryManager = std::make_unique<VulkanMemoryManager>(_log.get(), this);

	// Create command pool for graphics commands
	VkCommandPoolCreateInfo graphicsPoolCreateInfo = {};
	graphicsPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	graphicsPoolCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
	graphicsPoolCreateInfo.flags = 0;
	VkResult vkCreateCommandPoolResult = vkCreateCommandPool(_device, &graphicsPoolCreateInfo, nullptr, &_graphicsCommandPool);
	if (vkCreateCommandPoolResult != VK_SUCCESS)
	{
		_log.get()->Error("vkCreateCommandPool failed for graphics pool with error {0}", vkCreateCommandPoolResult);
		return false;
	}

	// Create command pool for transfer commands
	VkCommandPoolCreateInfo transferPoolCreateInfo = {};
	transferPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	transferPoolCreateInfo.queueFamilyIndex = transferQueueFamilyIndex;
	transferPoolCreateInfo.flags = 0;
	vkCreateCommandPoolResult = vkCreateCommandPool(_device, &transferPoolCreateInfo, nullptr, &_transferCommandPool);
	if (vkCreateCommandPoolResult != VK_SUCCESS)
	{
		_log.get()->Error("vkCreateCommandPool failed for transfer pool with error {0}", vkCreateCommandPoolResult);
		return false;
	}

	// Create command pool for batch transfer commands
	VkCommandPoolCreateInfo batchTransferPoolCreateInfo = {};
	batchTransferPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	batchTransferPoolCreateInfo.queueFamilyIndex = batchTransferQueueFamilyIndex;
	batchTransferPoolCreateInfo.flags = 0;
	vkCreateCommandPoolResult = vkCreateCommandPool(_device, &batchTransferPoolCreateInfo, nullptr, &_batchTransferCommandPool);
	if (vkCreateCommandPoolResult != VK_SUCCESS)
	{
		_log.get()->Error("vkCreateCommandPool failed for batch transfer pool with error {0}", vkCreateCommandPoolResult);
		return false;
	}

	// Create command pool for compute commands if required
	if (separateComputeQueue)
	{
		VkCommandPoolCreateInfo computePoolCreateInfo = {};
		computePoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		computePoolCreateInfo.queueFamilyIndex = computeQueueFamilyIndex;
		computePoolCreateInfo.flags = 0;
		vkCreateCommandPoolResult = vkCreateCommandPool(_device, &computePoolCreateInfo, nullptr, &_computeCommandPool);
		if (vkCreateCommandPoolResult != VK_SUCCESS)
		{
			_log.get()->Error("vkCreateCommandPool failed for compute pool with error {0}", vkCreateCommandPoolResult);
			return false;
		}
	}

	// Create a fence object for our draw commands
	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = 0;
	VkResult createDrawFenceResult = vkCreateFence(_device, &fenceInfo, nullptr, &_drawFence);
	if (createDrawFenceResult != VK_SUCCESS)
	{
		_log.get()->Error("Could not create draw fence with error code {0}", createDrawFenceResult);
		return false;
	}

	// Create a fence object for our build commands
	fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = 0;
	VkResult createTransferFenceResult = vkCreateFence(_device, &fenceInfo, nullptr, &_transferCommandsCompleteFence);
	if (createTransferFenceResult != VK_SUCCESS)
	{
		_log.get()->Error("Could not create transfer fence with error code {0}", createTransferFenceResult);
		return false;
	}

	// Create a semaphore to signal when the first stage of drawing is completed
	VkSemaphoreCreateInfo drawPrepFinishedSemaphoreInfo = {};
	drawPrepFinishedSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	drawPrepFinishedSemaphoreInfo.flags = 0;
	VkResult createDrawPrepFinishedSemaphoreResult = vkCreateSemaphore(_device, &drawPrepFinishedSemaphoreInfo, nullptr, &_drawPrepFinishedSemaphore);
	if (createDrawPrepFinishedSemaphoreResult != VK_SUCCESS)
	{
		_log.get()->Error("Could not create draw prep finished semaphore with error code {0}", createDrawPrepFinishedSemaphoreResult);
		return false;
	}

	// If we're using a separate compute queue, create some additional fences for synchronization. Note that we also
	// need to do this when we are missing features that require us to split drawing into several command buffers for
	// synchronization purposes. A lack of the VK_KHR_draw_indirect_count extension when the IndirectDraw feature is
	// requested currently does this.
	if (_separateComputeQueue || _indirectDrawCountFromBufferFallbackActive)
	{
		// Create a semaphore to signal when we're transitioning to a compute command buffer
		VkSemaphoreCreateInfo computeStartSemaphoreInfo = {};
		computeStartSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		computeStartSemaphoreInfo.flags = 0;
		VkResult createComputeStartSemaphoreResult = vkCreateSemaphore(_device, &computeStartSemaphoreInfo, nullptr, &_computeStartSemaphore);
		if (createComputeStartSemaphoreResult != VK_SUCCESS)
		{
			_log.get()->Error("Could not create compute start semaphore with error code {0}", createComputeStartSemaphoreResult);
			return false;
		}

		// Create a semaphore to signal when we're transitioning from a compute command buffer
		VkSemaphoreCreateInfo computeEndSemaphoreInfo = {};
		computeEndSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		computeEndSemaphoreInfo.flags = 0;
		VkResult createComputeEndSemaphoreResult = vkCreateSemaphore(_device, &computeEndSemaphoreInfo, nullptr, &_computeEndSemaphore);
		if (createComputeEndSemaphoreResult != VK_SUCCESS)
		{
			_log.get()->Error("Could not create compute end semaphore with error code {0}", createComputeEndSemaphoreResult);
			return false;
		}
	}

	// Create a semaphore to signal when drawing is completed
	VkSemaphoreCreateInfo drawCompleteSemaphoreInfo = {};
	drawCompleteSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	drawCompleteSemaphoreInfo.flags = 0;
	VkResult createDrawCompleteSemaphoreResult = vkCreateSemaphore(_device, &drawCompleteSemaphoreInfo, nullptr, &_drawCompleteSemaphore);
	if (createDrawCompleteSemaphoreResult != VK_SUCCESS)
	{
		_log.get()->Error("Could not create draw complete semaphore with error code {0}", createDrawCompleteSemaphoreResult);
		return false;
	}

	// Start render thread
	_renderThreadActive = true;
	_frameAdvanceInProgress = false;
	_buildingInProgress = true;
	_drawingInProgress = false;
	_buildToDrawRequestPending = false;
	_renderRequestPending = false;
	_swapRequestPending = false;
	_deleteLastDrawResourcesRequestPending = false;
	_earlyDeleteNextDrawResourcesRequestPending = false;
	std::thread workerThread(std::bind(std::mem_fn(&VulkanRenderer::RenderThread), this));
	workerThread.detach();
	return true;
}

//----------------------------------------------------------------------------------------
void VulkanRenderer::Delete()
{
	// Terminate the render worker thread
	std::unique_lock<std::mutex> lock(_renderThreadMutex);
	if (_renderThreadActive)
	{
		_renderThreadActive = false;
		_notifyRenderThreadTaskPending.notify_all();
		_notifyRenderThreadStopped.wait(lock);
	}
	lock.unlock();

	// Delete any objects which are pending deletion
	if (_device != VK_NULL_HANDLE)
	{
		PerformDeleteLastDrawResourcesOperation();
		PerformDeleteNextDrawResourcesOperation();
	}

	// If any temporary transfer buffers were allocated by either the last draw or build phase, destroy them now.
	if (!_state[_buildIndex].transferBufferAllocations.empty())
	{
		for (TransferBufferAllocation& entry : _state[_buildIndex].transferBufferAllocations)
		{
			_memoryManager->DestroyBuffer(entry.buffer, entry.bufferAllocation);
		}
		_state[_buildIndex].transferBufferAllocations.clear();
	}
	if (!_state[_drawIndex].transferBufferAllocations.empty())
	{
		for (TransferBufferAllocation& entry : _state[_drawIndex].transferBufferAllocations)
		{
			_memoryManager->DestroyBuffer(entry.buffer, entry.bufferAllocation);
		}
		_state[_drawIndex].transferBufferAllocations.clear();
	}

	// Wait for any running detached batch transfers to complete
	while (_detachedTransferBatchCount > 0)
	{
		lock.lock();
		_detachedTransferBatchCountReachedZero.wait(lock);
		lock.unlock();
	}

	if (_device != VK_NULL_HANDLE)
	{
		// Destroy any resources that were created to support null descriptor fallback
		DestroyNullDescriptorFallbackResources();

		// Destroy any transfer command buffers
		if (!_transferCommandBuffers.empty())
		{
			vkFreeCommandBuffers(_device, _transferCommandPool, (uint32_t)_transferCommandBuffers.size(), _transferCommandBuffers.data());
		}

		// Destroy all graphics and compute command buffers
		if (!_commandBuffers.empty())
		{
			vkFreeCommandBuffers(_device, _graphicsCommandPool, (uint32_t)_commandBuffers.size(), _commandBuffers.data());
		}
		if (!_computeCommandBuffers.empty())
		{
			vkFreeCommandBuffers(_device, _computeCommandPool, (uint32_t)_computeCommandBuffers.size(), _computeCommandBuffers.data());
		}
		if (_graphicsQueueReleaseCommandBuffer != VK_NULL_HANDLE)
		{
			vkFreeCommandBuffers(_device, _graphicsCommandPool, 1, &_graphicsQueueReleaseCommandBuffer);
		}

		// Destroy our synchronization primitives
		vkDestroyFence(_device, _drawFence, nullptr);
		vkDestroyFence(_device, _transferCommandsCompleteFence, nullptr);
		vkDestroySemaphore(_device, _drawPrepFinishedSemaphore, nullptr);
		vkDestroySemaphore(_device, _drawCompleteSemaphore, nullptr);
		for (auto semaphore : _graphicsQueueReleaseCompleteSemaphores)
		{
			vkDestroySemaphore(_device, semaphore, nullptr);
		}
		if (_computeStartSemaphore != VK_NULL_HANDLE)
		{
			vkDestroySemaphore(_device, _computeStartSemaphore, nullptr);
		}
		if (_computeEndSemaphore != VK_NULL_HANDLE)
		{
			vkDestroySemaphore(_device, _computeEndSemaphore, nullptr);
		}

		// Destroy our command pools
		vkDestroyCommandPool(_device, _graphicsCommandPool, nullptr);
		vkDestroyCommandPool(_device, _transferCommandPool, nullptr);
		vkDestroyCommandPool(_device, _batchTransferCommandPool, nullptr);
		if (_separateComputeQueue)
		{
			vkDestroyCommandPool(_device, _computeCommandPool, nullptr);
		}

		// Destroy our memory manager before the device
		_memoryManager.reset();

		// Destroy our device object
		vkDestroyDevice(_device, nullptr);
	}

	// Destroy our memory manager if it wasn't already destroyed
	_memoryManager.reset();

	// Delete this object
	delete this;
}

//----------------------------------------------------------------------------------------
// Geometry buffer methods
//----------------------------------------------------------------------------------------
IVertexBuffer* VulkanRenderer::CreateVertexBufferInternal()
{
	return new VulkanVertexBuffer(_log.get(), this);
}

//----------------------------------------------------------------------------------------
IIndexBuffer* VulkanRenderer::CreateIndexBufferInternal()
{
	return new VulkanIndexBuffer(_log.get(), this);
}

//----------------------------------------------------------------------------------------
// Image buffer methods
//----------------------------------------------------------------------------------------
ITextureBuffer1D* VulkanRenderer::CreateTextureBuffer1DInternal()
{
	return new VulkanTextureBuffer1D(_log.get(), this);
}

//----------------------------------------------------------------------------------------
ITextureBuffer2D* VulkanRenderer::CreateTextureBuffer2DInternal()
{
	return new VulkanTextureBuffer2D(_log.get(), this);
}

//----------------------------------------------------------------------------------------
ITextureBuffer3D* VulkanRenderer::CreateTextureBuffer3DInternal()
{
	return new VulkanTextureBuffer3D(_log.get(), this);
}

//----------------------------------------------------------------------------------------
ITextureBufferCube* VulkanRenderer::CreateTextureBufferCubeInternal()
{
	return new VulkanTextureBufferCube(_log.get(), this);
}

//----------------------------------------------------------------------------------------
ITextureBuffer1DArray* VulkanRenderer::CreateTextureBuffer1DArrayInternal()
{
	return new VulkanTextureBuffer1DArray(_log.get(), this);
}

//----------------------------------------------------------------------------------------
ITextureBuffer2DArray* VulkanRenderer::CreateTextureBuffer2DArrayInternal()
{
	return new VulkanTextureBuffer2DArray(_log.get(), this);
}

//----------------------------------------------------------------------------------------
ITextureBufferCubeArray* VulkanRenderer::CreateTextureBufferCubeArrayInternal()
{
	return new VulkanTextureBufferCubeArray(_log.get(), this);
}

//----------------------------------------------------------------------------------------
// Image sampler methods
//----------------------------------------------------------------------------------------
ITextureSampler1D* VulkanRenderer::CreateTextureSampler1DInternal()
{
	return new VulkanTextureSampler1D(_log.get(), this);
}

//----------------------------------------------------------------------------------------
ITextureSampler2D* VulkanRenderer::CreateTextureSampler2DInternal()
{
	return new VulkanTextureSampler2D(_log.get(), this);
}

//----------------------------------------------------------------------------------------
ITextureSampler3D* VulkanRenderer::CreateTextureSampler3DInternal()
{
	return new VulkanTextureSampler3D(_log.get(), this);
}

//----------------------------------------------------------------------------------------
ITextureSamplerCube* VulkanRenderer::CreateTextureSamplerCubeInternal()
{
	return new VulkanTextureSamplerCube(_log.get(), this);
}

//----------------------------------------------------------------------------------------
ITextureSampler1DArray* VulkanRenderer::CreateTextureSampler1DArrayInternal()
{
	return new VulkanTextureSampler1DArray(_log.get(), this);
}

//----------------------------------------------------------------------------------------
ITextureSampler2DArray* VulkanRenderer::CreateTextureSampler2DArrayInternal()
{
	return new VulkanTextureSampler2DArray(_log.get(), this);
}

//----------------------------------------------------------------------------------------
ITextureSamplerCubeArray* VulkanRenderer::CreateTextureSamplerCubeArrayInternal()
{
	return new VulkanTextureSamplerCubeArray(_log.get(), this);
}

//----------------------------------------------------------------------------------------
// Data array methods
//----------------------------------------------------------------------------------------
IDataArray* VulkanRenderer::CreateDataArrayInternal()
{
	return new VulkanDataArray(_log.get(), this);
}

//----------------------------------------------------------------------------------------
IDataArrayOutput* VulkanRenderer::CreateDataArrayOutputInternal()
{
	return new VulkanDataArrayOutput(_log.get(), this);
}

//----------------------------------------------------------------------------------------
ITexelArray* VulkanRenderer::CreateTexelArrayInternal()
{
	return new VulkanTexelArray(_log.get(), this);
}

//----------------------------------------------------------------------------------------
ITexelArrayOutput* VulkanRenderer::CreateTexelArrayOutputInternal()
{
	return new VulkanTexelArrayOutput(_log.get(), this);
}

//----------------------------------------------------------------------------------------
// Batch methods
//----------------------------------------------------------------------------------------
ITransferBatch* VulkanRenderer::CreateTransferBatchInternal(ITransferBatch::StartTiming startTiming, ITransferBatch::EndTiming endTiming)
{
	return new VulkanTransferBatch(_log.get(), this, startTiming, endTiming);
}

//----------------------------------------------------------------------------------------
// Frame buffer methods
//----------------------------------------------------------------------------------------
IFrameBuffer* VulkanRenderer::CreateFrameBufferInternal()
{
	return new VulkanFrameBuffer(_log.get(), this);
}

//----------------------------------------------------------------------------------------
IFrameBufferOutput* VulkanRenderer::CreateFrameBufferOutputInternal()
{
	return new VulkanFrameBufferOutput(_log.get(), this);
}

//----------------------------------------------------------------------------------------
// State buffer methods
//----------------------------------------------------------------------------------------
IStateBuffer* VulkanRenderer::CreateStateBufferInternal()
{
	return new VulkanStateBuffer(_log.get(), this);
}

//----------------------------------------------------------------------------------------
IStateBufferLayout* VulkanRenderer::CreateStateBufferLayoutInternal()
{
	return new VulkanStateBufferLayout(_log.get(), this);
}

//----------------------------------------------------------------------------------------
// Render tree node methods
//----------------------------------------------------------------------------------------
IRenderPassNode* VulkanRenderer::CreateRenderPassNodeInternal()
{
	return new VulkanRenderPassNode(_log.get(), this);
}

//----------------------------------------------------------------------------------------
IProgramNode* VulkanRenderer::CreateProgramNodeInternal()
{
	return new VulkanProgramNode(_log.get(), this);
}

//----------------------------------------------------------------------------------------
IStateGroupNode* VulkanRenderer::CreateStateGroupNodeInternal()
{
	return new VulkanStateGroupNode(_log.get(), this);
}

//----------------------------------------------------------------------------------------
IRenderableNode* VulkanRenderer::CreateRenderableNodeInternal()
{
	return new VulkanRenderableNode(_log.get(), this);
}

//----------------------------------------------------------------------------------------
IDefaultState* VulkanRenderer::CreateDefaultStateInternal()
{
	return new VulkanDefaultState(_log.get(), this);
}

//----------------------------------------------------------------------------------------
// Program methods
//----------------------------------------------------------------------------------------
IShaderProgram* VulkanRenderer::CreateShaderProgramInternal()
{
	return new VulkanShaderProgram(_log.get(), this);
}

//----------------------------------------------------------------------------------------
// Scene content methods
//----------------------------------------------------------------------------------------
void VulkanRenderer::SetRenderPasses(IRenderPassNode* const* childNodes, size_t childNodeCount, const int32_t* childNodeSortOrder)
{
	std::lock_guard<std::mutex> lock(_buildStateMutex);
	if (childNodeSortOrder != nullptr)
	{
		_state[_buildIndex].renderPasses.clear();
		for (size_t i = 0; i < childNodeCount; ++i)
		{
			RenderPassEntry renderPassEntry = {};
			renderPassEntry.renderPassNode = KnownDynamicCast<VulkanRenderPassNode*>(*(childNodes++));
			renderPassEntry.sortIndex = (childNodeSortOrder != nullptr ? *(childNodeSortOrder++) : (int)i);
			_state[_buildIndex].renderPasses.insert(std::upper_bound(_state[_buildIndex].renderPasses.begin(), _state[_buildIndex].renderPasses.end(), renderPassEntry, RenderPassEntry::Sorter()), renderPassEntry);
		}
	}
	else
	{
		_state[_buildIndex].renderPasses.resize(childNodeCount);
		for (size_t i = 0; i < childNodeCount; ++i)
		{
			RenderPassEntry& renderPassEntry = _state[_buildIndex].renderPasses[i];
			renderPassEntry.renderPassNode = KnownDynamicCast<VulkanRenderPassNode*>(*(childNodes++));
			renderPassEntry.sortIndex = (int)i;
		}
	}
}

//----------------------------------------------------------------------------------------
void VulkanRenderer::SetRenderPasses(IRenderPassNode::unique_ptr const* childNodes, size_t childNodeCount, const int32_t* childNodeSortOrder)
{
	std::lock_guard<std::mutex> lock(_buildStateMutex);
	if (childNodeSortOrder != nullptr)
	{
		_state[_buildIndex].renderPasses.clear();
		for (size_t i = 0; i < childNodeCount; ++i)
		{
			RenderPassEntry renderPassEntry = {};
			renderPassEntry.renderPassNode = KnownDynamicCast<VulkanRenderPassNode*>((childNodes++)->get());
			renderPassEntry.sortIndex = (childNodeSortOrder != nullptr ? *(childNodeSortOrder++) : (int)i);
			_state[_buildIndex].renderPasses.insert(std::upper_bound(_state[_buildIndex].renderPasses.begin(), _state[_buildIndex].renderPasses.end(), renderPassEntry, RenderPassEntry::Sorter()), renderPassEntry);
		}
	}
	else
	{
		_state[_buildIndex].renderPasses.resize(childNodeCount);
		for (size_t i = 0; i < childNodeCount; ++i)
		{
			RenderPassEntry& renderPassEntry = _state[_buildIndex].renderPasses[i];
			renderPassEntry.renderPassNode = KnownDynamicCast<VulkanRenderPassNode*>((childNodes++)->get());
			renderPassEntry.sortIndex = (int)i;
		}
	}
}

//----------------------------------------------------------------------------------------
void VulkanRenderer::RemoveAllRenderPasses()
{
	std::lock_guard<std::mutex> lock(_buildStateMutex);
	_state[_buildIndex].renderPasses.clear();
}

//----------------------------------------------------------------------------------------
// Render methods
//----------------------------------------------------------------------------------------
void VulkanRenderer::StartNewFrame()
{
	// Ensure a frame advance operation isn't already in progress
	std::unique_lock<std::mutex> lock(_renderThreadMutex);
	if (_frameAdvanceInProgress)
	{
		_log->Error("StartNewFrame was called when a previous call was still in progress");
		return;
	}
	_frameAdvanceInProgress = true;

	// Ensure the previous frame has finished rendering, and that all resources being freed from the last frame have
	// been cleaned up. We need to do this as we're about to overwrite state data from the last frame.
	while (_drawingInProgress || _swapRequestPending || _deleteLastDrawResourcesRequestPending || _earlyDeleteNextDrawResourcesRequestPending)
	{
		_notifyRenderThreadTaskComplete.wait(lock);
	}

	// Submit a build to draw request to the render thread if required, and wait for it to be processed. This will
	// also signal that a drawing operation is now in progress.
	if (_buildingInProgress)
	{
		_buildToDrawRequestPending = true;
		_notifyRenderThreadTaskPending.notify_all();
		while (_buildToDrawRequestPending)
		{
			_notifyRenderThreadTaskComplete.wait(lock);
		}
	}

	// Flag that we're beginning to build a new frame
	_buildingInProgress = true;

	// Signal that a frame advance operation is no longer in progress
	_frameAdvanceInProgress = false;
}

//----------------------------------------------------------------------------------------
void VulkanRenderer::WaitForDrawComplete() const
{
	// Wait for any current drawing operation to complete
	std::unique_lock<std::mutex> lock(_renderThreadMutex);
	while (_drawingInProgress)
	{
		_notifyRenderThreadTaskComplete.wait(lock);
	}
}

//----------------------------------------------------------------------------------------
void VulkanRenderer::WaitForOutputCaptureComplete() const
{
	// Wait for any current drawing operation to complete
	std::unique_lock<std::mutex> lock(_renderThreadMutex);
	while (_drawingInProgress)
	{
		_notifyRenderThreadTaskComplete.wait(lock);
	}

	// Flag to any framebuffer outputs that captured data in the frame we just completed a draw for that the application
	// is allowed to read the captured output from the live draw state. We can do this safely at this point as the
	// application has explicitly synchronized with the completion of framebuffer output capture, which it should be
	// noted is not necessarily complete when the draw process itself is complete. In the case of our renderer here it
	// currently is the same, so we use the same synchronization point.
	for (auto* entry : _capturedFramebufferOutputsInCurrentFrame)
	{
		entry->EnableCaptureReadFromCurrentDrawBuffer();
	}
	for (auto* entry : _capturedDataArrayOutputsInCurrentFrame)
	{
		entry->EnableCaptureReadFromCurrentDrawBuffer();
	}
	for (auto* entry : _capturedTexelArrayOutputsInCurrentFrame)
	{
		entry->EnableCaptureReadFromCurrentDrawBuffer();
	}
}

//----------------------------------------------------------------------------------------
void VulkanRenderer::WaitForDeferredDeletionComplete() const
{
	// Ensure the previous frame has finished rendering, and that all resources being freed from the last frame have
	// been cleaned up. Additionally, we also request early deletion of resources associated with the next frame. We
	// can do this safely, since we're synchronizing with the end of the previous frame first. This creates a safe
	// point at which external window resources can be freed without a new frame being sent to the renderer.
	std::unique_lock<std::mutex> lock(_renderThreadMutex);
	_earlyDeleteNextDrawResourcesRequestPending = true;
	_notifyRenderThreadTaskPending.notify_all();
	while (_drawingInProgress || _swapRequestPending || _deleteLastDrawResourcesRequestPending || _earlyDeleteNextDrawResourcesRequestPending)
	{
		_notifyRenderThreadTaskComplete.wait(lock);
	}
}

//----------------------------------------------------------------------------------------
void VulkanRenderer::AddCurrentFrameBufferOutput(VulkanFrameBufferOutput* frameBufferOutput)
{
	_capturedFramebufferOutputsInCurrentFrame.push_back(frameBufferOutput);
}

//----------------------------------------------------------------------------------------
void VulkanRenderer::AddCurrentDataArrayOutput(VulkanDataArrayOutput* resourceBufferOutput)
{
	_capturedDataArrayOutputsInCurrentFrame.push_back(resourceBufferOutput);
}

//----------------------------------------------------------------------------------------
void VulkanRenderer::AddCurrentDataArray(VulkanDataArray* resourceBuffer)
{
	_boundDataArrays.push_back(resourceBuffer);
}

//----------------------------------------------------------------------------------------
void VulkanRenderer::AddCurrentTexelArrayOutput(VulkanTexelArrayOutput* resourceBufferOutput)
{
	_capturedTexelArrayOutputsInCurrentFrame.push_back(resourceBufferOutput);
}

//----------------------------------------------------------------------------------------
void VulkanRenderer::AddCurrentTexelArray(VulkanTexelArray* resourceBuffer)
{
	_boundTexelArrays.push_back(resourceBuffer);
}

//----------------------------------------------------------------------------------------
void VulkanRenderer::RenderThread()
{
	std::unique_lock<std::mutex> lock(_renderThreadMutex);
	bool done = false;
	while (!done)
	{
		// Wait for work to be requested or the thread to be requested to terminate
		if (!_buildToDrawRequestPending && !_renderRequestPending && !_swapRequestPending && !_deleteLastDrawResourcesRequestPending && !_earlyDeleteNextDrawResourcesRequestPending)
		{
			// If the render thread has not already been instructed to stop, suspend this thread until a render task is
			// issued or this thread is instructed to stop.
			if (_renderThreadActive)
			{
				_notifyRenderThreadTaskPending.wait(lock);
			}

			// If the render thread has been suspended, flag that we need to exit the render loop.
			done = !_renderThreadActive;

			continue;
		}

		// Action the next pending render request
		if (_deleteLastDrawResourcesRequestPending)
		{
			lock.unlock();
			PerformDeleteLastDrawResourcesOperation();
			lock.lock();
			_deleteLastDrawResourcesRequestPending = false;
		}
		else if (_buildToDrawRequestPending)
		{
			_drawingInProgress = true;
			lock.unlock();
			PerformPrepareBuildOperation();
			lock.lock();
			_buildToDrawRequestPending = false;
			_renderRequestPending = true;
		}
		else if (_renderRequestPending)
		{
			lock.unlock();
			PerformRenderOperation();
			lock.lock();
			_renderRequestPending = false;
			_swapRequestPending = true;
		}
		else if (_swapRequestPending)
		{
			lock.unlock();
			PerformSwapOperation();
			lock.lock();
			_swapRequestPending = false;
			_drawingInProgress = false;
			_deleteLastDrawResourcesRequestPending = true;
		}
		else if (_earlyDeleteNextDrawResourcesRequestPending)
		{
			// Note that there's a priority order here. This request must be processed after all the above.
			lock.unlock();
			PerformDeleteNextDrawResourcesOperation();
			lock.lock();
			_earlyDeleteNextDrawResourcesRequestPending = false;
		}

		// Notify any waiting threads that a render thread task has completed
		_notifyRenderThreadTaskComplete.notify_all();
	}

	// Notify any waiting threads that the render thread has now terminated
	_notifyRenderThreadStopped.notify_all();
}

//----------------------------------------------------------------------------------------
void VulkanRenderer::PerformPrepareBuildOperation()
{
	// Transition our build state to our draw state
	std::swap(_buildIndex, _drawIndex);
	_state[_buildIndex].renderPasses = _state[_drawIndex].renderPasses;
	_state[_buildIndex].migrateStatePendingObjects.clear();
	_state[_buildIndex].bufferUpdatePendingObjects.clear();
	_state[_buildIndex].bufferTransferPendingObjects.clear();

	// Transfer build state into draw state in our scene tree
	for (const RenderPassEntry& renderPassEntry : _state[_drawIndex].renderPasses)
	{
		renderPassEntry.renderPassNode->MigrateBuildStateToDrawState();
	}

	// Transfer build state info draw state in our modified standalone objects
	for (const auto& entry : _state[_drawIndex].migrateStatePendingObjects)
	{
		std::visit([](auto* object) { object->MigrateBuildStateToDrawState(); }, entry);
	}

	// Obtain a lock on our transfer pool. We do this to ensure that the _transferCommandBuffers member only has
	// complete and submitted buffers in it, and that the contents don't change while we manipulate them here.
	std::unique_lock<std::mutex> lock(_transferPoolMutex);
	while (_transferPoolLocked.test_and_set(std::memory_order_acquire))
	{
		_transferPoolLockReleased.wait(lock);
	}

	// Submit any delayed transfer commands for execution which are still pending
	ExecutePendingDelayedTransferCommands();

	// If any transfer commands were issued during the build stage, complete them now.
	if (_frameStartTransferCommandsInProgress)
	{
		// Wait for all our build commands to complete. Note that we add a fence and wait for it here rather than using
		// vkQueueWaitIdle. The two commands are roughly equivalent generally speaking, but since we may in fact be
		// sharing one underlying queue between our graphics, transfer, and batch operations, we don't assume
		// exclusivity here and insert a fence at the end, which allows other commands to be submitted after our fence
		// while we're still waiting for completion without causing a longer delay than intended.
		std::unique_lock<std::mutex> queueLock(_queueMutex);
		VkResult vkResetFencesResult = vkResetFences(_device, 1, &_transferCommandsCompleteFence);
		if (vkResetFencesResult != VK_SUCCESS)
		{
			_log->Error("vkResetFences failed with error code {0} for frame start transfer commands", vkResetFencesResult);
		}
		VkResult vkQueueSubmitResult = vkQueueSubmit(_transferQueue, 0, nullptr, _transferCommandsCompleteFence);
		if (vkQueueSubmitResult != VK_SUCCESS)
		{
			_log->Error("vkQueueSubmit failed with error code {0} for frame start transfer commands", vkQueueSubmitResult);
		}
		queueLock.unlock();
		VkResult vkWaitForFencesResult = vkWaitForFences(_device, 1, &_transferCommandsCompleteFence, VK_TRUE, std::numeric_limits<uint64_t>::max());
		if (vkWaitForFencesResult != VK_SUCCESS)
		{
			_log->Error("vkWaitForFences failed with error code {0} for frame start transfer commands", vkWaitForFencesResult);
		}
		_frameStartTransferCommandsInProgress = false;

		// Free any allocated command buffers from the build stage
		if (!_transferCommandBuffers.empty())
		{
			vkFreeCommandBuffers(_device, _transferCommandPool, (uint32_t)_transferCommandBuffers.size(), _transferCommandBuffers.data());
			_transferCommandBuffers.clear();
		}
	}

	// Release the lock on the transfer pool
	_transferPoolLocked.clear(std::memory_order_release);
	_transferPoolLockReleased.notify_one();

	// If any temporary transfer buffers were allocated to be freed during this new frame, destroy them now.
	if (!_state[_drawIndex].transferBufferAllocations.empty())
	{
		for (TransferBufferAllocation& entry : _state[_drawIndex].transferBufferAllocations)
		{
			_memoryManager->DestroyBuffer(entry.buffer, entry.bufferAllocation);
		}
		_state[_drawIndex].transferBufferAllocations.clear();
	}
}

//----------------------------------------------------------------------------------------
void VulkanRenderer::PerformRenderOperation()
{
	// Destroy all graphics and compute command buffers
	if (!_commandBuffers.empty())
	{
		vkFreeCommandBuffers(_device, _graphicsCommandPool, (uint32_t)_commandBuffers.size(), _commandBuffers.data());
	}
	if (!_computeCommandBuffers.empty())
	{
		vkFreeCommandBuffers(_device, _computeCommandPool, (uint32_t)_computeCommandBuffers.size(), _computeCommandBuffers.data());
		_computeCommandBuffers.clear();
	}
	_commandBuffers.resize(DefaultCommandBufferCount);
	_currentCommandBufferIndex = 0;

	// Render marker structure
	VkDebugUtilsLabelEXT renderMarkerLabel;
	renderMarkerLabel.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
	renderMarkerLabel.pNext = nullptr;

	// Create initial command buffers
	VkCommandBufferAllocateInfo commandBufferInfo = {};
	commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferInfo.commandPool = _graphicsCommandPool;
	commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferInfo.commandBufferCount = DefaultCommandBufferCount;
	VkResult vkAllocateCommandBuffersResult = vkAllocateCommandBuffers(_device, &commandBufferInfo, _commandBuffers.data());
	if (vkAllocateCommandBuffersResult != VK_SUCCESS)
	{
		_log->Error("vkAllocateCommandBuffers failed with error code {0} for draw process", vkAllocateCommandBuffersResult);
		return;
	}

	// Start recording first command buffer
	VkCommandBuffer currentCommandBuffer = _commandBuffers[_currentCommandBufferIndex];
	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	VkResult vkBeginCommandBufferResult = vkBeginCommandBuffer(currentCommandBuffer, &commandBufferBeginInfo);
	if (vkBeginCommandBufferResult != VK_SUCCESS)
	{
		_log->Error("vkBeginCommandBuffer failed with error code {0} for draw command buffer {1}", vkBeginCommandBufferResult, _currentCommandBufferIndex);
	}

	// Push a render marker if required
	if (_useRenderMarkers)
	{
		renderMarkerLabel.pLabelName = "Setup";
		ConvertColorToFloat(SetupMarkerColor(), &renderMarkerLabel.color[0]);
		_pfnCmdBeginDebugUtilsLabelEXT(currentCommandBuffer, &renderMarkerLabel);
	}

	// If we have separate transfer and graphics queues, perform pending graphics queue acquire operations now.
	if (!IsTransferQueueSharedWithGraphics())
	{
		std::scoped_lock<std::mutex> lock(_pendingGraphicsQueueTransferMutex);
		for (const auto& entry : _pendingGraphicsQueueAcquireOperations)
		{
			std::visit([currentCommandBuffer](auto* object) { object->PerformGraphicsQueueAcquireOperation(currentCommandBuffer); }, entry);
		}
		_pendingGraphicsQueueAcquireOperations.clear();
	}

	// Initiate any pending data transfer operations within GPU memory
	for (const auto& entry : _state[_drawIndex].bufferTransferPendingObjects)
	{
		std::visit([currentCommandBuffer](auto* object) { object->CompletePendingDataTransfers(currentCommandBuffer); }, entry);
	}

	// Initiate any pending data transfer operations from CPU to GPU memory
	for (const auto& entry : _state[_drawIndex].bufferUpdatePendingObjects)
	{
		std::visit([currentCommandBuffer](auto* object) { object->CompletePendingDataWrites(currentCommandBuffer); }, entry);
	}

	// Since it's possible for the same shader program to be used more than once in the render tree, and there's some
	// data which is tracked per shader, we do a quick pre-pass to the program node level here to flag all active shader
	// programs in the scene as uninitialized. Although alternate implementations could avoid this extra partial scene
	// traversal, on the balance of things this was considered to be the simplest and possibly fastest approach in the
	// real world. Alternate methods should be profiled to determine effectiveness if changes are made here.
	const std::vector<RenderPassEntry>& renderPassEntries = _state[_drawIndex].renderPasses;
	for (const RenderPassEntry& renderPassEntry : renderPassEntries)
	{
		// If this render pass is disabled, skip it.
		VulkanRenderPassNode* renderPassNode = renderPassEntry.renderPassNode;
		if (!renderPassNode->IsEnabled())
		{
			continue;
		}

		const std::vector<VulkanRenderPassNode::ChildNodeEntry>& programNodeEntries = renderPassNode->GetChildNodes();
		for (const VulkanRenderPassNode::ChildNodeEntry& childNodeEntry : programNodeEntries)
		{
			VulkanShaderProgram* shaderProgram = childNodeEntry.node->GetShaderProgram();
			shaderProgram->ClearDescriptorSets();
			shaderProgram->ResetGlobalConstantBufferState();
		}
	}

	// Traverse the render tree to fill our global shared state buffer. This technique is the current state of the art
	// in preparing per-frame constant data buffers. See the following article from NVidia for more information:
	// https://developer.nvidia.com/content/constant-buffers-without-constant-pain-0
	_captureTargetsPresent = false;
	_boundWindowFramebuffers.clear();
	_boundTextureFramebuffers.clear();
	_boundDataArrays.clear();
	_boundTexelArrays.clear();
	for (const RenderPassEntry& renderPassEntry : renderPassEntries)
	{
		// If this render pass is disabled, skip it.
		VulkanRenderPassNode* renderPassNode = renderPassEntry.renderPassNode;
		if (!renderPassNode->IsEnabled())
		{
			continue;
		}

		// Process each child program node
		const std::vector<VulkanRenderPassNode::ChildNodeEntry>& programNodeEntries = renderPassNode->GetChildNodes();
		for (const VulkanRenderPassNode::ChildNodeEntry& childNodeEntry : programNodeEntries)
		{
			VulkanProgramNode* programNode = childNodeEntry.node;
			VulkanShaderProgram* shaderProgram = nullptr;
			VulkanShaderProgram::GlobalStateBufferBuildingSession constantBufferBuildingSession = {};

			// Retrieve any any textures or state buffer entries set as defaults at the render pass level
			VulkanDefaultState* defaultState = childNodeEntry.defaultState;
			const std::vector<ITextureBindingInfo*>* defaultTextureEntries = nullptr;
			const std::vector<ISamplerBindingInfo*>* defaultSamplerEntries = nullptr;
			const std::vector<StateBufferBindingInfo*>* defaultStateBufferEntries = nullptr;
			const std::vector<ResourceArrayBindingInfo*>* defaultResourceBufferEntries = nullptr;
			const std::vector<IStateValueInfo*>* defaultStateValueEntries = nullptr;
			if (defaultState != nullptr)
			{
				defaultTextureEntries = &defaultState->GetTextureEntries();
				defaultSamplerEntries = &defaultState->GetSamplerEntries();
				defaultStateBufferEntries = &defaultState->GetStateBufferEntries();
				defaultResourceBufferEntries = &defaultState->GetResourceBufferEntries();
				defaultStateValueEntries = &defaultState->GetValueEntries();
			}
			bool hasDefaultTextureEntries = (defaultTextureEntries != nullptr) && !defaultTextureEntries->empty();
			bool hasDefaultSamplerEntries = (defaultSamplerEntries != nullptr) && !defaultSamplerEntries->empty();
			bool hasDefaultStateBufferEntries = (defaultStateBufferEntries != nullptr) && !defaultStateBufferEntries->empty();
			bool hasDefaultStateEntries = (defaultStateValueEntries != nullptr) && !defaultStateValueEntries->empty();
			bool hasDefaultResourceBufferEntries = (defaultResourceBufferEntries != nullptr) && !defaultResourceBufferEntries->empty();
			int constantBufferStateIndex = hasDefaultStateEntries ? childNodeEntry.defaultStateIndex : -1;
			bool boundDefaultEntries = false;

			const std::vector<VulkanStateGroupNode*>& groupNodes = programNode->GetChildNodes();
			for (VulkanStateGroupNode* groupNode : groupNodes)
			{
				bool isComputeTask = groupNode->HasComputeTask();
				bool hasGroupStateValues = false;
				bool checkedGroupStateValues = false;
				bool builtBaseDescriptorSetForGroupNode = false;
				size_t baseIndex = 0;
				auto groupStateValues = groupNode->GetStateBufferEntries();
				auto groupTextureValues = groupNode->GetTextureEntries();
				auto groupSamplerValues = groupNode->GetSamplerEntries();
				auto groupResourceBufferValues = groupNode->GetResourceBufferEntries();
				size_t stateBucketCount = groupNode->GetStateBucketCount();
				for (size_t stateBucketID = 0; stateBucketID < stateBucketCount; ++stateBucketID)
				{
					// If this state bucket is empty, skip it.
					const std::vector<VulkanRenderableNode*>& renderableNodes = groupNode->GetChildNodes(stateBucketID);
					if (!groupNode->HasComputeTask() && renderableNodes.empty())
					{
						continue;
					}

					// If we haven't latched the shader program yet, do it now. We defer this process to minimize work
					// if all the state nodes for a given program node are empty.
					if (shaderProgram == nullptr)
					{
						// Retrieve the shader program
						shaderProgram = programNode->GetShaderProgram();

						// Apply constant state values for the program node
						const auto& constantValueEntries = programNode->GetConstantValueEntries();
						shaderProgram->RestoreGlobalStateBufferBaseline();
						for (IConstantStateValueInfo* constantValue : constantValueEntries)
						{
							constantValue->ApplyValue(shaderProgram);
						}

						// Apply default state values from the program node binding at the render pass level
						if (hasDefaultStateEntries)
						{
							for (const auto& stateValue : *defaultStateValueEntries)
							{
								stateValue->ApplyValue(shaderProgram);
							}
						}

						// Push the current global constant buffer state so we can restore it later
						shaderProgram->PushGlobalStateBufferState();

						// Start a new global constant buffer building session
						shaderProgram->BeginGlobalConstantBufferBuildingSession(currentCommandBuffer, constantBufferBuildingSession);
					}

					// If this state group node sets global constant state, load it now.
					if (!checkedGroupStateValues)
					{
						const auto& stateValueEntriesForGroupNode = groupNode->GetValueEntries();
						hasGroupStateValues = !stateValueEntriesForGroupNode.empty();
						if (hasGroupStateValues)
						{
							// Apply state values for this state group node
							for (const auto& stateValue : stateValueEntriesForGroupNode)
							{
								stateValue->ApplyValue(shaderProgram);
							}

							// Now that we've applied state values, push the current global constant buffer state so we
							// can restore it later.
							shaderProgram->PushGlobalStateBufferState();
						}
						checkedGroupStateValues = true;
					}

					if (!builtBaseDescriptorSetForGroupNode)
					{
						// Create base descriptor set for group node
						baseIndex = shaderProgram->AllocateBindingDescriptorSet();
						groupNode->SetDescriptorSetIndex(baseIndex, childNodeEntry.defaultStateIndex);

						// Pre-fill the base descriptor set with fallback resources if the null descriptor feature is unavailable
						if (NullDescriptorFeatureMissingOrBroken() && (baseIndex != std::numeric_limits<size_t>::max()))
						{
							shaderProgram->BindNullDescriptorFallbacks(baseIndex);
						}

						// Write values for base descriptor set
						if (hasDefaultStateBufferEntries)
						{
							BindStateBuffers(*defaultStateBufferEntries, shaderProgram, baseIndex);
						}
						if (hasDefaultSamplerEntries)
						{
							BindSamplers(*defaultSamplerEntries, shaderProgram, baseIndex);
						}
						if (hasDefaultTextureEntries)
						{
							BindTextures(*defaultTextureEntries, shaderProgram, baseIndex);
						}
						if (hasDefaultResourceBufferEntries)
						{
							BindResourceArrays(*defaultResourceBufferEntries, shaderProgram, baseIndex, currentCommandBuffer, !boundDefaultEntries);
						}
						BindStateBuffers(groupStateValues, shaderProgram, baseIndex);
						BindSamplers(groupSamplerValues, shaderProgram, baseIndex);
						BindTextures(groupTextureValues, shaderProgram, baseIndex);
						BindResourceArrays(groupResourceBufferValues, shaderProgram, baseIndex, currentCommandBuffer, true);
						builtBaseDescriptorSetForGroupNode = true;
						boundDefaultEntries = true;
					}

					// If this is a compute task, generate constant buffer bindings on the group node directly.
					if (isComputeTask)
					{
						shaderProgram->GenerateGlobalConstantBufferBindings(currentCommandBuffer, constantBufferBuildingSession, groupNode->GetGlobalConstantBufferBindingInfo(constantBufferStateIndex));
					}

					// Bind global constant state to each contained renderable node
					for (VulkanRenderableNode* renderableNode : renderableNodes)
					{
						// Apply any state values for this renderable node
						const auto& stateValueEntriesForRenderableNode = renderableNode->GetValueEntries();
						bool hasRenderableStateValues = !stateValueEntriesForRenderableNode.empty();
						if (hasRenderableStateValues)
						{
							for (const auto& stateValue : stateValueEntriesForRenderableNode)
							{
								stateValue->ApplyValue(shaderProgram);
							}
						}

						// Bind the generated global constant data to the target renderable
						shaderProgram->GenerateGlobalConstantBufferBindings(currentCommandBuffer, constantBufferBuildingSession, renderableNode->GetGlobalConstantBufferBindingInfo(constantBufferStateIndex));

						// If we changed state values for this renderable, restore it to the pushed state baseline.
						if (hasRenderableStateValues)
						{
							shaderProgram->RestoreGlobalStateBufferBaseline();
						}

						auto stateValues = renderableNode->GetStateBufferEntries();
						auto textureValues = renderableNode->GetTextureEntries();
						auto samplerValues = renderableNode->GetSamplerEntries();
						auto resourceBufferValues = renderableNode->GetResourceBufferEntries();
						if (stateValues.empty() && textureValues.empty() && samplerValues.empty() && resourceBufferValues.empty())
						{
							// No unique state, copy group node descriptor set
							renderableNode->SetDescriptorSetIndex(baseIndex, childNodeEntry.defaultStateIndex);
						}
						else
						{
							size_t index = shaderProgram->AllocateBindingDescriptorSet();
							renderableNode->SetDescriptorSetIndex(index, childNodeEntry.defaultStateIndex);

							//##TODO## We've implemented support for "cloning" our new descriptor set here from the
							//contents of the descriptor set in the parent node, however reportedly, using
							//VkCopyDescriptorSet can be a very expensive operation, as it can require a round-trip to
							//GPU memory if the descriptor data. Until we've profiled this and tested the performance
							//impacts, we're sticking with the manual rebuilding of the descriptor set here, where we
							//re-bind each resource from higher up the tree rather than using a descriptor copy
							//operation.
							//shaderProgram->CopyDescriptorSet(baseIndex, index);

							// Write group node state and renderable state
							if (hasDefaultStateBufferEntries)
							{
								BindStateBuffers(*defaultStateBufferEntries, shaderProgram, index);
							}
							if (hasDefaultSamplerEntries)
							{
								BindSamplers(*defaultSamplerEntries, shaderProgram, index);
							}
							if (hasDefaultTextureEntries)
							{
								BindTextures(*defaultTextureEntries, shaderProgram, index);
							}
							if (hasDefaultResourceBufferEntries)
							{
								BindResourceArrays(*defaultResourceBufferEntries, shaderProgram, index, currentCommandBuffer, false);
							}
							BindStateBuffers(groupStateValues, shaderProgram, index);
							BindSamplers(groupSamplerValues, shaderProgram, index);
							BindTextures(groupTextureValues, shaderProgram, index);
							BindResourceArrays(groupResourceBufferValues, shaderProgram, index, currentCommandBuffer, false);

							BindStateBuffers(stateValues, shaderProgram, index);
							BindSamplers(samplerValues, shaderProgram, index);
							BindTextures(textureValues, shaderProgram, index);
							BindResourceArrays(resourceBufferValues, shaderProgram, index, currentCommandBuffer, true);
						}
					}
				}

				// If we loaded state from the state group node, pop the global constant buffer state.
				if (hasGroupStateValues)
				{
					shaderProgram->PopGlobalStateBufferState();
				}
			}

			// If we built a global constant buffer for this program node, complete the session now, and pop the global
			// constant buffer state.
			if (shaderProgram != nullptr)
			{
				shaderProgram->CompleteGlobalConstantBufferBuildingSession(currentCommandBuffer, constantBufferBuildingSession);
				shaderProgram->PopGlobalStateBufferState();
			}
		}
	}

	// Pop a render marker if required
	if (_useRenderMarkers)
	{
		_pfnCmdEndDebugUtilsLabelEXT(currentCommandBuffer);
	}

	// Complete our first draw command buffer
	VkResult vkEndCommandBufferResult = vkEndCommandBuffer(currentCommandBuffer);
	if (vkEndCommandBufferResult != VK_SUCCESS)
	{
		_log->Error("vkEndCommandBuffer failed with error code {0} for draw command buffer {1}", vkEndCommandBufferResult, _currentCommandBufferIndex);
	}

	// Submit our first draw command buffer
	VkSubmitInfo drawPrepSubmitInfo = {};
	drawPrepSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	drawPrepSubmitInfo.commandBufferCount = 1;
	drawPrepSubmitInfo.waitSemaphoreCount = 0;
	drawPrepSubmitInfo.pWaitSemaphores = nullptr;
	drawPrepSubmitInfo.pWaitDstStageMask = nullptr;
	drawPrepSubmitInfo.signalSemaphoreCount = 1;
	drawPrepSubmitInfo.pSignalSemaphores = &_drawPrepFinishedSemaphore;
	drawPrepSubmitInfo.pCommandBuffers = &currentCommandBuffer;
	std::unique_lock<std::mutex> queueLock(_queueMutex);
	VkResult vkQueueSubmitResult = vkQueueSubmit(_graphicsQueue, 1, &drawPrepSubmitInfo, VK_NULL_HANDLE);
	if (vkQueueSubmitResult != VK_SUCCESS)
	{
		_log->Error("vkQueueSubmit failed with error code {0} for draw command buffer {1}", vkQueueSubmitResult, _currentCommandBufferIndex);
	}
	queueLock.unlock();

	// Start recording second draw command buffer
	++_currentCommandBufferIndex;
	currentCommandBuffer = _commandBuffers[_currentCommandBufferIndex];
	vkBeginCommandBufferResult = vkBeginCommandBuffer(currentCommandBuffer, &commandBufferBeginInfo);
	if (vkBeginCommandBufferResult != VK_SUCCESS)
	{
		_log->Error("vkBeginCommandBuffer failed with error code {0} for draw command buffer {1}", vkBeginCommandBufferResult, _currentCommandBufferIndex);
	}

	// Keep track of wait and signal semaphores
	_presentSemaphores.clear();
	_drawWaitSemaphores.clear();
	_drawWaitStages.clear();
	_drawWaitSemaphores.push_back(_drawPrepFinishedSemaphore);
	_drawWaitStages.push_back(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

	// Traverse the render tree and perform our draw operations
	bool insideComputeCommandBuffer = false;
	bool submittedFirstDrawCommandBuffer = false;
	bool lastCommandWasCompute = false;
	VulkanFrameBuffer* currentFramebuffer = nullptr;
	for (const RenderPassEntry& renderPassEntry : renderPassEntries)
	{
		// If this render pass is disabled, skip it.
		VulkanRenderPassNode* renderPassNode = renderPassEntry.renderPassNode;
		if (!renderPassNode->IsEnabled())
		{
			continue;
		}

		// Push a render marker if required
		if (_useRenderMarkers)
		{
			renderMarkerLabel.pLabelName = renderPassNode->DebugName().c_str();
			ConvertColorToFloat(RenderPassMarkerColor(), &renderMarkerLabel.color[0]);
			_pfnCmdBeginDebugUtilsLabelEXT(currentCommandBuffer, &renderMarkerLabel);
		}

		// Bind the framebuffer for this render pass
		VulkanFrameBuffer* newFramebuffer = renderPassNode->GetFrameBuffer();
		if (newFramebuffer != currentFramebuffer)
		{
			if (newFramebuffer != nullptr)
			{
				newFramebuffer->PrepareFrameBuffer(currentCommandBuffer);
				if (newFramebuffer->IsBoundToWindow())
				{
					// Add wait and signal semaphores for window renderpass
					_boundWindowFramebuffers.push_back(newFramebuffer);
					_presentSemaphores.push_back(newFramebuffer->GetPresentSemaphore());
					_drawWaitSemaphores.push_back(newFramebuffer->GetPrepareSemaphore());
					_drawWaitStages.push_back(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
				}
				else
				{
					_boundTextureFramebuffers.push_back(newFramebuffer);
				}
				_captureTargetsPresent |= newFramebuffer->HasCaptureTargets();
			}
			currentFramebuffer = newFramebuffer;
		}

		// If we last issued compute commands, we need to insert a pipeline barrier before we begin a render pass for it
		// to function correctly as per the Vulkan spec, as there are different rules for pipeline barriers that are
		// issued within a render pass (as with graphics commands) than there are for barriers that are issued outside a
		// render pass (as with compute commands).
		if (lastCommandWasCompute && !_separateComputeQueue && (currentFramebuffer != nullptr))
		{
			// If compute and graphics are sharing the same queue, we don't have the implicit ordering
			// guarantees provided by our use of vkQueueSubmit with wait and signal semaphores. We therefore
			// need to set up an in-queue execution barrier between our compute and graphics work, to ensure
			// resources modified during compute shaders are visible to the graphics shaders. We provide
			// that barrier here if this is the point at which we're transitioning between compute and
			// graphics work.
			VkMemoryBarrier memoryBarrier = {};
			memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
			memoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
			memoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
			vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
			lastCommandWasCompute = false;
		}

		// Apply fixed state from this render pass node
		renderPassNode->ApplyFixedState(currentCommandBuffer);

		// Render each child node
		bool renderPassBegun = false;
		const std::vector<VulkanRenderPassNode::ChildNodeEntry>& programNodeEntries = renderPassNode->GetChildNodes();
		for (const VulkanRenderPassNode::ChildNodeEntry& childNodeEntry : programNodeEntries)
		{
			bool markedProgram = false;

			// Select the shader program to use for this program node
			VulkanProgramNode* programNode = childNodeEntry.node;
			VulkanShaderProgram* shaderProgram = programNode->GetShaderProgram();

			VulkanDefaultState* defaultState = childNodeEntry.defaultState;
			const std::vector<IStateValueInfo*>* defaultStateValueEntries = nullptr;
			if (defaultState != nullptr)
			{
				defaultStateValueEntries = &defaultState->GetValueEntries();
			}
			bool hasDefaultStateEntries = (defaultStateValueEntries != nullptr) && !defaultStateValueEntries->empty();
			int constantBufferStateIndex = hasDefaultStateEntries ? childNodeEntry.defaultStateIndex : -1;

			// Render each child node
			size_t currentGlobalDescriptorSetIndex = std::numeric_limits<size_t>::max();
			size_t currentDescriptorSetIndex = std::numeric_limits<size_t>::max();
			for (VulkanStateGroupNode* groupNode : programNode->GetChildNodes())
			{
				bool markedGroup = false;

				// Render nodes from each state bucket in the group node
				bool isComputeTask = groupNode->HasComputeTask();
				int stateBucketCount = groupNode->GetStateBucketCount();
				for (int stateBucketID = 0; stateBucketID < stateBucketCount; ++stateBucketID)
				{
					// If this state bucket is empty, skip it.
					const std::vector<VulkanRenderableNode*>& renderableNodes = groupNode->GetChildNodes(stateBucketID);
					if (!isComputeTask && renderableNodes.empty())
					{
						continue;
					}

					// Push a render marker if required
					if (_useRenderMarkers)
					{
						// If program or group hasn't already been started, start marker for it now
						if (!markedProgram)
						{
							renderMarkerLabel.pLabelName = programNode->DebugName().c_str();
							ConvertColorToFloat(ProgramMarkerColor(), &renderMarkerLabel.color[0]);
							_pfnCmdBeginDebugUtilsLabelEXT(currentCommandBuffer, &renderMarkerLabel);
							markedProgram = true;
						}
						if (!markedGroup)
						{
							renderMarkerLabel.pLabelName = groupNode->DebugName().c_str();
							ConvertColorToFloat(GroupMarkerColor(), &renderMarkerLabel.color[0]);
							_pfnCmdBeginDebugUtilsLabelEXT(currentCommandBuffer, &renderMarkerLabel);
							markedGroup = true;
						}

						// Push the render marker
						if (!isComputeTask)
						{
							renderMarkerLabel.pLabelName = "Pipeline";
							ConvertColorToFloat(PipelineMarkerColor(), &renderMarkerLabel.color[0]);
							_pfnCmdBeginDebugUtilsLabelEXT(currentCommandBuffer, &renderMarkerLabel);
						}
					}

					// If this group node is performing a compute task, start a compute command buffer if required,
					// otherwise close off the current compute command buffer if present. If our graphics device only
					// provides separate graphics and compute queues, we need to submit them in an interleaved manner
					// and synchronize the work on each queue.
					if (isComputeTask)
					{
						// Close off the current graphics command buffer and start a new compute command buffer if
						// required
						if (_separateComputeQueue && !insideComputeCommandBuffer)
						{
							// Submit the current draw command buffer
							vkEndCommandBufferResult = vkEndCommandBuffer(currentCommandBuffer);
							if (vkEndCommandBufferResult != VK_SUCCESS)
							{
								_log->Error("vkEndCommandBuffer failed with error code {0} for draw command buffer {1}", vkEndCommandBufferResult, _currentCommandBufferIndex);
							}
							uint32_t waitSemaphoreCount = (!submittedFirstDrawCommandBuffer ? (uint32_t)_drawWaitSemaphores.size() : 1);
							_waitStageMaskBuffer.assign(waitSemaphoreCount, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
							VkSubmitInfo drawSubmitInfo = {};
							drawSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
							drawSubmitInfo.commandBufferCount = 1;
							drawSubmitInfo.pCommandBuffers = &currentCommandBuffer;
							drawSubmitInfo.waitSemaphoreCount = waitSemaphoreCount;
							drawSubmitInfo.pWaitSemaphores = (!submittedFirstDrawCommandBuffer ? _drawWaitSemaphores.data() : &_computeEndSemaphore);
							drawSubmitInfo.pWaitDstStageMask = _waitStageMaskBuffer.data();
							drawSubmitInfo.signalSemaphoreCount = 1;
							drawSubmitInfo.pSignalSemaphores = &_computeStartSemaphore;
							std::unique_lock<std::mutex> queueLock4(_queueMutex);
							vkQueueSubmitResult = vkQueueSubmit(_graphicsQueue, 1, &drawSubmitInfo, VK_NULL_HANDLE);
							if (vkQueueSubmitResult != VK_SUCCESS)
							{
								_log->Error("vkQueueSubmit failed with error code {0} for draw command buffer {1}", vkQueueSubmitResult, _currentCommandBufferIndex);
							}
							queueLock4.unlock();

							// Allocate a new command buffer for the compute task
							commandBufferInfo = {};
							commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
							commandBufferInfo.commandPool = _computeCommandPool;
							commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
							commandBufferInfo.commandBufferCount = 1;
							vkAllocateCommandBuffersResult = vkAllocateCommandBuffers(_device, &commandBufferInfo, &currentCommandBuffer);
							if (vkAllocateCommandBuffersResult != VK_SUCCESS)
							{
								_log->Error("vkAllocateCommandBuffers failed with error code {0} for compute command buffer", vkAllocateCommandBuffersResult);
								return;
							}
							_computeCommandBuffers.push_back(currentCommandBuffer);

							// Start recording the compute command buffer
							vkBeginCommandBufferResult = vkBeginCommandBuffer(currentCommandBuffer, &commandBufferBeginInfo);
							if (vkBeginCommandBufferResult != VK_SUCCESS)
							{
								_log->Error("vkBeginCommandBuffer failed with error code {0} for compute command buffer", vkBeginCommandBufferResult);
							}

							// Flag that we're now inside a compute command buffer
							insideComputeCommandBuffer = true;
							submittedFirstDrawCommandBuffer = true;
						}
						else if (!_separateComputeQueue && !lastCommandWasCompute)
						{
							// If compute and graphics are sharing the same queue, we don't have the implicit ordering
							// guarantees provided by our use of vkQueueSubmit with wait and signal semaphores. We
							// therefore need to set up an in-queue execution barrier between our compute and graphics
							// work, to ensure resources modified during graphics shaders are visible to the compute
							// shaders. We provide that barrier here if this is the point at which we're transitioning
							// between graphics and compute work.
							VkMemoryBarrier memoryBarrier = {};
							memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
							memoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
							memoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
							vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
						}
					}
					else if (insideComputeCommandBuffer)
					{
						// Submit the current compute command buffer
						vkEndCommandBufferResult = vkEndCommandBuffer(currentCommandBuffer);
						if (vkEndCommandBufferResult != VK_SUCCESS)
						{
							_log->Error("vkEndCommandBuffer failed with error code {0} for compute command buffer", vkEndCommandBufferResult);
						}
						VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
						VkSubmitInfo drawSubmitInfo = {};
						drawSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
						drawSubmitInfo.commandBufferCount = 1;
						drawSubmitInfo.pCommandBuffers = &currentCommandBuffer;
						drawSubmitInfo.waitSemaphoreCount = 1;
						drawSubmitInfo.pWaitSemaphores = &_computeStartSemaphore;
						drawSubmitInfo.pWaitDstStageMask = &waitStageMask;
						drawSubmitInfo.signalSemaphoreCount = 1;
						drawSubmitInfo.pSignalSemaphores = &_computeEndSemaphore;
						std::unique_lock<std::mutex> queueLock5(_queueMutex);
						vkQueueSubmitResult = vkQueueSubmit(_computeQueue, 1, &drawSubmitInfo, VK_NULL_HANDLE);
						if (vkQueueSubmitResult != VK_SUCCESS)
						{
							_log->Error("vkQueueSubmit failed with error code {0} for compute command buffer", vkQueueSubmitResult);
						}
						queueLock5.unlock();

						// Allocate a new command buffer for graphics tasks
						commandBufferInfo = {};
						commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
						commandBufferInfo.commandPool = _graphicsCommandPool;
						commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
						commandBufferInfo.commandBufferCount = 1;
						vkAllocateCommandBuffersResult = vkAllocateCommandBuffers(_device, &commandBufferInfo, &currentCommandBuffer);
						if (vkAllocateCommandBuffersResult != VK_SUCCESS)
						{
							_log->Error("vkAllocateCommandBuffers failed with error code {0} for draw command buffer", vkAllocateCommandBuffersResult);
							return;
						}
						_commandBuffers.push_back(currentCommandBuffer);

						// Start recording the graphics command buffer
						vkBeginCommandBufferResult = vkBeginCommandBuffer(currentCommandBuffer, &commandBufferBeginInfo);
						if (vkBeginCommandBufferResult != VK_SUCCESS)
						{
							_log->Error("vkBeginCommandBuffer failed with error code {0} for draw command buffer", vkBeginCommandBufferResult);
						}

						// Flag that we're no longer inside a compute command buffer
						insideComputeCommandBuffer = false;
					}

					// If this renderable is performing an indirect draw operation, and the counter for the draw
					// operation is coming from a GPU buffer, stall the graphics pipeline and wait for all work to
					// complete. This is inefficient, but to support this counter being written to by a compute
					// operation (or potentially even a regular draw), which is the main reason you'd be storing it in a
					// buffer on the GPU in the first place, we need to assume it has changed since it was last written
					// from the CPU side, so we can't just cache the last write. The counter could have just been
					// updated by a previous operation in this same frame, so we need to synchronize with drawing, map
					// the buffer, and read the true, live value. We perform synchronization here, so the draw operation
					// is free to map and read the buffer for this fallback behaviour.
					if (_indirectDrawCountFromBufferFallbackActive)
					{
						// Check if any renderable nodes in this group node need to use a draw count from a buffer
						bool needsCommandQueueSubmit = false;
						for (VulkanRenderableNode* renderableNode : renderableNodes)
						{
							if (renderableNode->UsesDrawCountFromBuffer())
							{
								needsCommandQueueSubmit = true;
								break;
							}
						}

						// If the draw count from buffer feature is actually used, perform synchronization.
						if (needsCommandQueueSubmit)
						{
							// Submit the current draw command buffer
							vkEndCommandBufferResult = vkEndCommandBuffer(currentCommandBuffer);
							if (vkEndCommandBufferResult != VK_SUCCESS)
							{
								_log->Error("vkEndCommandBuffer failed with error code {0} for draw command buffer {1}", vkEndCommandBufferResult, _currentCommandBufferIndex);
							}
							uint32_t waitSemaphoreCount = (!submittedFirstDrawCommandBuffer ? (uint32_t)_drawWaitSemaphores.size() : 1);
							_waitStageMaskBuffer.assign(waitSemaphoreCount, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
							VkSubmitInfo drawSubmitInfo = {};
							drawSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
							drawSubmitInfo.commandBufferCount = 1;
							drawSubmitInfo.pCommandBuffers = &currentCommandBuffer;
							drawSubmitInfo.waitSemaphoreCount = waitSemaphoreCount;
							drawSubmitInfo.pWaitSemaphores = (!submittedFirstDrawCommandBuffer ? _drawWaitSemaphores.data() : &_computeEndSemaphore);
							drawSubmitInfo.pWaitDstStageMask = _waitStageMaskBuffer.data();
							drawSubmitInfo.signalSemaphoreCount = 1;
							drawSubmitInfo.pSignalSemaphores = &_computeStartSemaphore;
							std::unique_lock<std::mutex> queueLock6(_queueMutex);
							vkQueueSubmitResult = vkQueueSubmit(_graphicsQueue, 1, &drawSubmitInfo, VK_NULL_HANDLE);
							if (vkQueueSubmitResult != VK_SUCCESS)
							{
								_log->Error("vkQueueSubmit failed with error code {0} for draw command buffer {1}", vkQueueSubmitResult, _currentCommandBufferIndex);
							}
							queueLock6.unlock();

							// Since we can't use the same semaphore in pSignalSemaphores and pWaitSemaphores, even if
							// we know it starts signalled or will be signalled by prior command buffers, we submit an
							// explicit step here to ensure _computeEndSemaphore gets signalled after the previous
							// command buffer completes.
							VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
							drawSubmitInfo = {};
							drawSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
							drawSubmitInfo.commandBufferCount = 0;
							drawSubmitInfo.pCommandBuffers = nullptr;
							drawSubmitInfo.waitSemaphoreCount = 1;
							drawSubmitInfo.pWaitSemaphores = &_computeStartSemaphore;
							drawSubmitInfo.pWaitDstStageMask = &waitStageMask;
							drawSubmitInfo.signalSemaphoreCount = 1;
							drawSubmitInfo.pSignalSemaphores = &_computeEndSemaphore;
							std::unique_lock<std::mutex> queueLock7(_queueMutex);
							vkQueueSubmitResult = vkQueueSubmit(_graphicsQueue, 1, &drawSubmitInfo, VK_NULL_HANDLE);
							if (vkQueueSubmitResult != VK_SUCCESS)
							{
								_log->Error("vkQueueSubmit failed with error code {0} for fence synchronization", vkQueueSubmitResult);
							}
							queueLock7.unlock();

							// Wait for the graphics queue to be idle, so we know the previous command buffers are fully
							// processed. There's no need to wait on the compute queue here, as the graphics command
							// buffer is defined to wait on a semaphore raised by compute tasks before proceeding.
							vkQueueWaitIdle(_graphicsQueue);

							// Allocate a new command buffer for graphics tasks
							commandBufferInfo = {};
							commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
							commandBufferInfo.commandPool = _graphicsCommandPool;
							commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
							commandBufferInfo.commandBufferCount = 1;
							vkAllocateCommandBuffersResult = vkAllocateCommandBuffers(_device, &commandBufferInfo, &currentCommandBuffer);
							if (vkAllocateCommandBuffersResult != VK_SUCCESS)
							{
								_log->Error("vkAllocateCommandBuffers failed with error code {0} for draw command buffer", vkAllocateCommandBuffersResult);
								return;
							}
							_commandBuffers.push_back(currentCommandBuffer);

							// Start recording the graphics command buffer
							vkBeginCommandBufferResult = vkBeginCommandBuffer(currentCommandBuffer, &commandBufferBeginInfo);
							if (vkBeginCommandBufferResult != VK_SUCCESS)
							{
								_log->Error("vkBeginCommandBuffer failed with error code {0} for draw command buffer", vkBeginCommandBufferResult);
							}

							// Flag that we've submitted our first draw command buffer, so the correct synchronization
							// primitives are used later.
							submittedFirstDrawCommandBuffer = true;
						}
					}

					// If we haven't begun the render pass yet, do it now. We deliberately defer this task, because it's
					// not possible to submit a command buffer during a render pass, and we may need to submit graphics
					// command buffers in the code above for synchronization purposes.
					if (!renderPassBegun)
					{
						renderPassNode->BeginRenderPass(currentCommandBuffer);
						renderPassBegun = true;
					}

					// Apply fixed state associated with this state group node
					groupNode->ApplyFixedState(stateBucketID, childNodeEntry.frameBufferIndex, currentCommandBuffer, shaderProgram, currentFramebuffer);

					// Flag whether the command we're about to execute next is a compute command
					lastCommandWasCompute = isComputeTask;

					// If this group node is performing a compute task, execute it now.
					if (isComputeTask)
					{
						// Attach the required global constant buffer entries
						size_t newGlobalDescriptorSetIndex = groupNode->GetGlobalConstantBufferBindingInfo(constantBufferStateIndex).descriptorIndex;
						if (currentGlobalDescriptorSetIndex != newGlobalDescriptorSetIndex)
						{
							currentGlobalDescriptorSetIndex = newGlobalDescriptorSetIndex;

							shaderProgram->ApplyGlobalConstantBufferBindings(currentCommandBuffer, groupNode->GetGlobalConstantBufferBindingInfo(constantBufferStateIndex), groupNode->GetPipelineLayout(stateBucketID, 0));
						}

						// Bind relevant descriptor set
						size_t newDescriptorSetIndex = groupNode->GetDescriptorSetIndex(childNodeEntry.defaultStateIndex);
						if (currentDescriptorSetIndex != newDescriptorSetIndex)
						{
							currentDescriptorSetIndex = newDescriptorSetIndex;
							if (currentDescriptorSetIndex != std::numeric_limits<size_t>::max())
							{
								shaderProgram->BindDescriptorSet(currentCommandBuffer, currentDescriptorSetIndex, groupNode->GetPipelineLayout(stateBucketID, childNodeEntry.frameBufferIndex));
							}
						}

						// Perform the compute operation
						auto threadGroupCounts = groupNode->GetComputeThreadGroupCounts();
						vkCmdDispatch(currentCommandBuffer, threadGroupCounts.X(), threadGroupCounts.Y(), threadGroupCounts.Z());
					}

					// Render each child node
					for (VulkanRenderableNode* renderableNode : renderableNodes)
					{
						// Push a render marker if required
						if (_useRenderMarkers)
						{
							renderMarkerLabel.pLabelName = renderableNode->DebugName().c_str();
							ConvertColorToFloat(RenderableMarkerColor(), &renderMarkerLabel.color[0]);
							_pfnCmdInsertDebugUtilsLabelEXT(currentCommandBuffer, &renderMarkerLabel);
						}

						// Attach the required global constant buffer entries
						size_t newGlobalDescriptorSetIndex = renderableNode->GetGlobalConstantBufferBindingInfo(constantBufferStateIndex).descriptorIndex;
						if (currentGlobalDescriptorSetIndex != newGlobalDescriptorSetIndex)
						{
							currentGlobalDescriptorSetIndex = newGlobalDescriptorSetIndex;

							shaderProgram->ApplyGlobalConstantBufferBindings(currentCommandBuffer, renderableNode->GetGlobalConstantBufferBindingInfo(constantBufferStateIndex), groupNode->GetPipelineLayout(stateBucketID, childNodeEntry.frameBufferIndex));
						}

						// Bind relevant descriptor set
						size_t newDescriptorSetIndex = renderableNode->GetDescriptorSetIndex(childNodeEntry.defaultStateIndex);
						if (currentDescriptorSetIndex != newDescriptorSetIndex)
						{
							currentDescriptorSetIndex = newDescriptorSetIndex;
							shaderProgram->BindDescriptorSet(currentCommandBuffer, currentDescriptorSetIndex, groupNode->GetPipelineLayout(stateBucketID, childNodeEntry.frameBufferIndex));
						}

						// Perform the draw operation
						renderableNode->Draw(currentCommandBuffer, shaderProgram);
					}

					// Pop a render marker if required
					if (_useRenderMarkers && !isComputeTask)
					{
						_pfnCmdEndDebugUtilsLabelEXT(currentCommandBuffer);
					}
				}

				// Pop a render marker if required
				if (_useRenderMarkers && markedGroup)
				{
					_pfnCmdEndDebugUtilsLabelEXT(currentCommandBuffer);
				}
			}

			// Pop a render marker if required
			if (_useRenderMarkers && markedProgram)
			{
				_pfnCmdEndDebugUtilsLabelEXT(currentCommandBuffer);
			}
		}

		// Perform any unbind operations for the render pass node. Note that we need to begin the render pass first and
		// end it immediately even if it was empty, so operations like doing buffer clear operations still take effect.
		// It's entirely possible subsequent render passes or capture operations will rely on this.
		if (!renderPassBegun)
		{
			renderPassNode->BeginRenderPass(currentCommandBuffer);
		}
		renderPassNode->EndRenderPass(currentCommandBuffer);

		// Pop a render marker if required
		if (_useRenderMarkers)
		{
			_pfnCmdEndDebugUtilsLabelEXT(currentCommandBuffer);
		}
	}

	// If we're inside a compute command buffer, submit it now.
	if (insideComputeCommandBuffer)
	{
		// Submit the current compute command buffer
		vkEndCommandBufferResult = vkEndCommandBuffer(currentCommandBuffer);
		if (vkEndCommandBufferResult != VK_SUCCESS)
		{
			_log->Error("vkEndCommandBuffer failed with error code {0} for compute command buffer", vkEndCommandBufferResult);
		}
		VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		VkSubmitInfo drawSubmitInfo = {};
		drawSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		drawSubmitInfo.commandBufferCount = 1;
		drawSubmitInfo.pCommandBuffers = &currentCommandBuffer;
		drawSubmitInfo.waitSemaphoreCount = 1;
		drawSubmitInfo.pWaitSemaphores = &_computeStartSemaphore;
		drawSubmitInfo.pWaitDstStageMask = &waitStageMask;
		drawSubmitInfo.signalSemaphoreCount = 1;
		drawSubmitInfo.pSignalSemaphores = &_computeEndSemaphore;
		std::unique_lock<std::mutex> queueLock5(_queueMutex);
		vkQueueSubmitResult = vkQueueSubmit(_computeQueue, 1, &drawSubmitInfo, VK_NULL_HANDLE);
		if (vkQueueSubmitResult != VK_SUCCESS)
		{
			_log->Error("vkQueueSubmit failed with error code {0} for compute command buffer", vkQueueSubmitResult);
		}
		queueLock5.unlock();

		// Allocate a new command buffer for graphics tasks
		commandBufferInfo = {};
		commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferInfo.commandPool = _graphicsCommandPool;
		commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		commandBufferInfo.commandBufferCount = 1;
		vkAllocateCommandBuffersResult = vkAllocateCommandBuffers(_device, &commandBufferInfo, &currentCommandBuffer);
		if (vkAllocateCommandBuffersResult != VK_SUCCESS)
		{
			_log->Error("vkAllocateCommandBuffers failed with error code {0} for draw command buffer", vkAllocateCommandBuffersResult);
			return;
		}
		_commandBuffers.push_back(currentCommandBuffer);

		// Start recording the graphics command buffer
		vkBeginCommandBufferResult = vkBeginCommandBuffer(currentCommandBuffer, &commandBufferBeginInfo);
		if (vkBeginCommandBufferResult != VK_SUCCESS)
		{
			_log->Error("vkBeginCommandBuffer failed with error code {0} for draw command buffer", vkBeginCommandBufferResult);
		}
	}

	// Submit draw command buffer
	vkEndCommandBufferResult = vkEndCommandBuffer(currentCommandBuffer);
	if (vkEndCommandBufferResult != VK_SUCCESS)
	{
		_log->Error("vkEndCommandBuffer failed with error code {0} for draw command buffer {1}", vkEndCommandBufferResult, _currentCommandBufferIndex);
	}
	uint32_t waitSemaphoreCount = (!submittedFirstDrawCommandBuffer ? (uint32_t)_drawWaitSemaphores.size() : 1);
	VkPipelineStageFlags computeWaitStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	VkSubmitInfo drawSubmitInfo = {};
	drawSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	drawSubmitInfo.commandBufferCount = 1;
	drawSubmitInfo.pCommandBuffers = &currentCommandBuffer;
	drawSubmitInfo.waitSemaphoreCount = waitSemaphoreCount;
	drawSubmitInfo.pWaitSemaphores = (!submittedFirstDrawCommandBuffer ? _drawWaitSemaphores.data() : &_computeEndSemaphore);
	drawSubmitInfo.pWaitDstStageMask = (!submittedFirstDrawCommandBuffer ? _drawWaitStages.data() : &computeWaitStageMask);
	drawSubmitInfo.signalSemaphoreCount = (_captureTargetsPresent ? 1 : (uint32_t)_presentSemaphores.size());
	drawSubmitInfo.pSignalSemaphores = (_captureTargetsPresent ? &_drawCompleteSemaphore : _presentSemaphores.data());
	std::unique_lock<std::mutex> queueLock2(_queueMutex);
	vkQueueSubmitResult = vkQueueSubmit(_graphicsQueue, 1, &drawSubmitInfo, (!_captureTargetsPresent ? _drawFence : VK_NULL_HANDLE));
	if (vkQueueSubmitResult != VK_SUCCESS)
	{
		_log->Error("vkQueueSubmit failed with error code {0} for draw command buffer {1}", vkQueueSubmitResult, _currentCommandBufferIndex);
	}
	queueLock2.unlock();

	// If we have at least one capture target, begin the capture process now.
	if (_captureTargetsPresent)
	{
		// Allocate a new command buffer for the capture process
		commandBufferInfo = {};
		commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferInfo.commandPool = _graphicsCommandPool;
		commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		commandBufferInfo.commandBufferCount = 1;
		vkAllocateCommandBuffersResult = vkAllocateCommandBuffers(_device, &commandBufferInfo, &currentCommandBuffer);
		if (vkAllocateCommandBuffersResult != VK_SUCCESS)
		{
			_log->Error("vkAllocateCommandBuffers failed with error code {0} for framebuffer capture process", vkAllocateCommandBuffersResult);
			return;
		}
		_commandBuffers.push_back(currentCommandBuffer);

		// Start recording second draw command buffer
		vkBeginCommandBufferResult = vkBeginCommandBuffer(currentCommandBuffer, &commandBufferBeginInfo);
		if (vkBeginCommandBufferResult != VK_SUCCESS)
		{
			_log->Error("vkBeginCommandBuffer failed with error code {0} for framebuffer capture process", vkBeginCommandBufferResult);
		}

		// Capture all requested buffer outputs
		for (VulkanFrameBuffer* frameBuffer : _boundWindowFramebuffers)
		{
			frameBuffer->CaptureFrameBufferOutput(currentCommandBuffer);
		}
		for (VulkanFrameBuffer* frameBuffer : _boundTextureFramebuffers)
		{
			frameBuffer->CaptureFrameBufferOutput(currentCommandBuffer);
		}
		for (VulkanDataArray* resourceBuffer : _boundDataArrays)
		{
			resourceBuffer->CaptureDataBufferOutput(currentCommandBuffer);
		}
		for (VulkanTexelArray* resourceBuffer : _boundTexelArrays)
		{
			resourceBuffer->CaptureDataBufferOutput(currentCommandBuffer);
		}

		// End the record process for the command buffer
		vkEndCommandBufferResult = vkEndCommandBuffer(currentCommandBuffer);
		if (vkEndCommandBufferResult != VK_SUCCESS)
		{
			_log->Error("vkEndCommandBuffer failed with error code {0} for framebuffer capture process", vkEndCommandBufferResult);
			return;
		}

		// Submit the command buffer for execution. Framebuffer captures begin with image layout transitions from their
		// render-pass layouts before the transfer reads are recorded, so wait the whole capture command buffer on the
		// draw submission rather than only waiting at the transfer stage.
		VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &currentCommandBuffer;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &_drawCompleteSemaphore;
		submitInfo.pWaitDstStageMask = &waitStageMask;
		submitInfo.signalSemaphoreCount = (uint32_t)_presentSemaphores.size();
		submitInfo.pSignalSemaphores = _presentSemaphores.data();
		std::unique_lock<std::mutex> queueLock3(_queueMutex);
		vkQueueSubmitResult = vkQueueSubmit(_graphicsQueue, 1, &submitInfo, _drawFence);
		queueLock3.unlock();
		if (vkQueueSubmitResult != VK_SUCCESS)
		{
			_log->Error("vkQueueSubmit failed with error code {0} for framebuffer capture process", vkQueueSubmitResult);
			return;
		}
	}
}

//----------------------------------------------------------------------------------------
void VulkanRenderer::PerformSwapOperation()
{
	// Swap the window buffers to the screen when draw completes
	for (VulkanFrameBuffer* frameBuffer : _boundWindowFramebuffers)
	{
		frameBuffer->PresentToWindow();
	}

	// Wait for draw to complete. Note that this will also wait for capture transfer to complete if they are present,
	// making buffers available for reading on the CPU.
	VkResult vkWaitForFencesResult = vkWaitForFences(_device, 1, &_drawFence, VK_TRUE, std::numeric_limits<uint64_t>::max());
	if (vkWaitForFencesResult != VK_SUCCESS)
	{
		_log->Error("vkWaitForFences failed with error code {0} for PerformSwapOperation", vkWaitForFencesResult);
	}
	VkResult vkResetFencesResult = vkResetFences(_device, 1, &_drawFence);
	if (vkResetFencesResult != VK_SUCCESS)
	{
		_log->Error("vkResetFences failed with error code {0} for PerformSwapOperation", vkResetFencesResult);
	}

	// Submit any delayed transfer commands for execution
	std::unique_lock<std::mutex> transferLock(_transferPoolMutex);
	ExecutePendingDelayedTransferCommands();
	transferLock.unlock();

	// If we have at least one capture target, complete the capture process.
	_capturedFramebufferOutputsInCurrentFrame.clear();
	_capturedDataArrayOutputsInCurrentFrame.clear();
	_capturedTexelArrayOutputsInCurrentFrame.clear();
	if (_captureTargetsPresent)
	{
		for (VulkanFrameBuffer* frameBuffer : _boundWindowFramebuffers)
		{
			frameBuffer->CompleteCaptureFrameBufferOutput();
		}
		for (VulkanFrameBuffer* frameBuffer : _boundTextureFramebuffers)
		{
			frameBuffer->CompleteCaptureFrameBufferOutput();
		}
		for (VulkanDataArray* resourceBuffer : _boundDataArrays)
		{
			resourceBuffer->CompleteCaptureDataBufferOutput();
		}
		for (VulkanTexelArray* resourceBuffer : _boundTexelArrays)
		{
			resourceBuffer->CompleteCaptureDataBufferOutput();
		}
	}

	// Get next swapchain images for any bound window framebuffers
	for (VulkanFrameBuffer* frameBuffer : _boundWindowFramebuffers)
	{
		frameBuffer->AcquireNextImage();
	}
}

//----------------------------------------------------------------------------------------
void VulkanRenderer::ExecutePendingDelayedTransferCommands()
{
	// Obtain and reset the current set of graphics queue release and batch transfer operations
	std::vector<PendingGraphicsUseObjectTypes> pendingGraphicsQueueReleaseOperations;
	std::vector<DelayedTransferEntry> delayedTransferCommandBuffers;
	{
		std::unique_lock<std::mutex> batchTransferLock(_pendingGraphicsQueueTransferMutex);
		pendingGraphicsQueueReleaseOperations = std::move(_pendingGraphicsQueueReleaseOperations);
		delayedTransferCommandBuffers = std::move(_delayedTransferCommandBuffers);
		_pendingGraphicsQueueReleaseOperations.clear();
		_delayedTransferCommandBuffers.clear();
	}

	// If there are pending graphics queue release operations, perform them now.
	bool waitOnReleaseOperationsPending = (!delayedTransferCommandBuffers.empty() && !pendingGraphicsQueueReleaseOperations.empty());
	if (!pendingGraphicsQueueReleaseOperations.empty())
	{
		// Ensure there are enough semaphores to signal completion of the graphics queue release operations for each
		// pending batch transfer
		size_t existingSemaphoreCount = _graphicsQueueReleaseCompleteSemaphores.size();
		size_t requiredSemaphoreCount = delayedTransferCommandBuffers.size();
		if (existingSemaphoreCount < requiredSemaphoreCount)
		{
			_graphicsQueueReleaseCompleteSemaphores.resize(requiredSemaphoreCount);
			for (size_t i = existingSemaphoreCount; i < requiredSemaphoreCount; ++i)
			{
				VkSemaphoreCreateInfo semaphoreInfo = {};
				semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
				semaphoreInfo.flags = 0;
				VkResult createGraphicsQueueReleaseSemaphoreResult = vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &_graphicsQueueReleaseCompleteSemaphores[i]);
				if (createGraphicsQueueReleaseSemaphoreResult != VK_SUCCESS)
				{
					_log.get()->Error("Could not create semaphore in ExecutePendingDelayedTransferCommands with error code {0}", createGraphicsQueueReleaseSemaphoreResult);
					return;
				}
			}
		}

		// If there was already a command buffer created for queue release operations, free it now.
		if (_graphicsQueueReleaseCommandBuffer != VK_NULL_HANDLE)
		{
			vkFreeCommandBuffers(_device, _graphicsCommandPool, 1, &_graphicsQueueReleaseCommandBuffer);
		}

		// Create a command buffer for our queue release operations
		VkCommandBufferAllocateInfo commandBufferInfo = {};
		commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		commandBufferInfo.commandPool = _graphicsCommandPool;
		commandBufferInfo.commandBufferCount = 1;
		VkResult vkAllocateCommandBuffersResult = vkAllocateCommandBuffers(_device, &commandBufferInfo, &_graphicsQueueReleaseCommandBuffer);
		if (vkAllocateCommandBuffersResult != VK_SUCCESS)
		{
			_log->Error("vkAllocateCommandBuffers failed with error code {0} for ExecutePendingDelayedTransferCommands", vkAllocateCommandBuffersResult);
			return;
		}
		VkCommandBuffer commandBuffer = _graphicsQueueReleaseCommandBuffer;

		// Start recording the command buffer
		VkCommandBufferBeginInfo commandBufferBeginInfo = {};
		commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		VkResult vkBeginCommandBufferResult = vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
		if (vkBeginCommandBufferResult != VK_SUCCESS)
		{
			_log->Error("vkBeginCommandBuffer failed with error code {0} for ExecutePendingDelayedTransferCommands", vkBeginCommandBufferResult);
		}

		// Perform each release operation
		for (const auto& entry : pendingGraphicsQueueReleaseOperations)
		{
			std::visit([commandBuffer](auto* object) { object->PerformGraphicsQueueReleaseOperation(commandBuffer); }, entry);
		}

		// Complete the command buffer recording process
		VkResult vkEndCommandBufferResult = vkEndCommandBuffer(commandBuffer);
		if (vkEndCommandBufferResult != VK_SUCCESS)
		{
			_log->Error("vkEndCommandBuffer failed with error code {0} for ExecutePendingDelayedTransferCommands", vkEndCommandBufferResult);
		}

		// Submit our command buffer
		VkSubmitInfo drawPrepSubmitInfo = {};
		drawPrepSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		drawPrepSubmitInfo.commandBufferCount = 1;
		drawPrepSubmitInfo.waitSemaphoreCount = 0;
		drawPrepSubmitInfo.pWaitSemaphores = nullptr;
		drawPrepSubmitInfo.pWaitDstStageMask = nullptr;
		if (waitOnReleaseOperationsPending)
		{
			drawPrepSubmitInfo.signalSemaphoreCount = (uint32_t)requiredSemaphoreCount;
			drawPrepSubmitInfo.pSignalSemaphores = _graphicsQueueReleaseCompleteSemaphores.data();
		}
		drawPrepSubmitInfo.pCommandBuffers = &commandBuffer;
		std::unique_lock<std::mutex> queueLock(_queueMutex);
		VkResult vkQueueSubmitResult = vkQueueSubmit(_graphicsQueue, 1, &drawPrepSubmitInfo, VK_NULL_HANDLE);
		if (vkQueueSubmitResult != VK_SUCCESS)
		{
			_log->Error("vkQueueSubmit failed with error code {0} for ExecutePendingDelayedTransferCommands", vkQueueSubmitResult);
		}
		queueLock.unlock();
	}

	// Submit our build command buffers which require no fence
	if (!_delayedTransferCommandBuffersNoFence.empty())
	{
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = (uint32_t)_delayedTransferCommandBuffersNoFence.size();
		submitInfo.pCommandBuffers = _delayedTransferCommandBuffersNoFence.data();
		std::unique_lock<std::mutex> queueLock(_queueMutex);
		VkResult vkQueueSubmitResult = vkQueueSubmit(_transferQueue, 1, &submitInfo, VK_NULL_HANDLE);
		queueLock.unlock();
		if (vkQueueSubmitResult != VK_SUCCESS)
		{
			_log->Error("vkQueueSubmit failed with error code {0} for delayed bulk transfer command", vkQueueSubmitResult);
		}
		_delayedTransferCommandBuffersNoFence.clear();
	}

	// Submit our batch transfer command buffers, which will raise completion fences for their parent batch transfer
	// objects as they each complete. Note that all of these submissions need to wait on any the graphics queue release
	// operations we triggered above.
	if (!delayedTransferCommandBuffers.empty())
	{
		for (size_t i = 0; i < delayedTransferCommandBuffers.size(); ++i)
		{
			// Build the submission entry for this batch transfer
			const auto& entry = delayedTransferCommandBuffers[i];
			VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &entry.commandBuffer;
			if (waitOnReleaseOperationsPending)
			{
				submitInfo.waitSemaphoreCount = 1;
				submitInfo.pWaitSemaphores = &_graphicsQueueReleaseCompleteSemaphores[i];
				submitInfo.pWaitDstStageMask = &waitStageMask;
			}

			// Submit the batch transfer
			std::unique_lock<std::mutex> queueLock(_queueMutex);
			VkResult vkQueueSubmitResult = vkQueueSubmit(entry.targetQueue, 1, &submitInfo, entry.completionFence);
			queueLock.unlock();
			if (vkQueueSubmitResult != VK_SUCCESS)
			{
				_log->Error("vkQueueSubmit failed with error code {0} for delayed individual transfer command", vkQueueSubmitResult);
			}

			// Notify any waiting threads that submission is now complete. Note that we need this, as VkFence objects
			// are not thread safe. It is illegal to call vkQueueSubmit with a fence on one thread, while another thread
			// is waiting on that fence with vkWaitForFences. We need to pre-synchronize with the submission process
			// here, the hand over the wait operation to the other thread. It is legal for more than one thread to call
			// vkWaitForFences one the same fence once already submitted however, so no further synchronization between
			// threads is required after submission.
			if (entry.submitCompleteToken != nullptr)
			{
				std::scoped_lock<std::mutex> lock(entry.submitCompleteToken->mutex);
				entry.submitCompleteToken->complete = true;
				entry.submitCompleteToken->notifyOnComplete.notify_all();
			}
		}
	}
}

//----------------------------------------------------------------------------------------
void VulkanRenderer::PerformDeleteLastDrawResourcesOperation()
{
	// Delete all pending objects to be deleted
	for (const auto& entry : _state[_drawIndex].deletePendingObjects)
	{
		std::visit([this](auto* object) {
			CancelPendingGraphicsQueueAcquireOperation(object);
			delete object;
		},
		           entry);
	}
	_state[_drawIndex].deletePendingObjects.clear();
}

//----------------------------------------------------------------------------------------
void VulkanRenderer::PerformDeleteNextDrawResourcesOperation()
{
	// If there are any window framebuffers bound, synchronize with their pending calls to vkAcquireNextImageKHR.
	if (!_boundWindowFramebuffers.empty())
	{
		// Wait for all the window framebuffer prepare semaphores to be signalled, and trigger a fence when complete.
		// This will allow us to synchronize with vkAcquireNextImageKHR. Note that we reuse the draw fence here, as it's
		// guaranteed to be idle and reset already, to avoid creating an additional fence unnecessarily.
		_waitStageMaskBuffer.assign(_boundWindowFramebuffers.size(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
		std::vector<VkSemaphore> semaphores(_boundWindowFramebuffers.size());
		for (size_t i = 0; i < _boundWindowFramebuffers.size(); ++i)
		{
			semaphores[i] = _boundWindowFramebuffers[i]->GetPrepareSemaphore();
		}
		VkSubmitInfo drawSubmitInfo = {};
		drawSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		drawSubmitInfo.commandBufferCount = 0;
		drawSubmitInfo.pCommandBuffers = nullptr;
		drawSubmitInfo.waitSemaphoreCount = (uint32_t)semaphores.size();
		drawSubmitInfo.pWaitSemaphores = semaphores.data();
		drawSubmitInfo.pWaitDstStageMask = _waitStageMaskBuffer.data();
		drawSubmitInfo.signalSemaphoreCount = 0;
		drawSubmitInfo.pSignalSemaphores = nullptr;
		std::unique_lock<std::mutex> queueLock(_queueMutex);
		VkResult vkQueueSubmitResult = vkQueueSubmit(_graphicsQueue, 1, &drawSubmitInfo, _drawFence);
		if (vkQueueSubmitResult != VK_SUCCESS)
		{
			_log->Error("vkQueueSubmit failed with error code {0} during cleanup", vkQueueSubmitResult);
		}
		queueLock.unlock();

		// Wait for the draw fence to be signalled, then reset the fence to leave it clear for the next draw operation.
		VkResult vkWaitForFencesResult = vkWaitForFences(_device, 1, &_drawFence, VK_TRUE, std::numeric_limits<uint64_t>::max());
		if (vkWaitForFencesResult != VK_SUCCESS)
		{
			_log->Error("vkWaitForFences failed with error code {0} during cleanup", vkWaitForFencesResult);
		}
		VkResult vkResetFencesResult = vkResetFences(_device, 1, &_drawFence);
		if (vkResetFencesResult != VK_SUCCESS)
		{
			_log->Error("vkResetFences failed with error code {0} during cleanup", vkResetFencesResult);
		}
	}

	// Submit any delayed transfer commands for execution. We need to do this here, as it's possible a transfer was
	// pending to a buffer which subsequently became eligible for deletion.
	std::unique_lock<std::mutex> transferLock(_transferPoolMutex);
	ExecutePendingDelayedTransferCommands();
	transferLock.unlock();

	// Wait for the graphics device to reach an idle state
	vkDeviceWaitIdle(_device);

	// Since we're advancing this delete step, strip the objects being deleted out of any pending update operations
	// which would normally be run first.
	std::unique_lock<std::mutex> lock(_buildStateMutex);
	std::unordered_set<void*> deleteSet;
	for (const auto& entry : _state[_buildIndex].deletePendingObjects)
	{
		deleteSet.insert(std::visit([](auto* object) { return static_cast<void*>(object); }, entry));
	}
	_state[_buildIndex].migrateStatePendingObjects.erase(std::remove_if(_state[_buildIndex].migrateStatePendingObjects.begin(), _state[_buildIndex].migrateStatePendingObjects.end(), [&](const auto& v) { return deleteSet.find(std::visit([](auto* p) { return static_cast<void*>(p); }, v)) != deleteSet.end(); }), _state[_buildIndex].migrateStatePendingObjects.end());
	_state[_buildIndex].bufferUpdatePendingObjects.erase(std::remove_if(_state[_buildIndex].bufferUpdatePendingObjects.begin(), _state[_buildIndex].bufferUpdatePendingObjects.end(), [&](const auto& v) { return deleteSet.find(std::visit([](auto* p) { return static_cast<void*>(p); }, v)) != deleteSet.end(); }), _state[_buildIndex].bufferUpdatePendingObjects.end());
	_state[_buildIndex].bufferTransferPendingObjects.erase(std::remove_if(_state[_buildIndex].bufferTransferPendingObjects.begin(), _state[_buildIndex].bufferTransferPendingObjects.end(), [&](const auto& v) { return deleteSet.find(std::visit([](auto* p) { return static_cast<void*>(p); }, v)) != deleteSet.end(); }), _state[_buildIndex].bufferTransferPendingObjects.end());
	_capturedFramebufferOutputsInCurrentFrame.erase(std::remove_if(_capturedFramebufferOutputsInCurrentFrame.begin(), _capturedFramebufferOutputsInCurrentFrame.end(), [&](const auto& v) { return deleteSet.find(v) != deleteSet.end(); }), _capturedFramebufferOutputsInCurrentFrame.end());
	_capturedDataArrayOutputsInCurrentFrame.erase(std::remove_if(_capturedDataArrayOutputsInCurrentFrame.begin(), _capturedDataArrayOutputsInCurrentFrame.end(), [&](const auto& v) { return deleteSet.find(v) != deleteSet.end(); }), _capturedDataArrayOutputsInCurrentFrame.end());
	_capturedTexelArrayOutputsInCurrentFrame.erase(std::remove_if(_capturedTexelArrayOutputsInCurrentFrame.begin(), _capturedTexelArrayOutputsInCurrentFrame.end(), [&](const auto& v) { return deleteSet.find(v) != deleteSet.end(); }), _capturedTexelArrayOutputsInCurrentFrame.end());

	// Delete all pending objects to be deleted
	for (const auto& entry : _state[_buildIndex].deletePendingObjects)
	{
		if (std::holds_alternative<VulkanFrameBuffer*>(entry))
		{
			// Erase this framebuffer from the list of bound window framebuffers, so we don't have dangling references
			// in the set. Without this, it's unsafe to call this function multiple times, and since it runs from the
			// renderer destructor, this will frequently occur.
			auto framebuffer = *std::get_if<VulkanFrameBuffer*>(&entry);
			auto entryInFramebufferSet = std::find(_boundWindowFramebuffers.begin(), _boundWindowFramebuffers.end(), framebuffer);
			if (entryInFramebufferSet != _boundWindowFramebuffers.end())
			{
				_boundWindowFramebuffers.erase(entryInFramebufferSet);
			}
			delete framebuffer;
		}
		else
		{
			std::visit([this](auto* object) {
				CancelPendingGraphicsQueueAcquireOperation(object);
				delete object;
			},
			           entry);
		}
	}
	_state[_buildIndex].deletePendingObjects.clear();
}

//----------------------------------------------------------------------------------------
void VulkanRenderer::BindTextures(const std::vector<ITextureBindingInfo*>& bindingEntries, VulkanShaderProgram* program, size_t setIndex)
{
	for (ITextureBindingInfo* bindingInfo : bindingEntries)
	{
		bindingInfo->BindTexture(this, program, setIndex);
	}
}

//----------------------------------------------------------------------------------------
void VulkanRenderer::BindSamplers(const std::vector<ISamplerBindingInfo*>& bindingEntries, VulkanShaderProgram* program, size_t setIndex)
{
	for (ISamplerBindingInfo* bindingInfo : bindingEntries)
	{
		bindingInfo->BindSampler(this, program, setIndex);
	}
}

//----------------------------------------------------------------------------------------
void VulkanRenderer::BindStateBuffers(const std::vector<StateBufferBindingInfo*>& bindingEntries, VulkanShaderProgram* program, size_t setIndex)
{
	for (StateBufferBindingInfo* bindingInfo : bindingEntries)
	{
		bindingInfo->BindStateBuffer(this, program, setIndex);
	}
}

//----------------------------------------------------------------------------------------
void VulkanRenderer::BindResourceArrays(const std::vector<ResourceArrayBindingInfo*>& bindingEntries, VulkanShaderProgram* program, size_t setIndex, VkCommandBuffer commandBuffer, bool performReset)
{
	for (ResourceArrayBindingInfo* bindingInfo : bindingEntries)
	{
		bindingInfo->BindResourceArray(this, program, setIndex, commandBuffer, performReset);
		_captureTargetsPresent |= bindingInfo->HasCaptureTargets();
	}
}

//----------------------------------------------------------------------------------------
// Instance and device methods
//----------------------------------------------------------------------------------------
bool VulkanRenderer::NullDescriptorFeatureMissingOrBroken() const
{
	return _nullDescriptorFeatureMissingOrBroken;
}

//----------------------------------------------------------------------------------------
VkInstance VulkanRenderer::GetInstance() const
{
	return _instanceData->instance;
}

//----------------------------------------------------------------------------------------
VkPhysicalDevice VulkanRenderer::GetPhysicalDevice() const
{
	return _physicalDevice;
}

//----------------------------------------------------------------------------------------
const VkPhysicalDeviceProperties& VulkanRenderer::GetPhysicalDeviceProperties() const
{
	return _physicalDeviceProperties;
}

//----------------------------------------------------------------------------------------
VkDevice VulkanRenderer::GetDevice() const
{
	return _device;
}

//----------------------------------------------------------------------------------------
const VulkanRenderer::ExtensionInfo& VulkanRenderer::GetExtensionInfo() const
{
	return _extensionInfo;
}

//----------------------------------------------------------------------------------------
uint32_t VulkanRenderer::GetMinVertexElementStride() const
{
	return _minVertexElementStride;
}

//----------------------------------------------------------------------------------------
bool VulkanRenderer::PrimitiveRestartSupported() const
{
	return _primitiveRestartSupported;
}

//----------------------------------------------------------------------------------------
// Queue methods
//----------------------------------------------------------------------------------------
uint32_t VulkanRenderer::GetTransferQueueFamily() const
{
	return _transferQueueFamilyIndex;
}

//----------------------------------------------------------------------------------------
uint32_t VulkanRenderer::GetBatchTransferQueueFamily() const
{
	return _batchTransferQueueFamilyIndex;
}

//----------------------------------------------------------------------------------------
uint32_t VulkanRenderer::GetGraphicsQueueFamily() const
{
	return _graphicsQueueFamilyIndex;
}

//----------------------------------------------------------------------------------------
uint32_t VulkanRenderer::GetPresentQueueFamily() const
{
	return _presentQueueFamilyIndex;
}

//----------------------------------------------------------------------------------------
VkQueue VulkanRenderer::GetGraphicsQueue() const
{
	return _graphicsQueue;
}

//----------------------------------------------------------------------------------------
VkQueue VulkanRenderer::GetPresentQueue() const
{
	return _presentQueue;
}

//----------------------------------------------------------------------------------------
VkCommandBuffer VulkanRenderer::GetBuildCommandBuffer()
{
	VkCommandPool commandPool;
	VkQueue targetQueue;
	return GetBatchCommandBuffer(ITransferBatch::EndTiming::BeforeNextFrame, commandPool, targetQueue);
}

//----------------------------------------------------------------------------------------
VkCommandBuffer VulkanRenderer::GetBatchCommandBuffer(ITransferBatch::EndTiming endTiming, VkCommandPool& commandPool, VkQueue& targetQueue)
{
	// Select the right command pool to allocate from, based on the end timing of the request.
	bool useBatchTransfer = (endTiming == ITransferBatch::EndTiming::AnyFrame);
	targetQueue = (useBatchTransfer ? _batchTransferQueue : _transferQueue);
	commandPool = (useBatchTransfer ? _batchTransferCommandPool : _transferCommandPool);

	// Allocate the command buffer from the shared transfer pool. The deferred batch path only reaches this method at
	// submit time, so we only need to synchronize around the command-pool operations themselves rather than holding the
	// pool lock across frames.
	std::unique_lock<std::mutex> lock(_transferPoolMutex);
	while (_transferPoolLocked.test_and_set(std::memory_order_acquire))
	{
		_transferPoolLockReleased.wait(lock);
	}

	// Create a command buffer for the caller
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;
	VkCommandBuffer commandBuffer;
	VkResult vkAllocateCommandBuffersResult = vkAllocateCommandBuffers(_device, &allocInfo, &commandBuffer);
	if (vkAllocateCommandBuffersResult != VK_SUCCESS)
	{
		_log->Error("vkAllocateCommandBuffers failed with error code {0} for GetBuildCommandBuffer", vkAllocateCommandBuffersResult);
		_transferPoolLocked.clear(std::memory_order_release);
		lock.unlock();
		_transferPoolLockReleased.notify_one();
		return VK_NULL_HANDLE;
	}

	// Begin the process of recording the command buffer
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	VkResult vkBeginCommandBufferResult = vkBeginCommandBuffer(commandBuffer, &beginInfo);
	if (vkBeginCommandBufferResult != VK_SUCCESS)
	{
		_log->Error("vkBeginCommandBuffer failed with error code {0} for GetBuildCommandBuffer", vkBeginCommandBufferResult);
		vkFreeCommandBuffers(_device, commandPool, 1, &commandBuffer);
		_transferPoolLocked.clear(std::memory_order_release);
		lock.unlock();
		_transferPoolLockReleased.notify_one();
		return VK_NULL_HANDLE;
	}

	// Release the pool lock now that allocation and command-buffer begin are complete.
	_transferPoolLocked.clear(std::memory_order_release);
	lock.unlock();
	_transferPoolLockReleased.notify_one();

	// Return the build command buffer to the caller
	return commandBuffer;
}

//----------------------------------------------------------------------------------------
void VulkanRenderer::FreeBatchCommandBuffer(VkCommandBuffer commandBuffer, VkCommandPool commandPool)
{
	std::unique_lock<std::mutex> lock(_transferPoolMutex);
	while (_transferPoolLocked.test_and_set(std::memory_order_acquire))
	{
		_transferPoolLockReleased.wait(lock);
	}
	if (commandBuffer != VK_NULL_HANDLE)
	{
		vkFreeCommandBuffers(_device, commandPool, 1, &commandBuffer);
	}
	_transferPoolLocked.clear(std::memory_order_release);
	lock.unlock();
	_transferPoolLockReleased.notify_one();
}

//----------------------------------------------------------------------------------------
void VulkanRenderer::SubmitBuildCommandBuffer(VkCommandBuffer commandBuffer)
{
	// Submit the command buffer
	SubmitBatchCommandBuffer(commandBuffer, VK_NULL_HANDLE, _transferQueue, ITransferBatch::StartTiming::Immediately, nullptr);

	// Add this command buffer to the list of command buffers to be cleaned up at the start of the next frame
	_transferCommandBuffers.push_back(commandBuffer);
	_frameStartTransferCommandsInProgress = true;
}

//----------------------------------------------------------------------------------------
void VulkanRenderer::SubmitBatchCommandBuffer(VkCommandBuffer commandBuffer, VkFence completionFence, VkQueue targetQueue, ITransferBatch::StartTiming startTiming, std::shared_ptr<CompletionToken> submitCompleteToken)
{
	// Adjust the start timing based on the available queues. If our target device only exposes one queue, we'll end up
	// with graphics operations and transfer operations being serialized on the one queue. In this case, we don't want
	// to interrupt a possible frame draw in progress partway with our submission here. Since we can only get a
	// serialized stream of transfers in this case anyway, we defer immediate operations to begin after the current
	// frame draw. Our queued transfers will then begin immediately after the frame draw is complete. Due to the serial
	// nature of operation, they will have to fully complete before draw operations can begin for the next frame anyway,
	// but by deferring them we'll reduce the overall time by potentially letting them run before the start of the new
	// frame without delaying its completion.
	if (targetQueue == _graphicsQueue)
	{
		startTiming = ITransferBatch::StartTiming::AfterCurrentFrame;
	}

	// Complete the recording process for the batch command buffer
	VkResult vkEndCommandBufferResult = vkEndCommandBuffer(commandBuffer);
	if (vkEndCommandBufferResult != VK_SUCCESS)
	{
		_log->Error("vkEndCommandBuffer failed with error code {0} for SubmitBatchCommandBuffer", vkEndCommandBufferResult);
		return;
	}

	// Submit the command buffer for processing, taking the start timing into account.
	if (startTiming == ITransferBatch::StartTiming::AfterCurrentFrame)
	{
		// Queue the command buffer for processing after the current frame completes. We optimize for the case where no
		// completion fence is requested, and the transfer queue is being targeted, as this will catch normal transfers
		// that have been requested during the build process, but have been deferred as there aren't separate graphics
		// and transfer queues available. This will be a common case, so it's desirable to do this in order to batch all
		// the command buffers into one submission.
		if ((completionFence == VK_NULL_HANDLE) && (targetQueue == _transferQueue))
		{
			_delayedTransferCommandBuffersNoFence.push_back(commandBuffer);
		}
		else
		{
			DelayedTransferEntry delayedTransferEntry = {};
			delayedTransferEntry.targetQueue = targetQueue;
			delayedTransferEntry.commandBuffer = commandBuffer;
			delayedTransferEntry.completionFence = completionFence;
			delayedTransferEntry.submitCompleteToken = std::move(submitCompleteToken);
			std::scoped_lock<std::mutex> lock(_pendingGraphicsQueueTransferMutex);
			_delayedTransferCommandBuffers.push_back(std::move(delayedTransferEntry));
		}

		// Flag that at least one transfer command is running to be completed before the next frame begins drawing
		_frameStartTransferCommandsInProgress = true;
	}
	else
	{
		// Submit the command buffer for execution. Note that we also need to ensure that the same command queue is only
		// used on one thread at a time, which for our purposes here is limited to calls to vkQueueSubmit. The added
		// twist with a queue is that we may not have unique queues between the graphics, transfer, and batch transfer
		// processes, depending on the queue resources that are exposed by the graphics driver. To make things simple,
		// we take a lock on all calls to vkQueueSubmit, which will work regardless of whether queues are separate or
		// shared.
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;
		std::unique_lock<std::mutex> queueLock(_queueMutex);
		VkResult vkQueueSubmitResult = vkQueueSubmit(targetQueue, 1, &submitInfo, completionFence);
		queueLock.unlock();
		if (vkQueueSubmitResult != VK_SUCCESS)
		{
			_log->Error("vkQueueSubmit failed with error code {0} for SubmitBatchCommandBuffer", vkQueueSubmitResult);
		}

		// Notify any waiting threads that submission is now complete. Note that we need this, as VkFence objects are
		// not thread safe. It is illegal to call vkQueueSubmit with a fence on one thread, while another thread is
		// waiting on that fence with vkWaitForFences. We need to pre-synchronize with the submission process here, the
		// hand over the wait operation to the other thread. It is legal for more than one thread to call
		// vkWaitForFences one the same fence once already submitted however, so no further synchronization between
		// threads is required after submission.
		if (submitCompleteToken != nullptr)
		{
			std::scoped_lock<std::mutex> lock(submitCompleteToken->mutex);
			submitCompleteToken->complete = true;
			submitCompleteToken->notifyOnComplete.notify_all();
		}
	}
}

//----------------------------------------------------------------------------------------
void VulkanRenderer::CreatePersistentUploadBuffer(size_t bufferSizeInBytes, VkBuffer& buffer, VmaAllocation& bufferAllocation)
{
	CreatePersistentTransferBuffer(bufferSizeInBytes, buffer, bufferAllocation, true);
}

//----------------------------------------------------------------------------------------
void VulkanRenderer::CreatePersistentReadbackBuffer(size_t bufferSizeInBytes, VkBuffer& buffer, VmaAllocation& bufferAllocation)
{
	CreatePersistentTransferBuffer(bufferSizeInBytes, buffer, bufferAllocation, false);
}

//----------------------------------------------------------------------------------------
void VulkanRenderer::CreatePersistentTransferBuffer(size_t bufferSizeInBytes, VkBuffer& buffer, VmaAllocation& bufferAllocation, bool isUploadBuffer)
{
	// Create the requested transfer buffer
	VkBufferUsageFlags usageFlags = (isUploadBuffer ? VK_BUFFER_USAGE_TRANSFER_SRC_BIT : VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	if (!_memoryManager->CreateBuffer(bufferSizeInBytes, usageFlags, VMA_MEMORY_USAGE_CPU_ONLY, 0, buffer, bufferAllocation))
	{
		_log->Error("CreateBuffer failed in CreatePersistentTransferBuffer");
		return;
	}
}

//----------------------------------------------------------------------------------------
void VulkanRenderer::CreateTemporaryUploadBuffer(size_t bufferSizeInBytes, VkBuffer& buffer, VmaAllocation& bufferAllocation, bool keepUntilNextFrame)
{
	// Create the requested transfer buffer
	if (!_memoryManager->CreateBuffer(bufferSizeInBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, 0, buffer, bufferAllocation))
	{
		_log->Error("CreateBuffer failed in CreateTemporaryUploadBuffer");
		return;
	}

	// Add this buffer to the set of allocated transfer buffers
	TransferBufferAllocation allocationEntry = {};
	allocationEntry.bufferSizeInBytes = bufferSizeInBytes;
	allocationEntry.buffer = buffer;
	allocationEntry.bufferAllocation = bufferAllocation;
	std::unique_lock<std::mutex> lock(_transferBufferMutex);
	if (keepUntilNextFrame)
	{
		_state[_drawIndex].transferBufferAllocations.push_back(allocationEntry);
	}
	else
	{
		_state[_buildIndex].transferBufferAllocations.push_back(allocationEntry);
	}
}

//----------------------------------------------------------------------------------------
void VulkanRenderer::IncrementDetachedTransferBatchCount()
{
	++_detachedTransferBatchCount;
}

//----------------------------------------------------------------------------------------
void VulkanRenderer::DecrementDetachedTransferBatchCount()
{
	if (--_detachedTransferBatchCount == 0)
	{
		_detachedTransferBatchCountReachedZero.notify_all();
	}
}

//----------------------------------------------------------------------------------------
// Memory methods
//----------------------------------------------------------------------------------------
VulkanMemoryManager* VulkanRenderer::GetMemoryManager() const
{
	return _memoryManager.get();
}

//----------------------------------------------------------------------------------------
// Texture and sampler methods
//----------------------------------------------------------------------------------------
float VulkanRenderer::GetMaxTextureSamplerAnisotropy() const
{
	return _physicalDeviceProperties.limits.maxSamplerAnisotropy;
}

//----------------------------------------------------------------------------------------
// Null descriptor fallback methods
//----------------------------------------------------------------------------------------
VkBuffer VulkanRenderer::GetNullDescriptorFallbackVertexBuffer() const
{
	if (!CreateNullDescriptorFallbackBufferIfRequired())
	{
		return VK_NULL_HANDLE;
	}
	return _nullDescriptorFallbackBuffer.buffer;
}

//----------------------------------------------------------------------------------------
VkBuffer VulkanRenderer::GetNullDescriptorFallbackUniformBuffer() const
{
	if (!CreateNullDescriptorFallbackBufferIfRequired())
	{
		return VK_NULL_HANDLE;
	}
	return _nullDescriptorFallbackBuffer.buffer;
}

//----------------------------------------------------------------------------------------
VkBuffer VulkanRenderer::GetNullDescriptorFallbackStorageBuffer() const
{
	if (!CreateNullDescriptorFallbackBufferIfRequired())
	{
		return VK_NULL_HANDLE;
	}
	return _nullDescriptorFallbackBuffer.buffer;
}

//----------------------------------------------------------------------------------------
VkBufferView VulkanRenderer::GetNullDescriptorFallbackTexelBufferView(VkFormat format, bool writeable) const
{
	if (!CreateNullDescriptorFallbackBufferIfRequired())
	{
		return VK_NULL_HANDLE;
	}
	if (format == VK_FORMAT_UNDEFINED)
	{
		format = VK_FORMAT_R32_UINT;
	}

	auto buildNullDescriptorFallbackTexelBufferViewKey = [](VkFormat format, bool writeable) {
		return (static_cast<uint64_t>(format) << 1) | static_cast<uint64_t>(writeable ? 1 : 0);
	};
	uint64_t key = buildNullDescriptorFallbackTexelBufferViewKey(format, writeable);
	auto existingEntry = _nullDescriptorFallbackTexelBufferViews.find(key);
	if (existingEntry != _nullDescriptorFallbackTexelBufferViews.end())
	{
		return existingEntry->second;
	}

	size_t elementSizeInBytes = GetNullDescriptorFallbackTexelSizeInBytes(format);
	if (elementSizeInBytes == 0)
	{
		_log->Error("No null descriptor fallback texel buffer format mapping exists for VkFormat {0}", format);
		return VK_NULL_HANDLE;
	}

	VkBufferViewCreateInfo bufferViewInfo = {};
	bufferViewInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
	bufferViewInfo.buffer = _nullDescriptorFallbackBuffer.buffer;
	bufferViewInfo.format = format;
	bufferViewInfo.offset = 0;
	bufferViewInfo.range = elementSizeInBytes;

	VkBufferView bufferView = VK_NULL_HANDLE;
	VkResult result = vkCreateBufferView(_device, &bufferViewInfo, nullptr, &bufferView);
	if (result != VK_SUCCESS)
	{
		_log->Error("vkCreateBufferView failed with error code {0} when creating a null descriptor fallback texel buffer view", result);
		return VK_NULL_HANDLE;
	}

	_nullDescriptorFallbackTexelBufferViews.insert(std::make_pair(key, bufferView));
	return bufferView;
}

//----------------------------------------------------------------------------------------
VkImageView VulkanRenderer::GetNullDescriptorFallbackTextureView(VkImageViewType viewType) const
{
	// If we've already created the fallback texture, return it now.
	auto existingEntry = _nullDescriptorFallbackTextures.find(viewType);
	if (existingEntry != _nullDescriptorFallbackTextures.end())
	{
		return existingEntry->second.imageView;
	}

	VkExtent3D imageExtent = {1, 1, 1};
	uint32_t arrayLayerCount = 1;
	VkImageType imageType = VK_IMAGE_TYPE_2D;
	VkImageCreateFlags imageCreateFlags = 0;
	switch (viewType)
	{
	case VK_IMAGE_VIEW_TYPE_1D:
		imageType = VK_IMAGE_TYPE_1D;
		imageExtent.height = 1;
		imageExtent.depth = 1;
		break;
	case VK_IMAGE_VIEW_TYPE_2D:
		imageType = VK_IMAGE_TYPE_2D;
		break;
	case VK_IMAGE_VIEW_TYPE_3D:
		imageType = VK_IMAGE_TYPE_3D;
		break;
	case VK_IMAGE_VIEW_TYPE_CUBE:
		imageType = VK_IMAGE_TYPE_2D;
		arrayLayerCount = 6;
		imageCreateFlags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
		break;
	case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
		imageType = VK_IMAGE_TYPE_1D;
		arrayLayerCount = 1;
		break;
	case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
		imageType = VK_IMAGE_TYPE_2D;
		arrayLayerCount = 1;
		break;
	case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
		imageType = VK_IMAGE_TYPE_2D;
		arrayLayerCount = 6;
		imageCreateFlags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
		break;
	default:
		_log->Error("No null descriptor fallback texture view mapping exists for VkImageViewType {0}", viewType);
		return VK_NULL_HANDLE;
	}

	VkImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = imageType;
	imageCreateInfo.extent = imageExtent;
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = arrayLayerCount;
	imageCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.flags = imageCreateFlags;

	VmaAllocationCreateInfo imageAllocationCreateInfo = {};
	imageAllocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	VmaAllocation imageAllocation = {};
	VkImage image = VK_NULL_HANDLE;
	if (vmaCreateImage(_memoryManager->Allocator(), &imageCreateInfo, &imageAllocationCreateInfo, &image, &imageAllocation, nullptr) != VK_SUCCESS)
	{
		_log->Error("Failed to create a null descriptor fallback texture image for VkImageViewType {0}", viewType);
		return VK_NULL_HANDLE;
	}

	VkImageViewCreateInfo imageViewCreateInfo = {};
	imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.image = image;
	imageViewCreateInfo.viewType = viewType;
	imageViewCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
	imageViewCreateInfo.subresourceRange.levelCount = 1;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imageViewCreateInfo.subresourceRange.layerCount = arrayLayerCount;

	VkImageView imageView = VK_NULL_HANDLE;
	VkResult imageViewResult = vkCreateImageView(_device, &imageViewCreateInfo, nullptr, &imageView);
	if (imageViewResult != VK_SUCCESS)
	{
		_log->Error("vkCreateImageView failed with error code {0} when creating a null descriptor fallback texture view", imageViewResult);
		_memoryManager->DestroyImage(image, imageAllocation);
		return VK_NULL_HANDLE;
	}

	VkCommandPool commandPool = VK_NULL_HANDLE;
	VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
	VkFence fence = VK_NULL_HANDLE;

	VkCommandPoolCreateInfo commandPoolCreateInfo = {};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.queueFamilyIndex = _graphicsQueueFamilyIndex;
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	VkResult commandPoolResult = vkCreateCommandPool(_device, &commandPoolCreateInfo, nullptr, &commandPool);
	if (commandPoolResult != VK_SUCCESS)
	{
		_log->Error("vkCreateCommandPool failed with error code {0} when preparing a null descriptor fallback texture", commandPoolResult);
		vkDestroyImageView(_device, imageView, nullptr);
		_memoryManager->DestroyImage(image, imageAllocation);
		return VK_NULL_HANDLE;
	}

	VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.commandPool = commandPool;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandBufferCount = 1;
	VkResult commandBufferResult = vkAllocateCommandBuffers(_device, &commandBufferAllocateInfo, &commandBuffer);
	if (commandBufferResult != VK_SUCCESS)
	{
		_log->Error("vkAllocateCommandBuffers failed with error code {0} when preparing a null descriptor fallback texture", commandBufferResult);
		vkDestroyCommandPool(_device, commandPool, nullptr);
		vkDestroyImageView(_device, imageView, nullptr);
		_memoryManager->DestroyImage(image, imageAllocation);
		return VK_NULL_HANDLE;
	}

	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	VkResult beginResult = vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
	if (beginResult != VK_SUCCESS)
	{
		_log->Error("vkBeginCommandBuffer failed with error code {0} when preparing a null descriptor fallback texture", beginResult);
		vkDestroyCommandPool(_device, commandPool, nullptr);
		vkDestroyImageView(_device, imageView, nullptr);
		_memoryManager->DestroyImage(image, imageAllocation);
		return VK_NULL_HANDLE;
	}

	if (!_memoryManager->RecordTransitionImageLayout(commandBuffer, image, VK_FORMAT_R8G8B8A8_UNORM, 1, arrayLayerCount, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL))
	{
		vkDestroyCommandPool(_device, commandPool, nullptr);
		vkDestroyImageView(_device, imageView, nullptr);
		_memoryManager->DestroyImage(image, imageAllocation);
		return VK_NULL_HANDLE;
	}

	VkResult endResult = vkEndCommandBuffer(commandBuffer);
	if (endResult != VK_SUCCESS)
	{
		_log->Error("vkEndCommandBuffer failed with error code {0} when preparing a null descriptor fallback texture", endResult);
		vkDestroyCommandPool(_device, commandPool, nullptr);
		vkDestroyImageView(_device, imageView, nullptr);
		_memoryManager->DestroyImage(image, imageAllocation);
		return VK_NULL_HANDLE;
	}

	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	VkResult fenceResult = vkCreateFence(_device, &fenceCreateInfo, nullptr, &fence);
	if (fenceResult != VK_SUCCESS)
	{
		_log->Error("vkCreateFence failed with error code {0} when preparing a null descriptor fallback texture", fenceResult);
		vkDestroyCommandPool(_device, commandPool, nullptr);
		vkDestroyImageView(_device, imageView, nullptr);
		_memoryManager->DestroyImage(image, imageAllocation);
		return VK_NULL_HANDLE;
	}

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	VkResult submitResult = VK_SUCCESS;
	{
		std::lock_guard<std::mutex> queueLock(_queueMutex);
		submitResult = vkQueueSubmit(_graphicsQueue, 1, &submitInfo, fence);
	}
	if (submitResult != VK_SUCCESS)
	{
		_log->Error("vkQueueSubmit failed with error code {0} when preparing a null descriptor fallback texture", submitResult);
		vkDestroyFence(_device, fence, nullptr);
		vkDestroyCommandPool(_device, commandPool, nullptr);
		vkDestroyImageView(_device, imageView, nullptr);
		_memoryManager->DestroyImage(image, imageAllocation);
		return VK_NULL_HANDLE;
	}

	vkWaitForFences(_device, 1, &fence, VK_TRUE, UINT64_MAX);
	vkDestroyFence(_device, fence, nullptr);
	vkDestroyCommandPool(_device, commandPool, nullptr);

	auto& createdEntry = _nullDescriptorFallbackTextures[viewType];
	createdEntry.image = image;
	createdEntry.allocation = imageAllocation;
	createdEntry.imageView = imageView;
	return imageView;
}

//----------------------------------------------------------------------------------------
VkSampler VulkanRenderer::GetNullDescriptorFallbackSampler() const
{
	// If we've already created the fallback sampler, return it now.
	if (_nullDescriptorFallbackSampler != VK_NULL_HANDLE)
	{
		return _nullDescriptorFallbackSampler;
	}

	// Create and return the fallback sampler
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.minFilter = VK_FILTER_NEAREST;
	samplerInfo.magFilter = VK_FILTER_NEAREST;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;
	samplerInfo.maxAnisotropy = 1.0f;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	VkResult vkCreateSamplerReturn = vkCreateSampler(_device, &samplerInfo, nullptr, &_nullDescriptorFallbackSampler);
	if (vkCreateSamplerReturn != VK_SUCCESS)
	{
		_log->Error("vkCreateSampler failed with error code {0} when creating a null descriptor fallback sampler", vkCreateSamplerReturn);
		return VK_NULL_HANDLE;
	}
	return _nullDescriptorFallbackSampler;
}

//----------------------------------------------------------------------------------------
bool VulkanRenderer::CreateNullDescriptorFallbackBufferIfRequired() const
{
	// If we've already created the fallback buffer, return true.
	if (_nullDescriptorFallbackBuffer.buffer != VK_NULL_HANDLE)
	{
		return true;
	}

	// A single small buffer is enough for our fallback vertex, uniform, storage, and texel bindings.
	constexpr VkDeviceSize NullDescriptorFallbackBufferSizeInBytes = 96;
	VkBufferUsageFlags usageFlags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
	void* mappedData = nullptr;
	if (!_memoryManager->CreateMappedBuffer(NullDescriptorFallbackBufferSizeInBytes, usageFlags, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, _nullDescriptorFallbackBuffer.buffer, _nullDescriptorFallbackBuffer.allocation, mappedData))
	{
		_log->Error("CreateBuffer failed when creating a null descriptor fallback buffer");
		return false;
	}
	std::memset(mappedData, 0, NullDescriptorFallbackBufferSizeInBytes);
	vmaFlushAllocation(_memoryManager->Allocator(), _nullDescriptorFallbackBuffer.allocation, 0, VK_WHOLE_SIZE);
	return true;
}

//----------------------------------------------------------------------------------------
size_t VulkanRenderer::GetNullDescriptorFallbackTexelSizeInBytes(VkFormat format) const
{
	switch (format)
	{
	case VK_FORMAT_R8_UINT:
		return 1;
	case VK_FORMAT_R32_SINT:
	case VK_FORMAT_R32_UINT:
	case VK_FORMAT_R32_SFLOAT:
		return 4;
	case VK_FORMAT_R8G8_UINT:
	case VK_FORMAT_R32G32_SINT:
	case VK_FORMAT_R32G32_UINT:
	case VK_FORMAT_R32G32_SFLOAT:
	case VK_FORMAT_R64_SINT:
	case VK_FORMAT_R64_UINT:
	case VK_FORMAT_R64_SFLOAT:
		return 8;
	case VK_FORMAT_R8G8B8_UINT:
	case VK_FORMAT_R32G32B32_SINT:
	case VK_FORMAT_R32G32B32_UINT:
	case VK_FORMAT_R32G32B32_SFLOAT:
		return 12;
	case VK_FORMAT_R8G8B8A8_UINT:
	case VK_FORMAT_R32G32B32A32_SINT:
	case VK_FORMAT_R32G32B32A32_UINT:
	case VK_FORMAT_R32G32B32A32_SFLOAT:
	case VK_FORMAT_R64G64_SINT:
	case VK_FORMAT_R64G64_UINT:
	case VK_FORMAT_R64G64_SFLOAT:
		return 16;
	case VK_FORMAT_R64G64B64_SINT:
	case VK_FORMAT_R64G64B64_UINT:
	case VK_FORMAT_R64G64B64_SFLOAT:
		return 24;
	case VK_FORMAT_R64G64B64A64_SINT:
	case VK_FORMAT_R64G64B64A64_UINT:
	case VK_FORMAT_R64G64B64A64_SFLOAT:
		return 32;
	}
	// If we couldn't determine the size, default to the largest one.
	return 32;
}

//----------------------------------------------------------------------------------------
void VulkanRenderer::DestroyNullDescriptorFallbackResources()
{
	for (const auto& entry : _nullDescriptorFallbackTexelBufferViews)
	{
		vkDestroyBufferView(_device, entry.second, nullptr);
	}
	_nullDescriptorFallbackTexelBufferViews.clear();

	for (const auto& entry : _nullDescriptorFallbackTextures)
	{
		vkDestroyImageView(_device, entry.second.imageView, nullptr);
		_memoryManager->DestroyImage(entry.second.image, entry.second.allocation);
	}
	_nullDescriptorFallbackTextures.clear();

	if (_nullDescriptorFallbackSampler != VK_NULL_HANDLE)
	{
		vkDestroySampler(_device, _nullDescriptorFallbackSampler, nullptr);
		_nullDescriptorFallbackSampler = VK_NULL_HANDLE;
	}

	if (_nullDescriptorFallbackBuffer.buffer != VK_NULL_HANDLE)
	{
		_memoryManager->DestroyBuffer(_nullDescriptorFallbackBuffer.buffer, _nullDescriptorFallbackBuffer.allocation);
		_nullDescriptorFallbackBuffer.buffer = VK_NULL_HANDLE;
		_nullDescriptorFallbackBuffer.allocation = {};
	}
}

//----------------------------------------------------------------------------------------
// Settings methods
//----------------------------------------------------------------------------------------
bool VulkanRenderer::DebugLoggingEnabled() const
{
	return _enableDebugLogging;
}

} // namespace cobalt::graphics
