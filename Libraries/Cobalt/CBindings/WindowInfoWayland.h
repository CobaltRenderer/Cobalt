// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Macros.h"
#include "WindowInfoBase.h"

typedef struct Cobalt_WindowInfoWayland
{
	Cobalt_WindowInfoBase base;
	// wl_display* from wayland-client
	void* display;
	// wl_surface* from wayland-client
	void* surface;
} Cobalt_WindowInfoWayland;
