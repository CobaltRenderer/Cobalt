// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "OpenGLRenderer.h"
#include "OpenGLDebug.h"
#include "OpenGLDefaultState.h"
#include "OpenGLFrameBuffer.h"
#include "OpenGLFrameBufferOutput.h"
#include "OpenGLIndexBuffer.h"
#include "OpenGLProgramNode.h"
#include "OpenGLRenderPassNode.h"
#include "OpenGLRenderableNode.h"
#include "OpenGLShaderProgram.h"
#include "OpenGLStateBuffer.h"
#include "OpenGLStateBufferLayout.h"
#include "OpenGLStateGroupNode.h"
#include "OpenGLTextureBuffer1D.h"
#include "OpenGLTextureBuffer1DArray.h"
#include "OpenGLTextureBuffer2D.h"
#include "OpenGLTextureBuffer2DArray.h"
#include "OpenGLTextureBuffer3D.h"
#include "OpenGLTextureBufferCube.h"
#include "OpenGLTextureBufferCubeArray.h"
#include "OpenGLTextureSampler1D.h"
#include "OpenGLTextureSampler1DArray.h"
#include "OpenGLTextureSampler2D.h"
#include "OpenGLTextureSampler2DArray.h"
#include "OpenGLTextureSampler3D.h"
#include "OpenGLTextureSamplerCube.h"
#include "OpenGLTextureSamplerCubeArray.h"
#include "OpenGLTransferBatch.h"
#include "OpenGLVertexBuffer.h"
#include <Internal/RendererSupport/KnownDynamicCast.h>
#ifdef GL_VERSION_4_3
#include "OpenGLDataArray.h"
#include "OpenGLDataArrayOutput.h"
#include "OpenGLTexelArray.h"
#include "OpenGLTexelArrayOutput.h"
#endif
#include "OpenGLHeaders.h"
#ifdef OPENGL_USE_PLATFORM_XLIB
// Workaround for WSL crash
#include <dlfcn.h>
#endif
#ifdef OPENGL_USE_EGL
#include <fcntl.h>
#include <unistd.h>
#endif
#include <cstring>
#include <numeric>
#include <sstream>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
#ifdef __linux__
OpenGLRenderer::OpenGLRenderer(cobalt::logging::ILogger::unique_ptr log, const std::set<IGraphicsDevice::Feature>& enabledFeatures, const std::set<Options>& enabledOptions, bool finishWhenSwitchingContexts, bool flushFramebufferTextureWritesBeforeWindowSampling, bool usingSoftwareRenderer, const std::string& renderNodePath)
#else
OpenGLRenderer::OpenGLRenderer(cobalt::logging::ILogger::unique_ptr log, const std::set<IGraphicsDevice::Feature>& enabledFeatures, const std::set<Options>& enabledOptions, bool finishWhenSwitchingContexts, bool flushFramebufferTextureWritesBeforeWindowSampling, bool usingSoftwareRenderer)
#endif
: _buildIndex(0), _drawIndex(1), _enabledFeatures(enabledFeatures), _enabledOptions(enabledOptions), _usingSoftwareRenderer(usingSoftwareRenderer), _finishWhenSwitchingContexts(finishWhenSwitchingContexts), _flushFramebufferTextureWritesBeforeWindowSampling(flushFramebufferTextureWritesBeforeWindowSampling)
#ifdef __linux__
  ,
  _renderNodePath(renderNodePath)
#endif
{
	_log = (log != nullptr ? std::move(log) : cobalt::logging::ILogger::unique_ptr(new cobalt::logging::NullLogger()));
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
#ifdef _WIN32
namespace {
HMODULE GetCurrentModule()
{
	HMODULE hModule = nullptr;
	GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCTSTR>(GetCurrentModule), &hModule);
	return hModule;
}
} // namespace
#endif

//----------------------------------------------------------------------------------------
SuccessToken OpenGLRenderer::Initialize(const WindowSystemInfoBase& windowSystemInfo, InitializationFlags flags)
{
	// Resolve and store the WindowSystemInfo structure
#if defined(OPENGL_USE_PLATFORM_WIN32) || defined(OPENGL_USE_EGL)
	if ((windowSystemInfo.windowSystemType == WindowSystemInfoBase::WindowSystemType::Headless) && (windowSystemInfo.structureSizeInBytes == sizeof(WindowSystemInfoHeadless)))
	{
		auto windowSystemInfoResolved = reinterpret_cast<const WindowSystemInfoHeadless*>(&windowSystemInfo);
		_windowSystemInfo = *windowSystemInfoResolved;
	}
	else
#endif
#ifdef COBALT_RENDERER_WIN32_SUPPORT
	  if ((windowSystemInfo.windowSystemType == WindowSystemInfoBase::WindowSystemType::Win32) && (windowSystemInfo.structureSizeInBytes == sizeof(WindowSystemInfoWin32)))
	{
		auto windowSystemInfoResolved = reinterpret_cast<const WindowSystemInfoWin32*>(&windowSystemInfo);
		_windowSystemInfo = *windowSystemInfoResolved;
	}
	else
#endif
#ifdef OPENGL_USE_EGL
#ifdef COBALT_RENDERER_XLIB_SUPPORT
	  if ((windowSystemInfo.windowSystemType == WindowSystemInfoBase::WindowSystemType::Xlib) && (windowSystemInfo.structureSizeInBytes == sizeof(WindowSystemInfoXlib)))
	{
		auto windowSystemInfoResolved = reinterpret_cast<const WindowSystemInfoXlib*>(&windowSystemInfo);
		_windowSystemInfo = *windowSystemInfoResolved;
	}
	else
#endif
#ifdef COBALT_RENDERER_XCB_SUPPORT
	  if ((windowSystemInfo.windowSystemType == WindowSystemInfoBase::WindowSystemType::XCB) && (windowSystemInfo.structureSizeInBytes == sizeof(WindowSystemInfoXCB)))
	{
		auto windowSystemInfoResolved = reinterpret_cast<const WindowSystemInfoXCB*>(&windowSystemInfo);
		_windowSystemInfo = *windowSystemInfoResolved;
	}
	else
#endif
#ifdef COBALT_RENDERER_WAYLAND_SUPPORT
	  if ((windowSystemInfo.windowSystemType == WindowSystemInfoBase::WindowSystemType::Wayland) && (windowSystemInfo.structureSizeInBytes == sizeof(WindowSystemInfoWayland)))
	{
		auto windowSystemInfoResolved = reinterpret_cast<const WindowSystemInfoWayland*>(&windowSystemInfo);
		_windowSystemInfo = *windowSystemInfoResolved;
	}
	else
#endif
#elif defined(COBALT_RENDERER_XLIB_SUPPORT)
	if ((windowSystemInfo.windowSystemType == WindowSystemInfoBase::WindowSystemType::Xlib) && (windowSystemInfo.structureSizeInBytes == sizeof(WindowSystemInfoXlib)))
	{
		auto windowSystemInfoResolved = reinterpret_cast<const WindowSystemInfoXlib*>(&windowSystemInfo);
		_windowSystemInfo = *windowSystemInfoResolved;
	}
	else
#endif
#ifdef COBALT_RENDERER_APPKIT_SUPPORT
	  if ((windowSystemInfo.windowSystemType == WindowSystemInfoBase::WindowSystemType::AppKit) && (windowSystemInfo.structureSizeInBytes == sizeof(WindowSystemInfoAppKit)))
	{
		auto windowSystemInfoResolved = reinterpret_cast<const WindowSystemInfoAppKit*>(&windowSystemInfo);
		_windowSystemInfo = *windowSystemInfoResolved;
	}
	else
#endif
	{
		_log->Error("Could not initialize renderer. Unsupported window system binding structure supplied with type \"{0}\" and size \"{1}\".", windowSystemInfo.windowSystemType, windowSystemInfo.structureSizeInBytes);
		return false;
	}

	// Bind to our window system
#ifdef _WIN32
	if (std::holds_alternative<WindowSystemInfoWin32>(_windowSystemInfo) || std::holds_alternative<WindowSystemInfoHeadless>(_windowSystemInfo))
	{
		// Register a dummy window class. Note that this call and the one below need to be run in the context of the calling
		// thread here, so they can ensure the window messages for the dummy window can be handled.
		auto* hinstance = reinterpret_cast<HINSTANCE>(GetCurrentModule());
		WNDCLASSEXW wc = {};
		std::wstring windowClassName = L"CobaltOpenGLRendererDummyWindow";
		std::wstring windowName;
		wc.cbSize = sizeof(WNDCLASSEX);
		wc.lpfnWndProc = DefWindowProc;
		wc.hInstance = hinstance;
		wc.lpszClassName = windowClassName.c_str();
		if ((RegisterClassExW(&wc) == 0) && (GetLastError() != ERROR_CLASS_ALREADY_EXISTS))
		{
			_log->Error("Failed to register dummy OpenGL window class");
			return false;
		}

		// Create a dummy window to enable us to create an initial OpenGL render context. Note that this doesn't need to be
		// made visible.
		_dummyMainWindowHandle = CreateWindowExW(0, windowClassName.c_str(), windowName.c_str(), WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, hinstance, reinterpret_cast<LPVOID>(this));
		if (_dummyMainWindowHandle == nullptr)
		{
			_log->Error("Failed to create dummy OpenGL window");
			return false;
		}
	}
#endif
#ifdef OPENGL_USE_EGL
#ifdef COBALT_RENDERER_XLIB_SUPPORT
	if (std::holds_alternative<WindowSystemInfoXlib>(_windowSystemInfo))
	{
		auto* windowSystemInfo = std::get_if<WindowSystemInfoXlib>(&_windowSystemInfo);
		_mainDisplay = eglGetPlatformDisplay(EGL_PLATFORM_X11_KHR, windowSystemInfo->display, nullptr);
		CheckEGLError(_log.get());
	}
#endif
#ifdef COBALT_RENDERER_XCB_SUPPORT
	if (std::holds_alternative<WindowSystemInfoXCB>(_windowSystemInfo))
	{
		static auto eglGetPlatformDisplayEXT = reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(eglGetProcAddress("eglGetPlatformDisplayEXT"));
		auto* windowSystemInfo = std::get_if<WindowSystemInfoXCB>(&_windowSystemInfo);
		_mainDisplay = eglGetPlatformDisplayEXT(EGL_PLATFORM_XCB_EXT, windowSystemInfo->connection, nullptr);
		CheckEGLError(_log.get());
	}
#endif
#ifdef COBALT_RENDERER_WAYLAND_SUPPORT
	if (std::holds_alternative<WindowSystemInfoWayland>(_windowSystemInfo))
	{
		auto* windowSystemInfo = std::get_if<WindowSystemInfoWayland>(&_windowSystemInfo);
		_mainDisplay = eglGetPlatformDisplay(EGL_PLATFORM_WAYLAND_KHR, windowSystemInfo->display, nullptr);
		CheckEGLError(_log.get());
	}
#endif
	if (std::holds_alternative<WindowSystemInfoHeadless>(_windowSystemInfo))
	{
		if (_renderNodePath.empty())
		{
			// If we don't have a render node path, which could happen if we're running in headless mode with a software
			// renderer, we try for the Mesa surfaceless extension, otherwise we fall back to EGL_DEFAULT_DISPLAY.
			bool supportsSurfacelessMesaPlatform = false;
			auto clientExtensionsRaw = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
			if (clientExtensionsRaw != nullptr)
			{
				std::istringstream clientExtensionStream(clientExtensionsRaw);
				std::string clientExtensionName;
				while (clientExtensionStream >> clientExtensionName)
				{
					if (clientExtensionName == "EGL_MESA_platform_surfaceless")
					{
						supportsSurfacelessMesaPlatform = true;
						break;
					}
				}
			}
			if (supportsSurfacelessMesaPlatform)
			{
				_log->Info("No render node path was supplied for headless rendering. Trying EGL_PLATFORM_SURFACELESS_MESA.");
				_mainDisplay = eglGetPlatformDisplay(EGL_PLATFORM_SURFACELESS_MESA, EGL_DEFAULT_DISPLAY, nullptr);
				if (_mainDisplay == EGL_NO_DISPLAY)
				{
					CheckEGLError(_log.get());
					_log->Warning("eglGetPlatformDisplay(EGL_PLATFORM_SURFACELESS_MESA) failed. Falling back to EGL_DEFAULT_DISPLAY.");
				}
			}
			if (_mainDisplay == EGL_NO_DISPLAY)
			{
				_log->Info("No render node path was supplied for headless rendering. Falling back to EGL_DEFAULT_DISPLAY.");
				_mainDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
				CheckEGLError(_log.get());
			}
		}
		else
		{
			// Attempt to open the GBM device for headless rendering
			_gbmDeviceFileHandle = open(_renderNodePath.c_str(), O_RDWR | O_CLOEXEC); // NOLINT(cppcoreguidelines-pro-type-vararg,hicpp-vararg)
			if (_gbmDeviceFileHandle < 0)
			{
				_log->Error("Could not initialize renderer for headless rendering. Failed to open the device with path \"{0}\": errno={1} ({2}).", _renderNodePath, errno, std::strerror(errno));
				return false;
			}
			_gbmDevice = gbm_create_device(_gbmDeviceFileHandle);
			if (_gbmDevice == nullptr)
			{
				_log->Error("Could not initialize renderer for headless rendering. gbm_create_device failed for device with path \"{0}\": errno={1} ({2}).", _renderNodePath, errno, std::strerror(errno));
				close(_gbmDeviceFileHandle);
				_gbmDeviceFileHandle = -1;
				return false;
			}

			// Attempt to create the display for headless rendering through GBM
			_mainDisplay = eglGetPlatformDisplay(EGL_PLATFORM_GBM_KHR, _gbmDevice, nullptr);
			CheckEGLError(_log.get());
		}
	}
#elif defined(COBALT_RENDERER_XLIB_SUPPORT)
	if (std::holds_alternative<WindowSystemInfoXlib>(_windowSystemInfo))
	{
		auto* windowSystemInfo = std::get_if<WindowSystemInfoXlib>(&_windowSystemInfo);
		_xDisplay = windowSystemInfo->display;
	}
#endif

	// Start the render worker thread
	_renderThreadActive = true;
	_frameAdvanceInProgress = false;
	_buildingInProgress = true;
	_drawingInProgress = false;
	_buildToDrawRequestPending = false;
	_renderRequestPending = false;
	_swapRequestPending = false;
	_deleteLastDrawResourcesRequestPending = false;
	_earlyDeleteNextDrawResourcesRequestPending = false;
	_initializeOpenGLRequestPending = false;
	std::thread workerThread(std::bind(std::mem_fn(&OpenGLRenderer::RenderThread), this));
	workerThread.detach();

	// Attempt to initialize OpenGL, and return the result to the caller.
	std::unique_lock<std::mutex> lock(_renderThreadMutex);
	_initializeOpenGLResult = false;
	_initializeOpenGLRequestPending = true;
	_notifyRenderThreadTaskPending.notify_all();
	while (_initializeOpenGLRequestPending)
	{
		_notifyRenderThreadTaskComplete.wait(lock);
	}
	return _initializeOpenGLResult;
}

//----------------------------------------------------------------------------------------
void OpenGLRenderer::Delete()
{
	// Terminate the render worker thread. Note that the worker thread will automatically clean up various additional
	// resources on shutdown.
	std::unique_lock<std::mutex> lock(_renderThreadMutex);
	if (_renderThreadActive)
	{
		_renderThreadActive = false;
		_notifyRenderThreadTaskPending.notify_all();
		_notifyRenderThreadStopped.wait(lock);
	}
	lock.unlock();

	// Delete this object
	delete this;
}

//----------------------------------------------------------------------------------------
// Geometry buffer methods
//----------------------------------------------------------------------------------------
IVertexBuffer* OpenGLRenderer::CreateVertexBufferInternal()
{
	return new OpenGLVertexBuffer(_log.get(), this);
}

//----------------------------------------------------------------------------------------
IIndexBuffer* OpenGLRenderer::CreateIndexBufferInternal()
{
	return new OpenGLIndexBuffer(_log.get(), this);
}

//----------------------------------------------------------------------------------------
// Image buffer methods
//----------------------------------------------------------------------------------------
ITextureBuffer1D* OpenGLRenderer::CreateTextureBuffer1DInternal()
{
	return new OpenGLTextureBuffer1D(_log.get(), this);
}

//----------------------------------------------------------------------------------------
ITextureBuffer2D* OpenGLRenderer::CreateTextureBuffer2DInternal()
{
	return new OpenGLTextureBuffer2D(_log.get(), this);
}

//----------------------------------------------------------------------------------------
ITextureBuffer3D* OpenGLRenderer::CreateTextureBuffer3DInternal()
{
	return new OpenGLTextureBuffer3D(_log.get(), this);
}

//----------------------------------------------------------------------------------------
ITextureBufferCube* OpenGLRenderer::CreateTextureBufferCubeInternal()
{
	return new OpenGLTextureBufferCube(_log.get(), this);
}

//----------------------------------------------------------------------------------------
ITextureBuffer1DArray* OpenGLRenderer::CreateTextureBuffer1DArrayInternal()
{
	return new OpenGLTextureBuffer1DArray(_log.get(), this);
}

//----------------------------------------------------------------------------------------
ITextureBuffer2DArray* OpenGLRenderer::CreateTextureBuffer2DArrayInternal()
{
	return new OpenGLTextureBuffer2DArray(_log.get(), this);
}

//----------------------------------------------------------------------------------------
ITextureBufferCubeArray* OpenGLRenderer::CreateTextureBufferCubeArrayInternal()
{
	return new OpenGLTextureBufferCubeArray(_log.get(), this);
}

//----------------------------------------------------------------------------------------
// Image sampler methods
//----------------------------------------------------------------------------------------
ITextureSampler1D* OpenGLRenderer::CreateTextureSampler1DInternal()
{
	return new OpenGLTextureSampler1D(_log.get(), this);
}

//----------------------------------------------------------------------------------------
ITextureSampler2D* OpenGLRenderer::CreateTextureSampler2DInternal()
{
	return new OpenGLTextureSampler2D(_log.get(), this);
}

//----------------------------------------------------------------------------------------
ITextureSampler3D* OpenGLRenderer::CreateTextureSampler3DInternal()
{
	return new OpenGLTextureSampler3D(_log.get(), this);
}

//----------------------------------------------------------------------------------------
ITextureSamplerCube* OpenGLRenderer::CreateTextureSamplerCubeInternal()
{
	return new OpenGLTextureSamplerCube(_log.get(), this);
}

//----------------------------------------------------------------------------------------
ITextureSampler1DArray* OpenGLRenderer::CreateTextureSampler1DArrayInternal()
{
	return new OpenGLTextureSampler1DArray(_log.get(), this);
}

//----------------------------------------------------------------------------------------
ITextureSampler2DArray* OpenGLRenderer::CreateTextureSampler2DArrayInternal()
{
	return new OpenGLTextureSampler2DArray(_log.get(), this);
}

//----------------------------------------------------------------------------------------
ITextureSamplerCubeArray* OpenGLRenderer::CreateTextureSamplerCubeArrayInternal()
{
	return new OpenGLTextureSamplerCubeArray(_log.get(), this);
}

//----------------------------------------------------------------------------------------
// Data array methods
//----------------------------------------------------------------------------------------
IDataArray* OpenGLRenderer::CreateDataArrayInternal()
{
#ifdef GL_VERSION_4_3
	return new OpenGLDataArray(_log.get(), this);
#else
	_log->Error("Attempted to create data array, which is not supported by this renderer.");
	return nullptr;
#endif
}

//----------------------------------------------------------------------------------------
IDataArrayOutput* OpenGLRenderer::CreateDataArrayOutputInternal()
{
#ifdef GL_VERSION_4_3
	return new OpenGLDataArrayOutput(_log.get(), this);
#else
	_log->Error("Attempted to create data array output, which is not supported by this renderer.");
	return nullptr;
#endif
}

//----------------------------------------------------------------------------------------
ITexelArray* OpenGLRenderer::CreateTexelArrayInternal()
{
#ifdef GL_VERSION_4_3
	return new OpenGLTexelArray(_log.get(), this);
#else
	_log->Error("Attempted to create texel array, which is not supported by this renderer.");
	return nullptr;
#endif
}

//----------------------------------------------------------------------------------------
ITexelArrayOutput* OpenGLRenderer::CreateTexelArrayOutputInternal()
{
#ifdef GL_VERSION_4_3
	return new OpenGLTexelArrayOutput(_log.get(), this);
#else
	_log->Error("Attempted to create texel array output, which is not supported by this renderer.");
	return nullptr;
#endif
}

//----------------------------------------------------------------------------------------
// Batch methods
//----------------------------------------------------------------------------------------
ITransferBatch* OpenGLRenderer::CreateTransferBatchInternal(ITransferBatch::StartTiming startTiming, ITransferBatch::EndTiming endTiming)
{
	return new OpenGLTransferBatch(_log.get(), this, startTiming, endTiming);
}

//----------------------------------------------------------------------------------------
// Frame buffer methods
//----------------------------------------------------------------------------------------
IFrameBuffer* OpenGLRenderer::CreateFrameBufferInternal()
{
	return new OpenGLFrameBuffer(_log.get(), this);
}

//----------------------------------------------------------------------------------------
IFrameBufferOutput* OpenGLRenderer::CreateFrameBufferOutputInternal()
{
	return new OpenGLFrameBufferOutput(_log.get(), this);
}

