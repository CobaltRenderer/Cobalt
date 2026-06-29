// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "FrameBuffer.h"
#include "PlatformBindings.pkg"
#include "Result.h"
#include <Cobalt/RendererInterface/PlatformBindings.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>

using namespace cobalt::graphics;

//----------------------------------------------------------------------------------------
// Binding methods
//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_FrameBuffer_BindTexture(Cobalt_FrameBuffer frameBuffer, Cobalt_TextureBuffer2D texture, Cobalt_AttachmentType type, size_t index)
{
	auto _this = reinterpret_cast<IFrameBuffer*>(frameBuffer);

	return _this->BindTexture(reinterpret_cast<ITextureBuffer2D*>(texture), (IFrameBuffer::AttachmentType)type, index) ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
void Cobalt_FrameBuffer_UnbindTexture(Cobalt_FrameBuffer frameBuffer, Cobalt_AttachmentType type, size_t index)
{
	auto _this = reinterpret_cast<IFrameBuffer*>(frameBuffer);

	return _this->UnbindTexture((IFrameBuffer::AttachmentType)type, index);
}

//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_FrameBuffer_BindMultiSamplingResolveTexture(Cobalt_FrameBuffer frameBuffer, Cobalt_TextureBuffer2D texture, Cobalt_AttachmentType type, size_t index)
{
	auto _this = reinterpret_cast<IFrameBuffer*>(frameBuffer);

	return _this->BindMultiSamplingResolveTexture(reinterpret_cast<ITextureBuffer2D*>(texture), (IFrameBuffer::AttachmentType)type, index) ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
void Cobalt_FrameBuffer_UnbindMultiSamplingResolveTexture(Cobalt_FrameBuffer frameBuffer, Cobalt_AttachmentType type, size_t index)
{
	auto _this = reinterpret_cast<IFrameBuffer*>(frameBuffer);

	return _this->UnbindMultiSamplingResolveTexture((IFrameBuffer::AttachmentType)type, index);
}

//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_FrameBuffer_BindWindow(Cobalt_FrameBuffer frameBuffer, const Cobalt_WindowInfoBase* windowInfo, Cobalt_WindowDepthStencilMode depthStencilMode, Cobalt_WindowColorSpaceMode colorSpaceMode, Cobalt_WindowBindingFlags bindingFlags)
{
	auto _this = reinterpret_cast<IFrameBuffer*>(frameBuffer);
	auto _depthStencilMode = (IFrameBuffer::WindowDepthStencilMode)depthStencilMode;
	auto _colorSpaceMode = (IFrameBuffer::WindowColorSpaceMode)colorSpaceMode;
	auto _bindingFlags = (IFrameBuffer::WindowBindingFlags)bindingFlags;

	// Note that we can't just reinterpret cast the structure, as it's not guaranteed to be layout compatible due to
	// tail padding. While in reality it probably will be on every platform ever, we can't guarantee that from the
	// standard alone, even if the structure sizes match. We could however calculate the offset of the first member of
	// the derived class, and if those match, we could know the meaningful data is laid out the same way, possibly
	// apart from tail padding. That still won't help us though, as we have a structure size embedded that would need to
	// match, and even if that did match, we'd still be violating the strict aliasing rule, meaning we'd be in undefined
	// territory anyway. We need to unpack the C structure here and pack a new C++ one to be safe, which is fine,
	// because this operation is already heavyweight and infrequently called, and the tiny bit of work we do here
	// repacking things, while not optimal, isn't an issue in practice.
	bool result = false;
	V2UInt32 _size = V2UInt32(windowInfo->windowSizeInPixels[0], windowInfo->windowSizeInPixels[1]);
	switch (windowInfo->type)
	{
	case Cobalt_WindowType_Headless:
	{
		cobalt::graphics::WindowInfoHeadless _windowInfo(_size);
		result = _this->BindWindow(_windowInfo, _depthStencilMode, _colorSpaceMode, _bindingFlags);
		break;
	}
#ifdef COBALT_RENDERER_WIN32_SUPPORT
	case Cobalt_WindowType_Win32:
	{
		auto windowInfoResolved = reinterpret_cast<const Cobalt_WindowInfoWin32*>(windowInfo);
		cobalt::graphics::WindowInfoWin32 _windowInfo(reinterpret_cast<HWND>(windowInfoResolved->windowHandle), reinterpret_cast<HINSTANCE>(windowInfoResolved->instanceHandle), _size);
		result = _this->BindWindow(_windowInfo, _depthStencilMode, _colorSpaceMode, _bindingFlags);
		break;
	}
#endif
#ifdef COBALT_RENDERER_XLIB_SUPPORT
	case Cobalt_WindowType_Xlib:
	{
		auto windowInfoResolved = reinterpret_cast<const Cobalt_WindowInfoXlib*>(windowInfo);
		cobalt::graphics::WindowInfoXlib _windowInfo(reinterpret_cast<Display*>(windowInfoResolved->display), static_cast<Window>(windowInfoResolved->window), _size);
		result = _this->BindWindow(_windowInfo, _depthStencilMode, _colorSpaceMode, _bindingFlags);
		break;
	}
#endif
#ifdef COBALT_RENDERER_XCB_SUPPORT
	case Cobalt_WindowType_XCB:
	{
		auto windowInfoResolved = reinterpret_cast<const Cobalt_WindowInfoXCB*>(windowInfo);
		cobalt::graphics::WindowInfoXCB _windowInfo(reinterpret_cast<xcb_connection_t*>(windowInfoResolved->connection), static_cast<xcb_window_t>(windowInfoResolved->window), _size);
		result = _this->BindWindow(_windowInfo, _depthStencilMode, _colorSpaceMode, _bindingFlags);
		break;
	}
#endif
#ifdef COBALT_RENDERER_WAYLAND_SUPPORT
	case Cobalt_WindowType_Wayland:
	{
		auto windowInfoResolved = reinterpret_cast<const Cobalt_WindowInfoWayland*>(windowInfo);
		cobalt::graphics::WindowInfoWayland _windowInfo(reinterpret_cast<wl_display*>(windowInfoResolved->display), reinterpret_cast<wl_surface*>(windowInfoResolved->surface), _size);
		result = _this->BindWindow(_windowInfo, _depthStencilMode, _colorSpaceMode, _bindingFlags);
		break;
	}
#endif
#ifdef COBALT_RENDERER_APPKIT_SUPPORT
	case Cobalt_WindowType_AppKit:
	{
		auto windowInfoResolved = reinterpret_cast<const Cobalt_WindowInfoAppKit*>(windowInfo);
		cobalt::graphics::WindowInfoAppKit _windowInfo(reinterpret_cast<NSView*>(windowInfoResolved->view), _size);
		result = _this->BindWindow(_windowInfo, _depthStencilMode, _colorSpaceMode, _bindingFlags);
		break;
	}
#endif
	}
	return result ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_FrameBuffer_NotifyWindowResized(Cobalt_FrameBuffer frameBuffer, const uint32_t windowSizeInPixels[2])
{
	auto _this = reinterpret_cast<IFrameBuffer*>(frameBuffer);

	bool result = _this->NotifyWindowResized(V2UInt32(windowSizeInPixels[0], windowSizeInPixels[1]));
	return result ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
// Viewport methods
//----------------------------------------------------------------------------------------
void Cobalt_FrameBuffer_DefineViewportRegion(Cobalt_FrameBuffer frameBuffer, const uint32_t startPos[2], const uint32_t size[2])
{
	auto _this = reinterpret_cast<IFrameBuffer*>(frameBuffer);

	_this->DefineViewportRegion(V2UInt32(startPos[0], startPos[1]), V2UInt32(size[0], size[1]));
}

//----------------------------------------------------------------------------------------
void Cobalt_FrameBuffer_DefineScissorRegion(Cobalt_FrameBuffer frameBuffer, const uint32_t startPos[2], const uint32_t size[2])
{
	auto _this = reinterpret_cast<IFrameBuffer*>(frameBuffer);

	_this->DefineScissorRegion(V2UInt32(startPos[0], startPos[1]), V2UInt32(size[0], size[1]));
}

//----------------------------------------------------------------------------------------
void Cobalt_FrameBuffer_RemoveScissorRegion(Cobalt_FrameBuffer frameBuffer)
{
	auto _this = reinterpret_cast<IFrameBuffer*>(frameBuffer);

	_this->RemoveScissorRegion();
}

//----------------------------------------------------------------------------------------
// Output capture methods
//----------------------------------------------------------------------------------------
void Cobalt_FrameBuffer_AddOutputCaptureTarget(Cobalt_FrameBuffer frameBuffer, Cobalt_FrameBufferOutput captureTarget, Cobalt_AttachmentType type, size_t index)
{
	auto _this = reinterpret_cast<IFrameBuffer*>(frameBuffer);

	_this->AddOutputCaptureTarget(reinterpret_cast<IFrameBufferOutput*>(captureTarget), (IFrameBuffer::AttachmentType)type, index);
}

//----------------------------------------------------------------------------------------
void Cobalt_FrameBuffer_RemoveOutputCaptureTarget(Cobalt_FrameBuffer frameBuffer, Cobalt_FrameBufferOutput captureTarget)
{
	auto _this = reinterpret_cast<IFrameBuffer*>(frameBuffer);

	_this->RemoveOutputCaptureTarget(reinterpret_cast<IFrameBufferOutput*>(captureTarget));
}

//----------------------------------------------------------------------------------------
void Cobalt_FrameBuffer_Delete(Cobalt_FrameBuffer frameBuffer)
{
	auto _this = reinterpret_cast<IFrameBuffer*>(frameBuffer);

	_this->Delete();
}
