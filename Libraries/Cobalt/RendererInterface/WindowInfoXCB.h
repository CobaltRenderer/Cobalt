// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#ifdef COBALT_RENDERER_XCB_SUPPORT
#include "IFrameBuffer.h"
#include <xcb/xcb.h>
namespace cobalt { namespace graphics {

struct WindowInfoXCB : public IFrameBuffer::WindowInfoBase
{
	WindowInfoXCB(xcb_connection_t* connection, xcb_window_t window, const V2UInt32& windowSizeInPixels)
	: connection(connection), window(window), windowSizeInPixels(windowSizeInPixels)
	{
		structureSizeInBytes = sizeof(*this);
		windowType = WindowType::XCB;
	}
	xcb_connection_t* connection;
	xcb_window_t window;
	V2UInt32 windowSizeInPixels;
};

}} // namespace cobalt::graphics
#endif
