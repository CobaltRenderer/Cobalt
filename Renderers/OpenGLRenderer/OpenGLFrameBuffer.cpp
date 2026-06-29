// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "OpenGLFrameBuffer.h"
#include "OpenGLDebug.h"
#include "OpenGLFrameBufferOutput.h"
#include "OpenGLHeaders.h"
#include "OpenGLRenderer.h"
#include "OpenGLTextureBuffer2D.h"
#include <Cobalt/RendererInterface/PlatformBindings.pkg>
#include <Internal/RendererSupport/KnownDynamicCast.h>
#include <vector>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
OpenGLFrameBuffer::OpenGLFrameBuffer(cobalt::logging::ILogger* log, OpenGLRenderer* renderer)
: _log(log), _renderer(renderer)
{
	_buildState.viewportRegionStartPos = {};
	_buildState.viewportRegionSize = {};
	_buildState.framebufferInvalid = false;
	_buildState.boundToWindow = false;
	_buildState.headlessWindow = false;
	_buildState.viewportInvalid = false;
	_buildState.scissorRegionDefined = false;
	_framebufferCreated = false;
}

//----------------------------------------------------------------------------------------
OpenGLFrameBuffer::~OpenGLFrameBuffer()
{
	// Delete our owned framebuffer resources
	PerformPreDelete();

	// Delete any staging PBOs for framebuffer capture
	for (const auto& entry : _stagingBuffersForCapture)
	{
		glDeleteBuffers(1, &entry.pixelBufferObjectID);
	}
	if (_textureAttachmentWriteFlushPixelBufferID != 0)
	{
		glDeleteBuffers(1, &_textureAttachmentWriteFlushPixelBufferID);
	}
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void OpenGLFrameBuffer::Delete()
{
	_renderer->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
void OpenGLFrameBuffer::PerformPreDelete()
{
	if (_framebufferCreated)
	{
		DeleteFrameBuffer();
	}
#if defined(OPENGL_USE_EGL) && defined(COBALT_RENDERER_WAYLAND_SUPPORT)
	if (_buildState.eglWindowPending != nullptr)
	{
		wl_egl_window_destroy(_buildState.eglWindowPending);
		_buildState.eglWindowPending = nullptr;
	}
	if (_drawState.eglWindowPending != nullptr)
	{
		wl_egl_window_destroy(_drawState.eglWindowPending);
		_drawState.eglWindowPending = nullptr;
	}
#endif
}

//----------------------------------------------------------------------------------------
// Binding methods
//----------------------------------------------------------------------------------------
SuccessToken OpenGLFrameBuffer::BindTexture(ITextureBuffer2D* texture, AttachmentType type, size_t index)
{
	// Ensure the specified texture is able to be bound
	auto* textureResolved = KnownDynamicCast<OpenGLTextureBuffer2D*>(texture);
	if (((uint32_t)textureResolved->GetUsageFlags() & (uint32_t)ITextureBuffer::UsageFlags::FrameBufferOutput) == 0)
	{
		_log->Error("Attempted to bind texture to framebuffer when the usage flags for the texture don't allow framebuffer output.");
		return false;
	}

	// Add this texture to the list of bound textures for this framebuffer
	std::unique_lock<std::mutex> lock(_accessMutex);
	BoundTextureInfo textureInfo = {};
	textureInfo.type = type;
	textureInfo.index = index;
	textureInfo.texture = textureResolved;
	bool foundEntry = false;
	for (auto& entry : _buildState.boundTextures)
	{
		if ((entry.type == type) && (entry.index == index))
		{
			entry = textureInfo;
			foundEntry = true;
			break;
		}
	}
	if (!foundEntry)
	{
		_buildState.boundTextures.push_back(textureInfo);
	}

	// Update the framebuffer state, and mark it as modified.
	if (_buildState.boundToWindow)
	{
		_buildState.boundToWindow = false;
		_buildState.headlessWindow = false;
		_buildState.hasBoundDepthBuffer = false;
	}
	_buildState.hasBoundDepthBuffer |= (type == AttachmentType::Depth);
	_buildState.windowInfo = std::monostate();
	_buildState.framebufferInvalid = true;
	lock.unlock();
	FlagBuildStateModified();
	return true;
}

//----------------------------------------------------------------------------------------
void OpenGLFrameBuffer::UnbindTexture(AttachmentType type, size_t index)
{
	// Attempt to locate the target bound texture entry
	std::unique_lock<std::mutex> lock(_accessMutex);
	bool foundEntry = false;
	auto boundTexturesIterator = _buildState.boundTextures.begin();
	while (boundTexturesIterator != _buildState.boundTextures.end())
	{
		if ((boundTexturesIterator->type == type) && (boundTexturesIterator->index == index))
		{
			foundEntry = true;
			break;
		}
		++boundTexturesIterator;
	}
	if (!foundEntry)
	{
		return;
	}

	// Remove this texture from the list of bound textures for this framebuffer
	_buildState.boundTextures.erase(boundTexturesIterator);

	// Update the framebuffer state, and mark it as modified.
	_buildState.framebufferInvalid = true;
	if (type == AttachmentType::Depth)
	{
		_buildState.hasBoundDepthBuffer = false;
	}
	lock.unlock();
	FlagBuildStateModified();
}

//----------------------------------------------------------------------------------------
SuccessToken OpenGLFrameBuffer::BindMultiSamplingResolveTexture(ITextureBuffer2D* texture, AttachmentType type, size_t index)
{
	// Ensure the specified texture is able to be bound
	auto* textureResolved = KnownDynamicCast<OpenGLTextureBuffer2D*>(texture);
	if (((uint32_t)textureResolved->GetUsageFlags() & (uint32_t)ITextureBuffer::UsageFlags::MultiSampleResolve) == 0)
	{
		_log->Error("Attempted to bind resolve texture to framebuffer when the usage flags for the texture don't allow multisample texture resolution.");
		return false;
	}
	if (type != IFrameBuffer::AttachmentType::Color)
	{
		_log->Error("Attempted to bind resolve texture with type {0}, but only resolution of color targets are supported at this time.", type);
		return false;
	}
	if (textureResolved->GetSampleCount() != ITextureBuffer::SampleCount::SampleCount1)
	{
		_log->Error("Attempted to bind resolve texture with sample count {0}, but resolve textures must have a sample count of {1}.", textureResolved->GetSampleCount(), ITextureBuffer::SampleCount::SampleCount1);
		return false;
	}

	// Add this texture to the list of bound resolve textures for this framebuffer
	std::unique_lock<std::mutex> lock(_accessMutex);
	BoundTextureInfo textureInfo{};
	textureInfo.type = type;
	textureInfo.index = index;
	textureInfo.texture = textureResolved;
	bool foundEntry = false;
	for (auto& entry : _buildState.resolveTextures)
	{
		if ((entry.type == type) && (entry.index == index))
		{
			entry = textureInfo;
			foundEntry = true;
			break;
		}
	}
	if (!foundEntry)
	{
		_buildState.resolveTextures.push_back(textureInfo);
	}

	// Update the framebuffer state, and mark it as modified.
	_buildState.framebufferInvalid = true;
	lock.unlock();
	FlagBuildStateModified();
	return true;
}

//----------------------------------------------------------------------------------------
void OpenGLFrameBuffer::UnbindMultiSamplingResolveTexture(AttachmentType type, size_t index)
{
	// Attempt to locate the target resolve texture entry
	std::unique_lock<std::mutex> lock(_accessMutex);
	bool foundEntry = false;
	auto resolveTexturesIterator = _buildState.resolveTextures.begin();
	while (resolveTexturesIterator != _buildState.resolveTextures.end())
	{
		if ((resolveTexturesIterator->type == type) && (resolveTexturesIterator->index == index))
		{
			foundEntry = true;
			break;
		}
		++resolveTexturesIterator;
	}
	if (!foundEntry)
	{
		return;
	}

	// Remove this texture from the list of resolve textures for this framebuffer
	_buildState.resolveTextures.erase(resolveTexturesIterator);

	// Update the framebuffer state, and mark it as modified.
	_buildState.framebufferInvalid = true;
	lock.unlock();
	FlagBuildStateModified();
}

//----------------------------------------------------------------------------------------
SuccessToken OpenGLFrameBuffer::BindWindow(const WindowInfoBase& windowInfo, WindowDepthStencilMode depthStencilMode, WindowColorSpaceMode colorSpaceMode, WindowBindingFlags bindingFlags)
{
	// Update the framebuffer state, and mark it as modified.
	std::unique_lock<std::mutex> lock(_accessMutex);
	if ((windowInfo.windowType == IFrameBuffer::WindowInfoBase::WindowType::Headless) && (windowInfo.structureSizeInBytes == sizeof(WindowInfoHeadless)))
	{
		auto windowInfoResolved = reinterpret_cast<const WindowInfoHeadless*>(&windowInfo);
		_buildState.windowInfo = *windowInfoResolved;
		_buildState.windowSizeInPixels = windowInfoResolved->windowSizeInPixels;
		_buildState.headlessWindow = true;
	}
	else
#ifdef COBALT_RENDERER_WIN32_SUPPORT
	  if ((windowInfo.windowType == IFrameBuffer::WindowInfoBase::WindowType::Win32) && (windowInfo.structureSizeInBytes == sizeof(WindowInfoWin32)))
	{
		auto windowInfoResolved = reinterpret_cast<const WindowInfoWin32*>(&windowInfo);
		_buildState.windowInfo = *windowInfoResolved;
		_buildState.windowSizeInPixels = windowInfoResolved->windowSizeInPixels;
		_buildState.headlessWindow = false;
	}
	else
#endif
#ifdef OPENGL_USE_EGL
#ifdef COBALT_RENDERER_XLIB_SUPPORT
	  if ((windowInfo.windowType == IFrameBuffer::WindowInfoBase::WindowType::Xlib) && (windowInfo.structureSizeInBytes == sizeof(WindowInfoXlib)))
	{
		auto windowInfoResolved = reinterpret_cast<const WindowInfoXlib*>(&windowInfo);
		_buildState.windowInfo = *windowInfoResolved;
		_buildState.windowSizeInPixels = windowInfoResolved->windowSizeInPixels;
		_buildState.headlessWindow = false;
	}
	else
#endif
#ifdef COBALT_RENDERER_XCB_SUPPORT
	  if ((windowInfo.windowType == IFrameBuffer::WindowInfoBase::WindowType::XCB) && (windowInfo.structureSizeInBytes == sizeof(WindowInfoXCB)))
	{
		auto windowInfoResolved = reinterpret_cast<const WindowInfoXCB*>(&windowInfo);
		_buildState.windowInfo = *windowInfoResolved;
		_buildState.windowSizeInPixels = windowInfoResolved->windowSizeInPixels;
		_buildState.headlessWindow = false;
	}
	else
#endif
#ifdef COBALT_RENDERER_WAYLAND_SUPPORT
	  if ((windowInfo.windowType == IFrameBuffer::WindowInfoBase::WindowType::Wayland) && (windowInfo.structureSizeInBytes == sizeof(WindowInfoWayland)))
	{
		// On Linux under Wayland through EGL, we need to create a corresponding wl_egl_window for the target surface
		// on the UI thread, or at least a thread which has appropriate locking/synchronization in place according to
		// Wayland specs. We can't provide those guarantees from our render thread, so we need to create the
		// wl_egl_window here so that the caller can provide that guarantee for us. Once we have the wl_egl_window
		// object, we can do everything from our background thread. It may be expected that we'd need to call the
		// wl_egl_window_resize function from the UI thread too, like how resize events are handled on AppKit for
		// macOS, but in fact that requirement is reversed on Wayland under Linux, and we MUST call wl_egl_window_resize
		// from the render thread, with the bound context for that window currently active, in order to dodge driver
		// issues. Official documentation is lacking about threading concerns, but see the following references:
		// https://github.com/NVIDIA/egl-wayland2
		// https://github.com/libsdl-org/SDL/pull/4821
		auto windowInfoResolved = reinterpret_cast<const WindowInfoWayland*>(&windowInfo);
		_buildState.windowInfo = *windowInfoResolved;
		_buildState.windowSizeInPixels = windowInfoResolved->windowSizeInPixels;
		_buildState.headlessWindow = false;

		// If we already had a window binding operation pending, clear any resources associated with it.
		if (_buildState.eglWindowPending != nullptr)
		{
			wl_egl_window_destroy(_buildState.eglWindowPending);
			_buildState.eglWindowPending = nullptr;
		}

		// Create the wl_egl_window wrapper
		_buildState.eglWindowPending = wl_egl_window_create(windowInfoResolved->surface, (int)windowInfoResolved->windowSizeInPixels.X(), (int)windowInfoResolved->windowSizeInPixels.Y());
		if (_buildState.eglWindowPending == nullptr)
		{
			_log->Error("wl_egl_window_create failed when binding window");
			return false;
		}
	}
	else
#endif
#elif defined(COBALT_RENDERER_XLIB_SUPPORT)
	  if ((windowInfo.windowType == IFrameBuffer::WindowInfoBase::WindowType::Xlib) && (windowInfo.structureSizeInBytes == sizeof(WindowInfoXlib)))
	{
		auto windowInfoResolved = reinterpret_cast<const WindowInfoXlib*>(&windowInfo);
		_buildState.windowInfo = *windowInfoResolved;
		_buildState.windowSizeInPixels = windowInfoResolved->windowSizeInPixels;
		_buildState.headlessWindow = false;
	}
	else
#endif
#ifdef COBALT_RENDERER_APPKIT_SUPPORT
	  if ((windowInfo.windowType == IFrameBuffer::WindowInfoBase::WindowType::AppKit) && (windowInfo.structureSizeInBytes == sizeof(WindowInfoAppKit)))
	{
		// On macOS under AppKit, we need to create an NSOpenGLContext object to manage the connection between OpenGL
		// rendering and the NSView window surface. We also need to ensure that the NSOpenGLContext object is created on
		// the UI thread. To ensure this is the case, we need to create the rendering context and NSOpenGLContext object
		// here in the context of the calling thread.
		auto windowInfoResolved = reinterpret_cast<const WindowInfoAppKit*>(&windowInfo);
		_buildState.windowInfo = *windowInfoResolved;
		_buildState.windowSizeInPixels = windowInfoResolved->windowSizeInPixels;
		_buildState.headlessWindow = false;

		// Select the pixel format for the target window
		if (!_renderer->SelectPixelFormat(colorSpaceMode, depthStencilMode, _buildState.pixelFormatInfoPending))
		{
			_log->Error("SelectPixelFormat failed when binding window");
			return false;
		}

		// Retrieve a rendering context for our window, but don't make it current.
		if (!_renderer->RetrieveOrCreateRenderingContextForWindow((__bridge NSView*)windowInfoResolved->view, _buildState.pixelFormatInfoPending, _buildState.renderingContextPending, _buildState.renderingContextIndexPending, false))
		{
			_log->Error("RetrieveOrCreateRenderingContextForWindow failed when binding window");
			return false;
		}

		// Create the NSOpenGLContext object to bind our rendering context to the target window surface. Note that this
		// critical step must be run on the UI thread.
		@autoreleasepool
		{
			NSOpenGLContext* ctx = [[NSOpenGLContext alloc] initWithCGLContextObj:_buildState.renderingContextPending];
			// Yes, setView was declared deprecated in favor of NSOpenGLView, but the entire OpenGL API was deprecated
			// shortly afterwards, so it's a moot point. We use NSOpenGLContext instead of the NSOpenGLView wrapper
			// because we want to keep context creation centralized and decoupled from the window system inside the
			// renderer.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
			[ctx setView:(__bridge NSView*)windowInfoResolved->view];
#pragma clang diagnostic pop
			[ctx update];

			// Store the NSOpenGLContext as the pending object to apply later, removing the existing pending object if
			// present.
			if (_buildState.nsOpenGLContextPending != nullptr)
			{
				@autoreleasepool
				{
					CFRelease(_buildState.nsOpenGLContextPending);
				}
				_buildState.nsOpenGLContextPending = nullptr;
			}
			_buildState.nsOpenGLContextPending = (const void*)CFRetain((__bridge CFTypeRef)ctx);
		}
	}
	else
#endif
	{
		_log->Error("Could not bind to window. Unsupported window binding structure supplied with type \"{0}\" and size \"{1}\".", windowInfo.windowType, windowInfo.structureSizeInBytes);
		return false;
	}
	_buildState.windowDepthStencilMode = depthStencilMode;
	_buildState.windowColorSpaceMode = colorSpaceMode;
	_buildState.windowBindingFlags = bindingFlags;
	_buildState.boundTextures.clear();
	_buildState.boundToWindow = true;
	_buildState.hasBoundDepthBuffer = (depthStencilMode != WindowDepthStencilMode::None);
	_buildState.framebufferInvalid = true;
	_buildState.viewportInvalid = true;
	lock.unlock();
	FlagBuildStateModified();
	return true;
}

//----------------------------------------------------------------------------------------
SuccessToken OpenGLFrameBuffer::NotifyWindowResized(const V2UInt32& windowSizeInPixels)
{
	// Ensure we're currently bound to a window
	std::unique_lock<std::mutex> lock(_accessMutex);
	if (!_buildState.boundToWindow)
	{
		_log->Warning("NotifyWindowResized called when the framebuffer is not currently bound to a window.");
		return false;
	}

	// Update the framebuffer state, and mark it as modified.
	if (std::holds_alternative<WindowInfoHeadless>(_buildState.windowInfo))
	{
		auto* windowInfo = std::get_if<WindowInfoHeadless>(&_buildState.windowInfo);
		windowInfo->windowSizeInPixels = windowSizeInPixels;
		_buildState.windowSizeInPixels = windowSizeInPixels;
	}
#ifdef COBALT_RENDERER_WIN32_SUPPORT
	if (std::holds_alternative<WindowInfoWin32>(_buildState.windowInfo))
	{
		auto* windowInfo = std::get_if<WindowInfoWin32>(&_buildState.windowInfo);
		windowInfo->windowSizeInPixels = windowSizeInPixels;
		_buildState.windowSizeInPixels = windowSizeInPixels;
	}
#endif
#ifdef OPENGL_USE_EGL
#ifdef COBALT_RENDERER_XLIB_SUPPORT
	if (std::holds_alternative<WindowInfoXlib>(_buildState.windowInfo))
	{
		auto* windowInfo = std::get_if<WindowInfoXlib>(&_buildState.windowInfo);
		windowInfo->windowSizeInPixels = windowSizeInPixels;
		_buildState.windowSizeInPixels = windowSizeInPixels;
	}
#endif
#ifdef COBALT_RENDERER_XCB_SUPPORT
	if (std::holds_alternative<WindowInfoXCB>(_buildState.windowInfo))
	{
		auto* windowInfo = std::get_if<WindowInfoXCB>(&_buildState.windowInfo);
		windowInfo->windowSizeInPixels = windowSizeInPixels;
		_buildState.windowSizeInPixels = windowSizeInPixels;
	}
#endif
#ifdef COBALT_RENDERER_WAYLAND_SUPPORT
	if (std::holds_alternative<WindowInfoWayland>(_buildState.windowInfo))
	{
		auto* windowInfo = std::get_if<WindowInfoWayland>(&_buildState.windowInfo);
		windowInfo->windowSizeInPixels = windowSizeInPixels;
		_buildState.windowSizeInPixels = windowSizeInPixels;
	}
#endif
#elif defined(COBALT_RENDERER_XLIB_SUPPORT)
	if (std::holds_alternative<WindowInfoXlib>(_buildState.windowInfo))
	{
		auto* windowInfo = std::get_if<WindowInfoXlib>(&_buildState.windowInfo);
		windowInfo->windowSizeInPixels = windowSizeInPixels;
		_buildState.windowSizeInPixels = windowSizeInPixels;
	}
#endif
#ifdef COBALT_RENDERER_APPKIT_SUPPORT
	if (std::holds_alternative<WindowInfoAppKit>(_buildState.windowInfo))
	{
		auto* windowInfo = std::get_if<WindowInfoAppKit>(&_buildState.windowInfo);
		windowInfo->windowSizeInPixels = windowSizeInPixels;
		_buildState.windowSizeInPixels = windowSizeInPixels;
		std::scoped_lock<std::mutex> nsOpenGLContextLock(_nsOpenGLContextMigrationMutex);
		auto* ctx = (__bridge NSOpenGLContext*)_buildState.nsOpenGLContextPending;
		if (ctx == nil)
		{
			ctx = (__bridge NSOpenGLContext*)_drawState.nsOpenGLContextPending;
		}
		if (ctx == nil)
		{
			ctx = (__bridge NSOpenGLContext*)_nsOpenGLContext;
		}
		if (ctx != nil)
		{
			[ctx update];
		}
	}
#endif
	_buildState.viewportInvalid = true;
	lock.unlock();
	FlagBuildStateModified();
	return true;
}

//----------------------------------------------------------------------------------------
#ifdef OPENGL_USE_PLATFORM_WIN32
void OpenGLFrameBuffer::BindFrameBuffer(HGLRC mainRenderingContext)
#elif defined(OPENGL_USE_EGL)
void OpenGLFrameBuffer::BindFrameBuffer(EGLContext mainRenderingContext)
#elif defined(OPENGL_USE_PLATFORM_XLIB)
void OpenGLFrameBuffer::BindFrameBuffer(GLXContext mainRenderingContext)
#elif defined(OPENGL_USE_PLATFORM_APPKIT)
void OpenGLFrameBuffer::BindFrameBuffer(CGLContextObj mainRenderingContext)
#endif
{
	// If the native objects for this framebuffer have previously been created but are no longer valid, release them
	// now.
	CheckGLError(_log);
	if (_framebufferCreated && _drawState.framebufferInvalid)
	{
		DeleteFrameBuffer();
	}

	// If we need to create or update our native framebuffer objects, do it now, and make the context current.
	if (!_framebufferCreated)
	{
		// Note that this will implicitly make the context current if we're bound to a window
		CreateFrameBuffer(mainRenderingContext);
	}
	else if (_drawState.boundToWindow && !_drawState.headlessWindow)
	{
		// Make the rendering context for the bound window current
#ifdef OPENGL_USE_PLATFORM_WIN32
		HWND framebufferWindowHandle;
		HDC framebufferDeviceContext;
		HGLRC framebufferRenderingContext;
		GetRenderingContext(framebufferWindowHandle, framebufferDeviceContext, framebufferRenderingContext);
		_renderer->ActivateRenderingContext(framebufferWindowHandle, framebufferDeviceContext, framebufferRenderingContext);
#elif defined(OPENGL_USE_EGL)
		EGLDisplay framebufferDisplay;
		EGLSurface framebufferSurface;
		EGLContext framebufferRenderingContext;
		GetRenderingContext(framebufferDisplay, framebufferSurface, framebufferRenderingContext);
		_renderer->ActivateRenderingContext(framebufferDisplay, framebufferSurface, framebufferRenderingContext);
#elif defined(OPENGL_USE_PLATFORM_XLIB)
		::Display* framebufferDisplay;
		::Window framebufferWindow;
		GLXContext framebufferRenderingContext;
		GetRenderingContext(framebufferDisplay, framebufferWindow, framebufferRenderingContext);
		_renderer->ActivateRenderingContext(framebufferDisplay, framebufferWindow, framebufferRenderingContext);
#elif defined(OPENGL_USE_PLATFORM_APPKIT)
		CGLContextObj framebufferRenderingContext;
		GetRenderingContext(framebufferRenderingContext);
		_renderer->ActivateRenderingContext(framebufferRenderingContext);
#endif
	}

	// If we're using a wl_egl_window wrapper over a Wayland surface, update the window size now. Note that this MUST
	// be done from the render thread, not the UI thread, when the context attached to the surface is activated. Note
	// that this is exactly opposite the requirements for resize handling on macOS. The EGL spec is largely silent and
	// rather unhelpful about calling thread requirements, but there are practical limits with implementations in the
	// real world which require us to resize here. See the following for more info:
	// https://github.com/NVIDIA/egl-wayland2
	// https://github.com/libsdl-org/SDL/pull/4821
#if defined(OPENGL_USE_EGL) && defined(COBALT_RENDERER_WAYLAND_SUPPORT)
	if (_drawState.boundToWindow && (_eglWindow != nullptr))
	{
		wl_egl_window_resize(_eglWindow, (int)_drawState.windowSizeInPixels.X(), (int)_drawState.windowSizeInPixels.Y(), 0, 0);
	}
#endif

	// Bind our render targets for this framebuffer
	CheckGLError(_log);
	if (_drawState.boundToWindow && !_drawState.headlessWindow)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	else
	{
		glBindFramebuffer(GL_FRAMEBUFFER, _frameBufferID);
	}
	CheckGLError(_log);

	// Get the current surface size. Note that as the view coordinate space is flipped in OpenGL, with positioning done
	// from the bottom left rather than the top left, we need to flip the Y coordinate here to get the correct result.
	V2UInt32 surfaceSize = GetSurfaceSizeInPixels();

	// Setup the viewport
	glViewport((GLint)_drawState.viewportRegionStartPos.X(), (GLint)(surfaceSize.Y() - (_drawState.viewportRegionSize.Y() + _drawState.viewportRegionStartPos.Y())), (GLsizei)_drawState.viewportRegionSize.X(), (GLsizei)_drawState.viewportRegionSize.Y());

	// Setup the scissor region
	if (_drawState.scissorRegionDefined)
	{
		glEnable(GL_SCISSOR_TEST);
		glScissor((GLint)_drawState.scissorRegionStartPos.X(), (GLint)(surfaceSize.Y() - (_drawState.scissorRegionSize.Y() + _drawState.scissorRegionStartPos.Y())), (GLsizei)_drawState.scissorRegionSize.X(), (GLsizei)_drawState.scissorRegionSize.Y());
	}
	else
	{
		glDisable(GL_SCISSOR_TEST);
	}
	_drawState.viewportInvalid = false;
	CheckGLError(_log);
}

#ifdef OPENGL_USE_PLATFORM_WIN32
//----------------------------------------------------------------------------------------
void OpenGLFrameBuffer::GetRenderingContext(HWND& framebufferWindowHandle, HDC& framebufferDeviceContext, HGLRC& framebufferRenderingContext) const
{
	const auto* windowInfo = std::get_if<WindowInfoWin32>(&_drawState.windowInfo);
	framebufferWindowHandle = windowInfo->windowHandle;
	framebufferDeviceContext = _deviceContext;
	framebufferRenderingContext = _renderingContext;
}
#elif defined(OPENGL_USE_EGL)
//----------------------------------------------------------------------------------------
void OpenGLFrameBuffer::GetRenderingContext(EGLDisplay& framebufferDisplay, EGLSurface& framebufferSurface, EGLContext& framebufferRenderingContext) const
{
	const auto* windowInfo = std::get_if<WindowInfoXlib>(&_drawState.windowInfo);
	framebufferDisplay = _eglDisplay;
	framebufferSurface = _eglSurface;
	framebufferRenderingContext = _renderingContext;
}
#elif defined(OPENGL_USE_PLATFORM_XLIB)
//----------------------------------------------------------------------------------------
void OpenGLFrameBuffer::GetRenderingContext(::Display*& framebufferDisplay, ::Window& framebufferWindow, GLXContext& framebufferRenderingContext) const
{
	const auto* windowInfo = std::get_if<WindowInfoXlib>(&_drawState.windowInfo);
	framebufferDisplay = windowInfo->display;
	framebufferWindow = windowInfo->window;
	framebufferRenderingContext = _renderingContext;
}
#elif defined(OPENGL_USE_PLATFORM_APPKIT)
//----------------------------------------------------------------------------------------
void OpenGLFrameBuffer::GetRenderingContext(CGLContextObj& framebufferRenderingContext) const
{
	framebufferRenderingContext = _renderingContext;
}
#endif

//----------------------------------------------------------------------------------------
bool OpenGLFrameBuffer::IsBoundToWindow() const
{
	return _drawState.boundToWindow && !_drawState.headlessWindow;
}

//----------------------------------------------------------------------------------------
bool OpenGLFrameBuffer::HasBoundDepthBuffer() const
{
	return _drawState.hasBoundDepthBuffer;
}

//----------------------------------------------------------------------------------------
OpenGLFrameBuffer::AttachmentFormat OpenGLFrameBuffer::GetColorAttachmentFormat(size_t index) const
{
	// If we're bound to a window, only floating point and normalized targets are supported, so return the float type
	// immediately to the caller.
	if (_drawState.boundToWindow)
	{
		return AttachmentFormat::Float;
	}

	// Attempt to locate the target resolve texture entry
	std::unique_lock<std::mutex> lock(_accessMutex);
	bool foundEntry = false;
	auto boundTexturesIterator = _drawState.boundTextures.begin();
	while (boundTexturesIterator != _drawState.boundTextures.end())
	{
		if ((boundTexturesIterator->type == IFrameBuffer::AttachmentType::Color) && (boundTexturesIterator->index == index))
		{
			foundEntry = true;
			break;
		}
		++boundTexturesIterator;
	}
	if (!foundEntry)
	{
		return AttachmentFormat::Float;
	}
	const auto& entry = *boundTexturesIterator;

	// Return the basic format of the target texture
	auto dataFormat = entry.texture->AllocatedDataFormat();
	switch (dataFormat)
	{
	case ITextureBuffer::DataFormat::Int8:
	case ITextureBuffer::DataFormat::Int16:
	case ITextureBuffer::DataFormat::Int32:
		return AttachmentFormat::Int;
	case ITextureBuffer::DataFormat::UInt8:
	case ITextureBuffer::DataFormat::UInt16:
	case ITextureBuffer::DataFormat::UInt32:
		return AttachmentFormat::UInt;
	case ITextureBuffer::DataFormat::Norm8:
	case ITextureBuffer::DataFormat::Norm16:
	case ITextureBuffer::DataFormat::UNorm8:
	case ITextureBuffer::DataFormat::UNorm16:
	case ITextureBuffer::DataFormat::Float16:
	case ITextureBuffer::DataFormat::Float32:
		return AttachmentFormat::Float;
	}
	return AttachmentFormat::Float;
}

//----------------------------------------------------------------------------------------
ITextureBuffer::SampleCount OpenGLFrameBuffer::GetSampleCount() const
{
	return _sampleCount;
}

//----------------------------------------------------------------------------------------
size_t OpenGLFrameBuffer::GetWindowRenderingContextIndex() const
{
	return _renderingContextIndex;
}

//----------------------------------------------------------------------------------------
// Viewport methods
//----------------------------------------------------------------------------------------
void OpenGLFrameBuffer::DefineViewportRegion(const V2UInt32& startPos, const V2UInt32& size)
{
	// Update the framebuffer state, and mark it as modified.
	std::unique_lock<std::mutex> lock(_accessMutex);
	_buildState.viewportRegionStartPos = startPos;
	_buildState.viewportRegionSize = size;
	_buildState.viewportInvalid = true;
	lock.unlock();
	FlagBuildStateModified();
}

//----------------------------------------------------------------------------------------
void OpenGLFrameBuffer::DefineScissorRegion(const V2UInt32& startPos, const V2UInt32& size)
{
	// Update the framebuffer state, and mark it as modified.
	std::unique_lock<std::mutex> lock(_accessMutex);
	_buildState.scissorRegionDefined = true;
	_buildState.scissorRegionStartPos = startPos;
	_buildState.scissorRegionSize = size;
	_buildState.viewportInvalid = true;
	lock.unlock();
	FlagBuildStateModified();
}

//----------------------------------------------------------------------------------------
void OpenGLFrameBuffer::RemoveScissorRegion()
{
	// Update the framebuffer state, and mark it as modified.
	std::unique_lock<std::mutex> lock(_accessMutex);
	_buildState.scissorRegionDefined = false;
	_buildState.viewportInvalid = true;
	lock.unlock();
	FlagBuildStateModified();
}

//----------------------------------------------------------------------------------------
// Output capture methods
//----------------------------------------------------------------------------------------
bool OpenGLFrameBuffer::HasCaptureTargets() const
{
	return !_drawState.captureTargets.empty();
}

//----------------------------------------------------------------------------------------
void OpenGLFrameBuffer::AddOutputCaptureTarget(IFrameBufferOutput* captureTarget, AttachmentType type, size_t index)
{
	std::unique_lock<std::mutex> lock(_accessMutex);
	CaptureTargetInfo captureTargetInfo = {};
	captureTargetInfo.captureTarget = KnownDynamicCast<OpenGLFrameBufferOutput*>(captureTarget);
	captureTargetInfo.type = type;
	captureTargetInfo.index = index;
	_buildState.captureTargets.push_back(captureTargetInfo);
	lock.unlock();
	FlagBuildStateModified();
}

//----------------------------------------------------------------------------------------
void OpenGLFrameBuffer::RemoveOutputCaptureTarget(IFrameBufferOutput* captureTarget)
{
	std::unique_lock<std::mutex> lock(_accessMutex);
	auto* captureTargetResolved = KnownDynamicCast<OpenGLFrameBufferOutput*>(captureTarget);
	for (auto captureTargetsIterator = _buildState.captureTargets.begin(); captureTargetsIterator != _buildState.captureTargets.end(); ++captureTargetsIterator)
	{
		if (captureTargetsIterator->captureTarget == captureTargetResolved)
		{
			_buildState.captureTargets.erase(captureTargetsIterator);
			lock.unlock();
			FlagBuildStateModified();
			return;
		}
	}
}

//----------------------------------------------------------------------------------------
void OpenGLFrameBuffer::CaptureFrameBufferOutput(bool usePixelBufferObjects, bool captureFrontBuffer)
{
	// If there are no bound capture targets, abort any further processing.
	if (_drawState.captureTargets.empty())
	{
		return;
	}

	// Bind our render targets for this framebuffer
	if (_drawState.boundToWindow && !_drawState.headlessWindow)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glReadBuffer(captureFrontBuffer ? GL_FRONT : GL_BACK);
	}
	else
	{
		glBindFramebuffer(GL_FRAMEBUFFER, _frameBufferID);
	}
	CheckGLError(_log);

	// Store our framebuffer output in any capture targets that have been bound
	for (size_t captureTargetIndex = 0; captureTargetIndex < _drawState.captureTargets.size(); ++captureTargetIndex)
	{
		// Attempt to locate the bound texture for the target framebuffer output
		CaptureTargetInfo& captureTargetInfo = _drawState.captureTargets[captureTargetIndex];
		const OpenGLTextureBuffer2D* targetTexture = nullptr;
		bool foundTargetBuffer = false;
		if (_drawState.boundToWindow)
		{
			if (captureTargetInfo.index == 0)
			{
				switch (captureTargetInfo.type)
				{
				case IFrameBuffer::AttachmentType::Color:
					foundTargetBuffer = true;
					break;
				case IFrameBuffer::AttachmentType::Depth:
					foundTargetBuffer = (_drawState.windowDepthStencilMode != IFrameBuffer::WindowDepthStencilMode::None);
					break;
				case IFrameBuffer::AttachmentType::Stencil:
					foundTargetBuffer = (_drawState.windowDepthStencilMode == IFrameBuffer::WindowDepthStencilMode::DepthUNorm24StencilUInt8) || (_drawState.windowDepthStencilMode == IFrameBuffer::WindowDepthStencilMode::DepthFloat32StencilUInt8);
					break;
				}
			}
		}
		else
		{
			for (const BoundTextureInfo& boundTexture : _drawState.boundTextures)
			{
				if ((boundTexture.type == captureTargetInfo.type) && (boundTexture.index == captureTargetInfo.index))
				{
					foundTargetBuffer = true;
					targetTexture = boundTexture.texture;
					break;
				}
			}
		}

		// If we couldn't locate the target framebuffer output, log an error, and advance to the next capture entry.
		if (!foundTargetBuffer)
		{
			_log->Error("Failed to locate target framebuffer output for target type {0} and index {1}", captureTargetInfo.type, captureTargetInfo.index);
			continue;
		}

		// Bind the correct buffer attachment. Note that we restore it to GL_COLOR_ATTACHMENT0 for reading depth/stencil
		// buffer attachents, as while the documentation seems to suggest calls to glReadBuffer should have no effect on
		// reading depth/stencil attachments, in practice it is unreliable on some drivers unless we restore it to the
		// default value.
		if (_drawState.boundToWindow && !_drawState.headlessWindow)
		{
			glReadBuffer(captureFrontBuffer ? GL_FRONT : GL_BACK);
		}
		else
		{
			GLenum nativeType = GL_COLOR_ATTACHMENT0;
			if (captureTargetInfo.type == IFrameBuffer::AttachmentType::Color)
			{
				nativeType = GetNativeAttachmentType(captureTargetInfo.type, captureTargetInfo.index);
			}
			glReadBuffer(nativeType);
		}
		CheckGLError(_log);

		// Determine the image and data formats to request when retrieving the framebuffer contents
		ITextureBuffer2D::ImageFormat imageFormat = ITextureBuffer2D::ImageFormat::R;
		ITextureBuffer2D::DataFormat dataFormat = ITextureBuffer2D::DataFormat::UInt8;
		if (_drawState.boundToWindow)
		{
			if ((captureTargetInfo.type == IFrameBuffer::AttachmentType::Depth) || (captureTargetInfo.type == IFrameBuffer::AttachmentType::Stencil))
			{
				switch (_drawState.windowDepthStencilMode)
				{
				case IFrameBuffer::WindowDepthStencilMode::DepthUNorm16:
					imageFormat = ITextureBuffer2D::ImageFormat::Depth;
					dataFormat = ITextureBuffer2D::DataFormat::DepthUNorm16;
					break;
				case IFrameBuffer::WindowDepthStencilMode::DepthUNorm24:
					imageFormat = ITextureBuffer2D::ImageFormat::Depth;
					dataFormat = ITextureBuffer2D::DataFormat::DepthUNorm24;
					break;
				case IFrameBuffer::WindowDepthStencilMode::DepthUNorm24StencilUInt8:
					imageFormat = ITextureBuffer2D::ImageFormat::DepthAndStencil;
					dataFormat = ITextureBuffer2D::DataFormat::DepthUNorm24StencilUInt8;
					break;
				case IFrameBuffer::WindowDepthStencilMode::DepthFloat32:
					imageFormat = ITextureBuffer2D::ImageFormat::Depth;
					dataFormat = ITextureBuffer2D::DataFormat::DepthFloat32;
					break;
				case IFrameBuffer::WindowDepthStencilMode::DepthFloat32StencilUInt8:
					imageFormat = ITextureBuffer2D::ImageFormat::DepthAndStencil;
					dataFormat = ITextureBuffer2D::DataFormat::DepthFloat32StencilUInt8;
					break;
				}
			}
			else
			{
				imageFormat = ITextureBuffer2D::ImageFormat::RGBA;
				dataFormat = ITextureBuffer2D::DataFormat::UNorm8;
			}
		}
		else
		{
			imageFormat = targetTexture->AllocatedImageFormat();
			dataFormat = targetTexture->AllocatedDataFormat();
		}

		// Retrieve the requested capture settings from our framebuffer output object
		OpenGLFrameBufferOutput* captureTarget = captureTargetInfo.captureTarget;
		V2UInt32 requestedImageOffset = captureTarget->GetRequestedImageOffset();
		V2UInt32 requestedImageSize = captureTarget->GetRequestedImageSize();
		bool detachAfterCapture = captureTarget->IsDetachingAfterCapture();

		// Calculate the cropped size of the image, taking into account the requested image offset and size, if any.
		V2UInt32 surfaceSize = GetSurfaceSizeInPixels();
		uint32_t imageWidth = surfaceSize.X();
		uint32_t imageHeight = surfaceSize.Y();
		V2UInt32 croppedImageSize = OpenGLFrameBufferOutput::CalculateCroppedImageDimensions(V2UInt32(imageWidth, imageHeight), requestedImageOffset, requestedImageSize);

		// Calculate the source and destination formats to use
		ITextureBuffer::SourceImageFormat optimalSourceImageFormat;
		ITextureBuffer::SourceDataFormat optimalSourceDataFormat;
		GLenum nativeDataType;
		GLenum nativeDataFormat;
		ITextureBuffer2D::ImageFormat finalImageFormat;
		ITextureBuffer2D::DataFormat finalDataFormat;
		OpenGLTextureBuffer2D::GetOptimalSourceFormat(imageFormat, dataFormat, finalImageFormat, finalDataFormat, optimalSourceImageFormat, optimalSourceDataFormat, nativeDataFormat, nativeDataType, (captureTargetInfo.type == IFrameBuffer::AttachmentType::Stencil));

		// Calculate the required size of the buffer to hold the transferred image data
		size_t elementCount = ITextureBuffer2D::ElementCountPerPixelFromFormat(optimalSourceImageFormat);
		size_t elementSizeInBytes = ITextureBuffer2D::ByteSizePerElementFromFormat(optimalSourceDataFormat);
		size_t pixelStrideInBytes = elementSizeInBytes * elementCount;
		size_t rowStrideInBytes = croppedImageSize.X() * pixelStrideInBytes;
		size_t bufferSizeInBytes = (croppedImageSize.Y() * rowStrideInBytes);

		// Read the image data
		GLint imageOffsetY = imageHeight - (croppedImageSize.Y() + requestedImageOffset.Y());
		GLint originalPackAlignment;
		glGetIntegerv(GL_PACK_ALIGNMENT, &originalPackAlignment);
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		captureTargetInfo.completionPending = usePixelBufferObjects;
		if (usePixelBufferObjects)
		{
			// Retrieve the pixel buffer for this capture target, allocating one if it hasn't previously been created.
			if (captureTargetIndex >= _stagingBuffersForCapture.size())
			{
				_stagingBuffersForCapture.resize(captureTargetIndex + 1);
				glGenBuffers(1, &_stagingBuffersForCapture[captureTargetIndex].pixelBufferObjectID);
				_stagingBuffersForCapture[captureTargetIndex].bufferSizeInBytes = 0;
			}
			auto& stagingBufferInfo = _stagingBuffersForCapture[captureTargetIndex];

			// Bind the pixel buffer
			glBindBuffer(GL_PIXEL_PACK_BUFFER, stagingBufferInfo.pixelBufferObjectID);

			// If the buffer hasn't been allocated backing storage yet, or the required size has changed, perform a
			// buffer allocation.
			if (stagingBufferInfo.bufferSizeInBytes != bufferSizeInBytes)
			{
				glBufferData(GL_PIXEL_PACK_BUFFER, bufferSizeInBytes, nullptr, GL_STREAM_READ);
				stagingBufferInfo.bufferSizeInBytes = bufferSizeInBytes;
			}

			// Queue a requested read into our pixel buffer object
			glReadPixels(requestedImageOffset.X(), imageOffsetY, croppedImageSize.X(), croppedImageSize.Y(), nativeDataFormat, nativeDataType, nullptr);

			// Unbind the pixel buffer
			glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

			// If we're bound to a window, generate a fence on this rendering context so we can synchronize with the PBO
			// write on the main context before we attempt to read from it. Note that we also explicitly flush pending
			// commands here. This is technically unnecessary, as we'll get an implicit flush when this rendering
			// context is switched out, but in the interests of best practice, we're flushing here explicitly as we need
			// to wait on this fence from a different rendering context, and we need to make certain the command has
			// been dispatched to the GPU so that it can eventually be signalled.
			GLsync completionFence = nullptr;
			if (_drawState.boundToWindow)
			{
				completionFence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
				glFlush();
			}
			captureTargetInfo.completionFence = completionFence;

			// Store information on the capture format
			captureTargetInfo.optimalSourceImageFormat = optimalSourceImageFormat;
			captureTargetInfo.optimalSourceDataFormat = optimalSourceDataFormat;
			captureTargetInfo.finalImageFormat = finalImageFormat;
			captureTargetInfo.finalDataFormat = finalDataFormat;
			captureTargetInfo.croppedImageSize = croppedImageSize;
		}
		else
		{
			// Allocate a buffer to hold the image data. Note that we've observed buffer overruns with glReadPixels on
			// Intel integrated graphics under Windows, copying a few dozen extra 0's after the pixels requested. We add
			// padding to the buffer here to protect against this.
			size_t safetyPadding = 1024;
			std::vector<uint8_t> imageData(bufferSizeInBytes + safetyPadding);

			// Perform the read of the image data
			glReadPixels(requestedImageOffset.X(), imageOffsetY, croppedImageSize.X(), croppedImageSize.Y(), nativeDataFormat, nativeDataType, reinterpret_cast<void*>(imageData.data()));

			// Write the texture data into our capture target
			captureTarget->StoreCaptureData(croppedImageSize, imageData.data(), bufferSizeInBytes, finalImageFormat, finalDataFormat, optimalSourceImageFormat, optimalSourceDataFormat, (captureTargetInfo.type == IFrameBuffer::AttachmentType::Stencil));

			// Record this captured framebuffer output with the renderer
			_renderer->AddCurrentFrameBufferOutput(captureTarget);

			// Now that we've captured a frame, detach the output capture target if requested.
			if (detachAfterCapture)
			{
				RemoveOutputCaptureTarget(captureTarget);
			}
		}
		glPixelStorei(GL_PACK_ALIGNMENT, originalPackAlignment);

		// It's been observed that leaving glReadBuffer set to GL_FRONT can stop subsequent frames from drawing. We
		// revert it here in the case of reads from the window to prevent this issue occurring.
		if (captureFrontBuffer && _drawState.boundToWindow)
		{
			glReadBuffer(GL_BACK);
		}
		CheckGLError(_log);
	}
}

//----------------------------------------------------------------------------------------
void OpenGLFrameBuffer::CompleteCaptureFrameBufferOutput()
{
	for (size_t captureTargetIndex = 0; captureTargetIndex < _drawState.captureTargets.size(); ++captureTargetIndex)
	{
		// If this capture slot isn't active, skip it.
		CaptureTargetInfo& captureTargetInfo = _drawState.captureTargets[captureTargetIndex];
		if (!captureTargetInfo.completionPending)
		{
			continue;
		}
		captureTargetInfo.completionPending = false;

		// If we have to wait on a fence for the PBO write to complete, wait for it now, and clean up the fence.
		if (captureTargetInfo.completionFence != nullptr)
		{
			glClientWaitSync(captureTargetInfo.completionFence, GL_SYNC_FLUSH_COMMANDS_BIT, GL_TIMEOUT_IGNORED);
			glDeleteSync(captureTargetInfo.completionFence);
			captureTargetInfo.completionFence = nullptr;
		}

		// Map the pixel buffer object into client memory
		const auto& stagingBufferInfo = _stagingBuffersForCapture[captureTargetIndex];
		glBindBuffer(GL_PIXEL_PACK_BUFFER, stagingBufferInfo.pixelBufferObjectID);
		auto mappedBuffer = reinterpret_cast<unsigned char*>(glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, stagingBufferInfo.bufferSizeInBytes, GL_MAP_READ_BIT));
		if (mappedBuffer == nullptr)
		{
			_log->Error("Failed to map pixel buffer for framebuffer capture");
			glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
			continue;
		}

		// Write the texture data into our capture target
		OpenGLFrameBufferOutput* captureTarget = captureTargetInfo.captureTarget;
		captureTarget->StoreCaptureData(captureTargetInfo.croppedImageSize, mappedBuffer, stagingBufferInfo.bufferSizeInBytes, captureTargetInfo.finalImageFormat, captureTargetInfo.finalDataFormat, captureTargetInfo.optimalSourceImageFormat, captureTargetInfo.optimalSourceDataFormat, (captureTargetInfo.type == IFrameBuffer::AttachmentType::Stencil));

		// Unmap the buffer
		glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
		glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

		// Record this captured framebuffer output with the renderer
		_renderer->AddCurrentFrameBufferOutput(captureTarget);

		// Now that we've captured a frame, detach the output capture target if requested.
		bool detachAfterCapture = captureTarget->IsDetachingAfterCapture();
		if (detachAfterCapture)
		{
			RemoveOutputCaptureTarget(captureTarget);
		}
	}
}

