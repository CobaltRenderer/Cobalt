// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Macros.h"
#include "WindowSystemInfoBase.h"

typedef struct Cobalt_WindowSystemInfoXlib
{
	Cobalt_WindowSystemInfoBase base;
	// Display* from x11
	void* display;
} Cobalt_WindowSystemInfoXlib;
