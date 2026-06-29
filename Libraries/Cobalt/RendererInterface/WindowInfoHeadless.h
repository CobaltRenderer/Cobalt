// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "IFrameBuffer.h"
namespace cobalt { namespace graphics {

struct WindowInfoHeadless : public IFrameBuffer::WindowInfoBase
{
	explicit WindowInfoHeadless(const V2UInt32& windowSizeInPixels)
	: windowSizeInPixels(windowSizeInPixels)
	{
		structureSizeInBytes = sizeof(*this);
		windowType = WindowType::Headless;
	}
	V2UInt32 windowSizeInPixels;
};

}} // namespace cobalt::graphics
