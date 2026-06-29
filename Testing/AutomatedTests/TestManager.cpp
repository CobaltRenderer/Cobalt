// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "TestManager.h"
#include "IUnitTest.h"
#include "TestSession.h"
#include <array>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <thread>
#include <utility>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#elif defined(__APPLE__)
#include <sys/sysctl.h>
#include <sys/types.h>
#elif defined(__linux__)
#include <sys/utsname.h>
#endif
namespace cobalt::graphics::testing {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
TestManager::TestManager(cobalt::logging::ILogger& log, cobalt::graphics::IGraphicsDevice& device, cobalt::graphics::IRenderer& renderer, const cobalt::graphics::RendererPlugin& rendererPlugin, Window& window, IThreadInvocation& uiThreadInvocation, Configuration& config, ITestSession::WindowSystem windowSystem)
: _log(log), _device(device), _renderer(renderer), _rendererPlugin(rendererPlugin), _windowSystem(windowSystem), _window(window), _uiThreadInvocation(uiThreadInvocation), _config(config), _rendererName(rendererPlugin.GetDisplayName()), _apiFamily(rendererPlugin.GetApiFamily()), _apiVersion(rendererPlugin.GetTargetApiVersion())
{
	_deviceName = _device.GetVendorName() + " - " + _device.GetDeviceName();
	_driverInfo = _device.GetDriverInfo();
}

//----------------------------------------------------------------------------------------
// Session properties
//----------------------------------------------------------------------------------------
cobalt::graphics::IGraphicsDevice& TestManager::Device() const
{
	return _device;
}

//----------------------------------------------------------------------------------------
cobalt::graphics::IRenderer& TestManager::Renderer() const
{
	return _renderer;
}

//----------------------------------------------------------------------------------------
const cobalt::graphics::RendererPlugin& TestManager::RendererPlugin() const
{
	return _rendererPlugin;
}

//----------------------------------------------------------------------------------------
Window& TestManager::MainWindow() const
{
	return _window;
}

//----------------------------------------------------------------------------------------
Configuration& TestManager::Config() const
{
	return _config;
}

//----------------------------------------------------------------------------------------
IThreadInvocation& TestManager::UIThread() const
{
	return _uiThreadInvocation;
}

//----------------------------------------------------------------------------------------
std::string TestManager::DeviceName() const
{
	return _deviceName;
}

//----------------------------------------------------------------------------------------
IGraphicsDevice::DeviceType TestManager::DeviceType() const
{
	return _device.GetDeviceType();
}

//----------------------------------------------------------------------------------------
std::string TestManager::DriverInfo() const
{
	return _driverInfo;
}

//----------------------------------------------------------------------------------------
std::string TestManager::RendererName() const
{
	return _rendererName;
}

//----------------------------------------------------------------------------------------
IRendererPlugin::ApiFamily TestManager::ApiFamily() const
{
	return _apiFamily;
}

//----------------------------------------------------------------------------------------
IRendererPlugin::ApiVersion TestManager::ApiVersion() const
{
	return _apiVersion;
}

//----------------------------------------------------------------------------------------
ITestSession::WindowSystem TestManager::WindowSystem() const
{
	return _windowSystem;
}

//----------------------------------------------------------------------------------------
std::string TestManager::ReportSectionName() const
{
	return (_config.isUsingHtmlIndex ? _config.htmlIndexSectionName.value() : "");
}

//----------------------------------------------------------------------------------------
// Test execution methods
//----------------------------------------------------------------------------------------
bool TestManager::ExecuteTests(const std::vector<IUnitTest*>& testSet, const std::filesystem::path& reportFilePath, const std::filesystem::path& passFailFilePath)
{
	// Reset our running counter of started tests
	_testStartedCount = 0;
	size_t testCount = testSet.size();

	// If we're generating a report file, write leading content to the file.
	bool enableHtmlReport = !reportFilePath.empty();
	bool enableHtmlIndex = _config.isUsingHtmlIndex;
	std::filesystem::path reportIndexPath;
	if (enableHtmlReport)
	{
		// Ensure the target directory exists
		CreateDirectoryTreeForFile(reportFilePath);

		// If this report is using a html index, add our content to it now.
		if (enableHtmlIndex)
		{
			// Build the path to the report index
			reportIndexPath = _config.htmlReportFile.value();

			// Write the leading content to the index report
			bool writeInitialIndexHeader = false;
			bool fixBrokenPreviousEntry = false;
			if (!std::filesystem::exists(reportIndexPath))
			{
				std::ofstream out(reportIndexPath, std::ios_base::out);
				out.close();
				writeInitialIndexHeader = true;
			}
			else
			{
				std::ifstream in(reportIndexPath, std::ios_base::in);
				in.seekg(-5, std::ios::end);
				char buf[6] = {};
				in.read(&buf[0], 5);
				fixBrokenPreviousEntry = (std::string(&buf[0]) == "</td>");
			}
			AppendIndexData(reportIndexPath, InitialIndexContent(_config.htmlIndexSectionName.value(), writeInitialIndexHeader, fixBrokenPreviousEntry));
		}

		// If we're not appending to an existing report, create or truncate the report file.
		bool writeInitialHeader = false;
		if (!std::filesystem::exists(reportFilePath))
		{
			std::ofstream out(reportFilePath, std::ios_base::out);
			out.close();
			writeInitialHeader = true;
		}

		// Load the initial content into the report file
		AppendReportData(reportFilePath, InitialReportContent(writeInitialHeader));
	}

	// Execute each requested test
	size_t passedTestCount = 0;
	for (auto* test : testSet)
	{
		if (ExecuteTestInternal(test, reportFilePath))
		{
			++passedTestCount;
		}
	}
	bool passedAllTests = (passedTestCount == testCount);

	// If we're generating a report file, write the final content to the file.
	if (enableHtmlReport)
	{
		AppendReportData(reportFilePath, FinalReportContent(passedTestCount, testCount));
		if (enableHtmlIndex)
		{
			AppendIndexData(reportIndexPath, FinalIndexContent(passedTestCount, testCount));
		}
	}

	// If we're generating a pass/fail file, write the file now.
	if (_config.passFailFile)
	{
		std::ofstream out(*_config.passFailFile, std::ios_base::app);
		if (!passedAllTests)
		{
			out.write("FAILED", 6);
		}
		else
		{
			out.write("PASS", 4);
		}
		out.close();
	}

	// Return the result to the caller
	return passedAllTests;
}

//----------------------------------------------------------------------------------------
bool TestManager::ExecuteTest(IUnitTest* test, const std::filesystem::path& reportFilePath)
{
	// Execute the test, and return the result to the caller
	std::vector<IUnitTest*> testSet;
	testSet.push_back(test);
	return ExecuteTests(testSet, reportFilePath);
}

//----------------------------------------------------------------------------------------
bool TestManager::ExecuteTestInternal(IUnitTest* test, const std::filesystem::path& reportFilePath)
{
	// Create the test session object
	auto testSpecificLogger = _log.GetLoggerChildScope("T_" + std::to_string(++_testStartedCount));
	TestSession testSession(*this, std::move(testSpecificLogger), test, _windowSystem);

	// Enable report output for the test session if required
	bool enableHtmlReport = !reportFilePath.empty();
	if (enableHtmlReport)
	{
		testSession.EnableReportOutput(reportFilePath);
	}

	// Execute the specified test under this test session
	testSession.InitializeSession();
	if (!test->ExecuteTest(testSession))
	{
		_log.Error("Test {0} reported failure", test->TestFullName());
	}
	testSession.FinalizeSession();

	// If all tests passed, return true to the caller.
	size_t totalNonSkippedTests = testSession.GetTotalTestCount() - testSession.GetSkippedTestCount();
	bool allTestsPassed = (testSession.GetPassedTestCount() == totalNonSkippedTests);
	return allTestsPassed;
};

//----------------------------------------------------------------------------------------
void TestManager::ExecuteStep()
{
	++_stepCount;
}

//----------------------------------------------------------------------------------------
void TestManager::SetPausedState(bool state)
{
	_isPaused = state;
}

//----------------------------------------------------------------------------------------
bool TestManager::IsPaused() const
{
	return _isPaused;
}

//----------------------------------------------------------------------------------------
bool TestManager::GetAlphaNumericKeyState(int key)
{
	ASSERT(key >= 'A' && key <= 'Z' || key >= '0' && key <= '9');
//##FIX## This needs to support other platforms
#ifdef _WIN32
	return (::GetAsyncKeyState(key) & ~1) != 0;
#else
	return false;
#endif
}

//----------------------------------------------------------------------------------------
void TestManager::WaitForStep()
{
	uint32_t startStepCount = _stepCount;
	while (_isPaused && (startStepCount == _stepCount))
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		if (GetAlphaNumericKeyState('N'))
		{
			++_stepCount;
			while (GetAlphaNumericKeyState('N'))
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
			break;
		}
	}
}

