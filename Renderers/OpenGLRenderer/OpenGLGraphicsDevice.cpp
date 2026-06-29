// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "OpenGLGraphicsDevice.h"
#include "OpenGLDebug.h"
#include "OpenGLHeaders.h"
#include "OpenGLRenderer.h"
#include <Internal/RendererSupport/UnicodeConversion.h>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <limits>
#include <utility>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
#ifdef __linux__
OpenGLGraphicsDevice::OpenGLGraphicsDevice(cobalt::logging::ILogger* log, bool usingSoftwareRenderer, int deviceIndex, const std::string& renderNodePath)
: _log(log), _usingSoftwareRenderer(usingSoftwareRenderer), _deviceIndex(deviceIndex), _renderNodePath(renderNodePath)
#else
OpenGLGraphicsDevice::OpenGLGraphicsDevice(cobalt::logging::ILogger* log, bool usingSoftwareRenderer)
: _log(log), _usingSoftwareRenderer(usingSoftwareRenderer)
#endif
{
	// Add our available features to the supported feature set
	_supportedFeatureSet.insert(Feature::GeometryShaders);
#ifdef GL_VERSION_4_6
	_supportedFeatureSet.insert(Feature::AnisotropicFiltering);
#elif defined(GL_EXT_texture_filter_anisotropic)
	if (GLAD_GL_EXT_texture_filter_anisotropic != 0)
	{
		_supportedFeatureSet.insert(Feature::AnisotropicFiltering);
	}
#endif
#ifdef GL_VERSION_4_3
	_supportedFeatureSet.insert(Feature::ComputeShaders);
#endif
#ifdef GL_EXT_polygon_offset_clamp
	if (GLAD_GL_EXT_polygon_offset_clamp != 0)
	{
		_supportedFeatureSet.insert(Feature::DepthBiasClamp);
	}
#endif
#ifdef GL_VERSION_4_3
	_supportedFeatureSet.insert(Feature::IndirectDraw);
#endif
#if defined(GL_VERSION_4_3) & defined(GL_ARB_indirect_parameters)
	if (GLAD_GL_ARB_indirect_parameters != 0)
	{
		_supportedFeatureSet.insert(Feature::IndirectMultiDrawNative);
	}
#endif
#ifdef GL_VERSION_4_2
	_supportedFeatureSet.insert(Feature::InstanceOffset);
#endif
	_supportedFeatureSet.insert(Feature::MipmapLevelBias);
	_supportedFeatureSet.insert(Feature::PolygonWireframeFillMode);
#ifdef GL_VERSION_4_3
	_supportedFeatureSet.insert(Feature::ResourceArrays);
#endif
#ifdef GL_VERSION_4_3
	// This is supposed to be a core feature in OpenGL 4.3, but due to Intel driver bugs on Windows, we can't guarantee
	// this yet. We do a feature check later on when we do device info to see if support is blacklisted.
	//_supportedFeatureSet.insert(Feature::ShaderArraysOfArrays);
#elif defined(GLAD_GL_ARB_arrays_of_arrays)
	if (GLAD_GL_ARB_arrays_of_arrays != 0)
	{
		_supportedFeatureSet.insert(Feature::ShaderArraysOfArrays);
	}
#endif
#ifdef GL_VERSION_4_0
	_supportedFeatureSet.insert(Feature::SeparateBlendModePerTarget);
#elif defined(GL_ARB_draw_buffers_blend)
	if (GLAD_GL_ARB_draw_buffers_blend != 0)
	{
		_supportedFeatureSet.insert(Feature::SeparateBlendModePerTarget);
	}
#endif
#ifdef GL_VERSION_4_0
	_supportedFeatureSet.insert(Feature::TextureCubeArray);
#endif
}

