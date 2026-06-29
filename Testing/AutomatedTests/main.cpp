// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#ifdef _WIN32
// We need to include this first so we don't have WIN32_LEAN_AND_MEAN defined, as we need ShellExecuteW.
#define NOMINMAX
#include <Windows.h>
#endif
#include "Configuration.h"
#include "Debug.h"
#include "IUnitTest.h"
#include "MenuItem.h"
#include "StringHelpers.h"
#include "TestManager.h"
#include "TestRegistry.h"
#include "UnicodeConversion.h"
#include "Window.h"
#include <Cobalt/Debug/Debug.pkg>
#include <Cobalt/Logging/Logging.pkg>
#include <Cobalt/RendererInterface/PlatformBindings.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <Internal/ImageDiff/ImageDiff.pkg>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <cwctype>
#include <exception>
#include <filesystem>
#include <iomanip>
#ifdef _WIN32
#include "UIThreadInvocation.Win32.h"
#include "resource.h"
#include <Internal/CrashHandling/CrashHandling.pkg>
#else
#ifdef __APPLE__
#include "UIThreadInvocation.Cocoa.h"
#include <ApplicationServices/ApplicationServices.h>
#import <Foundation/Foundation.h>
#include <mach-o/dyld.h>
#else
#include "UIThreadInvocation.X11.h"
#endif
#include <climits>
#include <csignal>
#include <dlfcn.h>
#include <execinfo.h>
#include <strings.h>
#endif
using namespace cobalt::graphics::testing;
using namespace cobalt::graphics;

//----------------------------------------------------------------------------------------
class ImmediateThreadInvocation : public IThreadInvocation
{
public:
	explicit ImmediateThreadInvocation(std::thread::id threadId)
	: _threadId(threadId)
	{}

protected:
	std::thread::id GetTargetThreadId() const override
	{
		return _threadId;
	}

	void PushPendingCallback(std::function<void()> callback) override
	{
		callback();
	}

private:
	std::thread::id _threadId;
};

//----------------------------------------------------------------------------------------
static std::filesystem::path GetReportFilePathForOpen(const Configuration& config)
{
	// Generate the report file path based on the config settings
	std::filesystem::path reportFilePath = config.htmlReportFile.value_or(config.executableFolderPath / "Report" / "Results.html");
	if (!std::filesystem::exists(reportFilePath))
	{
		return "";
	}
	return reportFilePath;
}

//----------------------------------------------------------------------------------------
static std::filesystem::path GetReportFilePath(const Configuration& config, bool ignoreReportGenerationActive = false)
{
	// If there's no report generation active, return an empty path.
	if (!config.isGeneratingReport && !ignoreReportGenerationActive)
	{
		return "";
	}

	// Generate the report file path based on the config settings
	std::filesystem::path reportFilePath;
	if (config.htmlReportFile.has_value())
	{
		reportFilePath = (config.isUsingHtmlIndex ? config.htmlReportFile.value().parent_path() / (config.htmlIndexSectionName.value() + ".html") : config.htmlReportFile.value());
	}
	else
	{
		auto defaultReportFile = config.executableFolderPath / "Report" / (config.isUsingHtmlIndex ? config.htmlIndexSectionName.value() : "Results.html");
		reportFilePath = defaultReportFile;
	}
	return reportFilePath;
}

//----------------------------------------------------------------------------------------
static void BuildTestMenu(Configuration& config, std::atomic<bool>& burnInTest, TestManager& testManager, IUnitTest* test, MenuItem* currentMenu, const std::string& pathLeft)
{
	auto findSlash = pathLeft.find('/');
	if (findSlash == std::string::npos)
	{
		// There is no slash, this is the end
		MenuItem item;
		item.onClick = [&, test] {
			std::thread([&, test]() {
				do
				{
					testManager.ExecuteTest(test, GetReportFilePath(config));
				} while (burnInTest);
			}).detach();
		};
		item.caption = pathLeft;

		// Check for duplicate tests
#ifdef _DEBUG
		{
			auto* currentTest = currentMenu->Find(pathLeft);
			ASSERT(currentTest == nullptr);
		}
#endif

		currentMenu->PushBack(std::move(item));
	}
	else
	{
		auto left = pathLeft.substr(0, findSlash);
		auto right = pathLeft.substr(findSlash + 1);

		auto* child = currentMenu->Find(left);
		if (child == nullptr)
		{
			// Create a new container
			child = &currentMenu->EmplaceBack();
			child->caption = left;
		}
		else
		{
			// This should be a parent menu item with children, not one with an action.
			ASSERT(child->onClick == nullptr);
		}

		BuildTestMenu(config, burnInTest, testManager, test, child, right);
	}
}

//----------------------------------------------------------------------------------------
static bool LoadRendererPlugin(cobalt::logging::ILogger& log, const std::filesystem::path& filePath, std::vector<cobalt::graphics::RendererPlugin>& rendererPluginList)
{
#ifdef _WIN32
	// Don't block the process with a modal error dialog on dll load errors. Ideally this has already been set, but if
	// not, we set it temporarily for the calling thread.
	DWORD oldErrorMode = 0;
	SetThreadErrorMode(SEM_FAILCRITICALERRORS, &oldErrorMode);

	// Load the target assembly into the process
	HMODULE moduleHandle = LoadLibraryExW(filePath.wstring().c_str(), nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);

	// Undo the SetThreadErrorMode call and return the previous settings
	SetThreadErrorMode(oldErrorMode, nullptr);

	// If we failed to load the target module, abort any further processing.
	if (moduleHandle == nullptr)
	{
		log.Warning("Error loading assembly \"{0}\"! LoadLibrary failed with error code {1}", filePath, GetLastError());
		return false;
	}

	// Obtain a pointer to the Cobalt API version function for the assembly. If the function isn't found, we assume this
	// assembly isn't a renderer plugin, and silently return false.
	auto getCobaltAPIVersionFunction = reinterpret_cast<void (*)(unsigned int&, unsigned int&, unsigned int&)>(GetProcAddress(moduleHandle, "GetCobaltAPIVersion"));
	if (getCobaltAPIVersionFunction == nullptr)
	{
		FreeLibrary(moduleHandle);
		return false;
	}

	// Ensure the assembly uses the same API version as we're consuming. Our API can actually preserve ABI stability on
	// all platforms if we choose to, meaning we could deploy new renderer plugins containing new API features without
	// needing to recompile the program they're used in, as long as we add new functions to the end of our interface and
	// use "thunks" where new overloads are added, but for the time being we're not shipping with this forwards
	// compatibility guarantee, so we require both major and minor numbers to match. In the future, if we ship with
	// forwards compatibility, we'll only require the major number to match, indicating a breaking change. We don't need
	// the patch number to match in either case though, as patch releases must not contain any API changes.
	unsigned int pluginCobaltApiVersionMajor;
	unsigned int pluginCobaltApiVersionMinor;
	unsigned int pluginCobaltApiVersionPatch;
	getCobaltAPIVersionFunction(pluginCobaltApiVersionMajor, pluginCobaltApiVersionMinor, pluginCobaltApiVersionPatch);
	if ((pluginCobaltApiVersionMajor != COBALT_RENDERER_API_VERSION_MAJOR) || (pluginCobaltApiVersionMinor != COBALT_RENDERER_API_VERSION_MINOR))
	{
		log.Warning("Incompatible Cobalt API used in assembly \"{0}\"! An API version of {1}.{2} was found, but {3}.{4} is required.", filePath.wstring(), pluginCobaltApiVersionMajor, pluginCobaltApiVersionMinor, COBALT_RENDERER_API_VERSION_MAJOR, COBALT_RENDERER_API_VERSION_MINOR);
		FreeLibrary(moduleHandle);
		return false;
	}

	// Obtain a pointer to the interface function for the assembly. We expect this to exist at this point, since this
	// appears to be a Cobalt renderer plugin.
	auto getRendererPluginFunction = reinterpret_cast<IRendererPlugin::GetRendererPluginFunctionType*>(GetProcAddress(moduleHandle, "GetRendererPlugin"));
	if (getRendererPluginFunction == nullptr)
	{
		log.Warning("Failed to locate GetRendererPlugin function for assembly \"{0}\"!", filePath.wstring());
		FreeLibrary(moduleHandle);
		return false;
	}

	// Load information on each renderer plugin provided by this assembly
	log.Debug("Loading renderer plugin assembly \"{0}\"", filePath.native());
	unsigned int currentRendererIndex = 0;
	cobalt::graphics::RendererPlugin rendererPlugin;
	while (getRendererPluginFunction(currentRendererIndex++, rendererPlugin))
	{
		HMODULE moduleHandleDuplicate;
		GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, reinterpret_cast<LPCWSTR>(getRendererPluginFunction), &moduleHandleDuplicate);
		rendererPlugin.SetModuleHandle(ModuleHandleWin32::Create(moduleHandleDuplicate));
		log.Debug(R"(Found renderer plugin with name "{0}" and display name "{1}")", rendererPlugin.GetName().Get(), rendererPlugin.GetDisplayName().Get());
		rendererPluginList.push_back(std::move(rendererPlugin));
		rendererPlugin = cobalt::graphics::RendererPlugin();
	}

	// Release the assembly handle. We created duplicate handles for each renderer plugin above using GetModuleHandleEx,
	// so this won't unload the assembly unless there were no renderer plugins defined by the assembly.
	FreeLibrary(moduleHandle);