//----------------------------------------------------------------------------------------
// Framebuffer update methods
//----------------------------------------------------------------------------------------
bool OpenGLFrameBuffer::PresentToWindow()
{
	// If this is a headless window, there's no presentation to do, so abort any further processing.
	if (_drawState.headlessWindow)
	{
		return true;
	}

	// Present our back buffer to the window
#ifdef OPENGL_USE_PLATFORM_WIN32
	if (_deviceContext != nullptr)
	{
		// Set the VSync/tearing swap state for the window
		static auto wglSwapIntervalEXT = reinterpret_cast<PFNWGLSWAPINTERVALEXTPROC>(wglGetProcAddress("wglSwapIntervalEXT"));
		static auto tearingSupported = _renderer->OpenGLExtensionPresent("WGL_EXT_swap_control_tear");
		if (wglSwapIntervalEXT != nullptr)
		{
			if ((_drawState.windowBindingFlags & WindowBindingFlags::LimitSwapToVSync) != WindowBindingFlags::None)
			{
				// Limit max framerate to vsync, optionally allowing tearing if we're lagging behind.
				wglSwapIntervalEXT((tearingSupported && ((_drawState.windowBindingFlags & WindowBindingFlags::AllowTearing) != WindowBindingFlags::None)) ? -1 : 1);
			}
			else
			{
				// Vsync off if tearing permitted and supported, otherwise leave it enabled.
				wglSwapIntervalEXT((tearingSupported && ((_drawState.windowBindingFlags & WindowBindingFlags::AllowTearing) != WindowBindingFlags::None)) ? 0 : 1);
			}
		}

		// Swap the buffers
		if (SwapBuffers(_deviceContext) != TRUE)
		{
			_log->Error("SwapBuffers failed.");
			return false;
		}
	}
#elif defined(OPENGL_USE_EGL)
	if (_eglSurface != EGL_NO_SURFACE)
	{
		// Enable vsync if specifically requested or tearing not permitted
		bool vsyncEnabled = ((_drawState.windowBindingFlags & WindowBindingFlags::LimitSwapToVSync) != WindowBindingFlags::None) || ((_drawState.windowBindingFlags & WindowBindingFlags::AllowTearing) == WindowBindingFlags::None);
		if (eglSwapInterval(_eglDisplay, (vsyncEnabled ? 1 : 0)) != EGL_TRUE)
		{
			_log->Warning("eglSwapInterval failed.");
		}

		// Swap the buffers
		if (eglSwapBuffers(_eglDisplay, _eglSurface) != EGL_TRUE)
		{
			_log->Error("eglSwapBuffers failed.");
			return false;
		}
	}
#elif defined(OPENGL_USE_PLATFORM_XLIB)
	if (std::holds_alternative<WindowInfoXlib>(_drawState.windowInfo))
	{
		// Set the VSync/tearing swap state for the window
		const auto* windowInfo = std::get_if<WindowInfoXlib>(&_drawState.windowInfo);
		// clang-format off
		typedef void (*PFNGLXSWAPINTERVALEXTPROC)(::Display* dpy, GLXDrawable drawable, int interval); // NOLINT
		// clang-format on
		typedef int (*PFNGLXSWAPINTERVALMESAPROC)(unsigned int interval);
		static auto glXSwapIntervalEXT = reinterpret_cast<PFNGLXSWAPINTERVALEXTPROC>(glXGetProcAddress(reinterpret_cast<const GLubyte*>("glXSwapIntervalEXT")));
		static auto glXSwapIntervalMESA = reinterpret_cast<PFNGLXSWAPINTERVALMESAPROC>(glXGetProcAddress(reinterpret_cast<const GLubyte*>("glXSwapIntervalMESA")));
		static auto tearingSupported = _renderer->OpenGLExtensionPresent("GLX_EXT_swap_control_tear");
		if (glXSwapIntervalEXT != nullptr)
		{
			if ((_drawState.windowBindingFlags & WindowBindingFlags::LimitSwapToVSync) != WindowBindingFlags::None)
			{
				// Limit max framerate to vsync, optionally allowing tearing if we're lagging behind.
				glXSwapIntervalEXT(windowInfo->display, windowInfo->window, (tearingSupported && ((_drawState.windowBindingFlags & WindowBindingFlags::AllowTearing) != WindowBindingFlags::None)) ? -1 : 1);
			}
			else
			{
				// Vsync off if tearing permitted and supported, otherwise leave it enabled.
				glXSwapIntervalEXT(windowInfo->display, windowInfo->window, (tearingSupported && ((_drawState.windowBindingFlags & WindowBindingFlags::AllowTearing) != WindowBindingFlags::None)) ? 0 : 1);
			}
		}
		else if (glXSwapIntervalMESA != nullptr)
		{
			glXSwapIntervalMESA(((_drawState.windowBindingFlags & WindowBindingFlags::LimitSwapToVSync) != WindowBindingFlags::None) ? 1 : 0);
		}

		// Swap the buffers
		glXSwapBuffers(windowInfo->display, windowInfo->window);
	}
#elif defined(OPENGL_USE_PLATFORM_APPKIT)
	if (std::holds_alternative<WindowInfoAppKit>(_drawState.windowInfo))
	{
		// Enable vsync if specifically requested or tearing not permitted
		bool vsyncEnabled = ((_drawState.windowBindingFlags & WindowBindingFlags::LimitSwapToVSync) != WindowBindingFlags::None) || ((_drawState.windowBindingFlags & WindowBindingFlags::AllowTearing) == WindowBindingFlags::None);
		GLint swapInterval = (vsyncEnabled ? 1 : 0);
		CGLSetParameter(_renderingContext, kCGLCPSwapInterval, &swapInterval);

		// Swap the buffers
		auto cglFlushDrawableReturn = CGLFlushDrawable(_renderingContext);
		if (cglFlushDrawableReturn != kCGLNoError)
		{
			_log->Error("CGLFlushDrawable failed with error code {0}.", cglFlushDrawableReturn);
			return false;
		}
	}
#endif
	else
	{
		_log->Error("PresentToWindow failed: framebuffer is not bound.");
		return false;
	}
	return true;
}

