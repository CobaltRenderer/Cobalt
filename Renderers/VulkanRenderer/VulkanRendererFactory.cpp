// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "AssemblyVersionInfo.h"
#include "VulkanGraphicsDeviceEnumerator.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <memory>
#ifdef _WIN32
#define FUNCTION_EXPORT __declspec(dllexport)
#else
#define FUNCTION_EXPORT __attribute__((visibility("default")))
#endif
namespace cobalt::graphics {

// Predeclarations
extern "C" FUNCTION_EXPORT IGraphicsDeviceEnumerator* CreateVulkanRenderer(cobalt::logging::ILogger* log);
extern "C" FUNCTION_EXPORT bool GetVulkanRendererPlugin(IRendererPlugin& rendererPlugin);
extern "C" FUNCTION_EXPORT bool GetRendererPlugin(unsigned int indexNo, IRendererPlugin& rendererPlugin);
extern "C" FUNCTION_EXPORT void GetCobaltAPIVersion(unsigned int& major, unsigned int& minor, unsigned int& patch);

//----------------------------------------------------------------------------------------
extern "C" FUNCTION_EXPORT IGraphicsDeviceEnumerator* CreateVulkanRenderer(cobalt::logging::ILogger* log)
{
	return new VulkanGraphicsDeviceEnumerator(cobalt::logging::ILogger::unique_ptr(log));
}

//----------------------------------------------------------------------------------------
extern "C" FUNCTION_EXPORT bool GetVulkanRendererPlugin(IRendererPlugin& rendererPlugin)
{
	rendererPlugin.SetAllocationFunction(CreateVulkanRenderer);
	rendererPlugin.SetApiFamily(IRendererPlugin::ApiFamily::Vulkan);
	rendererPlugin.SetTargetApiVersion(IRendererPlugin::ApiVersion(1, 1));
	rendererPlugin.SetName("Vulkan1_1");
	rendererPlugin.SetDisplayName("Vulkan 1.1");
	return true;
}

//----------------------------------------------------------------------------------------
extern "C" FUNCTION_EXPORT bool GetRendererPlugin(unsigned int indexNo, IRendererPlugin& rendererPlugin)
{
	switch (indexNo)
	{
	case 0:
		return GetVulkanRendererPlugin(rendererPlugin);
	}
	return false;
}

//----------------------------------------------------------------------------------------
extern "C" FUNCTION_EXPORT void GetCobaltAPIVersion(unsigned int& major, unsigned int& minor, unsigned int& patch)
{
	major = AssemblyVersionInfo_VersionDigit_Major;
	minor = AssemblyVersionInfo_VersionDigit_Minor;
	patch = AssemblyVersionInfo_VersionDigit_Revision;
}

} // namespace cobalt::graphics