#else
	// Load the target assembly into the process
	auto filePathRaw = filePath.c_str();
	auto moduleHandle = dlopen(filePathRaw, RTLD_NOW);
	auto lastError = dlerror();
	if (moduleHandle == nullptr)
	{
		log.Warning("Error loading assembly \"{0}\": {1}!", filePathRaw, lastError);
		return false;
	}

	// Obtain a pointer to the Cobalt API version function for the assembly. If the function isn't found, we assume this
	// assembly isn't a renderer plugin, and silently return false.
	dlerror();
	auto getCobaltAPIVersionFunction = reinterpret_cast<void (*)(unsigned int&, unsigned int&, unsigned int&)>(dlsym(moduleHandle, "GetCobaltAPIVersion"));
	lastError = dlerror();
	if (lastError != nullptr)
	{
		dlclose(moduleHandle);
		return false;
	}

	// Ensure the assembly uses the same API version as we're consuming. Our API can actually preserve ABI stability on
	// all platforms if we choose to, meaning we could deploy new renderer plugins containing new API features without
	// needing to recompile the program they're used in, as long as we add new functions to the end of our interface and
	// use "thunks" where new overloads are added, but for the time being we're not shipping with this forwards
	// compatibility guarantee, so we require both major and minor numbers to match. In the future, if we ship with
	// forwards compatibility, we'll only require the major number to match, indicating a breaking change. We don't need
	// the patch number to match in either case though, as patch releases must not contain any API changes.
	unsigned int pluginCobaltApiVersionMajor;
	unsigned int pluginCobaltApiVersionMinor;
	unsigned int pluginCobaltApiVersionPatch;
	getCobaltAPIVersionFunction(pluginCobaltApiVersionMajor, pluginCobaltApiVersionMinor, pluginCobaltApiVersionPatch);
	if ((pluginCobaltApiVersionMajor != COBALT_RENDERER_API_VERSION_MAJOR) || (pluginCobaltApiVersionMinor != COBALT_RENDERER_API_VERSION_MINOR))
	{
		log.Warning("Incompatible Cobalt API used in assembly \"{0}\"! An API version of {1}.{2} was found, but {3}.{4} is required.", filePath.native(), pluginCobaltApiVersionMajor, pluginCobaltApiVersionMinor, COBALT_RENDERER_API_VERSION_MAJOR, COBALT_RENDERER_API_VERSION_MINOR);
		dlclose(moduleHandle);
		return false;
	}

	// Obtain a pointer to the interface function for the assembly. We expect this to exist at this point, since this
	// appears to be a Cobalt renderer plugin.
	dlerror();
	auto getRendererPluginFunction = reinterpret_cast<IRendererPlugin::GetRendererPluginFunctionType*>(dlsym(moduleHandle, "GetRendererPlugin"));
	lastError = dlerror();
	if (lastError != nullptr)
	{
		log.Warning("Failed to locate GetRendererPlugin function for assembly \"{0}\"!", filePath.native());
		dlclose(moduleHandle);
		return false;
	}

	// Load information on each renderer plugin provided by this assembly
	log.Debug("Loading renderer plugin assembly \"{0}\"", filePath);
	unsigned int currentRendererIndex = 0;
	cobalt::graphics::RendererPlugin rendererPlugin;
	while (getRendererPluginFunction(currentRendererIndex++, rendererPlugin))
	{
		rendererPlugin.SetModuleHandle(ModuleHandlePosix::Create(dlopen(filePath.c_str(), RTLD_NOW), filePath.string()));
		log.Debug(R"(Found renderer plugin with name "{0}" and display name "{1}")", rendererPlugin.GetName().Get(), rendererPlugin.GetDisplayName().Get());
		rendererPluginList.push_back(std::move(rendererPlugin));
		rendererPlugin = cobalt::graphics::RendererPlugin();
	}

	// Release the assembly handle. We created duplicate handles for each renderer plugin above, so this won't unload
	// the assembly unless there were no renderer plugins defined by the assembly.
	dlclose(moduleHandle);
#endif

	// Return true if we found at least one renderer plugin
	return !rendererPluginList.empty();
}

//----------------------------------------------------------------------------------------
static bool FindRendererPlugins(cobalt::logging::ILogger& log, const std::filesystem::path& pluginDirectoryPath, std::vector<cobalt::graphics::RendererPlugin>& rendererPluginList)
{
	// Attempt to create a directory enumerator in the target directory
	std::error_code errorCode;
	std::filesystem::directory_iterator directoryIterator(pluginDirectoryPath, errorCode);
	if (errorCode)
	{
		log.Error("Failed to enumerate contents of plugin directory \"{0}\": {1}", pluginDirectoryPath.string(), errorCode.message());
		return false;
	}

	// Build a list of all possible plugins in the target folder
	std::list<std::filesystem::path> pluginPaths;
	std::filesystem::directory_iterator directoryIteratorEnd;
	while (directoryIterator != directoryIteratorEnd)
	{
		// Retrieve the target file path
		const auto& filePath = *directoryIterator;
#ifdef _WIN32
		if (_wcsicmp(filePath.path().extension().c_str(), L".dll") == 0)
#elif defined(__APPLE__)
		if (strcasecmp(filePath.path().extension().c_str(), ".dylib") == 0)
#else
		if (strcasecmp(filePath.path().extension().c_str(), ".so") == 0)
#endif
		{
			pluginPaths.push_back(filePath.path());
		}

		// Advance to the next file path
		directoryIterator.increment(errorCode);
		if (errorCode)
		{
			log.Error("Failed to advance directory iterator for plugin directory \"{0}\": {1}", pluginDirectoryPath.string(), errorCode.message());
			return false;
		}
	}

	// Attempt to load all possible plugins found in the target path
	for (const auto& pluginPath : pluginPaths)
	{
		if (!LoadRendererPlugin(log, pluginPath, rendererPluginList))
		{
			continue;
		}
	}
	return true;
}

//----------------------------------------------------------------------------------------
enum class LogMessageAction
{
	Nothing = 0,
	Break,
	BreakOnce,
	BreakIfDebuggerAttached,
	BreakOnceIfDebuggerAttached,
	Abort,
	Terminate,
	QuickExit,
};

//----------------------------------------------------------------------------------------
static cobalt::logging::ILogTarget::unique_ptr CreateLogTargetBreakOnError(LogMessageAction action)
{
	auto callbackFunction = [=](const std::string& scope, cobalt::logging::ILogger::Severity severity, const std::string& message) {
		switch (action)
		{
		case LogMessageAction::Break:
			cobalt::debug::Break();
			return;
		case LogMessageAction::BreakOnce:
			cobalt::debug::BreakOnce();
			return;
		case LogMessageAction::BreakIfDebuggerAttached:
			if (IsDebugBuildWithDebuggerAttached())
			{
				cobalt::debug::Break();
			}
			return;
		case LogMessageAction::BreakOnceIfDebuggerAttached:
			if (IsDebugBuildWithDebuggerAttached())
			{
				cobalt::debug::BreakOnce();
			}
			return;
		case LogMessageAction::Abort:
			std::abort();
		case LogMessageAction::Terminate:
			std::terminate();
		case LogMessageAction::QuickExit:
#ifdef __APPLE__
			// Still no support for std::quick_exit() on macos as of 2026.
			std::terminate();
#else
			std::quick_exit(1);
#endif
		}
	};
	return cobalt::logging::LogTargetCallback::Create(callbackFunction);
}

//----------------------------------------------------------------------------------------
static void BuildRequestedTestSet(Configuration& config, cobalt::logging::ILogger& log, std::vector<IUnitTest*>& testSet)
{
	testSet.clear();

	if (config.testsToRun)
	{
		std::vector<std::string> testsToRun = config.testsToRun.value();

		std::sort(testsToRun.begin(), testsToRun.end());

		for (auto* test : TestRegistry::GetAllRegisteredTests())
		{
			auto testFullName = test->TestFullName();
			if (std::binary_search(testsToRun.begin(), testsToRun.end(), testFullName))
			{
				testSet.push_back(test);
				testsToRun.erase(std::find(testsToRun.begin(), testsToRun.end(), testFullName));
			}
		}

		if (!testsToRun.empty())
		{
			std::cout << "Tests were not found!" << std::endl;

			for (auto& missing : testsToRun)
			{
				std::cout << missing << std::endl;
			}
		}
	}
	else if (config.testMustContain || config.testIgnoreIfContains || config.isTestingAll || config.runAllPerformanceTests)
	{
		for (auto* test : TestRegistry::GetAllRegisteredTests())
		{
			if (!config.isTestingAll && (test->GetType() == IUnitTest::Type::UnitTest))
			{
				continue;
			}

			if (!config.runAllPerformanceTests && (test->GetType() == IUnitTest::Type::PerformanceTest))
			{
				continue;
			}

			auto testFullName = test->TestFullName();
			if (config.testMustContain)
			{
				if (!CaseInsensitiveContains(testFullName, *config.testMustContain))
				{
					continue;
				}
			}

			if (config.testIgnoreIfContains)
			{
				if (CaseInsensitiveContains(testFullName, *config.testIgnoreIfContains))
				{
					continue;
				}
			}

			testSet.push_back(test);
		}
	}
}