//----------------------------------------------------------------------------------------
void OpenGLFrameBuffer::FlushTextureAttachmentWritesForSampling()
{
	// This path is only useful for texture-backed framebuffers. Some old macOS Intel OpenGL drivers appear to leave a
	// just-cleared framebuffer texture invisible to a later window-context sampling pass until a framebuffer read
	// forces the attachment to be materialized. The contents read are irrelevant, so keep this to tiny PBO reads that
	// never leave the GPU side.
	if (_drawState.boundToWindow || (_frameBufferID == 0) || (_sampleCount != ITextureBuffer::SampleCount::SampleCount1))
	{
		return;
	}

	// Bind our framebuffer
	glBindFramebuffer(GL_READ_FRAMEBUFFER, _frameBufferID);

	// Perform dummy reads where required
	bool issuedRead = false;
	for (const BoundTextureInfo& boundTexture : _drawState.boundTextures)
	{
		// If this is our first dummy read, create the dummy read buffer if required and bind it.
		if (!issuedRead)
		{
			// Create the dummy read buffer if one hasn't previously been generated
			bool pixelBufferCreated = false;
			if (_textureAttachmentWriteFlushPixelBufferID == 0)
			{
				glGenBuffers(1, &_textureAttachmentWriteFlushPixelBufferID);
				pixelBufferCreated = true;
			}

			// Bind and configure the buffer
			glBindBuffer(GL_PIXEL_PACK_BUFFER, _textureAttachmentWriteFlushPixelBufferID);
			if (pixelBufferCreated)
			{
				glBufferData(GL_PIXEL_PACK_BUFFER, 4, nullptr, GL_STREAM_READ);
			}
		}

		// Determine the data format and type flags to use
		GLenum nativeDataFormat = GL_RED;
		GLenum nativeDataType = GL_UNSIGNED_BYTE;
		switch (boundTexture.type)
		{
		case IFrameBuffer::AttachmentType::Color:
			switch (GetColorAttachmentFormat(boundTexture.index))
			{
			case AttachmentFormat::Int:
				nativeDataFormat = GL_RED_INTEGER;
				nativeDataType = GL_INT;
				break;
			case AttachmentFormat::UInt:
				nativeDataFormat = GL_RED_INTEGER;
				nativeDataType = GL_UNSIGNED_INT;
				break;
			case AttachmentFormat::Float:
				break;
			}
			break;
		case IFrameBuffer::AttachmentType::Depth:
			nativeDataFormat = GL_DEPTH_COMPONENT;
			nativeDataType = GL_FLOAT;
			break;
		case IFrameBuffer::AttachmentType::Stencil:
			nativeDataFormat = GL_STENCIL_INDEX;
			nativeDataType = GL_UNSIGNED_BYTE;
			break;
		}

		// Perform our dummy read. The read buffer only selects colour attachments, but leave it on a valid colour
		// attachment value when reading depth/stencil as this matches the normal capture path.
		if (boundTexture.type == IFrameBuffer::AttachmentType::Color)
		{
			glReadBuffer(GetNativeAttachmentType(boundTexture.type, boundTexture.index));
		}
		else
		{
			glReadBuffer(GL_COLOR_ATTACHMENT0);
		}
		glReadPixels(0, 0, 1, 1, nativeDataFormat, nativeDataType, nullptr);
		issuedRead = true;
	}

	// Unbind the dummy read buffer
	if (issuedRead)
	{
		glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
		glReadBuffer(GL_COLOR_ATTACHMENT0);
	}

	// Unbind our framebuffer
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	CheckGLError(_log);
}

