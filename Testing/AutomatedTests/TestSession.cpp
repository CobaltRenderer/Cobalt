// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "TestSession.h"
#include "AssemblyVersionInfo.h"
#include "Debug.h"
#include "StringHelpers.h"
#include "TestManager.h"
#include <Cobalt/RendererInterface/PlatformBindings.pkg>
#include <Internal/ImageDiff/ImageDiff.pkg>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <optional>
#include <thread>
namespace cobalt::graphics::testing {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
TestSession::TestSession(TestManager& testManager, cobalt::logging::ILogger::unique_ptr log, IUnitTest* test, WindowSystem windowSystem)
: _testManager(testManager), _log(std::move(log)), _test(test), _testFullName(test->TestFullName()), _windowSystem(windowSystem)
{}

//----------------------------------------------------------------------------------------
// Session properties
//----------------------------------------------------------------------------------------
cobalt::graphics::IGraphicsDevice& TestSession::Device() const
{
	return _testManager.Device();
}

//----------------------------------------------------------------------------------------
cobalt::graphics::IRenderer& TestSession::Renderer() const
{
	return _testManager.Renderer();
}

//----------------------------------------------------------------------------------------
const cobalt::graphics::RendererPlugin& TestSession::RendererPlugin() const
{
	return _testManager.RendererPlugin();
}

//----------------------------------------------------------------------------------------
cobalt::logging::ILogger& TestSession::Log() const
{
	return *_log;
}

//----------------------------------------------------------------------------------------
cobalt::graphics::IFrameBuffer::WindowInfoBase* TestSession::TestWindowPlatformInfo() const
{
	// If we're targeting a headless window, return the headless window info.
	if (_windowSystem == WindowSystem::Headless)
	{
		if (_windowInfoHeadless == nullptr)
		{
			_windowInfoHeadless = std::make_unique<cobalt::graphics::WindowInfoHeadless>(TestWindowSize());
		}
		return _windowInfoHeadless.get();
	}

#ifdef COBALT_RENDERER_WIN32_SUPPORT
	// For the benefit of the OpenGL renderers in particular, we need to create a new child window for each test for the
	// renderers to bind to rather than reusing the one window between tests. As per the documentation for
	// SetPixelFormat, Windows only allows the pixel format for a window to be set once for its lifetime. After this,
	// attempts to change to a pixel format with different settings will fail. Since the depth and stencil bits are
	// defined in the pixel format, this means different tests will not work if different depth/stencil settings are
	// used on the one shared window between tests. It appears that only OpenGL has this limitation, however in order to
	// keep things simple, we create a new child window for each test and destroy it afterwards, regardless of which
	// renderer is in use.
	if (_childWindow == nullptr)
	{
		_childWindow = std::make_unique<Window>();
		_testManager.UIThread().InvokeSyncVoidReturn([&] {
			auto& mainWindow = _testManager.MainWindow();
			HWND mainWindowHandle = mainWindow.GetOsHandle();
			RECT rect;
			GetClientRect(mainWindowHandle, &rect);
			HINSTANCE hinstance = ::GetModuleHandle(nullptr);
			_childWindow->Create("", rect.right, rect.bottom, WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0, hinstance, mainWindowHandle, nullptr, true);
			mainWindow.AttachHandler(WM_SIZE, [&](WPARAM, LPARAM) {
				RECT rect2;
				GetClientRect(mainWindowHandle, &rect2);
				SetWindowPos(_childWindow->GetOsHandle(), nullptr, 0, 0, rect2.right, rect2.bottom, SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOACTIVATE);
				return 0;
			});
			_childWindow->AttachCloseHandler([&] { mainWindow.RemoveHandler(WM_SIZE); });
		});
	}
#endif

#ifdef COBALT_RENDERER_WIN32_SUPPORT
	if (_windowSystem == WindowSystem::Win32)
	{
		if (_windowInfoWin32 == nullptr)
		{
			_windowInfoWin32 = std::make_unique<cobalt::graphics::WindowInfoWin32>(_childWindow->GetOsHandle(), TestWindowSize());
		}
		return _windowInfoWin32.get();
	}
#endif
#ifdef COBALT_RENDERER_XLIB_SUPPORT
	if (_windowSystem == WindowSystem::Xlib)
	{
		if (_windowInfoXlib == nullptr)
		{
			_windowInfoXlib = std::make_unique<cobalt::graphics::WindowInfoXlib>(_testManager.MainWindow().GetDisplay(), _testManager.MainWindow().GetWindow(), TestWindowSize());
		}
		return _windowInfoXlib.get();
	}
#endif
#ifdef COBALT_RENDERER_XCB_SUPPORT
	if (_windowSystem == WindowSystem::XCB)
	{
		if (_windowInfoXcb == nullptr)
		{
			_windowInfoXcb = std::make_unique<cobalt::graphics::WindowInfoXCB>(_testManager.MainWindow().GetXCBConnection(), _testManager.MainWindow().GetXCBWindow(), TestWindowSize());
		}
		return _windowInfoXcb.get();
	}
#endif
#ifdef COBALT_RENDERER_APPKIT_SUPPORT
	if (_windowSystem == WindowSystem::AppKit)
	{
		if (_windowInfoAppKit == nullptr)
		{
			_windowInfoAppKit = std::make_unique<cobalt::graphics::WindowInfoAppKit>(_testManager.MainWindow().GetAppKitView(), TestWindowSize());
		}
		return _windowInfoAppKit.get();
	}
#endif
	return nullptr;
}

//----------------------------------------------------------------------------------------
V2UInt32 TestSession::TestWindowSize() const
{
	// If we're targeting a headless window, return a fixed size.
	//##TODO## Make it possible to set this in main
	if (_windowSystem == WindowSystem::Headless)
	{
		return V2UInt32(1024, 768);
	}

#ifdef _WIN32
	if (_childWindow == nullptr)
	{
		TestWindowPlatformInfo();
	}
	RECT rect;
	GetClientRect(_childWindow->GetOsHandle(), &rect);
	return V2UInt32(rect.right, rect.bottom);
#elif defined(__APPLE__)
	auto view = _testManager.MainWindow().GetAppKitView();
	NSSize viewSizeInFractionalPixels = [view convertSizeToBacking:view.bounds.size];
	CGSize viewSizeInPixels = CGSizeMake(std::floor(viewSizeInFractionalPixels.width), std::floor(viewSizeInFractionalPixels.height));
	return V2UInt32((unsigned int)viewSizeInPixels.width, (unsigned int)viewSizeInPixels.height);
#else
	int x, y;
	unsigned int width, height, border, depth;
	::Window root;
	XGetGeometry(_testManager.MainWindow().GetDisplay(), _testManager.MainWindow().GetWindow(), &root, &x, &y, &width, &height, &border, &depth);
	return V2UInt32(width, height);
#endif
}

//----------------------------------------------------------------------------------------
V2Float32 TestSession::TestWindowSizeAsFloat() const
{
	auto windowSize = TestWindowSize();
	return V2Float32((float)windowSize.X(), (float)windowSize.Y());
}

//----------------------------------------------------------------------------------------
IRendererPlugin::ApiFamily TestSession::ApiFamily() const
{
	return _testManager.ApiFamily();
}

//----------------------------------------------------------------------------------------
IRendererPlugin::ApiVersion TestSession::ApiVersion() const
{
	return _testManager.ApiVersion();
}

//----------------------------------------------------------------------------------------
ITestSession::WindowSystem TestSession::GetWindowSystem() const
{
	return _windowSystem;
}

//----------------------------------------------------------------------------------------
IThreadInvocation& TestSession::UIThread() const
{
	return _testManager.UIThread();
}

//----------------------------------------------------------------------------------------
// Session methods
//----------------------------------------------------------------------------------------
void TestSession::InitializeSession()
{
	_log->Info("Started test {0}", _testFullName);
	if (_testManager.Config().isDemo)
	{
		_log->Info("Reference image comparison disabled. Tests will briefly pause to show visual results.");
	}

	auto testSafeName = _testFullName + " " + _testManager.RendererName() + " " + _testManager.DeviceName();
	std::replace(testSafeName.begin(), testSafeName.end(), '/', '_');
	std::replace(testSafeName.begin(), testSafeName.end(), ' ', '_');
	std::replace(testSafeName.begin(), testSafeName.end(), ':', '_');
	std::replace(testSafeName.begin(), testSafeName.end(), '\\', '_');
	auto fileName = _testManager.ReportSectionName();
	if (!fileName.empty())
	{
		fileName += " ";
	}
	fileName += testSafeName + ".html";

	if (_enableHtmlReport)
	{
		auto root = std::filesystem::path(_reportFilePath).parent_path();
		auto file = root;
		file /= fileName;
		_testSpecificHtmlFile = file;

		std::error_code errorCode;
		std::filesystem::remove(file, errorCode);

		AppendReportData(_testSpecificHtmlFile, ReportInitialContent());

		std::string mainEntry = R"(
<tr><td>
<h3><a href=')" +
		  fileName + "'>" + _testFullName + "</a></h3></td>";
		AppendReportData(_reportFilePath, mainEntry);
	}
}

//----------------------------------------------------------------------------------------
void TestSession::FinalizeSession()
{
	if (_enableHtmlReport)
	{
		AppendReportData(_testSpecificHtmlFile, ReportFinalContent());

		if (!_hasPreviewSummary)
		{
			AppendReportData(_reportFilePath, "<td>No image</td>");
		}
		std::string summaryEnd = "<td ";
		size_t passedTests = GetPassedTestCount();
		size_t skippedTests = GetSkippedTestCount();
		size_t totalTests = GetTotalTestCount();
		size_t totalNonSkippedTests = totalTests - skippedTests;
		bool allTestsSkipped = (skippedTests == totalTests);
		bool allTestsPassed = (passedTests == totalNonSkippedTests);
		if (allTestsSkipped)
		{
			summaryEnd += "class='testCaseSkipped'";
		}
		else if (allTestsPassed)
		{
			summaryEnd += "class='testCasePassed'";
		}
		else
		{
			summaryEnd += "class='testCaseFailed'";
		}
		summaryEnd += ">";
		if (allTestsSkipped)
		{
			summaryEnd += "Skipped";
		}
		else
		{
			summaryEnd += std::to_string(passedTests) + " / " + std::to_string(totalNonSkippedTests);
		}
		summaryEnd += "</td></tr>";
		AppendReportData(_reportFilePath, summaryEnd);
	}

	Renderer().WaitForDeferredDeletionComplete();
	_log->Info("Finished test {0}", _testFullName);
}

//----------------------------------------------------------------------------------------
void TestSession::EnableReportOutput(const std::filesystem::path& reportFilePath)
{
	_enableHtmlReport = true;
	_reportFilePath = reportFilePath;
}

//----------------------------------------------------------------------------------------
void TestSession::NotifyBeforeFrameDraw() const
{}

//----------------------------------------------------------------------------------------
size_t TestSession::GetTotalTestCount() const
{
	return _testResults.size();
}

//----------------------------------------------------------------------------------------
size_t TestSession::GetPassedTestCount() const
{
	size_t passedCount = 0;
	for (const auto& entry : _testResults)
	{
		if (!entry.skipped && entry.result)
		{
			++passedCount;
		}
	}
	return passedCount;
}

//----------------------------------------------------------------------------------------
size_t TestSession::GetSkippedTestCount() const
{
	size_t skippedCount = 0;
	for (const auto& entry : _testResults)
	{
		if (entry.skipped)
		{
			++skippedCount;
		}
	}
	return skippedCount;
}

//----------------------------------------------------------------------------------------
// Test result methods
//----------------------------------------------------------------------------------------
void TestSession::AddTestInfo(const std::string& testName, const std::string& message)
{
	auto& testResult = _testResults.emplace_back();
	testResult.testName = testName;
	testResult.testDescription = message;
	testResult.result = true;
	testResult.hasImage = false;
	ProcessTestResult(testResult);
}

//----------------------------------------------------------------------------------------
void TestSession::AddTestSuccess(const std::string& testName, const std::string& testDescription)
{
	auto& testResult = _testResults.emplace_back();
	testResult.testName = testName;
	testResult.testDescription = testDescription;
	testResult.result = true;
	testResult.skipped = false;
	testResult.hasImage = false;
	ProcessTestResult(testResult);
}

//----------------------------------------------------------------------------------------
void TestSession::AddTestFailure(const std::string& testName, const std::string& testDescription, const std::string& failureMessage)
{
	auto& testResult = _testResults.emplace_back();
	testResult.testName = testName;
	testResult.testDescription = testDescription;
	testResult.result = false;
	testResult.skipped = false;
	testResult.hasImage = false;
	testResult.failureMessage = failureMessage;
	ProcessTestResult(testResult);
}

//----------------------------------------------------------------------------------------
void TestSession::AddTestSkipped(const std::string& testName, const std::string& testDescription)
{
	auto& testResult = _testResults.emplace_back();
	testResult.testName = testName;
	testResult.testDescription = testDescription;
	testResult.result = true;
	testResult.skipped = true;
	testResult.hasImage = false;
	ProcessTestResult(testResult);
}

//----------------------------------------------------------------------------------------
void TestSession::AddTestImageWithoutReferenceCompare(const std::string& testName, const std::string& testDescription, cobalt::graphics::IFrameBufferOutput::unique_ptr capturedImage)
{
	auto& testResult = _testResults.emplace_back();
	testResult.testName = testName;
	testResult.testDescription = testDescription;
	testResult.result = true;
	testResult.skipped = false;
	testResult.hasImage = true;
	testResult.performImageComparison = false;
	testResult.capturedImage = std::move(capturedImage);
	ProcessTestResult(testResult);
}

//----------------------------------------------------------------------------------------
void TestSession::AddTestImageResult(const std::string& testName, const std::string& testDescription, cobalt::graphics::IFrameBufferOutput::unique_ptr capturedImage, IImageDiff::Algorithm imageDiffAlgorithm, double imageDiffThreshold)
{
	auto& testResult = _testResults.emplace_back();
	testResult.testName = testName;
	testResult.testDescription = testDescription;
	testResult.result = true;
	testResult.skipped = false;
	testResult.hasImage = true;
	testResult.performImageComparison = true;
	testResult.capturedImage = std::move(capturedImage);
	testResult.imageDiffAlgorithm = imageDiffAlgorithm;
	testResult.imageDiffThreshold = imageDiffThreshold;
	ProcessTestResult(testResult);
}

//----------------------------------------------------------------------------------------
void TestSession::AddTestProfileResult(const std::string& testName, const std::string& testDescription, const ProfileResults& results)
{
	auto& testResult = _testResults.emplace_back();
	testResult.testName = testName;
	testResult.testDescription = testDescription;
	testResult.result = true;
	testResult.skipped = false;
	testResult.hasImage = false;
	testResult.performImageComparison = false;
	testResult.hasProfileResults = true;
	testResult.profileResults = results;
	ProcessTestResult(testResult);
}

//----------------------------------------------------------------------------------------
void TestSession::ProcessTestResult(TestResult& testResult)
{
	// Inform the user that we're processing this test result
	_log->Info("Processing test result {0}: {1}", testResult.testName, testResult.testDescription);

	// Attempt to retrieve the captured image output if present
	bool hasActualImage = false;
	auto actualImage = IImageRGBA::Create();
	if (testResult.hasImage)
	{
		// Read the pixel data from the framebuffer output
		std::string failureReason;
		std::vector<V4UInt8> pixels;
		bool readPixelData = true;
		if ((testResult.capturedImage == nullptr) || !testResult.capturedImage->HasCapturedOutput())
		{
			testResult.failureMessage += std::string(testResult.failureMessage.empty() ? "" : "\n") + "Failed to capture a framebuffer output for test " + testResult.testName;
			testResult.result = false;
			readPixelData = false;
		}
		else if (!testResult.capturedImage->ReadBufferData(pixels))
		{
			testResult.failureMessage += std::string(testResult.failureMessage.empty() ? "" : "\n") + "Failed to read framebuffer output for test " + testResult.testName;
			testResult.result = false;
			readPixelData = false;
		}

		// Load the pixel data into our image object
		if (readPixelData)
		{
			auto capturedImageSize = testResult.capturedImage->GetImageDimensions();
			actualImage->FromPixelData(IImageRGBA::ImageSize{capturedImageSize.X(), capturedImageSize.Y()}, reinterpret_cast<const IImageRGBA::PixelEntry*>(pixels.data()));
			auto actualImageSize = actualImage->Size();
			if ((actualImageSize.width < 16) || (actualImageSize.height < 16))
			{
				testResult.failureMessage += std::string(testResult.failureMessage.empty() ? "" : "\n") + "Rendered image too small: " + testResult.testName + " " + ImageSizeToString(actualImage->Size());
				testResult.result = false;
			}
			if ((actualImageSize.width > 16384) || (actualImageSize.height > 16384))
			{
				testResult.failureMessage += std::string(testResult.failureMessage.empty() ? "" : "\n") + "Rendered image too large: " + testResult.testName + " " + ImageSizeToString(actualImage->Size());
				testResult.result = false;
			}
			else
			{
				hasActualImage = true;
			}
		}
	}

	// Validate that the reference image directory exists, creating it if required.
	const auto& config = _testManager.Config();
	if (hasActualImage && !_hasValidatedRefereceImageDirectory && testResult.performImageComparison)
	{
		auto referenceImageDataFolder = ReferenceDataFolder(_testFullName);
		std::error_code errorCode;
		if (!std::filesystem::is_directory(referenceImageDataFolder, errorCode))
		{
			if (config.isUpdateData || config.isUpdateDataAll || config.isUpdateDataNoPrompt)
			{
				std::filesystem::create_directories(referenceImageDataFolder, errorCode);
			}
			if (!std::filesystem::is_directory(referenceImageDataFolder, errorCode))
			{
				_log->Error("{0} doesn't exist! Set --reference-dir to specify where reference image files are located", referenceImageDataFolder);
			}
		}
	}

	// Attempt to retrieve the reference image if required
	bool hasReferenceImage = false;
	auto referenceImage = IImageRGBA::Create();
	std::filesystem::path referenceImagePath;
	if (!config.isDemo && testResult.hasImage && testResult.performImageComparison)
	{
		std::string failureReason;
		hasReferenceImage = GetReferenceImagePathForTest(testResult.testName, referenceImagePath);
		if (!hasReferenceImage)
		{
			failureReason = "Failed to locate reference image file";
		}
		else
		{
			// Attempt to load the reference image
			if (!referenceImage->FromFile(referenceImagePath.native(), _log.get()))
			{
				failureReason = "Failed to load reference image";
				hasReferenceImage = false;
			}
			else
			{
				auto referenceImageSize = referenceImage->Size();
				if ((referenceImageSize.width < 16) || (referenceImageSize.height < 16))
				{
					failureReason = "Reference image too small: " + ImageSizeToString(referenceImage->Size());
					hasReferenceImage = false;
				}
				else if ((referenceImageSize.width > 16384) || (referenceImageSize.height > 16384))
				{
					failureReason = "Reference image too large: " + ImageSizeToString(referenceImage->Size());
					hasReferenceImage = false;
				}
			}
		}
		if (!hasReferenceImage)
		{
			testResult.failureMessage += std::string(testResult.failureMessage.empty() ? "" : "\n") + failureReason;
			testResult.result = false;
		}
	}

	// Compare the generated image to the reference image if required
	bool performedImageComparison = false;
	bool imageComparisonPassed = false;
	std::string imageComparisonReportData;
	if (!config.isDemo && testResult.hasImage && testResult.performImageComparison)
	{
		if (hasReferenceImage && hasActualImage)
		{
			_log->Info("Performing reference image comparison...");
			auto imageComparison = IImageDiff::Create(*referenceImage, *actualImage);
			double diffValue;
			std::map<std::string, std::string> diffMessages;
			imageComparisonPassed = imageComparison->CompareImages(testResult.imageDiffAlgorithm, testResult.imageDiffThreshold, diffValue, diffMessages);
			performedImageComparison = true;

			imageComparisonReportData = ReportImageDiffBlock(testResult.testName, testResult.testDescription, *referenceImage, *actualImage, testResult.imageDiffThreshold, diffValue, diffMessages);
			_log->Log(imageComparisonPassed ? cobalt::logging::ILogger::Severity::Info : cobalt::logging::ILogger::Severity::Warning, "Image comparison {0}: {1}, scored {2}, target {3}", imageComparisonPassed ? "passed" : "failed", testResult.testName, diffValue, testResult.imageDiffThreshold);
			if (!imageComparisonPassed)
			{
				testResult.result = false;
			}
		}
	}

	// Write the test result to the log file
	if (testResult.result)
	{
		_log->Info("Test {0} passed", testResult.testName);
	}
	else
	{
		_log->Warning("Test {0} failed: {1}", testResult.testName, testResult.failureMessage);
	}

	// Write the result of this test to the report file if required
	std::string profileResults;
	if (!config.isDemo && _enableHtmlReport)
	{
		if (performedImageComparison)
		{
			AppendReportData(_testSpecificHtmlFile, ReportTestResultBeginBlock(imageComparisonPassed, testResult.skipped));
			AppendReportData(_testSpecificHtmlFile, imageComparisonReportData);
			AppendReportData(_testSpecificHtmlFile, ReportTestResultEndBlock());
		}
		else if (testResult.hasImage)
		{
			if (!testResult.performImageComparison)
			{
				AppendReportData(_testSpecificHtmlFile, ReportTestResultBeginBlock(true, testResult.skipped));
				AppendReportData(_testSpecificHtmlFile, ReportSingleImageNonReferenceBlock(testResult.testName, testResult.testDescription, *actualImage));
				AppendReportData(_testSpecificHtmlFile, ReportTestResultEndBlock());
			}
			else if (hasReferenceImage)
			{
				AppendReportData(_testSpecificHtmlFile, ReportTestResultBeginBlock(false, testResult.skipped));
				AppendReportData(_testSpecificHtmlFile, ReportSingleImageFailureBlock(testResult.testName, testResult.testDescription, testResult.failureMessage, *referenceImage));
				AppendReportData(_testSpecificHtmlFile, ReportTestResultEndBlock());
			}
			else if (hasActualImage)
			{
				AppendReportData(_testSpecificHtmlFile, ReportTestResultBeginBlock(false, testResult.skipped));
				AppendReportData(_testSpecificHtmlFile, ReportSingleImageFailureBlock(testResult.testName, testResult.testDescription, testResult.failureMessage, *actualImage));
				AppendReportData(_testSpecificHtmlFile, ReportTestResultEndBlock());
			}
			else
			{
				AppendReportData(_testSpecificHtmlFile, ReportTestResultBeginBlock(false, testResult.skipped));
				AppendReportData(_testSpecificHtmlFile, ReportTextFailureBlock(testResult.testName, testResult.testDescription, testResult.failureMessage));
				AppendReportData(_testSpecificHtmlFile, ReportTestResultEndBlock());
			}
		}
		else if (testResult.hasProfileResults)
		{
			profileResults = "averageFPS: " + std::to_string(testResult.profileResults.averageFPS) + "\n" + "averageFrameDrawTime: " + std::to_string(testResult.profileResults.averageFrameDrawTime.count()) + "\n" + "framesDrawn: " + std::to_string(testResult.profileResults.framesDrawn) + "\n" + "totalRenderTime: " + std::to_string(testResult.profileResults.totalRenderTime.count()) + "\n" + "totalSetupTime: " + std::to_string(testResult.profileResults.totalSetupTime.count()) + "\n";
			AppendReportData(_testSpecificHtmlFile, ReportTestResultBeginBlock(testResult.result, testResult.skipped));
			AppendReportData(_testSpecificHtmlFile, ReportTextSuccessBlock(testResult.testName, testResult.testDescription + "\n" + profileResults));
			AppendReportData(_testSpecificHtmlFile, ReportTestResultEndBlock());
		}
		else
		{
			AppendReportData(_testSpecificHtmlFile, ReportTestResultBeginBlock(testResult.result, testResult.skipped));
			AppendReportData(_testSpecificHtmlFile, (testResult.result ? ReportTextSuccessBlock(testResult.testName, testResult.testDescription) : ReportTextFailureBlock(testResult.testName, testResult.testDescription, testResult.failureMessage)));
			AppendReportData(_testSpecificHtmlFile, ReportTestResultEndBlock());
		}
	}

	// Write profile results to the summary of the parent report if required
	if (!config.isDemo && _enableHtmlReport && !_hasPreviewSummary && testResult.hasProfileResults)
	{
		std::string data = "<td>" + profileResults + "</td>";
		AppendReportData(_reportFilePath, data);
		_hasPreviewSummary = true;
	}

	// Perform any actions required in response to the image being generated
	if (!config.isDemo && hasActualImage)
	{
		// Write this image to the parent report if no image has been written so far.
		if (!_hasPreviewSummary && _enableHtmlReport)
		{
			std::vector<uint8_t> pngData;
			if (!actualImage->ToPngData(pngData, _log.get()))
			{
				_log->Error("Failed to export image to PNG data stream");
			}
			else
			{
				std::string data = "<td><img width=" + std::to_string(actualImage->Size().width / 4) + " src='data:image/png;base64," + EncodeDataBase64(pngData.data(), pngData.size()) + "'></td>";
				AppendReportData(_reportFilePath, data);
				_hasPreviewSummary = true;
			}
		}

		// Update the output image file if required
		bool updateTest = config.isUpdateDataAll && testResult.performImageComparison;
		if ((!hasReferenceImage || !imageComparisonPassed) && testResult.performImageComparison)
		{
			updateTest |= config.isUpdateData || config.isUpdateDataNoPrompt;
		}
		if (updateTest)
		{
			if (referenceImagePath.empty())
			{
				std::error_code errorCode;
				std::filesystem::path referenceImagePathBuilder = std::filesystem::weakly_canonical(ReferenceDataFolder(_testFullName), errorCode);
				referenceImagePathBuilder /= testResult.testName + ".png";
				referenceImagePath = referenceImagePathBuilder;
			}

			_log->Info("Updating reference image file {0}", referenceImagePath);
			std::filesystem::path pngFilePath = referenceImagePath;

			if (config.isUpdateData && !config.isUpdateDataNoPrompt)
			{
				std::wcout << L"Update image data for " + pngFilePath.wstring() + L" [yn]?\n";
				char response;
				std::cin >> response;
				std::cout << "\n";
				if (response != 'y' && response != 'Y')
				{
					updateTest = false;
				}
			}

			if (updateTest)
			{
				if (!actualImage->ToPngFile(pngFilePath.native(), _log.get()))
				{
					_log->Error("Couldn't write to {0}", pngFilePath.native());
				}
				referenceImagePath = pngFilePath;
			}
		}
	}

	// If we need to handle a delay before proceeding with the next test, do that now.
	if (_testManager.IsPaused())
	{
		_log->Info("Pausing, as single step mode active. Press 'n' or use menu to advance");
		_testManager.WaitForStep();
	}
	else if (config.isDemo)
	{
		// Pause for a bit here so that the user can see what's happening
		std::this_thread::sleep_for(std::chrono::milliseconds(3000));
	}
}

//----------------------------------------------------------------------------------------
std::filesystem::path TestSession::ReferenceDataFolder(const std::string& testFullName)
{
	// If an explicit reference data folder is defined, use that.
	auto& config = _testManager.Config();
	if (config.referenceDir)
	{
		return (*config.referenceDir / testFullName);
	}

	// If no reference data folder is defined, and we appear to be in our output build directory, use the location of
	// the committed reference images as the target.
	auto buildRelativeDirectory = config.executableFolderPath / ".." / ".." / "Testing" / "AutomatedTests" / "ReferenceImages";
	std::error_code errorCode;
	buildRelativeDirectory = std::filesystem::weakly_canonical(buildRelativeDirectory, errorCode);
	if (std::filesystem::exists(buildRelativeDirectory, errorCode))
	{
		return (buildRelativeDirectory / testFullName);
	}

	// Default to a reference images subdirectory next to the executable
	return (config.executableFolderPath / "ReferenceImages" / testFullName);
}

//----------------------------------------------------------------------------------------
bool TestSession::GetReferenceImagePathForTest(const std::string& testName, std::filesystem::path& imagePath)
{
	// Attempt to locate the reference image file
	bool foundReferenceImage = false;
	auto screenshotExtensions = {".png"};
	for (const auto& extension : screenshotExtensions)
	{
		std::error_code errorCode;
		std::filesystem::path referenceImagePath = std::filesystem::weakly_canonical(ReferenceDataFolder(_testFullName), errorCode);
		referenceImagePath /= testName + extension;
		if (std::filesystem::exists(referenceImagePath, errorCode))
		{
			foundReferenceImage = true;
			imagePath = referenceImagePath;
			break;
		}
	}
	return foundReferenceImage;
}

//----------------------------------------------------------------------------------------
// Report builder methods
//----------------------------------------------------------------------------------------
void TestSession::AppendReportData(const std::filesystem::path& filePath, const std::string& content) const
{
	std::ofstream out(filePath, std::ios_base::app);
	out.write(content.data(), content.size());
	out.close();
}

//----------------------------------------------------------------------------------------
std::string TestSession::ReportInitialContent() const
{
	std::string deviceTypeAsString = _testManager.GetDeviceTypeAsString(_testManager.DeviceType());
	std::string timestampString = _testManager.GetCurrentTimestampUtcString();
	return R"(
<html>
<head>
<title>)" +
	  _testFullName +
	  R"(</title>
<style>
.testCaseImage
{
  background-image: url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAGQAAABkCAIAAAD/gAIDAAAACXBIWXMAAAsTAAALEwEAmpwYAAAAB3RJTUUH3ggRDQA1Yl77tAAAAN1JREFUeNrt1rEJwCAURVEVFwruv4BEMtGHdGJpF4LHLqQ7+OTm3u+0nNau9XOMx995SnK2DyxYsD4/OSK86Jt/3SwzhAXrT6d60RW8GcKCpeAVvPtihrBgKXgFb4YIYMFS8AreDGEhgKXgFbwZwoKFQMEreDOEBUvBe9EVvBnCgqXgFbz7YoawYCl4BW+GCGDBUvAK3gxhIYCl4BW8GcKChUDBK3gzhAVLwXvRFbwZwoKl4BW8+2KGsGApeAVvhghgwVLwCt4MYSGApeAVvBnCgoVAwSt4M4QF6/jzAlCrcKXDcpR1AAAAAElFTkSuQmCC);
}
.testCaseImage img
{
  border:1px solid black;
}
.testCasePassed
{
  background: #afa;
}
.testCaseFailed
{
  background: #faa;
}
.testCaseSkipped
{
  background: #c9c9c9;
}
</style>
</head>
<body>
<h1>)" +
	  _testFullName + R"(</h1>
