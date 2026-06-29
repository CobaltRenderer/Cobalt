// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include <type_traits>
namespace cobalt { namespace graphics {

//----------------------------------------------------------------------------------------
// Enumeration operators
//----------------------------------------------------------------------------------------
inline IFrameBuffer::WindowBindingFlags operator|(IFrameBuffer::WindowBindingFlags left, IFrameBuffer::WindowBindingFlags right)
{
	return (IFrameBuffer::WindowBindingFlags)((std::underlying_type<IFrameBuffer::WindowBindingFlags>::type)left | (std::underlying_type<IFrameBuffer::WindowBindingFlags>::type)right);
}

//----------------------------------------------------------------------------------------
inline IFrameBuffer::WindowBindingFlags& operator|=(IFrameBuffer::WindowBindingFlags& left, IFrameBuffer::WindowBindingFlags right)
{
	left = (left | right);
	return left;
}

//----------------------------------------------------------------------------------------
inline IFrameBuffer::WindowBindingFlags operator&(IFrameBuffer::WindowBindingFlags left, IFrameBuffer::WindowBindingFlags right)
{
	return (IFrameBuffer::WindowBindingFlags)((std::underlying_type<IFrameBuffer::WindowBindingFlags>::type)left & (std::underlying_type<IFrameBuffer::WindowBindingFlags>::type)right);
}

//----------------------------------------------------------------------------------------
inline IFrameBuffer::WindowBindingFlags& operator&=(IFrameBuffer::WindowBindingFlags& left, IFrameBuffer::WindowBindingFlags right)
{
	left = (left & right);
	return left;
}

}} // namespace cobalt::graphics