//----------------------------------------------------------------------------------------
// State buffer methods
//----------------------------------------------------------------------------------------
IStateBuffer* OpenGLRenderer::CreateStateBufferInternal()
{
	return new OpenGLStateBuffer(_log.get(), this);
}

//----------------------------------------------------------------------------------------
IStateBufferLayout* OpenGLRenderer::CreateStateBufferLayoutInternal()
{
	return new OpenGLStateBufferLayout(_log.get(), this);
}

//----------------------------------------------------------------------------------------
// Render tree node methods
//----------------------------------------------------------------------------------------
IRenderPassNode* OpenGLRenderer::CreateRenderPassNodeInternal()
{
	return new OpenGLRenderPassNode(_log.get(), this);
}

//----------------------------------------------------------------------------------------
IProgramNode* OpenGLRenderer::CreateProgramNodeInternal()
{
	return new OpenGLProgramNode(_log.get(), this);
}

//----------------------------------------------------------------------------------------
IStateGroupNode* OpenGLRenderer::CreateStateGroupNodeInternal()
{
	return new OpenGLStateGroupNode(_log.get(), this);
}

//----------------------------------------------------------------------------------------
IRenderableNode* OpenGLRenderer::CreateRenderableNodeInternal()
{
	return new OpenGLRenderableNode(_log.get(), this);
}

//----------------------------------------------------------------------------------------
IDefaultState* OpenGLRenderer::CreateDefaultStateInternal()
{
	return new OpenGLDefaultState(_log.get(), this);
}

//----------------------------------------------------------------------------------------
// Program methods
//----------------------------------------------------------------------------------------
IShaderProgram* OpenGLRenderer::CreateShaderProgramInternal()
{
	return new OpenGLShaderProgram(_log.get(), this);
}

//----------------------------------------------------------------------------------------
// Scene content methods
//----------------------------------------------------------------------------------------
void OpenGLRenderer::SetRenderPasses(IRenderPassNode* const* childNodes, size_t childNodeCount, const int32_t* childNodeSortOrder)
{
	std::lock_guard<std::mutex> lock(_buildStateMutex);
	if (childNodeSortOrder != nullptr)
	{
		_state[_buildIndex].renderPasses.clear();
		for (size_t i = 0; i < childNodeCount; ++i)
		{
			RenderPassEntry renderPassEntry{};
			renderPassEntry.renderPassNode = KnownDynamicCast<OpenGLRenderPassNode*>(*(childNodes++));
			renderPassEntry.sortIndex = (childNodeSortOrder != nullptr ? *(childNodeSortOrder++) : (int)i);
			_state[_buildIndex].renderPasses.insert(std::upper_bound(_state[_buildIndex].renderPasses.begin(), _state[_buildIndex].renderPasses.end(), renderPassEntry, RenderPassEntry::Sorter()), renderPassEntry);
		}
	}
	else
	{
		_state[_buildIndex].renderPasses.resize(childNodeCount);
		for (size_t i = 0; i < childNodeCount; ++i)
		{
			RenderPassEntry& renderPassEntry = _state[_buildIndex].renderPasses[i];
			renderPassEntry.renderPassNode = KnownDynamicCast<OpenGLRenderPassNode*>(*(childNodes++));
			renderPassEntry.sortIndex = (int)i;
		}
	}
}

//----------------------------------------------------------------------------------------
void OpenGLRenderer::SetRenderPasses(IRenderPassNode::unique_ptr const* childNodes, size_t childNodeCount, const int32_t* childNodeSortOrder)
{
	std::lock_guard<std::mutex> lock(_buildStateMutex);
	if (childNodeSortOrder != nullptr)
	{
		_state[_buildIndex].renderPasses.clear();
		for (size_t i = 0; i < childNodeCount; ++i)
		{
			RenderPassEntry renderPassEntry{};
			renderPassEntry.renderPassNode = KnownDynamicCast<OpenGLRenderPassNode*>((childNodes++)->get());
			renderPassEntry.sortIndex = (childNodeSortOrder != nullptr ? *(childNodeSortOrder++) : (int)i);
			_state[_buildIndex].renderPasses.insert(std::upper_bound(_state[_buildIndex].renderPasses.begin(), _state[_buildIndex].renderPasses.end(), renderPassEntry, RenderPassEntry::Sorter()), renderPassEntry);
		}
	}
	else
	{
		_state[_buildIndex].renderPasses.resize(childNodeCount);
		for (size_t i = 0; i < childNodeCount; ++i)
		{
			RenderPassEntry& renderPassEntry = _state[_buildIndex].renderPasses[i];
			renderPassEntry.renderPassNode = KnownDynamicCast<OpenGLRenderPassNode*>((childNodes++)->get());
			renderPassEntry.sortIndex = (int)i;
		}
	}
}

//----------------------------------------------------------------------------------------
void OpenGLRenderer::RemoveAllRenderPasses()
{
	std::lock_guard<std::mutex> lock(_buildStateMutex);
	_state[_buildIndex].renderPasses.clear();
}

//----------------------------------------------------------------------------------------
// Render methods
//----------------------------------------------------------------------------------------
void OpenGLRenderer::StartNewFrame()
{
	// Ensure a frame advance operation isn't already in progress
	std::unique_lock<std::mutex> lock(_renderThreadMutex);
	if (_frameAdvanceInProgress)
	{
		_log->Error("StartNewFrame was called when a previous call was still in progress");
		return;
	}
	_frameAdvanceInProgress = true;

	// Ensure the previous frame has finished rendering, and that all resources being freed from the last frame have
	// been cleaned up. We need to do this as we're about to overwrite state data from the last frame.
	while (_drawingInProgress || _swapRequestPending || _deleteLastDrawResourcesRequestPending || _earlyDeleteNextDrawResourcesRequestPending)
	{
		_notifyRenderThreadTaskComplete.wait(lock);
	}

	// Submit a build to draw request to the render thread if required, and wait for it to be processed. This will
	// also signal that a drawing operation is now in progress.
	if (_buildingInProgress)
	{
		_buildToDrawRequestPending = true;
		_notifyRenderThreadTaskPending.notify_all();
		while (_buildToDrawRequestPending)
		{
			_notifyRenderThreadTaskComplete.wait(lock);
		}
	}

	// Flag that we're beginning to build a new frame
	_buildingInProgress = true;

	// Signal that a frame advance operation is no longer in progress
	_frameAdvanceInProgress = false;
}

//----------------------------------------------------------------------------------------
void OpenGLRenderer::WaitForDrawComplete() const
{
	// Wait for any current drawing operation to complete
	std::unique_lock<std::mutex> lock(_renderThreadMutex);
	while (_drawingInProgress)
	{
		_notifyRenderThreadTaskComplete.wait(lock);
	}
}

//----------------------------------------------------------------------------------------
void OpenGLRenderer::WaitForOutputCaptureComplete() const
{
	// Wait for any current drawing operation to complete
	std::unique_lock<std::mutex> lock(_renderThreadMutex);
	while (_drawingInProgress)
	{
		_notifyRenderThreadTaskComplete.wait(lock);
	}

	// Flag to any framebuffer outputs that captured data in the frame we just completed a draw for that the application
	// is allowed to read the captured output from the live draw state. We can do this safely at this point as the
	// application has explicitly synchronized with the completion of framebuffer output capture, which it should be
	// noted is not necessarily complete when the draw process itself is complete. In the case of our renderer here it
	// currently is the same, so we use the same synchronization point.
	for (auto* entry : _capturedFramebufferOutputsInCurrentFrame)
	{
		entry->EnableCaptureReadFromCurrentDrawBuffer();
	}
#ifdef GL_VERSION_4_3
	for (auto* entry : _capturedDataArrayOutputsInCurrentFrame)
	{
		entry->EnableCaptureReadFromCurrentDrawBuffer();
	}
	for (auto* entry : _capturedTexelArrayOutputsInCurrentFrame)
	{
		entry->EnableCaptureReadFromCurrentDrawBuffer();
	}
#endif
}

//----------------------------------------------------------------------------------------
void OpenGLRenderer::WaitForDeferredDeletionComplete() const
{
	// Ensure the previous frame has finished rendering, and that all resources being freed from the last frame have
	// been cleaned up. Additionally, we also request early deletion of resources associated with the next frame. We
	// can do this safely, since we're synchronizing with the end of the previous frame first. This creates a safe
	// point at which external window resources can be freed without a new frame being sent to the renderer.
	std::unique_lock<std::mutex> lock(_renderThreadMutex);
	_earlyDeleteNextDrawResourcesRequestPending = true;
	_notifyRenderThreadTaskPending.notify_all();
	while (_drawingInProgress || _swapRequestPending || _deleteLastDrawResourcesRequestPending || _earlyDeleteNextDrawResourcesRequestPending)
	{
		_notifyRenderThreadTaskComplete.wait(lock);
	}
}

//----------------------------------------------------------------------------------------
void OpenGLRenderer::AddCurrentFrameBufferOutput(OpenGLFrameBufferOutput* frameBufferOutput)
{
	_capturedFramebufferOutputsInCurrentFrame.push_back(frameBufferOutput);
}

//----------------------------------------------------------------------------------------
void OpenGLRenderer::AddCurrentDataArrayOutput(OpenGLDataArrayOutput* resourceBufferOutput)
{
	_capturedDataArrayOutputsInCurrentFrame.push_back(resourceBufferOutput);
}

//----------------------------------------------------------------------------------------
void OpenGLRenderer::AddCurrentDataArray(OpenGLDataArray* resourceBuffer)
{
	_boundDataArrays.push_back(resourceBuffer);
}

//----------------------------------------------------------------------------------------
void OpenGLRenderer::AddCurrentTexelArrayOutput(OpenGLTexelArrayOutput* resourceBufferOutput)
{
	_capturedTexelArrayOutputsInCurrentFrame.push_back(resourceBufferOutput);
}

//----------------------------------------------------------------------------------------
void OpenGLRenderer::AddCurrentTexelArray(OpenGLTexelArray* resourceBuffer)
{
	_boundTexelArrays.push_back(resourceBuffer);
}

//----------------------------------------------------------------------------------------
void OpenGLRenderer::RenderThread()
{
	// Set the render thread ID
	std::unique_lock<std::mutex> lock(_renderThreadMutex);
	_renderThreadID = std::this_thread::get_id();

	// Start processing render thread tasks
	bool done = false;
	while (!done)
	{
		// Wait for work to be requested or the thread to be requested to terminate
		if (_pendingInvocations.empty() && !_initializeOpenGLRequestPending && !_buildToDrawRequestPending && !_renderRequestPending && !_swapRequestPending && !_deleteLastDrawResourcesRequestPending && !_earlyDeleteNextDrawResourcesRequestPending)
		{
			// If the render thread has not already been instructed to stop, suspend this thread until a render task is
			// issued or this thread is instructed to stop.
			if (_renderThreadActive)
			{
				_notifyRenderThreadTaskPending.wait(lock);
			}

			// If the render thread has been suspended, flag that we need to exit the render loop.
			done = !_renderThreadActive;

			// Begin the loop again
			continue;
		}

		// Action the next pending render request
		if (!_pendingInvocations.empty())
		{
			while (!_pendingInvocations.empty())
			{
				auto& callback = _pendingInvocations.front();
				lock.unlock();
				callback();
				lock.lock();
				_pendingInvocations.pop();
			}
		}
		if (_initializeOpenGLRequestPending)
		{
			lock.unlock();
			_initializeOpenGLResult = PerformInitializeOpenGL();
			lock.lock();
			_initializeOpenGLRequestPending = false;
		}
		else if (_deleteLastDrawResourcesRequestPending)
		{
			lock.unlock();
			PerformDeleteLastDrawResourcesOperation();
			lock.lock();
			_deleteLastDrawResourcesRequestPending = false;
		}
		else if (_buildToDrawRequestPending)
		{
			_drawingInProgress = true;
			lock.unlock();
			PerformPrepareBuildOperation();
			lock.lock();
			_buildToDrawRequestPending = false;
			_renderRequestPending = true;
		}
		else if (_renderRequestPending)
		{
			lock.unlock();
			PerformRenderOperation();
			lock.lock();
			_renderRequestPending = false;
			_swapRequestPending = true;
		}
		else if (_swapRequestPending)
		{
			lock.unlock();
			PerformSwapOperation();
			lock.lock();
			_swapRequestPending = false;
			_drawingInProgress = false;
			_deleteLastDrawResourcesRequestPending = true;
		}
		else if (_earlyDeleteNextDrawResourcesRequestPending)
		{
			// Note that there's a priority order here. This request must be processed after all the above.
			lock.unlock();
			PerformDeleteNextDrawResourcesOperation();
			lock.lock();
			_earlyDeleteNextDrawResourcesRequestPending = false;
		}

		// Notify any waiting threads that a render thread task has completed
		_notifyRenderThreadTaskComplete.notify_all();
	}

	// Release any remaining resources which need to be deallocated on this thread
	PerformFreeOpenGLResources();

	// Notify any waiting threads that the render thread has now terminated
	_notifyRenderThreadStopped.notify_all();
}

//----------------------------------------------------------------------------------------
bool OpenGLRenderer::PerformInitializeOpenGL()
{
	// Record if debug logging is enabled. We need to do this here, as we'll be relying on this flag being set correctly
	// when we create a rendering context further down.
	_enableDebugLogging = (_enabledOptions.find(Options::EnableDebugLogging) != _enabledOptions.end());

#ifdef OPENGL_USE_PLATFORM_WIN32
	// Bind to our dummy window. We only do this to allow us to create a rendering context. Note that this will form our
	// "base" (shared) rendering context. We will allocate all resources and perform all transfers under this context,
	// however during rendering we may switch to window-specific rendering contexts when binding to a window. This is
	// required, as rendering contexts can only be shared between windows which are using the same pixel format, which
	// would mean we couldn't vary depth/stencil buffer settings per window for example. See the following documentation
	// for more information:
	// https://www.khronos.org/opengl/wiki/Platform_specifics:_Windows#Multiple_Windows
	// With wglCreateContextAttribsARB, we don't have to go through the messy process of sharing each resource with each
	// rendering context. Instead, where we create each window-specific window context, we pass in the base rendering
	// context we create here as our shared context, allowing each window rendering context access to all global
	// resources.
	if (!SelectWindowPixelFormat(_dummyMainWindowHandle, IFrameBuffer::WindowColorSpaceMode::Default, IFrameBuffer::WindowDepthStencilMode::None, _deviceContext, _dummyWindowPixelFormatInfo))
	{
		_log->Error("SelectWindowPixelFormat failed for dummy OpenGL window");
		return false;
	}

	// Attempt to retrieve a rendering context for our dummy window. Note that ideally we wouldn't supply a device
	// context here at all, as we want there to be no default framebuffer bound. We have only bound a dummy window we
	// don't intend to draw to, so we don't want it to be used as a valid framebuffer for drawing purposes. There is the
	// following note in the description of WGL_ARB_create_context:
	// "If the OpenGL context version of <hglrc> is 3.0 or above, and if either the <hdc> parameter of wglMakeCurrent is
	// NULL, or both of the <hDrawDC> and <hReadDC> parameters of wglMakeContextCurrentARB are NULL, then the context is
	// made current, but with no default framebuffer defined."
	// Despite this statement however, testing has shown that wglMakeCurrent will fail with ERROR_INVALID_HANDLE when
	// NULL is passed in as HDC, on both Intel and AMD hardware.
	size_t renderingContextIndex;
	if (!RetrieveOrCreateRenderingContextForWindow(_dummyMainWindowHandle, _deviceContext, _dummyWindowPixelFormatInfo, _mainRenderingContext, renderingContextIndex, true))
	{
		_log->Error("RetrieveOrCreateRenderingContextForWindow failed for dummy OpenGL window");
		return false;
	}

	// Initiaize glad
	if (gladLoadGL() == 0)
	{
		_log->Error("Failed to initialize GLAD");
		return false;
	}

	// If we failed to locate the wglCreateContextAttribsARB method, it's entirely possible we've been supplied an
	// OpenGL implementation which is too old to meet our minimum requirements, as we weren't able to specify the target
	// version. This used to occur over Remote Desktop connections with GeForce drivers for example, when the Microsoft
	// software adapter would be loaded. As an imperfect yet effective test for this case, we attempt to retrieve the
	// value of "GL_MAX_UNIFORM_BUFFER_BINDINGS", which is only supported on OpenGL 3.1 or above. If this call fails
	// with GL_INVALID_ENUM, we know we're on an earlier version and can't continue.
	static auto wglCreateContextAttribsARB = reinterpret_cast<PFNWGLCREATECONTEXTATTRIBSARBPROC>(wglGetProcAddress("wglCreateContextAttribsARB"));
	if (wglCreateContextAttribsARB == nullptr)
	{
		GLint error = glGetError();
		int dummyRead = 0;
		glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &dummyRead);
		error = glGetError();
		if (error == GL_INVALID_ENUM)
		{
			_log->Error("Failed to initialize OpenGL. The graphics device doesn't appear to meet the minimum OpenGL version requirements.");
			return false;
		}
	}

	// Build a set of available extensions
	typedef const char*(WINAPI * PFNWGLGETEXTENSIONSSTRINGARBPROC)(HDC);
	auto wglGetExtensionsStringARB = reinterpret_cast<PFNWGLGETEXTENSIONSSTRINGARBPROC>(wglGetProcAddress("wglGetExtensionsStringARB"));
	if (wglGetExtensionsStringARB != nullptr)
	{
		auto extensionString = wglGetExtensionsStringARB(_deviceContext);
		if (extensionString != nullptr)
		{
			std::istringstream extensionStringStream(extensionString);
			std::string extensionName;
			while (extensionStringStream >> extensionName)
			{
				_availableExtensions.insert(extensionName);
			}
		}
	}

#elif defined(OPENGL_USE_EGL)
	// Ensure we were supplied with a platform display object
	if (_mainDisplay == EGL_NO_DISPLAY)
	{
		_log->Error("eglGetDisplay failed");
		return false;
	}

	// Initialize the EGL display object
	if (eglInitialize(_mainDisplay, nullptr, nullptr) != EGL_TRUE)
	{
		CheckEGLError(_log.get());
		_log->Error("eglInitialize failed");
		return false;
	}

	// Select the OpenGL API for following EGL calls
	if (eglBindAPI(EGL_OPENGL_API) != EGL_TRUE)
	{
		CheckEGLError(_log.get());
		_log->Error("eglBindAPI failed");
		return false;
	}

	// Select a pixel format
	if (!OpenGLRenderer::SelectWindowPixelFormat(_mainDisplay, IFrameBuffer::WindowColorSpaceMode::Default, IFrameBuffer::WindowDepthStencilMode::None, _dummyWindowPixelFormatInfo, true, *_log, false))
	{
		_log->Error("Failed to find a compatible pixel format");
		return false;
	}

	// We assume EGL 1.5 support here, and we're relying on the previous "EGL_KHR_surfaceless_context" extension, now
	// core, which allows us to create a rendering context with no bound surface. Since this is just our main context,
	// which does no rendering, this is exactly what we want. Note that we can't use a pbuffer here, as EGL doesn't
	// support pbuffer surface creation on Wayland.
	_dummySurface = EGL_NO_SURFACE;

	// Attempt to retrieve a rendering context for our dummy window
	size_t renderingContextIndex;
	if (!RetrieveOrCreateRenderingContextForWindow(_mainDisplay, _dummySurface, _dummyWindowPixelFormatInfo, _mainRenderingContext, renderingContextIndex, true))
	{
		_log->Error("RetrieveOrCreateRenderingContextForWindow failed for dummy OpenGL window");
		return false;
	}

	// Initialize glad
	if (gladLoadGLLoader(reinterpret_cast<GLADloadproc>(eglGetProcAddress)) == 0)
	{
		_log->Error("Failed to initialize GLAD");
		eglMakeCurrent(_mainDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		CheckEGLError(_log.get());
		return false;
	}

	// Build a set of available extensions
	auto extensionString = eglQueryString(_mainDisplay, EGL_EXTENSIONS);
	if (extensionString != nullptr)
	{
		std::istringstream extensionStringStream(extensionString);
		std::string extensionName;
		while (extensionStringStream >> extensionName)
		{
			_availableExtensions.insert(extensionName);
		}
	}
	CheckEGLError(_log.get());

#elif defined(OPENGL_USE_PLATFORM_XLIB)
	// Select a pixel format
	if (!SelectWindowPixelFormat(_xDisplay, IFrameBuffer::WindowColorSpaceMode::Default, IFrameBuffer::WindowDepthStencilMode::None, _dummyWindowPixelFormatInfo))
	{
		_log->Error("SelectWindowPixelFormat failed for dummy OpenGL window");
		return false;
	}

	// Create a dummy X11 window for the shared context
	const int screen = DefaultScreen(_xDisplay);
	::Window root = RootWindow(_xDisplay, screen);
	_dummyWindowColormap = XCreateColormap(_xDisplay, root, _dummyWindowPixelFormatInfo.visualInfo.visual, X11Constants::kAllocNone);
	XSetWindowAttributes windowAttributes = {};
	windowAttributes.colormap = _dummyWindowColormap;
	_dummyWindow = XCreateWindow(_xDisplay, root, 0, 0, 1, 1, 0, _dummyWindowPixelFormatInfo.visualInfo.depth, X11Constants::kInputOutput, _dummyWindowPixelFormatInfo.visualInfo.visual, X11Constants::kCWBorderPixel | X11Constants::kCWColormap | X11Constants::kCWEventMask, &windowAttributes);

	// Attempt to retrieve a rendering context for our dummy window
	size_t renderingContextIndex;
	if (!RetrieveOrCreateRenderingContextForWindow(_xDisplay, _dummyWindow, _dummyWindowPixelFormatInfo, _mainRenderingContext, renderingContextIndex, true))
	{
		_log->Error("RetrieveOrCreateRenderingContextForWindow failed for dummy OpenGL window");
		return false;
	}

	// Initiaize glad
	if (gladLoadGL() == 0)
	{
		_log->Error("Failed to initialize GLAD");
		return false;
	}

	// Build a set of available extensions
	auto extensionString = glXQueryExtensionsString(_xDisplay, screen);
	if (extensionString != nullptr)
	{
		std::istringstream extensionStringStream(extensionString);
		std::string extensionName;
		while (extensionStringStream >> extensionName)
		{
			_availableExtensions.insert(extensionName);
		}
	}

#elif defined(OPENGL_USE_PLATFORM_APPKIT)
	// Select a pixel format
	if (!SelectPixelFormat(IFrameBuffer::WindowColorSpaceMode::Default, IFrameBuffer::WindowDepthStencilMode::None, _dummyWindowPixelFormatInfo))
	{
		_log->Error("SelectPixelFormat failed for main rendering context");
		return false;
	}

	// Attempt to create a main rendering context
	size_t renderingContextIndex;
	if (!RetrieveOrCreateRenderingContextForWindow(nullptr, _dummyWindowPixelFormatInfo, _mainRenderingContext, renderingContextIndex, true))
	{
		_log->Error("RetrieveOrCreateRenderingContext failed for main rendering context");
		return false;
	}

	// Initiaize glad
	if (gladLoadGL() == 0)
	{
		_log->Error("Failed to initialize GLAD");
		return false;
	}

	// Build a set of available extensions
	GLint extensionCount = 0;
	glGetIntegerv(GL_NUM_EXTENSIONS, &extensionCount);
	for (GLint i = 0; i < extensionCount; ++i)
	{
		std::string extensionName = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i));
		_availableExtensions.insert(extensionName);
	}
