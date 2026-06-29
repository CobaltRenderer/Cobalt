// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include <filesystem>
#include <optional>
#include <string>
#include <vector>
namespace cobalt::graphics::testing {

struct Configuration
{
	bool isNoPrompt = false;
	bool isForceIntegrated = false;
	bool isForceSoftware = false;
	bool isForceDiscrete = false;
	bool isNoInterface = false;
	bool isHelp = false;
	bool isKillAfterTest = false;
	bool isListingTests = false;
	bool isWaitingForDebugger = false;
	bool isTestingAll = false;
	bool runAllPerformanceTests = false;
	bool isGeneratingReport = false;
	bool isUpdateData = false;
	bool isUpdateDataNoPrompt = false;
	bool isUpdateDataAll = false;
	bool isUsingHtmlIndex = false;
	bool isDemo = false;
	bool isApiValidation = false;

	std::optional<uint32_t> logVerbosity;
	std::optional<std::string> onErrorBehavior;
	std::optional<std::string> engine;
	std::optional<std::string> card;
	std::optional<std::string> windowSystem;
	std::optional<std::string> htmlIndexSectionName;
	std::optional<std::filesystem::path> logFile;
	std::optional<std::filesystem::path> passFailFile;
	std::optional<std::filesystem::path> htmlReportFile;
	std::optional<std::filesystem::path> referenceDir;
	std::optional<std::string> testMustContain;
	std::optional<std::string> testIgnoreIfContains;
	std::optional<std::vector<std::string>> testsToRun;
	std::optional<std::vector<std::string>> pluginsToLoad;
	std::optional<std::vector<std::string>> dllSearchDirs;

	std::filesystem::path executableFolderPath;
};

} // namespace cobalt::graphics::testing
