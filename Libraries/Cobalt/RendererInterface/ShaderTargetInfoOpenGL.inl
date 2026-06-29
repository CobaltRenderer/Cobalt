// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include <type_traits>
namespace cobalt { namespace graphics {

//----------------------------------------------------------------------------------------
// Enumeration operators
//----------------------------------------------------------------------------------------
inline ShaderTargetInfoOpenGL::Flags operator|(ShaderTargetInfoOpenGL::Flags left, ShaderTargetInfoOpenGL::Flags right)
{
	return (ShaderTargetInfoOpenGL::Flags)((std::underlying_type<ShaderTargetInfoOpenGL::Flags>::type)left | (std::underlying_type<ShaderTargetInfoOpenGL::Flags>::type)right);
}

//----------------------------------------------------------------------------------------
inline ShaderTargetInfoOpenGL::Flags& operator|=(ShaderTargetInfoOpenGL::Flags& left, ShaderTargetInfoOpenGL::Flags right)
{
	left = (left | right);
	return left;
}

//----------------------------------------------------------------------------------------
inline ShaderTargetInfoOpenGL::Flags operator&(ShaderTargetInfoOpenGL::Flags left, ShaderTargetInfoOpenGL::Flags right)
{
	return (ShaderTargetInfoOpenGL::Flags)((std::underlying_type<ShaderTargetInfoOpenGL::Flags>::type)left & (std::underlying_type<ShaderTargetInfoOpenGL::Flags>::type)right);
}

//----------------------------------------------------------------------------------------
inline ShaderTargetInfoOpenGL::Flags& operator&=(ShaderTargetInfoOpenGL::Flags& left, ShaderTargetInfoOpenGL::Flags right)
{
	left = (left & right);
	return left;
}

}} // namespace cobalt::graphics