#endif

	// Log the actual OpenGL vendor and device name obtained if debug logging is enabled
	if (_enableDebugLogging)
	{
		const auto* vendorStringRaw = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
		const auto* rendererStringRaw = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
		std::string vendorNameAsString = (vendorStringRaw != nullptr ? vendorStringRaw : "Unknown");
		std::string deviceNameAsString = (rendererStringRaw != nullptr ? rendererStringRaw : "Unknown");
		_log->Info("Actual OpenGL Vendor: {0}", vendorNameAsString);
		_log->Info("Actual OpenGL Device: {0}", deviceNameAsString);
	}

	// Retrieve limits on uniform buffers
	glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &_uniformBufferOffsetAlignment);
	glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &_maxUniformBlockSize);
	glGetIntegerv(GL_MAX_VERTEX_UNIFORM_BLOCKS, &_maxVertexShaderUniformBlocks);
	glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_BLOCKS, &_maxFragmentShaderUniformBlocks);
	glGetIntegerv(GL_MAX_GEOMETRY_UNIFORM_BLOCKS, &_maxGeometryShaderUniformBlocks);
#ifdef GL_VERSION_4_3
	glGetIntegerv(GL_MAX_COMPUTE_UNIFORM_BLOCKS, &_maxComputeShaderUniformBlocks);
#endif

	// Retrieve limits on textures and samplers
#ifdef GL_EXT_texture_filter_anisotropic
	if (GLAD_GL_EXT_texture_filter_anisotropic != 0)
	{
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &_maxTextureSamplerAnisotropy);
	}
#endif

	// Flag render markers as active if requested and supported
#ifdef GL_KHR_debug
	if ((GLAD_GL_KHR_debug != 0) && _enabledOptions.find(Options::EnableRenderMarkers) != _enabledOptions.end())
	{
		_useRenderMarkers = true;
	}
#endif
	return true;
}

//----------------------------------------------------------------------------------------
void OpenGLRenderer::PerformFreeOpenGLResources()
{
	// Delete any objects which are pending deletion
	PerformDeleteLastDrawResourcesOperation();
	PerformDeleteNextDrawResourcesOperation();

#ifdef OPENGL_USE_PLATFORM_WIN32
	// Release the current OpenGL rendering context
	wglMakeCurrent(nullptr, nullptr);

	// Delete all allocated rendering contexts
	for (const auto& entry : _allocatedRenderingContexts)
	{
		if (entry.slotAllocated)
		{
			wglDeleteContext(entry.renderingContext);
		}
	}
	_allocatedRenderingContexts.clear();
	_mainRenderingContext = nullptr;

	// Release resources associated with the dummy main window
	if (_deviceContext != nullptr)
	{
		ReleaseDC(_dummyMainWindowHandle, _deviceContext);
		_deviceContext = nullptr;
	}
	if (_dummyMainWindowHandle != nullptr)
	{
		DestroyWindow(_dummyMainWindowHandle);
		_dummyMainWindowHandle = nullptr;
	}

#elif defined(OPENGL_USE_EGL)
	if (_mainDisplay != EGL_NO_DISPLAY)
	{
		// Release the current OpenGL rendering context
		eglMakeCurrent(_mainDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

		// Delete all allocated rendering contexts
		for (const auto& entry : _allocatedRenderingContexts)
		{
			if (entry.slotAllocated)
			{
				eglDestroyContext(entry.display, entry.renderingContext);
			}
		}
		_allocatedRenderingContexts.clear();
		_mainRenderingContext = nullptr;

		// Release resources associated with the dummy main window. Note that _dummySurface should always be set to
		// EGL_NO_SURFACE, so there's nothing to free.
		_dummySurface = EGL_NO_SURFACE;
		eglTerminate(_mainDisplay);
		_mainDisplay = EGL_NO_DISPLAY;
	}

	// If we opened a GBM device for headless rendering, close it now.
	if (_gbmDevice != nullptr)
	{
		gbm_device_destroy(_gbmDevice);
		_gbmDevice = nullptr;
	}
	if (_gbmDeviceFileHandle >= 0)
	{
		close(_gbmDeviceFileHandle);
		_gbmDeviceFileHandle = -1;
	}

#elif defined(OPENGL_USE_PLATFORM_XLIB)
	// Release the current OpenGL rendering context
	if (_xDisplay != nullptr)
	{
		glXMakeContextCurrent(_xDisplay, X11Constants::kNone, X11Constants::kNone, nullptr);
	}

	// Delete all allocated rendering contexts
	for (const auto& entry : _allocatedRenderingContexts)
	{
		if (entry.slotAllocated)
		{
			glXDestroyContext(entry.display, entry.renderingContext);
		}
	}
	_allocatedRenderingContexts.clear();
	_mainRenderingContext = nullptr;

	// Release resources associated with the dummy main window
	if (_dummyWindow != 0)
	{
		XDestroyWindow(_xDisplay, _dummyWindow);
		XFreeColormap(_xDisplay, _dummyWindowColormap);
		_dummyWindow = 0;
	}
	_xDisplay = nullptr;

#elif defined(OPENGL_USE_PLATFORM_APPKIT)
	// Release the current OpenGL rendering context
	CGLSetCurrentContext(nullptr);

	// Delete all allocated rendering contexts
	for (const auto& entry : _allocatedRenderingContexts)
	{
		if (entry.slotAllocated)
		{
			CGLDestroyContext(entry.renderingContext);
		}
	}
	_allocatedRenderingContexts.clear();
	_mainRenderingContext = nullptr;
#endif
}

//----------------------------------------------------------------------------------------
void OpenGLRenderer::PerformPrepareBuildOperation()
{
	// Transition our build state to our draw state
	std::swap(_buildIndex, _drawIndex);
	_state[_buildIndex].renderPasses = _state[_drawIndex].renderPasses;
	_state[_buildIndex].migrateStatePendingObjects.clear();
	_state[_buildIndex].bufferUpdatePendingObjects.clear();
#ifdef GL_VERSION_4_3
	_state[_buildIndex].bufferTransferPendingObjects.clear();
#endif

	// Transfer build state into draw state in our scene tree
	for (const RenderPassEntry& renderPassEntry : _state[_drawIndex].renderPasses)
	{
		renderPassEntry.renderPassNode->MigrateBuildStateToDrawState();
	}

	// Transfer build state info draw state in our modified standalone objects
	for (const auto& entry : _state[_drawIndex].migrateStatePendingObjects)
	{
		std::visit([](auto* object) { object->MigrateBuildStateToDrawState(); }, entry);
	}
}

//----------------------------------------------------------------------------------------
void OpenGLRenderer::PerformRenderOperation()
{
	// Check for any previous errors coming into the render operation
	CheckGLError(_log.get());

	// Perform pre-deletion of framebuffer resources. We need this to allow a framebuffer to replace a previous
	// framebuffer while binding to the same window surface on some platforms. This hook gives us an opportunity to free
	// window resources prior to the new framebuffer attempting to bind to the window, while also knowing that the
	// previous framebuffer resources are no longer in use.
	for (const auto& entry : _state[_drawIndex].deletePendingObjects)
	{
		if (std::holds_alternative<OpenGLFrameBuffer*>(entry))
		{
			auto framebuffer = *std::get_if<OpenGLFrameBuffer*>(&entry);
			framebuffer->PerformPreDelete();
		}
	}

	// Push a render marker if required
#ifdef GL_VERSION_4_3
	if (_useRenderMarkers)
	{
		glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, RenderMarkerId, -1, "Setup");
	}
#endif

#ifdef GL_VERSION_4_3
	// Initiate any pending data transfer operations within GPU memory
	for (const auto& entry : _state[_drawIndex].bufferTransferPendingObjects)
	{
		std::visit([](auto* object) { object->CompletePendingDataTransfers(); }, entry);
	}
#endif

	// Initiate any pending data transfer operations from CPU to GPU memory
	for (const auto& entry : _state[_drawIndex].bufferUpdatePendingObjects)
	{
		std::visit([](auto* object) { object->CompletePendingDataWrites(); }, entry);
	}
	CheckGLError(_log.get());

	// Pop a render marker if required
#ifdef GL_VERSION_4_3
	if (_useRenderMarkers)
	{
		glPopDebugGroup();
	}
#endif

	// Traverse the render tree and perform our draw operations
	_captureTargetsPresent = false;
	_boundWindowFramebuffers.clear();
	_boundTextureFramebuffers.clear();
	_boundDataArrays.clear();
	_boundTexelArrays.clear();
	size_t renderingContextIndex = 0;
	size_t renderingContextGenerationIndex = 0;
	bool lastCommandWasCompute = false;
	OpenGLShaderProgram* currentShaderProgram = nullptr;
	OpenGLFrameBuffer* currentFramebuffer = nullptr;
	const std::vector<RenderPassEntry>& renderPassEntries = _state[_drawIndex].renderPasses;
	for (const RenderPassEntry& renderPassEntry : renderPassEntries)
	{
		// If this render pass is disabled, skip it.
		OpenGLRenderPassNode* renderPassNode = renderPassEntry.renderPassNode;
		if (!renderPassNode->IsEnabled())
		{
			continue;
		}

		// If we last did compute work, put a minimal barrier in place before binding the new framebuffer, to ensure
		// any changes which could affect the framebuffer binding are visible.
#ifdef GL_VERSION_4_3
		if (lastCommandWasCompute)
		{
			glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		}
#endif

		// Bind the framebuffer for this render pass
		OpenGLFrameBuffer* newFramebuffer = renderPassNode->GetFrameBuffer();
		if (newFramebuffer != currentFramebuffer)
		{
			// Restore the default rendering context if we're switching from a bound window to offscreen
			bool defaultRenderingContextCurrentlyActive = ((currentFramebuffer == nullptr) || !currentFramebuffer->IsBoundToWindow());
			bool newFramebufferUsesDefaultRenderingContext = ((newFramebuffer == nullptr) || !newFramebuffer->IsBoundToWindow());
			if (newFramebufferUsesDefaultRenderingContext && !defaultRenderingContextCurrentlyActive)
			{
#ifdef OPENGL_USE_PLATFORM_WIN32
				ActivateRenderingContext(_dummyMainWindowHandle, _deviceContext, _mainRenderingContext);
#elif defined(OPENGL_USE_EGL)
				ActivateRenderingContext(_mainDisplay, _dummySurface, _mainRenderingContext);
#elif defined(OPENGL_USE_PLATFORM_XLIB)
				ActivateRenderingContext(_xDisplay, _dummyWindow, _mainRenderingContext);
#elif defined(OPENGL_USE_PLATFORM_APPKIT)
				ActivateRenderingContext(_mainRenderingContext);
#endif
				renderingContextIndex = 0;
				renderingContextGenerationIndex = 0;
			}

#ifdef __APPLE__
			// On old Intel macOS OpenGL drivers, texture-backed framebuffer clears can remain invisible when the
			// texture is sampled immediately after switching to a window rendering context. This is a definite driver
			// bug, but we work around it here. A direct framebuffer read from the offscreen context makes the correct
			// contents visible. This was confimed on testing with Intel Iris Pro hardware under macOS Monterey. Since
			// this is a narrow macOS specific fix for legacy hardware, we only do it here on Mac systems when an Intel
			// graphics device has been detected. Newer systems all run Apple silicon and don't exhibit this bug, nor was
			// it observed on AMD discrete graphics devices on the same legacy system.
			if (_flushFramebufferTextureWritesBeforeWindowSampling && !newFramebufferUsesDefaultRenderingContext && defaultRenderingContextCurrentlyActive)
			{
				for (OpenGLFrameBuffer* frameBuffer : _boundTextureFramebuffers)
				{
					frameBuffer->FlushTextureAttachmentWritesForSampling();
				}
			}
#endif

			// Bind the new framebuffer
			if (newFramebuffer != nullptr)
			{
				// Bind the new framebuffer. Note that this will activate the rendering context if the new framebuffer
				// is bound to a window.
				newFramebuffer->BindFrameBuffer(_mainRenderingContext);

				// Add this framebuffer to the list of used framebuffers
				if (newFramebuffer->IsBoundToWindow())
				{
					renderingContextIndex = newFramebuffer->GetWindowRenderingContextIndex();
#ifdef OPENGL_USE_PLATFORM_APPKIT
					std::unique_lock<std::mutex> lock(_renderingContextMutex);
#endif
					renderingContextGenerationIndex = _allocatedRenderingContexts[renderingContextIndex].generationIndex;
					_boundWindowFramebuffers.push_back(newFramebuffer);
				}
				else
				{
					renderingContextIndex = 0;
					renderingContextGenerationIndex = 0;
					_boundTextureFramebuffers.push_back(newFramebuffer);
				}

				// Record if we've encountered a capture target
				_captureTargetsPresent |= newFramebuffer->HasCaptureTargets();
			}

			// Set the new framebuffer as current
			currentFramebuffer = newFramebuffer;

			// Mark no shader program as current, since we might have switched rendering contexts here. Since the bound
			// shader program is context-specific, it's theoretically possible two different contexts could try and use
			// the same program in two consecutive render passes, and we need to re-bind the program when the second
			// context is swapped in.
			currentShaderProgram = nullptr;
		}

		// Push a render marker if required. Note that render markers are per-context state, so we need to set it here
		// after binding the framebuffer to keep things consistent.
#ifdef GL_VERSION_4_3
		if (_useRenderMarkers)
		{
			glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, RenderMarkerId, -1, renderPassNode->DebugName().c_str());
		}
#endif

		// Apply fixed state from this render pass node
		if (currentFramebuffer != nullptr)
		{
			renderPassNode->ApplyFixedState(currentFramebuffer);
		}

		// Render each child node
		const std::vector<OpenGLRenderPassNode::ChildNodeEntry>& programNodeEntries = renderPassNode->GetChildNodes();
		for (const OpenGLRenderPassNode::ChildNodeEntry& childNodeEntry : programNodeEntries)
		{
			// If there are no child nodes in this program node, skip it.
			OpenGLProgramNode* programNode = childNodeEntry.node;
			const std::vector<OpenGLStateGroupNode*>& groupNodes = programNode->GetChildNodes();
			if (groupNodes.empty())
			{
				continue;
			}

			// Push a render marker if required
#ifdef GL_VERSION_4_3
			if (_useRenderMarkers)
			{
				glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, RenderMarkerId, -1, programNode->DebugName().c_str());
			}
#endif

			// Bind the shader program for this program node
			OpenGLShaderProgram* shaderProgram = programNode->GetShaderProgram();
			if (shaderProgram != currentShaderProgram)
			{
				shaderProgram->BindShaderProgram();
				currentShaderProgram = shaderProgram;
			}

			// Bind any textures or state buffer entries set as defaults at the render pass level
			OpenGLDefaultState* defaultState = childNodeEntry.defaultState;
			const std::vector<ITextureBindingInfo*>* defaultTextureEntries = nullptr;
			const std::vector<StateBufferBindingInfo*>* defaultStateBufferEntries = nullptr;
			const std::vector<ResourceArrayBindingInfo*>* defaultResourceBufferEntries = nullptr;
			const std::vector<IStateValueInfo*>* defaultStateValueEntries = nullptr;
			if (defaultState != nullptr)
			{
				defaultTextureEntries = &defaultState->GetTextureEntries();
				defaultStateBufferEntries = &defaultState->GetStateBufferEntries();
				defaultResourceBufferEntries = &defaultState->GetResourceBufferEntries();
				defaultStateValueEntries = &defaultState->GetValueEntries();
			}
			bool hasDefaultTextureEntries = (defaultTextureEntries != nullptr) && !defaultTextureEntries->empty();
			bool hasDefaultStateBufferEntries = (defaultStateBufferEntries != nullptr) && !defaultStateBufferEntries->empty();
			bool hasDefaultResourceBufferEntries = (defaultResourceBufferEntries != nullptr) && !defaultResourceBufferEntries->empty();
			bool hasDefaultStateEntries = (defaultStateValueEntries != nullptr) && !defaultStateValueEntries->empty();
			if (hasDefaultTextureEntries)
			{
				BindTextures(*defaultTextureEntries, shaderProgram);
			}
			if (hasDefaultStateBufferEntries)
			{
				BindStateBuffers(*defaultStateBufferEntries, shaderProgram);
			}
			if (hasDefaultResourceBufferEntries)
			{
				BindResourceArrays(*defaultResourceBufferEntries, shaderProgram, true);
			}

			// Apply constant state values for this program node
			const auto& constantValueEntries = programNode->GetConstantValueEntries();
			shaderProgram->RestoreGlobalConstantBufferBaseline();
			for (IStateValueInfo* constantValue : constantValueEntries)
			{
				constantValue->ApplyValue(shaderProgram);
			}

			// Push the current global constant buffer state so we can restore it later
			shaderProgram->PushGlobalConstantBufferState(&constantValueEntries);

			// Apply default state values from the program node binding at the render pass level
			if (hasDefaultStateEntries)
			{
				for (const auto& stateValue : *defaultStateValueEntries)
				{
					stateValue->ApplyValue(shaderProgram);
				}
				shaderProgram->PushGlobalConstantBufferState(defaultStateValueEntries);
			}

			// Render each child node
			for (OpenGLStateGroupNode* groupNode : groupNodes)
			{
				// If this state group node is empty, skip it.
				bool isComputeTask = groupNode->HasComputeTask();
				const std::vector<OpenGLRenderableNode*>& renderableNodes = groupNode->GetChildNodes();
				if (!isComputeTask && renderableNodes.empty())
				{
					continue;
				}

				// Push a render marker if required
#ifdef GL_VERSION_4_3
				if (_useRenderMarkers)
				{
					glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, RenderMarkerId, -1, groupNode->DebugName().c_str());
				}
#endif

				// Apply fixed state associated with this state group node
				if (!isComputeTask)
				{
					groupNode->ApplyFixedState(currentFramebuffer);
				}

				// Apply state values for this state group node
				const auto& stateValueEntriesForGroupNode = groupNode->GetValueEntries();
				for (const auto& stateValue : stateValueEntriesForGroupNode)
				{
					stateValue->ApplyValue(shaderProgram);
				}

				// Now that we've applied state values, push the current global constant buffer state so we can restore
				// it later.
				shaderProgram->PushGlobalConstantBufferState(&stateValueEntriesForGroupNode);

				// Bind any textures or state buffer entries set on this state node
				const auto& textureEntriesForGroupNode = groupNode->GetTextureEntries();
				const auto& stateBufferEntriesForGroupNode = groupNode->GetStateBufferEntries();
				const auto& resourceBufferEntriesForGroupNode = groupNode->GetResourceBufferEntries();
				bool hasTextureEntriesForStateGroupNode = !textureEntriesForGroupNode.empty();
				bool hasStateBufferEntriesForStateGroupNode = !stateBufferEntriesForGroupNode.empty();
				bool hasResourceBufferEntriesForStateGroupNode = !resourceBufferEntriesForGroupNode.empty();
				if (hasTextureEntriesForStateGroupNode)
				{
					if (hasDefaultTextureEntries)
					{
						BindTextures(*defaultTextureEntries, shaderProgram);
					}
					BindTextures(textureEntriesForGroupNode, shaderProgram);
				}
				if (hasStateBufferEntriesForStateGroupNode)
				{
					if (hasDefaultStateBufferEntries)
					{
						BindStateBuffers(*defaultStateBufferEntries, shaderProgram);
					}
					BindStateBuffers(stateBufferEntriesForGroupNode, shaderProgram);
				}
				if (hasResourceBufferEntriesForStateGroupNode)
				{
					if (hasDefaultResourceBufferEntries)
					{
						BindResourceArrays(*defaultResourceBufferEntries, shaderProgram, false);
					}
					BindResourceArrays(resourceBufferEntriesForGroupNode, shaderProgram, true);
				}

#ifdef GL_VERSION_4_3
				// If this group node is performing a compute task, execute it now.
				if (isComputeTask)
				{
					// Previous graphics or compute work may have written shader-visible memory. Make those writes
					// visible before this compute dispatch consumes them.
					glMemoryBarrier(
					  GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT |
					  GL_ELEMENT_ARRAY_BARRIER_BIT |
					  GL_UNIFORM_BARRIER_BIT |
					  GL_SHADER_STORAGE_BARRIER_BIT |
					  GL_TEXTURE_FETCH_BARRIER_BIT |
					  GL_FRAMEBUFFER_BARRIER_BIT |
					  GL_COMMAND_BARRIER_BIT |
					  GL_ATOMIC_COUNTER_BARRIER_BIT |
					  GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT |
					  GL_PIXEL_BUFFER_BARRIER_BIT |
					  GL_TEXTURE_UPDATE_BARRIER_BIT |
					  GL_BUFFER_UPDATE_BARRIER_BIT |
					  GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
					CheckGLError(_log.get());

					// Execute the compute task
					auto threadGroupCounts = groupNode->GetComputeThreadGroupCounts();
					glDispatchCompute(threadGroupCounts.X(), threadGroupCounts.Y(), threadGroupCounts.Z());
					lastCommandWasCompute = true;
				}
				else if (lastCommandWasCompute)
				{
					// If we last issued compute commands and we're transitoning to graphics commands, we need to insert
					// a memory barrier before we begin, to ensure memory state is properly synchronized. Note that we
					// have already tested above to ensure that there is at least one renderable node present here, so
					// we know a draw command is indeed going to follow here.
					glMemoryBarrier(
					  GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT |
					  GL_ELEMENT_ARRAY_BARRIER_BIT |
					  GL_UNIFORM_BARRIER_BIT |
					  GL_SHADER_STORAGE_BARRIER_BIT |
					  GL_TEXTURE_FETCH_BARRIER_BIT |
					  GL_FRAMEBUFFER_BARRIER_BIT |
					  GL_COMMAND_BARRIER_BIT |
					  GL_ATOMIC_COUNTER_BARRIER_BIT |
					  GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT |
					  GL_PIXEL_BUFFER_BARRIER_BIT |
					  GL_TEXTURE_UPDATE_BARRIER_BIT |
					  GL_BUFFER_UPDATE_BARRIER_BIT |
					  GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
					CheckGLError(_log.get());
					lastCommandWasCompute = false;
				}
#endif

				// Render each child node
				for (OpenGLRenderableNode* renderableNode : renderableNodes)
				{
					// Push a render marker if required
#ifdef GL_VERSION_4_3
					if (_useRenderMarkers)
					{
						glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_MARKER, RenderMarkerId, GL_DEBUG_SEVERITY_NOTIFICATION, -1, renderableNode->DebugName().c_str());
					}
#endif

					// Apply state values for this renderable node
					const auto& stateValueEntriesForRenderableNode = renderableNode->GetValueEntries();
					bool hasStateValues = !stateValueEntriesForRenderableNode.empty();
					if (hasStateValues)
					{
						for (const auto& stateValue : stateValueEntriesForRenderableNode)
						{
							stateValue->ApplyValue(shaderProgram);
						}
					}

					// Bind any textures or state buffer entries set on this renderable node
					const auto& textureEntriesForRenderableNode = renderableNode->GetTextureEntries();
					const auto& stateBufferEntriesForRenderableNode = renderableNode->GetStateBufferEntries();
					const auto& resourceBufferEntriesForRenderableNode = renderableNode->GetResourceBufferEntries();
					bool hasTextureEntries = !textureEntriesForRenderableNode.empty();
					bool hasStateBufferEntries = !stateBufferEntriesForRenderableNode.empty();
					bool hasResourceBufferEntries = !resourceBufferEntriesForRenderableNode.empty();
					if (hasTextureEntries)
					{
						BindTextures(textureEntriesForRenderableNode, shaderProgram);
					}
					if (hasStateBufferEntries)
					{
						BindStateBuffers(stateBufferEntriesForRenderableNode, shaderProgram);
					}
					if (hasResourceBufferEntries)
					{
						BindResourceArrays(resourceBufferEntriesForRenderableNode, shaderProgram, true);
					}

					// Draw this renderable object
					renderableNode->Draw(shaderProgram, renderingContextIndex, renderingContextGenerationIndex);

					// If we changed state values for this renderable, restore it to the pushed state baseline.
					if (hasStateValues)
					{
						shaderProgram->RestoreGlobalConstantBufferBaseline();
					}

					// Unbind any textures or state buffer entries set on this renderable node, and restore the default
					// bindings. We need to do this to ensure resources are free, even when rendering contexts switch,
					// IE, a texture bound as a framebuffer target which is subsequently bound as a shader resource.
					if (hasTextureEntries)
					{
						UnbindTextures(textureEntriesForRenderableNode, shaderProgram);
						if (hasDefaultTextureEntries)
						{
							BindTextures(*defaultTextureEntries, shaderProgram);
						}
						if (hasTextureEntriesForStateGroupNode)
						{
							BindTextures(textureEntriesForGroupNode, shaderProgram);
						}
					}
					if (hasStateBufferEntries)
					{
						UnbindStateBuffers(stateBufferEntriesForRenderableNode, shaderProgram);
						if (hasDefaultStateBufferEntries)
						{
							BindStateBuffers(*defaultStateBufferEntries, shaderProgram);
						}
						if (hasStateBufferEntriesForStateGroupNode)
						{
							BindStateBuffers(stateBufferEntriesForGroupNode, shaderProgram);
						}
					}
					if (hasResourceBufferEntries)
					{
						UnbindResourceArrays(resourceBufferEntriesForRenderableNode, shaderProgram);
						if (hasDefaultResourceBufferEntries)
						{
							BindResourceArrays(*defaultResourceBufferEntries, shaderProgram, false);
						}
						if (hasResourceBufferEntriesForStateGroupNode)
						{
							BindResourceArrays(resourceBufferEntriesForGroupNode, shaderProgram, false);
						}
					}
				}

				// Unbind any textures or state buffer entries set on this state node, and restore the default bindings.
				// We need to do this to ensure resources are free, even when rendering contexts switch, IE, a texture
				// bound as a framebuffer target which is subsequently bound as a shader resource.
				if (hasTextureEntriesForStateGroupNode)
				{
					UnbindTextures(textureEntriesForGroupNode, shaderProgram);
					if (hasDefaultTextureEntries)
					{
						BindTextures(*defaultTextureEntries, shaderProgram);
					}
				}
				if (hasStateBufferEntriesForStateGroupNode)
				{
					UnbindStateBuffers(stateBufferEntriesForGroupNode, shaderProgram);
					if (hasDefaultStateBufferEntries)
					{
						BindStateBuffers(*defaultStateBufferEntries, shaderProgram);
					}
				}
				if (hasResourceBufferEntriesForStateGroupNode)
				{
					UnbindResourceArrays(resourceBufferEntriesForGroupNode, shaderProgram);
					if (hasDefaultResourceBufferEntries)
					{
						BindResourceArrays(*defaultResourceBufferEntries, shaderProgram, false);
					}
				}

				// Pop the global constant buffer state
				shaderProgram->PopGlobalConstantBufferState();

				// Pop a render marker if required
#ifdef GL_VERSION_4_3
				if (_useRenderMarkers)
				{
					glPopDebugGroup();
				}
#endif
			}

			// Pop the global constant buffer state
			shaderProgram->PopGlobalConstantBufferState();
			if (hasDefaultStateEntries)
			{
				shaderProgram->PopGlobalConstantBufferState();
			}

			// Unbind any default textures or state buffer entries set on this program node. We need to do this to
			// ensure resources are free, even when rendering contexts switch, IE, a texture bound as a framebuffer
			// target which is subsequently bound as a shader resource.
			if (hasDefaultTextureEntries)
			{
				UnbindTextures(*defaultTextureEntries, shaderProgram);
			}
			if (hasDefaultStateBufferEntries)
			{
				UnbindStateBuffers(*defaultStateBufferEntries, shaderProgram);
			}
			if (hasDefaultResourceBufferEntries)
			{
				UnbindResourceArrays(*defaultResourceBufferEntries, shaderProgram);
			}

			// Pop a render marker if required
#ifdef GL_VERSION_4_3
			if (_useRenderMarkers)
			{
				glPopDebugGroup();
			}
#endif
		}

		// Perform any unbind operations for the render pass node. Currently this is limited to multisample framebuffer
		// resolution.
		renderPassNode->PerformUnbindOperations();

		// Pop a render marker if required
