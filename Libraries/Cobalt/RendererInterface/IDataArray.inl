// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include <type_traits>
namespace cobalt { namespace graphics {

//----------------------------------------------------------------------------------------
// Enumeration operators
//----------------------------------------------------------------------------------------
inline IDataArray::UsageFlags operator|(IDataArray::UsageFlags left, IDataArray::UsageFlags right)
{
	return (IDataArray::UsageFlags)((std::underlying_type<IDataArray::UsageFlags>::type)left | (std::underlying_type<IDataArray::UsageFlags>::type)right);
}

//----------------------------------------------------------------------------------------
inline IDataArray::UsageFlags& operator|=(IDataArray::UsageFlags& left, IDataArray::UsageFlags right)
{
	left = (left | right);
	return left;
}

//----------------------------------------------------------------------------------------
inline IDataArray::UsageFlags operator&(IDataArray::UsageFlags left, IDataArray::UsageFlags right)
{
	return (IDataArray::UsageFlags)((std::underlying_type<IDataArray::UsageFlags>::type)left & (std::underlying_type<IDataArray::UsageFlags>::type)right);
}

//----------------------------------------------------------------------------------------
inline IDataArray::UsageFlags& operator&=(IDataArray::UsageFlags& left, IDataArray::UsageFlags right)
{
	left = (left & right);
	return left;
}

}} // namespace cobalt::graphics