<table>
<tr><td>Renderer</td><td>)" +
	  _testManager.RendererName() + R"(</td></tr>
<tr><td>Build version</td><td>)" +
	  std::string(AssemblyVersionInfo_VersionLabelFull) + R"(</td></tr>
<tr><td>Device</td><td>)" +
	  _testManager.DeviceName() + R"(</td></tr>
<tr><td>Device type</td><td>)" +
	  deviceTypeAsString + R"(</td></tr>
<tr><td>Driver info</td><td>)" +
	  _testManager.DriverInfo() + R"(</td></tr>
<tr><td>OS version</td><td>)" +
	  _testManager.GetOsVersionString() + R"(</td></tr>
<tr><td>Start time</td><td>)" +
	  timestampString + R"(</td></tr>
</table><br><table width='100%' border=1>
<tr><th>Test</th><th>Actual</th><th>Expected</th></tr>
)";
}

//----------------------------------------------------------------------------------------
std::string TestSession::ReportFinalContent() const
{
	return R"(</table></body></html>)";
}

//----------------------------------------------------------------------------------------
std::string TestSession::ReportTestResultBeginBlock(bool passed, bool skipped) const
{
	return std::string("<tr class='testCaseRow ") + (skipped ? std::string("testCaseSkipped") : (passed ? std::string("testCasePassed") : std::string("testCaseFailed"))) + "'>";
}

