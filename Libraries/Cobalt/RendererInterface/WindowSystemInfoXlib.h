// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#ifdef COBALT_RENDERER_XLIB_SUPPORT
#include "IRenderer.h"
#include "XlibHeaders.h"
namespace cobalt { namespace graphics {

struct WindowSystemInfoXlib : public IRenderer::WindowSystemInfoBase
{
	explicit WindowSystemInfoXlib(::Display* display)
	: display(display)
	{
		structureSizeInBytes = sizeof(*this);
		windowSystemType = WindowSystemType::Xlib;
	}
	::Display* display = {};
};

}} // namespace cobalt::graphics
#endif
