// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "RendererPlugin.h"
#include <Cobalt/Logging/Logging.pkg>
#include <Cobalt/RendererInterface/PlatformBindings.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <cstring>

using namespace cobalt::graphics;

//----------------------------------------------------------------------------------------
struct Cobalt_Library_Internal
{
	cobalt::logging::LogManager logManager;
};

//----------------------------------------------------------------------------------------
struct Cobalt_RendererPlugin_Internal
{
	RendererPlugin rendererPlugin;
	Cobalt_Library_Internal* library = nullptr;
};

//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_Initialize(Cobalt_LogCallback logCallback, Cobalt_LogSeverityFilter logFilter, Cobalt_Library* library)
{
	// Ensure that the output variable is currently null
	if (*library != nullptr)
	{
		return COBALT_FAILURE;
	}

	// Construct the library structure
	auto _library = new Cobalt_Library_Internal();
	*library = _library;

	// Configure the logging system using the supplied settings
	_library->logManager.SetIncludeSeverity((cobalt::logging::ILogger::SeverityFilter)logFilter);
	if (logCallback != nullptr)
	{
		auto logTarget = cobalt::logging::LogTargetCallback::Create([logCallback](const std::string& scope, cobalt::logging::ILogger::Severity severity, const std::string& message) {
			logCallback((Cobalt_LogSeverity)severity, scope.c_str(), scope.length(), message.c_str(), message.length());
		});

		_library->logManager.AddLogTarget(std::move(logTarget));
	}
	return COBALT_SUCCESS;
}

//----------------------------------------------------------------------------------------
void Cobalt_Terminate(Cobalt_Library library)
{
	delete library;
}

//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_GetRendererPlugin(Cobalt_Library library, void* moduleHandle, unsigned int index, Cobalt_RendererPlugin* plugin)
{
	// Ensure that the output variable is currently null
	if (*plugin != nullptr)
	{
		return COBALT_FAILURE;
	}

	// Construct a logger
	auto log = library->logManager.GetLogger("GetRendererPlugin");

	// Ensure the target assembly is a Cobalt renderer assembly and it matches the required API version
#ifdef _WIN32
	// Obtain a pointer to the Cobalt API version function for the assembly. If the function isn't found, we assume this
	// assembly isn't a renderer plugin, and silently return false.
	auto getCobaltAPIVersionFunction = reinterpret_cast<void (*)(unsigned int&, unsigned int&, unsigned int&)>(GetProcAddress(reinterpret_cast<HMODULE>(moduleHandle), "GetCobaltAPIVersion"));
	if (getCobaltAPIVersionFunction == nullptr)
	{
		return COBALT_FAILURE;
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
		log->Warning("Incompatible Cobalt API used in assembly! An API version of {0}.{1} was found, but {2}.{3} is required.", pluginCobaltApiVersionMajor, pluginCobaltApiVersionMinor, COBALT_RENDERER_API_VERSION_MAJOR, COBALT_RENDERER_API_VERSION_MINOR);
		return COBALT_FAILURE;
	}
#else
	// Obtain a pointer to the Cobalt API version function for the assembly. If the function isn't found, we assume this
	// assembly isn't a renderer plugin, and silently return false.
	dlerror();
	auto getCobaltAPIVersionFunction = reinterpret_cast<void (*)(unsigned int&, unsigned int&, unsigned int&)>(dlsym(moduleHandle, "GetCobaltAPIVersion"));
	if (dlerror() != nullptr)
	{
		return COBALT_FAILURE;
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
		log->Warning("Incompatible Cobalt API used in assembly! An API version of {0}.{1} was found, but {2}.{3} is required.", pluginCobaltApiVersionMajor, pluginCobaltApiVersionMinor, COBALT_RENDERER_API_VERSION_MAJOR, COBALT_RENDERER_API_VERSION_MINOR);
		return COBALT_FAILURE;
	}
#endif

	// Retrieve the requested renderer plugin
	RendererPlugin rendererPlugin;
#ifdef _WIN32
	auto getRendererPluginFunction = reinterpret_cast<IRendererPlugin::GetRendererPluginFunctionType*>(GetProcAddress(reinterpret_cast<HMODULE>(moduleHandle), "GetRendererPlugin"));
	if (getRendererPluginFunction == nullptr)
	{
		log->Warning("Failed to locate GetRendererPlugin function for assembly!");
		return COBALT_FAILURE;
	}
	if (!getRendererPluginFunction(index, rendererPlugin))
	{
		return COBALT_FAILURE;
	}
#else
	dlerror();
	auto getRendererPluginFunction = reinterpret_cast<IRendererPlugin::GetRendererPluginFunctionType*>(dlsym(moduleHandle, "GetRendererPlugin"));
	if (dlerror() != nullptr)
	{
		log->Warning("Failed to locate GetRendererPlugin function for assembly!");
		return COBALT_FAILURE;
	}
	if (!getRendererPluginFunction(index, rendererPlugin))
	{
		return COBALT_FAILURE;
	}
#endif

	// Return the renderer plugin to the caller
	auto _plugin = new Cobalt_RendererPlugin_Internal();
	_plugin->library = library;
	_plugin->rendererPlugin = std::move(rendererPlugin);
	*plugin = _plugin;
	return COBALT_SUCCESS;
}

