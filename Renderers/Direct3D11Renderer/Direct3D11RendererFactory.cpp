// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "AssemblyVersionInfo.h"
#include "Direct3DGraphicsDeviceEnumerator.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <memory>
namespace cobalt::graphics {

// Predeclarations
extern "C" __declspec(dllexport) IGraphicsDeviceEnumerator* CreateDirect3D11Renderer(cobalt::logging::ILogger* log);
extern "C" __declspec(dllexport) bool GetDirect3D11RendererPlugin(IRendererPlugin& rendererPlugin);
extern "C" __declspec(dllexport) bool GetRendererPlugin(unsigned int indexNo, IRendererPlugin& rendererPlugin);
extern "C" __declspec(dllexport) void GetCobaltAPIVersion(unsigned int& major, unsigned int& minor, unsigned int& patch);

//----------------------------------------------------------------------------------------
extern "C" __declspec(dllexport) IGraphicsDeviceEnumerator* CreateDirect3D11Renderer(cobalt::logging::ILogger* log)
{
	return new Direct3DGraphicsDeviceEnumerator(cobalt::logging::ILogger::unique_ptr(log));
}

//----------------------------------------------------------------------------------------
extern "C" __declspec(dllexport) bool GetDirect3D11RendererPlugin(IRendererPlugin& rendererPlugin)
{
	rendererPlugin.SetAllocationFunction(CreateDirect3D11Renderer);
	rendererPlugin.SetApiFamily(IRendererPlugin::ApiFamily::Direct3D);
	rendererPlugin.SetTargetApiVersion(IRendererPlugin::ApiVersion(11, 1));
	rendererPlugin.SetName("Direct3D11_1");
	rendererPlugin.SetDisplayName("Direct3D 11");
	return true;
}

//----------------------------------------------------------------------------------------
extern "C" __declspec(dllexport) bool GetRendererPlugin(unsigned int indexNo, IRendererPlugin& rendererPlugin)
{
	switch (indexNo)
	{
	case 0:
		return GetDirect3D11RendererPlugin(rendererPlugin);
	}
	return false;
}

//----------------------------------------------------------------------------------------
extern "C" __declspec(dllexport) void GetCobaltAPIVersion(unsigned int& major, unsigned int& minor, unsigned int& patch)
{
	major = AssemblyVersionInfo_VersionDigit_Major;
	minor = AssemblyVersionInfo_VersionDigit_Minor;
	patch = AssemblyVersionInfo_VersionDigit_Revision;
}

} // namespace cobalt::graphics
