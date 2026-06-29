// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#ifdef COBALT_RENDERER_WAYLAND_SUPPORT
#include "IFrameBuffer.h"
#include <wayland-client-core.h>
#include <wayland-client.h>
namespace cobalt { namespace graphics {

struct WindowInfoWayland : public IFrameBuffer::WindowInfoBase
{
	WindowInfoWayland(wl_display* display, wl_surface* surface, const V2UInt32& windowSizeInPixels)
	: display(display), surface(surface), windowSizeInPixels(windowSizeInPixels)
	{
		structureSizeInBytes = sizeof(*this);
		windowType = WindowType::Wayland;
	}
	wl_display* display;
	wl_surface* surface;
	V2UInt32 windowSizeInPixels;
};

}} // namespace cobalt::graphics
#endif
