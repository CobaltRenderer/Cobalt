// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#ifdef COBALT_RENDERER_SDL3_SUPPORT
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#ifdef COBALT_RENDERER_XCB_SUPPORT
#include <X11/Xlib-xcb.h>
// Note that we deliberately include this after Xlib-xcb.h
#include "XlibHeaders.h"
#endif
#include <cstring>
#include <memory>
namespace cobalt { namespace graphics {

class WindowSystemInfoSDL3
{
public:
	bool Initialize(cobalt::logging::ILogger* log)
	{
		// Create a hidden window so we can query the window system properties
#if defined(COBALT_RENDERER_XLIB_SUPPORT) || defined(COBALT_RENDERER_XCB_SUPPORT) || defined(COBALT_RENDERER_WAYLAND_SUPPORT)
		SDL_Window* hiddenWindow = SDL_CreateWindow("HiddenWindow", 64, 64, SDL_WINDOW_HIDDEN);
		if (hiddenWindow == nullptr)
		{
			log->Error("SDL_CreateWindow failed with error code {0}", SDL_GetError());
			return false;
		}
		SDL_PropertiesID windowProperties = SDL_GetWindowProperties(hiddenWindow);
		if (windowProperties == 0)
		{
			log->Error("SDL_GetWindowProperties failed for the hidden SDL window: {0}", SDL_GetError());
			SDL_DestroyWindow(hiddenWindow);
			return false;
		}
#endif

		// Build the window system info structure for the bound window system
		auto driver = SDL_GetCurrentVideoDriver();
		if (driver == nullptr)
		{
			log->Error("SDL_GetCurrentVideoDriver returned no active video driver.");
#if defined(COBALT_RENDERER_XLIB_SUPPORT) || defined(COBALT_RENDERER_XCB_SUPPORT) || defined(COBALT_RENDERER_WAYLAND_SUPPORT)
			SDL_DestroyWindow(hiddenWindow);
#endif
			return false;
		}
#ifdef COBALT_RENDERER_WIN32_SUPPORT
		if (std::strcmp(driver, "windows") == 0)
		{
			win32Info.reset(new WindowSystemInfoWin32());
			type = cobalt::graphics::IRenderer::WindowSystemInfoBase::WindowSystemType::Win32;
		}
		else
#endif
#ifdef COBALT_RENDERER_XLIB_SUPPORT
		  if (std::strcmp(driver, "x11") == 0)
		{
			auto display = (::Display*)SDL_GetPointerProperty(windowProperties, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
			if (display == nullptr)
			{
				log->Error("Failed to locate X11 window system details.");
				SDL_DestroyWindow(hiddenWindow);
				return false;
			}
			xlibInfo.reset(new WindowSystemInfoXlib(display));
			type = cobalt::graphics::IRenderer::WindowSystemInfoBase::WindowSystemType::Xlib;
		}
		else
#endif
#ifdef COBALT_RENDERER_XCB_SUPPORT
		  if (std::strcmp(driver, "x11") == 0)
		{
			auto display = (::Display*)SDL_GetPointerProperty(windowProperties, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
			if (display == nullptr)
			{
				log->Error("Failed to locate XCB window system details.");
				SDL_DestroyWindow(hiddenWindow);
				return false;
			}
			auto connection = XGetXCBConnection(display);
			if (connection == nullptr)
			{
				log->Error("Failed to retrieve XCB connection details from X11 display.");
				SDL_DestroyWindow(hiddenWindow);
				return false;
			}
			xcbInfo.reset(new WindowSystemInfoXCB(connection));
			type = cobalt::graphics::IRenderer::WindowSystemInfoBase::WindowSystemType::XCB;
		}
		else
#endif
#ifdef COBALT_RENDERER_WAYLAND_SUPPORT
		  if (std::strcmp(driver, "wayland") == 0)
		{
			auto display = (wl_display*)SDL_GetPointerProperty(windowProperties, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr);
			if (display == nullptr)
			{
				log->Error("Failed to locate Wayland window system details.");
				SDL_DestroyWindow(hiddenWindow);
				return false;
			}
			waylandInfo.reset(new WindowSystemInfoWayland(display));
			type = cobalt::graphics::IRenderer::WindowSystemInfoBase::WindowSystemType::Wayland;
		}
		else
#endif
#ifdef COBALT_RENDERER_APPKIT_SUPPORT
		  if (std::strcmp(driver, "cocoa") == 0)
		{
			appKitInfo.reset(new WindowSystemInfoAppKit());
			type = cobalt::graphics::IRenderer::WindowSystemInfoBase::WindowSystemType::AppKit;
		}
		else
#endif
		{
			log->Error("Unsupported SDL window system of type: {0}", driver);
#if defined(COBALT_RENDERER_XLIB_SUPPORT) || defined(COBALT_RENDERER_XCB_SUPPORT) || defined(COBALT_RENDERER_WAYLAND_SUPPORT)
			SDL_DestroyWindow(hiddenWindow);
#endif
			return false;
		}

		// Destroy the hidden window, and return true to the caller.
#if defined(COBALT_RENDERER_XLIB_SUPPORT) || defined(COBALT_RENDERER_XCB_SUPPORT) || defined(COBALT_RENDERER_WAYLAND_SUPPORT)
		SDL_DestroyWindow(hiddenWindow);
#endif
		return true;
	}

	cobalt::graphics::IRenderer::WindowSystemInfoBase::WindowSystemType GetType()
	{
		return type;
	}

	cobalt::graphics::IRenderer::WindowSystemInfoBase* Get()
	{
		switch (type)
		{
#ifdef COBALT_RENDERER_WIN32_SUPPORT
		case cobalt::graphics::IRenderer::WindowSystemInfoBase::WindowSystemType::Win32:
			return win32Info.get();
#endif
#ifdef COBALT_RENDERER_XLIB_SUPPORT
		case cobalt::graphics::IRenderer::WindowSystemInfoBase::WindowSystemType::Xlib:
			return xlibInfo.get();
#endif
#ifdef COBALT_RENDERER_XCB_SUPPORT
		case cobalt::graphics::IRenderer::WindowSystemInfoBase::WindowSystemType::XCB:
			return xcbInfo.get();
#endif
#ifdef COBALT_RENDERER_WAYLAND_SUPPORT
		case cobalt::graphics::IRenderer::WindowSystemInfoBase::WindowSystemType::Wayland:
			return waylandInfo.get();
#endif
#ifdef COBALT_RENDERER_APPKIT_SUPPORT
		case cobalt::graphics::IRenderer::WindowSystemInfoBase::WindowSystemType::AppKit:
			return appKitInfo.get();
#endif
		}
		return nullptr;
	}

private:
	cobalt::graphics::IRenderer::WindowSystemInfoBase::WindowSystemType type = {};
#ifdef COBALT_RENDERER_WIN32_SUPPORT
	std::unique_ptr<WindowSystemInfoWin32> win32Info;
#endif
#ifdef COBALT_RENDERER_XLIB_SUPPORT
	std::unique_ptr<WindowSystemInfoXlib> xlibInfo;
#endif
#ifdef COBALT_RENDERER_XCB_SUPPORT
	std::unique_ptr<WindowSystemInfoXCB> xcbInfo;
#endif
#ifdef COBALT_RENDERER_WAYLAND_SUPPORT
	std::unique_ptr<WindowSystemInfoWayland> waylandInfo;
#endif
#ifdef COBALT_RENDERER_APPKIT_SUPPORT
	std::unique_ptr<WindowSystemInfoAppKit> appKitInfo;
#endif
};

}} // namespace cobalt::graphics
#endif