//----------------------------------------------------------------------------------------
// Info methods
//----------------------------------------------------------------------------------------
bool OpenGLGraphicsDevice::ReadDeviceInfo()
{
	// Do an initial check for any OpenGL errors
	CheckGLError(_log);

	// Extract the vendor and device strings
	const auto* vendorStringRaw = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
	const auto* rendererStringRaw = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
	const auto* versionStringRaw = reinterpret_cast<const char*>(glGetString(GL_VERSION));
	std::string vendorNameAsString = (vendorStringRaw != nullptr ? vendorStringRaw : "Unknown");
	std::string deviceNameAsString = (rendererStringRaw != nullptr ? rendererStringRaw : "Unknown");
	std::string versionStringAsString = (versionStringRaw != nullptr ? versionStringRaw : "Unknown");
	_vendorName.assign(vendorNameAsString.data(), vendorNameAsString.data() + vendorNameAsString.size());
	_deviceName.assign(deviceNameAsString.data(), deviceNameAsString.data() + deviceNameAsString.size());
	_versionString.assign(versionStringAsString.data(), versionStringAsString.data() + versionStringAsString.size());

	// If this appears to be a software renderer running under Mesa on Linux, flag the device as a software renderer.
	//##FIX## It's inappropriate to do this check in this function
	auto stringContains = [](const std::string& test, const std::string& searchString, bool caseInsensitive = false) {
		if (caseInsensitive)
		{
			return (std::search(test.begin(), test.end(), searchString.begin(), searchString.end(), [](char left, char right) { return std::toupper(left) == std::toupper(right); }) != test.end());
		}
		return (std::search(test.begin(), test.end(), searchString.begin(), searchString.end()) != test.end());
	};
	if (stringContains(vendorNameAsString, "Mesa", true) && (stringContains(deviceNameAsString, "llvmpipe", true) || stringContains(deviceNameAsString, "softpipe", true)))
	{
		_usingSoftwareRenderer = true;
	}

	// Fetch device memory information
	auto vendor = GetVendor();
	FetchMemoryInfoInternal(vendor);

	// Fetch device limits
	FetchImageLimitsInternal();
	FetchShaderLimitsInternal();
	FetchDrawLimitsInternal();
	FetchFrameBufferLimitsInternal();
	FetchDataBufferLimitsInternal();

	// Arrays of arrays in shaders is supposed to be a core feature in OpenGL 4.3, but due to Intel driver bugs on
	// Windows, we have to defer detection. We've confirmed broken support for this feature in at least Intel HD
	// Graphics 620 and Intel UHD Graphics 630 devices, and likely more. We block the entire "Gen 9.5" family of
	// devices, not advertising support for this feature there due to the driver problems. This is still current as
	// of 2026-05-20, with it likely to never be fixed. Newer devices appear to work without issue. The symptom is that
	// shaders which use this feature load without complaint, but arrays of arrays are completely removed and invisible,
	// not being reported in any way via shader reflection, with their contents acting as though they're set to a
	// constant value of 0. Note that we have also seen this issue on an Intel Iris Pro Graphics 5200 device from the
	// Gen 7.5 era, meaning all older devices are probably affected.
#ifdef GL_VERSION_4_3
	if (vendor == Vendor::Intel)
	{
		// Check if the current device is blocked from advertising this feature
		bool deviceBlocked = false;
		std::vector<std::string> blockList;
		// Gen 9.5
		blockList.emplace_back("HD Graphics 615");
		blockList.emplace_back("HD Graphics 620");
		blockList.emplace_back("HD Graphics 630");
		blockList.emplace_back("UHD Graphics 617");
		blockList.emplace_back("UHD Graphics 620");
		blockList.emplace_back("UHD Graphics 630");
		blockList.emplace_back("Plus Graphics 640"); // Iris
		blockList.emplace_back("Plus Graphics 645"); // Iris
		blockList.emplace_back("Plus Graphics 650"); // Iris
		blockList.emplace_back("Plus Graphics 655"); // Iris
		blockList.emplace_back("HD Graphics P630");
		blockList.emplace_back("HD Graphics 600");
		blockList.emplace_back("HD Graphics 605");
		// Gen 8.5
		blockList.emplace_back("HD Graphics 510");
		blockList.emplace_back("HD Graphics 515");
		blockList.emplace_back("HD Graphics 520");
		blockList.emplace_back("HD Graphics 530");
		blockList.emplace_back("Graphics 540");     // Iris
		blockList.emplace_back("Graphics 550");     // Iris
		blockList.emplace_back("Pro Graphics 580"); // Iris
		blockList.emplace_back("HD Graphics P530");
		blockList.emplace_back("Pro Graphics P580"); // Iris
		// Gen 7.5
		blockList.emplace_back("Pro Graphics 5200"); // Iris
		blockList.emplace_back("Graphics 5100");     // Iris
		blockList.emplace_back("HD Graphics 5000");
		blockList.emplace_back("HD Graphics 4600");
		blockList.emplace_back("HD Graphics 4400");
		blockList.emplace_back("HD Graphics 4200");
		blockList.emplace_back("HD Graphics P4600");
		blockList.emplace_back("HD Graphics P4700");
		for (const auto& blockListEntry : blockList)
		{
			if (_deviceName.find(blockListEntry) != std::string::npos)
			{
				deviceBlocked = true;
				break;
			}
		}

		// If this device isn't blocked from advertising the feature due to driver bugs, add it to the supported
		// features list.
		if (!deviceBlocked)
		{
			_supportedFeatureSet.insert(Feature::ShaderArraysOfArrays);
		}
	}
	else
	{
		// Since this isn't an Intel graphics device and we're on OpenGL 4.3, which has this feature as core, advertise
		// the feature as available.
		_supportedFeatureSet.insert(Feature::ShaderArraysOfArrays);
	}
#endif

	// Do a final check for any OpenGL errors, and return true to the caller.
	CheckGLError(_log);
	return true;
}

//----------------------------------------------------------------------------------------
OpenGLGraphicsDevice::DeviceType OpenGLGraphicsDevice::GetDeviceType() const
{
	// If this is flagged as a software device, report it as such to the caller.
	if (_usingSoftwareRenderer)
	{
		return DeviceType::Software;
	}

	// If the adapter appears to be from Intel or Apple, consider it to be an integrated graphics device. This appears
	// to be the best we can do on OpenGL. Modern graphics APIs provide better information about the device, which
	// allows us to give more reliable results.
	auto vendor = GetVendor();
	bool isIntegratedDevice = (vendor == Vendor::Intel) || (vendor == Vendor::Apple);
	if (isIntegratedDevice)
	{
		return DeviceType::Integrated;
	}

	// Assume a discrete device otherwise
	return DeviceType::Discrete;
}