//----------------------------------------------------------------------------------------
// Report builder methods
//----------------------------------------------------------------------------------------
void TestManager::CreateDirectoryTreeForFile(const std::filesystem::path& filePath) const
{
	auto parentFolder = filePath.parent_path();
	std::error_code errorCode;
	if (!std::filesystem::is_directory(parentFolder, errorCode))
	{
		if (!std::filesystem::create_directories(parentFolder, errorCode))
		{
			_log.Error("Can't create directories for {0} to hold {1}. Error code {2}:{3}", parentFolder.native(), filePath.native(), errorCode.value(), errorCode.message());
		}
	}
}

//----------------------------------------------------------------------------------------
void TestManager::AppendIndexData(const std::filesystem::path& filePath, const std::string& content) const
{
	std::ofstream out(filePath, std::ios_base::app);
	out.write(content.data(), content.size());
	out.close();
}

//----------------------------------------------------------------------------------------
void TestManager::AppendReportData(const std::filesystem::path& filePath, const std::string& content) const
{
	std::ofstream out(filePath, std::ios_base::app);
	out.write(content.data(), content.size());
	out.close();
}

//----------------------------------------------------------------------------------------
std::string TestManager::InitialIndexContent(const std::string& sectionName, bool writeInitialHeader, bool fixBrokenPreviousEntry) const
{
	// If we're not appending to the report file, fill the initial report content.
	std::string setup;
	if (writeInitialHeader)
	{
		setup = R"(<html><head><title>Renderer test results</title>
<style>
.testCasePassed
{
  background: #afa;
}
.testCaseFailed
{
  background: #faa;
}
</style>
</head>
<body>
<table width='100%' border=1>
<tr><th>Run name</th><th>Graphics hardware</th><th>Passed tests</th></tr>
)";
	}

	// If we've detected a broken previous entry, IE, because the unit test process crashed before finishing writing
	// results, close off the previous entry and prepare for a new one.
	if (fixBrokenPreviousEntry)
	{
		setup += "<td class='testCaseFailed'>Crashed</td></tr>";
	}

	// Write the start of our results table to the report
	std::string deviceTypeAsString = GetDeviceTypeAsString(_device.GetDeviceType());
	setup += R"(
