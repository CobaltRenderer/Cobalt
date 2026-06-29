// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "IRenderer.h"
namespace cobalt { namespace graphics {

struct WindowSystemInfoHeadless : public IRenderer::WindowSystemInfoBase
{
	WindowSystemInfoHeadless()
	{
		structureSizeInBytes = sizeof(*this);
		windowSystemType = WindowSystemType::Headless;
	}
};

}} // namespace cobalt::graphics
