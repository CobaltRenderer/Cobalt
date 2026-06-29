// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#ifdef COBALT_RENDERER_WIN32_SUPPORT
#include "IRenderer.h"
#include "Win32Headers.h"
namespace cobalt { namespace graphics {

struct WindowSystemInfoWin32 : public IRenderer::WindowSystemInfoBase
{
	WindowSystemInfoWin32()
	{
		structureSizeInBytes = sizeof(*this);
		windowSystemType = WindowSystemType::Win32;
	}
};

}} // namespace cobalt::graphics
#endif