<tr>
<td><h3><a href=")" +
	  sectionName + R"(.html">)" + sectionName + R"(</a></h3></td><td>)" + _device.GetDeviceName().Get() + R"( [Type: )" + deviceTypeAsString + R"(]</td>)";
	return setup;
}

//----------------------------------------------------------------------------------------
std::string TestManager::InitialReportContent(bool writeInitialHeader) const
{
	// If we're not appending to the report file, fill the initial report content.
	std::string setup;
	if (writeInitialHeader)
	{
		setup = R"(<html><head><title>Renderer test results</title>
<style>
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
<body>)";
	}

	// Write the device information tables and start of our results table to the report
	std::string deviceTypeAsString = GetDeviceTypeAsString(_device.GetDeviceType());
	const auto imageLimits = _device.GetImageLimits();
	const auto shaderLimits = _device.GetShaderLimits();
	const auto drawLimits = _device.GetDrawLimits();
	const auto frameBufferLimits = _device.GetFrameBufferLimits();
	const auto dataBufferLimits = _device.GetDataBufferLimits();
	setup += R"(
<h1>)" +
	  _rendererName + " - " + _deviceName + R"( [Type: )" + deviceTypeAsString + R"(])" + R"(</h1>
<table width='100%' border=0 cellspacing=0 cellpadding=0>
<tr>
<td width='50%' valign='top'>
<h2>Device Limits</h2>
<table width='100%' border=1>
<tr><th>Limit</th><th>Value</th></tr>
<tr><td colspan='2'><b>MemorySize</b></td></tr>
)";
	setup += "<tr><td>Dedicated</td><td>" + std::to_string(_device.GetMemorySizeInBytes(IGraphicsDevice::MemoryType::Dedicated)) + "</td></tr>\n";
	setup += "<tr><td>Shared</td><td>" + std::to_string(_device.GetMemorySizeInBytes(IGraphicsDevice::MemoryType::Shared)) + "</td></tr>\n";
	setup += R"(<tr><td colspan='2'><b>ImageLimits</b></td></tr>
)";
	setup += "<tr><td>maxImageDimensionTexture1D</td><td>" + std::to_string(imageLimits.maxImageDimensionTexture1D) + "</td></tr>\n";
	setup += "<tr><td>maxImageDimensionTexture2D</td><td>" + std::to_string(imageLimits.maxImageDimensionTexture2D) + "</td></tr>\n";
	setup += "<tr><td>maxImageDimensionTexture3D</td><td>" + std::to_string(imageLimits.maxImageDimensionTexture3D) + "</td></tr>\n";
	setup += "<tr><td>maxImageDimensionTextureCube</td><td>" + std::to_string(imageLimits.maxImageDimensionTextureCube) + "</td></tr>\n";
	setup += "<tr><td>maxImageArraySizeTexture1D</td><td>" + std::to_string(imageLimits.maxImageArraySizeTexture1D) + "</td></tr>\n";
	setup += "<tr><td>maxImageArraySizeTexture2D</td><td>" + std::to_string(imageLimits.maxImageArraySizeTexture2D) + "</td></tr>\n";
	setup += "<tr><td>maxSamplerAnisotropicFilteringLevel</td><td>" + std::to_string(imageLimits.maxSamplerAnisotropicFilteringLevel) + "</td></tr>\n";
	setup += R"(<tr><td colspan='2'><b>ShaderLimits</b></td></tr>
)";
	setup += "<tr><td>maxVertexShaderInputAttributes</td><td>" + std::to_string(shaderLimits.maxVertexShaderInputAttributes) + "</td></tr>\n";
	setup += "<tr><td>maxVertexShaderOutputComponents</td><td>" + std::to_string(shaderLimits.maxVertexShaderOutputComponents) + "</td></tr>\n";
	setup += "<tr><td>maxGeometryShaderInputComponents</td><td>" + std::to_string(shaderLimits.maxGeometryShaderInputComponents) + "</td></tr>\n";
	setup += "<tr><td>maxGeometryShaderOutputComponents</td><td>" + std::to_string(shaderLimits.maxGeometryShaderOutputComponents) + "</td></tr>\n";
	setup += "<tr><td>maxGeometryShaderOutputVertices</td><td>" + std::to_string(shaderLimits.maxGeometryShaderOutputVertices) + "</td></tr>\n";
	setup += "<tr><td>maxGeometryShaderTotalOutputComponents</td><td>" + std::to_string(shaderLimits.maxGeometryShaderTotalOutputComponents) + "</td></tr>\n";
	setup += "<tr><td>maxFragmentShaderInputComponents</td><td>" + std::to_string(shaderLimits.maxFragmentShaderInputComponents) + "</td></tr>\n";
	setup += R"(<tr><td colspan='2'><b>DrawLimits</b></td></tr>
)";
	setup += "<tr><td>maxVertexCountPerDraw</td><td>" + std::to_string(drawLimits.maxVertexCountPerDraw) + "</td></tr>\n";
	setup += "<tr><td>maxIndexValue</td><td>" + std::to_string(drawLimits.maxIndexValue) + "</td></tr>\n";
	setup += "<tr><td>maxTextureResourcesPerDraw</td><td>" + std::to_string(drawLimits.maxTextureResourcesPerDraw) + "</td></tr>\n";
	setup += "<tr><td>maxResourcesPerDraw</td><td>" + std::to_string(drawLimits.maxResourcesPerDraw) + "</td></tr>\n";
	setup += R"(<tr><td colspan='2'><b>FrameBufferLimits</b></td></tr>
)";
	setup += "<tr><td>maxFrameBufferWidth</td><td>" + std::to_string(frameBufferLimits.maxFrameBufferWidth) + "</td></tr>\n";
	setup += "<tr><td>maxFrameBufferHeight</td><td>" + std::to_string(frameBufferLimits.maxFrameBufferHeight) + "</td></tr>\n";
	setup += "<tr><td>maxFrameBufferColorAttachments</td><td>" + std::to_string(frameBufferLimits.maxFrameBufferColorAttachments) + "</td></tr>\n";
	setup += "<tr><td>depthRange</td><td>" + std::to_string(static_cast<int>(frameBufferLimits.depthRange)) + "</td></tr>\n";
	setup += R"(<tr><td colspan='2'><b>DataBufferLimits</b></td></tr>
)";
	setup += "<tr><td>maxStateBufferPageSize</td><td>" + std::to_string(dataBufferLimits.maxStateBufferPageSize) + "</td></tr>\n";
	setup += "<tr><td>stateBufferAlignmentFloatOrInt</td><td>" + std::to_string(dataBufferLimits.stateBufferAlignmentFloatOrInt) + "</td></tr>\n";
	setup += "<tr><td>stateBufferAlignmentVector4f</td><td>" + std::to_string(dataBufferLimits.stateBufferAlignmentVector4f) + "</td></tr>\n";
	setup += "<tr><td>stateBufferAlignmentMatrix4f</td><td>" + std::to_string(dataBufferLimits.stateBufferAlignmentMatrix4f) + "</td></tr>\n";
	setup += "<tr><td>stateBufferAlignmentArrayWhole</td><td>" + std::to_string(dataBufferLimits.stateBufferAlignmentArrayWhole) + "</td></tr>\n";
	setup += "<tr><td>stateBufferAlignmentArrayStride</td><td>" + std::to_string(dataBufferLimits.stateBufferAlignmentArrayStride) + "</td></tr>\n";
	setup += "<tr><td>stateBufferAlignmentStruct</td><td>" + std::to_string(dataBufferLimits.stateBufferAlignmentStruct) + "</td></tr>\n";
	setup += R"(</table>
