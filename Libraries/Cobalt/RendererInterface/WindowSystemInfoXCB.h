// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#ifdef COBALT_RENDERER_XCB_SUPPORT
#include "IRenderer.h"
#include <xcb/xcb.h>
namespace cobalt { namespace graphics {

struct WindowSystemInfoXCB : public IRenderer::WindowSystemInfoBase
{
	explicit WindowSystemInfoXCB(xcb_connection_t* connection)
	: connection(connection)
	{
		structureSizeInBytes = sizeof(*this);
		windowSystemType = WindowSystemType::XCB;
	}
	xcb_connection_t* connection;
};

}} // namespace cobalt::graphics
#endif
