// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Macros.h"
#include "WindowInfoBase.h"

typedef struct Cobalt_WindowInfoAppKit
{
	Cobalt_WindowInfoBase base;
	// NSView* from AppKit
	void* view;
} Cobalt_WindowInfoAppKit;
