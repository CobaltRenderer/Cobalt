// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include <type_traits>
namespace cobalt { namespace graphics {

//----------------------------------------------------------------------------------------
// Enumeration operators
//----------------------------------------------------------------------------------------
inline IResourceArray::PerformanceHint operator|(IResourceArray::PerformanceHint left, IResourceArray::PerformanceHint right)
{
	return (IResourceArray::PerformanceHint)((std::underlying_type<IResourceArray::PerformanceHint>::type)left | (std::underlying_type<IResourceArray::PerformanceHint>::type)right);
}

//----------------------------------------------------------------------------------------
inline IResourceArray::PerformanceHint& operator|=(IResourceArray::PerformanceHint& left, IResourceArray::PerformanceHint right)
{
	left = (left | right);
	return left;
}

//----------------------------------------------------------------------------------------
inline IResourceArray::PerformanceHint operator&(IResourceArray::PerformanceHint left, IResourceArray::PerformanceHint right)
{
	return (IResourceArray::PerformanceHint)((std::underlying_type<IResourceArray::PerformanceHint>::type)left & (std::underlying_type<IResourceArray::PerformanceHint>::type)right);
}

//----------------------------------------------------------------------------------------
inline IResourceArray::PerformanceHint& operator&=(IResourceArray::PerformanceHint& left, IResourceArray::PerformanceHint right)
{
	left = (left & right);
	return left;
}

//----------------------------------------------------------------------------------------
inline IResourceArray::DataPersistenceFlags operator|(IResourceArray::DataPersistenceFlags left, IResourceArray::DataPersistenceFlags right)
{
	return (IResourceArray::DataPersistenceFlags)((std::underlying_type<IResourceArray::DataPersistenceFlags>::type)left | (std::underlying_type<IResourceArray::DataPersistenceFlags>::type)right);
}

//----------------------------------------------------------------------------------------
inline IResourceArray::DataPersistenceFlags& operator|=(IResourceArray::DataPersistenceFlags& left, IResourceArray::DataPersistenceFlags right)
{
	left = (left | right);
	return left;
}

//----------------------------------------------------------------------------------------
inline IResourceArray::DataPersistenceFlags operator&(IResourceArray::DataPersistenceFlags left, IResourceArray::DataPersistenceFlags right)
{
	return (IResourceArray::DataPersistenceFlags)((std::underlying_type<IResourceArray::DataPersistenceFlags>::type)left & (std::underlying_type<IResourceArray::DataPersistenceFlags>::type)right);
}

//----------------------------------------------------------------------------------------
inline IResourceArray::DataPersistenceFlags& operator&=(IResourceArray::DataPersistenceFlags& left, IResourceArray::DataPersistenceFlags right)
{
	left = (left & right);
	return left;
}

}} // namespace cobalt::graphics
