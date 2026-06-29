// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include <array>
#include <type_traits>
namespace cobalt { namespace graphics {

//----------------------------------------------------------------------------------------
// Constant state value methods
//----------------------------------------------------------------------------------------
template<class T>
void IProgramNode::SetConstantStateValue(StateValueId stateId, const T& value)
{
	SetConstantStateValue(stateId, value, nullptr, 0);
}

//----------------------------------------------------------------------------------------
template<class T, class... IndexTs>
void IProgramNode::SetConstantStateValue(StateValueId stateId, const T& value, size_t firstArrayIndex, IndexTs... additionalArrayIndices)
{
#if (defined(__cplusplus) && (__cplusplus >= 201703L)) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 201703L))
	static_assert((std::is_convertible_v<IndexTs, size_t> && ...), "IProgramNode::SetConstantStateValue called with an array index that was not convertible to size_t");
#endif
	std::array<size_t, 1 + sizeof...(additionalArrayIndices)> indices{firstArrayIndex, (size_t)additionalArrayIndices...};
	SetConstantStateValue(stateId, value, indices.data(), indices.size());
}

//----------------------------------------------------------------------------------------
template<class T>
void IProgramNode::SetConstantStateValue(StateValueId stateId, const T& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	static_assert(!std::is_same<T, T>::value, "IProgramNode::SetConstantStateValue called with an unsupported type!");
}

//----------------------------------------------------------------------------------------
template<>
inline void IProgramNode::SetConstantStateValue(StateValueId stateId, const bool& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IProgramNode::SetConstantStateValue(StateValueId stateId, const V1Int8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IProgramNode::SetConstantStateValue(StateValueId stateId, const V1Int16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IProgramNode::SetConstantStateValue(StateValueId stateId, const V1Int32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IProgramNode::SetConstantStateValue(StateValueId stateId, const V1UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IProgramNode::SetConstantStateValue(StateValueId stateId, const V1UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IProgramNode::SetConstantStateValue(StateValueId stateId, const V1UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IProgramNode::SetConstantStateValue(StateValueId stateId, const V1Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IProgramNode::SetConstantStateValue(StateValueId stateId, const V1Float64& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IProgramNode::SetConstantStateValue(StateValueId stateId, const V2Int8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IProgramNode::SetConstantStateValue(StateValueId stateId, const V2Int16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IProgramNode::SetConstantStateValue(StateValueId stateId, const V2Int32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IProgramNode::SetConstantStateValue(StateValueId stateId, const V2UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IProgramNode::SetConstantStateValue(StateValueId stateId, const V2UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IProgramNode::SetConstantStateValue(StateValueId stateId, const V2UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IProgramNode::SetConstantStateValue(StateValueId stateId, const V2Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IProgramNode::SetConstantStateValue(StateValueId stateId, const V2Float64& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IProgramNode::SetConstantStateValue(StateValueId stateId, const V3Int8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IProgramNode::SetConstantStateValue(StateValueId stateId, const V3Int16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IProgramNode::SetConstantStateValue(StateValueId stateId, const V3Int32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IProgramNode::SetConstantStateValue(StateValueId stateId, const V3UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IProgramNode::SetConstantStateValue(StateValueId stateId, const V3UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IProgramNode::SetConstantStateValue(StateValueId stateId, const V3UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IProgramNode::SetConstantStateValue(StateValueId stateId, const V3Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IProgramNode::SetConstantStateValue(StateValueId stateId, const V3Float64& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IProgramNode::SetConstantStateValue(StateValueId stateId, const V4Int8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IProgramNode::SetConstantStateValue(StateValueId stateId, const V4Int16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IProgramNode::SetConstantStateValue(StateValueId stateId, const V4Int32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IProgramNode::SetConstantStateValue(StateValueId stateId, const V4UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IProgramNode::SetConstantStateValue(StateValueId stateId, const V4UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IProgramNode::SetConstantStateValue(StateValueId stateId, const V4UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IProgramNode::SetConstantStateValue(StateValueId stateId, const V4Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IProgramNode::SetConstantStateValue(StateValueId stateId, const V4Float64& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IProgramNode::SetConstantStateValue(StateValueId stateId, const M2Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IProgramNode::SetConstantStateValue(StateValueId stateId, const M3Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
template<>
inline void IProgramNode::SetConstantStateValue(StateValueId stateId, const M4Float32& value, const size_t* arrayIndices, size_t arrayIndexCount)
{
	SetConstantStateValueInternal(stateId, value, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void IProgramNode::ResetConstantStateValue(StateValueId stateId)
{
	ResetConstantStateValue(stateId, nullptr, 0);
}

//----------------------------------------------------------------------------------------
template<class... IndexTs>
void IProgramNode::ResetConstantStateValue(StateValueId stateId, size_t firstArrayIndex, IndexTs... additionalArrayIndices)
{
#if (defined(__cplusplus) && (__cplusplus >= 201703L)) || (defined(_MSVC_LANG) && (_MSVC_LANG >= 201703L))
	static_assert((std::is_convertible_v<IndexTs, size_t> && ...), "IProgramNode::ResetConstantStateValue called with an array index that was not convertible to size_t");
#endif
	std::array<size_t, 1 + sizeof...(additionalArrayIndices)> indices{firstArrayIndex, (size_t)additionalArrayIndices...};
	ResetConstantStateValue(stateId, indices.data(), indices.size());
}

}} // namespace cobalt::graphics