</td>
<td style='width: 24px'>&nbsp;</td>
<td width='50%' valign='top'>
<h2>Device Feature Flags</h2>
<table width='100%' border=1>
<tr><th>Feature</th><th>Supported</th></tr>
)";
	const std::array<std::pair<IGraphicsDevice::Feature, const char*>, 15> featureReportEntries = {{
	  {IGraphicsDevice::Feature::AnisotropicFiltering, "AnisotropicFiltering"},
	  {IGraphicsDevice::Feature::GeometryShaders, "GeometryShaders"},
	  {IGraphicsDevice::Feature::ComputeShaders, "ComputeShaders"},
	  {IGraphicsDevice::Feature::MeshShaders, "MeshShaders"},
	  {IGraphicsDevice::Feature::DepthBiasClamp, "DepthBiasClamp"},
	  {IGraphicsDevice::Feature::IndirectDraw, "IndirectDraw"},
	  {IGraphicsDevice::Feature::IndirectMultiDrawNative, "IndirectMultiDrawNative"},
	  {IGraphicsDevice::Feature::InstanceOffset, "InstanceOffset"},
	  {IGraphicsDevice::Feature::PolygonWireframeFillMode, "PolygonWireframeFillMode"},
	  {IGraphicsDevice::Feature::ResourceArrays, "ResourceArrays"},
	  {IGraphicsDevice::Feature::ShaderArraysOfArrays, "ShaderArraysOfArrays"},
	  {IGraphicsDevice::Feature::SeparateBlendModePerTarget, "SeparateBlendModePerTarget"},
	  {IGraphicsDevice::Feature::SeparateTextureSamplers, "SeparateTextureSamplers"},
	  {IGraphicsDevice::Feature::TextureCubeArray, "TextureCubeArray"},
	  {IGraphicsDevice::Feature::MipmapLevelBias, "MipmapLevelBias"},
	}};
	for (const auto& featureEntry : featureReportEntries)
	{
		const bool supported = _device.IsFeatureSupported(featureEntry.first);
		setup += "<tr><td>";
		setup += featureEntry.second;
		setup += "</td><td class='";
		setup += (supported ? "testCasePassed" : "testCaseFailed");
		setup += "'>";
		setup += (supported ? "Yes" : "No");
		setup += "</td></tr>\n";
	}
	setup += R"(</table>