//----------------------------------------------------------------------------------------
static std::filesystem::path GetProcessDirectory()
{
	// Get the path to the directory we're running from
#ifdef _WIN32
	wchar_t filePathBuffer[MAX_PATH];
	GetModuleFileNameW(GetModuleHandle(nullptr), &filePathBuffer[0], MAX_PATH);
	auto executablePath = std::filesystem::path(&filePathBuffer[0]);
#elif defined(__APPLE__)
	uint32_t filePathLength = 0;
	_NSGetExecutablePath(nullptr, &filePathLength);
	std::vector<char> filePathBuffer(filePathLength + 1, 0);
	// Note that _NSGetExecutablePath does NOT null terminate the output, nor does it update filePathLength when it
	// succeeds.
	_NSGetExecutablePath(filePathBuffer.data(), &filePathLength);
	auto executablePath = std::filesystem::path(std::string(filePathBuffer.data(), filePathLength));
#else
	char filePathBuffer[PATH_MAX];
	ssize_t filePathLengthInChars = readlink("/proc/self/exe", &filePathBuffer[0], PATH_MAX - 1);
	filePathBuffer[filePathLengthInChars] = 0;
	auto executablePath = std::filesystem::path(filePathBuffer);
#endif
	return executablePath.parent_path();
}

//----------------------------------------------------------------------------------------
static void PrependVKLayerPath(const std::filesystem::path& newPath)
{
#ifdef _WIN32
	const wchar_t separator = L';';
	const wchar_t* envVarW = L"VK_LAYER_PATH";

	// Convert UTF-8 filesystem::path to UTF-16
	std::wstring newPathW = newPath.wstring();

	DWORD requiredSize = GetEnvironmentVariableW(envVarW, nullptr, 0);
	std::wstring newValue = newPathW;

	if (requiredSize > 0)
	{
		std::wstring existing(requiredSize, L'\0');
		GetEnvironmentVariableW(envVarW, existing.data(), requiredSize);

		// Remove trailing null inserted by GetEnvironmentVariableW
		if (!existing.empty() && existing.back() == L'\0')
		{
			existing.pop_back();
		}

		if (!existing.empty())
		{
			newValue += separator;
			newValue += existing;
		}
	}

	SetEnvironmentVariableW(envVarW, newValue.c_str());

#else
	const char separator = ':';
	const char* envVarA = "VK_LAYER_PATH";

	std::string newPathUtf8 = newPath.string();
	const char* existing = std::getenv(envVarA);

	std::string newValue = newPathUtf8;
	if ((existing != nullptr) && (*existing != 0))
	{
		newValue += separator;
		newValue += existing;
	}

	setenv(envVarA, newValue.c_str(), 1);
#endif
}

//----------------------------------------------------------------------------------------
static cobalt::logging::ILogger* UnhandledExceptionLogger = nullptr;
#ifdef _WIN32
static LONG WINAPI UnhandledExceptionHandler(LPEXCEPTION_POINTERS ep)
{
	// Log an error about this exception
	if (UnhandledExceptionLogger != nullptr)
	{
		UnhandledExceptionLogger->Critical("Unhandled exception:\n{0}", cobalt::debug::CrashHandler::GenerateExceptionReport(ep));
	}

	// Bypass the standard windows crash report dialog, and run the exception handler, which will terminate the process.
	return EXCEPTION_EXECUTE_HANDLER;
}

#else
#ifdef __APPLE__
//----------------------------------------------------------------------------------------
static void UnhandledExceptionHandler(NSException* ex)
{
	// Log an error about this exception
	if (UnhandledExceptionLogger != nullptr)
	{
		std::string exceptionReport;
		if (ex.name != nullptr)
		{
			exceptionReport += "name   = " + std::string([ex.name UTF8String]) + "\n";
		}
		if (ex.reason != nullptr)
		{
			exceptionReport += "reason = " + std::string([ex.reason UTF8String]) + "\n";
		}
		if ((ex.userInfo != nullptr) && (ex.userInfo.description != nullptr))
		{
			exceptionReport += "info   = " + std::string([ex.userInfo.description UTF8String]) + "\n";
		}
		if (ex.callStackSymbols != nullptr)
		{
			exceptionReport += "stack  = " + std::string([[ex.callStackSymbols componentsJoinedByString:@"\n"] UTF8String]) + "\n";
		}
		UnhandledExceptionLogger->Critical("Unhandled exception:\n{0}", exceptionReport);
	}
}
#endif

//----------------------------------------------------------------------------------------
static std::string GenerateStackTrace(int skipFrames = 0)
{
	constexpr int MaxFrames = 64;
	void* frames[MaxFrames];

	int frameCount = backtrace(&frames[0], MaxFrames);
	char** symbols = backtrace_symbols(&frames[0], frameCount);

	std::ostringstream oss;
	for (int i = skipFrames; i < frameCount; ++i)
	{
		oss << symbols[i] << '\n';
	}

	std::free(symbols); // NOLINT(cppcoreguidelines-no-malloc,hicpp-no-malloc)
	return oss.str();
}

//----------------------------------------------------------------------------------------
static void FatalSignalHandler(int sig, siginfo_t* info, void* /*context*/)
{
	if (UnhandledExceptionLogger != nullptr)
	{
		std::string report;

		report += "signal = ";
		report += strsignal(sig);
		report += "\n";

		if (sig == SIGSEGV)
		{
			report += "fault address = ";
			report += std::to_string(reinterpret_cast<uintptr_t>(info->si_addr));
			report += "\n";
		}

		report += "stack  = \n";
		report += GenerateStackTrace(2);

		UnhandledExceptionLogger->Critical("Unhandled exception:\n{0}", report);
	}

	// Restore default handler and re-raise to get proper exit code / core dump
	(void)signal(sig, SIG_DFL);
	(void)raise(sig);
}

//----------------------------------------------------------------------------------------
static void InstallSignalHandlers()
{
	struct sigaction sa = {};
	sa.sa_sigaction = FatalSignalHandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_SIGINFO | SA_RESETHAND;

	sigaction(SIGSEGV, &sa, nullptr);
	sigaction(SIGABRT, &sa, nullptr);
	sigaction(SIGFPE, &sa, nullptr);
	sigaction(SIGILL, &sa, nullptr);
	sigaction(SIGBUS, &sa, nullptr);
}

//----------------------------------------------------------------------------------------
static void UnhandledTerminateHandler()
{
	if (UnhandledExceptionLogger != nullptr)
	{
		std::string report = "Unhandled C++ exception\n";

		if (auto eptr = std::current_exception())
		{
			try
			{
				std::rethrow_exception(eptr);
			}
			catch (const std::exception& ex)
			{
				report += std::string("what() = ") + ex.what() + "\n";
			}
			catch (...)
			{
				report += "what() = <non-std exception>\n";
			}
		}

		report += "stack  = \n";
		report += GenerateStackTrace(2);

		UnhandledExceptionLogger->Critical("Unhandled exception:\n{0}", report);
	}

	std::_Exit(EXIT_FAILURE); // async-signal-safe termination
}
#endif

//----------------------------------------------------------------------------------------
// GPU integrated/discrete preference control
//----------------------------------------------------------------------------------------
#ifdef _WIN32
// Various documented tricks on Windows to try and force OpenGL to use a discrete graphics device over an integrated
// one. This just sets our defaults though, as of Windows 10 version 1803 from 2018, registry settings can now override
// these and can take effect at runtime, so we just set these as a fallback.
extern "C" __declspec(dllexport) const int AmdPowerXpressRequestHighPerformance;
extern "C" __declspec(dllexport) const unsigned int NvOptimusEnablement;
extern "C" const int AmdPowerXpressRequestHighPerformance = 1;
extern "C" const unsigned int NvOptimusEnablement = 1;

//----------------------------------------------------------------------------------------
static void SetGpuPreference(bool forceDiscrete)
{
	// Obtain the path of the current executable
	wchar_t filePathBuffer[MAX_PATH];
	GetModuleFileNameW(GetModuleHandle(nullptr), &filePathBuffer[0], MAX_PATH);
	auto executablePath = std::filesystem::path(&filePathBuffer[0]);

	// Set the GPU preference. This takes effect as long as we set it prior to initializing graphics.
	// Preference 1 = power saving / integrated
	// Preference 2 = high performance / discrete
	HKEY key;
	RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\DirectX\\UserGpuPreferences", 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr);
	std::wstring value = (forceDiscrete ? L"GpuPreference=2;" : L"GpuPreference=1;");
	RegSetValueExW(key, executablePath.wstring().c_str(), 0, REG_SZ, reinterpret_cast<const BYTE*>(value.c_str()), (DWORD)((value.length() + 1) * sizeof(wchar_t)));
	RegCloseKey(key);
}

//----------------------------------------------------------------------------------------
static void ClearGpuPreference()
{
	// Obtain the path of the current executable
	wchar_t filePathBuffer[MAX_PATH];
	GetModuleFileNameW(GetModuleHandle(nullptr), &filePathBuffer[0], MAX_PATH);
	auto executablePath = std::filesystem::path(&filePathBuffer[0]);

	// Remove the key setting the GPU affinity if present
	HKEY key;
	if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\DirectX\\UserGpuPreferences", 0, KEY_SET_VALUE, &key) == ERROR_SUCCESS)
	{
		RegDeleteValueW(key, executablePath.wstring().c_str());
		RegCloseKey(key);
	}
}
#endif

