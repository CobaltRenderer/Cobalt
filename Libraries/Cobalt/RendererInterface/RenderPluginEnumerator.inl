// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "CobaltVersionInfo.h"
#ifdef _WIN32
#include <string.h>
#else
#include <strings.h>
#endif
namespace cobalt { namespace graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
RenderPluginEnumerator::RenderPluginEnumerator(cobalt::logging::ILogger::unique_ptr log)
: _log(std::move(log))
{}

//----------------------------------------------------------------------------------------
RenderPluginEnumerator::~RenderPluginEnumerator()
{
	ClearAllPlugins();
}

//----------------------------------------------------------------------------------------
// Plugin methods
//----------------------------------------------------------------------------------------
SuccessToken RenderPluginEnumerator::EnumeratePluginsInDirectory(const std::filesystem::path& pluginDirectoryPath)
{
	// Attempt to create a directory enumerator in the target directory
	std::error_code errorCode;
	std::filesystem::directory_iterator directoryIterator(pluginDirectoryPath, errorCode);
	if (errorCode)
	{
		_log->Error("Failed to enumerate contents of plugin directory \"{0}\": {1}", pluginDirectoryPath.string(), errorCode.message());
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
			_log->Error("Failed to advance directory iterator for plugin directory \"{0}\": {1}", pluginDirectoryPath.string(), errorCode.message());
			return false;
		}
	}

	// Attempt to load all possible plugins found in the target path
	bool addedRenderPlugin = false;
	for (const auto& pluginPath : pluginPaths)
	{
		addedRenderPlugin |= AddPluginByPath(pluginPath);
	}

	// Return true if we found at least one renderer plugin
	return addedRenderPlugin;
}

//----------------------------------------------------------------------------------------
SuccessToken RenderPluginEnumerator::AddPluginByPath(const std::filesystem::path& pluginFilePath)
{
	bool addedRenderPlugin = false;
#ifdef _WIN32
	// Don't block the process with a modal error dialog on dll load errors. Ideally this has already been set, but if
	// not, we set it temporarily for the calling thread.
	DWORD oldErrorMode = 0;
	SetThreadErrorMode(SEM_FAILCRITICALERRORS, &oldErrorMode);

	// Load the target assembly into the process
	HMODULE moduleHandleRaw = LoadLibraryExW(pluginFilePath.wstring().c_str(), nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);

	// Undo the SetThreadErrorMode call and return the previous settings
	SetThreadErrorMode(oldErrorMode, nullptr);

	// If we failed to load the target module, abort any further processing.
	if (moduleHandleRaw == nullptr)
	{
		_log->Warning("Error loading assembly \"{0}\"! LoadLibrary failed with error code {1}", pluginFilePath.wstring(), GetLastError());
		return false;
	}

	// Obtain a pointer to the Cobalt API version function for the assembly. If the function isn't found, we assume this
	// assembly isn't a renderer plugin, and silently return false.
	auto getCobaltAPIVersionFunction = reinterpret_cast<void (*)(unsigned int&, unsigned int&, unsigned int&)>(GetProcAddress(moduleHandleRaw, "GetCobaltAPIVersion"));
	if (getCobaltAPIVersionFunction == nullptr)
	{
		FreeLibrary(moduleHandleRaw);
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
		_log->Warning("Incompatible Cobalt API used in assembly \"{0}\"! An API version of {1}.{2} was found, but {3}.{4} is required.", pluginFilePath.wstring(), pluginCobaltApiVersionMajor, pluginCobaltApiVersionMinor, COBALT_RENDERER_API_VERSION_MAJOR, COBALT_RENDERER_API_VERSION_MINOR);
		FreeLibrary(moduleHandleRaw);
		return false;
	}

	// Obtain a pointer to the interface function for the assembly. We expect this to exist at this point, since this
	// appears to be a Cobalt renderer plugin.
	auto getRendererPluginFunction = reinterpret_cast<IRendererPlugin::GetRendererPluginFunctionType*>(GetProcAddress(moduleHandleRaw, "GetRendererPlugin"));
	if (getRendererPluginFunction == nullptr)
	{
		_log->Warning("Failed to locate GetRendererPlugin function for assembly \"{0}\"!", pluginFilePath.wstring());
		FreeLibrary(moduleHandleRaw);
		return false;
	}

	// Load information on each renderer plugin provided by this assembly
	_log->Debug("Loading renderer plugin assembly \"{0}\"", pluginFilePath.wstring());
	unsigned int currentRendererIndex = 0;
	RendererPlugin rendererPlugin;
	while (getRendererPluginFunction(currentRendererIndex++, rendererPlugin))
	{
		HMODULE moduleHandleDuplicate;
		GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, reinterpret_cast<LPCWSTR>(getRendererPluginFunction), &moduleHandleDuplicate);
		rendererPlugin.SetModuleHandle(ModuleHandleWin32::Create(moduleHandleDuplicate));
		_allPlugins.push_back(rendererPlugin);
		_filteredPlugins.push_back(rendererPlugin);
		addedRenderPlugin = true;
		_log->Debug(R"(Found renderer plugin with name "{0}" and display name "{1}")", rendererPlugin.GetName().Get(), rendererPlugin.GetDisplayName().Get());
		rendererPlugin = RendererPlugin();
	}

	// Release the assembly handle. We created duplicate handles for each renderer plugin above using GetModuleHandleEx,
	// so this won't unload the assembly unless there were no renderer plugins defined by the assembly.
	FreeLibrary(moduleHandleRaw);

