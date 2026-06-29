// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "AssemblyVersionInfo.h"
#include "OpenGLGraphicsDeviceEnumerator.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <memory>
#ifdef _WIN32
#define FUNCTION_EXPORT __declspec(dllexport)
#else
#define FUNCTION_EXPORT __attribute__((visibility("default")))
#endif
namespace cobalt::graphics {

// Predeclarations
extern "C" FUNCTION_EXPORT IGraphicsDeviceEnumerator* CreateOpenGL4Renderer(cobalt::logging::ILogger* log);
extern "C" FUNCTION_EXPORT bool GetOpenGL4RendererPlugin(IRendererPlugin& rendererPlugin);
extern "C" FUNCTION_EXPORT bool GetRendererPlugin(unsigned int indexNo, IRendererPlugin& rendererPlugin);
extern "C" FUNCTION_EXPORT void GetCobaltAPIVersion(unsigned int& major, unsigned int& minor, unsigned int& patch);

//----------------------------------------------------------------------------------------
extern "C" FUNCTION_EXPORT IGraphicsDeviceEnumerator* CreateOpenGL4Renderer(cobalt::logging::ILogger* log)
{
	return new OpenGLGraphicsDeviceEnumerator(cobalt::logging::ILogger::unique_ptr(log));
}

//----------------------------------------------------------------------------------------
extern "C" FUNCTION_EXPORT bool GetOpenGL4RendererPlugin(IRendererPlugin& rendererPlugin)
{
	rendererPlugin.SetAllocationFunction(CreateOpenGL4Renderer);
	rendererPlugin.SetApiFamily(IRendererPlugin::ApiFamily::OpenGL);
	rendererPlugin.SetTargetApiVersion(IRendererPlugin::ApiVersion(4, 3));
	rendererPlugin.SetName("OpenGL4_3");
	rendererPlugin.SetDisplayName("OpenGL 4.3 Core");
	return true;
}

//----------------------------------------------------------------------------------------
extern "C" FUNCTION_EXPORT bool GetRendererPlugin(unsigned int indexNo, IRendererPlugin& rendererPlugin)
{
	switch (indexNo)
	{
	case 0:
		return GetOpenGL4RendererPlugin(rendererPlugin);
	}
	return false;
}

//----------------------------------------------------------------------------------------
extern "C" FUNCTION_EXPORT void GetCobaltAPIVersion(unsigned int& major, unsigned int& minor, unsigned int& patch)
{
	major = AssemblyVersionInfo_VersionDigit_Major;
	minor = AssemblyVersionInfo_VersionDigit_Minor;
	patch = AssemblyVersionInfo_VersionDigit_Revision;
}

} // namespace cobalt::graphics