//----------------------------------------------------------------------------------------
std::string TestSession::ReportTestResultEndBlock() const
{
	return "</tr>";
}

//----------------------------------------------------------------------------------------
std::string TestSession::ReportSingleImageFailureBlock(const std::string& imageName, const std::string& imageDescription, const std::string& errorDescription, const IImageRGBA& image) const
{
	auto out = "<td class='testCaseBegin'><h1>" + imageName + "</h1>";
	if (!imageDescription.empty())
	{
		out += "<p>" + imageDescription + "</p>";
	}

	if (!errorDescription.empty())
	{
		out += "<p>" + errorDescription + "</p>";
	}

	std::vector<uint8_t> pngData;
	image.ToPngData(pngData, _log.get());

	out += "</td><td colspan=2 class='testCaseImage'><img width=" + std::to_string(image.Size().width / 2) + " src='";
	out += "data:image/png;base64," + EncodeDataBase64(pngData.data(), pngData.size());
	out += "'></td>";
	return out;
}

//----------------------------------------------------------------------------------------
std::string TestSession::ReportSingleImageNonReferenceBlock(const std::string& imageName, const std::string& imageDescription, const IImageRGBA& image) const
{
	auto out = "<td class='testCaseBegin'><h1>" + imageName + "</h1>";
	if (!imageDescription.empty())
	{
		out += "<p>" + imageDescription + "</p>";
	}

	std::vector<uint8_t> pngData;
	image.ToPngData(pngData, _log.get());

	out += "</td><td colspan=2 class='testCaseImage'><img width=" + std::to_string(image.Size().width / 2) + " src='";
	out += "data:image/png;base64," + EncodeDataBase64(pngData.data(), pngData.size());
	out += "'></td>";
	return out;
}

