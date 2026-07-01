// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "OpenGLGraphicsDeviceEnumerator.h"
#include "OpenGLDebug.h"
#include "OpenGLGraphicsDevice.h"
#include "OpenGLHeaders.h"
#include "OpenGLRenderer.h"
#include <functional>
#include <memory>
#include <sstream>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
#ifdef OPENGL_USE_PLATFORM_WIN32
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
// Constructors
//----------------------------------------------------------------------------------------
OpenGLGraphicsDeviceEnumerator::OpenGLGraphicsDeviceEnumerator(cobalt::logging::ILogger::unique_ptr log)
{
	_log = (log != nullptr ? std::move(log) : cobalt::logging::ILogger::unique_ptr(new cobalt::logging::NullLogger()));
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void OpenGLGraphicsDeviceEnumerator::Delete()
{
	delete this;
}

//----------------------------------------------------------------------------------------
// Device methods
//----------------------------------------------------------------------------------------
SuccessToken OpenGLGraphicsDeviceEnumerator::EnumerateDevices(EnumerationFlags flags)
{
	// Clear any existing device entries
	_readDeviceInfo = false;
	_devices.clear();
	_filteredDevices.clear();

	// Spawn a worker thread to handle device enumeration
	std::thread workerThread(std::bind(std::mem_fn(&OpenGLGraphicsDeviceEnumerator::ReadOpenGLGraphicsDeviceInfo), this, flags));
	workerThread.join();

	// Initialize the filtered device set
	ClearDeviceFilters();

	// Return the result to the caller
	return _readDeviceInfo;
}

#ifdef OPENGL_USE_PLATFORM_WIN32
//----------------------------------------------------------------------------------------
bool OpenGLGraphicsDeviceEnumerator::ReadOpenGLGraphicsDeviceInfo(EnumerationFlags flags)
{
	// Register a dummy window class
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
	HWND renderWindowHandle = CreateWindowExW(0, windowClassName.c_str(), windowName.c_str(), WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, hinstance, reinterpret_cast<LPVOID>(this));
	if (renderWindowHandle == nullptr)
	{
		_log->Error("Failed to create dummy OpenGL window");
		return false;
	}

	// Bind to our dummy window. We only do this to allow us to create a rendering context.
	HDC deviceContext;
	OpenGLRenderer::PixelFormatInfo pixelFormatInfo = {};
	if (!OpenGLRenderer::SelectWindowPixelFormat(renderWindowHandle, IFrameBuffer::WindowColorSpaceMode::Default, IFrameBuffer::WindowDepthStencilMode::None, deviceContext, pixelFormatInfo, *_log, false))
	{
		_log->Error("Failed to bind to dummy OpenGL window");
		DestroyWindow(renderWindowHandle);
		return false;
	}

	// Initialize an OpenGL rendering context bound to the dummy window
	HGLRC renderingContext = wglCreateContext(deviceContext);
	if (renderingContext == nullptr)
	{
		_log->Error("Failed to create OpenGL rendering context");
		ReleaseDC(renderWindowHandle, deviceContext);
		DestroyWindow(renderWindowHandle);
		return false;
	}

	// Attempt to activate our rendering context
	if (wglMakeCurrent(deviceContext, renderingContext) != TRUE)
	{
		_log->Error("Failed to activate OpenGL rendering context. GetLastError: {0}", GetLastError());
		wglDeleteContext(renderingContext);
		ReleaseDC(renderWindowHandle, deviceContext);
		DestroyWindow(renderWindowHandle);
		return false;
	}

	// Attempt to retrieve the wglCreateContextAttribsARB function
	auto wglCreateContextAttribsARB = reinterpret_cast<PFNWGLCREATECONTEXTATTRIBSARBPROC>(wglGetProcAddress("wglCreateContextAttribsARB"));
	if (wglCreateContextAttribsARB == nullptr)
	{
		_log->Warning("Failed to locate wglCreateContextAttribsARB. Falling back to wglCreateContext.");
	}

	// If we managed to retrieve the wglCreateContextAttribsARB function, attempt to re-create the rendering context
	// with the new API.
	if (wglCreateContextAttribsARB != nullptr)
	{
		// Build the set of attributes to control the OpenGL rendering context creation
		constexpr int targetVersionMajor = OPENGL_VERSION_MAJOR;
		constexpr int targetVersionMinor = OPENGL_VERSION_MINOR;
		const int contextFlags = (((flags & EnumerationFlags::NativeApiValidation) == EnumerationFlags::NativeApiValidation) ? WGL_CONTEXT_DEBUG_BIT_ARB : 0);
		auto contextAttribs = {WGL_CONTEXT_MAJOR_VERSION_ARB, targetVersionMajor, WGL_CONTEXT_MINOR_VERSION_ARB, targetVersionMinor, WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB, WGL_CONTEXT_FLAGS_ARB, contextFlags, 0};

		// Attempt to create our new rendering context
		HGLRC renderingContextNew = wglCreateContextAttribsARB(deviceContext, nullptr, contextAttribs.begin());
		if (renderingContextNew == nullptr)
		{
			_log->Warning("Call to wglCreateContextAttribsARB failed. Falling back to wglCreateContext.");
		}
		else
		{
			wglDeleteContext(renderingContext);
			renderingContext = renderingContextNew;
		}

		// Attempt to activate our rendering context
		if (wglMakeCurrent(deviceContext, renderingContext) != TRUE)
		{
			_log->Error("Failed to activate OpenGL rendering context. GetLastError: {0}", GetLastError());
			wglDeleteContext(renderingContext);
			ReleaseDC(renderWindowHandle, deviceContext);
			DestroyWindow(renderWindowHandle);
			return false;
		}
	}

	// Initialize glad
	if (gladLoadGL() == 0)
	{
		_log->Error("Failed to initialize GLAD");
		wglMakeCurrent(nullptr, nullptr);
		wglDeleteContext(renderingContext);
		ReleaseDC(renderWindowHandle, deviceContext);
		DestroyWindow(renderWindowHandle);
		return false;
	}

	// If we failed to locate the wglCreateContextAttribsARB method, it's entirely possible we've been supplied an
	// OpenGL implementation which is too old to meet our minimum requirements, as we weren't able to specify the target
	// version. This will currently occur over Remote Desktop connections with GeForce drivers for example, when the
	// Microsoft software adapter will be loaded. As an imperfect yet effective test for this case, we attempt to
	// retrieve the value of "GL_MAX_UNIFORM_BUFFER_BINDINGS", which is only supported on OpenGL 3.1 or above. If this
	// call fails with GL_INVALID_ENUM, we know we're on an earlier version and can't continue.
	if (wglCreateContextAttribsARB == nullptr)
	{
		GLenum error = glGetError();
		int dummyRead = 0;
		glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &dummyRead);
		error = glGetError();
		if (error == GL_INVALID_ENUM)
		{
			_log->Warning("Unable to use OpenGL graphics device. The graphics device doesn't appear to meet the minimum OpenGL version requirements.");
			wglMakeCurrent(nullptr, nullptr);
			wglDeleteContext(renderingContext);
			ReleaseDC(renderWindowHandle, deviceContext);
			DestroyWindow(renderWindowHandle);
			_readDeviceInfo = true;
			return true;
		}
	}

	// Create a graphics device object, and attempt to load device information into it.
	auto device = std::make_unique<OpenGLGraphicsDevice>(_log.get(), false);
	if (!device->ReadDeviceInfo())
	{
		_log->Error("Failed to read OpenGL device info");
		wglMakeCurrent(nullptr, nullptr);
		wglDeleteContext(renderingContext);
		ReleaseDC(renderWindowHandle, deviceContext);
		DestroyWindow(renderWindowHandle);
		return false;
	}
	_devices.push_back(std::move(device));

	// Release our allocated resources
	wglMakeCurrent(nullptr, nullptr);
	wglDeleteContext(renderingContext);
	ReleaseDC(renderWindowHandle, deviceContext);
	DestroyWindow(renderWindowHandle);

	// Flag that we've successfully read device info, and return true to the caller.
	_readDeviceInfo = true;
	return true;
}
#endif

