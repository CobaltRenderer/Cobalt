// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include <Cobalt/Debug/Debug.pkg>
#include <Cobalt/RendererInterface/PlatformBindings.pkg>
#ifdef COBALT_RENDERER_WIN32_SUPPORT
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#ifdef COBALT_RENDERER_WAYLAND_SUPPORT
#define VK_USE_PLATFORM_WAYLAND_KHR
#endif
#ifdef COBALT_RENDERER_XCB_SUPPORT
#define VK_USE_PLATFORM_XCB_KHR
#endif
#ifdef COBALT_RENDERER_XLIB_SUPPORT
#define VK_USE_PLATFORM_XLIB_KHR
#endif
WARNINGS_PUSH_OFF
#ifdef _MSC_VER
#pragma warning(disable : 2220)
#pragma warning(disable : 4189)
#pragma warning(disable : 4244)
#pragma warning(disable : 4355)
#pragma warning(disable : 4701)
#pragma warning(disable : 4703)
#pragma warning(disable : 4826)
#pragma warning(disable : 6386)
#pragma warning(disable : 6387)
#pragma warning(disable : 26110)
#pragma warning(disable : 26819)
#endif
#include <cstdio>
#include <vk_mem_alloc.h>
WARNINGS_POP
#include <vulkan/vulkan.h>
#ifdef __APPLE__
#include <vulkan/vulkan_metal.h>
#endif
