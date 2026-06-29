// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
namespace cobalt { namespace graphics {

//----------------------------------------------------------------------------------------
// Data access methods
//----------------------------------------------------------------------------------------
RendererPlugin::ApiFamily RendererPlugin::GetApiFamily() const
{
	return _apiFamily;
}

//----------------------------------------------------------------------------------------
void RendererPlugin::SetApiFamily(ApiFamily value)
{
	_apiFamily = value;
}

//----------------------------------------------------------------------------------------
RendererPlugin::ApiVersion RendererPlugin::GetTargetApiVersion() const
{
	return _apiVersion;
}

//----------------------------------------------------------------------------------------
void RendererPlugin::SetTargetApiVersion(ApiVersion value)
{
	_apiVersion = value;
}

//----------------------------------------------------------------------------------------
Marshal::Ret<std::string> RendererPlugin::GetName() const
{
	return _name;
}

//----------------------------------------------------------------------------------------
void RendererPlugin::SetName(const Marshal::In<std::string>& value)
{
	_name = value;
}

//----------------------------------------------------------------------------------------
Marshal::Ret<std::string> RendererPlugin::GetDisplayName() const
{
	return _displayName;
}

//----------------------------------------------------------------------------------------
void RendererPlugin::SetDisplayName(const Marshal::In<std::string>& value)
{
	_displayName = value;
}

//----------------------------------------------------------------------------------------
void RendererPlugin::SetModuleHandle(IModuleHandle::unique_ptr moduleHandle)
{
	_moduleHandle = std::shared_ptr<IModuleHandle>(moduleHandle.release(), Deleter<IModuleHandle>{});
}

//----------------------------------------------------------------------------------------
// Allocation methods
//----------------------------------------------------------------------------------------
IGraphicsDeviceEnumerator::unique_ptr RendererPlugin::CreateGraphicsDeviceEnumerator(cobalt::logging::ILogger::unique_ptr log) const
{
	return IGraphicsDeviceEnumerator::unique_ptr(_allocator(log.release()));
}

//----------------------------------------------------------------------------------------
void RendererPlugin::SetAllocationFunction(AllocatorPointer allocator)
{
	_allocator = allocator;
}

}} // namespace cobalt::graphics
