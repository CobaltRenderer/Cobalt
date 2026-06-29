// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include <type_traits>
namespace cobalt { namespace graphics {

//----------------------------------------------------------------------------------------
// Enumeration operators
//----------------------------------------------------------------------------------------
inline IGraphicsDeviceEnumerator::EnumerationFlags operator|(IGraphicsDeviceEnumerator::EnumerationFlags left, IGraphicsDeviceEnumerator::EnumerationFlags right)
{
	return (IGraphicsDeviceEnumerator::EnumerationFlags)((std::underlying_type<IGraphicsDeviceEnumerator::EnumerationFlags>::type)left | (std::underlying_type<IGraphicsDeviceEnumerator::EnumerationFlags>::type)right);
}

//----------------------------------------------------------------------------------------
inline IGraphicsDeviceEnumerator::EnumerationFlags& operator|=(IGraphicsDeviceEnumerator::EnumerationFlags& left, IGraphicsDeviceEnumerator::EnumerationFlags right)
{
	left = (left | right);
	return left;
}

//----------------------------------------------------------------------------------------
inline IGraphicsDeviceEnumerator::EnumerationFlags operator&(IGraphicsDeviceEnumerator::EnumerationFlags left, IGraphicsDeviceEnumerator::EnumerationFlags right)
{
	return (IGraphicsDeviceEnumerator::EnumerationFlags)((std::underlying_type<IGraphicsDeviceEnumerator::EnumerationFlags>::type)left & (std::underlying_type<IGraphicsDeviceEnumerator::EnumerationFlags>::type)right);
}

//----------------------------------------------------------------------------------------
inline IGraphicsDeviceEnumerator::EnumerationFlags& operator&=(IGraphicsDeviceEnumerator::EnumerationFlags& left, IGraphicsDeviceEnumerator::EnumerationFlags right)
{
	left = (left & right);
	return left;
}

}} // namespace cobalt::graphics
