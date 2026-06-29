// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#ifdef COBALT_RENDERER_APPKIT_SUPPORT
#include "IFrameBuffer.h"
#include <OpenGL/CGLTypes.h>
#ifdef __OBJC__
#import <AppKit/AppKit.h>
#else
struct NSView;
#endif
namespace cobalt { namespace graphics {

struct WindowInfoAppKit : public IFrameBuffer::WindowInfoBase
{
#ifdef __OBJC__
	WindowInfoAppKit(void* view, const V2UInt32& windowSizeInPixels)
#else
	WindowInfoAppKit(NSView* view, const V2UInt32& windowSizeInPixels)
#endif
	: view(view), windowSizeInPixels(windowSizeInPixels)
	{
		structureSizeInBytes = sizeof(*this);
		windowType = WindowType::AppKit;
	}
#ifdef __OBJC__
	void* view;
#else
	NSView* view;
#endif
	V2UInt32 windowSizeInPixels;
};

}} // namespace cobalt::graphics
#endif
