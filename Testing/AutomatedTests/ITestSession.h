// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "IThreadInvocation.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/PlatformBindings.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <Internal/ImageDiff/ImageDiff.pkg>
#include <chrono>
#include <string>
namespace cobalt::graphics::testing {

class ITestSession
{
public:
	// Enumerations
	enum class WindowSystem
	{
		// Headless
		Headless,
		// Windows
		Win32,
		// Linux
		Xlib,
		XCB,
		// macOS
		AppKit,
	};

	// Structures
	struct ProfileResults
	{
		size_t framesDrawn;
		std::chrono::duration<double, std::chrono::milliseconds::period> totalSetupTime;
		std::chrono::duration<double, std::chrono::milliseconds::period> totalRenderTime;
		std::chrono::duration<float, std::chrono::milliseconds::period> averageFrameDrawTime;
		float averageFPS;
	};

public:
	// Session properties
	virtual cobalt::graphics::IGraphicsDevice& Device() const = 0;
	virtual cobalt::graphics::IRenderer& Renderer() const = 0;
	virtual const cobalt::graphics::RendererPlugin& RendererPlugin() const = 0;
	virtual cobalt::logging::ILogger& Log() const = 0;
	virtual cobalt::graphics::IFrameBuffer::WindowInfoBase* TestWindowPlatformInfo() const = 0;
	virtual V2UInt32 TestWindowSize() const = 0;
	virtual V2Float32 TestWindowSizeAsFloat() const = 0;
	virtual IRendererPlugin::ApiFamily ApiFamily() const = 0;
	virtual IRendererPlugin::ApiVersion ApiVersion() const = 0;
	virtual WindowSystem GetWindowSystem() const = 0;
	virtual IThreadInvocation& UIThread() const = 0;

	// Session methods
	virtual void NotifyBeforeFrameDraw() const = 0;

	// Test result methods
	virtual void AddTestInfo(const std::string& testName, const std::string& message) = 0;
	virtual void AddTestSuccess(const std::string& testName, const std::string& testDescription) = 0;
	virtual void AddTestFailure(const std::string& testName, const std::string& testDescription, const std::string& failureMessage) = 0;
	virtual void AddTestSkipped(const std::string& testName, const std::string& testDescription) = 0;
	virtual void AddTestImageWithoutReferenceCompare(const std::string& testName, const std::string& testDescription, cobalt::graphics::IFrameBufferOutput::unique_ptr capturedImage) = 0;
	virtual void AddTestImageResult(const std::string& testName, const std::string& testDescription, cobalt::graphics::IFrameBufferOutput::unique_ptr capturedImage, cobalt::graphics::IImageDiff::Algorithm imageDiffAlgorithm = IImageDiff::Algorithm::AllOfThem, double imageDiffThreshold = 0.99) = 0;
	virtual void AddTestProfileResult(const std::string& testName, const std::string& testDescription, const ProfileResults& results) = 0;
};

} // namespace cobalt::graphics::testing
