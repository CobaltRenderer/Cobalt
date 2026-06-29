// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "IIndexBuffer.h"
#include "VectorTypes.h"
#include <Cobalt/Debug/Debug.pkg>
namespace cobalt { namespace graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
template<class T>
IndexAttribute<T>::IndexAttribute(size_t indexCount, PerformanceHint performanceHintCpu, PerformanceHint performanceHintGpu, DataPersistenceFlags dataPersistenceFlags)
: IndexAttribute()
{
	_indexCount = indexCount;
	_performanceHintCpu = performanceHintCpu;
	_performanceHintGpu = performanceHintGpu;
	_dataPersistenceFlags = dataPersistenceFlags;
}

//----------------------------------------------------------------------------------------
template<class T>
IndexAttribute<T>::IndexAttribute()
: _dataType(DataType::UInt16)
{
	static_assert(!std::is_same<T, T>::value, "IndexAttribute class instantiated with an unsupported type!");
}

//----------------------------------------------------------------------------------------
template<>
inline IndexAttribute<V1UInt16>::IndexAttribute()
: _dataType(DataType::UInt16)
{}

//----------------------------------------------------------------------------------------
template<>
inline IndexAttribute<V1UInt32>::IndexAttribute()
: _dataType(DataType::UInt32)
{}

//----------------------------------------------------------------------------------------
// Type methods
//----------------------------------------------------------------------------------------
template<class T>
IIndexAttribute::DataType IndexAttribute<T>::GetDataType() const
{
	return _dataType;
}

//----------------------------------------------------------------------------------------
template<class T>
size_t IndexAttribute<T>::GetIndexCount() const
{
	return _indexCount;
}

//----------------------------------------------------------------------------------------
template<class T>
T IndexAttribute<T>::GetPrimitiveRestartValue()
{
	UNREACHABLE();
}

//----------------------------------------------------------------------------------------
template<>
inline V1UInt16 IndexAttribute<V1UInt16>::GetPrimitiveRestartValue()
{
	return V1UInt16(0xFFFF);
}

//----------------------------------------------------------------------------------------
template<>
inline V1UInt32 IndexAttribute<V1UInt32>::GetPrimitiveRestartValue()
{
	return V1UInt32(0xFFFFFFFF);
}

//----------------------------------------------------------------------------------------
// Usage methods
//----------------------------------------------------------------------------------------
template<class T>
IIndexAttribute::PerformanceHint IndexAttribute<T>::GetPerformanceHintCpu() const
{
	return _performanceHintCpu;
}

//----------------------------------------------------------------------------------------
template<class T>
IIndexAttribute::PerformanceHint IndexAttribute<T>::GetPerformanceHintGpu() const
{
	return _performanceHintGpu;
}

//----------------------------------------------------------------------------------------
template<class T>
IIndexAttribute::DataPersistenceFlags IndexAttribute<T>::GetDataPersistenceFlags() const
{
	return _dataPersistenceFlags;
}

//----------------------------------------------------------------------------------------
// Binding methods
//----------------------------------------------------------------------------------------
template<class T>
bool IndexAttribute<T>::IsBoundToBuffer() const
{
	return _boundToIndexBuffer;
}

//----------------------------------------------------------------------------------------
template<class T>
void IndexAttribute<T>::SetIndexBufferInfo(IIndexBuffer* indexBuffer, size_t attributeIndex)
{
	_indexBuffer = indexBuffer;
	_attributeIndex = attributeIndex;
	_boundToIndexBuffer = true;
}

//----------------------------------------------------------------------------------------
template<class T>
IIndexBuffer* IndexAttribute<T>::GetBoundIndexBuffer() const
{
	return _indexBuffer;
}

//----------------------------------------------------------------------------------------
template<class T>
size_t IndexAttribute<T>::GetBufferAttributeIndex() const
{
	return _attributeIndex;
}

//----------------------------------------------------------------------------------------
// Data methods
//----------------------------------------------------------------------------------------
template<class T>
SuccessToken IndexAttribute<T>::SetInitialData(const T* data, size_t entryCount, size_t entryStrideInBytes)
{
	if (!_boundToIndexBuffer)
	{
		return false;
	}
	return SetInitialDataInternal(_indexBuffer, reinterpret_cast<const uint8_t*>(data), entryCount, entryStrideInBytes);
}

//----------------------------------------------------------------------------------------
template<class T>
SuccessToken IndexAttribute<T>::SetInitialData(const std::vector<T>& data)
{
	if (!_boundToIndexBuffer)
	{
		return false;
	}
	return SetInitialDataInternal(_indexBuffer, reinterpret_cast<const uint8_t*>(data.data()), data.size(), sizeof(T));
}

//----------------------------------------------------------------------------------------
template<class T>
SuccessToken IndexAttribute<T>::SetInitialData(std::initializer_list<T> data)
{
	if (!_boundToIndexBuffer)
	{
		return false;
	}
	return SetInitialDataInternal(_indexBuffer, reinterpret_cast<const uint8_t*>(data.begin()), data.size(), sizeof(T));
}

//----------------------------------------------------------------------------------------
template<class T>
SuccessToken IndexAttribute<T>::QueueDataUpdate(const T* data, size_t entryCount, size_t initialIndexNo, size_t entryStrideInBytes, ITransferBatch* transferBatch)
{
	if (!_boundToIndexBuffer)
	{
		return false;
	}
	return QueueDataUpdateInternal(_indexBuffer, reinterpret_cast<const uint8_t*>(data), entryCount, initialIndexNo, entryStrideInBytes, transferBatch);
}

}} // namespace cobalt::graphics
