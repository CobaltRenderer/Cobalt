// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once

// C++ usage
#ifdef __cplusplus
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
namespace cobalt::graphics {

#ifdef _WIN32
extern "C" __declspec(dllimport) bool GetOpenGL3RendererPlugin(IRendererPlugin& rendererPlugin);
extern "C" __declspec(dllimport) IGraphicsDeviceEnumerator* CreateOpenGL3Renderer(cobalt::logging::ILogger* log);
#else
extern "C" bool GetOpenGL3RendererPlugin(IRendererPlugin& rendererPlugin);
extern "C" IGraphicsDeviceEnumerator* CreateOpenGL3Renderer(cobalt::logging::ILogger* log);
#endif

} // namespace cobalt::graphics

// C usage
#else
#ifdef _WIN32
__declspec(dllimport) int GetOpenGL3RendererPlugin(void);
#else
int GetOpenGL3RendererPlugin(void);
#endif
#endif
