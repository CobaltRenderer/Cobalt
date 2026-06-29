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

class WindowInfoSDL3
{
public:
	bool Initialize(const cobalt::logging::ILogger* log, SDL_Window* sdlWindow, const V2UInt32& windowSize)
	{
		// Retrieve details on the underlying target window surface bound to the SDL window
		if (sdlWindow == nullptr)
		{
			log->Error("WindowInfoSDL3::Initialize was given a null SDL window.");
			return false;
		}
		SDL_PropertiesID windowProperties = SDL_GetWindowProperties(sdlWindow);
		if (windowProperties == 0)
		{
			log->Error("SDL_GetWindowProperties failed for the supplied SDL window: {0}", SDL_GetError());
			return false;
		}
		auto driver = SDL_GetCurrentVideoDriver();
		if (driver == nullptr)
		{
			log->Error("SDL_GetCurrentVideoDriver returned no active video driver.");
			return false;
		}

		// Build the window info structure for the target window
#ifdef COBALT_RENDERER_WIN32_SUPPORT
		if (std::strcmp(driver, "windows") == 0)
		{
			auto hinstance = (HINSTANCE)SDL_GetPointerProperty(windowProperties, SDL_PROP_WINDOW_WIN32_INSTANCE_POINTER, nullptr);
			auto hwnd = (HWND)SDL_GetPointerProperty(windowProperties, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
			if ((hinstance == nullptr) || (hwnd == nullptr))
			{
				log->Error("SDL window is missing Win32 window properties.");
				return false;
			}
			win32Info.reset(new WindowInfoWin32(hwnd, hinstance, windowSize));
			type = cobalt::graphics::IFrameBuffer::WindowInfoBase::WindowType::Win32;
		}
		else
#endif
#ifdef COBALT_RENDERER_XLIB_SUPPORT
		  if (std::strcmp(driver, "x11") == 0)
		{
			auto display = (::Display*)SDL_GetPointerProperty(windowProperties, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
			auto window = (::Window)SDL_GetNumberProperty(windowProperties, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
			if ((display == nullptr) || (window == 0))
			{
				log->Error("SDL window is missing X11 window properties.");
				return false;
			}
			xlibInfo.reset(new WindowInfoXlib(display, window, windowSize));
			type = cobalt::graphics::IFrameBuffer::WindowInfoBase::WindowType::Xlib;
		}
		else
#endif
#ifdef COBALT_RENDERER_XCB_SUPPORT
		  if (std::strcmp(driver, "x11") == 0)
		{
			auto display = (::Display*)SDL_GetPointerProperty(windowProperties, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
			auto window = (::Window)SDL_GetNumberProperty(windowProperties, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
			if ((display == nullptr) || (window == 0))
			{
				log->Error("SDL window is missing XCB window properties.");
				return false;
			}
			auto connection = XGetXCBConnection(display);
			if (connection == nullptr)
			{
				log->Error("Failed to retrieve XCB connection details from X11 display.");
				return false;
			}
			xcbInfo.reset(new WindowInfoXCB(connection, (xcb_window_t)window, windowSize));
			type = cobalt::graphics::IFrameBuffer::WindowInfoBase::WindowType::XCB;
		}
		else
#endif
#ifdef COBALT_RENDERER_WAYLAND_SUPPORT
		  if (std::strcmp(driver, "wayland") == 0)
		{
			auto display = (wl_display*)SDL_GetPointerProperty(windowProperties, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr);
			auto surface = (wl_surface*)SDL_GetPointerProperty(windowProperties, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr);
			if ((display == nullptr) || (surface == nullptr))
			{
				log->Error("SDL window is missing Wayland window properties.");
				return false;
			}
			waylandInfo.reset(new WindowInfoWayland(display, surface, windowSize));
			type = cobalt::graphics::IFrameBuffer::WindowInfoBase::WindowType::Wayland;
		}
		else
#endif
#ifdef COBALT_RENDERER_APPKIT_SUPPORT
#ifndef __OBJC__
		  if (true)
		{
			log->Error("On macOS, WindowInfoSDL3::Initialize() must be called from a module compiled with Obj-C++ support.");
			return false;
		}
#else
		  if (std::strcmp(driver, "cocoa") == 0)
		{
			auto window = (NSWindow*)SDL_GetPointerProperty(windowProperties, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);
			if (window == nullptr)
			{
				log->Error("SDL window is missing AppKit window properties.");
				return false;
			}
			appKitInfo.reset(new WindowInfoAppKit(window.contentView, windowSize));
			type = cobalt::graphics::IFrameBuffer::WindowInfoBase::WindowType::AppKit;
		}
		else
#endif
#endif
		{
			log->Error("Unsupported SDL window system of type: {0}", driver);
			return false;
		}

		// Return true
		return true;
	}

	cobalt::graphics::IFrameBuffer::WindowInfoBase::WindowType GetType()
	{
		return type;
	}

	cobalt::graphics::IFrameBuffer::WindowInfoBase* Get()
	{
		switch (type)
		{
#ifdef COBALT_RENDERER_WIN32_SUPPORT
		case cobalt::graphics::IFrameBuffer::WindowInfoBase::WindowType::Win32:
			return win32Info.get();
#endif
#ifdef COBALT_RENDERER_XLIB_SUPPORT
		case cobalt::graphics::IFrameBuffer::WindowInfoBase::WindowType::Xlib:
			return xlibInfo.get();
#endif
#ifdef COBALT_RENDERER_XCB_SUPPORT
		case cobalt::graphics::IFrameBuffer::WindowInfoBase::WindowType::XCB:
			return xcbInfo.get();
#endif
#ifdef COBALT_RENDERER_WAYLAND_SUPPORT
		case cobalt::graphics::IFrameBuffer::WindowInfoBase::WindowType::Wayland:
			return waylandInfo.get();
#endif
#ifdef COBALT_RENDERER_APPKIT_SUPPORT
		case cobalt::graphics::IFrameBuffer::WindowInfoBase::WindowType::AppKit:
			return appKitInfo.get();
#endif
		}
		return nullptr;
	}

private:
	cobalt::graphics::IFrameBuffer::WindowInfoBase::WindowType type = {};
#ifdef COBALT_RENDERER_WIN32_SUPPORT
	std::unique_ptr<WindowInfoWin32> win32Info;
#endif
#ifdef COBALT_RENDERER_XLIB_SUPPORT
	std::unique_ptr<WindowInfoXlib> xlibInfo;
#endif
#ifdef COBALT_RENDERER_XCB_SUPPORT
	std::unique_ptr<WindowInfoXCB> xcbInfo;
#endif
#ifdef COBALT_RENDERER_WAYLAND_SUPPORT
	std::unique_ptr<WindowInfoWayland> waylandInfo;
#endif
#ifdef COBALT_RENDERER_APPKIT_SUPPORT
	std::unique_ptr<WindowInfoAppKit> appKitInfo;
#endif
};

}} // namespace cobalt::graphics
#endif