#ifdef GL_VERSION_4_3
		if (_useRenderMarkers)
		{
			glPopDebugGroup();
		}
#endif
	}

	// If we last issued compute commands, insert a final memory barrier to ensure the results of that compute operation
	// are fully visible.
	if (lastCommandWasCompute)
	{
		glMemoryBarrier(
		  GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT |
		  GL_ELEMENT_ARRAY_BARRIER_BIT |
		  GL_UNIFORM_BARRIER_BIT |
		  GL_SHADER_STORAGE_BARRIER_BIT |
		  GL_TEXTURE_FETCH_BARRIER_BIT |
		  GL_FRAMEBUFFER_BARRIER_BIT |
		  GL_COMMAND_BARRIER_BIT |
		  GL_ATOMIC_COUNTER_BARRIER_BIT |
		  GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT |
		  GL_PIXEL_BUFFER_BARRIER_BIT |
		  GL_TEXTURE_UPDATE_BARRIER_BIT |
		  GL_BUFFER_UPDATE_BARRIER_BIT |
		  GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		CheckGLError(_log.get());
		lastCommandWasCompute = false;
	}

	// Check for errors now that all content has been submitted for drawing
	CheckGLError(_log.get());

	// If we were last bound to a window framebuffer, re-activate our generic base rendering context.
	if ((currentFramebuffer != nullptr) && currentFramebuffer->IsBoundToWindow())
	{
#ifdef OPENGL_USE_PLATFORM_WIN32
		ActivateRenderingContext(_dummyMainWindowHandle, _deviceContext, _mainRenderingContext);
#elif defined(OPENGL_USE_EGL)
		ActivateRenderingContext(_mainDisplay, _dummySurface, _mainRenderingContext);
#elif defined(OPENGL_USE_PLATFORM_XLIB)
		ActivateRenderingContext(_xDisplay, _dummyWindow, _mainRenderingContext);
#elif defined(OPENGL_USE_PLATFORM_APPKIT)
		ActivateRenderingContext(_mainRenderingContext);
#endif
	}

	// If we have at least one framebuffer capture target, and we're using pixel buffer objects to perform the capture,
	// begin the capture process now.
	bool usingPixelBufferObjectsForCapture = UsePixelBufferObjectsForFrameCapture();
	_capturedFramebufferOutputsInCurrentFrame.clear();
	_capturedDataArrayOutputsInCurrentFrame.clear();
	_capturedTexelArrayOutputsInCurrentFrame.clear();
	if (_captureTargetsPresent && usingPixelBufferObjectsForCapture)
	{
		// Begin the capture process for each window framebuffer output
		for (OpenGLFrameBuffer* frameBuffer : _boundWindowFramebuffers)
		{
#ifdef OPENGL_USE_PLATFORM_WIN32
			HWND framebufferWindowHandle;
			HDC framebufferDeviceContext;
			HGLRC framebufferRenderingContext;
			frameBuffer->GetRenderingContext(framebufferWindowHandle, framebufferDeviceContext, framebufferRenderingContext);
			ActivateRenderingContext(framebufferWindowHandle, framebufferDeviceContext, framebufferRenderingContext);
#elif defined(OPENGL_USE_EGL)
			EGLDisplay framebufferDisplay;
			EGLSurface framebufferSurface;
			EGLContext framebufferRenderingContext;
			frameBuffer->GetRenderingContext(framebufferDisplay, framebufferSurface, framebufferRenderingContext);
			ActivateRenderingContext(framebufferDisplay, framebufferSurface, framebufferRenderingContext);
#elif defined(OPENGL_USE_PLATFORM_XLIB)
			::Display* framebufferDisplay;
			::Window framebufferWindow;
			GLXContext framebufferRenderingContext;
			frameBuffer->GetRenderingContext(framebufferDisplay, framebufferWindow, framebufferRenderingContext);
			ActivateRenderingContext(framebufferDisplay, framebufferWindow, framebufferRenderingContext);
#elif defined(OPENGL_USE_PLATFORM_APPKIT)
			CGLContextObj framebufferRenderingContext;
			frameBuffer->GetRenderingContext(framebufferRenderingContext);
			ActivateRenderingContext(framebufferRenderingContext);
#endif
			frameBuffer->CaptureFrameBufferOutput(true, false);
		}

		// Restore the generic base rendering context
#ifdef OPENGL_USE_PLATFORM_WIN32
		ActivateRenderingContext(_dummyMainWindowHandle, _deviceContext, _mainRenderingContext);
#elif defined(OPENGL_USE_EGL)
		ActivateRenderingContext(_mainDisplay, _dummySurface, _mainRenderingContext);
#elif defined(OPENGL_USE_PLATFORM_XLIB)
		ActivateRenderingContext(_xDisplay, _dummyWindow, _mainRenderingContext);
#elif defined(OPENGL_USE_PLATFORM_APPKIT)
		ActivateRenderingContext(_mainRenderingContext);
#endif

		// Begin the capture process for each non-window framebuffer output
		for (OpenGLFrameBuffer* frameBuffer : _boundTextureFramebuffers)
		{
			frameBuffer->CaptureFrameBufferOutput(true, false);
		}

		//##TODO## Add a fence we can synchronize with. This is important in the case we're only doing offscreen
		//rendering in particular.
	}

	// Flush the command queue to minimize render delay. Tests have shown this gives a small but measurable performance
	// boost.
	glFlush();

	// Do a final check for errors at the end of the render process
	CheckGLError(_log.get());
}

//----------------------------------------------------------------------------------------
void OpenGLRenderer::PerformSwapOperation()
{
	// Initialize our set of captured buffers
	bool usingPixelBufferObjectsForCapture = UsePixelBufferObjectsForFrameCapture();
	bool captureWindowFramebuffersFromFrontBuffer = CaptureWindowFramebuffersFromFrontBuffer();
	_capturedFramebufferOutputsInCurrentFrame.clear();
	_capturedDataArrayOutputsInCurrentFrame.clear();
	_capturedTexelArrayOutputsInCurrentFrame.clear();

	// If we're capturing window framebuffers from the back buffers, perform the capture now.
	if (!usingPixelBufferObjectsForCapture && !_boundWindowFramebuffers.empty() && !captureWindowFramebuffersFromFrontBuffer)
	{
		// Ensure rendering has completed first before we try and read the back buffers
		glFinish();

		// Capture each window framebuffer output
		for (OpenGLFrameBuffer* frameBuffer : _boundWindowFramebuffers)
		{
#ifdef OPENGL_USE_PLATFORM_WIN32
			HWND framebufferWindowHandle;
			HDC framebufferDeviceContext;
			HGLRC framebufferRenderingContext;
			frameBuffer->GetRenderingContext(framebufferWindowHandle, framebufferDeviceContext, framebufferRenderingContext);
			ActivateRenderingContext(framebufferWindowHandle, framebufferDeviceContext, framebufferRenderingContext);
#elif defined(OPENGL_USE_EGL)
			EGLDisplay framebufferDisplay;
			EGLSurface framebufferSurface;
			EGLContext framebufferRenderingContext;
			frameBuffer->GetRenderingContext(framebufferDisplay, framebufferSurface, framebufferRenderingContext);
			ActivateRenderingContext(framebufferDisplay, framebufferSurface, framebufferRenderingContext);
#elif defined(OPENGL_USE_PLATFORM_XLIB)
			::Display* framebufferDisplay;
			::Window framebufferWindow;
			GLXContext framebufferRenderingContext;
			frameBuffer->GetRenderingContext(framebufferDisplay, framebufferWindow, framebufferRenderingContext);
			ActivateRenderingContext(framebufferDisplay, framebufferWindow, framebufferRenderingContext);
#elif defined(OPENGL_USE_PLATFORM_APPKIT)
			CGLContextObj framebufferRenderingContext;
			frameBuffer->GetRenderingContext(framebufferRenderingContext);
			ActivateRenderingContext(framebufferRenderingContext);
#endif
			frameBuffer->CaptureFrameBufferOutput(false, false);
		}

		// Restore the generic base rendering context
#ifdef OPENGL_USE_PLATFORM_WIN32
		ActivateRenderingContext(_dummyMainWindowHandle, _deviceContext, _mainRenderingContext);
#elif defined(OPENGL_USE_EGL)
		ActivateRenderingContext(_mainDisplay, _dummySurface, _mainRenderingContext);
#elif defined(OPENGL_USE_PLATFORM_XLIB)
		ActivateRenderingContext(_xDisplay, _dummyWindow, _mainRenderingContext);
#elif defined(OPENGL_USE_PLATFORM_APPKIT)
		ActivateRenderingContext(_mainRenderingContext);
#endif
	}

	// Swap any framebuffers to the screen as required
	if (!_boundWindowFramebuffers.empty())
	{
		// Perform the present operation for each window framebuffer
		for (OpenGLFrameBuffer* frameBuffer : _boundWindowFramebuffers)
		{
#ifdef OPENGL_USE_PLATFORM_WIN32
			HWND framebufferWindowHandle;
			HDC framebufferDeviceContext;
			HGLRC framebufferRenderingContext;
			frameBuffer->GetRenderingContext(framebufferWindowHandle, framebufferDeviceContext, framebufferRenderingContext);
			ActivateRenderingContext(framebufferWindowHandle, framebufferDeviceContext, framebufferRenderingContext);
#elif defined(OPENGL_USE_EGL)
			EGLDisplay framebufferDisplay;
			EGLSurface framebufferSurface;
			EGLContext framebufferRenderingContext;
			frameBuffer->GetRenderingContext(framebufferDisplay, framebufferSurface, framebufferRenderingContext);
			ActivateRenderingContext(framebufferDisplay, framebufferSurface, framebufferRenderingContext);
#elif defined(OPENGL_USE_PLATFORM_XLIB)
			::Display* framebufferDisplay;
			::Window framebufferWindow;
			GLXContext framebufferRenderingContext;
			frameBuffer->GetRenderingContext(framebufferDisplay, framebufferWindow, framebufferRenderingContext);
			ActivateRenderingContext(framebufferDisplay, framebufferWindow, framebufferRenderingContext);
#elif defined(OPENGL_USE_PLATFORM_APPKIT)
			CGLContextObj framebufferRenderingContext;
			frameBuffer->GetRenderingContext(framebufferRenderingContext);
			ActivateRenderingContext(framebufferRenderingContext);
#endif
			frameBuffer->PresentToWindow();
		}

		// Restore the generic base rendering context
#ifdef OPENGL_USE_PLATFORM_WIN32
		ActivateRenderingContext(_dummyMainWindowHandle, _deviceContext, _mainRenderingContext);
#elif defined(OPENGL_USE_EGL)
		ActivateRenderingContext(_mainDisplay, _dummySurface, _mainRenderingContext);
#elif defined(OPENGL_USE_PLATFORM_XLIB)
		ActivateRenderingContext(_xDisplay, _dummyWindow, _mainRenderingContext);
#elif defined(OPENGL_USE_PLATFORM_APPKIT)
		ActivateRenderingContext(_mainRenderingContext);
#endif
	}

	// If we're capturing window framebuffers from the back buffers, perform the capture now.
	if (!usingPixelBufferObjectsForCapture && !_boundWindowFramebuffers.empty() && captureWindowFramebuffersFromFrontBuffer)
	{
		// Capture each window framebuffer output
		for (OpenGLFrameBuffer* frameBuffer : _boundWindowFramebuffers)
		{
#ifdef OPENGL_USE_PLATFORM_WIN32
			HWND framebufferWindowHandle;
			HDC framebufferDeviceContext;
			HGLRC framebufferRenderingContext;
			frameBuffer->GetRenderingContext(framebufferWindowHandle, framebufferDeviceContext, framebufferRenderingContext);
			ActivateRenderingContext(framebufferWindowHandle, framebufferDeviceContext, framebufferRenderingContext);
#elif defined(OPENGL_USE_EGL)
			EGLDisplay framebufferDisplay;
			EGLSurface framebufferSurface;
			EGLContext framebufferRenderingContext;
			frameBuffer->GetRenderingContext(framebufferDisplay, framebufferSurface, framebufferRenderingContext);
			ActivateRenderingContext(framebufferDisplay, framebufferSurface, framebufferRenderingContext);
#elif defined(OPENGL_USE_PLATFORM_XLIB)
			::Display* framebufferDisplay;
			::Window framebufferWindow;
			GLXContext framebufferRenderingContext;
			frameBuffer->GetRenderingContext(framebufferDisplay, framebufferWindow, framebufferRenderingContext);
			ActivateRenderingContext(framebufferDisplay, framebufferWindow, framebufferRenderingContext);
#elif defined(OPENGL_USE_PLATFORM_APPKIT)
			CGLContextObj framebufferRenderingContext;
			frameBuffer->GetRenderingContext(framebufferRenderingContext);
			ActivateRenderingContext(framebufferRenderingContext);
#endif
			frameBuffer->CaptureFrameBufferOutput(false, true);
		}

		// Restore the generic base rendering context
#ifdef OPENGL_USE_PLATFORM_WIN32
		ActivateRenderingContext(_dummyMainWindowHandle, _deviceContext, _mainRenderingContext);
#elif defined(OPENGL_USE_EGL)
		ActivateRenderingContext(_mainDisplay, _dummySurface, _mainRenderingContext);
#elif defined(OPENGL_USE_PLATFORM_XLIB)
		ActivateRenderingContext(_xDisplay, _dummyWindow, _mainRenderingContext);
#elif defined(OPENGL_USE_PLATFORM_APPKIT)
		ActivateRenderingContext(_mainRenderingContext);
#endif
	}

	// Capture the output of any non-window framebuffers as required
	if (!usingPixelBufferObjectsForCapture)
	{
		for (OpenGLFrameBuffer* frameBuffer : _boundTextureFramebuffers)
		{
			frameBuffer->CaptureFrameBufferOutput(false, true);
		}
	}

	// If we're using pixel buffer objects for framebuffer capture, synchronize with the previously initiated capture
	// process and complete the transfer to system memory now.
	if (usingPixelBufferObjectsForCapture)
	{
		for (OpenGLFrameBuffer* frameBuffer : _boundWindowFramebuffers)
		{
			frameBuffer->CompleteCaptureFrameBufferOutput();
		}
		for (OpenGLFrameBuffer* frameBuffer : _boundTextureFramebuffers)
		{
			frameBuffer->CompleteCaptureFrameBufferOutput();
		}
	}

	// Capture the output of any resource buffers as required
#ifdef GL_VERSION_4_3
	for (OpenGLDataArray* resourceBuffer : _boundDataArrays)
	{
		resourceBuffer->CaptureDataBufferOutput();
	}
	for (OpenGLTexelArray* resourceBuffer : _boundTexelArrays)
	{
		resourceBuffer->CaptureDataBufferOutput();
	}
#endif
}