//----------------------------------------------------------------------------------------
IGraphicsDevice::Vendor OpenGLGraphicsDevice::GetVendor() const
{
	// Under OpenGL, we can only get the vendor name as a string. We therefore use the dubious approach here of trying
	// to perform string matching to determine the vendor. Modern graphics APIs have reliable ways of determining this
	// information that doesn't rely on string matching. Since OpenGL is considered a legacy renderer, we do our best to
	// match the functionality here, rather than limiting the graphics API for newer renderers to string results only.
	auto stringContains = [](const std::string& test, const std::string& searchString, bool caseInsensitive = false) {
		if (caseInsensitive)
		{
			return (std::search(test.begin(), test.end(), searchString.begin(), searchString.end(), [](char left, char right) { return std::toupper(left) == std::toupper(right); }) != test.end());
		}
		return (std::search(test.begin(), test.end(), searchString.begin(), searchString.end()) != test.end());
	};

	// Perform string matching to try and determine the vendor. Note that since we have to resort to string searches
	// here, we've ordered the searches to try and avoid bad matches.
	if (stringContains(_vendorName, "ImgTec", true) || stringContains(_deviceName, "ImgTec", true))
	{
		return Vendor::ImgTec;
	}
	if (stringContains(_vendorName, "Nvidia", true) || stringContains(_deviceName, "Nvidia", true))
	{
		return Vendor::Nvidia;
	}
	if (stringContains(_vendorName, "Qualcomm", true) || stringContains(_deviceName, "Qualcomm", true))
	{
		return Vendor::Qualcomm;
	}
	if (stringContains(_vendorName, "Intel", true) || stringContains(_deviceName, "Intel", true))
	{
		return Vendor::Intel;
	}
	if (stringContains(_vendorName, "Apple", true) || stringContains(_deviceName, "Apple", true))
	{
		return Vendor::Apple;
	}
	if (stringContains(_vendorName, "Vivante", true) || stringContains(_deviceName, "Vivante", true))
	{
		return Vendor::Vivante;
	}
	if (stringContains(_vendorName, "VeriSilicon", true) || stringContains(_deviceName, "VeriSilicon", true))
	{
		return Vendor::VeriSilicon;
	}
	if (stringContains(_vendorName, "ARM", true) || stringContains(_deviceName, "ARM", true))
	{
		return Vendor::ARM;
	}
	if (stringContains(_vendorName, "AMD", true) || stringContains(_deviceName, "AMD", true) || stringContains(_vendorName, "ATI", false) || stringContains(_deviceName, "ATI", false))
	{
		// We need this down the bottom of the list, as it's more likely to get partial matches with other results.
		// "ATI" for example if we do a case insensitive comparison matches on "Generation", which was capturing some
		// NVidia devices.
		return Vendor::AMD;
	}
	if (stringContains(_vendorName, "Mesa", true) || stringContains(_deviceName, "Mesa", true))
	{
		return Vendor::Mesa;
	}
	if (stringContains(_vendorName, "Microsoft", true) || stringContains(_deviceName, "Microsoft", true))
	{
		// Note that we put Microsoft last to handle WSLg cases, where OpenGL support is enabled via a redirection
		// driver that passes through OpenGL calls to Direct3D12 using the native graphics driver. In this case, the
		// driver string can contain both "Microsoft" and the underlying vendor name.
		return Vendor::Microsoft;
	}
	return Vendor::Unknown;
}

//----------------------------------------------------------------------------------------
Marshal::Ret<std::string> OpenGLGraphicsDevice::GetVendorName() const
{
	return _vendorName;
}

//----------------------------------------------------------------------------------------
Marshal::Ret<std::string> OpenGLGraphicsDevice::GetDeviceName() const
{
	return _deviceName;
}

//----------------------------------------------------------------------------------------
Marshal::Ret<std::string> OpenGLGraphicsDevice::GetDriverInfo() const
{
	// As per the OpenGL spec, there's a leading OpenGL version number here, with a space separating it and the
	// following vendor information. We strip off the OpenGL version number here and just return the vendor component.
	const char* whiteSpaceDelimiters = " \t\n\r";
	auto whiteSpacePos = _versionString.find_first_of(whiteSpaceDelimiters);
	return (whiteSpacePos == std::string::npos) ? _versionString : _versionString.substr(_versionString.find_first_not_of(whiteSpaceDelimiters, whiteSpacePos));
}

//----------------------------------------------------------------------------------------
size_t OpenGLGraphicsDevice::GetMemorySizeInBytes(MemoryType memoryType) const
{
	if (memoryType == IGraphicsDevice::MemoryType::Dedicated)
	{
		return _dedicatedMemorySizeInBytes;
	}
	return _sharedMemorySizeInBytes;
}

