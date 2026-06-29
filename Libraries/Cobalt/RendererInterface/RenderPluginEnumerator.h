// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "IModuleHandle.h"
#include "RendererPlugin.h"
#include <algorithm>
#include <filesystem>
#include <list>
#include <optional>
#include <vector>
#ifdef _WIN32
#include "ModuleHandleWin32.h"
#else
#include "ModuleHandlePosix.h"
#endif
namespace cobalt { namespace graphics {

class RenderPluginEnumerator
{
public:
	// Constructors
	inline explicit RenderPluginEnumerator(cobalt::logging::ILogger::unique_ptr log);
	inline ~RenderPluginEnumerator();

	// Plugin methods
	inline SuccessToken EnumeratePluginsInDirectory(const std::filesystem::path& pluginDirectoryPath);
	inline SuccessToken AddPluginByPath(const std::filesystem::path& pluginFilePath);
	inline std::vector<RendererPlugin> GetAllPlugins() const;
	inline std::vector<RendererPlugin> GetFilteredPlugins() const;
	inline std::optional<RendererPlugin> GetPreferredPlugin() const;
	inline void ClearAllPlugins();

	// Filtering methods
	inline void FilterPlugin(const RendererPlugin& targetPlugin);
	inline void FilterPluginsOfFamily(IRendererPlugin::ApiFamily apiFamily);
	inline void FilterPluginsNotOfFamily(IRendererPlugin::ApiFamily apiFamily);
	inline void FilterPluginsOfFamilyBelowVersion(IRendererPlugin::ApiFamily apiFamily, IRendererPlugin::ApiVersion apiVersion);
	inline void ClearPluginFilters();

	// Helper methods
	inline std::filesystem::path GetProcessDirectory() const;

private:
	cobalt::logging::ILogger::unique_ptr _log = nullptr;
	std::vector<RendererPlugin> _allPlugins;
	std::vector<RendererPlugin> _filteredPlugins;
};

}} // namespace cobalt::graphics
#include "RenderPluginEnumerator.inl"
