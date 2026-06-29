// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Macros.h"
#include "WindowInfoBase.h"

typedef struct Cobalt_WindowInfoWin32
{
	Cobalt_WindowInfoBase base;
	// HINSTANCE from win32
	void* instanceHandle;
	// HWND from win32
	void* windowHandle;
} Cobalt_WindowInfoWin32;
