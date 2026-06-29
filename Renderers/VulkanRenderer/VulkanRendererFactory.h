// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once

// C++ usage
#ifdef __cplusplus
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
namespace cobalt::graphics {

#ifdef _WIN32
extern "C" __declspec(dllexport) bool GetVulkanRendererPlugin(IRendererPlugin& rendererPlugin);
extern "C" __declspec(dllexport) IGraphicsDeviceEnumerator* CreateVulkanRenderer(cobalt::logging::ILogger* log);
#else
extern "C" bool GetVulkanRendererPlugin(IRendererPlugin& rendererPlugin);
extern "C" IGraphicsDeviceEnumerator* CreateVulkanRenderer(cobalt::logging::ILogger* log);
#endif

} // namespace cobalt::graphics

// C usage
#else
#ifdef _WIN32
__declspec(dllimport) int GetVulkanRendererPlugin(void);
#else
int GetVulkanRendererPlugin(void);
#endif
#endif