//----------------------------------------------------------------------------------------
#ifdef _WIN32
static std::vector<std::string> TokenizeCommandLine(const std::string& commandLine, bool stripFirstArgument = false, int literalRemainderAfterArg = -1)
{
	// Strip off leading whitespace from the start of the command line string
	std::string::const_iterator commandLineIterator = commandLine.begin();
	while ((commandLineIterator != commandLine.end()) && (iswspace(*commandLineIterator) != 0))
	{
		++commandLineIterator;
	}

	// Tokenize the command line string into a set of arguments, taking into account the correct rules for quotation and
	// escape characters.
	std::vector<std::string> arguments;
	while ((commandLineIterator != commandLine.end()) && ((literalRemainderAfterArg < 0) || ((int)arguments.size() < literalRemainderAfterArg)))
	{
		std::string argument;
		int backslashCount = 0;
		bool whitespacePreservationActive = false;
		bool foundEndOfArgument = false;
		while (!foundEndOfArgument && (commandLineIterator != commandLine.end()))
		{
			// Retrieve the next character from the command line string
			auto character = *commandLineIterator++;
			bool characterIsBackslash = (character == '\\');
			bool characterIsDoubleQuote = (character == '\"');

			// If the character is a backslash, increment the current backslash count and advance to the next character.
			if (characterIsBackslash)
			{
				++backslashCount;
				continue;
			}

			// Process backslashes in the command line. The way they are processed is very strange, but we mimic the
			// standard Windows behaviour here. The rules are basically this: backslashes are preserved and passed
			// through as-is, unless a double quote character is found directly following one or more backslashes, in
			// which case all preceding backslashes in the block become escape characters, so each consecutive pair of
			// backslashes emits one backslash. If the double quote itself is escaped by an unmatched (odd) number of
			// backslashes preceding it, it is passed through literally as part of the argument. If the double quote is
			// preceded by an even number of backslashes however, it is active in either enabling or disabling the
			// whitespace escape mode for the current argument.
			if (backslashCount > 0)
			{
				// Insert the required number of leading backslashes before the current character
				int backslashesToInsert = (characterIsDoubleQuote ? backslashCount / 2 : backslashCount);
				bool escapingDoubleQuote = characterIsDoubleQuote && ((backslashCount % 2) != 0);
				for (int i = 0; i < backslashesToInsert; ++i)
				{
					argument.push_back('\\');
				}
				backslashCount = 0;

				// If we're escaping a double quote, insert the character into the output argument and advance to the
				// next character.
				if (escapingDoubleQuote)
				{
					argument.push_back('\"');
					continue;
				}
			}

			// If we've encountered an unescaped double quote, toggle whitespace preservation mode, and advance to the
			// next character.
			if (characterIsDoubleQuote)
			{
				whitespacePreservationActive = !whitespacePreservationActive;
				continue;
			}

			// If whitespace preservation isn't active and we've encountered a whitespace character, flag that we've
			// reached the end of the current argument, and stop processing subsequent characters.
			if (!whitespacePreservationActive && (iswspace(character) != 0))
			{
				foundEndOfArgument = true;
				continue;
			}

			// Add this character to the current output argument
			argument.push_back(character);
		}

		// If the argument ended with one or more backslashes, append them to the parsed argument now. This case will
		// only occur here where the entire command line string ends with one or more backslashes.
		if (backslashCount > 0)
		{
			for (int i = 0; i < backslashCount; ++i)
			{
				argument.push_back('\\');
			}
		}

		// Add this separated argument to the set of arguments. Note that it is possible for this to be an empty string,
		// in particular if a pair of double quotes is followed by a whitespace character.
		arguments.push_back(argument);

		// Strip any trailing whitespace after this argument. This will either take us to the end of the command line or
		// the start of a new argument.
		while ((commandLineIterator != commandLine.end()) && (iswspace(*commandLineIterator) != 0))
		{
			++commandLineIterator;
		}
	}

	// If the literal remainder of the command line has been requested after a specified number of arguments are
	// extracted, and we reached the target argument count, append the remainder of the command line as an additional
	// argument without any processing.
	if ((literalRemainderAfterArg >= 0) && ((int)arguments.size() == literalRemainderAfterArg))
	{
		std::string commandLineRemainder;
		while (commandLineIterator != commandLine.end())
		{
			auto character = *commandLineIterator++;
			commandLineRemainder.push_back(character);
		}
		arguments.push_back(commandLineRemainder);
	}

	// If we've been requested to strip the first argument and at least one argument was found, remove the leading
	// argument now.
	if (stripFirstArgument && !arguments.empty())
	{
		arguments.erase(arguments.begin());
	}

	// Return the tokenized set of command line arguments to the caller
	return arguments;
}
#endif

