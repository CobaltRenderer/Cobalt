// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "VulkanHeaders.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <string>
#include <vector>
namespace cobalt::graphics {

struct VulkanInstanceData
{
public:
	// Constructors
	VulkanInstanceData(cobalt::logging::ILogger::unique_ptr log, VkInstance instance);
	~VulkanInstanceData();

	// Logging methods
	bool RegisterDebugMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severityFilter, const std::vector<std::string>& ignoredMessageNames);

private:
	// Logging methods
	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData);
	VkBool32 DebugMessengerCallbackInternal(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* callbackData) const;

public:
	//##FIX## Make this private. Some inline getters would be appropriate.
	VkInstance instance = nullptr;
	bool debugUtilsAvailable = false;
	bool enableValidationLayers = false;

private:
	logging::ILogger::unique_ptr _log;
	VkDebugUtilsMessengerEXT _debugMessenger = VK_NULL_HANDLE;
	VkDebugUtilsMessageSeverityFlagBitsEXT _messageSeverityFilter = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	std::vector<std::string> _ignoredMessageNames;
};

} // namespace cobalt::graphics