#else
	// Load the target assembly into the process
	auto filePathRaw = pluginFilePath.c_str();
	auto moduleHandleRaw = dlopen(filePathRaw, RTLD_NOW);
	auto lastError = dlerror();
	if (moduleHandleRaw == nullptr)
	{
		_log->Warning("Error loading assembly \"{0}\": {1}!", filePathRaw, lastError);
		return false;
	}

	// Obtain a pointer to the Cobalt API version function for the assembly. If the function isn't found, we assume this
	// assembly isn't a renderer plugin, and silently return false.
	dlerror();
	auto getCobaltAPIVersionFunction = reinterpret_cast<void (*)(unsigned int&, unsigned int&, unsigned int&)>(dlsym(moduleHandleRaw, "GetCobaltAPIVersion"));
	lastError = dlerror();
	if (lastError != nullptr)
	{
		dlclose(moduleHandleRaw);
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
		_log->Warning("Incompatible Cobalt API used in assembly \"{0}\"! An API version of {1}.{2} was found, but {3}.{4} is required.", pluginFilePath.native(), pluginCobaltApiVersionMajor, pluginCobaltApiVersionMinor, COBALT_RENDERER_API_VERSION_MAJOR, COBALT_RENDERER_API_VERSION_MINOR);
		dlclose(moduleHandleRaw);
		return false;
	}

	// Obtain a pointer to the interface function for the assembly. We expect this to exist at this point, since this
	// appears to be a Cobalt renderer plugin.
	dlerror();
	auto getRendererPluginFunction = reinterpret_cast<IRendererPlugin::GetRendererPluginFunctionType*>(dlsym(moduleHandleRaw, "GetRendererPlugin"));
	lastError = dlerror();
	if (lastError != nullptr)
	{
		_log->Warning("Failed to locate GetRendererPlugin function for assembly \"{0}\"!", pluginFilePath.native());
		dlclose(moduleHandleRaw);
		return false;
	}

	// Load information on each renderer plugin provided by this assembly
	_log->Debug("Loading renderer plugin assembly \"{0}\"", pluginFilePath.native());
	unsigned int currentRendererIndex = 0;
	RendererPlugin rendererPlugin;
	while (getRendererPluginFunction(currentRendererIndex++, rendererPlugin))
	{
		rendererPlugin.SetModuleHandle(ModuleHandlePosix::Create(dlopen(pluginFilePath.c_str(), RTLD_NOW), pluginFilePath.string()));
		_allPlugins.push_back(rendererPlugin);
		_filteredPlugins.push_back(rendererPlugin);
		addedRenderPlugin = true;
		_log->Debug(R"(Found renderer plugin with name "{0}" and display name "{1}")", rendererPlugin.GetName().Get(), rendererPlugin.GetDisplayName().Get());
		rendererPlugin = RendererPlugin();
	}

	// Release the assembly handle. We created duplicate handles for each renderer plugin above, so this won't unload
	// the assembly unless there were no renderer plugins defined by the assembly.
	dlclose(moduleHandleRaw);
