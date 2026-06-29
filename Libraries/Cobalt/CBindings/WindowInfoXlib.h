// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Macros.h"
#include "WindowInfoBase.h"
#include <stdint.h>

typedef struct Cobalt_WindowInfoXlib
{
	Cobalt_WindowInfoBase base;
	// Display* from x11
	void* display;
	// Window from x11
	uint64_t window;
} Cobalt_WindowInfoXlib;
