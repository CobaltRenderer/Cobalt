// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Macros.h"
#include <stdint.h>

typedef enum
{
	// Headless
	Cobalt_WindowType_Headless = 0x10001,
	// Windows
	Cobalt_WindowType_Win32 = 0x20001,
	// Linux
	Cobalt_WindowType_Xlib = 0x30001,
	Cobalt_WindowType_XCB = 0x30002,
	Cobalt_WindowType_Wayland = 0x30003,
	// MacOS
	Cobalt_WindowType_AppKit = 0x40001
} Cobalt_WindowType;

typedef struct Cobalt_WindowInfoBase
{
	Cobalt_WindowType type;
	uint32_t windowSizeInPixels[2];
} Cobalt_WindowInfoBase;