</td>
</tr>
</table>
<h2>Test Results</h2>
<table width='100%' border=1>
<tr><th>Test Name</th><th>Preview</th><th>Passed tests</th></tr>
)";
	return setup;
}

//----------------------------------------------------------------------------------------
std::string TestManager::FinalIndexContent(size_t passedTestCount, size_t testCount) const
{
	std::string report;
	report += "<td class='" + ((passedTestCount == testCount) ? std::string("testCasePassed") : std::string("testCaseFailed")) + "'>" + std::to_string(passedTestCount) + " / " + std::to_string(testCount) + "</td>";
	report += "</tr>";
	return report;
}

//----------------------------------------------------------------------------------------
std::string TestManager::FinalReportContent(size_t passedTestCount, size_t testCount) const
{
	std::string report;
	report += "<tr>";
	report += "<td colspan=\"2\"><h3>" + ((passedTestCount == testCount) ? std::string("All tests passed") : std::to_string(testCount - passedTestCount) + std::string(" tests failed")) + "</h3></td>";
	report += "<td class='" + ((passedTestCount == testCount) ? std::string("testCasePassed") : std::string("testCaseFailed")) + "'>" + std::to_string(passedTestCount) + " / " + std::to_string(testCount) + "</td>";
	report += "</tr></table></body></html>";
	return report;
}

