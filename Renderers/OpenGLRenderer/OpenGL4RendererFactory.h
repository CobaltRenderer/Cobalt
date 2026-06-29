// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once

// C++ usage
#ifdef __cplusplus
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
namespace cobalt::graphics {

#ifdef _WIN32
extern "C" __declspec(dllimport) bool GetOpenGL4RendererPlugin(IRendererPlugin& rendererPlugin);
extern "C" __declspec(dllimport) IGraphicsDeviceEnumerator* CreateOpenGL4Renderer(cobalt::logging::ILogger* log);
#else
extern "C" bool GetOpenGL4RendererPlugin(IRendererPlugin& rendererPlugin);
extern "C" IGraphicsDeviceEnumerator* CreateOpenGL4Renderer(cobalt::logging::ILogger* log);
#endif

} // namespace cobalt::graphics

// C usage
#else
#ifdef _WIN32
__declspec(dllimport) int GetOpenGL4RendererPlugin(void);
#else
int GetOpenGL4RendererPlugin(void);
#endif
#endif