//----------------------------------------------------------------------------------------
// Memory methods
//----------------------------------------------------------------------------------------
void OpenGLGraphicsDevice::FetchMemoryInfoInternal(Vendor vendor)
{
	// Query the memory info for NVidia devices, as per the following extension:
	// https://www.khronos.org/registry/OpenGL/extensions/NVX/NVX_gpu_memory_info.txt
	size_t dedicatedMemorySizeInBytes = 0;
	size_t sharedMemorySizeInBytes = 0;
	GLint dedicatedVideoMemoryTotalInKilobytes = 0;
#ifdef GL_NVX_gpu_memory_info
	if ((GLAD_GL_NVX_gpu_memory_info != 0) && (dedicatedVideoMemoryTotalInKilobytes == 0) && (vendor == Vendor::Nvidia))
	{
		glGetIntegerv(GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX, &dedicatedVideoMemoryTotalInKilobytes);
	}
#endif

	// Query the memory info for AMD/ATI devices, as per the following extension:
	// https://www.khronos.org/registry/OpenGL/extensions/ATI/ATI_meminfo.txt
	//##TODO## See if we can get a true memory total rather than just a free memory total for AMD hardware. The
	//"WGL_GPU_RAM_AMD" query through the "WGL_AMD_gpu_association" extension was shown to return 0 on all our test
	//hardware, so this doesn't seem to be a viable option.
#ifdef GL_ATI_meminfo
	if ((GLAD_GL_ATI_meminfo != 0) && (dedicatedVideoMemoryTotalInKilobytes == 0) && (vendor == Vendor::AMD))
	{
		GLint amdMemoryInfo[4];
		glGetIntegerv(GL_TEXTURE_FREE_MEMORY_ATI, &amdMemoryInfo[0]);
		dedicatedVideoMemoryTotalInKilobytes = amdMemoryInfo[0];
	}
#endif
	if (dedicatedVideoMemoryTotalInKilobytes > 0)
	{
		dedicatedMemorySizeInBytes = (size_t)dedicatedVideoMemoryTotalInKilobytes * (size_t)1024;
	}

	// On macOS, CGL exposes the renderer video memory value directly. Treat Apple and Intel devices as using shared
	// memory, as they are known to be integrated devices.
#ifdef OPENGL_USE_PLATFORM_APPKIT
	if ((dedicatedMemorySizeInBytes == 0) && (sharedMemorySizeInBytes == 0))
	{
		size_t macOSVideoMemorySizeInBytes = 0;
		if (TryReadMacOSOpenGLVideoMemory(macOSVideoMemorySizeInBytes))
		{
			if ((vendor == Vendor::Apple) || (vendor == Vendor::Intel))
			{
				sharedMemorySizeInBytes = macOSVideoMemorySizeInBytes;
			}
			else
			{
				dedicatedMemorySizeInBytes = macOSVideoMemorySizeInBytes;
			}
		}
	}
#endif

#ifdef __linux__
	// Query the memory info when using GLX on Linux, as per the following extension:
	// https://www.khronos.org/registry/OpenGL/extensions/MESA/GLX_MESA_query_renderer.txt
#ifndef OPENGL_USE_EGL
	if ((dedicatedMemorySizeInBytes == 0) && (sharedMemorySizeInBytes == 0))
	{
		TryReadGLXMesaRendererMemory(dedicatedMemorySizeInBytes, sharedMemorySizeInBytes);
	}
#else

	// If we have EGL support on Linux, and we've been provided a DRM render node path, query the kernel driver memory
	// totals to populate the device memory if we still don't have a value. This is only defined for the amdgpu driver,
	// and is documented here:
	// https://docs.kernel.org/gpu/amdgpu/driver-misc.html#gpu-memory-usage-information
	if ((dedicatedMemorySizeInBytes == 0) && (sharedMemorySizeInBytes == 0))
	{
		size_t linuxDedicatedMemorySizeInBytes = 0;
		size_t linuxSharedMemorySizeInBytes = 0;
		if (TryReadLinuxDrmMemoryInfo(_renderNodePath, linuxDedicatedMemorySizeInBytes, linuxSharedMemorySizeInBytes))
		{
			if ((dedicatedMemorySizeInBytes == 0) && (linuxDedicatedMemorySizeInBytes != 0))
			{
				dedicatedMemorySizeInBytes = linuxDedicatedMemorySizeInBytes;
			}
			if ((sharedMemorySizeInBytes == 0) && (linuxSharedMemorySizeInBytes != 0))
			{
				sharedMemorySizeInBytes = linuxSharedMemorySizeInBytes;
			}
		}
	}
#endif
#endif

	// Record the retrieved memory totals for the graphics device
	_dedicatedMemorySizeInBytes = dedicatedMemorySizeInBytes;
	_sharedMemorySizeInBytes = sharedMemorySizeInBytes;
}

