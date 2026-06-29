// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Macros.h"
#include "WindowSystemInfoBase.h"

typedef struct Cobalt_WindowSystemInfoWayland
{
	Cobalt_WindowSystemInfoBase base;
	// wl_display* from wayland-client
	void* display;
} Cobalt_WindowSystemInfoWayland;
