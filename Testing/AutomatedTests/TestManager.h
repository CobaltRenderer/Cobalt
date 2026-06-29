// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Configuration.h"
#include "ITestSession.h"
#include "IThreadInvocation.h"
#include "Window.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <Internal/ImageDiff/ImageDiff.pkg>
#include <atomic>
#include <filesystem>
#include <vector>
namespace cobalt::graphics::testing {
class IUnitTest;

class TestManager
{
public:
	// Constructors
	TestManager(cobalt::logging::ILogger& log, cobalt::graphics::IGraphicsDevice& device, cobalt::graphics::IRenderer& renderer, const cobalt::graphics::RendererPlugin& rendererPlugin, Window& window, IThreadInvocation& uiThreadInvocation, Configuration& config, ITestSession::WindowSystem windowSystem);

	// Session properties
	cobalt::graphics::IGraphicsDevice& Device() const;
	cobalt::graphics::IRenderer& Renderer() const;
	const cobalt::graphics::RendererPlugin& RendererPlugin() const;
	Window& MainWindow() const;
	Configuration& Config() const;
	IThreadInvocation& UIThread() const;
	std::string DeviceName() const;
	IGraphicsDevice::DeviceType DeviceType() const;
	std::string DriverInfo() const;
	std::string RendererName() const;
	IRendererPlugin::ApiFamily ApiFamily() const;
	IRendererPlugin::ApiVersion ApiVersion() const;
	ITestSession::WindowSystem WindowSystem() const;
	std::string ReportSectionName() const;

	// Test execution methods
	bool ExecuteTests(const std::vector<IUnitTest*>& testSet, const std::filesystem::path& reportFilePath = "", const std::filesystem::path& passFailFilePath = "");
	bool ExecuteTest(IUnitTest* test, const std::filesystem::path& reportFilePath = "");
	void ExecuteStep();
	void SetPausedState(bool state);
	bool IsPaused() const;
	void WaitForStep();

	// Report builder methods
	std::string GetDeviceTypeAsString(IGraphicsDevice::DeviceType deviceType) const;
	std::string GetCurrentTimestampUtcString() const;
	std::string GetOsVersionString() const;

private:
	// Test execution methods
	bool ExecuteTestInternal(IUnitTest* test, const std::filesystem::path& reportFilePath);
	static bool GetAlphaNumericKeyState(int key);

	// Report builder methods
	void CreateDirectoryTreeForFile(const std::filesystem::path& filePath) const;
	void AppendIndexData(const std::filesystem::path& filePath, const std::string& content) const;
	void AppendReportData(const std::filesystem::path& filePath, const std::string& content) const;
	std::string InitialIndexContent(const std::string& sectionName, bool writeInitialHeader, bool fixBrokenPreviousEntry) const;
	std::string InitialReportContent(bool writeInitialHeader) const;
	std::string FinalIndexContent(size_t passedTestCount, size_t testCount) const;
	std::string FinalReportContent(size_t passedTestCount, size_t testCount) const;

private:
	cobalt::logging::ILogger& _log;
	cobalt::graphics::IGraphicsDevice& _device;
	cobalt::graphics::IRenderer& _renderer;
	const cobalt::graphics::RendererPlugin& _rendererPlugin;
	ITestSession::WindowSystem _windowSystem;
	Window& _window;
	IThreadInvocation& _uiThreadInvocation;
	Configuration& _config;
	std::string _rendererName;
	std::string _deviceName;
	std::string _driverInfo;
	IRendererPlugin::ApiFamily _apiFamily;
	IRendererPlugin::ApiVersion _apiVersion;
	std::atomic<bool> _isPaused = false;
	std::atomic<uint32_t> _stepCount = 0;
	uint32_t _testStartedCount = 0;
};

} // namespace cobalt::graphics::testing
