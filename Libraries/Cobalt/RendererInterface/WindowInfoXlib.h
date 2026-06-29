// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#ifdef COBALT_RENDERER_XLIB_SUPPORT
#include "IFrameBuffer.h"
#include "XlibHeaders.h"
namespace cobalt { namespace graphics {

struct WindowInfoXlib : public IFrameBuffer::WindowInfoBase
{
	WindowInfoXlib(::Display* display, ::Window window, const V2UInt32& windowSizeInPixels)
	: display(display), window(window), windowSizeInPixels(windowSizeInPixels)
	{
		structureSizeInBytes = sizeof(*this);
		windowType = WindowType::Xlib;
	}
	::Display* display;
	::Window window;
	V2UInt32 windowSizeInPixels;
};

}} // namespace cobalt::graphics
#endif