#ifdef OPENGL_USE_EGL
//----------------------------------------------------------------------------------------
bool OpenGLGraphicsDeviceEnumerator::ReadOpenGLGraphicsDeviceInfo(EnumerationFlags flags)
{
	// Attempt to locate EGL extension entry points for device enumeration
	static auto eglQueryDevicesEXT = reinterpret_cast<PFNEGLQUERYDEVICESEXTPROC>(eglGetProcAddress("eglQueryDevicesEXT"));
	static auto eglGetPlatformDisplayEXT = reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(eglGetProcAddress("eglGetPlatformDisplayEXT"));
	static auto eglQueryDeviceStringEXT = reinterpret_cast<PFNEGLQUERYDEVICESTRINGEXTPROC>(eglGetProcAddress("eglQueryDeviceStringEXT"));

	// Mesa's surfaceless platform is useful on hosted Linux runners where there may be no X11, Wayland, or DRM render
	// node available, but llvmpipe can still provide a valid headless OpenGL implementation.
	bool supportsSurfacelessMesaPlatform = false;
	if ((flags & EnumerationFlags::HeadlessRendering) != EnumerationFlags::None)
	{
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
	}

	// Build a list of EGL device display objects to probe. If device enumeration is supported, we'll create one display
	// per device, otherwise we will probe the default display only. Note that these "device" display objects cannot
	// bind to window surfaces. We're only creating these devices to enumerate availability and features.
	struct DisplayEntry
	{
		EGLDisplay display = EGL_NO_DISPLAY;
		EGLDeviceEXT device = nullptr;
	};
	std::vector<DisplayEntry> displayList;
	bool canEnumerateDevices = (eglQueryDevicesEXT != nullptr) && (eglGetPlatformDisplayEXT != nullptr);
	if (canEnumerateDevices)
	{
		// Query the number of EGL devices available
		EGLint deviceCount = 0;
		if ((eglQueryDevicesEXT(0, nullptr, &deviceCount) != EGL_TRUE) || (deviceCount <= 0))
		{
			CheckEGLError(_log.get());
			_log->Warning("eglQueryDevicesEXT failed. Falling back to platform-based EGL display probing.");
		}
		else
		{
			// Retrieve the device handles
			std::vector<EGLDeviceEXT> deviceList(deviceCount);
			if ((eglQueryDevicesEXT(deviceCount, deviceList.data(), &deviceCount) != EGL_TRUE) || (deviceCount <= 0))
			{
				CheckEGLError(_log.get());
				_log->Warning("eglQueryDevicesEXT failed to retrieve device list. Falling back to platform-based EGL display probing.");
			}
			else
			{
				// Create an EGLDisplay for each enumerated device
				displayList.reserve(deviceCount);
				for (size_t i = 0; i < (size_t)deviceCount; ++i)
				{
					EGLDisplay display = eglGetPlatformDisplayEXT(EGL_PLATFORM_DEVICE_EXT, deviceList[i], nullptr);
					if (display == EGL_NO_DISPLAY)
					{
						CheckEGLError(_log.get());
						_log->Warning("eglGetPlatformDisplayEXT(EGL_PLATFORM_DEVICE_EXT) failed for device {0}", i);
						continue;
					}
					displayList.push_back({display, deviceList[i]});
				}
				if (displayList.empty())
				{
					_log->Warning("No usable EGL_PLATFORM_DEVICE_EXT displays were created. Falling back to platform-based EGL display probing.");
				}
			}
		}
	}
	CheckEGLError(_log.get());

	// If device enumeration isn't supported or failed, fall back to probing the default display.
	if (displayList.empty())
	{
		EGLDisplay display = EGL_NO_DISPLAY;
		if (((flags & EnumerationFlags::HeadlessRendering) != EnumerationFlags::None) && supportsSurfacelessMesaPlatform)
		{
			display = eglGetPlatformDisplay(EGL_PLATFORM_SURFACELESS_MESA, EGL_DEFAULT_DISPLAY, nullptr);
			if (display == EGL_NO_DISPLAY)
			{
				CheckEGLError(_log.get());
				_log->Warning("eglGetPlatformDisplay(EGL_PLATFORM_SURFACELESS_MESA) failed. Falling back to EGL_DEFAULT_DISPLAY.");
			}
		}
		if (display == EGL_NO_DISPLAY)
		{
			display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
		}
		if (display == EGL_NO_DISPLAY)
		{
			CheckEGLError(_log.get());
			_log->Warning("eglGetDisplay failed. No compatible OpenGL devices will be enumerated.");
			_readDeviceInfo = true;
			return true;
		}
		displayList.push_back({display, nullptr});
	}
	CheckEGLError(_log.get());

	// Probe each display and build a list of devices
	for (size_t displayIndex = 0; displayIndex < displayList.size(); ++displayIndex)
	{
		// Initialize the EGL display object
		EGLDisplay display = displayList[displayIndex].display;
		if (eglInitialize(display, nullptr, nullptr) != EGL_TRUE)
		{
			CheckEGLError(_log.get());
			_log->Warning("eglInitialize failed (display index: {0})", displayIndex);
			eglTerminate(display);
			continue;
		}

		// Select the OpenGL API for following EGL calls
		if (eglBindAPI(EGL_OPENGL_API) != EGL_TRUE)
		{
			CheckEGLError(_log.get());
			_log->Warning("eglBindAPI failed (display index: {0})", displayIndex);
			eglTerminate(display);
			continue;
		}

		// Select a pixel format
		OpenGLRenderer::PixelFormatInfo pixelFormatInfo = {};
		if (!OpenGLRenderer::SelectWindowPixelFormat(display, IFrameBuffer::WindowColorSpaceMode::Default, IFrameBuffer::WindowDepthStencilMode::None, pixelFormatInfo, true, *_log, false))
		{
			CheckEGLError(_log.get());
			_log->Warning("Failed to find a compatible pixel format (display index: {0})", displayIndex);
			eglTerminate(display);
			continue;
		}

		// Build the set of attributes to control the OpenGL rendering context creation
		constexpr int targetVersionMajor = OPENGL_VERSION_MAJOR;
		constexpr int targetVersionMinor = OPENGL_VERSION_MINOR;
		const int contextFlags = (((flags & EnumerationFlags::NativeApiValidation) == EnumerationFlags::NativeApiValidation) ? EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR : 0);
		EGLint contextAttribs[] = {EGL_CONTEXT_MAJOR_VERSION_KHR, targetVersionMajor, EGL_CONTEXT_MINOR_VERSION_KHR, targetVersionMinor, EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR, EGL_CONTEXT_FLAGS_KHR, contextFlags, EGL_NONE};

		// Attempt to create our new rendering context
		EGLContext context = eglCreateContext(display, pixelFormatInfo.fbConfig, EGL_NO_CONTEXT, &contextAttribs[0]);
		if (context == EGL_NO_CONTEXT)
		{
			CheckEGLError(_log.get());
			_log->Warning("eglCreateContext failed (display index: {0}). This device may not support OpenGL {1}.{2} Core.", displayIndex, OPENGL_VERSION_MAJOR, OPENGL_VERSION_MINOR);
			eglTerminate(display);
			continue;
		}

		// Attempt to activate our rendering context. We assume EGL 1.5 support here, and we're relying on the previous
		// "EGL_KHR_surfaceless_context" extension, now core, which allows us to create a rendering context with no
		// bound surface. Traditional alternartive here would be to call eglCreatePbufferSurface to create a 1x1 pbuffer
		// and bind to that instead of passing in EGL_NO_SURFACE here.
		if (eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, context) != EGL_TRUE)
		{
			CheckEGLError(_log.get());
			_log->Warning("Failed to activate OpenGL rendering context (display index: {0})", displayIndex);
			eglDestroyContext(display, context);
			eglTerminate(display);
			continue;
		}
		CheckEGLError(_log.get());

		// Initialize glad
		if (gladLoadGLLoader(reinterpret_cast<GLADloadproc>(eglGetProcAddress)) == 0)
		{
			CheckEGLError(_log.get());
			_log->Warning("Failed to initialize GLAD (display index: {0})", displayIndex);
			eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
			eglDestroyContext(display, context);
			eglTerminate(display);
			continue;
		}

		// If we're doing headless rendering, retrieve the render node device path.
		std::string renderNodePath;
		if ((flags & EnumerationFlags::HeadlessRendering) != EnumerationFlags::None)
		{
			if (displayList[displayIndex].device != nullptr)
			{
				auto renderNodeFilePathRaw = eglQueryDeviceStringEXT(displayList[displayIndex].device, EGL_DRM_RENDER_NODE_FILE_EXT);
				auto deviceFilePathRaw = eglQueryDeviceStringEXT(displayList[displayIndex].device, EGL_DRM_DEVICE_FILE_EXT);
				CheckEGLError(_log.get());
				if (renderNodeFilePathRaw != nullptr)
				{
					renderNodePath = renderNodeFilePathRaw;
				}
				else if (deviceFilePathRaw != nullptr)
				{
					renderNodePath = deviceFilePathRaw;
				}
			}
			if (renderNodePath.empty())
			{
				_log->Info("Failed to locate render node path. Headless rendering will use the renderer's surfaceless/default display fallback. (display index: {0})", displayIndex);
			}
		}

		// Create a graphics device object, and attempt to load device information into it. Note that we need to use the
		// index of the device in the set of valid devices here, as we've seen that non-functioning devices which fail
		// on eglInitialize will not be counted in the DRI_PRIME index, meaning deviceIndex here may not correspond to
		// the correct index value in DRI_PRIME.
		int deviceIndex = (int)_devices.size();
		auto device = std::make_unique<OpenGLGraphicsDevice>(_log.get(), false, deviceIndex, renderNodePath);
		if (!device->ReadDeviceInfo())
		{
			CheckEGLError(_log.get());
			_log->Warning("Failed to read device info (display index: {0})", displayIndex);
		}
		else
		{
			_devices.push_back(std::move(device));
		}
		CheckEGLError(_log.get());

		// Release our allocated resources
		eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		eglDestroyContext(display, context);
		CheckEGLError(_log.get());
		eglTerminate(display);
	}
	CheckEGLError(_log.get());

	// Return true to the caller
	_readDeviceInfo = true;
	return _readDeviceInfo;
}