//----------------------------------------------------------------------------------------
int main(int argc, const char* argv[])
{
	// Initialize Xlib for multithreaded access
#ifdef COBALT_RENDERER_XLIB_SUPPORT
	XInitThreads();
#endif

	// Prepend the "Renderers" subdirectory to VK_LAYER_PATH so we can find the Vulkan validation layer without relying
	// on an installed Vulkan SDK with environment variables set.
	std::filesystem::path rendererDir = GetProcessDirectory() / "Renderers";
	PrependVKLayerPath(rendererDir);

	// Create the log manager
	cobalt::logging::LogManager logManager;
	logManager.SetIncludeSeverity(cobalt::logging::ILogger::SeverityFilter::All);
	auto log = logManager.GetLogger("");

	// Create a console window log target
	auto consoleLogTarget = cobalt::logging::LogTargetStandardOut::Create(true);
	logManager.AddLogTarget(std::move(consoleLogTarget));

	// Retrieve and tokenize the command line arguments
#ifdef _WIN32
	std::string originalCommandLine = UTF16ToUTF8(GetCommandLineW());
	std::vector<std::string> commandLineArguments = TokenizeCommandLine(originalCommandLine, true);
#else
	std::vector<std::string> commandLineArguments(argv + 1, argv + argc);
#endif

	// Parse our command line arguments
	Configuration config;
	std::atomic<bool> burnInTest = false;
	std::vector<std::string> switchPrefixes = {"/", "--", "-"};
	auto stringStartsWith = [](const std::string& str, const std::string& prefix) { return (str.size() >= prefix.size()) && (str.compare(0, prefix.size(), prefix) == 0); };
	auto stringToLower = [](const std::string& str) { auto stringConverted = str; std::transform(stringConverted.begin(), stringConverted.end(), stringConverted.begin(), [](char c) { return (char)std::tolower((unsigned char)c); }); return stringConverted; };
	size_t nextArgumentIndex = 0;
	while (nextArgumentIndex < commandLineArguments.size())
	{
		// Determine if the next command line parameter is a switch or a parameter.
		auto argument = commandLineArguments[nextArgumentIndex++];
		bool argumentIsSwitch = false;
		for (const auto& prefix : switchPrefixes)
		{
			if (stringStartsWith(argument, prefix))
			{
				argumentIsSwitch = true;
				argument = argument.substr(prefix.size());
				break;
			}
		}

		// Process the next command line argument
		auto remainingArgumentCount = commandLineArguments.size() - nextArgumentIndex;
		if (argumentIsSwitch)
		{
			bool missingRequiredArguments = false;
			size_t requiredArgumentCount = 0;
			auto argumentLowerCase = stringToLower(argument);
			if ((argumentLowerCase == "help") || (argumentLowerCase == "h") || (argumentLowerCase == "?"))
			{
				config.isHelp = true;
			}
			else if ((argumentLowerCase == "no-prompt") || (argumentLowerCase == "p"))
			{
				config.isNoPrompt = true;
			}
			else if (argumentLowerCase == "use-integrated")
			{
				config.isForceIntegrated = true;
			}
			else if (argumentLowerCase == "use-software")
			{
				config.isForceSoftware = true;
			}
			else if (argumentLowerCase == "use-discrete")
			{
				config.isForceDiscrete = true;
			}
			else if ((argumentLowerCase == "no-interface") || (argumentLowerCase == "i"))
			{
				config.isNoInterface = true;
			}
			else if ((argumentLowerCase == "kill-afterwards") || (argumentLowerCase == "k"))
			{
				config.isKillAfterTest = true;
			}
			else if (argumentLowerCase == "list-tests")
			{
				config.isListingTests = true;
			}
			else if ((argumentLowerCase == "wait") || (argumentLowerCase == "w"))
			{
				config.isWaitingForDebugger = true;
			}
			else if ((argumentLowerCase == "test-all") || (argumentLowerCase == "a"))
			{
				config.isTestingAll = true;
			}
			else if (argumentLowerCase == "performance-all")
			{
				config.runAllPerformanceTests = true;
			}
			else if (argumentLowerCase == "api-validation")
			{
				config.isApiValidation = true;
			}
			else if (argumentLowerCase == "append-multi-report")
			{
				requiredArgumentCount = 1;
				missingRequiredArguments = (remainingArgumentCount < requiredArgumentCount);
				if (!missingRequiredArguments)
				{
					config.htmlIndexSectionName = commandLineArguments[nextArgumentIndex++];
					config.isUsingHtmlIndex = true;
				}
			}
			else if (argumentLowerCase == "demo")
			{
				config.isDemo = true;
			}
			else if (argumentLowerCase == "update-data")
			{
				config.isUpdateData = true;
			}
			else if (argumentLowerCase == "update-data-no-prompt")
			{
				config.isUpdateDataNoPrompt = true;
			}
			else if (argumentLowerCase == "update-all")
			{
				config.isUpdateDataAll = true;
			}
			else if ((argumentLowerCase == "engine") || (argumentLowerCase == "e"))
			{
				requiredArgumentCount = 1;
				missingRequiredArguments = (remainingArgumentCount < requiredArgumentCount);
				if (!missingRequiredArguments)
				{
					config.engine = commandLineArguments[nextArgumentIndex++];
				}
			}
			else if ((argumentLowerCase == "card") || (argumentLowerCase == "c"))
			{
				requiredArgumentCount = 1;
				missingRequiredArguments = (remainingArgumentCount < requiredArgumentCount);
				if (!missingRequiredArguments)
				{
					config.card = commandLineArguments[nextArgumentIndex++];
				}
			}
			else if (argumentLowerCase == "window-system")
			{
				requiredArgumentCount = 1;
				missingRequiredArguments = (remainingArgumentCount < requiredArgumentCount);
				if (!missingRequiredArguments)
				{
					config.windowSystem = commandLineArguments[nextArgumentIndex++];
				}
			}
			else if ((argumentLowerCase == "log-file") || (argumentLowerCase == "l"))
			{
				requiredArgumentCount = 1;
				missingRequiredArguments = (remainingArgumentCount < requiredArgumentCount);
				if (!missingRequiredArguments)
				{
					config.logFile = commandLineArguments[nextArgumentIndex++];
				}
			}
			else if (argumentLowerCase == "log-level")
			{
				requiredArgumentCount = 1;
				missingRequiredArguments = (remainingArgumentCount < requiredArgumentCount);
				if (!missingRequiredArguments)
				{
					config.logVerbosity = std::stoi(commandLineArguments[nextArgumentIndex++]);
				}
			}
			else if (argumentLowerCase == "html-report")
			{
				requiredArgumentCount = 1;
				missingRequiredArguments = (remainingArgumentCount < requiredArgumentCount);
				if (!missingRequiredArguments)
				{
					config.htmlReportFile = commandLineArguments[nextArgumentIndex++];
					config.isGeneratingReport = true;
				}
			}
			else if (argumentLowerCase == "pass-fail")
			{
				requiredArgumentCount = 1;
				missingRequiredArguments = (remainingArgumentCount < requiredArgumentCount);
				if (!missingRequiredArguments)
				{
					config.passFailFile = commandLineArguments[nextArgumentIndex++];
				}
			}
			else if (argumentLowerCase == "reference-dir")
			{
				requiredArgumentCount = 1;
				missingRequiredArguments = (remainingArgumentCount < requiredArgumentCount);
				if (!missingRequiredArguments)
				{
					config.referenceDir = commandLineArguments[nextArgumentIndex++];
				}
			}
			else if ((argumentLowerCase == "test") || (argumentLowerCase == "t"))
			{
				requiredArgumentCount = 1;
				missingRequiredArguments = (remainingArgumentCount < requiredArgumentCount);
				if (!missingRequiredArguments)
				{
					if (!config.testsToRun)
					{
						config.testsToRun.emplace();
					}
					config.testsToRun.value().push_back(commandLineArguments[nextArgumentIndex++]);
				}
			}
			else if ((argumentLowerCase == "renderer") || (argumentLowerCase == "r"))
			{
				requiredArgumentCount = 1;
				missingRequiredArguments = (remainingArgumentCount < requiredArgumentCount);
				if (!missingRequiredArguments)
				{
					if (!config.pluginsToLoad)
					{
						config.pluginsToLoad.emplace();
					}
					config.pluginsToLoad.value().push_back(commandLineArguments[nextArgumentIndex++]);
				}
			}
			else if ((argumentLowerCase == "directory") || (argumentLowerCase == "d"))
			{
				requiredArgumentCount = 1;
				missingRequiredArguments = (remainingArgumentCount < requiredArgumentCount);
				if (!missingRequiredArguments)
				{
					if (!config.dllSearchDirs)
					{
						config.dllSearchDirs.emplace();
					}
					config.dllSearchDirs.value().push_back(commandLineArguments[nextArgumentIndex++]);
				}
			}
			else if (argumentLowerCase == "test-contains")
			{
				requiredArgumentCount = 1;
				missingRequiredArguments = (remainingArgumentCount < requiredArgumentCount);
				if (!missingRequiredArguments)
				{
					config.testMustContain = commandLineArguments[nextArgumentIndex++];
				}
			}
			else if (argumentLowerCase == "ignore")
			{
				requiredArgumentCount = 1;
				missingRequiredArguments = (remainingArgumentCount < requiredArgumentCount);
				if (!missingRequiredArguments)
				{
					config.testIgnoreIfContains = commandLineArguments[nextArgumentIndex++];
				}
			}
			else if (argumentLowerCase == "on-error")
			{
				requiredArgumentCount = 1;
				missingRequiredArguments = (remainingArgumentCount < requiredArgumentCount);
				if (!missingRequiredArguments)
				{
					config.onErrorBehavior = commandLineArguments[nextArgumentIndex++];
				}
			}
			else
			{
				log->Warning("Unrecognized command line option {0}", argument);
			}
			if (missingRequiredArguments)
			{
				log->Error("Missing required arguments for option \"{0}\". {1} arguments were required, but {2} were provided.", argument, requiredArgumentCount, remainingArgumentCount);
			}
		}
		else
		{
			log->Warning("Unrecognized command line parameter {0}", argument);
		}
	}

	// If command line help has been requested, display it, and abort any further processing.
	if (config.isHelp)
	{
		std::cout << R"qwerty(Device selection:
  -r [ --renderer ] arg      The full path to a renderer plugin to load. Can
                             be specified more than once. Using this argument
                             will disable the automatic loading of plugins.
  -d [ --directory ] arg     The full path to a folder to add to dll search
                             paths when resolving dependent dlls for render
                             plugins. Can be specified more than once.
  -p [ --no-prompt ]         Don't prompt for card / engine to use. Do best
                             guess or use hints.
  -e [ --engine ] arg        Graphics engine to use.
  -c [ --card ] arg          Card to use
  --use-integrated           Try to use integrated graphics otherwise abort
  --use-software             Try to use sortware graphics otherwise abort
  --use-discrete             Try to use discrete graphics otherwise abort
  --api-validation           Enable renderer API validation, debug logging
                             and render markers when supported.

Interface configuration:
  -h [ --help ]              Prints this help message and exit
  -i [ --no-interface ]      Don't show interface. Use when doing automatic
                             regression testing.
  --window-system arg        Window system to use [Auto, Headless, Win32,
                             Xlib, XCB, AppKit]
  --list-tests               Prints a list of matching tests and exit, rather
                             than run them.
  -k [ --kill-afterwards ]   Close the interface after finishing the tests.
  -l [ --log-file ] arg      Log debug information to a given log file.
  --log-level arg            Verbosity of information in the log
  --html-report arg          Write a html report of the test progress
  --append-multi-report arg  Append to a multi-session report with section name
                             of <arg>
  --pass-fail arg            Write PASS or FAILED to a file on test
                             completion. We can't rely on the return code for
                             regression testing, as some abnormal terminations
                             can give return code 0, and we can have a non-0
                             return code yet not complete.
  -w [ --wait ]              Wait until a debugger is attached before
                             initializing.
  --on-error arg             Behavior when an error occurs [Ignore, Break,
                             Abort]

Test selection:
  -t [ --test ] arg          Test(s) to run. Category/SubCategory/TestName. Can
                             give multiple
  --test-contains arg        Run tests whose Category/SubCategory/TestName
                             string containing only this string
  --ignore arg               Don't run tests whose Category/SubCategory/TestNam
                             e string containing only this string
  -a [ --test-all ]          Run all unit tests
  --performance-all          Run all performance tests

Reference data:
  --reference-dir arg        Where the reference data for tests are located
  --demo                     Don't use reference data, even if found. This
                             becomes a demo app, not a test framework.
  --update-data              Update data for failed tests, prompting for each
  --update-data-no-prompt    Update data for failed tests without prompting
  --update-all               Update data for all run tests without prompting
)qwerty";
		return 0;
	}

	// Get the path to the directory we're running from
	auto executableFolderPath = GetProcessDirectory();
	config.executableFolderPath = executableFolderPath;

	// Validate the reference directory path
	if (config.referenceDir && config.referenceDir.value().is_relative())
	{
		std::error_code errorCode;
		config.referenceDir = std::filesystem::absolute(*config.referenceDir, errorCode);
		if (!std::filesystem::is_directory(*config.referenceDir))
		{
			if (!std::filesystem::create_directories(*config.referenceDir, errorCode))
			{
				log->Error("Failed to locate or create reference directory {0}", *config.referenceDir);
				return 1;
			}
		}
	}

	// Add a log file target if requested
	if (config.logFile)
	{
		std::error_code errorCode;
		auto parentFolder = config.logFile->parent_path();
		if (!std::filesystem::is_directory(parentFolder, errorCode))
		{
			if (!std::filesystem::create_directories(parentFolder, errorCode))
			{
				log->Error("Failed to create directory tree for log file {0}", *config.logFile);
				return 1;
			}
		}

		// Create a file log target
		auto fileLogTarget = cobalt::logging::LogTargetFile::Create();
		if (fileLogTarget->OpenLogFile(config.logFile.value()))
		{
			logManager.AddLogTarget(std::move(fileLogTarget));
		}
		else
		{
			log->Error("Failed to open log file at path {0}", *config.logFile);
		}

		// Record that the logging system has been initialized.
		log->Debug("File logging initialized");
	}

	// Register handlers to log unhandled exceptions
	UnhandledExceptionLogger = log.get();
#ifdef _WIN32
	SetUnhandledExceptionFilter(UnhandledExceptionHandler);
