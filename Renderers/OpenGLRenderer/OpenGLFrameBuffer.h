// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "OpenGLHeaders.h"
#include "OpenGLRenderer.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/PlatformBindings.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <atomic>
#include <mutex>
#include <variant>
#include <vector>
namespace cobalt::graphics {
class OpenGLTextureBuffer2D;
class OpenGLFrameBufferOutput;

class OpenGLFrameBuffer : public IFrameBuffer
{
public:
	// Enumerations
	enum class AttachmentFormat
	{
		Int,
		UInt,
		Float,
	};

public:
	// Constructors
	OpenGLFrameBuffer(cobalt::logging::ILogger* log, OpenGLRenderer* renderer);
	~OpenGLFrameBuffer();

	// Initialization methods
	void Delete() override;
	void PerformPreDelete();

	// Binding methods
	SuccessToken BindTexture(ITextureBuffer2D* texture, AttachmentType type, size_t index) override;
	void UnbindTexture(AttachmentType type, size_t index) override;
	SuccessToken BindMultiSamplingResolveTexture(ITextureBuffer2D* texture, AttachmentType type, size_t index) override;
	void UnbindMultiSamplingResolveTexture(AttachmentType type, size_t index) override;
	SuccessToken BindWindow(const WindowInfoBase& windowInfo, WindowDepthStencilMode depthStencilMode, WindowColorSpaceMode colorSpaceMode, WindowBindingFlags bindingFlags) override;
	SuccessToken NotifyWindowResized(const V2UInt32& windowSizeInPixels) override;
	bool IsBoundToWindow() const;
	bool HasBoundDepthBuffer() const;
	AttachmentFormat GetColorAttachmentFormat(size_t index) const;
	ITextureBuffer::SampleCount GetSampleCount() const;
	size_t GetWindowRenderingContextIndex() const;
#ifdef OPENGL_USE_PLATFORM_WIN32
	void BindFrameBuffer(HGLRC mainRenderingContext);
	void GetRenderingContext(HWND& framebufferWindowHandle, HDC& framebufferDeviceContext, HGLRC& framebufferRenderingContext) const;
#elif defined(OPENGL_USE_EGL)
	void BindFrameBuffer(EGLContext mainRenderingContext);
	void GetRenderingContext(EGLDisplay& framebufferDisplay, EGLSurface& framebufferSurface, EGLContext& framebufferRenderingContext) const;
#elif defined(OPENGL_USE_PLATFORM_XLIB)
	void BindFrameBuffer(GLXContext mainRenderingContext);
	void GetRenderingContext(::Display*& framebufferDisplay, ::Window& framebufferWindow, GLXContext& framebufferRenderingContext) const;
#elif defined(OPENGL_USE_PLATFORM_APPKIT)
	void BindFrameBuffer(CGLContextObj mainRenderingContext);
	void GetRenderingContext(CGLContextObj& framebufferRenderingContext) const;
#endif

	// Viewport methods
	void DefineViewportRegion(const V2UInt32& startPos, const V2UInt32& size) override;
	void DefineScissorRegion(const V2UInt32& startPos, const V2UInt32& size) override;
	void RemoveScissorRegion() override;

	// Output capture methods
	bool HasCaptureTargets() const;
	void AddOutputCaptureTarget(IFrameBufferOutput* captureTarget, AttachmentType type, size_t index) override;
	void RemoveOutputCaptureTarget(IFrameBufferOutput* captureTarget) override;
	void CaptureFrameBufferOutput(bool usePixelBufferObjects, bool captureFrontBuffer);
	void CompleteCaptureFrameBufferOutput();

	// Framebuffer update methods
	bool PresentToWindow();
	void FlushTextureAttachmentWritesForSampling();
	void ResolveMultiSamplingAttachmentToTexture(AttachmentType type, size_t index, size_t resolveIndex);