//----------------------------------------------------------------------------------------
std::string TestManager::GetDeviceTypeAsString(IGraphicsDevice::DeviceType deviceType) const
{
	std::string deviceTypeAsString;
	switch (deviceType)
	{
	case IGraphicsDevice::DeviceType::Discrete:
		deviceTypeAsString = "Discrete";
		break;
	case IGraphicsDevice::DeviceType::Integrated:
		deviceTypeAsString = "Integrated";
		break;
	case IGraphicsDevice::DeviceType::Software:
		deviceTypeAsString = "Software";
		break;
	case IGraphicsDevice::DeviceType::Unknown:
		deviceTypeAsString = "Unknown";
		break;
	}
	return deviceTypeAsString;
}

//----------------------------------------------------------------------------------------
std::string TestManager::GetCurrentTimestampUtcString() const
{
	std::time_t t = std::time(nullptr);
	std::tm tm{};
#if defined(_WIN32)
	(void)gmtime_s(&tm, &t);
#else
	(void)gmtime_r(&t, &tm);
#endif
	std::ostringstream s;
	s << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
	return s.str();
}

//----------------------------------------------------------------------------------------
std::string TestManager::GetOsVersionString() const
{
#if defined(_WIN32)
	// Attempt to obtain the OS version information
	bool gotOsVersionInfo = false;
	RTL_OSVERSIONINFOEXW osVersionInfo = {};
	osVersionInfo.dwOSVersionInfoSize = sizeof(osVersionInfo);
	HMODULE moduleHandle = GetModuleHandleW(L"ntdll.dll");
	if (moduleHandle != nullptr)
	{
		auto rtlGetVersion = reinterpret_cast<LONG(WINAPI*)(PRTL_OSVERSIONINFOW)>(GetProcAddress(moduleHandle, "RtlGetVersion"));
		gotOsVersionInfo = ((rtlGetVersion != nullptr) && (rtlGetVersion(reinterpret_cast<PRTL_OSVERSIONINFOW>(&osVersionInfo)) == 0));
	}

	// Build the version string
	std::ostringstream stringBuilder;
	if (gotOsVersionInfo)
	{
		DWORD majorVersion = ((osVersionInfo.dwMajorVersion == 10) && (osVersionInfo.dwBuildNumber >= 22000) ? 11 : osVersionInfo.dwMajorVersion);
		stringBuilder << "Windows " << majorVersion << "." << osVersionInfo.dwMinorVersion << " (Build " << osVersionInfo.dwBuildNumber << ")";
	}
	else
	{
		stringBuilder << "Windows (unknown version)";
	}
	return stringBuilder.str();

#elif defined(__APPLE__)
	// Obtain the product version label
	char buffer[256] = {};
	size_t bufferSize = sizeof(buffer);
	std::string productVersion = "unknown";
	if ((sysctlbyname("kern.osproductversion", &buffer[0], &bufferSize, nullptr, 0) == 0) && (bufferSize > 0))
	{
		productVersion.assign(&buffer[0]);
	}

	// Obtain the build version
	std::string buildVersion = "unknown";
	bufferSize = sizeof(buffer);
	if ((sysctlbyname("kern.osversion", &buffer[0], &bufferSize, nullptr, 0) == 0) && (bufferSize > 0))
	{
		buildVersion.assign(&buffer[0]);
	}

	// Build and return the version string
	std::ostringstream stringBuilder;
	stringBuilder << "macOS " << productVersion << " (Build " << buildVersion << ")";
	return stringBuilder.str();

#elif defined(__linux__)
	// Attempt to obtain the "pretty name" for this linux release
	std::string prettyName;
	std::ifstream osRelease("/etc/os-release");
	std::string line;
	while (!std::getline(osRelease, line).fail())
	{
		constexpr char key[] = "PRETTY_NAME=";
		constexpr std::size_t keyLength = sizeof(key) - 1;
		if (line.rfind(&key[0], 0) == 0)
		{
			prettyName = line.substr(keyLength);
			if ((prettyName.size() >= 2) && (prettyName.front() == '"') && (prettyName.back() == '"'))
			{
				prettyName = prettyName.substr(1, prettyName.size() - 2);
			}
			break;
		}
	}

	// Attempt to obtain the OS version info using uname
	utsname unameInfo = {};
	bool gotUnameInfo = (uname(&unameInfo) >= 0);

	// Build and return the version string
	std::ostringstream stringBuilder;
	stringBuilder << (!prettyName.empty() ? prettyName : (gotUnameInfo ? &unameInfo.sysname[0] : "Linux (unknown distro)"));
	if (gotUnameInfo)
	{
		stringBuilder << " " << &unameInfo.release[0] << " (" << &unameInfo.version[0] << ")";
	}
	return stringBuilder.str();

#else
	return "Unknown OS";
#endif
}

} // namespace cobalt::graphics::testing