#else
#if defined(__APPLE__)
	NSSetUncaughtExceptionHandler(UnhandledExceptionHandler);
#endif
	std::set_terminate(UnhandledTerminateHandler);
	InstallSignalHandlers();
#endif

	// Configure our behaviour when an error is reported
	if (config.onErrorBehavior.has_value() && (*config.onErrorBehavior != "Ignore"))
	{
		if (*config.onErrorBehavior == "Break")
		{
			logManager.AddLogTarget(CreateLogTargetBreakOnError(LogMessageAction::Break), cobalt::logging::ILogger::SeverityFilter::CriticalOrHigher);
			logManager.AddLogTarget(CreateLogTargetBreakOnError(LogMessageAction::BreakOnceIfDebuggerAttached), cobalt::logging::ILogger::SeverityFilter::ErrorOrHigher);
		}
		else if (*config.onErrorBehavior == "Abort")
		{
			logManager.AddLogTarget(CreateLogTargetBreakOnError(LogMessageAction::Abort), cobalt::logging::ILogger::SeverityFilter::ErrorOrHigher);
		}
		else if (*config.onErrorBehavior == "QuickExit")
		{
			logManager.AddLogTarget(CreateLogTargetBreakOnError(LogMessageAction::QuickExit), cobalt::logging::ILogger::SeverityFilter::ErrorOrHigher);
		}
		else
		{
			log->Critical("Invalid value for --on-error (must be Abort/Ignore/Break/QuickExit)");
			return 1;
		}
	}

	// If we're a debug build, running from Visual Studio or some other debugging tool, allow errors to be easily
	// investigated by breaking when the first error occurs.
	if (!config.onErrorBehavior.has_value() && IsDebugBuildWithDebuggerAttached())
	{
		logManager.AddLogTarget(CreateLogTargetBreakOnError(LogMessageAction::BreakOnce), cobalt::logging::ILogger::SeverityFilter::ErrorOrHigher);
	}

	// If we're on windows, attempt to force switch between integrated/discrete graphics. This only applies to the
	// OpenGL renderer, as all other graphics APIs have proper structured device selection.
#ifdef _WIN32
	if (config.isForceDiscrete)
	{
		// Attempt to flag to NVidia devices that we want to use discrete graphics rather than integrated graphics by
		// loading a support library. This is as per the NVidia document "Enabling High Performance Graphics Rendering
		// on Optimus Systems", in the "Static Library Bindings" section. Whether the library is actually "statically
		// linked" or not isn't relevant or known to the graphics driver, the important thing is just that one of the
		// indicated libaries is loaded as a library into the process prior to calling into the OpenGL graphics driver
		// for the first time. We therefore attempt to load a library from the list of options into the process memory
		// space to force NVidia graphics here. This is overkill, as this is trumped by the registry settings we're
		// about to set below for Windows 10 versions, and we've already done our NvOptimusEnablement exported variable
		// as well, but for completeness we try this here too, showing all the various ways to force this selection in
		// code.
		LoadLibraryW(L"nvapi64.dll");

		// Set the GPU preference via registry settings
		SetGpuPreference(true);
	}
	else if (config.isForceIntegrated)
	{
		SetGpuPreference(false);
	}
	else
	{
		ClearGpuPreference();
	}
#endif

	// Handle our window system argument
	ITestSession::WindowSystem windowSystem;
	if (config.windowSystem.has_value() && (*config.windowSystem != "Auto"))
	{
		if (*config.windowSystem == "Headless")
		{
			windowSystem = ITestSession::WindowSystem::Headless;
		}
		else if (*config.windowSystem == "Win32")
		{
#ifdef _WIN32
			windowSystem = ITestSession::WindowSystem::Win32;
#else
			log->Error("Value \"{0}\" for --window-system unsupported on this platform", *config.windowSystem);
			return 1;
#endif
		}
		else if (*config.windowSystem == "Xlib")
		{
#ifdef __linux__
			windowSystem = ITestSession::WindowSystem::Xlib;
#else
			log->Error("Value \"{0}\" for --window-system unsupported on this platform", *config.windowSystem);
			return 1;
#endif
		}
		else if (*config.windowSystem == "XCB")
		{
#ifdef __linux__
			windowSystem = ITestSession::WindowSystem::XCB;
#else
			log->Error("Value \"{0}\" for --window-system unsupported on this platform", *config.windowSystem);
			return 1;
#endif
		}
		else if (*config.windowSystem == "AppKit")
		{
#ifdef __APPLE__
			windowSystem = ITestSession::WindowSystem::AppKit;
#else
			log->Error("Value \"{0}\" for --window-system unsupported on this platform", *config.windowSystem);
			return 1;
#endif
		}
		else
		{
			log->Critical("Invalid value for --window-system (must be Auto/Headless/Win32/Xlib/XCB/AppKit)");
			return 1;
		}
	}
	else
	{
#ifdef _WIN32
		windowSystem = ITestSession::WindowSystem::Win32;
#elif defined(__APPLE__)
		windowSystem = ITestSession::WindowSystem::AppKit;
#else
		windowSystem = ITestSession::WindowSystem::Xlib;
#endif
	}

	// If we've been requested to simply list all available tests, list them here and abort any further processing.
	if (config.isListingTests)
	{
		std::cout << "Unit tests:" << std::endl;
		for (auto* test : TestRegistry::GetAllRegisteredUnitTests())
		{
			std::cout << test->TestFullName() << std::endl;
		}
		std::cout << "Performance tests:" << std::endl;
		for (auto* test : TestRegistry::GetAllRegisteredPerformanceTests())
		{
			std::cout << test->TestFullName() << std::endl;
		}
		return 0;
	}

	// Wait for a debugger to be attached if requested
	if (config.isWaitingForDebugger)
	{
		log->Info("Waiting for debugger...");
		while (cobalt::debug::IsDebug() && !IsDebugBuildWithDebuggerAttached())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		log->Info("Debugger attached!");
	}

	// Validate and clean the list of registered tests
	TestRegistry::ValidateTestRegistry(*log);

	// Load the required set of renderer plugins
	std::vector<cobalt::graphics::RendererPlugin> rendererPluginList;
	if (config.pluginsToLoad)
	{
		// Build a list of all locations to search for our target list of plugins
		std::filesystem::path rendererPluginDir = executableFolderPath / "Renderers";
		std::list<std::filesystem::path> searchDirs;
		searchDirs.push_back(rendererPluginDir);
		if (config.dllSearchDirs)
		{
			for (const auto& searchDirectory : *config.dllSearchDirs)
			{
				searchDirs.emplace_back(UTF8ToNativeWideString(searchDirectory));
			}
		}

		// Attempt to load each specified target plugin
		for (const auto& pluginPath : *config.pluginsToLoad)
		{
			std::filesystem::path pluginPathResolved = pluginPath;
			bool loadedPlugin = false;
			if (pluginPathResolved.is_absolute())
			{
				loadedPlugin = LoadRendererPlugin(*log, pluginPathResolved, rendererPluginList);
			}
			else
			{
				for (const auto& searchDir : searchDirs)
				{
					std::filesystem::path pluginSearchPath = searchDir / pluginPathResolved;
					if (LoadRendererPlugin(*log, pluginSearchPath, rendererPluginList))
					{
						loadedPlugin = true;
						break;
					}
				}
			}
			if (!loadedPlugin)
			{
				log->Error("Failed to locate renderer plugin with path {0}.", pluginPath);
			}
		}
	}
	else
	{
		// Attempt to load all renderer plugins in the plugin directory
		std::filesystem::path rendererPluginDir = executableFolderPath / "Renderers";
		if (!FindRendererPlugins(*log, rendererPluginDir, rendererPluginList))
		{
			log->Critical("Failed to enumerate renderer plugins.");
			return 1;
		}
	}
	if (rendererPluginList.empty())
	{
		log->Critical("Failed to locate any renderer plugins.");
		return 1;
	}

	// Sort the array of renderers by display name
	std::sort(rendererPluginList.begin(), rendererPluginList.end(), [](const cobalt::graphics::RendererPlugin& first, const cobalt::graphics::RendererPlugin& second) { return first.GetDisplayName().Get() < second.GetDisplayName().Get(); });

	// Select the renderer plugin to use
	cobalt::graphics::RendererPlugin* activePlugin = nullptr;
	if (config.engine)
	{
		for (auto& plugin : rendererPluginList)
		{
			if (*config.engine == plugin.GetName())
			{
				activePlugin = &plugin;
				break;
			}
		}
		if (activePlugin == nullptr)
		{
			log->Critical("Couldn't find engine: {0}", *config.engine);
			return 1;
		}
	}
	else if (rendererPluginList.size() == 1)
	{
		activePlugin = rendererPluginList.data();
	}
	else if (!config.isNoPrompt)
	{
		// Select the renderer to use
		std::cout << "Please select the renderer to load:" << std::endl;
		for (size_t i = 0; i < rendererPluginList.size(); ++i)
		{
			std::cout << i + 1 << ". " << rendererPluginList[i].GetDisplayName() << std::endl;
		}

		unsigned int selectionNo;
		std::cin >> selectionNo;
		std::cin.ignore(32767, '\n');
		std::cout << std::endl;
		selectionNo = ((selectionNo <= 0) || (selectionNo > (unsigned int)rendererPluginList.size()) ? 0 : (selectionNo - 1));
		activePlugin = &rendererPluginList[selectionNo];
	}
	else
	{
		// We should just pick "The best" renderer. Currently DirectX11.
		const std::string bestEngine = "Direct3D11_1";
		for (auto& plugin : rendererPluginList)
		{
			std::string name = plugin.GetDisplayName();
			if (CaseInsensitiveContains(name, bestEngine))
			{
				activePlugin = &plugin;
				break;
			}
		}

		// Or if not, just take the only one.
		if (activePlugin == nullptr)
		{
			activePlugin = &rendererPluginList.front();
		}
	}
	log->Info("Selected renderer: {0} [{1}]", activePlugin->GetDisplayName().Get(), activePlugin->GetName().Get());

	// Retrieve a device enumerator from the renderer plugin
	auto deviceEnumerator = activePlugin->CreateGraphicsDeviceEnumerator(log->GetLoggerChildScope("Renderer"));
	auto enumerationFlags = IGraphicsDeviceEnumerator::EnumerationFlags::None;
	if (config.isApiValidation)
	{
		enumerationFlags |= IGraphicsDeviceEnumerator::EnumerationFlags::NativeApiValidation;
	}
	if (windowSystem == ITestSession::WindowSystem::Headless)
	{
		enumerationFlags |= IGraphicsDeviceEnumerator::EnumerationFlags::HeadlessRendering;
	}
	if (!deviceEnumerator->EnumerateDevices(enumerationFlags))
	{
		log->Critical("EnumerateDevices failed");
		return 1;
	}

	// Select the device to use
	std::vector<IGraphicsDevice*> deviceList = deviceEnumerator->GetAllDevices();
	IGraphicsDevice* preferredDevice = deviceEnumerator->GetPreferredDevice();
	IGraphicsDevice* device = nullptr;
	if (deviceList.empty())
	{
		log->Error("Failed to locate a compatible graphics device");
		return 2;
	}
	if (config.card)
	{
		for (auto& card : deviceList)
		{
			if (CaseInsensitiveContains(card->GetVendorName(), *config.card) || CaseInsensitiveContains(card->GetDeviceName(), *config.card))
			{
				device = card;
				break;
			}
		}
		if (device == nullptr)
		{
			log->Error("Couldn't find requested device: {0}", *config.card);
			return 2;
		}
	}
	else if (config.isForceDiscrete)
	{
		deviceEnumerator->FilterDevicesNotOfType(IGraphicsDevice::DeviceType::Discrete);
		device = deviceEnumerator->GetPreferredDevice();
		if (device == nullptr)
		{
			log->Error("Couldn't find discrete device");
			return 2;
		}
	}
	else if (config.isForceIntegrated)
	{
		deviceEnumerator->FilterDevicesNotOfType(IGraphicsDevice::DeviceType::Integrated);
		device = deviceEnumerator->GetPreferredDevice();
		if (device == nullptr)
		{
			log->Error("Couldn't find integrated device");
			return 2;
		}
	}
	else if (config.isForceSoftware)
	{
		deviceEnumerator->FilterDevicesNotOfType(IGraphicsDevice::DeviceType::Software);
		device = deviceEnumerator->GetPreferredDevice();
		if (device == nullptr)
		{
			log->Error("Couldn't find software device");
			return 2;
		}
	}
	else if (config.isNoPrompt || (deviceList.size() == 1))
	{
		device = preferredDevice;
	}
	else
	{
		std::cout << "Please select the adapter to use: (0 for preferred)" << std::endl;
		for (size_t i = 0; i < deviceList.size(); ++i)
		{
			device = deviceList[i];
			uint64_t dedicatedMemorySizeInBytes = device->GetMemorySizeInBytes(IGraphicsDevice::MemoryType::Dedicated);
			double dedicatedMemorySizeInGigabytes = (double)dedicatedMemorySizeInBytes / (double)(1024 * 1024 * 1024);
			std::stringstream stringStream;
			stringStream << std::fixed << std::setprecision(2);
			stringStream << dedicatedMemorySizeInGigabytes;
			std::string dedicatedMemorySizeString = stringStream.str();
			std::cout << i + 1 << ". " << (preferredDevice == device ? "*" : "") << device->GetVendorName() << "\t" << device->GetDeviceName() << "\t"
			          << "Dedicated Memory: " << dedicatedMemorySizeString << " GB\t" << (device->GetDeviceType() == IGraphicsDevice::DeviceType::Integrated ? "(Integrated)" : "") << (device->GetDeviceType() == IGraphicsDevice::DeviceType::Software ? "(Software)" : "") << std::endl;
		}
		unsigned int selectedDeviceNo;
		std::cin >> selectedDeviceNo;
		std::cin.ignore(32767, '\n');
		std::cout << std::endl;
		selectedDeviceNo = ((selectedDeviceNo <= 0) || (selectedDeviceNo > (unsigned int)deviceList.size())) ? 0 : selectedDeviceNo;
		if (selectedDeviceNo == 0)
		{
			device = preferredDevice;
		}
		else
		{
			device = deviceList[selectedDeviceNo - 1];
		}
	}
	log->Info("Selected device {0} from vendor {1}", device->GetDeviceName().Get(), device->GetVendorName().Get());

	// Obtain an X display server connection on Linux if required