#endif

	// Return true if we found at least one renderer plugin
	return addedRenderPlugin;
}

//----------------------------------------------------------------------------------------
std::vector<RendererPlugin> RenderPluginEnumerator::GetAllPlugins() const
{
	// Return a sorted list to the caller
	auto pluginSet = _allPlugins;
	std::sort(pluginSet.begin(), pluginSet.end(), [](const RendererPlugin& first, const RendererPlugin& second) { return first.GetDisplayName().Get() < second.GetDisplayName().Get(); });
	return pluginSet;
}

//----------------------------------------------------------------------------------------
std::vector<RendererPlugin> RenderPluginEnumerator::GetFilteredPlugins() const
{
	// Return a sorted list to the caller
	auto pluginSet = _filteredPlugins;
	std::sort(pluginSet.begin(), pluginSet.end(), [](const RendererPlugin& first, const RendererPlugin& second) { return first.GetDisplayName().Get() < second.GetDisplayName().Get(); });
	return pluginSet;
}

//----------------------------------------------------------------------------------------
std::optional<RendererPlugin> RenderPluginEnumerator::GetPreferredPlugin() const
{
	// Define our sort function to prioritize the current preferred renderers for each platform
	auto sortFunc = [](const RendererPlugin& first, const RendererPlugin& second) {
		auto firstApiFamily = first.GetApiFamily();
		auto secondApiFamily = second.GetApiFamily();
		auto firstVersion = first.GetTargetApiVersion();
		auto secondVersion = second.GetTargetApiVersion();
		if ((firstApiFamily != secondApiFamily))
		{
#ifdef _WIN32
			// Windows (Direct3D, Vulkan, and OpenGL supported)
			if (firstApiFamily == IRendererPlugin::ApiFamily::Direct3D)
			{
				return true;
			}
			if (secondApiFamily == IRendererPlugin::ApiFamily::Direct3D)
			{
				return false;
			}
			if (firstApiFamily == IRendererPlugin::ApiFamily::Vulkan)
			{
				return true;
			}
			if (secondApiFamily == IRendererPlugin::ApiFamily::Vulkan)
			{
				return false;
			}
			// Shouldn't get here. Only OpenGL left.
#elif defined(__APPLE__)
			// macOS (Metal, Vulkan (via MoltenVK), and OpenGL supported)
			if (firstApiFamily == IRendererPlugin::ApiFamily::Metal)
			{
				return true;
			}
			if (secondApiFamily == IRendererPlugin::ApiFamily::Metal)
			{
				return false;
			}
			if (firstApiFamily == IRendererPlugin::ApiFamily::Vulkan)
			{
				return true;
			}
			if (secondApiFamily == IRendererPlugin::ApiFamily::Vulkan)
			{
				return false;
			}
			// Shouldn't get here. Only OpenGL left.
#else
			// Linux (Vulkan and OpenGL supported)
			if (firstApiFamily == IRendererPlugin::ApiFamily::Vulkan)
			{
				return true;
			}
			if (secondApiFamily == IRendererPlugin::ApiFamily::Vulkan)
			{
				return false;
			}
			// Shouldn't get here. Only OpenGL left.
#endif
		}
#ifdef _WIN32
		// Prioritize Direct3D 11 over Direct3D 12 on Windows still. Generally gives better performance and less
		// potential failure cases, especially with multiple concurrent apps sharing intensive GPU usage, due to
		// kernel-level video memory residency management.
		else if ((firstApiFamily == IRendererPlugin::ApiFamily::Direct3D) && (firstVersion.major == 11))
		{
			return true;
		}
		if ((firstApiFamily == IRendererPlugin::ApiFamily::Direct3D) && (secondVersion.major == 11))
		{
			return false;
		}
#endif
		else if (firstVersion.major > secondVersion.major)
		{
			return true;
		}
		if (secondVersion.major > firstVersion.major)
		{
			return false;
		}
		return (firstVersion.minor > secondVersion.minor);
	};
	auto pluginSet = _filteredPlugins;
	std::sort(pluginSet.begin(), pluginSet.end(), sortFunc);
	return (pluginSet.empty() ? std::nullopt : std::optional<RendererPlugin>(_allPlugins.front()));
}

//----------------------------------------------------------------------------------------
void RenderPluginEnumerator::ClearAllPlugins()
{
	// Clear all our plugin lists
	_allPlugins.clear();
	_filteredPlugins.clear();
}

