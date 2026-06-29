// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "ITestSession.h"
#include "IUnitTest.h"
#include "Window.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/PlatformBindings.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <Internal/ImageDiff/ImageDiff.pkg>
#include <filesystem>
#include <vector>
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/CGLTypes.h>
#endif
namespace cobalt::graphics::testing {
class TestManager;

class TestSession : public ITestSession
{
public:
	// Constructors
	TestSession(TestManager& testManager, cobalt::logging::ILogger::unique_ptr log, IUnitTest* test, WindowSystem windowSystem);

	// Session properties
	cobalt::graphics::IGraphicsDevice& Device() const override;
	cobalt::graphics::IRenderer& Renderer() const override;
	const cobalt::graphics::RendererPlugin& RendererPlugin() const override;
	cobalt::logging::ILogger& Log() const override;
	cobalt::graphics::IFrameBuffer::WindowInfoBase* TestWindowPlatformInfo() const override;
	V2UInt32 TestWindowSize() const override;
	V2Float32 TestWindowSizeAsFloat() const override;
	IRendererPlugin::ApiFamily ApiFamily() const override;
	IRendererPlugin::ApiVersion ApiVersion() const override;
	WindowSystem GetWindowSystem() const override;
	IThreadInvocation& UIThread() const override;

	// Session methods
	void InitializeSession();
	void FinalizeSession();
	void EnableReportOutput(const std::filesystem::path& reportFilePath);
	void NotifyBeforeFrameDraw() const override;
	size_t GetTotalTestCount() const;
	size_t GetPassedTestCount() const;
	size_t GetSkippedTestCount() const;

	// Test result methods
	void AddTestInfo(const std::string& testName, const std::string& message) override;
	void AddTestSuccess(const std::string& testName, const std::string& testDescription) override;
	void AddTestFailure(const std::string& testName, const std::string& testDescription, const std::string& failureMessage) override;
	void AddTestSkipped(const std::string& testName, const std::string& testDescription) override;
	void AddTestImageWithoutReferenceCompare(const std::string& testName, const std::string& testDescription, cobalt::graphics::IFrameBufferOutput::unique_ptr capturedImage) override;
	void AddTestImageResult(const std::string& testName, const std::string& testDescription, cobalt::graphics::IFrameBufferOutput::unique_ptr capturedImage, IImageDiff::Algorithm imageDiffAlgorithm, double imageDiffThreshold) override;
	void AddTestProfileResult(const std::string& testName, const std::string& testDescription, const ProfileResults& results) override;

private:
	// Structures
	struct TestResult
	{
		bool result = false;
		bool skipped = false;
		bool hasImage = false;
		bool performImageComparison = false;
		bool hasProfileResults = false;
		std::string failureMessage;
		cobalt::graphics::IFrameBufferOutput::unique_ptr capturedImage;
		std::string testName;
		std::string testDescription;
		IImageDiff::Algorithm imageDiffAlgorithm = {};
		double imageDiffThreshold = {};
		ProfileResults profileResults = {};
	};

private:
	// Test result methods
	void ProcessTestResult(TestResult& testResult);
	std::filesystem::path ReferenceDataFolder(const std::string& testFullName);
	bool GetReferenceImagePathForTest(const std::string& testName, std::filesystem::path& imagePath);

	// Report builder methods
	void AppendReportData(const std::filesystem::path& filePath, const std::string& content) const;
	std::string ReportInitialContent() const;
	std::string ReportFinalContent() const;
	std::string ReportTestResultBeginBlock(bool passed, bool skipped) const;
	std::string ReportTestResultEndBlock() const;
	std::string ReportSingleImageFailureBlock(const std::string& imageName, const std::string& imageDescription, const std::string& errorDescription, const IImageRGBA& image) const;
	std::string ReportSingleImageNonReferenceBlock(const std::string& imageName, const std::string& imageDescription, const IImageRGBA& image) const;
	std::string ReportTextFailureBlock(const std::string& imageName, const std::string& imageDescription, const std::string& errorDescription) const;
	std::string ReportTextSuccessBlock(const std::string& testName, const std::string& testDescription) const;
	std::string ReportImageDiffBlock(const std::string& shotName, const std::string& shotExtraData, const IImageRGBA& expectedImage, const IImageRGBA& actualImage, double diffPassValue, double actualDiffValue, const std::map<std::string, std::string>& diffMessages) const;
	std::string ImageSizeToString(const IImageRGBA::ImageSize& imageSize) const;
	std::string EncodeDataBase64(const uint8_t* data, size_t dataSizeInBytes) const;

private:
	TestManager& _testManager;
	cobalt::logging::ILogger::unique_ptr _log;
	WindowSystem _windowSystem;
	IUnitTest* _test;
	mutable std::unique_ptr<Window> _childWindow;
	std::vector<TestResult> _testResults;
	bool _hasPreviewSummary = false;
	bool _enableHtmlReport = false;
	bool _hasValidatedRefereceImageDirectory = false;
	std::string _testFullName;
	std::filesystem::path _reportFilePath;
	std::filesystem::path _testSpecificHtmlFile;
	mutable std::unique_ptr<cobalt::graphics::WindowInfoHeadless> _windowInfoHeadless;
#ifdef COBALT_RENDERER_WIN32_SUPPORT
	mutable std::unique_ptr<cobalt::graphics::WindowInfoWin32> _windowInfoWin32;
#endif
#ifdef COBALT_RENDERER_XCB_SUPPORT
	mutable std::unique_ptr<cobalt::graphics::WindowInfoXCB> _windowInfoXcb;
#endif
#ifdef COBALT_RENDERER_XLIB_SUPPORT
	mutable std::unique_ptr<cobalt::graphics::WindowInfoXlib> _windowInfoXlib;
#endif
#ifdef COBALT_RENDERER_APPKIT_SUPPORT
	mutable std::unique_ptr<cobalt::graphics::WindowInfoAppKit> _windowInfoAppKit;
#endif
};

} // namespace cobalt::graphics::testing