#elif defined(OPENGL_USE_PLATFORM_XLIB)
//----------------------------------------------------------------------------------------
bool OpenGLGraphicsDeviceEnumerator::ReadOpenGLGraphicsDeviceInfo(EnumerationFlags flags)
{
	// Since we don't support headless mode on XLib, abort any further processing.
	if ((flags & EnumerationFlags::HeadlessRendering) != EnumerationFlags::None)
	{
		_log->Warning("Headless rendering is only supported by the OpenGL renderer when built with EGL support.");
		_readDeviceInfo = true;
		return true;
	}

	// Search for each available device on the system
	bool foundLastDevice = false;
	for (size_t deviceIndex = 0; !foundLastDevice; ++deviceIndex)
	{
		// Set the target device number using the MESA environment variable
		setenv("DRI_PRIME", std::to_string(deviceIndex).c_str(), 1);

		// Open a display
		auto* display = XOpenDisplay(nullptr);
		if (display == nullptr)
		{
			_log->Warning("Failed to open X display for OpenGL graphics device enumeration. No compatible OpenGL devices will be enumerated.");
			break;
		}

		// Select a pixel format
		OpenGLRenderer::PixelFormatInfo pixelFormatInfo = {};
		if (!OpenGLRenderer::SelectWindowPixelFormat(display, IFrameBuffer::WindowColorSpaceMode::Default, IFrameBuffer::WindowDepthStencilMode::None, pixelFormatInfo, *_log, false))
		{
			_log->Error("Failed to select GLX framebuffer configuration for dummy OpenGL context.");
			XCloseDisplay(display);
			continue;
		}

		// Create a dummy window. We only do this to allow us to create a rendering context.
		const int screen = DefaultScreen(display);
		::Window root = RootWindow(display, screen);
		Colormap colorMap = XCreateColormap(display, root, pixelFormatInfo.visualInfo.visual, X11Constants::kAllocNone);
		XSetWindowAttributes windowAttributes = {};
		windowAttributes.colormap = colorMap;
		::Window renderWindow = XCreateWindow(display, root, 0, 0, 1, 1, 0, pixelFormatInfo.visualInfo.depth, X11Constants::kInputOutput, pixelFormatInfo.visualInfo.visual, X11Constants::kCWBorderPixel | X11Constants::kCWColormap | X11Constants::kCWEventMask, &windowAttributes);

		// Attempt to create our new rendering context
		GLXContext renderingContext = nullptr;
		auto glXCreateContextAttribsARB = reinterpret_cast<PFNGLXCREATECONTEXTATTRIBSARBPROC>(glXGetProcAddressARB(reinterpret_cast<const GLubyte*>("glXCreateContextAttribsARB")));
		if (glXCreateContextAttribsARB != nullptr)
		{
			constexpr int targetVersionMajor = OPENGL_VERSION_MAJOR;
			constexpr int targetVersionMinor = OPENGL_VERSION_MINOR;
			const int contextFlags = (((flags & EnumerationFlags::NativeApiValidation) == EnumerationFlags::NativeApiValidation) ? GLX_CONTEXT_DEBUG_BIT_ARB : 0);
			int contextAttribs[] = {GLX_CONTEXT_MAJOR_VERSION_ARB, targetVersionMajor, GLX_CONTEXT_MINOR_VERSION_ARB, targetVersionMinor, GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB, GLX_CONTEXT_FLAGS_ARB, contextFlags, 0};
			renderingContext = glXCreateContextAttribsARB(display, pixelFormatInfo.fbConfig, nullptr, X11Constants::kTrue, &contextAttribs[0]);
			if (renderingContext == nullptr)
			{
				_log->Warning("glXCreateContextAttribsARB failed. Falling back to glXCreateNewContext.");
			}
		}
		if (renderingContext == nullptr)
		{
			renderingContext = glXCreateNewContext(display, pixelFormatInfo.fbConfig, GLX_RGBA_TYPE, nullptr, X11Constants::kTrue);
			if (renderingContext == nullptr)
			{
				_log->Error("Failed to create GLX context for OpenGL graphics device enumeration.");
				XDestroyWindow(display, renderWindow);
				XFreeColormap(display, colorMap);
				XCloseDisplay(display);
				continue;
			}
		}

		// Attempt to activate our rendering context
		if (glXMakeCurrent(display, renderWindow, renderingContext) != X11Constants::kTrue)
		{
			_log->Error("glXMakeCurrent failed for OpenGL graphics device enumeration.");
			glXDestroyContext(display, renderingContext);
			XDestroyWindow(display, renderWindow);
			XFreeColormap(display, colorMap);
			XCloseDisplay(display);
			continue;
		}

		// Initialize glad
		if (gladLoadGL() == 0)
		{
			_log->Error("Failed to initialize GLAD");
			glXMakeCurrent(display, X11Constants::kNone, nullptr);
			glXDestroyContext(display, renderingContext);
			XDestroyWindow(display, renderWindow);
			XFreeColormap(display, colorMap);
			XCloseDisplay(display);
			continue;
		}

		// If we failed to locate the glXCreateContextAttribsARB method, it's entirely possible we've been supplied an
		// OpenGL implementation which is too old to meet our minimum requirements, as we weren't able to specify the
		// target version. As an imperfect yet effective test for this case, we attempt to retrieve the value of
		// "GL_MAX_UNIFORM_BUFFER_BINDINGS", which is only supported on OpenGL 3.1 or above. If this call fails with
		// GL_INVALID_ENUM, we know we're on an earlier version and can't continue.
		if (glXCreateContextAttribsARB == nullptr)
		{
			GLenum error = glGetError();
			int dummyRead = 0;
			glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &dummyRead);
			error = glGetError();
			if (error == GL_INVALID_ENUM)
			{
				_log->Warning("Unable to use OpenGL graphics device. The graphics device doesn't appear to meet the minimum OpenGL version requirements.");
				glXMakeCurrent(display, X11Constants::kNone, nullptr);
				glXDestroyContext(display, renderingContext);
				XDestroyWindow(display, renderWindow);
				XFreeColormap(display, colorMap);
				XCloseDisplay(display);
				continue;
			}
		}

		// Create a graphics device object, and attempt to load device information into it.
		auto device = std::make_unique<OpenGLGraphicsDevice>(_log.get(), false, deviceIndex, "");
		if (!device->ReadDeviceInfo())
		{
			_log->Error("Failed to read device info");
			glXMakeCurrent(display, X11Constants::kNone, nullptr);
			glXDestroyContext(display, renderingContext);
			XDestroyWindow(display, renderWindow);
			XFreeColormap(display, colorMap);
			XCloseDisplay(display);
			continue;
		}

		// If this device doesn't match an existing device, add it to the list of devices, otherwise flag the device
		// search is now over.
		for (const auto& existingDevice : _devices)
		{
			if ((existingDevice->GetDeviceName().Get() == device->GetDeviceName().Get()) && (existingDevice->GetVendorName().Get() == device->GetVendorName().Get()))
			{
				foundLastDevice = true;
				break;
			}
		}
		if (!foundLastDevice)
		{
			_devices.push_back(std::move(device));
		}

		// Release our allocated resources
		glXMakeCurrent(display, X11Constants::kNone, nullptr);
		glXDestroyContext(display, renderingContext);
		XDestroyWindow(display, renderWindow);
		XFreeColormap(display, colorMap);
		XCloseDisplay(display);

		// Remove the device selection environment variable
		unsetenv("DRI_PRIME");
	}

	// Flag that we've read the device info, and return true to the caller.
	_readDeviceInfo = true;
	return true;
}
#endif