//----------------------------------------------------------------------------------------
void OpenGLRenderer::PerformDeleteLastDrawResourcesOperation()
{
	PerformDeleteDrawResourcesOperation(false);
}

//----------------------------------------------------------------------------------------
void OpenGLRenderer::PerformDeleteNextDrawResourcesOperation()
{
	PerformDeleteDrawResourcesOperation(true);
}

//----------------------------------------------------------------------------------------
void OpenGLRenderer::PerformDeleteDrawResourcesOperation(bool nextDrawDelete)
{
	// If we're advancing the delete step of the next draw, strip the objects being deleted out of any pending update
	// operations which would normally be run first.
	std::unique_lock<std::mutex> buildStateLock(_buildStateMutex, std::defer_lock);
	unsigned int stateIndex = (nextDrawDelete ? _buildIndex : _drawIndex);
	if (nextDrawDelete)
	{
		buildStateLock.lock();
		std::unordered_set<void*> deleteSet;
		for (const auto& entry : _state[_buildIndex].deletePendingObjects)
		{
			deleteSet.insert(std::visit([](auto* object) { return static_cast<void*>(object); }, entry));
		}
		_state[_buildIndex].migrateStatePendingObjects.erase(std::remove_if(_state[_buildIndex].migrateStatePendingObjects.begin(), _state[_buildIndex].migrateStatePendingObjects.end(), [&](const auto& v) { return deleteSet.find(std::visit([](auto* p) { return static_cast<void*>(p); }, v)) != deleteSet.end(); }), _state[_buildIndex].migrateStatePendingObjects.end());
		_state[_buildIndex].bufferUpdatePendingObjects.erase(std::remove_if(_state[_buildIndex].bufferUpdatePendingObjects.begin(), _state[_buildIndex].bufferUpdatePendingObjects.end(), [&](const auto& v) { return deleteSet.find(std::visit([](auto* p) { return static_cast<void*>(p); }, v)) != deleteSet.end(); }), _state[_buildIndex].bufferUpdatePendingObjects.end());
#ifdef GL_VERSION_4_3
		_state[_buildIndex].bufferTransferPendingObjects.erase(std::remove_if(_state[_buildIndex].bufferTransferPendingObjects.begin(), _state[_buildIndex].bufferTransferPendingObjects.end(), [&](const auto& v) { return deleteSet.find(std::visit([](auto* p) { return static_cast<void*>(p); }, v)) != deleteSet.end(); }), _state[_buildIndex].bufferTransferPendingObjects.end());
#endif
		_capturedFramebufferOutputsInCurrentFrame.erase(std::remove_if(_capturedFramebufferOutputsInCurrentFrame.begin(), _capturedFramebufferOutputsInCurrentFrame.end(), [&](const auto& v) { return deleteSet.find(v) != deleteSet.end(); }), _capturedFramebufferOutputsInCurrentFrame.end());
#ifdef GL_VERSION_4_3
		_capturedDataArrayOutputsInCurrentFrame.erase(std::remove_if(_capturedDataArrayOutputsInCurrentFrame.begin(), _capturedDataArrayOutputsInCurrentFrame.end(), [&](const auto& v) { return deleteSet.find(v) != deleteSet.end(); }), _capturedDataArrayOutputsInCurrentFrame.end());
		_capturedTexelArrayOutputsInCurrentFrame.erase(std::remove_if(_capturedTexelArrayOutputsInCurrentFrame.begin(), _capturedTexelArrayOutputsInCurrentFrame.end(), [&](const auto& v) { return deleteSet.find(v) != deleteSet.end(); }), _capturedTexelArrayOutputsInCurrentFrame.end());
#endif

		// If we're on EGL, we may have wayland window bindings in use. In this case, we can have wl_egl_window wrapper
		// objects which tie to underlying window surfaces. In the case of a framebuffer not being deleted, but being
		// unbound from a given window during a state update, we need to advance the removal of the wrapper here to
		// ensure it's freed on a call to WaitForDeferredDeletionComplete, so that the window can safely be disposed of
		// afterwards without a frame being advanced. We also have a similar requirement under macOS with the
		// NSOpenGLContext objects.
#if (defined(OPENGL_USE_EGL) && defined(COBALT_RENDERER_WAYLAND_SUPPORT)) || defined(OPENGL_USE_PLATFORM_APPKIT)
		for (const auto& entry : _state[_buildIndex].migrateStatePendingObjects)
		{
			auto entryAsFrameBuffer = std::get_if<OpenGLFrameBuffer*>(&entry);
			if (entryAsFrameBuffer != nullptr)
			{
				if ((*entryAsFrameBuffer)->IsBoundToWindow())
				{
					(*entryAsFrameBuffer)->ReleaseInvalidatedStateForNextFrame();
				}
			}
		}
#endif
	}

	// Delete any framebuffer rendering contexts first, so that we don't need to release VAOs for renderable nodes which
	// belong to the deleted rendering contexts. If we don't do this first, the renderable nodes will consider the
	// contexts still active, and all allocated VAOs will be collected into a free buffer list, which is pointless busy
	// work if the context is also being freed here.
	for (const auto& entry : _state[stateIndex].deletePendingObjects)
	{
		auto entryAsFrameBuffer = std::get_if<OpenGLFrameBuffer*>(&entry);
		if (entryAsFrameBuffer != nullptr)
		{
			if ((*entryAsFrameBuffer)->IsBoundToWindow())
			{
				DeleteRenderingContext((*entryAsFrameBuffer)->GetWindowRenderingContextIndex());
			}
		}
	}

	// Update our container object release helpers so we can gather freed resource IDs
	{
#ifdef OPENGL_USE_PLATFORM_APPKIT
		std::unique_lock<std::mutex> lock(_renderingContextMutex);
#endif
		_containerObjectsReleaseHelpers.resize(_allocatedRenderingContexts.size());
		for (size_t i = 0; i < _allocatedRenderingContexts.size(); ++i)
		{
			auto& renderingContext = _allocatedRenderingContexts[i];
			auto& releaseHelper = _containerObjectsReleaseHelpers[i];
			releaseHelper.slotAllocated = renderingContext.slotAllocated;
			releaseHelper.generationIndex = renderingContext.generationIndex;
			releaseHelper.vertexArrayObjects = &renderingContext.freeVertexArrayObjectIDs;
		}
	}

	// Delete all pending objects to be deleted. We also collect all VAO IDs here which belong to rendering contexts
	// which are still active, merging them back into the free list.
	for (const auto& entry : _state[stateIndex].deletePendingObjects)
	{
		auto entryAsRenderableNode = std::get_if<OpenGLRenderableNode*>(&entry);
		if (entryAsRenderableNode != nullptr)
		{
			(*entryAsRenderableNode)->ExtractVertexArrayIDs(_containerObjectsReleaseHelpers);
		}
		std::visit([](auto* object) { delete object; }, entry);
	}
	_state[stateIndex].deletePendingObjects.clear();
}

//----------------------------------------------------------------------------------------
void OpenGLRenderer::BindTextures(const std::vector<ITextureBindingInfo*>& bindingEntries, OpenGLShaderProgram* program)
{
	for (ITextureBindingInfo* bindingInfo : bindingEntries)
	{
		bindingInfo->BindTexture();
	}
}

//----------------------------------------------------------------------------------------
void OpenGLRenderer::BindStateBuffers(const std::vector<StateBufferBindingInfo*>& bindingEntries, OpenGLShaderProgram* program)
{
	for (StateBufferBindingInfo* bindingInfo : bindingEntries)
	{
		bindingInfo->BindStateBuffer(this, program);
	}
}

//----------------------------------------------------------------------------------------
void OpenGLRenderer::BindResourceArrays(const std::vector<ResourceArrayBindingInfo*>& bindingEntries, OpenGLShaderProgram* program, bool performReset)
{
	for (ResourceArrayBindingInfo* bindingInfo : bindingEntries)
	{
		bindingInfo->BindResourceArray(this, program, performReset);
	}
}

//----------------------------------------------------------------------------------------
void OpenGLRenderer::UnbindTextures(const std::vector<ITextureBindingInfo*>& bindingEntries, OpenGLShaderProgram* program)
{
	for (ITextureBindingInfo* bindingInfo : bindingEntries)
	{
		bindingInfo->UnbindTexture();
	}
}

//----------------------------------------------------------------------------------------
void OpenGLRenderer::UnbindStateBuffers(const std::vector<StateBufferBindingInfo*>& bindingEntries, OpenGLShaderProgram* program)
{
	for (StateBufferBindingInfo* bindingInfo : bindingEntries)
	{
		bindingInfo->UnbindStateBuffer();
	}
}

//----------------------------------------------------------------------------------------
void OpenGLRenderer::UnbindResourceArrays(const std::vector<ResourceArrayBindingInfo*>& bindingEntries, OpenGLShaderProgram* program)
{
	for (ResourceArrayBindingInfo* bindingInfo : bindingEntries)
	{
		bindingInfo->UnbindResourceArray();
	}
}

//----------------------------------------------------------------------------------------
// State buffer methods
//----------------------------------------------------------------------------------------
int OpenGLRenderer::GetMaxUniformBlockSize() const
{
	return _maxUniformBlockSize;
}

//----------------------------------------------------------------------------------------
int OpenGLRenderer::GetMaxShaderUniformBlocks(IShaderProgram::ShaderStage stage) const
{
	switch (stage)
	{
	case IShaderProgram::ShaderStage::Vertex:
		return _maxVertexShaderUniformBlocks;
	case IShaderProgram::ShaderStage::Fragment:
		return _maxFragmentShaderUniformBlocks;
	case IShaderProgram::ShaderStage::Geometry:
		return _maxGeometryShaderUniformBlocks;
#ifdef GL_VERSION_4_3
	case IShaderProgram::ShaderStage::Compute:
		return _maxComputeShaderUniformBlocks;
#endif
	}
	return 0;
}

//----------------------------------------------------------------------------------------
int OpenGLRenderer::GetUniformBufferOffsetAlignment() const
{
	return _uniformBufferOffsetAlignment;
}

//----------------------------------------------------------------------------------------
// Texture and sampler methods
//----------------------------------------------------------------------------------------
float OpenGLRenderer::GetMaxTextureSamplerAnisotropy() const
{
	return _maxTextureSamplerAnisotropy;
}

//----------------------------------------------------------------------------------------
// Debug log methods
//----------------------------------------------------------------------------------------
void GLAPIENTRY OpenGLRenderer::MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
	const auto* objectPointer = reinterpret_cast<const OpenGLRenderer*>(userParam);
	objectPointer->MessageCallback(source, type, id, severity, length, message);
}

//----------------------------------------------------------------------------------------
void OpenGLRenderer::MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message) const
{
	// Ignore all application messages that come from render markers
	if ((source == GL_DEBUG_SOURCE_APPLICATION) && (id == RenderMarkerId))
	{
		return;
	}

	// Map the source to a readable value
	std::string sourceAsString;
	switch (source)
	{
	case GL_DEBUG_SOURCE_API:
		sourceAsString = "API";
		break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
		sourceAsString = "WindowSystem";
		break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER:
		sourceAsString = "ShaderCompiler";
		break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:
		sourceAsString = "ThirdParty";
		break;
	case GL_DEBUG_SOURCE_APPLICATION:
		sourceAsString = "Application";
		break;
	case GL_DEBUG_SOURCE_OTHER:
	default:
		sourceAsString = "Other";
		break;
	}

	// Map the type to a readable value
	std::string typeAsString;
	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR:
		typeAsString = "Error";
		break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		typeAsString = "DeprecatedBehaviour";
		break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		typeAsString = "UndefinedBehaviour";
		break;
	case GL_DEBUG_TYPE_PORTABILITY:
		typeAsString = "Portability";
		break;
	case GL_DEBUG_TYPE_PERFORMANCE:
		typeAsString = "Performance";
		break;
	case GL_DEBUG_TYPE_OTHER:
	default:
		typeAsString = "Other";
		break;
	case GL_DEBUG_TYPE_MARKER:
		typeAsString = "Marker";
		break;
	}

	// Map the severity value to our logging severity values
	cobalt::logging::ILogger::Severity loggerSeverity;
	switch (severity)
	{
	case GL_DEBUG_SEVERITY_HIGH:
		loggerSeverity = cobalt::logging::ILogger::Severity::Error;
		break;
	case GL_DEBUG_SEVERITY_MEDIUM:
	default:
		loggerSeverity = cobalt::logging::ILogger::Severity::Warning;
		break;
	case GL_DEBUG_SEVERITY_LOW:
	case GL_DEBUG_SEVERITY_NOTIFICATION:
		loggerSeverity = cobalt::logging::ILogger::Severity::Info;
		break;
	}

	// Log this debug message
	_log->Log(loggerSeverity, "OpenGLDebug message ID {0} from {1} of type {2}: {3}", id, sourceAsString, typeAsString, message);
}

//----------------------------------------------------------------------------------------
// Rendering context methods
//----------------------------------------------------------------------------------------
#ifdef OPENGL_USE_PLATFORM_WIN32
//----------------------------------------------------------------------------------------
bool OpenGLRenderer::SelectWindowPixelFormat(HWND hwnd, IFrameBuffer::WindowColorSpaceMode windowColorSpaceMode, IFrameBuffer::WindowDepthStencilMode windowDepthStencilMode, HDC& deviceContext, PixelFormatInfo& pixelFormatInfo) const
{
	return SelectWindowPixelFormat(hwnd, windowColorSpaceMode, windowDepthStencilMode, deviceContext, pixelFormatInfo, *_log, _enableDebugLogging);
}
#endif

#ifdef OPENGL_USE_EGL
//----------------------------------------------------------------------------------------
bool OpenGLRenderer::SelectWindowPixelFormat(EGLDisplay display, IFrameBuffer::WindowColorSpaceMode windowColorSpaceMode, IFrameBuffer::WindowDepthStencilMode windowDepthStencilMode, PixelFormatInfo& pixelFormatInfo, bool offscreenDevice) const
{
	return SelectWindowPixelFormat(display, windowColorSpaceMode, windowDepthStencilMode, pixelFormatInfo, offscreenDevice, *_log, _enableDebugLogging);
}
#endif

#if defined(OPENGL_USE_PLATFORM_XLIB) && !defined(OPENGL_USE_EGL)
//----------------------------------------------------------------------------------------
bool OpenGLRenderer::SelectWindowPixelFormat(::Display* display, IFrameBuffer::WindowColorSpaceMode windowColorSpaceMode, IFrameBuffer::WindowDepthStencilMode windowDepthStencilMode, PixelFormatInfo& pixelFormatInfo) const
{
	return SelectWindowPixelFormat(display, windowColorSpaceMode, windowDepthStencilMode, pixelFormatInfo, *_log, _enableDebugLogging);
}
#endif

#ifdef OPENGL_USE_PLATFORM_APPKIT
//----------------------------------------------------------------------------------------
bool OpenGLRenderer::SelectPixelFormat(IFrameBuffer::WindowColorSpaceMode windowColorSpaceMode, IFrameBuffer::WindowDepthStencilMode windowDepthStencilMode, PixelFormatInfo& pixelFormatInfo) const
{
	return SelectPixelFormat(windowColorSpaceMode, windowDepthStencilMode, pixelFormatInfo, *_log, _enableDebugLogging);
}
#endif