//----------------------------------------------------------------------------------------
std::string TestSession::ReportTextFailureBlock(const std::string& imageName, const std::string& imageDescription, const std::string& errorDescription) const
{
	auto out = "<td colspan=3 class='testCaseBegin'><h1>" + imageName + "</h1>";
	if (!imageDescription.empty())
	{
		out += "<p>" + imageDescription + "</p>";
	}

	if (!errorDescription.empty())
	{
		out += "<p>" + errorDescription + "</p>";
	}

	out += "</td>";
	return out;
}

//----------------------------------------------------------------------------------------
std::string TestSession::ReportTextSuccessBlock(const std::string& testName, const std::string& testDescription) const
{
	auto out = "<td colspan=3 class='testCaseBegin'><h1>" + testName + "</h1>";
	if (!testDescription.empty())
	{
		out += "<p>" + StringReplaceAll(testDescription, "\n", "<br>") + "</p>";
	}

	out += "</td>";
	return out;
}

//----------------------------------------------------------------------------------------
std::string TestSession::ReportImageDiffBlock(const std::string& shotName, const std::string& shotExtraData, const IImageRGBA& expectedImage, const IImageRGBA& actualImage, double diffPassValue, double actualDiffValue, const std::map<std::string, std::string>& diffMessages) const
{
	auto out = "<td class='testCaseBegin'><h1>" + shotName + "</h1>";
	if (!shotExtraData.empty())
	{
		out += "<p>" + shotExtraData + "</p>";
	}

	out += "<p>Scored " + std::to_string(actualDiffValue) + ", needed: " + std::to_string(diffPassValue) + "</p>";

	if (!diffMessages.empty())
	{
		out += "<table>";
		for (const auto& text : diffMessages)
		{
			out += "<tr><th align='right'>" + text.first + ":</th><td>" + text.second + "</td></tr>";
		}
		out += "</table>";
	}

	std::vector<uint8_t> actualImagePngData;
	actualImage.ToPngData(actualImagePngData, _log.get());
	std::vector<uint8_t> expectedImagePngData;
	expectedImage.ToPngData(expectedImagePngData, _log.get());

	out += "</td><td class='testCaseImage'><img width=" + std::to_string(actualImage.Size().width / 2) + " src=\"data:image/png;base64," + EncodeDataBase64(actualImagePngData.data(), actualImagePngData.size()) + "\">";
	out += "</td><td class='testCaseImage'><img width=" + std::to_string(expectedImage.Size().width / 2) + " src=\"data:image/png;base64," + EncodeDataBase64(expectedImagePngData.data(), expectedImagePngData.size()) + "\">";

	out += "</td>";

	return out;
}