#ifdef OPENGL_USE_PLATFORM_APPKIT
//----------------------------------------------------------------------------------------
bool OpenGLGraphicsDeviceEnumerator::ReadOpenGLGraphicsDeviceInfo(EnumerationFlags flags)
{
	// Select a pixel format
	OpenGLRenderer::PixelFormatInfo pixelFormatInfo = {};
	if (!OpenGLRenderer::SelectPixelFormat(IFrameBuffer::WindowColorSpaceMode::Default, IFrameBuffer::WindowDepthStencilMode::None, pixelFormatInfo, *_log, false))
	{
		_log->Warning("Failed to find a compatible pixel format. No compatible OpenGL device will be enumerated.");
		_readDeviceInfo = true;
		return true;
	}

	// Initialize an OpenGL rendering context
	CGLContextObj renderingContext = nullptr;
	auto cglCreateContextReturn = CGLCreateContext(pixelFormatInfo.pixelFormatObj, nullptr, &renderingContext);
	CGLDestroyPixelFormat(pixelFormatInfo.pixelFormatObj);
	if (cglCreateContextReturn != kCGLNoError)
	{
		_log->Error("CGLCreateContext failed with error code: {0}", cglCreateContextReturn);
		return false;
	}

	// Attempt to activate our rendering context
	auto cglSetCurrentContextReturn = CGLSetCurrentContext(renderingContext);
	if (cglSetCurrentContextReturn != kCGLNoError)
	{
		_log->Error("Failed to activate OpenGL rendering context with error code: {0}", cglSetCurrentContextReturn);
		CGLReleaseContext(renderingContext);
		return false;
	}

	// Initialize glad
	if (gladLoadGL() == 0)
	{
		_log->Error("Failed to initialize GLAD");
		CGLSetCurrentContext(nullptr);
		CGLReleaseContext(renderingContext);
		return false;
	}

	// We can only request an OpenGL 3.2 Core profile on macOS, not a 3.3 core profile, but it's a "forwards compatible"
	// request, so we may have got higher than 3.2 returned. We query the actual OpenGL version provided here to ensure
	// that at least 3.3 support is provided.
	GLint openGLVersionMajor = 0;
	GLint openGLVersionMinor = 0;
	glGetIntegerv(GL_MAJOR_VERSION, &openGLVersionMajor);
	glGetIntegerv(GL_MINOR_VERSION, &openGLVersionMinor);
	if ((openGLVersionMajor <= 3) && (openGLVersionMinor <= 2))
	{
		_log->Warning("Requested OpenGL 3.3 Core, but only version {0}.{1} is available. No compatible OpenGL device will be enumerated.", openGLVersionMajor, openGLVersionMinor);
		CGLSetCurrentContext(nullptr);
		CGLReleaseContext(renderingContext);
		_readDeviceInfo = true;
		return true;
	}

	// Create a graphics device object, and attempt to load device information into it.
	auto device = std::make_unique<OpenGLGraphicsDevice>(_log.get(), false);
	if (!device->ReadDeviceInfo())
	{
		_log->Error("Failed to read OpenGL device info");
		CGLSetCurrentContext(nullptr);
		CGLReleaseContext(renderingContext);
		return false;
	}

	// Store the graphics device for later use
	_devices.push_back(std::move(device));

	// Release our allocated resources
	CGLSetCurrentContext(nullptr);
	CGLReleaseContext(renderingContext);

	// Flag that we've successfully read device info, and return true to the caller.
	_readDeviceInfo = true;
	return true;
}
#endif