#ifdef OPENGL_USE_PLATFORM_WIN32
//----------------------------------------------------------------------------------------
bool OpenGLRenderer::SelectWindowPixelFormat(HWND hwnd, IFrameBuffer::WindowColorSpaceMode windowColorSpaceMode, IFrameBuffer::WindowDepthStencilMode windowDepthStencilMode, HDC& deviceContext, PixelFormatInfo& pixelFormatInfo, const cobalt::logging::ILogger& log, bool debugLoggingEnabled)
{
	// Obtain a device context for the target window
	deviceContext = GetDC(hwnd);
	if (deviceContext == nullptr)
	{
		log.Error("Failed to get device context for target window");
		return false;
	}

	// While using multiple rendering contexts, sharing resources between them, and switching between them during
	// rendering is all supported and well documented in the OpenGL spec, experience has shown that driver
	// implementations are currently very poor (Jan 2021), and numerous distinct problems have been encountered across
	// vendors when trying to use differing render contexts for non-trivial render pipelines. The biggest issues seem to
	// relate to the use of wglCreateContextAttribsARB with the hshareContext attribute. Driver implementations seem to
	// fail to correctly share resources in multiple scenarios when this feature is used, which is vital in order to
	// allow us to render our scene correctly when multiple window surfaces are used which differ in their depth/stencil
	// buffer modes. The only viable alternative to using wglCreateContextAttribsARB with hshareContext is to fall back
	// to using a single rendering context, which is preferable from our point of view, but rendering contexts have the
	// restriction that they can only be used to render to different windows than they were created with, if the pixel
	// format bound to the target window matches the one that was used to create the rendering context originally, as
	// per the documentation for wglMakeCurrent: "It need not be the same hdc that was passed to wglCreateContext when
	// hglrc was created, but it must be on the same device and have the same pixel format". In order to meet this
	// requirement, we explicitly force the color and depth/stencil modes here to a single preferred format where we are
	// forcing all rendering to occur under a single rendering context. This means we will waste some video memory where
	// buffers are not required or end up larger than are needed, but for the sake of trouble-free rendering while
	// retaining the flexibility to bind to multiple windows with differing buffer formats, this seems like the most
	// reasonable option at this time. Our OpenGL renderer is considered a legacy inclusion anyway, and although not
	// officially deprecated by the Khronos Group currently, OpenGL is clearly not being invested in, and won't evolve
	// or improve in meaningful ways going forward. Considering this, it seems acceptable to use an imperfect workaround
	// here to make the OpenGL renderer usable for the next few years, until it becomes obsolete.
	static const bool forceSingleRenderingContext = true;
	if (forceSingleRenderingContext)
	{
		if (debugLoggingEnabled)
		{
			log.Info("Forcing shared render context for window with color space {0} and depth/stencil mode {1}.", windowColorSpaceMode, windowDepthStencilMode);
		}
		windowColorSpaceMode = IFrameBuffer::WindowColorSpaceMode::Default;
		windowDepthStencilMode = IFrameBuffer::WindowDepthStencilMode::DepthUNorm24StencilUInt8;
	}

	// Select the colour format to use for the window framebuffer
	BYTE colorBits = 24;
	BYTE alphaBits = 8;

	// Select the depth/stencil format to use for the window framebuffer
	BYTE depthBits = 0;
	BYTE stencilBits = 0;
	switch (windowDepthStencilMode)
	{
	case IFrameBuffer::WindowDepthStencilMode::DepthUNorm16:
		depthBits = 16;
		break;
	case IFrameBuffer::WindowDepthStencilMode::DepthUNorm24:
		depthBits = 24;
		break;
	case IFrameBuffer::WindowDepthStencilMode::DepthUNorm24StencilUInt8:
		depthBits = 24;
		stencilBits = 8;
		break;
	case IFrameBuffer::WindowDepthStencilMode::DepthFloat32:
		depthBits = 32;
		break;
	case IFrameBuffer::WindowDepthStencilMode::DepthFloat32StencilUInt8:
		depthBits = 32;
		stencilBits = 8;
		break;
	}

	// Attempt to locate a pixel format that matches the desired settings. If the requested depth/stencil combination is
	// unavailable, retry with lower precision depth buffers while keeping the requested stencil buffer size fixed.
	auto TryChoosePixelFormat = [&](BYTE requestedDepthBits, PIXELFORMATDESCRIPTOR& outPfd) -> int {
		outPfd = {};
		outPfd.nSize = sizeof(outPfd);
		outPfd.nVersion = 1;
		outPfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_SUPPORT_COMPOSITION;
		outPfd.iPixelType = PFD_TYPE_RGBA;
		outPfd.cColorBits = colorBits;
		outPfd.cAlphaBits = alphaBits;
		outPfd.cAccumBits = 0;
		outPfd.cDepthBits = requestedDepthBits;
		outPfd.cStencilBits = stencilBits;
		outPfd.cAuxBuffers = 0;
		outPfd.iLayerType = PFD_MAIN_PLANE;
		return ChoosePixelFormat(deviceContext, &outPfd);
	};
	PIXELFORMATDESCRIPTOR pfd = {};
	int pixelFormat = TryChoosePixelFormat(depthBits, pfd);
	if (pixelFormat == 0)
	{
		// Define our possible depth buffer size fallback options
		std::vector<BYTE> depthFallbacks;
		if (depthBits >= 32)
		{
			depthFallbacks.push_back(24);
			depthFallbacks.push_back(16);
		}
		else if (depthBits >= 24)
		{
			depthFallbacks.push_back(16);
		}

		// Iteratively attempt to select a pixel format with a lower depth buffer size
		for (BYTE fallbackDepthBits : depthFallbacks)
		{
			pixelFormat = TryChoosePixelFormat(fallbackDepthBits, pfd);
			if (pixelFormat != 0)
			{
				break;
			}
		}
		if (pixelFormat == 0)
		{
			ReleaseDC(hwnd, deviceContext);
			log.Error("ChoosePixelFormat returned 0. GetLastError: {0}", GetLastError());
			return false;
		}
	}

	// Retrieve information on the actual pixel format that was selected. Note that the driver may select one that
	// differs to our request. It may promote our depth buffer size for example above the precision that was requested,
	// if the driver knows that there's a performance advantage on the hardware for using a particular preferred format.
	PIXELFORMATDESCRIPTOR selectedPfd;
	if (DescribePixelFormat(deviceContext, pixelFormat, sizeof(selectedPfd), &selectedPfd) == 0)
	{
		ReleaseDC(hwnd, deviceContext);
		log.Error("DescribePixelFormat returned 0. GetLastError: {0}", GetLastError());
		return false;
	}

	// Verify that the selected pixel format still satisfies the requested stencil buffer size. Depth precision may be
	// intentionally reduced by the fallback path above, but stencil precision must remain fixed.
	if (selectedPfd.cStencilBits < stencilBits)
	{
		ReleaseDC(hwnd, deviceContext);
		log.Error("Selected PFD has insufficient stencil bits. Requested {0}, but got {1}.", stencilBits, selectedPfd.cStencilBits);
		return false;
	}
	if ((depthBits > 0) && (selectedPfd.cDepthBits == 0))
	{
		ReleaseDC(hwnd, deviceContext);
		log.Error("Selected PFD has no depth bits. Requested {0}.", depthBits);
		return false;
	}

	// If this is a debug build, print information on the selected pixel format.
	if (debugLoggingEnabled)
	{
		if ((colorBits != selectedPfd.cColorBits) || (alphaBits != selectedPfd.cAlphaBits) || (depthBits != selectedPfd.cDepthBits) || (stencilBits != selectedPfd.cStencilBits))
		{
			log.Info("Requested PFD with (C,A,D,S) of ({0},{1},{2},{3}), but got one with ({4},{5},{6},{7}).", colorBits, alphaBits, depthBits, stencilBits, selectedPfd.cColorBits, selectedPfd.cAlphaBits, selectedPfd.cDepthBits, selectedPfd.cStencilBits);
		}
		else
		{
			log.Info("Requested PFD with (C,A,D,S) of ({0},{1},{2},{3}), and got an exact match.", colorBits, alphaBits, depthBits, stencilBits);
		}
	}

	// Attempt to associate the requested pixel format with the target window. Note that importantly, as per the
	// documentation from Microsoft "An application can only set the pixel format of a window one time. Once a window's
	// pixel format is set, it cannot be changed". This prevents window surface reuse by the OpenGL renderer with
	// different pixel formats.
	if (SetPixelFormat(deviceContext, pixelFormat, &pfd) != TRUE)
	{
		ReleaseDC(hwnd, deviceContext);
		log.Error("SetPixelFormat returned FALSE. GetLastError: {0}", GetLastError());
		return false;
	}

	// Record information on the selected pixel format, and return true.
	pixelFormatInfo = {};
	pixelFormatInfo.pixelFormatDescriptor = selectedPfd;
	pixelFormatInfo.pixelFormatIndex = pixelFormat;
	return true;
}
#endif

#ifdef OPENGL_USE_EGL
//----------------------------------------------------------------------------------------
bool OpenGLRenderer::SelectWindowPixelFormat(EGLDisplay display, IFrameBuffer::WindowColorSpaceMode windowColorSpaceMode, IFrameBuffer::WindowDepthStencilMode windowDepthStencilMode, PixelFormatInfo& pixelFormatInfo, bool offscreenDevice, const cobalt::logging::ILogger& log, bool debugLoggingEnabled)
{
	//##FIX## Workaround for WSL crash
	//https://github.com/microsoft/WSL/issues/12171
	WARNINGS_PUSH_OFF
	static auto wslCrashFix = dlopen("/usr/lib/wsl/lib/libd3d12core.so", RTLD_LAZY | RTLD_GLOBAL); //NOLINT
	WARNINGS_POP

	// Select the colour format to use for the window framebuffer
	int redBits = 8;
	int greenBits = 8;
	int blueBits = 8;
	int alphaBits = 8;

	// Select the depth/stencil format to use for the window framebuffer
	int depthBits = 0;
	int stencilBits = 0;
	switch (windowDepthStencilMode)
	{
	case IFrameBuffer::WindowDepthStencilMode::DepthUNorm16:
		depthBits = 16;
		break;
	case IFrameBuffer::WindowDepthStencilMode::DepthUNorm24:
		depthBits = 24;
		break;
	case IFrameBuffer::WindowDepthStencilMode::DepthUNorm24StencilUInt8:
		depthBits = 24;
		stencilBits = 8;
		break;
	case IFrameBuffer::WindowDepthStencilMode::DepthFloat32:
		depthBits = 32;
		break;
	case IFrameBuffer::WindowDepthStencilMode::DepthFloat32StencilUInt8:
		depthBits = 32;
		stencilBits = 8;
		break;
	}

	// Select a config which matches the requested format. If the requested depth/stencil combination is unavailable,
	// retry with lower precision depth buffers while keeping the requested stencil buffer size fixed.
	auto TryChooseConfig = [&](int requestedDepthBits, EGLConfig& outFbConfig, int& outFbCount) -> EGLBoolean {
		std::vector<EGLint> fbAttribs;
		if (!offscreenDevice)
		{
			fbAttribs.push_back(EGL_SURFACE_TYPE);
			fbAttribs.push_back(EGL_WINDOW_BIT);
		}
		else
		{
			fbAttribs.push_back(EGL_SURFACE_TYPE);
			fbAttribs.push_back(EGL_DONT_CARE);
		}
		fbAttribs.push_back(EGL_RENDERABLE_TYPE);
		fbAttribs.push_back(EGL_OPENGL_BIT);
		fbAttribs.push_back(EGL_RED_SIZE);
		fbAttribs.push_back(redBits);
		fbAttribs.push_back(EGL_GREEN_SIZE);
		fbAttribs.push_back(greenBits);
		fbAttribs.push_back(EGL_BLUE_SIZE);
		fbAttribs.push_back(blueBits);
		fbAttribs.push_back(EGL_ALPHA_SIZE);
		fbAttribs.push_back(alphaBits);
		fbAttribs.push_back(EGL_DEPTH_SIZE);
		fbAttribs.push_back(requestedDepthBits);
		fbAttribs.push_back(EGL_STENCIL_SIZE);
		fbAttribs.push_back(stencilBits);
		fbAttribs.push_back(EGL_NONE);

		outFbCount = 0;
		return eglChooseConfig(display, fbAttribs.data(), &outFbConfig, 1, &outFbCount);
	};
	int fbCount = 0;
	EGLConfig fbConfig = {};
	EGLBoolean eglChooseConfigReturn = TryChooseConfig(depthBits, fbConfig, fbCount);
	CheckEGLError(&log);
	if ((eglChooseConfigReturn != EGL_TRUE) || (fbCount == 0))
	{
		// Define our possible depth buffer size fallback options
		std::vector<int> depthFallbacks;
		if (depthBits >= 32)
		{
			depthFallbacks.push_back(24);
			depthFallbacks.push_back(16);
		}
		else if (depthBits >= 24)
		{
			depthFallbacks.push_back(16);
		}

		// Iteratively attempt to select a pixel format with a lower depth buffer size
		for (int fallbackDepthBits : depthFallbacks)
		{
			eglChooseConfigReturn = TryChooseConfig(fallbackDepthBits, fbConfig, fbCount);
			CheckEGLError(&log);
			if ((eglChooseConfigReturn == EGL_TRUE) && (fbCount > 0))
			{
				break;
			}
		}
		if ((eglChooseConfigReturn != EGL_TRUE) || (fbCount == 0))
		{
			log.Error("eglChooseConfig failed in SelectWindowPixelFormat.");
			return false;
		}
	}

	// Retrieve information on the actual pixel format that was selected. Note that the driver may select one that
	// differs to our request. It may promote our depth buffer size for example above the precision that was requested,
	// if the driver knows that there's a performance advantage on the hardware for using a particular preferred format.
	int actualRedBits = 0;
	int actualGreenBits = 0;
	int actualBlueBits = 0;
	int actualAlphaBits = 0;
	int actualDepthBits = 0;
	int actualStencilBits = 0;
	bool describePixelFormatSucceeded = true;
	describePixelFormatSucceeded &= (eglGetConfigAttrib(display, fbConfig, EGL_RED_SIZE, &actualRedBits) == EGL_TRUE);
	describePixelFormatSucceeded &= (eglGetConfigAttrib(display, fbConfig, EGL_GREEN_SIZE, &actualGreenBits) == EGL_TRUE);
	describePixelFormatSucceeded &= (eglGetConfigAttrib(display, fbConfig, EGL_BLUE_SIZE, &actualBlueBits) == EGL_TRUE);
	describePixelFormatSucceeded &= (eglGetConfigAttrib(display, fbConfig, EGL_ALPHA_SIZE, &actualAlphaBits) == EGL_TRUE);
	describePixelFormatSucceeded &= (eglGetConfigAttrib(display, fbConfig, EGL_DEPTH_SIZE, &actualDepthBits) == EGL_TRUE);
	describePixelFormatSucceeded &= (eglGetConfigAttrib(display, fbConfig, EGL_STENCIL_SIZE, &actualStencilBits) == EGL_TRUE);
	CheckEGLError(&log);
	if (!describePixelFormatSucceeded)
	{
		log.Error("eglGetConfigAttrib failed");
		return false;
	}

	// Verify that the selected pixel format still satisfies the requested stencil buffer size. Depth precision may be
	// intentionally reduced by the fallback path above, but stencil precision must remain fixed.
	if (actualStencilBits < stencilBits)
	{
		log.Error("Selected PFD has insufficient stencil bits. Requested {0}, but got {1}.", stencilBits, actualStencilBits);
		return false;
	}
	if ((depthBits > 0) && (actualDepthBits == 0))
	{
		log.Error("Selected PFD has no depth bits. Requested {0}.", depthBits);
		return false;
	}

	// If this is a debug build, print information on the selected pixel format.
	if (debugLoggingEnabled)
	{
		if ((redBits != actualRedBits) || (greenBits != actualGreenBits) || (blueBits != actualBlueBits) || (alphaBits != actualAlphaBits) || (depthBits != actualDepthBits) || (stencilBits != actualStencilBits))
		{
			log.Info("Requested PFD with (C,A,D,S) of ({0},{1},{2},{3}), but got one with ({4},{5},{6},{7}).", (redBits + greenBits + blueBits), alphaBits, depthBits, stencilBits, (actualRedBits + actualGreenBits + actualBlueBits), actualAlphaBits, actualDepthBits, actualStencilBits);
		}
		else
		{
			log.Info("Requested PFD with (C,A,D,S) of ({0},{1},{2},{3}), and got an exact match.", (redBits + greenBits + blueBits), alphaBits, depthBits, stencilBits);
		}
	}

	// Fill out the pixel format info
	pixelFormatInfo.fbConfig = fbConfig;
	pixelFormatInfo.redBits = actualRedBits;
	pixelFormatInfo.greenBits = actualGreenBits;
	pixelFormatInfo.blueBits = actualBlueBits;
	pixelFormatInfo.alphaBits = actualAlphaBits;
	pixelFormatInfo.depthBits = actualDepthBits;
	pixelFormatInfo.stencilBits = actualStencilBits;
	return true;
}
#endif

#if defined(OPENGL_USE_PLATFORM_XLIB) && !defined(OPENGL_USE_EGL)
//----------------------------------------------------------------------------------------
bool OpenGLRenderer::SelectWindowPixelFormat(::Display* display, IFrameBuffer::WindowColorSpaceMode windowColorSpaceMode, IFrameBuffer::WindowDepthStencilMode windowDepthStencilMode, PixelFormatInfo& pixelFormatInfo, const cobalt::logging::ILogger& log, bool debugLoggingEnabled)
{
	//##FIX## Workaround for WSL crash
	//https://github.com/microsoft/WSL/issues/12171
	WARNINGS_PUSH_OFF
	static auto wslCrashFix = dlopen("/usr/lib/wsl/lib/libd3d12core.so", RTLD_LAZY | RTLD_GLOBAL); //NOLINT
	WARNINGS_POP

	// Select the colour format to use for the window framebuffer
	int redBits = 8;
	int greenBits = 8;
	int blueBits = 8;
	int alphaBits = 8;

	// Select the depth/stencil format to use for the window framebuffer
	int depthBits = 0;
	int stencilBits = 0;
	switch (windowDepthStencilMode)
	{
	case IFrameBuffer::WindowDepthStencilMode::DepthUNorm16:
		depthBits = 16;
		break;
	case IFrameBuffer::WindowDepthStencilMode::DepthUNorm24:
		depthBits = 24;
		break;
	case IFrameBuffer::WindowDepthStencilMode::DepthUNorm24StencilUInt8:
		depthBits = 24;
		stencilBits = 8;
		break;
	case IFrameBuffer::WindowDepthStencilMode::DepthFloat32:
		depthBits = 32;
		break;
	case IFrameBuffer::WindowDepthStencilMode::DepthFloat32StencilUInt8:
		depthBits = 32;
		stencilBits = 8;
		break;
	}

	// Select a config which matches the requested format. If the requested depth/stencil combination is unavailable,
	// retry with lower precision depth buffers while keeping the requested stencil buffer size fixed.
	int fbCount = 0;
	const int screen = DefaultScreen(display);
	auto TryChooseFBConfig = [&](int requestedDepthBits, GLXFBConfig& outChosenConfig, GLXFBConfig*& outFbConfigs, int& outFbCount) -> bool {
		int fbAttribs[] = {GLX_X_RENDERABLE, X11Constants::kTrue, GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT, GLX_RENDER_TYPE, GLX_RGBA_BIT, GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR, GLX_RED_SIZE, redBits, GLX_GREEN_SIZE, greenBits, GLX_BLUE_SIZE, blueBits, GLX_ALPHA_SIZE, alphaBits, GLX_DEPTH_SIZE, requestedDepthBits, GLX_STENCIL_SIZE, stencilBits, GLX_DOUBLEBUFFER, X11Constants::kTrue, X11Constants::kNone};
		outChosenConfig = {};
		outFbCount = 0;
		outFbConfigs = glXChooseFBConfig(display, screen, &fbAttribs[0], &outFbCount);
		if ((outFbConfigs == nullptr) || (outFbCount == 0))
		{
			XFree(outFbConfigs);
			outFbConfigs = nullptr;
			return false;
		}
		outChosenConfig = outFbConfigs[0];
		return true;
	};
	GLXFBConfig chosenConfig = {};
	GLXFBConfig* fbConfigs = nullptr;
	bool chooseFBConfigSucceeded = TryChooseFBConfig(depthBits, chosenConfig, fbConfigs, fbCount);
	if ((fbConfigs == nullptr) || (fbCount == 0))
	{
		// Define our possible depth buffer size fallback options
		std::vector<int> depthFallbacks;
		if (depthBits >= 32)
		{
			depthFallbacks.push_back(24);
			depthFallbacks.push_back(16);
		}
		else if (depthBits >= 24)
		{
			depthFallbacks.push_back(16);
		}

		// Iteratively attempt to select a pixel format with a lower depth buffer size
		for (int fallbackDepthBits : depthFallbacks)
		{
			chooseFBConfigSucceeded = TryChooseFBConfig(fallbackDepthBits, chosenConfig, fbConfigs, fbCount);
			if (chooseFBConfigSucceeded)
			{
				break;
			}
		}
		if (!chooseFBConfigSucceeded)
		{
			log.Error("glXChooseFBConfig failed in SelectWindowPixelFormat.");
			return false;
		}
	}

	// Retrieve the visual object matching the target config
	auto visualInfo = glXGetVisualFromFBConfig(display, chosenConfig);
	if (visualInfo == nullptr)
	{
		log.Error("glXGetVisualFromFBConfig failed in SelectWindowPixelFormat.");
		XFree(fbConfigs);
		return false;
	}

	// Retrieve information on the actual pixel format that was selected. Note that the driver may select one that
	// differs to our request. It may promote our depth buffer size for example above the precision that was requested,
	// if the driver knows that there's a performance advantage on the hardware for using a particular preferred format.
	int actualRedBits = 0;
	int actualGreenBits = 0;
	int actualBlueBits = 0;
	int actualAlphaBits = 0;
	int actualDepthBits = 0;
	int actualStencilBits = 0;
	bool describePixelFormatSucceeded = true;
	describePixelFormatSucceeded &= (glXGetFBConfigAttrib(display, chosenConfig, GLX_RED_SIZE, &actualRedBits) == X11Constants::kSuccess);
	describePixelFormatSucceeded &= (glXGetFBConfigAttrib(display, chosenConfig, GLX_GREEN_SIZE, &actualGreenBits) == X11Constants::kSuccess);
	describePixelFormatSucceeded &= (glXGetFBConfigAttrib(display, chosenConfig, GLX_BLUE_SIZE, &actualBlueBits) == X11Constants::kSuccess);
	describePixelFormatSucceeded &= (glXGetFBConfigAttrib(display, chosenConfig, GLX_ALPHA_SIZE, &actualAlphaBits) == X11Constants::kSuccess);
	describePixelFormatSucceeded &= (glXGetFBConfigAttrib(display, chosenConfig, GLX_DEPTH_SIZE, &actualDepthBits) == X11Constants::kSuccess);
	describePixelFormatSucceeded &= (glXGetFBConfigAttrib(display, chosenConfig, GLX_STENCIL_SIZE, &actualStencilBits) == X11Constants::kSuccess);
	if (!describePixelFormatSucceeded)
	{
		log.Error("glXGetFBConfigAttrib failed");
		XFree(fbConfigs);
		return false;
	}

	// Verify that the selected pixel format still satisfies the requested stencil buffer size. Depth precision may be
	// intentionally reduced by the fallback path above, but stencil precision must remain fixed.
	if (actualStencilBits < stencilBits)
	{
		log.Error("Selected PFD has insufficient stencil bits. Requested {0}, but got {1}.", stencilBits, actualStencilBits);
		XFree(fbConfigs);
		return false;
	}
	if ((depthBits > 0) && (actualDepthBits == 0))
	{
		log.Error("Selected PFD has no depth bits. Requested {0}.", depthBits);
		XFree(fbConfigs);
		return false;
	}

	// Free the configs now that we're done with them
	XFree(fbConfigs);

	// If this is a debug build, print information on the selected pixel format.
	if (debugLoggingEnabled)
	{
		if ((redBits != actualRedBits) || (greenBits != actualGreenBits) || (blueBits != actualBlueBits) || (alphaBits != actualAlphaBits) || (depthBits != actualDepthBits) || (stencilBits != actualStencilBits))
		{
			log.Info("Requested PFD with (C,A,D,S) of ({0},{1},{2},{3}), but got one with ({4},{5},{6},{7}).", (redBits + greenBits + blueBits), alphaBits, depthBits, stencilBits, (actualRedBits + actualGreenBits + actualBlueBits), actualAlphaBits, actualDepthBits, actualStencilBits);
		}
		else
		{
			log.Info("Requested PFD with (C,A,D,S) of ({0},{1},{2},{3}), and got an exact match.", (redBits + greenBits + blueBits), alphaBits, depthBits, stencilBits);
		}
	}

	// Fill out the pixel format info
	pixelFormatInfo.fbConfig = chosenConfig;
	pixelFormatInfo.visualInfo = *visualInfo;
	pixelFormatInfo.redBits = actualRedBits;
	pixelFormatInfo.greenBits = actualGreenBits;
	pixelFormatInfo.blueBits = actualBlueBits;
	pixelFormatInfo.alphaBits = actualAlphaBits;
	pixelFormatInfo.depthBits = actualDepthBits;
	pixelFormatInfo.stencilBits = actualStencilBits;
	return true;
}
#endif