//----------------------------------------------------------------------------------------
void OpenGLFrameBuffer::ResolveMultiSamplingAttachmentToTexture(AttachmentType type, size_t index, size_t resolveIndex)
{
	// Locate the resolve framebuffer ID
	bool foundResolveEntry = false;
	GLuint resolveFrameBufferId = 0;
	for (const auto& entry : _drawState.resolveTextures)
	{
		if ((entry.type == type) && (entry.index == resolveIndex))
		{
			foundResolveEntry = true;
			resolveFrameBufferId = entry.resolveFrameBufferID;
		}
	}
	if (!foundResolveEntry)
	{
		_log->Error("Failed to locate the target multisampling resolve attachment with type {0} and index {1}", type, resolveIndex);
		return;
	}

	// Clear our framebuffer binding, and re-bind it as the read buffer only.
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, _frameBufferID);

	// Set our target framebuffer attachment as the read entry from the framebuffer
	glReadBuffer(GetNativeAttachmentType(type, index));

	// Bind our resolve framebuffer, and nominate our only attachment as the only draw attachment.
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resolveFrameBufferId);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	// Resolve the multisample framebuffer attachment down to a single sample image
	V2UInt32 surfaceSize = GetSurfaceSizeInPixels();
	glBlitFramebuffer(0, 0, surfaceSize.X(), surfaceSize.Y(), 0, 0, surfaceSize.X(), surfaceSize.Y(), GL_COLOR_BUFFER_BIT, GL_NEAREST);

	// Restore the original framebuffer binding
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, _frameBufferID);
}

