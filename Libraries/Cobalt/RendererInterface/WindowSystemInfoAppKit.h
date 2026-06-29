// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#ifdef COBALT_RENDERER_APPKIT_SUPPORT
#include "IRenderer.h"
namespace cobalt { namespace graphics {

struct WindowSystemInfoAppKit : public IRenderer::WindowSystemInfoBase
{
	WindowSystemInfoAppKit()
	{
		structureSizeInBytes = sizeof(*this);
		windowSystemType = WindowSystemType::AppKit;
	}
};

}} // namespace cobalt::graphics
#endif