#ifdef OPENGL_USE_PLATFORM_APPKIT
//----------------------------------------------------------------------------------------
bool OpenGLRenderer::SelectPixelFormat(IFrameBuffer::WindowColorSpaceMode windowColorSpaceMode, IFrameBuffer::WindowDepthStencilMode windowDepthStencilMode, PixelFormatInfo& pixelFormatInfo, const cobalt::logging::ILogger& log, bool debugLoggingEnabled)
{
	// Select the colour format to use for the window framebuffer
	int colorBits = 24;
	int alphaBits = 8;

	// Select the depth/stencil format to use for the window framebuffer
	int depthBits = 0;
	int stencilBits = 0;
	switch (windowDepthStencilMode)
	{
	case IFrameBuffer::WindowDepthStencilMode::DepthUNorm16:
		depthBits = 16;
		break;
	case IFrameBuffer::WindowDepthStencilMode::DepthUNorm24:
		depthBits = 24;
		break;
	case IFrameBuffer::WindowDepthStencilMode::DepthUNorm24StencilUInt8:
		depthBits = 24;
		stencilBits = 8;
		break;
	case IFrameBuffer::WindowDepthStencilMode::DepthFloat32:
		depthBits = 32;
		break;
	case IFrameBuffer::WindowDepthStencilMode::DepthFloat32StencilUInt8:
		depthBits = 32;
		stencilBits = 8;
		break;
	}

	// Attempt to locate a pixel format that matches the desired settings. If the requested depth/stencil combination is
	// unavailable, retry with lower precision depth buffers while keeping the requested stencil buffer size fixed. This
	// handles common macOS configurations where 24-bit depth with 8-bit stencil is available, but 32-bit depth with
	// 8-bit stencil is not. Note that we don't bother with supporting the macOS OpenGL software renderer
	// (kCGLRendererGenericFloatID or kCGLRendererAppleSWID) as they max out at an OpenGL 2.1 compatibility profile,
	// which is well below our minimum requirements.
	auto TryChoosePixelFormat = [&](int requestedDepthBits, CGLPixelFormatObj& outPixelFormatObj, GLint& outNumFormats) -> CGLError {
		std::vector<CGLPixelFormatAttribute> attribs;
		attribs.push_back(kCGLPFAOpenGLProfile);
		attribs.push_back((CGLPixelFormatAttribute)kCGLOGLPVersion_3_2_Core);
		attribs.push_back(kCGLPFAColorSize);
		attribs.push_back((CGLPixelFormatAttribute)colorBits);
		attribs.push_back(kCGLPFAAlphaSize);
		attribs.push_back((CGLPixelFormatAttribute)alphaBits);
		attribs.push_back(kCGLPFADepthSize);
		attribs.push_back((CGLPixelFormatAttribute)requestedDepthBits);
		attribs.push_back(kCGLPFAStencilSize);
		attribs.push_back((CGLPixelFormatAttribute)stencilBits);
		attribs.push_back(kCGLPFAAccelerated);
		attribs.push_back(kCGLPFADoubleBuffer);
		attribs.push_back(kCGLPFAMinimumPolicy);
		attribs.push_back((CGLPixelFormatAttribute)0);

		outPixelFormatObj = nullptr;
		outNumFormats = 0;
		return CGLChoosePixelFormat(attribs.data(), &outPixelFormatObj, &outNumFormats);
	};
	CGLPixelFormatObj pixelFormatObj = nullptr;
	GLint numFormats = 0;
	auto cglChoosePixelFormatResult = TryChoosePixelFormat(depthBits, pixelFormatObj, numFormats);
	if ((cglChoosePixelFormatResult != kCGLNoError) || (numFormats == 0) || (pixelFormatObj == nullptr))
	{
		// Destroy the pixel format object if one was returned
		if (pixelFormatObj != nullptr)
		{
			CGLDestroyPixelFormat(pixelFormatObj);
			pixelFormatObj = nullptr;
		}

		// Define our possible depth buffer size fallback options
		std::vector<int> depthFallbacks;
		if (depthBits >= 32)
		{
			depthFallbacks.push_back(24);
			depthFallbacks.push_back(16);
		}
		else if (depthBits >= 24)
		{
			depthFallbacks.push_back(16);
		}

		// Iteratively attempt to select a pixel format with a lower depth buffer size
		for (int fallbackDepthBits : depthFallbacks)
		{
			// If we managed to select a pixel format, latch it, and break out of the fallback loop.
			cglChoosePixelFormatResult = TryChoosePixelFormat(fallbackDepthBits, pixelFormatObj, numFormats);
			if ((cglChoosePixelFormatResult == kCGLNoError) && (numFormats > 0) && (pixelFormatObj != nullptr))
			{
				break;
			}

			// Since we didn't get a pixel format, destroy the pixel format object if one was returned.
			if (pixelFormatObj != nullptr)
			{
				CGLDestroyPixelFormat(pixelFormatObj);
				pixelFormatObj = nullptr;
			}
		}
	}

	// If we didn't manage to select a compatible pixel format, log an error and abort any further processing.
	if (cglChoosePixelFormatResult != kCGLNoError)
	{
		log.Error("CGLChoosePixelFormat failed with error code {0}", cglChoosePixelFormatResult);
		return false;
	}
	if ((numFormats == 0) || (pixelFormatObj == nullptr))
	{
		log.Error("CGLChoosePixelFormat returned zero compatible formats");
		if (pixelFormatObj != nullptr)
		{
			CGLDestroyPixelFormat(pixelFormatObj);
		}
		return false;
	}

	// Retrieve information on the actual pixel format that was selected. Note that the driver may select one that
	// differs to our request. It may promote our depth buffer size for example above the precision that was requested,
	// if the driver knows that there's a performance advantage on the hardware for using a particular preferred format.
	int virtualScreenNo = 0;
	int actualColorBits = 0;
	int actualAlphaBits = 0;
	int actualDepthBits = 0;
	int actualStencilBits = 0;
	bool describePixelFormatSucceeded = true;
	describePixelFormatSucceeded &= (CGLDescribePixelFormat(pixelFormatObj, virtualScreenNo, kCGLPFAColorSize, &actualColorBits) == kCGLNoError);
	describePixelFormatSucceeded &= (CGLDescribePixelFormat(pixelFormatObj, virtualScreenNo, kCGLPFAAlphaSize, &actualAlphaBits) == kCGLNoError);
	describePixelFormatSucceeded &= (CGLDescribePixelFormat(pixelFormatObj, virtualScreenNo, kCGLPFADepthSize, &actualDepthBits) == kCGLNoError);
	describePixelFormatSucceeded &= (CGLDescribePixelFormat(pixelFormatObj, virtualScreenNo, kCGLPFAStencilSize, &actualStencilBits) == kCGLNoError);
	if (!describePixelFormatSucceeded)
	{
		log.Error("CGLDescribePixelFormat failed");
		CGLDestroyPixelFormat(pixelFormatObj);
		return false;
	}

	// Verify that the selected pixel format still satisfies the requested stencil buffer size. Depth precision may be
	// intentionally reduced by the fallback path above, but stencil precision must remain fixed.
	if (actualStencilBits < stencilBits)
	{
		log.Error("Selected PFD has insufficient stencil bits. Requested {0}, but got {1}.", stencilBits, actualStencilBits);
		CGLDestroyPixelFormat(pixelFormatObj);
		return false;
	}

	// If this is a debug build, print information on the selected pixel format.
	if (debugLoggingEnabled)
	{
		if ((colorBits != actualColorBits) || (alphaBits != actualAlphaBits) || (depthBits != actualDepthBits) || (stencilBits != actualStencilBits))
		{
			log.Info("Requested PFD with (C,A,D,S) of ({0},{1},{2},{3}), but got one with ({4},{5},{6},{7}).", colorBits, alphaBits, depthBits, stencilBits, actualColorBits, actualAlphaBits, actualDepthBits, actualStencilBits);
		}
		else
		{
			log.Info("Requested PFD with (C,A,D,S) of ({0},{1},{2},{3}), and got an exact match.", colorBits, alphaBits, depthBits, stencilBits);
		}
	}

	// Record information on the selected pixel format, and return true.
	pixelFormatInfo = {};
	pixelFormatInfo.pixelFormatObj = pixelFormatObj;
	pixelFormatInfo.virtualScreenNo = virtualScreenNo;
	pixelFormatInfo.colorBits = actualColorBits;
	pixelFormatInfo.alphaBits = actualAlphaBits;
	pixelFormatInfo.depthBits = actualDepthBits;
	pixelFormatInfo.stencilBits = actualStencilBits;
	return true;
}
#endif

#ifdef OPENGL_USE_PLATFORM_WIN32
//----------------------------------------------------------------------------------------
bool OpenGLRenderer::RetrieveOrCreateRenderingContextForWindow(HWND hwnd, HDC deviceContext, const PixelFormatInfo& pixelFormatInfo, HGLRC& renderingContext, size_t& renderingContextIndex, bool makeCurrent)
{
	// Return an existing rendering context if one exists
	bool hasFreeSlot = false;
	size_t freeSlotIndex = 0;
	for (size_t i = 0; i < _allocatedRenderingContexts.size(); ++i)
	{
		const auto& entry = _allocatedRenderingContexts[i];
		if (!entry.slotAllocated)
		{
			if (!hasFreeSlot)
			{
				hasFreeSlot = true;
				freeSlotIndex = i;
			}
		}
		else if (entry.hwnd == hwnd)
		{
			// Retrieve the existing rendering context
			renderingContext = entry.renderingContext;
			renderingContextIndex = i;

			// Make the rendering context current if requested
			if (makeCurrent && !ActivateRenderingContext(hwnd, deviceContext, renderingContext))
			{
				_log->Error("Failed to activate rendering context");
				return false;
			}
			return true;
		}
	}
	if (_enableDebugLogging)
	{
		_log->Info("Creating new rendering context for window");
	}

	// Initialize an OpenGL rendering context bound to the window
	HGLRC newContext = wglCreateContext(deviceContext);
	if (newContext == nullptr)
	{
		_log->Error("Failed to create OpenGL rendering context");
		return false;
	}

	// Attempt to activate our rendering context
	HDC currentDeviceContext = wglGetCurrentDC();
	HGLRC currentRenderingContext = wglGetCurrentContext();
	if (!ActivateRenderingContext(hwnd, deviceContext, newContext))
	{
		_log->Error("Failed to activate new OpenGL rendering context.");
		if (!ActivateRenderingContext(hwnd, currentDeviceContext, currentRenderingContext))
		{
			_log->Error("Failed to restore original OpenGL rendering context.");
		}
		wglDeleteContext(newContext);
		return false;
	}

	// Attempt to retrieve the wglCreateContextAttribsARB function
	static auto wglCreateContextAttribsARB = reinterpret_cast<PFNWGLCREATECONTEXTATTRIBSARBPROC>(wglGetProcAddress("wglCreateContextAttribsARB"));
	if (wglCreateContextAttribsARB == nullptr)
	{
		_log->Warning("Failed to locate wglCreateContextAttribsARB. Falling back to wglCreateContext.");
	}

	// If we managed to retrieve the wglCreateContextAttribsARB function, attempt to re-create the rendering context
	// with the new API.
	if (wglCreateContextAttribsARB != nullptr)
	{
		// Build the set of attributes to control the OpenGL rendering context creation
		const int targetVersionMajor = OPENGL_VERSION_MAJOR;
		const int targetVersionMinor = OPENGL_VERSION_MINOR;
		const int contextFlags = (_enableDebugLogging ? WGL_CONTEXT_DEBUG_BIT_ARB : 0);
		auto contextAttribs = {WGL_CONTEXT_MAJOR_VERSION_ARB, targetVersionMajor, WGL_CONTEXT_MINOR_VERSION_ARB, targetVersionMinor, WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB, WGL_CONTEXT_FLAGS_ARB, contextFlags, 0};

		// Attempt to create our new rendering context. Note that we provide the main rendering context as the shared
		// rendering context here if it has been defined, so we can share resources between this main context and all
		// other contexts which are sharing with it.
		HGLRC updatedContext = wglCreateContextAttribsARB(deviceContext, _mainRenderingContext, contextAttribs.begin());
		if (updatedContext != nullptr)
		{
			// Attempt to activate our rendering context
			if (!ActivateRenderingContext(hwnd, deviceContext, updatedContext))
			{
				_log->Error("Failed to activate new OpenGL rendering context.");
				if (!ActivateRenderingContext(hwnd, currentDeviceContext, currentRenderingContext))
				{
					_log->Error("Failed to restore original OpenGL rendering context.");
				}
				wglDeleteContext(newContext);
				return false;
			}

			// Delete the original wglCreateContext version of the render context
			wglDeleteContext(newContext);
			newContext = updatedContext;
		}
		else
		{
			_log->Warning("Call to wglCreateContextAttribsARB failed. Falling back to wglCreateContext.");
		}
	}

	// Record information on this allocated rendering context
	size_t selectedRenderingContextIndex;
	if (hasFreeSlot)
	{
		selectedRenderingContextIndex = freeSlotIndex;
		RenderingContextInfo& renderingContextInfo = _allocatedRenderingContexts[freeSlotIndex];
		renderingContextInfo.pixelFormatInfo = pixelFormatInfo;
		renderingContextInfo.hwnd = hwnd;
		renderingContextInfo.deviceContext = deviceContext;
		renderingContextInfo.renderingContext = newContext;
		renderingContextInfo.freeVertexArrayObjectIDs.clear();
		++renderingContextInfo.generationIndex;
		renderingContextInfo.slotAllocated = true;
	}
	else
	{
		selectedRenderingContextIndex = _allocatedRenderingContexts.size();
		RenderingContextInfo renderingContextInfo{};
		renderingContextInfo.pixelFormatInfo = pixelFormatInfo;
		renderingContextInfo.hwnd = hwnd;
		renderingContextInfo.deviceContext = deviceContext;
		renderingContextInfo.renderingContext = newContext;
		renderingContextInfo.generationIndex = 1;
		renderingContextInfo.slotAllocated = true;
		_allocatedRenderingContexts.push_back(renderingContextInfo);
	}

	// Initialize the default state for this new rendering context
	InitializeDefaultRenderContextState();

	// If we weren't asked to Make the rendering context current, restore the original rendering context.
	if (!makeCurrent && !ActivateRenderingContext(hwnd, currentDeviceContext, currentRenderingContext))
	{
		_log->Error("Failed to restore original OpenGL rendering context.");
		return false;
	}

	// Return the rendering context to the caller
	renderingContext = newContext;
	renderingContextIndex = selectedRenderingContextIndex;
	return true;
}
#endif

#ifdef OPENGL_USE_EGL
//----------------------------------------------------------------------------------------
bool OpenGLRenderer::RetrieveOrCreateRenderingContextForWindow(EGLDisplay display, EGLSurface window, const PixelFormatInfo& pixelFormatInfo, EGLContext& renderingContext, size_t& renderingContextIndex, bool makeCurrent)
{
	// Return an existing rendering context if one exists
	bool hasFreeSlot = false;
	size_t freeSlotIndex = 0;
	for (size_t i = 0; i < _allocatedRenderingContexts.size(); ++i)
	{
		const auto& entry = _allocatedRenderingContexts[i];
		if (!entry.slotAllocated)
		{
			if (!hasFreeSlot)
			{
				hasFreeSlot = true;
				freeSlotIndex = i;
			}
		}
		else if ((entry.display == display) && (entry.window == window))
		{
			// Retrieve the existing rendering context
			renderingContext = entry.renderingContext;
			renderingContextIndex = i;

			// Make the rendering context current if requested
			if (makeCurrent && !ActivateRenderingContext(display, window, renderingContext))
			{
				_log->Error("Failed to activate rendering context");
				return false;
			}
			return true;
		}
	}
	if (_enableDebugLogging)
	{
		_log->Info("Creating new rendering context for window");
	}

	// Create a new rendering context, shared with the main rendering context.
	EGLContext shareContext = _mainRenderingContext;
	const int targetVersionMajor = OPENGL_VERSION_MAJOR;
	const int targetVersionMinor = OPENGL_VERSION_MINOR;
	const int contextFlags = (_enableDebugLogging ? EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR : 0);
	EGLint contextAttribs[] = {EGL_CONTEXT_MAJOR_VERSION_KHR, targetVersionMajor, EGL_CONTEXT_MINOR_VERSION_KHR, targetVersionMinor, EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR, EGL_CONTEXT_FLAGS_KHR, contextFlags, EGL_NONE};
	EGLContext newContext = eglCreateContext(display, pixelFormatInfo.fbConfig, shareContext, &contextAttribs[0]);
	CheckEGLError(_log.get());
	if (newContext == nullptr)
	{
		_log->Error("Failed to create EGL context for window.");
		CheckEGLError(_log.get());
		return false;
	}

	// Attempt to activate our rendering context
	EGLDisplay currentDisplay = eglGetCurrentDisplay();
	EGLSurface currentSurface = eglGetCurrentSurface(EGL_DRAW);
	EGLContext currentRenderingContext = eglGetCurrentContext();
	CheckEGLError(_log.get());
	if (!ActivateRenderingContext(display, window, newContext))
	{
		_log->Error("Failed to activate new OpenGL rendering context.");
		if (!ActivateRenderingContext(currentDisplay, currentSurface, currentRenderingContext))
		{
			_log->Error("Failed to restore original OpenGL rendering context.");
		}
		eglDestroyContext(display, newContext);
		CheckEGLError(_log.get());
		return false;
	}

	// Record information on this allocated rendering context
	size_t selectedRenderingContextIndex;
	if (hasFreeSlot)
	{
		selectedRenderingContextIndex = freeSlotIndex;
		RenderingContextInfo& renderingContextInfo = _allocatedRenderingContexts[freeSlotIndex];
		renderingContextInfo.pixelFormatInfo = pixelFormatInfo;
		renderingContextInfo.display = display;
		renderingContextInfo.window = window;
		renderingContextInfo.renderingContext = newContext;
		renderingContextInfo.freeVertexArrayObjectIDs.clear();
		++renderingContextInfo.generationIndex;
		renderingContextInfo.slotAllocated = true;
	}
	else
	{
		selectedRenderingContextIndex = _allocatedRenderingContexts.size();
		RenderingContextInfo renderingContextInfo{};
		renderingContextInfo.pixelFormatInfo = pixelFormatInfo;
		renderingContextInfo.display = display;
		renderingContextInfo.window = window;
		renderingContextInfo.renderingContext = newContext;
		renderingContextInfo.generationIndex = 1;
		renderingContextInfo.slotAllocated = true;
		_allocatedRenderingContexts.push_back(renderingContextInfo);
	}

	// Initialize the default state for this new rendering context
	InitializeDefaultRenderContextState();

	// If we weren't asked to Make the rendering context current, restore the original rendering context.
	if (!makeCurrent && !ActivateRenderingContext(currentDisplay, currentSurface, currentRenderingContext))
	{
		_log->Error("Failed to restore original OpenGL rendering context.");
		return false;
	}

	// Return the rendering context to the caller
	renderingContext = newContext;
	renderingContextIndex = selectedRenderingContextIndex;
	return true;
}
#endif