//----------------------------------------------------------------------------------------
bool OpenGLGraphicsDeviceEnumerator::FoundDevice() const
{
	return !_devices.empty();
}

//----------------------------------------------------------------------------------------
Marshal::Ret<std::vector<IGraphicsDevice*>> OpenGLGraphicsDeviceEnumerator::GetAllDevices() const
{
	std::vector<IGraphicsDevice*> deviceSet;
	deviceSet.reserve(_devices.size());
	for (const auto& device : _devices)
	{
		deviceSet.push_back(device.get());
	}
	return deviceSet;
}

//----------------------------------------------------------------------------------------
Marshal::Ret<std::vector<IGraphicsDevice*>> OpenGLGraphicsDeviceEnumerator::GetFilteredDevices() const
{
	return _filteredDevices;
}

//----------------------------------------------------------------------------------------
IGraphicsDevice* OpenGLGraphicsDeviceEnumerator::GetPreferredDevice() const
{
	// Find the best devices of each type, based on their reported dedicated memory availability.
	IGraphicsDevice* bestDedicated = nullptr;
	IGraphicsDevice* bestIntegrated = nullptr;
	IGraphicsDevice* bestSoftware = nullptr;
	for (IGraphicsDevice* graphicsDevice : _filteredDevices)
	{
		auto deviceType = graphicsDevice->GetDeviceType();
		if (deviceType == IGraphicsDevice::DeviceType::Discrete)
		{
			if ((bestDedicated == nullptr) || (bestDedicated->GetMemorySizeInBytes(IGraphicsDevice::MemoryType::Dedicated) < graphicsDevice->GetMemorySizeInBytes(IGraphicsDevice::MemoryType::Dedicated)))
			{
				bestDedicated = graphicsDevice;
			}
		}
		else if (deviceType == IGraphicsDevice::DeviceType::Integrated)
		{
			if ((bestIntegrated == nullptr) || (bestIntegrated->GetMemorySizeInBytes(IGraphicsDevice::MemoryType::Dedicated) < graphicsDevice->GetMemorySizeInBytes(IGraphicsDevice::MemoryType::Dedicated)))
			{
				bestIntegrated = graphicsDevice;
			}
		}
		else
		{
			if ((bestSoftware == nullptr) || (bestSoftware->GetMemorySizeInBytes(IGraphicsDevice::MemoryType::Dedicated) < graphicsDevice->GetMemorySizeInBytes(IGraphicsDevice::MemoryType::Dedicated)))
			{
				bestSoftware = graphicsDevice;
			}
		}
	}

	// Prefer dedicated, then integrated, and finally software.
	if (bestDedicated != nullptr)
	{
		return bestDedicated;
	}
	if (bestIntegrated != nullptr)
	{
		return bestIntegrated;
	}
	return bestSoftware;
}