#ifdef __linux__
	Display* display = nullptr;
	if (!config.isNoInterface || (windowSystem != ITestSession::WindowSystem::Headless))
	{
		display = XOpenDisplay(nullptr);
		if (display == nullptr)
		{
			if (!config.isNoInterface)
			{
				log->Error("Failed to open X display for user interface.");
				return 1;
			}
			log->Error("Non-headless rendering is unavailable because no native X display could be opened.");
			return 2;
		}
	}
#endif

	// Create the renderer
	std::set<IGraphicsDevice::Feature> enabledFeatures = device->GetAllSupportedFeatures();
	std::set<IRenderer::Options> enabledOptions;
	if (config.isApiValidation)
	{
		enabledOptions.insert(IRenderer::Options::EnableDebugLogging);
		enabledOptions.insert(IRenderer::Options::EnableRenderMarkers);
	}
	auto renderer = device->CreateRenderer(enabledFeatures, enabledOptions);

	// Initialize the renderer
	IRenderer::InitializationFlags initializationFlags = IRenderer::InitializationFlags::None;
	bool rendererInitialized = false;
	if (windowSystem == ITestSession::WindowSystem::Headless)
	{
		WindowSystemInfoHeadless windowSystemInfo;
		rendererInitialized = renderer->Initialize(windowSystemInfo, initializationFlags);
	}
	else
	{
#ifdef _WIN32
		WindowSystemInfoWin32 windowSystemInfo;
		rendererInitialized = renderer->Initialize(windowSystemInfo, initializationFlags);
#elif defined(__APPLE__)
		WindowSystemInfoAppKit windowSystemInfo;
		rendererInitialized = renderer->Initialize(windowSystemInfo, initializationFlags);
#else
		WindowSystemInfoXlib windowSystemInfo(display);
		rendererInitialized = renderer->Initialize(windowSystemInfo, initializationFlags);
#endif
	}
	if (!rendererInitialized)
	{
		log->Critical("Failed to initialize renderer");
		return 1;
	}

	// Headless rendering and interface visibility are independent. Only bypass the interface when explicitly requested.
	if ((windowSystem == ITestSession::WindowSystem::Headless) && config.isNoInterface)
	{
		cobalt::graphics::testing::Window window;
		ImmediateThreadInvocation threadInvocation(std::this_thread::get_id());
		TestManager testManager(*log, *device, *renderer, *activePlugin, window, threadInvocation, config, windowSystem);
		std::vector<IUnitTest*> testSet;
		BuildRequestedTestSet(config, *log, testSet);
		bool testResult = testManager.ExecuteTests(testSet, GetReportFilePath(config), config.passFailFile.value_or(""));
		renderer.reset();
		return (!testResult ? 3 : 0);
	}

	// Create the application object if required
