// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "IVertexBuffer.h"
#include "VectorTypes.h"
namespace cobalt { namespace graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
template<class T>
VertexAttribute<T>::VertexAttribute(size_t vertexCount, PerformanceHint performanceHintCpu, PerformanceHint performanceHintGpu, DataPersistenceFlags dataPersistenceFlags)
: VertexAttribute()
{
	_vertexCount = vertexCount;
	_performanceHintCpu = performanceHintCpu;
	_performanceHintGpu = performanceHintGpu;
	_dataPersistenceFlags = dataPersistenceFlags;
}

//----------------------------------------------------------------------------------------
template<class T>
VertexAttribute<T>::VertexAttribute()
: _dataType(DataType::Int8)
{
	static_assert(!std::is_same<T, T>::value, "VertexAttribute class instantiated with an unsupported type!");
}

//----------------------------------------------------------------------------------------
template<>
inline VertexAttribute<V1Int8>::VertexAttribute()
: _dataType(DataType::Int8), _elementCount(1)
{}

//----------------------------------------------------------------------------------------
template<>
inline VertexAttribute<V1Int16>::VertexAttribute()
: _dataType(DataType::Int16), _elementCount(1)
{}

//----------------------------------------------------------------------------------------
template<>
inline VertexAttribute<V1Int32>::VertexAttribute()
: _dataType(DataType::Int32), _elementCount(1)
{}

//----------------------------------------------------------------------------------------
template<>
inline VertexAttribute<V1UInt8>::VertexAttribute()
: _dataType(DataType::UInt8), _elementCount(1)
{}

//----------------------------------------------------------------------------------------
template<>
inline VertexAttribute<V1UInt16>::VertexAttribute()
: _dataType(DataType::UInt16), _elementCount(1)
{}

//----------------------------------------------------------------------------------------
template<>
inline VertexAttribute<V1UInt32>::VertexAttribute()
: _dataType(DataType::UInt32), _elementCount(1)
{}

//----------------------------------------------------------------------------------------
template<>
inline VertexAttribute<V1Norm8>::VertexAttribute()
: _dataType(DataType::Norm8), _elementCount(1)
{}

//----------------------------------------------------------------------------------------
template<>
inline VertexAttribute<V1Norm16>::VertexAttribute()
: _dataType(DataType::Norm16), _elementCount(1)
{}

//----------------------------------------------------------------------------------------
template<>
inline VertexAttribute<V1UNorm8>::VertexAttribute()
: _dataType(DataType::UNorm8), _elementCount(1)
{}

//----------------------------------------------------------------------------------------
template<>
inline VertexAttribute<V1UNorm16>::VertexAttribute()
: _dataType(DataType::UNorm16), _elementCount(1)
{}

//----------------------------------------------------------------------------------------
template<>
inline VertexAttribute<V1Float16>::VertexAttribute()
: _dataType(DataType::Float16), _elementCount(1)
{}

//----------------------------------------------------------------------------------------
template<>
inline VertexAttribute<V1Float32>::VertexAttribute()
: _dataType(DataType::Float32), _elementCount(1)
{}

//----------------------------------------------------------------------------------------
template<>
inline VertexAttribute<V2Int8>::VertexAttribute()
: _dataType(DataType::Int8), _elementCount(2)
{}

//----------------------------------------------------------------------------------------
template<>
inline VertexAttribute<V2Int16>::VertexAttribute()
: _dataType(DataType::Int16), _elementCount(2)
{}

//----------------------------------------------------------------------------------------
template<>
inline VertexAttribute<V2Int32>::VertexAttribute()
: _dataType(DataType::Int32), _elementCount(2)
{}

//----------------------------------------------------------------------------------------
template<>
inline VertexAttribute<V2UInt8>::VertexAttribute()
: _dataType(DataType::UInt8), _elementCount(2)
{}

//----------------------------------------------------------------------------------------
template<>
inline VertexAttribute<V2UInt16>::VertexAttribute()
: _dataType(DataType::UInt16), _elementCount(2)
{}

//----------------------------------------------------------------------------------------
template<>
inline VertexAttribute<V2UInt32>::VertexAttribute()
: _dataType(DataType::UInt32), _elementCount(2)
{}

//----------------------------------------------------------------------------------------
template<>
inline VertexAttribute<V2Norm8>::VertexAttribute()
: _dataType(DataType::Norm8), _elementCount(2)
{}

//----------------------------------------------------------------------------------------
template<>
inline VertexAttribute<V2Norm16>::VertexAttribute()
: _dataType(DataType::Norm16), _elementCount(2)
{}

//----------------------------------------------------------------------------------------
template<>
inline VertexAttribute<V2UNorm8>::VertexAttribute()
: _dataType(DataType::UNorm8), _elementCount(2)
{}

//----------------------------------------------------------------------------------------
template<>
inline VertexAttribute<V2UNorm16>::VertexAttribute()
: _dataType(DataType::UNorm16), _elementCount(2)
{}

//----------------------------------------------------------------------------------------
template<>
inline VertexAttribute<V2Float16>::VertexAttribute()
: _dataType(DataType::Float16), _elementCount(2)
{}

//----------------------------------------------------------------------------------------
template<>
inline VertexAttribute<V2Float32>::VertexAttribute()
: _dataType(DataType::Float32), _elementCount(2)
{}

//----------------------------------------------------------------------------------------
template<>
inline VertexAttribute<V3Int8>::VertexAttribute()
: _dataType(DataType::Int8), _elementCount(3)
{}

//----------------------------------------------------------------------------------------
template<>
inline VertexAttribute<V3Int16>::VertexAttribute()
: _dataType(DataType::Int16), _elementCount(3)
{}

//----------------------------------------------------------------------------------------
template<>
inline VertexAttribute<V3Int32>::VertexAttribute()
: _dataType(DataType::Int32), _elementCount(3)
{}

//----------------------------------------------------------------------------------------
template<>
inline VertexAttribute<V3UInt8>::VertexAttribute()
: _dataType(DataType::UInt8), _elementCount(3)
{}

//----------------------------------------------------------------------------------------
template<>
inline VertexAttribute<V3UInt16>::VertexAttribute()
: _dataType(DataType::UInt16), _elementCount(3)
{}

//----------------------------------------------------------------------------------------
template<>
inline VertexAttribute<V3UInt32>::VertexAttribute()
: _dataType(DataType::UInt32), _elementCount(3)
{}

//----------------------------------------------------------------------------------------
template<>
inline VertexAttribute<V3Norm8>::VertexAttribute()
: _dataType(DataType::Norm8), _elementCount(3)
{}

//----------------------------------------------------------------------------------------
template<>
inline VertexAttribute<V3Norm16>::VertexAttribute()
: _dataType(DataType::Norm16), _elementCount(3)
{}

//----------------------------------------------------------------------------------------
template<>
inline VertexAttribute<V3UNorm8>::VertexAttribute()
: _dataType(DataType::UNorm8), _elementCount(3)
{}

//----------------------------------------------------------------------------------------
template<>
inline VertexAttribute<V3UNorm16>::VertexAttribute()
: _dataType(DataType::UNorm16), _elementCount(3)
{}

//----------------------------------------------------------------------------------------
template<>
inline VertexAttribute<V3Float16>::VertexAttribute()
: _dataType(DataType::Float16), _elementCount(3)
{}

//----------------------------------------------------------------------------------------
template<>
inline VertexAttribute<V3Float32>::VertexAttribute()
: _dataType(DataType::Float32), _elementCount(3)
{}

//----------------------------------------------------------------------------------------
template<>
inline VertexAttribute<V4Int8>::VertexAttribute()
: _dataType(DataType::Int8), _elementCount(4)
{}

//----------------------------------------------------------------------------------------
template<>
inline VertexAttribute<V4Int16>::VertexAttribute()
: _dataType(DataType::Int16), _elementCount(4)
{}

//----------------------------------------------------------------------------------------
template<>
inline VertexAttribute<V4Int32>::VertexAttribute()
: _dataType(DataType::Int32), _elementCount(4)
{}

//----------------------------------------------------------------------------------------
template<>
inline VertexAttribute<V4UInt8>::VertexAttribute()
: _dataType(DataType::UInt8), _elementCount(4)
{}

//----------------------------------------------------------------------------------------
template<>
inline VertexAttribute<V4UInt16>::VertexAttribute()
: _dataType(DataType::UInt16), _elementCount(4)
{}

//----------------------------------------------------------------------------------------
template<>
inline VertexAttribute<V4UInt32>::VertexAttribute()
: _dataType(DataType::UInt32), _elementCount(4)
{}

//----------------------------------------------------------------------------------------
template<>
inline VertexAttribute<V4Norm8>::VertexAttribute()
: _dataType(DataType::Norm8), _elementCount(4)
{}

//----------------------------------------------------------------------------------------
template<>
inline VertexAttribute<V4Norm16>::VertexAttribute()
: _dataType(DataType::Norm16), _elementCount(4)
{}

//----------------------------------------------------------------------------------------
template<>
inline VertexAttribute<V4UNorm8>::VertexAttribute()
: _dataType(DataType::UNorm8), _elementCount(4)
{}

//----------------------------------------------------------------------------------------
template<>
inline VertexAttribute<V4UNorm16>::VertexAttribute()
: _dataType(DataType::UNorm16), _elementCount(4)
{}

//----------------------------------------------------------------------------------------
template<>
inline VertexAttribute<V4Float16>::VertexAttribute()
: _dataType(DataType::Float16), _elementCount(4)
{}

//----------------------------------------------------------------------------------------
template<>
inline VertexAttribute<V4Float32>::VertexAttribute()
: _dataType(DataType::Float32), _elementCount(4)
{}

//----------------------------------------------------------------------------------------
// Type methods
//----------------------------------------------------------------------------------------
template<class T>
IVertexAttribute::DataType VertexAttribute<T>::GetDataType() const
{
	return _dataType;
}

//----------------------------------------------------------------------------------------
template<class T>
size_t VertexAttribute<T>::GetVertexCount() const
{
	return _vertexCount;
}

//----------------------------------------------------------------------------------------
template<class T>
size_t VertexAttribute<T>::GetAttributeElementCount() const
{
	return _elementCount;
}

//----------------------------------------------------------------------------------------
// Usage methods
//----------------------------------------------------------------------------------------
template<class T>
IVertexAttribute::PerformanceHint VertexAttribute<T>::GetPerformanceHintCpu() const
{
	return _performanceHintCpu;
}

//----------------------------------------------------------------------------------------
template<class T>
IVertexAttribute::PerformanceHint VertexAttribute<T>::GetPerformanceHintGpu() const
{
	return _performanceHintGpu;
}

//----------------------------------------------------------------------------------------
template<class T>
IVertexAttribute::DataPersistenceFlags VertexAttribute<T>::GetDataPersistenceFlags() const
{
	return _dataPersistenceFlags;
}

//----------------------------------------------------------------------------------------
// Binding methods
//----------------------------------------------------------------------------------------
template<class T>
bool VertexAttribute<T>::IsBoundToBuffer() const
{
	return _boundToVertexBuffer;
}

//----------------------------------------------------------------------------------------
template<class T>
void VertexAttribute<T>::SetVertexBufferInfo(IVertexBuffer* vertexBuffer, size_t attributeIndex)
{
	_vertexBuffer = vertexBuffer;
	_attributeIndex = attributeIndex;
	_boundToVertexBuffer = true;
}

//----------------------------------------------------------------------------------------
template<class T>
IVertexBuffer* VertexAttribute<T>::GetBoundVertexBuffer() const
{
	return _vertexBuffer;
}

//----------------------------------------------------------------------------------------
template<class T>
size_t VertexAttribute<T>::GetBufferAttributeIndex() const
{
	return _attributeIndex;
}

//----------------------------------------------------------------------------------------
// Data methods
//----------------------------------------------------------------------------------------
template<class T>
SuccessToken VertexAttribute<T>::SetInitialData(const T* data, size_t entryCount, size_t entryStrideInBytes)
{
	if (!_boundToVertexBuffer)
	{
		return false;
	}
	return SetInitialDataInternal(_vertexBuffer, _attributeIndex, reinterpret_cast<const uint8_t*>(data), entryCount, entryStrideInBytes);
}

//----------------------------------------------------------------------------------------
template<class T>
SuccessToken VertexAttribute<T>::SetInitialData(const std::vector<T>& data)
{
	if (!_boundToVertexBuffer)
	{
		return false;
	}
	return SetInitialDataInternal(_vertexBuffer, _attributeIndex, reinterpret_cast<const uint8_t*>(data.data()), data.size(), sizeof(T));
}

//----------------------------------------------------------------------------------------
template<class T>
SuccessToken VertexAttribute<T>::SetInitialData(std::initializer_list<T> data)
{
	if (!_boundToVertexBuffer)
	{
		return false;
	}
	return SetInitialDataInternal(_vertexBuffer, _attributeIndex, reinterpret_cast<const uint8_t*>(data.begin()), data.size(), sizeof(T));
}

//----------------------------------------------------------------------------------------
template<class T>
SuccessToken VertexAttribute<T>::QueueDataUpdate(const T* data, size_t entryCount, size_t initialVertexNo, size_t entryStrideInBytes, ITransferBatch* transferBatch)
{
	if (!_boundToVertexBuffer)
	{
		return false;
	}
	return QueueDataUpdateInternal(_vertexBuffer, _attributeIndex, reinterpret_cast<const uint8_t*>(data), entryCount, initialVertexNo, entryStrideInBytes, transferBatch);
}

}} // namespace cobalt::graphics