//----------------------------------------------------------------------------------------
// Filtering methods
//----------------------------------------------------------------------------------------
void OpenGLGraphicsDeviceEnumerator::FilterDevice(IGraphicsDevice* targetDevice)
{
	// Iterate all remaining filtered devices, and remove any devices that don't match the filter requirement.
	auto deviceIterator = _filteredDevices.begin();
	while (deviceIterator != _filteredDevices.end())
	{
		// Determine if we should filter this device
		auto* device = *deviceIterator;
		bool filterOutDevice = (device == targetDevice);

		// Erase or retain this device in the filtered device list as required
		if (filterOutDevice)
		{
			deviceIterator = _filteredDevices.erase(deviceIterator);
		}
		else
		{
			++deviceIterator;
		}
	}
}

//----------------------------------------------------------------------------------------
void OpenGLGraphicsDeviceEnumerator::FilterDevicesOfType(IGraphicsDevice::DeviceType type)
{
	// Iterate all remaining filtered devices, and remove any devices that don't match the filter requirement.
	auto deviceIterator = _filteredDevices.begin();
	while (deviceIterator != _filteredDevices.end())
	{
		// Determine if we should filter this device
		auto* device = *deviceIterator;
		bool filterOutDevice = (device->GetDeviceType() == type);

		// Erase or retain this device in the filtered device list as required
		if (filterOutDevice)
		{
			deviceIterator = _filteredDevices.erase(deviceIterator);
		}
		else
		{
			++deviceIterator;
		}
	}
}

