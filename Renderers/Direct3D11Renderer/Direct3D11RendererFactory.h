// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once

// C++ usage
#ifdef __cplusplus
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
namespace cobalt::graphics {

extern "C" __declspec(dllimport) bool GetDirect3D11RendererPlugin(IRendererPlugin& rendererPlugin);
extern "C" __declspec(dllimport) IGraphicsDeviceEnumerator* CreateDirect3D11Renderer(cobalt::logging::ILogger* log);

} // namespace cobalt::graphics

// C usage
#else
__declspec(dllimport) int GetDirect3D11RendererPlugin(void);
#endif
