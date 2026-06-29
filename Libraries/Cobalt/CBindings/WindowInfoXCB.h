// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Macros.h"
#include "WindowInfoBase.h"
#include <stdint.h>

typedef struct Cobalt_WindowInfoXCB
{
	Cobalt_WindowInfoBase base;
	// xcb_connection_t* from x11
	void* connection;
	// xcb_window_t from x11
	uint32_t window;
} Cobalt_WindowInfoXCB;