//----------------------------------------------------------------------------------------
void OpenGLGraphicsDeviceEnumerator::FilterDevicesNotOfType(IGraphicsDevice::DeviceType type)
{
	// Iterate all remaining filtered devices, and remove any devices that don't match the filter requirement.
	auto deviceIterator = _filteredDevices.begin();
	while (deviceIterator != _filteredDevices.end())
	{
		// Determine if we should filter this device
		auto* device = *deviceIterator;
		bool filterOutDevice = (device->GetDeviceType() != type);

		// Erase or retain this device in the filtered device list as required
		if (filterOutDevice)
		{
			deviceIterator = _filteredDevices.erase(deviceIterator);
		}
		else
		{
			++deviceIterator;
		}
	}
}

//----------------------------------------------------------------------------------------
void OpenGLGraphicsDeviceEnumerator::FilterDevicesWithoutFeature(IGraphicsDevice::Feature feature)
{
	// Iterate all remaining filtered devices, and remove any devices that don't match the filter requirement.
	auto deviceIterator = _filteredDevices.begin();
	while (deviceIterator != _filteredDevices.end())
	{
		// Determine if we should filter this device
		auto* device = *deviceIterator;
		bool filterOutDevice = !device->IsFeatureSupported(feature);

		// Erase or retain this device in the filtered device list as required
		if (filterOutDevice)
		{
			deviceIterator = _filteredDevices.erase(deviceIterator);
		}
		else
		{
			++deviceIterator;
		}
	}
}

//----------------------------------------------------------------------------------------
void OpenGLGraphicsDeviceEnumerator::FilterDevicesWithoutAllFeatures(const Marshal::In<std::set<IGraphicsDevice::Feature>>& featureSet)
{
	// Iterate all remaining filtered devices, and remove any devices that don't match the filter requirement.
	auto deviceIterator = _filteredDevices.begin();
	while (deviceIterator != _filteredDevices.end())
	{
		// Determine if we should filter this device
		auto* device = *deviceIterator;
		bool filterOutDevice = !device->AreAllFeaturesSupported(featureSet);

		// Erase or retain this device in the filtered device list as required
		if (filterOutDevice)
		{
			deviceIterator = _filteredDevices.erase(deviceIterator);
		}
		else
		{
			++deviceIterator;
		}
	}
}

//----------------------------------------------------------------------------------------
void OpenGLGraphicsDeviceEnumerator::ClearDeviceFilters()
{
	_filteredDevices.clear();
	for (const auto& device : _devices)
	{
		_filteredDevices.push_back(device.get());
	}
}

} // namespace cobalt::graphics
