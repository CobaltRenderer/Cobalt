// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#ifdef COBALT_RENDERER_WAYLAND_SUPPORT
#include "IRenderer.h"
#include <wayland-client-core.h>
#include <wayland-client.h>
namespace cobalt { namespace graphics {

struct WindowSystemInfoWayland : public IRenderer::WindowSystemInfoBase
{
	explicit WindowSystemInfoWayland(wl_display* display)
	: display(display)
	{
		structureSizeInBytes = sizeof(*this);
		windowSystemType = WindowSystemType::Wayland;
	}
	wl_display* display;
};

}} // namespace cobalt::graphics
#endif