//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_GetRendererPluginStatic(Cobalt_Library library, void* getRendererPluginFunction, Cobalt_RendererPlugin* plugin)
{
	// Ensure that the output variable is currently null
	if (*plugin != nullptr)
	{
		return COBALT_FAILURE;
	}

	// Construct a logger
	auto log = library->logManager.GetLogger("GetRendererPluginStatic");

	// Retrieve the requested renderer plugin
	RendererPlugin rendererPlugin;
	auto _getRendererPluginFunction = reinterpret_cast<bool (*)(IRendererPlugin&)>(getRendererPluginFunction);
	if (!_getRendererPluginFunction(rendererPlugin))
	{
		return COBALT_FAILURE;
	}

	// Return the renderer plugin to the caller
	auto _plugin = new Cobalt_RendererPlugin_Internal();
	_plugin->library = library;
	_plugin->rendererPlugin = std::move(rendererPlugin);
	*plugin = _plugin;
	return COBALT_SUCCESS;
}

//----------------------------------------------------------------------------------------
Cobalt_ApiFamily Cobalt_RendererPlugin_GetApiFamily(Cobalt_RendererPlugin plugin)
{
	return (Cobalt_ApiFamily)plugin->rendererPlugin.GetApiFamily();
}

//----------------------------------------------------------------------------------------
void Cobalt_RendererPlugin_GetTargetApiVersion(Cobalt_RendererPlugin plugin, Cobalt_Version* version)
{
	auto _version = plugin->rendererPlugin.GetTargetApiVersion();
	version->major = _version.major;
	version->minor = _version.minor;
}

//----------------------------------------------------------------------------------------
void Cobalt_RendererPlugin_GetName(Cobalt_RendererPlugin plugin, char* name, size_t* nameLength)
{
	auto _name = plugin->rendererPlugin.GetName().Get();
	if (_name.size() <= *nameLength)
	{
		std::memcpy(name, _name.data(), _name.size()); // NOLINT(bugprone-not-null-terminated-result)
	}
	*nameLength = _name.size();
}

//----------------------------------------------------------------------------------------
void Cobalt_RendererPlugin_GetDisplayName(Cobalt_RendererPlugin plugin, char* name, size_t* nameLength)
{
	auto _name = plugin->rendererPlugin.GetDisplayName().Get();
	if (_name.size() <= *nameLength)
	{
		std::memcpy(name, _name.data(), _name.size()); // NOLINT(bugprone-not-null-terminated-result)
	}
	*nameLength = _name.size();
}

//----------------------------------------------------------------------------------------
void Cobalt_RendererPlugin_CreateGraphicsDeviceEnumerator(Cobalt_RendererPlugin plugin, Cobalt_GraphicsDeviceEnumerator* enumerator)
{
	auto log = plugin->library->logManager.GetLogger("Renderer");
	auto _enumerator = plugin->rendererPlugin.CreateGraphicsDeviceEnumerator(std::move(log));
	*enumerator = reinterpret_cast<Cobalt_GraphicsDeviceEnumerator>(_enumerator.release());
}

//----------------------------------------------------------------------------------------
void Cobalt_RendererPlugin_Delete(Cobalt_RendererPlugin plugin)
{
	delete plugin;
}