#if defined(OPENGL_USE_PLATFORM_XLIB) && !defined(OPENGL_USE_EGL)
//----------------------------------------------------------------------------------------
bool OpenGLRenderer::RetrieveOrCreateRenderingContextForWindow(::Display* display, ::Window window, const PixelFormatInfo& pixelFormatInfo, GLXContext& renderingContext, size_t& renderingContextIndex, bool makeCurrent)
{
	// Return an existing rendering context if one exists
	bool hasFreeSlot = false;
	size_t freeSlotIndex = 0;
	for (size_t i = 0; i < _allocatedRenderingContexts.size(); ++i)
	{
		const auto& entry = _allocatedRenderingContexts[i];
		if (!entry.slotAllocated)
		{
			if (!hasFreeSlot)
			{
				hasFreeSlot = true;
				freeSlotIndex = i;
			}
		}
		else if ((entry.display == display) && (entry.window == window))
		{
			// Retrieve the existing rendering context
			renderingContext = entry.renderingContext;
			renderingContextIndex = i;

			// Make the rendering context current if requested
			if (makeCurrent && !ActivateRenderingContext(display, window, renderingContext))
			{
				_log->Error("Failed to activate rendering context");
				return false;
			}
			return true;
		}
	}
	if (_enableDebugLogging)
	{
		_log->Info("Creating new rendering context for window");
	}

	// Create a new rendering context, shared with the main rendering context.
	static auto glXCreateContextAttribsARB = reinterpret_cast<PFNGLXCREATECONTEXTATTRIBSARBPROC>(glXGetProcAddressARB(reinterpret_cast<const GLubyte*>("glXCreateContextAttribsARB")));
	GLXContext shareContext = _mainRenderingContext;
	GLXContext newContext = nullptr;
	if (glXCreateContextAttribsARB != nullptr)
	{
		const int targetVersionMajor = OPENGL_VERSION_MAJOR;
		const int targetVersionMinor = OPENGL_VERSION_MINOR;
		const int contextFlags = (_enableDebugLogging ? GLX_CONTEXT_DEBUG_BIT_ARB : 0);
		int contextAttribs[] = {GLX_CONTEXT_MAJOR_VERSION_ARB, targetVersionMajor, GLX_CONTEXT_MINOR_VERSION_ARB, targetVersionMinor, GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB, GLX_CONTEXT_FLAGS_ARB, contextFlags, 0};
		newContext = glXCreateContextAttribsARB(display, pixelFormatInfo.fbConfig, shareContext, X11Constants::kTrue, &contextAttribs[0]);
		if (newContext == nullptr)
		{
			_log->Warning("glXCreateContextAttribsARB failed. Falling back to glXCreateNewContext.");
		}
	}
	if (newContext == nullptr)
	{
		newContext = glXCreateNewContext(display, pixelFormatInfo.fbConfig, GLX_RGBA_TYPE, shareContext, X11Constants::kTrue);
		if (newContext == nullptr)
		{
			_log->Error("Failed to create GLX context for window.");
			return false;
		}
	}

	// Attempt to activate our rendering context
	::Display* currentDisplay = glXGetCurrentDisplay();
	GLXDrawable currentDrawable = glXGetCurrentDrawable();
	GLXContext currentRenderingContext = glXGetCurrentContext();
	if (!ActivateRenderingContext(display, window, newContext))
	{
		_log->Error("Failed to activate new OpenGL rendering context.");
		if (!ActivateRenderingContext(currentDisplay, currentDrawable, currentRenderingContext))
		{
			_log->Error("Failed to restore original OpenGL rendering context.");
		}
		glXDestroyContext(display, newContext);
		return false;
	}

	// Record information on this allocated rendering context
	size_t selectedRenderingContextIndex;
	if (hasFreeSlot)
	{
		selectedRenderingContextIndex = freeSlotIndex;
		RenderingContextInfo& renderingContextInfo = _allocatedRenderingContexts[freeSlotIndex];
		renderingContextInfo.pixelFormatInfo = pixelFormatInfo;
		renderingContextInfo.display = display;
		renderingContextInfo.window = window;
		renderingContextInfo.renderingContext = newContext;
		renderingContextInfo.freeVertexArrayObjectIDs.clear();
		++renderingContextInfo.generationIndex;
		renderingContextInfo.slotAllocated = true;
	}
	else
	{
		selectedRenderingContextIndex = _allocatedRenderingContexts.size();
		RenderingContextInfo renderingContextInfo{};
		renderingContextInfo.pixelFormatInfo = pixelFormatInfo;
		renderingContextInfo.display = display;
		renderingContextInfo.window = window;
		renderingContextInfo.renderingContext = newContext;
		renderingContextInfo.generationIndex = 1;
		renderingContextInfo.slotAllocated = true;
		_allocatedRenderingContexts.push_back(renderingContextInfo);
	}

	// Initialize the default state for this new rendering context
	InitializeDefaultRenderContextState();

	// If we weren't asked to Make the rendering context current, restore the original rendering context.
	if (!makeCurrent && !ActivateRenderingContext(currentDisplay, currentDrawable, currentRenderingContext))
	{
		_log->Error("Failed to restore original OpenGL rendering context.");
		return false;
	}

	// Return the rendering context to the caller
	renderingContext = newContext;
	renderingContextIndex = selectedRenderingContextIndex;
	return true;
}
#endif

#ifdef OPENGL_USE_PLATFORM_APPKIT
//----------------------------------------------------------------------------------------
bool OpenGLRenderer::RetrieveOrCreateRenderingContextForWindow(NSView* window, const PixelFormatInfo& pixelFormatInfo, CGLContextObj& renderingContext, size_t& renderingContextIndex, bool makeCurrent)
{
	// Return an existing rendering context if one exists. Note that on macOS, we need to allocate the rendering context
	// for framebuffers bound to windows in the context of the calling thread, as we need to bind it to the NSView under
	// AppKit. This needs to happen on the main UI thread, so we can't do it automatically on our render thread. This
	// means we need to make this function thread-safe, so it can be called from both the render thread and the thread
	// context of the caller. We use a mutex to protect access to the _allocatedRenderingContexts collection in order to
	// provide that.
	std::unique_lock<std::mutex> lock(_renderingContextMutex);
	bool hasFreeSlot = false;
	size_t freeSlotIndex = 0;
	for (size_t i = 0; i < _allocatedRenderingContexts.size(); ++i)
	{
		const auto& entry = _allocatedRenderingContexts[i];
		if (!entry.slotAllocated)
		{
			if (!hasFreeSlot)
			{
				hasFreeSlot = true;
				freeSlotIndex = i;
			}
		}
		else if (entry.window == window)
		{
			// Retrieve the existing rendering context
			renderingContext = entry.renderingContext;
			renderingContextIndex = i;

			// Make the rendering context current if requested
			if (makeCurrent && !ActivateRenderingContext(renderingContext))
			{
				_log->Error("Failed to activate rendering context");
				return false;
			}
			return true;
		}
	}
	if (_enableDebugLogging)
	{
		_log->Info("Creating new rendering context for window");
	}

	// Initialize an OpenGL rendering context
	CGLContextObj newContext = nullptr;
	auto cglCreateContextReturn = CGLCreateContext(pixelFormatInfo.pixelFormatObj, _mainRenderingContext, &newContext);
	CGLDestroyPixelFormat(pixelFormatInfo.pixelFormatObj);
	if (cglCreateContextReturn != kCGLNoError)
	{
		_log->Error("CGLCreateContext failed with error code: {0}", cglCreateContextReturn);
		return false;
	}

	// Attempt to activate our rendering context
	CGLContextObj currentRenderingContext = CGLGetCurrentContext();
	if (!ActivateRenderingContext(newContext))
	{
		_log->Error("Failed to activate new OpenGL rendering context.");
		if (!ActivateRenderingContext(currentRenderingContext))
		{
			_log->Error("Failed to restore original OpenGL rendering context.");
		}
		CGLReleaseContext(newContext);
		return false;
	}

	// Record information on this allocated rendering context
	size_t selectedRenderingContextIndex;
	if (hasFreeSlot)
	{
		selectedRenderingContextIndex = freeSlotIndex;
		RenderingContextInfo& renderingContextInfo = _allocatedRenderingContexts[freeSlotIndex];
		renderingContextInfo.pixelFormatInfo = pixelFormatInfo;
		renderingContextInfo.window = window;
		renderingContextInfo.renderingContext = newContext;
		renderingContextInfo.freeVertexArrayObjectIDs.clear();
		++renderingContextInfo.generationIndex;
		renderingContextInfo.slotAllocated = true;
	}
	else
	{
		RenderingContextInfo renderingContextInfo{};
		renderingContextInfo.pixelFormatInfo = pixelFormatInfo;
		renderingContextInfo.window = window;
		renderingContextInfo.renderingContext = newContext;
		renderingContextInfo.generationIndex = 1;
		renderingContextInfo.slotAllocated = true;
		selectedRenderingContextIndex = _allocatedRenderingContexts.size();
		_allocatedRenderingContexts.push_back(renderingContextInfo);
	}

	// Initialize the default state for this new rendering context
	InitializeDefaultRenderContextState();

	// If we weren't asked to Make the rendering context current, restore the original rendering context.
	if (!makeCurrent && !ActivateRenderingContext(currentRenderingContext))
	{
		_log->Error("Failed to restore original OpenGL rendering context.");
		return false;
	}

	// Return the rendering context to the caller
	renderingContext = newContext;
	renderingContextIndex = selectedRenderingContextIndex;
	return true;
}
#endif

#ifdef OPENGL_USE_PLATFORM_WIN32
//----------------------------------------------------------------------------------------
bool OpenGLRenderer::ActivateRenderingContext(HWND hwnd, HDC deviceContext, HGLRC renderingContext) const
{
	// Ensure the supplied rendering context is set as the active rendering context. Note that this method incorporates
	// an automatic retry mechanism. On some hardware, spurious failures have been observed from this method. This has
	// been seen on Intel hardware with both ERROR_INVALID_HANDLE (6) and ERROR_TRANSFORM_NOT_SUPPORTED (2004) error
	// codes. In both cases, calling the method a second time succeeds. This is almost certainly a driver issue, and
	// other people have reported similar issues online, with no clear resolution. To be defensive against these kind of
	// driver errors now and in the future, we perform several retry attempts where a failure occurs, giving us a high
	// likelihood of recovering from a spurious failure.
	HDC currentDeviceContext = wglGetCurrentDC();
	HGLRC currentRenderingContext = wglGetCurrentContext();
	bool deviceContextNeedsSwitching = (currentDeviceContext != deviceContext);
	bool renderingContextNeedsSwitching = (currentRenderingContext != renderingContext);
	if (deviceContextNeedsSwitching || renderingContextNeedsSwitching)
	{
		// This is something we never ever want to do, however we've found real world cases where Intel graphics drivers
		// mess up when clear operations are performed against offscreen buffers without any draw operations being
		// performed on them, then another rendering context is activated to use the texture outputs from those buffers
		// in a compositing pass. In this case, the rendering context for the window enters some kind of error state,
		// with draw operations on that context appearing to be ignored from that point on, and nothing can be read from
		// the window framebuffer, despite no errors being reported. If a draw call occurs to the offscreen framebuffer,
		// things work as expected, and things also work correctly if debug contexts are used, but otherwise if this
		// case is encountered, rendering doesn't work correctly on Intel hardware only. It was discovered that we could
		// work around this issue by calling glFinish() prior to switching out the rendering context, so if Intel
		// hardware is being used we perform that operation here. If the rendering context itself matches however, and
		// we're only changing the target draw surface, we don't need to perform this step.
		if (renderingContextNeedsSwitching && _finishWhenSwitchingContexts)
		{
			glFinish();
		}

		// Attempt to activate the supplied rendering context
		static const int maxRetryCount = 5;
		int retryCount = 0;
		while (wglMakeCurrent(deviceContext, renderingContext) != TRUE)
		{
			if (retryCount >= maxRetryCount)
			{
				_log->Error("wglMakeCurrent failed. GetLastError: {0}. Call has failed {1} times, giving up.", GetLastError(), retryCount);
				return false;
			}
			_log->Warning("wglMakeCurrent failed. GetLastError: {0}. Retrying operation.", GetLastError());
			++retryCount;
		}
	}
	return true;
}
#endif

#ifdef OPENGL_USE_EGL
//----------------------------------------------------------------------------------------
bool OpenGLRenderer::ActivateRenderingContext(EGLDisplay display, EGLSurface surface, EGLContext renderingContext) const
{
	EGLDisplay currentDisplay = eglGetCurrentDisplay();
	EGLSurface currentDrawSurface = eglGetCurrentSurface(EGL_DRAW);
	EGLSurface currentReadSurface = eglGetCurrentSurface(EGL_READ);
	EGLContext currentContext = eglGetCurrentContext();
	CheckEGLError(_log.get());
	bool displayNeedsSwitching = (currentDisplay != display);
	bool surfaceNeedsSwitching = (currentDrawSurface != surface) || (currentReadSurface != surface);
	bool contextNeedsSwitching = (currentContext != renderingContext);
	if (displayNeedsSwitching || surfaceNeedsSwitching || contextNeedsSwitching)
	{
		// Attempt to activate the supplied rendering context
		if (eglMakeCurrent(display, surface, surface, renderingContext) != EGL_TRUE)
		{
			CheckEGLError(_log.get());
			_log->Error("eglMakeCurrent failed in ActivateRenderingContext.");
			return false;
		}
	}
	CheckEGLError(_log.get());
	return true;
}
#endif

#if defined(OPENGL_USE_PLATFORM_XLIB) && !defined(OPENGL_USE_EGL)
//----------------------------------------------------------------------------------------
bool OpenGLRenderer::ActivateRenderingContext(::Display* display, ::Window window, GLXContext renderingContext) const
{
	::Display* currentDisplay = glXGetCurrentDisplay();
	GLXDrawable currentDrawable = glXGetCurrentDrawable();
	GLXContext currentRenderingContext = glXGetCurrentContext();
	bool displayNeedsSwitching = (currentDisplay != display);
	bool drawableNeedsSwitching = (currentDrawable != window);
	bool renderingContextNeedsSwitching = (currentRenderingContext != renderingContext);
	if (displayNeedsSwitching || drawableNeedsSwitching || renderingContextNeedsSwitching)
	{
		// Attempt to activate the supplied rendering context
		if (glXMakeContextCurrent(display, window, window, renderingContext) != X11Constants::kTrue)
		{
			_log->Error("glXMakeContextCurrent failed in ActivateRenderingContext.");
			return false;
		}
	}
	return true;
}
#endif

#ifdef OPENGL_USE_PLATFORM_APPKIT
//----------------------------------------------------------------------------------------
bool OpenGLRenderer::ActivateRenderingContext(CGLContextObj renderingContext) const
{
	CGLContextObj currentRenderingContext = CGLGetCurrentContext();
	bool renderingContextNeedsSwitching = (currentRenderingContext != renderingContext);
	if (renderingContextNeedsSwitching)
	{
		// Unlike other platforms, macOS does not generate an implicit glFlush() when switching contexts. We therefore
		// issue one explicitly here. See "Using glFlush Effectively" in the following:
		// https://developer.apple.com/library/archive/documentation/GraphicsImaging/Conceptual/OpenGL-MacProgGuide/opengl_designstrategies/opengl_designstrategies.html
		glFlush();

		// Attempt to activate the supplied rendering context
		if (CGLSetCurrentContext(renderingContext) != kCGLNoError)
		{
			_log->Error("CGLSetCurrentContext failed in ActivateRenderingContext.");
			return false;
		}
	}
	return true;
}
#endif

//----------------------------------------------------------------------------------------
GLuint OpenGLRenderer::GenerateVertexArrayObject(size_t renderingContextIndex)
{
	// Pull a vertex array object ID from the back of the free list, or allocate a new one if the free list is empty.
	// Note that we do NOT need to track the lifetime of these vertex arrays. The renderable nodes will keep track of
	// them from here, and we return them to the "free" pool when they're deleted. If the rendering context is released
	// first, the VAOs are freed automatically, as they're container objects and exclusively owned by the rendering
	// context, not shared (OpenGL 4.3 Chapter 5 - Shared Objects and Multiple Contexts). This is convenient, because
	// we don't want to make the context current just to free them. They take up a very small amount of memory, so
	// keeping them in a free pool isn't a problem. As a result, we never actually call glDeleteVertexArrays.
#ifdef OPENGL_USE_PLATFORM_APPKIT
	std::unique_lock<std::mutex> lock(_renderingContextMutex);
#endif
	GLuint vertexArrayId;
	auto& renderingContextEntry = _allocatedRenderingContexts[renderingContextIndex];
	if (!renderingContextEntry.freeVertexArrayObjectIDs.empty())
	{
		vertexArrayId = renderingContextEntry.freeVertexArrayObjectIDs.back();
		renderingContextEntry.freeVertexArrayObjectIDs.pop_back();
	}
	else
	{
		glGenVertexArrays(1, &vertexArrayId);
		CheckGLError(_log.get());
	}
	return vertexArrayId;
}

#ifdef OPENGL_USE_PLATFORM_WIN32
//----------------------------------------------------------------------------------------
void OpenGLRenderer::DeleteRenderingContext(size_t renderingContextIndex)
{
	auto& entry = _allocatedRenderingContexts[renderingContextIndex];
	wglDeleteContext(entry.renderingContext);
	entry.renderingContext = nullptr;
	entry.slotAllocated = false;
}
#endif

#ifdef OPENGL_USE_EGL
//----------------------------------------------------------------------------------------
void OpenGLRenderer::DeleteRenderingContext(size_t renderingContextIndex)
{
	auto& entry = _allocatedRenderingContexts[renderingContextIndex];
	eglDestroyContext(entry.display, entry.renderingContext);
	entry.display = EGL_NO_DISPLAY;
	entry.renderingContext = nullptr;
	entry.slotAllocated = false;
}
#endif

#if defined(OPENGL_USE_PLATFORM_XLIB) && !defined(OPENGL_USE_EGL)
//----------------------------------------------------------------------------------------
void OpenGLRenderer::DeleteRenderingContext(size_t renderingContextIndex)
{
	auto& entry = _allocatedRenderingContexts[renderingContextIndex];
	glXDestroyContext(entry.display, entry.renderingContext);
	entry.display = nullptr;
	entry.renderingContext = nullptr;
	entry.slotAllocated = false;
}
#endif

#ifdef OPENGL_USE_PLATFORM_APPKIT
//----------------------------------------------------------------------------------------
void OpenGLRenderer::DeleteRenderingContext(size_t renderingContextIndex)
{
	std::unique_lock<std::mutex> lock(_renderingContextMutex);
	auto& entry = _allocatedRenderingContexts[renderingContextIndex];
	CGLDestroyContext(entry.renderingContext);
	entry.renderingContext = nullptr;
	entry.slotAllocated = false;
}
#endif

//----------------------------------------------------------------------------------------
void OpenGLRenderer::InitializeDefaultRenderContextState() const
{
	// Initialize debug logging if required
#ifdef GL_KHR_debug
	if ((GLAD_GL_KHR_debug != 0) && _enableDebugLogging)
	{
		// Enable debug logging with all messages being directed to our logging callback
		glEnable(GL_DEBUG_OUTPUT);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
		glDebugMessageCallback(MessageCallback, this);

		// Disable API_ID_REDUNDANT_FBO notifications. This occurs when you render to window surfaces in two consecutive
		// render passes, or when a frame ends with rendering to a window surface, and the next frame starts with
		// rendering to a window surface. We don't care to optimize for this case, since the real world performance
		// impact is almost certainly zero, and rendering directly to a single window only really happens in example
		// programs.
		GLuint messageIdRedundantFbo = 8;
		glDebugMessageControl(GL_DEBUG_SOURCE_API, GL_DEBUG_TYPE_PERFORMANCE, GL_DONT_CARE, 1, &messageIdRedundantFbo, GL_FALSE);

		// Insert one custom notification report that debug logging has been initialized
		glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_MARKER, 1, GL_DEBUG_SEVERITY_NOTIFICATION, -1, "GL_KHR_debug initialized");
		CheckGLError(_log.get());
	}
	if ((GLAD_GL_KHR_debug != 0) && _enabledOptions.find(Options::EnableRenderMarkers) != _enabledOptions.end())
	{
		// Enable debug output for render marker support
		glEnable(GL_DEBUG_OUTPUT);
	}
#endif

	// Set the provoking vertex to the first coordinate for all primitive types, instead of the OpenGL default of the
	// last vertex. This brings OpenGL into line with the default convention under Vulkan and Direct3D. Although this
	// behaviour is configurable under Vulkan, in Direct3D it is fixed and cannot be changed, so we can't make this
	// configurable over our unified API.
	glProvokingVertex(GL_FIRST_VERTEX_CONVENTION);
}

//----------------------------------------------------------------------------------------
// Settings methods
//----------------------------------------------------------------------------------------
bool OpenGLRenderer::DebugLoggingEnabled() const
{
	return _enableDebugLogging;
}

//----------------------------------------------------------------------------------------
bool OpenGLRenderer::UsePixelBufferObjectsForFrameCapture() const
{
	// Select whether we use Pixel Buffer Objects (PBOs) to read from framebuffers during framebuffer output capture, or
	// whether we just call glReadPixels synchronously from the client. Using PBOs is significantly faster with no real
	// disadvantages, so it's enabled always by default, but the alternate method of synchronous client calls is
	// supported for reference.
	return true;
}

//----------------------------------------------------------------------------------------
bool OpenGLRenderer::CaptureWindowFramebuffersFromFrontBuffer() const
{
	// If we're not using Pixel Buffer Objects (PBOs) to read from framebuffers during framebuffer output capture,
	// select whether we're reading window framebuffers from the front buffer or the back buffer. When using PBOs,
	// capture always occurs from the back buffer. Note that when not using PBOs, we've observed that under the Mesa
	// llvmpipe software renderer on Linux, reading from the front buffer fails. We've also seen that capturing from the
	// front buffer is unreliable on Linux under EGL. We therefore fall back to using back buffer reading in these cases
	// as a workaround. We prefer reading from the front buffers by default, as doing a blocking glReadPixels call prior
	// to swapping the buffers increases latency to get the next frame shown on the screen. We could also theoretically
	// advance the frame draw process further, without blocking when reading from front buffers, IE, we could overlap
	// the read with advancing the build to draw state, however we currently don't bother, as reading using PBOs is a
	// faster, preferred method anyway, and that can't be delayed in this manner.
	bool captureWindowFramebuffersFromFrontBuffer = !_usingSoftwareRenderer;
#ifdef OPENGL_USE_EGL
	captureWindowFramebuffersFromFrontBuffer = false;
#endif
	return captureWindowFramebuffersFromFrontBuffer;
}

//----------------------------------------------------------------------------------------
// Extension methods
//----------------------------------------------------------------------------------------
bool OpenGLRenderer::OpenGLExtensionPresent(const std::string& name) const
{
	bool extensionPresent = (_availableExtensions.find(name) != _availableExtensions.end());
	return extensionPresent;
}

//----------------------------------------------------------------------------------------
bool OpenGLRenderer::SpirvShadersSupported() const
{
#ifdef GL_ARB_gl_spirv
	return (GLAD_GL_ARB_gl_spirv != 0);
#else
	return false;
#endif
}

} // namespace cobalt::graphics