#ifdef OPENGL_USE_PLATFORM_APPKIT
//----------------------------------------------------------------------------------------
bool OpenGLGraphicsDevice::TryReadMacOSOpenGLVideoMemory(size_t& videoMemorySizeInBytes)
{
	// Retrieve the display mask for the virtual screen of the current device
	CGLContextObj currentContext = CGLGetCurrentContext();
	if (currentContext == nullptr)
	{
		return false;
	}
	CGLPixelFormatObj pixelFormat = CGLGetPixelFormat(currentContext);
	if (pixelFormat == nullptr)
	{
		return false;
	}
	GLint virtualScreen = 0;
	if (CGLGetVirtualScreen(currentContext, &virtualScreen) != kCGLNoError)
	{
		return false;
	}
	GLint displayMask = 0;
	if (CGLDescribePixelFormat(pixelFormat, virtualScreen, kCGLPFADisplayMask, &displayMask) != kCGLNoError)
	{
		return false;
	}

	// Retrieve the rendererID for the current device
	GLint rendererID = 0;
	if (CGLDescribePixelFormat(pixelFormat, virtualScreen, kCGLPFARendererID, &rendererID) != kCGLNoError)
	{
		return false;
	}

	// Retrieve info for all renderers on the virtual screen of the current device
	CGLRendererInfoObj rendererInfo = nullptr;
	GLint rendererCount = 0;
	if ((CGLQueryRendererInfo((GLuint)displayMask, &rendererInfo, &rendererCount) != kCGLNoError) || (rendererInfo == nullptr))
	{
		return false;
	}

	// Locate the video memory info for the target device
	bool foundMemorySize = false;
	for (GLint rendererIndex = 0; rendererIndex < rendererCount; ++rendererIndex)
	{
		GLint currentRendererID = 0;
		CGLDescribeRenderer(rendererInfo, rendererIndex, kCGLRPRendererID, &currentRendererID);
		if (currentRendererID == rendererID)
		{
			GLint videoMemorySizeInMegabytes = 0;
			CGLDescribeRenderer(rendererInfo, rendererIndex, kCGLRPVideoMemoryMegabytes, &videoMemorySizeInMegabytes);
			videoMemorySizeInBytes = (size_t)videoMemorySizeInMegabytes * (size_t)1024 * (size_t)1024;
			foundMemorySize = true;
			break;
		}
	}

	// Destroy our renderer info for the virtual screen of the current device
	CGLDestroyRendererInfo(rendererInfo);

	// If we found the memory size of the target device, return true to the caller.
	return foundMemorySize;
}
#endif

#if defined(__linux__) && !defined(OPENGL_USE_EGL)
//----------------------------------------------------------------------------------------
bool OpenGLGraphicsDevice::TryReadGLXMesaRendererMemory(size_t& dedicatedMemorySizeInBytes, size_t& sharedMemorySizeInBytes)
{
	// Determine our constants to use
#ifdef GLX_RENDERER_VIDEO_MEMORY_MESA
	constexpr int rendererVideoMemory = GLX_RENDERER_VIDEO_MEMORY_MESA;
#else
	constexpr int rendererVideoMemory = 0x8187;
#endif
#ifdef GLX_RENDERER_UNIFIED_MEMORY_ARCHITECTURE_MESA
	constexpr int rendererUnifiedMemoryArchitecture = GLX_RENDERER_UNIFIED_MEMORY_ARCHITECTURE_MESA;
#else
	constexpr int rendererUnifiedMemoryArchitecture = 0x8188;
#endif

	// Retrieve the glXQueryCurrentRendererIntegerMESA endpoint
	auto glXQueryCurrentRendererIntegerMESA = reinterpret_cast<X11Constants::BoolType (*)(int, unsigned int*)>(glXGetProcAddressARB(reinterpret_cast<const GLubyte*>("glXQueryCurrentRendererIntegerMESA")));
	if (glXQueryCurrentRendererIntegerMESA == nullptr)
	{
		return false;
	}

	// Query the video memory size of the target device
	unsigned int videoMemorySizeInMegabytes = 0;
	if ((glXQueryCurrentRendererIntegerMESA(rendererVideoMemory, &videoMemorySizeInMegabytes) != X11Constants::kTrue) || (videoMemorySizeInMegabytes == 0))
	{
		return false;
	}

	// Check if the tatget device uses UMA
	unsigned int unifiedMemoryArchitecture = 0;
	glXQueryCurrentRendererIntegerMESA(rendererUnifiedMemoryArchitecture, &unifiedMemoryArchitecture);

	// Return the memory size of the target device, reporting it as shared memory if it's a UMA device.
	size_t memorySizeInBytes = (size_t)videoMemorySizeInMegabytes * (size_t)1024 * (size_t)1024;
	if (unifiedMemoryArchitecture != 0)
	{
		sharedMemorySizeInBytes = memorySizeInBytes;
	}
	else
	{
		dedicatedMemorySizeInBytes = memorySizeInBytes;
	}
	return true;
}
#endif

#ifdef __linux__
//----------------------------------------------------------------------------------------
bool OpenGLGraphicsDevice::TryReadLinuxDrmMemoryInfo(const std::string& renderNodePath, size_t& dedicatedMemorySizeInBytes, size_t& sharedMemorySizeInBytes)
{
	// Ensure we have a valid path to the device node
	if (renderNodePath.empty())
	{
		return false;
	}

	// Build a path to the device
	std::string::size_type lastSeparator = renderNodePath.find_last_of("/\\");
	std::string deviceNodeName = (lastSeparator == std::string::npos) ? renderNodePath : renderNodePath.substr(lastSeparator + 1);
	if (deviceNodeName.empty())
	{
		return false;
	}
	std::string sysfsDevicePath = "/sys/class/drm/" + deviceNodeName + "/device/";

	// Attempt to read the device memory totals, and return the results to the caller.
	bool foundMemorySize = false;
	size_t memorySizeInBytes = 0;
	if (TryReadUnsignedIntegerFile(sysfsDevicePath + "mem_info_vram_total", memorySizeInBytes))
	{
		dedicatedMemorySizeInBytes = memorySizeInBytes;
		foundMemorySize = true;
	}
	if (TryReadUnsignedIntegerFile(sysfsDevicePath + "mem_info_gtt_total", memorySizeInBytes))
	{
		sharedMemorySizeInBytes = memorySizeInBytes;
		foundMemorySize = true;
	}
	return foundMemorySize;
}

