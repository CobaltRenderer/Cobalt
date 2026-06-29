// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include <type_traits>
namespace cobalt { namespace graphics {

//----------------------------------------------------------------------------------------
// Enumeration operators
//----------------------------------------------------------------------------------------
inline ShaderTargetInfoDirect3D::Flags operator|(ShaderTargetInfoDirect3D::Flags left, ShaderTargetInfoDirect3D::Flags right)
{
	return (ShaderTargetInfoDirect3D::Flags)((std::underlying_type<ShaderTargetInfoDirect3D::Flags>::type)left | (std::underlying_type<ShaderTargetInfoDirect3D::Flags>::type)right);
}

//----------------------------------------------------------------------------------------
inline ShaderTargetInfoDirect3D::Flags& operator|=(ShaderTargetInfoDirect3D::Flags& left, ShaderTargetInfoDirect3D::Flags right)
{
	left = (left | right);
	return left;
}

//----------------------------------------------------------------------------------------
inline ShaderTargetInfoDirect3D::Flags operator&(ShaderTargetInfoDirect3D::Flags left, ShaderTargetInfoDirect3D::Flags right)
{
	return (ShaderTargetInfoDirect3D::Flags)((std::underlying_type<ShaderTargetInfoDirect3D::Flags>::type)left & (std::underlying_type<ShaderTargetInfoDirect3D::Flags>::type)right);
}

//----------------------------------------------------------------------------------------
inline ShaderTargetInfoDirect3D::Flags& operator&=(ShaderTargetInfoDirect3D::Flags& left, ShaderTargetInfoDirect3D::Flags right)
{
	left = (left & right);
	return left;
}

}} // namespace cobalt::graphics