//----------------------------------------------------------------------------------------
#ifdef OPENGL_USE_PLATFORM_WIN32
void OpenGLFrameBuffer::CreateFrameBuffer(HGLRC mainRenderingContext)
#elif defined(OPENGL_USE_EGL)
void OpenGLFrameBuffer::CreateFrameBuffer(EGLContext mainRenderingContext)
#elif defined(OPENGL_USE_PLATFORM_XLIB)
void OpenGLFrameBuffer::CreateFrameBuffer(GLXContext mainRenderingContext)
#elif defined(OPENGL_USE_PLATFORM_APPKIT)
void OpenGLFrameBuffer::CreateFrameBuffer(CGLContextObj mainRenderingContext)
#endif
{
	if (_drawState.boundToWindow && _drawState.headlessWindow)
	{
		// Generate a framebuffer with renderer-owned renderbuffers, giving callers a window-like target without a
		// native surface.
		glGenFramebuffers(1, &_frameBufferID);
		glBindFramebuffer(GL_FRAMEBUFFER, _frameBufferID);

		glGenRenderbuffers(1, &_headlessColorRenderBufferID);
		glBindRenderbuffer(GL_RENDERBUFFER, _headlessColorRenderBufferID);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, (GLsizei)_drawState.windowSizeInPixels.X(), (GLsizei)_drawState.windowSizeInPixels.Y());
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, _headlessColorRenderBufferID);

		if (_drawState.windowDepthStencilMode != WindowDepthStencilMode::None)
		{
			GLenum depthStencilFormat = GL_DEPTH_COMPONENT24;
			GLenum attachmentType = GL_DEPTH_ATTACHMENT;
			switch (_drawState.windowDepthStencilMode)
			{
			case WindowDepthStencilMode::DepthUNorm16:
				depthStencilFormat = GL_DEPTH_COMPONENT16;
				attachmentType = GL_DEPTH_ATTACHMENT;
				break;
			case WindowDepthStencilMode::DepthUNorm24:
				depthStencilFormat = GL_DEPTH_COMPONENT24;
				attachmentType = GL_DEPTH_ATTACHMENT;
				break;
			case WindowDepthStencilMode::DepthUNorm24StencilUInt8:
				depthStencilFormat = GL_DEPTH24_STENCIL8;
				attachmentType = GL_DEPTH_STENCIL_ATTACHMENT;
				break;
			case WindowDepthStencilMode::DepthFloat32:
				depthStencilFormat = GL_DEPTH_COMPONENT32F;
				attachmentType = GL_DEPTH_ATTACHMENT;
				break;
			case WindowDepthStencilMode::DepthFloat32StencilUInt8:
				depthStencilFormat = GL_DEPTH32F_STENCIL8;
				attachmentType = GL_DEPTH_STENCIL_ATTACHMENT;
				break;
			case WindowDepthStencilMode::None:
				break;
			}
			glGenRenderbuffers(1, &_headlessDepthStencilRenderBufferID);
			glBindRenderbuffer(GL_RENDERBUFFER, _headlessDepthStencilRenderBufferID);
			glRenderbufferStorage(GL_RENDERBUFFER, depthStencilFormat, (GLsizei)_drawState.windowSizeInPixels.X(), (GLsizei)_drawState.windowSizeInPixels.Y());
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachmentType, GL_RENDERBUFFER, _headlessDepthStencilRenderBufferID);
		}

		GLenum drawBuffer = GL_COLOR_ATTACHMENT0;
		glDrawBuffers(1, &drawBuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		CheckGLError(_log);
		_sampleCount = ITextureBuffer::SampleCount::SampleCount1;
	}
	else if (!_drawState.boundToWindow)
	{
		// Generate a new framebuffer, and bind it as the current framebuffer.
		glGenFramebuffers(1, &_frameBufferID);
		glBindFramebuffer(GL_FRAMEBUFFER, _frameBufferID);

		// Bind each supplied texture to the framebuffer
		std::vector<GLenum> drawBuffers;
		ITextureBuffer::SampleCount framebufferSampleCount = ITextureBuffer::SampleCount::SampleCount1;
		bool latchedInitialSampleCount = false;
		for (const BoundTextureInfo& textureInfo : _drawState.boundTextures)
		{
			// Obtain and validate the sample count for this texture. We need to ensure that all textures bound to this
			// framebuffer share the same sample count.
			OpenGLTextureBuffer2D* texture = textureInfo.texture;
			auto textureSampleCount = texture->GetSampleCount();
			if (!latchedInitialSampleCount)
			{
				framebufferSampleCount = textureSampleCount;
				latchedInitialSampleCount = true;
			}
			else if (framebufferSampleCount != textureSampleCount)
			{
				_log->Error("Mismatched sample counts detected for framebuffer texture bindings. All framebuffer textures must share the same sample count to be combined into a framebuffer.");
			}

			// Bind the texture to the framebuffer
			GLenum nativeType = GetNativeAttachmentType(textureInfo.type, textureInfo.index);
			GLenum textureTarget = (texture->GetSampleCount() != ITextureBuffer::SampleCount::SampleCount1) ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
			glFramebufferTexture2D(GL_FRAMEBUFFER, nativeType, textureTarget, texture->GetTextureNo(), 0);

			// If this texture is a colour target, add it to the list of framebuffer colour targets.
			if (textureInfo.type == IFrameBuffer::AttachmentType::Color)
			{
				if (drawBuffers.size() <= textureInfo.index)
				{
					drawBuffers.resize(textureInfo.index + 1, GL_NONE);
				}
				drawBuffers[textureInfo.index] = nativeType;
			}
		}

		// Record the calculated framebuffer sample count
		_sampleCount = framebufferSampleCount;

		// Attach the bound colour framebuffer textures as drawable buffers from the fragment shader stage
		glDrawBuffers((GLsizei)drawBuffers.size(), drawBuffers.data());

		// Unbind the framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		CheckGLError(_log);

		// Generate framebuffers for each resolve target, and bind our resolve textures to them. Note that we need to
		// make a separate framebuffer and perform a separate blit operation per texture, as per the OpenGL spec in
		// section "18.3 Copying Pixels", only one read buffer can be nominated for a transfer. Multiple write buffers
		// can receive the data, but there can be only one buffer as source for a given transfer.
		for (BoundTextureInfo& textureInfo : _drawState.resolveTextures)
		{
			// Generate a new framebuffer, and bind it as the current framebuffer.
			glGenFramebuffers(1, &textureInfo.resolveFrameBufferID);
			_resolveFrameBufferIDs.push_back(textureInfo.resolveFrameBufferID);
			glBindFramebuffer(GL_FRAMEBUFFER, textureInfo.resolveFrameBufferID);

			// Bind the texture to the framebuffer
			OpenGLTextureBuffer2D* texture = textureInfo.texture;
			GLenum nativeType = GL_COLOR_ATTACHMENT0;
			glFramebufferTexture2D(GL_FRAMEBUFFER, nativeType, GL_TEXTURE_2D, texture->GetTextureNo(), 0);

			// Set the bound texture as the draw buffer for the framebuffer
			glDrawBuffers(1, &nativeType);

			// Unbind the framebuffer
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			CheckGLError(_log);
		}
	}
	else
	{
		// Retrieve the rendering context for our window, creating it if necessary. Note that we also activate it at
		// this point.
#ifdef COBALT_RENDERER_WIN32_SUPPORT
		if (std::holds_alternative<WindowInfoWin32>(_drawState.windowInfo))
		{
			// Select the pixel format for the target window
			const auto* windowInfo = std::get_if<WindowInfoWin32>(&_drawState.windowInfo);
			if (!_renderer->SelectWindowPixelFormat(windowInfo->windowHandle, _drawState.windowColorSpaceMode, _drawState.windowDepthStencilMode, _deviceContext, _pixelFormatInfo))
			{
				_log->Error("SelectWindowPixelFormat failed on framebuffer");
			}

			// Retrieve a rendering context for our window, and make it current.
			if (!_renderer->RetrieveOrCreateRenderingContextForWindow(windowInfo->windowHandle, _deviceContext, _pixelFormatInfo, _renderingContext, _renderingContextIndex, true))
			{
				_log->Error("RetrieveOrCreateRenderingContextForWindow failed on framebuffer");
			}
		}
#endif
#ifdef OPENGL_USE_EGL
#ifdef COBALT_RENDERER_XLIB_SUPPORT
		if (std::holds_alternative<WindowInfoXlib>(_drawState.windowInfo))
		{
			// Select the pixel format for the target window
			const auto* windowInfoResolved = std::get_if<WindowInfoXlib>(&_drawState.windowInfo);

			// Create the EGL display object from the native object. If we've previously generated an EGL display from
			// the given native object, this is documented as being guaranteed to return the same EGL display handle.
			_eglDisplay = eglGetPlatformDisplay(EGL_PLATFORM_X11_KHR, (void*)windowInfoResolved->display, nullptr);
			if (_eglDisplay == EGL_NO_DISPLAY)
			{
				CheckEGLError(_log);
				_log->Error("eglGetPlatformDisplay failed when binding window");
				return;
			}

			// Initialize the EGL display object. If we've previously initialized this EGL display object, this is
			// documented as having no effect.
			if (eglInitialize(_eglDisplay, nullptr, nullptr) != EGL_TRUE)
			{
				CheckEGLError(_log);
				_log->Error("eglInitialize failed when binding window");
				_eglDisplay = EGL_NO_DISPLAY;
				return;
			}

			// Select the pixel format for the target window
			if (!_renderer->SelectWindowPixelFormat(_eglDisplay, _drawState.windowColorSpaceMode, _drawState.windowDepthStencilMode, _pixelFormatInfo, false))
			{
				_log->Error("SelectPixelFormat failed when binding window");
				_eglDisplay = EGL_NO_DISPLAY;
				return;
			}

			// Create the EGLSurface wrapper for the native window
			EGLint attribs[] = {EGL_RENDER_BUFFER, EGL_BACK_BUFFER, EGL_NONE};
			_eglSurface = eglCreateWindowSurface(_eglDisplay, _pixelFormatInfo.fbConfig, windowInfoResolved->window, &attribs[0]);
			if (_eglSurface == EGL_NO_SURFACE)
			{
				CheckEGLError(_log);
				_log->Error("eglCreateWindowSurface failed when binding window");
				_eglDisplay = EGL_NO_DISPLAY;
				_eglSurface = EGL_NO_SURFACE;
				return;
			}
			CheckEGLError(_log);
		}
#endif
#ifdef COBALT_RENDERER_XCB_SUPPORT
		if (std::holds_alternative<WindowInfoXCB>(_drawState.windowInfo))
		{
			// Select the pixel format for the target window
			const auto* windowInfoResolved = std::get_if<WindowInfoXCB>(&_drawState.windowInfo);

			// Create the EGL display object from the native object. If we've previously generated an EGL display from
			// the given native object, this is documented as being guaranteed to return the same EGL display handle.
			_eglDisplay = eglGetPlatformDisplay(EGL_PLATFORM_XCB_EXT, (void*)windowInfoResolved->connection, nullptr);
			if (_eglDisplay == EGL_NO_DISPLAY)
			{
				CheckEGLError(_log);
				_log->Error("eglGetPlatformDisplay failed when binding window");
				return;
			}

			// Initialize the EGL display object. If we've previously initialized this EGL display object, this is
			// documented as having no effect.
			if (eglInitialize(_eglDisplay, nullptr, nullptr) != EGL_TRUE)
			{
				CheckEGLError(_log);
				_log->Error("eglInitialize failed when binding window");
				_eglDisplay = EGL_NO_DISPLAY;
				return;
			}

			// Select the pixel format for the target window
			if (!_renderer->SelectWindowPixelFormat(_eglDisplay, _drawState.windowColorSpaceMode, _drawState.windowDepthStencilMode, _pixelFormatInfo, false))
			{
				_log->Error("SelectPixelFormat failed when binding window");
				_eglDisplay = EGL_NO_DISPLAY;
				return;
			}

			// Create the EGLSurface wrapper for the native window
			EGLint attribs[] = {EGL_RENDER_BUFFER, EGL_BACK_BUFFER, EGL_NONE};
			_eglSurface = eglCreateWindowSurface(_eglDisplay, _pixelFormatInfo.fbConfig, windowInfoResolved->window, &attribs[0]);
			if (_eglSurface == EGL_NO_SURFACE)
			{
				CheckEGLError(_log);
				_log->Error("eglCreateWindowSurface failed when binding window");
				_eglDisplay = EGL_NO_DISPLAY;
				_eglSurface = EGL_NO_SURFACE;
				return;
			}
			CheckEGLError(_log);
		}
#endif
#ifdef COBALT_RENDERER_WAYLAND_SUPPORT
		if (std::holds_alternative<WindowInfoWayland>(_drawState.windowInfo))
		{
			// Select the pixel format for the target window
			const auto* windowInfoResolved = std::get_if<WindowInfoWayland>(&_drawState.windowInfo);

			// Create the EGL display object from the native object. If we've previously generated an EGL display from
			// the given native object, this is documented as being guaranteed to return the same EGL display handle.
			_eglDisplay = eglGetPlatformDisplay(EGL_PLATFORM_WAYLAND_KHR, windowInfoResolved->display, nullptr);
			if (_eglDisplay == EGL_NO_DISPLAY)
			{
				CheckEGLError(_log);
				_log->Error("eglGetPlatformDisplay failed when binding window");
				return;
			}

			// Initialize the EGL display object. If we've previously initialized this EGL display object, this is
			// documented as having no effect.
			if (eglInitialize(_eglDisplay, nullptr, nullptr) != EGL_TRUE)
			{
				CheckEGLError(_log);
				_log->Error("eglInitialize failed when binding window");
				_eglDisplay = EGL_NO_DISPLAY;
				return;
			}

			// Select the pixel format for the target window
			if (!_renderer->SelectWindowPixelFormat(_eglDisplay, _drawState.windowColorSpaceMode, _drawState.windowDepthStencilMode, _pixelFormatInfo, false))
			{
				_log->Error("SelectPixelFormat failed when binding window");
				_eglDisplay = EGL_NO_DISPLAY;
				return;
			}

			// Move the Wayland window wrapper out of the draw state
			if (_drawState.eglWindowPending != nullptr)
			{
				if (_eglWindow != nullptr)
				{
					// Note that in theory this should never occur, as this object is supposed to be removed during
					// build state migration.
					wl_egl_window_destroy(_eglWindow);
				}
				_eglWindow = _drawState.eglWindowPending;
				_drawState.eglWindowPending = nullptr;
			}

			// Create the EGLSurface wrapper from the wl_egl_window wrapper
			EGLint attribs[] = {EGL_RENDER_BUFFER, EGL_BACK_BUFFER, EGL_NONE};
			_eglSurface = eglCreateWindowSurface(_eglDisplay, _pixelFormatInfo.fbConfig, reinterpret_cast<EGLNativeWindowType>(_eglWindow), &attribs[0]);
			if (_eglSurface == EGL_NO_SURFACE)
			{
				CheckEGLError(_log);
				_log->Error("eglCreateWindowSurface failed when binding window");
				_eglDisplay = EGL_NO_DISPLAY;
				_eglSurface = EGL_NO_SURFACE;
				return;
			}
			CheckEGLError(_log);
		}
#endif

		// Retrieve a rendering context for our window, and make it current.
		if (!_renderer->RetrieveOrCreateRenderingContextForWindow(_eglDisplay, _eglSurface, _pixelFormatInfo, _renderingContext, _renderingContextIndex, true))
		{
			_log->Error("RetrieveOrCreateRenderingContextForWindow failed on framebuffer");
			return;
		}

#elif defined(COBALT_RENDERER_XLIB_SUPPORT)
		if (std::holds_alternative<WindowInfoXlib>(_drawState.windowInfo))
		{
			// Select the pixel format for the target window
			const auto* windowInfo = std::get_if<WindowInfoXlib>(&_drawState.windowInfo);
			if (!_renderer->SelectWindowPixelFormat(windowInfo->display, _drawState.windowColorSpaceMode, _drawState.windowDepthStencilMode, _pixelFormatInfo))
			{
				_log->Error("SelectWindowPixelFormat failed on framebuffer");
			}

			// Retrieve a rendering context for our window, and make it current.
			if (!_renderer->RetrieveOrCreateRenderingContextForWindow(windowInfo->display, windowInfo->window, _pixelFormatInfo, _renderingContext, _renderingContextIndex, true))
			{
				_log->Error("RetrieveOrCreateRenderingContextForWindow failed on framebuffer");
			}
		}
#endif
#ifdef OPENGL_USE_PLATFORM_APPKIT
		if (std::holds_alternative<WindowInfoAppKit>(_drawState.windowInfo))
		{
			// Transfer pending window state into the live window state
			if (_drawState.renderingContextPending != nullptr)
			{
				_pixelFormatInfo = _drawState.pixelFormatInfoPending;
				_renderingContext = _drawState.renderingContextPending;
				_renderingContextIndex = _drawState.renderingContextIndexPending;
				_drawState.renderingContextPending = nullptr;
				std::scoped_lock<std::mutex> nsOpenGLContextLock(_nsOpenGLContextMigrationMutex);
				if (_nsOpenGLContext != nullptr)
				{
					// Note that in theory this should never occur, as this object is supposed to be removed during
					// build state migration.
					@autoreleasepool
					{
						CFRelease(_nsOpenGLContext);
					}
					_nsOpenGLContext = nullptr;
				}
				_nsOpenGLContext = _drawState.nsOpenGLContextPending;
				_drawState.nsOpenGLContextPending = nullptr;
			}

			// Activate the rendering context
			_renderer->ActivateRenderingContext(_renderingContext);
		}
#endif
		// Record the framebuffer sample count
		_sampleCount = ITextureBuffer::SampleCount::SampleCount1;
	}

	// Flag that the framebuffer object has been created
	_framebufferCreated = true;
	_drawState.framebufferInvalid = false;
}