//----------------------------------------------------------------------------------------
bool OpenGLGraphicsDevice::TryReadUnsignedIntegerFile(const std::string& filePath, size_t& value)
{
	// Open the target file
	std::ifstream file(filePath);
	if (!file.good())
	{
		return false;
	}

	// Read the desired value
	unsigned long long fileValue = 0;
	file >> fileValue;
	if (file.fail())
	{
		return false;
	}

	// Return the value to the caller
	value = (fileValue > (unsigned long long)std::numeric_limits<size_t>::max()) ? std::numeric_limits<size_t>::max() : (size_t)fileValue;
	return true;
}
#endif

//----------------------------------------------------------------------------------------
// Limit methods
//----------------------------------------------------------------------------------------
IGraphicsDevice::ImageLimits OpenGLGraphicsDevice::GetImageLimits() const
{
	return _imageLimits;
}

//----------------------------------------------------------------------------------------
IGraphicsDevice::ShaderLimits OpenGLGraphicsDevice::GetShaderLimits() const
{
	return _shaderLimits;
}

//----------------------------------------------------------------------------------------
IGraphicsDevice::DrawLimits OpenGLGraphicsDevice::GetDrawLimits() const
{
	return _drawLimits;
}

//----------------------------------------------------------------------------------------
IGraphicsDevice::FrameBufferLimits OpenGLGraphicsDevice::GetFrameBufferLimits() const
{
	return _frameBufferLimits;
}

//----------------------------------------------------------------------------------------
IGraphicsDevice::DataBufferLimits OpenGLGraphicsDevice::GetDataBufferLimits() const
{
	return _dataBufferLimits;
}

//----------------------------------------------------------------------------------------
void OpenGLGraphicsDevice::FetchImageLimitsInternal()
{
	ImageLimits imageLimits = {};
	GLint maxTextureSize = 0;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
	imageLimits.maxImageDimensionTexture1D = (uint32_t)maxTextureSize;
	imageLimits.maxImageDimensionTexture2D = (uint32_t)maxTextureSize;

	GLint max3DTextureSize = 0;
	glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &max3DTextureSize);
	imageLimits.maxImageDimensionTexture3D = (uint32_t)max3DTextureSize;

	GLint maxCubeTextureSize = 0;
	glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &maxCubeTextureSize);
	imageLimits.maxImageDimensionTextureCube = (uint32_t)maxCubeTextureSize;

	GLint maxArrayTextureLayers = 0;
	glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &maxArrayTextureLayers);
	imageLimits.maxImageArraySizeTexture1D = maxArrayTextureLayers;
	imageLimits.maxImageArraySizeTexture2D = maxArrayTextureLayers;

	GLfloat maxTextureAnisotropy = 0;
#ifdef GL_EXT_texture_filter_anisotropic
	if (GLAD_GL_EXT_texture_filter_anisotropic != 0)
	{
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxTextureAnisotropy);
	}
#endif
	imageLimits.maxSamplerAnisotropicFilteringLevel = (int)maxTextureAnisotropy;
	_imageLimits = imageLimits;
}

//----------------------------------------------------------------------------------------
void OpenGLGraphicsDevice::FetchShaderLimitsInternal()
{
	ShaderLimits shaderLimits = {};
	GLint maxVertexAttributes = 0;
	glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxVertexAttributes);
	shaderLimits.maxVertexShaderInputAttributes = (uint32_t)maxVertexAttributes;
	GLint maxVertexOutputComponents = 0;
	glGetIntegerv(GL_MAX_VERTEX_OUTPUT_COMPONENTS, &maxVertexOutputComponents);
	shaderLimits.maxVertexShaderOutputComponents = (uint32_t)maxVertexOutputComponents;
	GLint maxGeometryInputComponents = 0;
	glGetIntegerv(GL_MAX_GEOMETRY_INPUT_COMPONENTS, &maxGeometryInputComponents);
	shaderLimits.maxGeometryShaderInputComponents = (uint32_t)maxGeometryInputComponents;
	GLint maxGeometryOutputComponents = 0;
	glGetIntegerv(GL_MAX_GEOMETRY_OUTPUT_COMPONENTS, &maxGeometryOutputComponents);
	shaderLimits.maxGeometryShaderOutputComponents = (uint32_t)maxGeometryOutputComponents;
	GLint maxGeometryOutputVertices = 0;
	glGetIntegerv(GL_MAX_GEOMETRY_OUTPUT_VERTICES, &maxGeometryOutputVertices);
	shaderLimits.maxGeometryShaderOutputVertices = (uint32_t)maxGeometryOutputVertices;
	GLint maxGeometryTotalOutputComponents = 0;
	glGetIntegerv(GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS, &maxGeometryTotalOutputComponents);
	shaderLimits.maxGeometryShaderTotalOutputComponents = (uint32_t)maxGeometryTotalOutputComponents;
	GLint maxFragmentInputComponents = 0;
	glGetIntegerv(GL_MAX_FRAGMENT_INPUT_COMPONENTS, &maxFragmentInputComponents);
	shaderLimits.maxFragmentShaderInputComponents = (uint32_t)maxFragmentInputComponents;
	_shaderLimits = shaderLimits;
}