//----------------------------------------------------------------------------------------
std::string TestSession::ImageSizeToString(const IImageRGBA::ImageSize& imageSize) const
{
	return "(" + std::to_string(imageSize.width) + ", " + std::to_string(imageSize.height) + ")";
}

//----------------------------------------------------------------------------------------
std::string TestSession::EncodeDataBase64(const uint8_t* data, size_t dataSizeInBytes) const
{
	static const std::string base64Chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	uint8_t const* bytesToEncode = data;
	size_t inLen = dataSizeInBytes;
	std::string ret;
	int i = 0;
	int j = 0;
	uint8_t charArray3[3] = {0, 0, 0};
	uint8_t charArray4[4] = {0, 0, 0, 0};

	while ((inLen--) != 0u)
	{
		charArray3[i++] = *(bytesToEncode++);
		if (i == 3)
		{
			charArray4[0] = (charArray3[0] & 0xFC) >> 2;
			charArray4[1] = (uint8_t)(((charArray3[0] & 0x03) << 4) + ((charArray3[1] & 0xF0) >> 4));
			charArray4[2] = (uint8_t)(((charArray3[1] & 0x0F) << 2) + ((charArray3[2] & 0xC0) >> 6));
			charArray4[3] = charArray3[2] & 0x3F;

			for (i = 0; (i < 4); ++i)
			{
				ret += base64Chars[charArray4[i]];
			}
			i = 0;
		}
	}

	if (i != 0)
	{
		for (j = i; j < 3; j++)
		{
			charArray3[j] = '\0';
		}

		charArray4[0] = (charArray3[0] & 0xFC) >> 2;
		charArray4[1] = (uint8_t)(((charArray3[0] & 0x03) << 4) + ((charArray3[1] & 0xF0) >> 4));
		charArray4[2] = (uint8_t)(((charArray3[1] & 0x0F) << 2) + ((charArray3[2] & 0xC0) >> 6));

		for (j = 0; (j < i + 1); j++)
		{
			ret += base64Chars[charArray4[j]];
		}

		while ((i++ < 3))
		{
			ret += '=';
		}
	}

	return ret;
}

} // namespace cobalt::graphics::testing