//----------------------------------------------------------------------------------------
// Filtering methods
//----------------------------------------------------------------------------------------
void RenderPluginEnumerator::FilterPlugin(const RendererPlugin& targetPlugin)
{
	// Iterate all remaining filtered plugins, and remove any plugins that don't match the filter requirement.
	auto pluginIterator = _filteredPlugins.begin();
	while (pluginIterator != _filteredPlugins.end())
	{
		// Determine if we should filter this plugin
		const auto& plugin = *pluginIterator;
		bool filterOutPlugin = (plugin.GetApiFamily() == targetPlugin.GetApiFamily()) &&
		  (plugin.GetName().Get() == targetPlugin.GetName().Get()) &&
		  (plugin.GetTargetApiVersion().major == targetPlugin.GetTargetApiVersion().major) &&
		  (plugin.GetTargetApiVersion().minor == targetPlugin.GetTargetApiVersion().minor);

		// Erase or retain this plugin in the filtered plugin list as required
		if (filterOutPlugin)
		{
			pluginIterator = _filteredPlugins.erase(pluginIterator);
		}
		else
		{
			++pluginIterator;
		}
	}
}

//----------------------------------------------------------------------------------------
void RenderPluginEnumerator::FilterPluginsOfFamily(IRendererPlugin::ApiFamily apiFamily)
{
	// Iterate all remaining filtered plugins, and remove any plugins that don't match the filter requirement.
	auto pluginIterator = _filteredPlugins.begin();
	while (pluginIterator != _filteredPlugins.end())
	{
		// Determine if we should filter this plugin
		const auto& plugin = *pluginIterator;
		bool filterOutPlugin = (plugin.GetApiFamily() == apiFamily);

		// Erase or retain this plugin in the filtered plugin list as required
		if (filterOutPlugin)
		{
			pluginIterator = _filteredPlugins.erase(pluginIterator);
		}
		else
		{
			++pluginIterator;
		}
	}
}

//----------------------------------------------------------------------------------------
void RenderPluginEnumerator::FilterPluginsNotOfFamily(IRendererPlugin::ApiFamily apiFamily)
{
	// Iterate all remaining filtered plugins, and remove any plugins that don't match the filter requirement.
	auto pluginIterator = _filteredPlugins.begin();
	while (pluginIterator != _filteredPlugins.end())
	{
		// Determine if we should filter this plugin
		const auto& plugin = *pluginIterator;
		bool filterOutPlugin = (plugin.GetApiFamily() != apiFamily);

		// Erase or retain this plugin in the filtered plugin list as required
		if (filterOutPlugin)
		{
			pluginIterator = _filteredPlugins.erase(pluginIterator);
		}
		else
		{
			++pluginIterator;
		}
	}
}

//----------------------------------------------------------------------------------------
void RenderPluginEnumerator::FilterPluginsOfFamilyBelowVersion(IRendererPlugin::ApiFamily apiFamily, IRendererPlugin::ApiVersion apiVersion)
{
	// Iterate all remaining filtered plugins, and remove any plugins that don't match the filter requirement.
	auto pluginIterator = _filteredPlugins.begin();
	while (pluginIterator != _filteredPlugins.end())
	{
		// Determine if we should filter this plugin
		const auto& plugin = *pluginIterator;
		auto pluginVersion = plugin.GetTargetApiVersion();
		bool filterOutPlugin = (plugin.GetApiFamily() == apiFamily) && ((pluginVersion.major < apiVersion.major) || ((pluginVersion.major == apiVersion.major) && (pluginVersion.minor < apiVersion.minor)));

		// Erase or retain this plugin in the filtered plugin list as required
		if (filterOutPlugin)
		{
			pluginIterator = _filteredPlugins.erase(pluginIterator);
		}
		else
		{
			++pluginIterator;
		}
	}
}

//----------------------------------------------------------------------------------------
void RenderPluginEnumerator::ClearPluginFilters()
{
	_filteredPlugins = _allPlugins;
}

//----------------------------------------------------------------------------------------
// Helper methods
//----------------------------------------------------------------------------------------
std::filesystem::path RenderPluginEnumerator::GetProcessDirectory() const
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

}} // namespace cobalt::graphics
