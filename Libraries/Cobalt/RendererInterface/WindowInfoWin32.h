// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#ifdef COBALT_RENDERER_WIN32_SUPPORT
#include "IFrameBuffer.h"
#include "Win32Headers.h"
namespace cobalt { namespace graphics {

struct WindowInfoWin32 : public IFrameBuffer::WindowInfoBase
{
	WindowInfoWin32(HWND windowHandle, const V2UInt32& windowSizeInPixels)
	: windowHandle(windowHandle), instanceHandle(nullptr), windowSizeInPixels(windowSizeInPixels)
	{
		structureSizeInBytes = sizeof(*this);
		windowType = WindowType::Win32;
	}
	WindowInfoWin32(HWND windowHandle, HINSTANCE instanceHandle, const V2UInt32& windowSizeInPixels)
	: windowHandle(windowHandle), instanceHandle(instanceHandle), windowSizeInPixels(windowSizeInPixels)
	{
		structureSizeInBytes = sizeof(*this);
		windowType = WindowType::Win32;
	}
	HINSTANCE instanceHandle;
	HWND windowHandle;
	V2UInt32 windowSizeInPixels;
};

}} // namespace cobalt::graphics
#endif
