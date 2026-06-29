// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Macros.h"
#include "WindowSystemInfoBase.h"

typedef struct Cobalt_WindowSystemInfoXCB
{
	Cobalt_WindowSystemInfoBase base;
	// xcb_connection_t* from x11
	void* connection;
} Cobalt_WindowSystemInfoXCB;
