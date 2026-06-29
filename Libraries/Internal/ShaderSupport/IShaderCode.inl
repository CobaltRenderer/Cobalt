// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
namespace internal {
#ifdef _WIN32
#ifdef SHADERSUPPORT_DLL
extern "C" __declspec(dllexport) IShaderCode* CreateIShaderCode(cobalt::logging::ILogger* log);
#else
extern "C" __declspec(dllimport) IShaderCode* CreateIShaderCode(cobalt::logging::ILogger* log);
#endif
#else
extern "C" __attribute__((visibility("default"))) IShaderCode* CreateIShaderCode(cobalt::logging::ILogger* log);
#endif
} // namespace internal

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
inline IShaderCode::unique_ptr IShaderCode::Create(cobalt::logging::ILogger::unique_ptr log)
{
	return unique_ptr(internal::CreateIShaderCode(log.release()));
}

//----------------------------------------------------------------------------------------
// Enumeration operators
//----------------------------------------------------------------------------------------
inline IShaderCode::Stage operator|(IShaderCode::Stage left, IShaderCode::Stage right)
{
	return (IShaderCode::Stage)((std::underlying_type<IShaderCode::Stage>::type)left | (std::underlying_type<IShaderCode::Stage>::type)right);
}

//----------------------------------------------------------------------------------------
inline IShaderCode::Stage& operator|=(IShaderCode::Stage& left, IShaderCode::Stage right)
{
	left = (left | right);
	return left;
}

//----------------------------------------------------------------------------------------
inline IShaderCode::Stage operator&(IShaderCode::Stage left, IShaderCode::Stage right)
{
	return (IShaderCode::Stage)((std::underlying_type<IShaderCode::Stage>::type)left & (std::underlying_type<IShaderCode::Stage>::type)right);
}

//----------------------------------------------------------------------------------------
inline IShaderCode::Stage& operator&=(IShaderCode::Stage& left, IShaderCode::Stage right)
{
	left = (left & right);
	return left;
}

} // namespace cobalt::graphics
