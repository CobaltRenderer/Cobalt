// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Macros.h"

typedef enum
{
	// Headless
	Cobalt_WindowSystemType_Headless = 0x10001,
	// Windows
	Cobalt_WindowSystemType_Win32 = 0x20001,
	// Linux
	Cobalt_WindowSystemType_Xlib = 0x30001,
	Cobalt_WindowSystemType_XCB = 0x30002,
	Cobalt_WindowSystemType_Wayland = 0x30003,
	// MacOS
	Cobalt_WindowSystemType_AppKit = 0x40001
} Cobalt_WindowSystemType;

typedef struct Cobalt_WindowSystemInfoBase
{
	Cobalt_WindowSystemType type;
} Cobalt_WindowSystemInfoBase;