#ifdef __APPLE__
	[NSApplication sharedApplication];
	[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
#endif

	// Create the main window
	MenuItem menuRoot;
	cobalt::graphics::testing::Window window;
	std::string windowTitle = "Graphics Test Framework - " + activePlugin->GetDisplayName();
#ifdef _WIN32
	window.UpdateMenu(&menuRoot);
	HINSTANCE hinstance = ::GetModuleHandle(nullptr);
	auto appIcon = LoadIconW(hinstance, MAKEINTRESOURCEW(IDI_APP_ICON));
	if (!window.Create(windowTitle, 1024, 768, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, WS_EX_CONTROLPARENT | WS_EX_CLIENTEDGE, hinstance, nullptr, appIcon, !config.isNoInterface))
#elif defined(__APPLE__)
	if (!window.Create(windowTitle, 1024, 768, !config.isNoInterface))
#else
	if (display == nullptr)
	{
		display = XOpenDisplay(nullptr);
	}
	if (display == nullptr)
	{
		log->Critical("Failed to open X display");
		return 1;
	}
	if (!window.Create(display, windowTitle, 1024, 768, !config.isNoInterface))
#endif
	{
		log->Critical("Failed to create main window");
		return 1;
	}

	// Attach our window close handler
#ifdef _WIN32
	window.AttachCloseHandler([&] { PostQuitMessage(0); });
#elif defined(__APPLE__)
	window.AttachCloseHandler([&] {
		[NSApp stop:nil];
		[NSApp postEvent:[NSEvent otherEventWithType:NSEventTypeApplicationDefined location:NSZeroPoint modifierFlags:0 timestamp:0 windowNumber:0 context:nil subtype:0 data1:0 data2:0] atStart:NO];
	});
#else
	bool quitMessagePosted = false;
	window.AttachCloseHandler([&] { quitMessagePosted = true; });
#endif

	// Attach our handler for UI thread invocation
#ifdef _WIN32
	window.AttachHandler(WM_USER, [&](WPARAM wparam, LPARAM lparam) -> LRESULT { auto *callback = reinterpret_cast<std::function<void()>*>(lparam); (*callback)(); delete callback; return 0; });
	Win32MessageThreadInvocation threadInvocation(std::this_thread::get_id(), window.GetOsHandle(), WM_USER);
#elif defined(__APPLE__)
	MacOSThreadInvocation threadInvocation(std::this_thread::get_id());
#else
	X11MessageThreadInvocation threadInvocation(std::this_thread::get_id(), window.GetDisplay(), window.GetInvokeWindow(), window.GetInvokeAtom());
#endif

	// Create the test manager
	TestManager testManager(*log, *device, *renderer, *activePlugin, window, threadInvocation, config, windowSystem);

	// Populate the menu for the main window
	auto& fileMenu = menuRoot.EmplaceBack();
	auto& configMenu = menuRoot.EmplaceBack();
	auto& testsMenu = menuRoot.EmplaceBack();
	auto& performanceTestsMenu = menuRoot.EmplaceBack();
	fileMenu.caption = "File";
	configMenu.caption = "Config";
	testsMenu.caption = "Tests";
	performanceTestsMenu.caption = "Performance";
	{
		MenuItem exit;
		exit.caption = "Exit";
		exit.onClick = [&] { window.Close(); };
		fileMenu.PushBack(std::move(exit));
	}
	{
		MenuItem testControl;
		testControl.caption = "Test control";

		MenuItem stepButton;
		stepButton.caption = "Step";
		stepButton.onClick = [&] { testManager.ExecuteStep(); };

		testControl.PushBack(std::move(stepButton));

		MenuItem playButton;
		playButton.caption = "Play";
		playButton.onClick = [&] { testManager.SetPausedState(false); };
		testControl.PushBack(std::move(playButton));

		MenuItem pauseButton;
		pauseButton.caption = "Pause";
		pauseButton.onClick = [&] { testManager.SetPausedState(true); };
		testControl.PushBack(std::move(pauseButton));

		configMenu.PushBack(std::move(testControl));

		MenuItem& burnInButton = configMenu.EmplaceBack();
		burnInButton.caption = "Repeat Tests (Burn-in Mode)";
		burnInButton.isTickable = true;
		burnInButton.isTicked = config.isGeneratingReport;
		burnInButton.onClick = [&] { burnInTest = burnInButton.isTicked; };

		configMenu.EmplaceBack().isSeparator = true;

		MenuItem& performImageComparisonButton = configMenu.EmplaceBack();
		performImageComparisonButton.caption = "Perform Image Comparison";
		performImageComparisonButton.isTickable = true;
		performImageComparisonButton.isTicked = !config.isDemo;
		performImageComparisonButton.onClick = [&] { config.isDemo = !performImageComparisonButton.isTicked; };

		MenuItem& dataUpdateButton = configMenu.EmplaceBack();
		dataUpdateButton.caption = "Update Reference Images";
		dataUpdateButton.isTickable = true;
		dataUpdateButton.isTicked = config.isUpdateData;
		dataUpdateButton.onClick = [&] { config.isUpdateData = dataUpdateButton.isTicked; };

		MenuItem& clearReportButton = configMenu.EmplaceBack();
		clearReportButton.caption = "Clear Report";
		clearReportButton.onClick = [&] {
			auto reportFilePath = GetReportFilePathForOpen(config);
			if (!reportFilePath.empty())
			{
				std::filesystem::remove(reportFilePath);
			}
		};

		MenuItem& generateReportButton = configMenu.EmplaceBack();
		generateReportButton.caption = "Generate Report";
		generateReportButton.isTickable = true;
		generateReportButton.isTicked = config.isGeneratingReport;
		generateReportButton.onClick = [&] { config.isGeneratingReport = generateReportButton.isTicked; };

		MenuItem openReportButton;
		openReportButton.caption = "Open Report";
#ifdef _WIN32
		openReportButton.onClick = [&] {
			auto reportFilePath = GetReportFilePathForOpen(config);
			if (!reportFilePath.empty())
			{
				ShellExecuteW(reinterpret_cast<HWND>(window.GetOsHandle()), L"open", reportFilePath.wstring().c_str(), nullptr, nullptr, SW_RESTORE);
			}
		};
#elif defined(__APPLE__)
		openReportButton.onClick = [&] {
			auto reportFilePath = GetReportFilePathForOpen(config);
			if (!reportFilePath.empty())
			{
				const auto path = reportFilePath.string();
				const auto url = CFURLCreateFromFileSystemRepresentation(nullptr, reinterpret_cast<const UInt8*>(path.c_str()), path.size(), false);
				LSOpenCFURLRef(url, nullptr);
				CFRelease(url);
			}
		};
#else
		openReportButton.onClick = [&] {
			auto reportFilePathAsString = GetReportFilePathForOpen(config).string();
			if (!reportFilePathAsString.empty())
			{
				if (fork() == 0)
				{
					execlp("xdg-open", "xdg-open", reportFilePathAsString.c_str(), static_cast<char*>(nullptr)); // NOLINT(cppcoreguidelines-pro-type-vararg,hicpp-vararg)
					_exit(127);
				}
			}
		};
#endif
		configMenu.PushBack(std::move(openReportButton));
	}
	{
		MenuItem runAllTestsButton;
		runAllTestsButton.caption = "Run All Tests";
		runAllTestsButton.onClick = [&] {
			std::thread([&]() {
				do
				{
					testManager.ExecuteTests(TestRegistry::GetAllRegisteredUnitTests(), GetReportFilePath(config));
				} while (burnInTest);
			}).detach();
		};
		testsMenu.PushBack(std::move(runAllTestsButton));
		MenuItem runRequestedTestsButton;
		runRequestedTestsButton.caption = "Run Command-line Requested Tests";
		runRequestedTestsButton.onClick = [&] {
			std::thread([&]() {
				do
				{
					std::vector<IUnitTest*> testSet;
					BuildRequestedTestSet(config, *log, testSet);
					testManager.ExecuteTests(testSet, GetReportFilePath(config), config.passFailFile.value_or(""));
				} while (burnInTest);
			}).detach();
		};
		testsMenu.PushBack(std::move(runRequestedTestsButton));

		testsMenu.EmplaceBack().isSeparator = true;
	}
	for (auto* test : TestRegistry::GetAllRegisteredUnitTests())
	{
		BuildTestMenu(config, burnInTest, testManager, test, &testsMenu, test->TestFullName());
	}
	{
		MenuItem runAllTestsButton;
		runAllTestsButton.caption = "Run All Tests";
		runAllTestsButton.onClick = [&] {
			std::thread([&]() {
				do
				{
					testManager.ExecuteTests(TestRegistry::GetAllRegisteredPerformanceTests(), GetReportFilePath(config));
				} while (burnInTest);
			}).detach();
		};
		performanceTestsMenu.PushBack(std::move(runAllTestsButton));
		MenuItem runRequestedTestsButton;
		runRequestedTestsButton.caption = "Run Command-line Requested Tests";
		runRequestedTestsButton.onClick = [&] {
			std::thread([&]() {
				do
				{
					std::vector<IUnitTest*> testSet;
					BuildRequestedTestSet(config, *log, testSet);
					testManager.ExecuteTests(testSet, GetReportFilePath(config), config.passFailFile.value_or(""));
				} while (burnInTest);
			}).detach();
		};
		performanceTestsMenu.PushBack(std::move(runRequestedTestsButton));

		performanceTestsMenu.EmplaceBack().isSeparator = true;
	}
	for (auto* test : TestRegistry::GetAllRegisteredPerformanceTests())
	{
		BuildTestMenu(config, burnInTest, testManager, test, &performanceTestsMenu, test->TestFullName());
	}

	window.UpdateMenu(&menuRoot);

	// Spawn a thread to manage execution of the requested tests
	bool testResult = true;
	std::thread executeThread([&]() {
		std::vector<IUnitTest*> testSet;
		BuildRequestedTestSet(config, *log, testSet);
		testResult = testManager.ExecuteTests(testSet, GetReportFilePath(config), config.passFailFile.value_or(""));
		if (config.isKillAfterTest)
		{
			window.Close();
		}
	});
	executeThread.detach();

	// Start the window message loop
#ifdef _WIN32
	bool done = false;
	while (!done)
	{
		MSG msg;
		BOOL getMessageReturn = GetMessage(&msg, nullptr, 0, 0);
		if ((getMessageReturn == -1) || (msg.message == WM_QUIT))
		{
			done = true;
			continue;
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
#elif defined(__APPLE__)
	[NSApp run];
#else
	while (!quitMessagePosted)
	{
		XEvent ev;
		XNextEvent(display, &ev);
		window.HandleX11Event(ev);
	}
#endif

	// Destroy the renderer before we destroy the window system
	renderer.reset();

	// Destroy the window system
#if !defined(_WIN32) && !defined(__APPLE__)
	if (display != nullptr)
	{
		XCloseDisplay(display);
	}
#endif

	// Return the overall result of test execution
	return (!testResult ? 3 : 0);
}
