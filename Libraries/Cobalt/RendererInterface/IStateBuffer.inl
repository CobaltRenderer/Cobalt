// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include <array>
#include <type_traits>
namespace cobalt { namespace graphics {

//----------------------------------------------------------------------------------------
// State value methods
//----------------------------------------------------------------------------------------
template<class T>
void IStateBuffer::SetStateValue(StateValueId stateId, const T& value)
{
	SetStateValueForPage(0, stateId, value, nullptr, 0);
}

//----------------------------------------------------------------------------------------
template<class T, class... IndexTs>
void IStateBuffer::SetStateValue(StateValueId stateId, const T& value, size_t firstArrayIndex, IndexTs... additionalArrayIndices)
{
#if (defined(__cplusplus) && (__cplusplus >= 201703L)) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 201703L))
	static_assert((std::is_convertible_v<IndexTs, size_t> && ...), "IStateBuffer::SetStateValue called with an array index that was not convertible to size_t");
#endif
	std::array<size_t, 1 + sizeof...(additionalArrayIndices)> indices{firstArrayIndex, (size_t)additionalArrayIndices...};
	SetStateValueForPage(0, stateId, value, indices.data(), indices.size());
}

//----------------------------------------------------------------------------------------
template<class T>
void IStateBuffer::SetStateValue(StateValueId stateId, const T& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueForPage(0, stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<class T>
void IStateBuffer::SetStateValueForPage(uint32_t pageNo, StateValueId stateId, const T& value)
{
	SetStateValueForPage(pageNo, stateId, value, nullptr, 0);
}

//----------------------------------------------------------------------------------------
template<class T, class... IndexTs>
void IStateBuffer::SetStateValueForPage(uint32_t pageNo, StateValueId stateId, const T& value, size_t firstArrayIndex, IndexTs... additionalArrayIndices)
{
#if (defined(__cplusplus) && (__cplusplus >= 201703L)) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 201703L))
	static_assert((std::is_convertible_v<IndexTs, size_t> && ...), "IStateBuffer::SetStateValue called with an array index that was not convertible to size_t");
#endif
	std::array<size_t, 1 + sizeof...(additionalArrayIndices)> indices{firstArrayIndex, (size_t)additionalArrayIndices...};
	SetStateValueForPage(pageNo, stateId, value, indices.data(), indices.size());
}

//----------------------------------------------------------------------------------------
template<class T>
void IStateBuffer::SetStateValueForPage(uint32_t pageNo, StateValueId stateId, const T& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	static_assert(!std::is_same<T, T>::value, "IStateBuffer::SetStateValue called with an unsupported type!");
}

//----------------------------------------------------------------------------------------
template<>
inline void IStateBuffer::SetStateValueForPage(uint32_t pageNo, StateValueId stateId, const bool& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IStateBuffer::SetStateValueForPage(uint32_t pageNo, StateValueId stateId, const V1Int8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IStateBuffer::SetStateValueForPage(uint32_t pageNo, StateValueId stateId, const V1Int16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IStateBuffer::SetStateValueForPage(uint32_t pageNo, StateValueId stateId, const V1Int32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IStateBuffer::SetStateValueForPage(uint32_t pageNo, StateValueId stateId, const V1UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IStateBuffer::SetStateValueForPage(uint32_t pageNo, StateValueId stateId, const V1UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IStateBuffer::SetStateValueForPage(uint32_t pageNo, StateValueId stateId, const V1UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IStateBuffer::SetStateValueForPage(uint32_t pageNo, StateValueId stateId, const V1Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IStateBuffer::SetStateValueForPage(uint32_t pageNo, StateValueId stateId, const V1Float64& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IStateBuffer::SetStateValueForPage(uint32_t pageNo, StateValueId stateId, const V2Int8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IStateBuffer::SetStateValueForPage(uint32_t pageNo, StateValueId stateId, const V2Int16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IStateBuffer::SetStateValueForPage(uint32_t pageNo, StateValueId stateId, const V2Int32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IStateBuffer::SetStateValueForPage(uint32_t pageNo, StateValueId stateId, const V2UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IStateBuffer::SetStateValueForPage(uint32_t pageNo, StateValueId stateId, const V2UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IStateBuffer::SetStateValueForPage(uint32_t pageNo, StateValueId stateId, const V2UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IStateBuffer::SetStateValueForPage(uint32_t pageNo, StateValueId stateId, const V2Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IStateBuffer::SetStateValueForPage(uint32_t pageNo, StateValueId stateId, const V2Float64& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IStateBuffer::SetStateValueForPage(uint32_t pageNo, StateValueId stateId, const V3Int8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IStateBuffer::SetStateValueForPage(uint32_t pageNo, StateValueId stateId, const V3Int16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IStateBuffer::SetStateValueForPage(uint32_t pageNo, StateValueId stateId, const V3Int32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IStateBuffer::SetStateValueForPage(uint32_t pageNo, StateValueId stateId, const V3UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IStateBuffer::SetStateValueForPage(uint32_t pageNo, StateValueId stateId, const V3UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IStateBuffer::SetStateValueForPage(uint32_t pageNo, StateValueId stateId, const V3UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IStateBuffer::SetStateValueForPage(uint32_t pageNo, StateValueId stateId, const V3Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IStateBuffer::SetStateValueForPage(uint32_t pageNo, StateValueId stateId, const V3Float64& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IStateBuffer::SetStateValueForPage(uint32_t pageNo, StateValueId stateId, const V4Int8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IStateBuffer::SetStateValueForPage(uint32_t pageNo, StateValueId stateId, const V4Int16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IStateBuffer::SetStateValueForPage(uint32_t pageNo, StateValueId stateId, const V4Int32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IStateBuffer::SetStateValueForPage(uint32_t pageNo, StateValueId stateId, const V4UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IStateBuffer::SetStateValueForPage(uint32_t pageNo, StateValueId stateId, const V4UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IStateBuffer::SetStateValueForPage(uint32_t pageNo, StateValueId stateId, const V4UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IStateBuffer::SetStateValueForPage(uint32_t pageNo, StateValueId stateId, const V4Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IStateBuffer::SetStateValueForPage(uint32_t pageNo, StateValueId stateId, const V4Float64& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IStateBuffer::SetStateValueForPage(uint32_t pageNo, StateValueId stateId, const M2Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IStateBuffer::SetStateValueForPage(uint32_t pageNo, StateValueId stateId, const M3Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IStateBuffer::SetStateValueForPage(uint32_t pageNo, StateValueId stateId, const M4Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetStateValueInternal(pageNo, stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
// Enumeration operators
//----------------------------------------------------------------------------------------
inline IStateBuffer::PerformanceHint operator|(IStateBuffer::PerformanceHint left, IStateBuffer::PerformanceHint right)
{
	return (IStateBuffer::PerformanceHint)((std::underlying_type<IStateBuffer::PerformanceHint>::type)left | (std::underlying_type<IStateBuffer::PerformanceHint>::type)right);
}

//----------------------------------------------------------------------------------------
inline IStateBuffer::PerformanceHint& operator|=(IStateBuffer::PerformanceHint& left, IStateBuffer::PerformanceHint right)
{
	left = (left | right);
	return left;
}

//----------------------------------------------------------------------------------------
inline IStateBuffer::PerformanceHint operator&(IStateBuffer::PerformanceHint left, IStateBuffer::PerformanceHint right)
{
	return (IStateBuffer::PerformanceHint)((std::underlying_type<IStateBuffer::PerformanceHint>::type)left & (std::underlying_type<IStateBuffer::PerformanceHint>::type)right);
}

//----------------------------------------------------------------------------------------
inline IStateBuffer::PerformanceHint& operator&=(IStateBuffer::PerformanceHint& left, IStateBuffer::PerformanceHint right)
{
	left = (left & right);
	return left;
}

}} // namespace cobalt::graphics
