// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "VulkanInstanceData.h"
#include <cstring>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
VulkanInstanceData::VulkanInstanceData(cobalt::logging::ILogger::unique_ptr log, VkInstance instance)
: _log(std::move(log)), instance(instance)
{}

//----------------------------------------------------------------------------------------
VulkanInstanceData::~VulkanInstanceData()
{
	// If a debug message callback is attached, destroy it now.
	if (_debugMessenger != VK_NULL_HANDLE)
	{
		auto destroyFunc = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
		if (destroyFunc != nullptr)
		{
			destroyFunc(instance, _debugMessenger, nullptr);
		}
	}

	// At this point, all renderers, devices, and the device enumerator itself should be destroyed now, so we can clean
	// up the Vulkan instance now.
	_log->Debug("Destroying Vulkan Instance");
	vkDestroyInstance(instance, nullptr);
}

//----------------------------------------------------------------------------------------
// Logging methods
//----------------------------------------------------------------------------------------
bool VulkanInstanceData::RegisterDebugMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severityFilter, const std::vector<std::string>& ignoredMessageNames)
{
	// Attempt to locate the vkCreateDebugUtilsMessengerEXT extension method
	auto vkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
	if (vkCreateDebugUtilsMessengerEXT == nullptr)
	{
		_log->Error("Failed to locate vkCreateDebugUtilsMessengerEXT.");
		return false;
	}

	// Configure debug message filtering settings
	_messageSeverityFilter = severityFilter;
	_ignoredMessageNames = ignoredMessageNames;

	// Create the debug message handler
	VkDebugUtilsMessengerCreateInfoEXT messengerInfo = {};
	messengerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	messengerInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	messengerInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	messengerInfo.pfnUserCallback = DebugMessengerCallback;
	messengerInfo.pUserData = reinterpret_cast<void*>(this);
	VkResult vkCreateDebugUtilsMessengerEXTReturn = vkCreateDebugUtilsMessengerEXT(instance, &messengerInfo, nullptr, &_debugMessenger);
	if (vkCreateDebugUtilsMessengerEXTReturn != VK_SUCCESS)
	{
		_log->Error("vkCreateDebugUtilsMessengerEXT failed with error code {0}", vkCreateDebugUtilsMessengerEXTReturn);
		_debugMessenger = VK_NULL_HANDLE;
		return false;
	}
	return true;
}

//----------------------------------------------------------------------------------------
VKAPI_ATTR VkBool32 VKAPI_CALL VulkanInstanceData::DebugMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData)
{
	auto* instanceData = reinterpret_cast<VulkanInstanceData*>(userData);
	return instanceData->DebugMessengerCallbackInternal(messageSeverity, messageType, callbackData);
}

//----------------------------------------------------------------------------------------
VkBool32 VulkanInstanceData::DebugMessengerCallbackInternal(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* callbackData) const
{
	// Some validation layers don't provide an ID name
	if (callbackData->pMessageIdName == nullptr)
	{
		_log->Debug("Validator No ID: {0}", callbackData->pMessage);
		return VK_TRUE;
	}

	// Check priority and output if severity is above filter
	if (messageSeverity >= _messageSeverityFilter)
	{
		// Filter out ignored messages. Note that we use std::strcmp here. This callback could get hit frequently, and
		// we don't want to be allocating memory here just for a string comparison.
		for (const auto& name : _ignoredMessageNames)
		{
			if (std::strcmp(name.c_str(), callbackData->pMessageIdName) == 0)
			{
				return VK_FALSE;
			}
		}

		// Output message in corresponding log severity
		if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		{
			_log->Error("Validator Error {0}: {1}", callbackData->pMessageIdName, callbackData->pMessage);
		}
		else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		{
			_log->Warning("Validator Warning {0}: {1}", callbackData->pMessageIdName, callbackData->pMessage);
		}
		else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
		{
			_log->Info("Validator Info {0}: {1}", callbackData->pMessageIdName, callbackData->pMessage);
		}
		else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
		{
			_log->Debug("Validator Verbose {0}: {1}", callbackData->pMessageIdName, callbackData->pMessage);
		}
	}
	return VK_FALSE;
}

} // namespace cobalt::graphics