//----------------------------------------------------------------------------------------
void OpenGLGraphicsDevice::FetchDrawLimitsInternal()
{
	DrawLimits drawLimits = {};

	// There's no obtainable max vertex count per draw on OpenGL. Note that this is NOT GL_MAX_ELEMENTS_VERTICES.
	drawLimits.maxVertexCountPerDraw = std::numeric_limits<uint32_t>::max();

	// We only have a way of getting the max index value on OpenGL 4.3 and above. For below, we substitute the maximum
	// numeric value the index can hold. The real maximum is likely lower than this, but we don't know the real limit.
#ifdef GL_VERSION_4_3
	GLint maxIndex = 0;
	glGetIntegerv(GL_MAX_ELEMENT_INDEX, &maxIndex);
	drawLimits.maxIndexValue = (uint32_t)maxIndex;
#else
	drawLimits.maxIndexValue = std::numeric_limits<uint32_t>::max();
#endif

	GLint maxTextureResouresPerDraw;
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTextureResouresPerDraw);
	drawLimits.maxTextureResourcesPerDraw = (uint32_t)maxTextureResouresPerDraw;

	// No way to query this on OpenGL
	drawLimits.maxResourcesPerDraw = std::numeric_limits<uint32_t>::max();

	_drawLimits = drawLimits;
}

//----------------------------------------------------------------------------------------
void OpenGLGraphicsDevice::FetchFrameBufferLimitsInternal()
{
	FrameBufferLimits frameBufferLimits = {};

	// GL_MAX_FRAMEBUFFER_WIDTH and GL_MAX_FRAMEBUFFER_HEIGHT are only available on OpenGL 4.3 or above. We report the
	// minimim guaranteed framebuffer dimensions here, which is 16384x16384 on earlier OpenGL versions.
	GLint maxFramebufferWidth;
#ifdef GL_VERSION_4_3
	glGetIntegerv(GL_MAX_FRAMEBUFFER_WIDTH, &maxFramebufferWidth);
#else
	maxFramebufferWidth = 16384;
#endif
	frameBufferLimits.maxFrameBufferWidth = (uint32_t)maxFramebufferWidth;
	GLint maxFramebufferHeight;
#ifdef GL_VERSION_4_3
	glGetIntegerv(GL_MAX_FRAMEBUFFER_HEIGHT, &maxFramebufferHeight);
#else
	maxFramebufferHeight = 16384;
#endif
	frameBufferLimits.maxFrameBufferHeight = (uint32_t)maxFramebufferHeight;

	GLint maxDrawBuffers;
	glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBuffers);
	frameBufferLimits.maxFrameBufferColorAttachments = (uint32_t)maxDrawBuffers;
	frameBufferLimits.depthRange = IGraphicsDevice::DepthRange::NegativeOneToOne;
	_frameBufferLimits = frameBufferLimits;
}

//----------------------------------------------------------------------------------------
void OpenGLGraphicsDevice::FetchDataBufferLimitsInternal()
{
	DataBufferLimits limits = {};
	GLint maxStateBufferPageSize;
	glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxStateBufferPageSize);
	limits.maxStateBufferPageSize = (uint32_t)maxStateBufferPageSize;

	// These are consistant between OpenGL versions.
	limits.stateBufferAlignmentFloatOrInt = 4;
	limits.stateBufferAlignmentMatrix4f = 16;
	limits.stateBufferAlignmentVector4f = 16;

#ifdef GL_VERSION_4_3
	// We can use std430
	limits.stateBufferAlignmentStruct = 4;
	limits.stateBufferAlignmentArrayWhole = 4;
	limits.stateBufferAlignmentArrayStride = 4;
#else
	// We have to use std140
	limits.stateBufferAlignmentStruct = 16;
	limits.stateBufferAlignmentArrayWhole = 16;
	limits.stateBufferAlignmentArrayStride = 16;
#endif // GL_VERSION_4_3

	_dataBufferLimits = limits;
}

//----------------------------------------------------------------------------------------
// Feature methods
//----------------------------------------------------------------------------------------
bool OpenGLGraphicsDevice::IsFeatureSupported(Feature feature) const
{
	return (_supportedFeatureSet.find(feature) != _supportedFeatureSet.end());
}

//----------------------------------------------------------------------------------------
bool OpenGLGraphicsDevice::AreAllFeaturesSupported(const Marshal::In<std::set<Feature>>& featureSet) const
{
	auto featureSetResolved = featureSet.Get();
	return std::includes(_supportedFeatureSet.begin(), _supportedFeatureSet.end(), featureSetResolved.begin(), featureSetResolved.end());
}

//----------------------------------------------------------------------------------------
Marshal::Ret<std::set<OpenGLGraphicsDevice::Feature>> OpenGLGraphicsDevice::GetAllSupportedFeatures() const
{
	return _supportedFeatureSet;
}