	// Build state methods
	void MigrateBuildStateToDrawState();
#if (defined(OPENGL_USE_EGL) && defined(COBALT_RENDERER_WAYLAND_SUPPORT)) || defined(OPENGL_USE_PLATFORM_APPKIT)
	void ReleaseInvalidatedStateForNextFrame();
#endif

private:
	// Structures
	struct BoundTextureInfo
	{
		AttachmentType type = {};
		size_t index = {};
		OpenGLTextureBuffer2D* texture = nullptr;
		GLuint resolveFrameBufferID = 0;
	};
	struct CaptureTargetInfo
	{
		AttachmentType type;
		size_t index;
		OpenGLFrameBufferOutput* captureTarget;
		bool completionPending = false;
		GLsync completionFence;
		ITextureBuffer::SourceImageFormat optimalSourceImageFormat;
		ITextureBuffer::SourceDataFormat optimalSourceDataFormat;
		ITextureBuffer2D::ImageFormat finalImageFormat;
		ITextureBuffer2D::DataFormat finalDataFormat;
		V2UInt32 croppedImageSize;
	};
	struct CaptureStagingPixelBufferInfo
	{
		GLuint pixelBufferObjectID;
		size_t bufferSizeInBytes;
	};
	struct MutableState
	{
		bool framebufferInvalid = false;
		bool viewportInvalid = false;
		bool boundToWindow = false;
		bool headlessWindow = false;
		bool hasBoundDepthBuffer = false;
		bool scissorRegionDefined = false;
		std::variant<std::monostate,
		             WindowInfoHeadless
#ifdef COBALT_RENDERER_WIN32_SUPPORT
		             ,
		             WindowInfoWin32
#endif
#ifdef COBALT_RENDERER_XLIB_SUPPORT
		             ,
		             WindowInfoXlib
#endif
#ifdef OPENGL_USE_EGL
#ifdef COBALT_RENDERER_XCB_SUPPORT
		             ,
		             WindowInfoXCB
#endif
#ifdef COBALT_RENDERER_WAYLAND_SUPPORT
		             ,
		             WindowInfoWayland
#endif
#endif
#ifdef COBALT_RENDERER_APPKIT_SUPPORT
		             ,
		             WindowInfoAppKit
#endif
		             >
		  windowInfo;
		WindowDepthStencilMode windowDepthStencilMode = {};
		WindowColorSpaceMode windowColorSpaceMode = {};
		WindowBindingFlags windowBindingFlags = WindowBindingFlags::None;
		V2UInt32 windowSizeInPixels = {};
		V2UInt32 viewportRegionStartPos = {};
		V2UInt32 viewportRegionSize = {};
		V2UInt32 scissorRegionStartPos = {};
		V2UInt32 scissorRegionSize = {};
#if defined(OPENGL_USE_EGL) && defined(COBALT_RENDERER_WAYLAND_SUPPORT)
		wl_egl_window* eglWindowPending = nullptr;
#endif
#ifdef OPENGL_USE_PLATFORM_APPKIT
		OpenGLRenderer::PixelFormatInfo pixelFormatInfoPending = {};
		CGLContextObj renderingContextPending = nullptr;
		size_t renderingContextIndexPending = {};
		const void* nsOpenGLContextPending = nullptr;
#endif
		std::vector<BoundTextureInfo> boundTextures;
		std::vector<BoundTextureInfo> resolveTextures;
		std::vector<CaptureTargetInfo> captureTargets;
	};

private:
	// Framebuffer update methods
#ifdef OPENGL_USE_PLATFORM_WIN32
	void CreateFrameBuffer(HGLRC mainRenderingContext);
#elif defined(OPENGL_USE_EGL)
	void CreateFrameBuffer(EGLContext mainRenderingContext);
#elif defined(OPENGL_USE_PLATFORM_XLIB)
	void CreateFrameBuffer(GLXContext mainRenderingContext);
#elif defined(OPENGL_USE_PLATFORM_APPKIT)
	void CreateFrameBuffer(CGLContextObj mainRenderingContext);
#endif
	void DeleteFrameBuffer();
	constexpr static GLenum GetNativeAttachmentType(AttachmentType type, size_t index);
	V2UInt32 GetSurfaceSizeInPixels() const;

	// Build state methods
	void FlagBuildStateModified();

private:
	cobalt::logging::ILogger* _log;
	OpenGLRenderer* _renderer;
	mutable std::mutex _accessMutex;
	GLuint _frameBufferID = 0;
	GLuint _headlessColorRenderBufferID = 0;
	GLuint _headlessDepthStencilRenderBufferID = 0;
	GLuint _textureAttachmentWriteFlushPixelBufferID = 0;
	std::vector<GLuint> _resolveFrameBufferIDs;
	bool _framebufferCreated;
	ITextureBuffer::SampleCount _sampleCount = {};
	OpenGLRenderer::PixelFormatInfo _pixelFormatInfo = {};
	std::vector<CaptureStagingPixelBufferInfo> _stagingBuffersForCapture;
	size_t _renderingContextIndex = 0;
#ifdef OPENGL_USE_PLATFORM_WIN32
	HDC _deviceContext = nullptr;
	HGLRC _renderingContext = nullptr;
#elif defined(OPENGL_USE_EGL)
	EGLContext _renderingContext = nullptr;
#ifdef COBALT_RENDERER_WAYLAND_SUPPORT
	wl_egl_window* _eglWindow = nullptr;
#endif
	EGLDisplay _eglDisplay = EGL_NO_DISPLAY;
	EGLSurface _eglSurface = EGL_NO_SURFACE;
#elif defined(OPENGL_USE_PLATFORM_XLIB)
	GLXContext _renderingContext = nullptr;
#elif defined(OPENGL_USE_PLATFORM_APPKIT)
	mutable std::mutex _nsOpenGLContextMigrationMutex;
	CGLContextObj _renderingContext = nullptr;
	const void* _nsOpenGLContext = nullptr;
#endif
	std::atomic_flag _stateModified = ATOMIC_FLAG_INIT;
	MutableState _drawState;
	MutableState _buildState;
};

} // namespace cobalt::graphics