//----------------------------------------------------------------------------------------
void OpenGLFrameBuffer::DeleteFrameBuffer()
{
	// Delete any resolve framebuffers generated for multisampled attachment resolves. These are owned native objects,
	// not build state, so track and free them independently of the current resolved texture binding list.
	if (!_resolveFrameBufferIDs.empty())
	{
		glDeleteFramebuffers((GLsizei)_resolveFrameBufferIDs.size(), _resolveFrameBufferIDs.data());
		_resolveFrameBufferIDs.clear();
	}
	for (BoundTextureInfo& textureInfo : _drawState.resolveTextures)
	{
		textureInfo.resolveFrameBufferID = 0;
	}

	if (!_drawState.boundToWindow || _drawState.headlessWindow)
	{
		glDeleteFramebuffers(1, &_frameBufferID);
		_frameBufferID = 0;
		if (_headlessColorRenderBufferID != 0)
		{
			glDeleteRenderbuffers(1, &_headlessColorRenderBufferID);
			_headlessColorRenderBufferID = 0;
		}
		if (_headlessDepthStencilRenderBufferID != 0)
		{
			glDeleteRenderbuffers(1, &_headlessDepthStencilRenderBufferID);
			_headlessDepthStencilRenderBufferID = 0;
		}
	}
	else
	{
		// Note that rendering contexts are released separately by the OpenGLRenderer class, so we just need to free
		// surface-related resources here.
#ifdef COBALT_RENDERER_WIN32_SUPPORT
		if (std::holds_alternative<WindowInfoWin32>(_drawState.windowInfo))
		{
			const auto* windowInfo = std::get_if<WindowInfoWin32>(&_drawState.windowInfo);
			_renderingContext = nullptr;
			ReleaseDC(windowInfo->windowHandle, _deviceContext);
			_deviceContext = nullptr;
		}
#endif
#ifdef OPENGL_USE_EGL
		if (_eglSurface != EGL_NO_SURFACE)
		{
			// Note that in theory this should never occur, as this object is supposed to be removed during build state
			// migration.
			eglDestroySurface(_eglDisplay, _eglSurface);
			_eglSurface = EGL_NO_SURFACE;
		}
		_renderingContext = nullptr;
		_eglDisplay = EGL_NO_DISPLAY;
#elif defined(COBALT_RENDERER_XLIB_SUPPORT)
		if (std::holds_alternative<WindowInfoXlib>(_drawState.windowInfo))
		{
			_renderingContext = nullptr;
		}
#endif
#ifdef OPENGL_USE_PLATFORM_APPKIT
		std::scoped_lock<std::mutex> nsOpenGLContextLock(_nsOpenGLContextMigrationMutex);
		if (_nsOpenGLContext != nullptr)
		{
			// Note that in theory this should never occur, as this object is supposed to be removed during build state
			// migration.
			@autoreleasepool
			{
				CFRelease(_nsOpenGLContext);
			}
			_nsOpenGLContext = nullptr;
		}
		_renderingContext = nullptr;
#endif
	}
	_framebufferCreated = false;
	CheckGLError(_log);
}