//----------------------------------------------------------------------------------------
bool OpenGLGraphicsDevice::IsTextureFormatSupported(ITextureBuffer::ImageFormat imageFormat, ITextureBuffer::DataFormat dataFormat) const
{
	// All non-compressed formats are supported either natively or via conversion, so return true for all of them.
	if (!ITextureBuffer::IsCompressedTextureFormat(dataFormat))
	{
		return true;
	}

	// If this is a compressed texture format, return true if it's supported.
	switch (dataFormat)
	{
	case ITextureBuffer::DataFormat::DXT1:
#ifdef GL_EXT_texture_compression_s3tc
		return (GLAD_GL_EXT_texture_compression_s3tc != 0) && ((imageFormat == ITextureBuffer::ImageFormat::RGB) || (imageFormat == ITextureBuffer::ImageFormat::RGBA));
#else
		return false;
#endif
	case ITextureBuffer::DataFormat::DXT3:
	case ITextureBuffer::DataFormat::DXT5:
#ifdef GL_EXT_texture_compression_s3tc
		return (GLAD_GL_EXT_texture_compression_s3tc != 0) && (imageFormat == ITextureBuffer::ImageFormat::RGBA);
#else
		return false;
#endif
	case ITextureBuffer::DataFormat::ETC2:
	{
#if defined(GL_VERSION_4_3) || defined(GL_ARB_ES3_compatibility)
		bool etc2Supported = false;
#ifdef GL_VERSION_4_3
		etc2Supported = etc2Supported || (GLAD_GL_VERSION_4_3 != 0);
#endif
#ifdef GL_ARB_ES3_compatibility
		etc2Supported = etc2Supported || (GLAD_GL_ARB_ES3_compatibility != 0);
#endif
		return etc2Supported && ((imageFormat == ITextureBuffer::ImageFormat::RGB) || (imageFormat == ITextureBuffer::ImageFormat::RGBA));
#else
		return false;
#endif
	}
	case ITextureBuffer::DataFormat::BPTC:
	{
#if defined(GL_VERSION_4_2) || defined(GL_ARB_texture_compression_bptc)
		bool bptcSupported = false;
#ifdef GL_VERSION_4_2
		bptcSupported = bptcSupported || (GLAD_GL_VERSION_4_2 != 0);
#endif
#ifdef GL_ARB_texture_compression_bptc
		bptcSupported = bptcSupported || (GLAD_GL_ARB_texture_compression_bptc != 0);
#endif
		return bptcSupported && (imageFormat == ITextureBuffer::ImageFormat::RGBA);
#else
		return false;
#endif
	}
	case ITextureBuffer::DataFormat::ASTC4x4:
	case ITextureBuffer::DataFormat::ASTC5x5:
	case ITextureBuffer::DataFormat::ASTC6x6:
	case ITextureBuffer::DataFormat::ASTC8x8:
	{
#if defined(GL_KHR_texture_compression_astc_ldr) || defined(GL_KHR_texture_compression_astc_hdr)
		bool astcSupported = false;
#ifdef GL_KHR_texture_compression_astc_ldr
		astcSupported = astcSupported || (GLAD_GL_KHR_texture_compression_astc_ldr != 0);
#endif
#ifdef GL_KHR_texture_compression_astc_hdr
		astcSupported = astcSupported || (GLAD_GL_KHR_texture_compression_astc_hdr != 0);
#endif
		return astcSupported && (imageFormat == ITextureBuffer::ImageFormat::RGBA);
#else
		return false;
#endif
	}
	}
	return false;
}

//----------------------------------------------------------------------------------------
// Renderer methods
//----------------------------------------------------------------------------------------
IRenderer::unique_ptr OpenGLGraphicsDevice::CreateRenderer(const Marshal::In<std::set<Feature>>& enabledFeatures, const Marshal::In<std::set<IRenderer::Options>>& enabledOptions)
{
	// Try and force use of the selected adapter
#ifdef __linux__
	setenv("DRI_PRIME", std::to_string(_deviceIndex).c_str(), 1);
	//##TODO## Verify this does what we want on nvidia hardware
	if (_deviceIndex > 0)
	{
		setenv("__NV_PRIME_RENDER_OFFLOAD", "1", 1);
		setenv("__GLX_VENDOR_LIBRARY_NAME", "nvidia", 1);
	}
	else
	{
		unsetenv("__NV_PRIME_RENDER_OFFLOAD");
		unsetenv("__GLX_VENDOR_LIBRARY_NAME");
	}

	//##TODO## Experiment with this for virtual devices
	//setenv("LIBGL_ALWAYS_SOFTWARE", "1", 1);
	//setenv("GALLIUM_DRIVER", "softpipe", 1);
#endif

	bool finishWhenSwitchingContexts = false;
	bool flushFramebufferTextureWritesBeforeWindowSampling = false;
	bool isIntelDevice = (GetVendor() == Vendor::Intel);
#ifdef _WIN32
	finishWhenSwitchingContexts = isIntelDevice;
#endif
#ifdef __APPLE__
	flushFramebufferTextureWritesBeforeWindowSampling = isIntelDevice;
#endif
#ifdef __linux__
	return IRenderer::unique_ptr(new OpenGLRenderer(_log->CloneLogger(), enabledFeatures.Get(), enabledOptions.Get(), finishWhenSwitchingContexts, flushFramebufferTextureWritesBeforeWindowSampling, _usingSoftwareRenderer, _renderNodePath));
#else
	return IRenderer::unique_ptr(new OpenGLRenderer(_log->CloneLogger(), enabledFeatures.Get(), enabledOptions.Get(), finishWhenSwitchingContexts, flushFramebufferTextureWritesBeforeWindowSampling, _usingSoftwareRenderer));
#endif
}

} // namespace cobalt::graphics
