// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "VulkanHeaders.h"
#include "VulkanInstanceData.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <memory>
#include <unordered_map>
namespace cobalt::graphics {

class VulkanGraphicsDevice : public IGraphicsDevice
{
public:
	// Constructors
	VulkanGraphicsDevice(cobalt::logging::ILogger* log, std::shared_ptr<VulkanInstanceData> instanceData, VkPhysicalDevice physicalDevice);

	// Info methods
	DeviceType GetDeviceType() const override;
	Vendor GetVendor() const override;
	Marshal::Ret<std::string> GetVendorName() const override;
	Marshal::Ret<std::string> GetDeviceName() const override;
	Marshal::Ret<std::string> GetDriverInfo() const override;
	size_t GetMemorySizeInBytes(MemoryType memoryType) const override;
	bool SupportsDeviceExtension(const std::string& extensionName) const;

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
	logging::ILogger* _log = nullptr;
	std::shared_ptr<VulkanInstanceData> _instanceData;
	VkPhysicalDevice _physicalDevice = {};
	VkPhysicalDeviceProperties2 _deviceProperties = {};
	VkPhysicalDeviceDriverProperties _driverProperties = {};
	VkPhysicalDeviceFeatures _deviceFeatures = {};
	uint32_t _minVertexInputBindingStrideAlignment = 0;
	std::unordered_map<std::string, VkExtensionProperties> _deviceExtensions;
	std::set<std::string> _deviceExtensionsSet;
	std::set<Feature> _supportedFeatureSet;
};

} // namespace cobalt::graphics