//----------------------------------------------------------------------------------------
constexpr GLenum OpenGLFrameBuffer::GetNativeAttachmentType(AttachmentType type, size_t index)
{
	switch (type)
	{
	case IFrameBuffer::AttachmentType::Depth:
		return GL_DEPTH_ATTACHMENT;
	case IFrameBuffer::AttachmentType::Stencil:
		return GL_STENCIL_ATTACHMENT;
	case IFrameBuffer::AttachmentType::Color:
		return GLenum(GL_COLOR_ATTACHMENT0 + index);
	}
	UNREACHABLE();
	return {};
}

//----------------------------------------------------------------------------------------
V2UInt32 OpenGLFrameBuffer::GetSurfaceSizeInPixels() const
{
	return (_drawState.boundToWindow ? _drawState.windowSizeInPixels : (!_drawState.boundTextures.empty() ? _drawState.boundTextures.front().texture->MipmapLevelDimensions(0) : V2UInt32(0, 0)));
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
void OpenGLFrameBuffer::MigrateBuildStateToDrawState()
{
	// If we have a pending window update that wasn't processed in the last frame, and we now have another pending
	// window update, free any resources associated with the missed window update.
#if defined(OPENGL_USE_EGL) && defined(COBALT_RENDERER_WAYLAND_SUPPORT)
	if ((_drawState.eglWindowPending != nullptr) && (_buildState.eglWindowPending != nullptr))
	{
		wl_egl_window_destroy(_drawState.eglWindowPending);
	}
#elif defined(OPENGL_USE_PLATFORM_APPKIT)
	if ((_drawState.nsOpenGLContextPending != nullptr) && (_buildState.nsOpenGLContextPending != nullptr))
	{
		@autoreleasepool
		{
			CFRelease(_drawState.nsOpenGLContextPending);
		}
	}
#endif

	// If we have a pending window update that wasn't processed in the last frame, and we haven't received another
	// window update, carry the pending window update forward.
#if defined(OPENGL_USE_EGL) && defined(COBALT_RENDERER_WAYLAND_SUPPORT)
	if ((_drawState.eglWindowPending != nullptr) && (_buildState.eglWindowPending == nullptr))
	{
		_buildState.eglWindowPending = _drawState.eglWindowPending;
	}
#elif defined(OPENGL_USE_PLATFORM_APPKIT)
	if ((_drawState.renderingContextPending != nullptr) && (_buildState.renderingContextPending == nullptr))
	{
		_buildState.pixelFormatInfoPending = _drawState.pixelFormatInfoPending;
		_buildState.renderingContextIndexPending = _drawState.renderingContextIndexPending;
		_buildState.renderingContextPending = _drawState.renderingContextPending;
		_buildState.nsOpenGLContextPending = _drawState.nsOpenGLContextPending;
	}
#endif

	// If there are pending updates to process in the current draw state, carry them over to the new build state too.
	_buildState.framebufferInvalid |= _drawState.framebufferInvalid;
	_buildState.viewportInvalid |= _drawState.viewportInvalid;

	// Transfer all state data from the (now updated) build state into the draw state
	_drawState = _buildState;

	// Since the pending update flags will now be handled by the draw state, and rolled back into the build state if
	// they aren't processed, clear the update flags in the new build state.
	_buildState.framebufferInvalid = false;
	_buildState.viewportInvalid = false;

	// Mark that there is no pending window update for the new build stage
#if defined(OPENGL_USE_EGL) && defined(COBALT_RENDERER_WAYLAND_SUPPORT)
	_buildState.eglWindowPending = nullptr;
#elif defined(OPENGL_USE_PLATFORM_APPKIT)
	_buildState.renderingContextPending = nullptr;
	_buildState.nsOpenGLContextPending = nullptr;
#endif

	//##TODO## Fix this comment block
	// If the framebuffer is now invalid, and we have a live EGL window wrapper, delete it now. We do this because it
	// allows a previous Wayland surface to be deterministically released if the bound window changes, at a point the
	// calling application can synchronize with. We can do this safely here, as the previous frame draw is complete
	// during build state migration, so it won't be in use for drawing/presentation.
#if defined(OPENGL_USE_EGL) && defined(COBALT_RENDERER_WAYLAND_SUPPORT)
	if (_drawState.framebufferInvalid && (_eglWindow != nullptr))
	{
		wl_egl_window_destroy(_eglWindow);
		_eglWindow = nullptr;
	}
#elif defined(OPENGL_USE_PLATFORM_APPKIT)
	if (_drawState.framebufferInvalid && (_nsOpenGLContext != nullptr))
	{
		@autoreleasepool
		{
			CFRelease(_nsOpenGLContext);
		}
		_nsOpenGLContext = nullptr;
	}
#endif

	// Reset the flag indicating that the object has been modified
	_stateModified.clear(std::memory_order_relaxed);
}

//----------------------------------------------------------------------------------------
void OpenGLFrameBuffer::FlagBuildStateModified()
{
	if (!_stateModified.test_and_set(std::memory_order_acquire))
	{
		_renderer->FlagObjectModified(this);
	}
}

#if (defined(OPENGL_USE_EGL) && defined(COBALT_RENDERER_WAYLAND_SUPPORT)) || defined(OPENGL_USE_PLATFORM_APPKIT)
//----------------------------------------------------------------------------------------
void OpenGLFrameBuffer::ReleaseInvalidatedStateForNextFrame()
{
	std::scoped_lock<std::mutex> lock(_accessMutex);
#if defined(OPENGL_USE_EGL) && defined(COBALT_RENDERER_WAYLAND_SUPPORT)
	// If the framebuffer is going to be invalid after a build state migration, and we have a live EGL window wrapper,
	// delete it now. We do this because it allows a previous Wayland surface to be deterministically released if the
	// bound window changes, at a point the calling application can synchronize with. We can do this safely here, as
	// the previous frame draw is complete during early next frame resource deletion, so it won't be in use for
	// drawing/presentation.
	if (_buildState.framebufferInvalid && (_eglWindow != nullptr))
	{
		wl_egl_window_destroy(_eglWindow);
		_eglWindow = nullptr;
	}
#elif defined(OPENGL_USE_PLATFORM_APPKIT)
	// As per above for EGL, on macOS, we need to ensure we detach the NSOpenGLContext from the view it's bound to prior
	// to the view being deleted. This early release makes that possible.
	std::scoped_lock<std::mutex> nsOpenGLContextLock(_nsOpenGLContextMigrationMutex);
	if (_buildState.framebufferInvalid && (_nsOpenGLContext != nullptr))
	{
		@autoreleasepool
		{
			CFRelease(_nsOpenGLContext);
		}
		_nsOpenGLContext = nullptr;
	}
#endif
}
#endif

} // namespace cobalt::graphics
