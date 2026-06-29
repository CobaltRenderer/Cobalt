// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
namespace cobalt { namespace graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
ReadOnlyIndexAttribute::ReadOnlyIndexAttribute()
: _dataType(DataType::UInt16), _indexCount(0), _dataPersistenceFlags(DataPersistenceFlags::PersistAlways)
{}

//----------------------------------------------------------------------------------------
// Type methods
//----------------------------------------------------------------------------------------
IIndexAttribute::DataType ReadOnlyIndexAttribute::GetDataType() const
{
	return _dataType;
}

//----------------------------------------------------------------------------------------
size_t ReadOnlyIndexAttribute::GetIndexCount() const
{
	return _indexCount;
}

//----------------------------------------------------------------------------------------
// Usage methods
//----------------------------------------------------------------------------------------
IIndexAttribute::PerformanceHint ReadOnlyIndexAttribute::GetPerformanceHintCpu() const
{
	return _performanceHintCpu;
}

//----------------------------------------------------------------------------------------
IIndexAttribute::PerformanceHint ReadOnlyIndexAttribute::GetPerformanceHintGpu() const
{
	return _performanceHintGpu;
}

//----------------------------------------------------------------------------------------
IIndexAttribute::DataPersistenceFlags ReadOnlyIndexAttribute::GetDataPersistenceFlags() const
{
	return _dataPersistenceFlags;
}

//----------------------------------------------------------------------------------------
// Binding methods
//----------------------------------------------------------------------------------------
bool ReadOnlyIndexAttribute::IsBoundToBuffer() const
{
	return _boundToIndexBuffer;
}

//----------------------------------------------------------------------------------------
void ReadOnlyIndexAttribute::SetIndexBufferInfo(IIndexBuffer* indexBuffer, size_t attributeIndex)
{
	_indexBuffer = indexBuffer;
	_attributeIndex = attributeIndex;
	_boundToIndexBuffer = true;
}

//----------------------------------------------------------------------------------------
IIndexBuffer* ReadOnlyIndexAttribute::GetBoundIndexBuffer() const
{
	return _indexBuffer;
}

//----------------------------------------------------------------------------------------
size_t ReadOnlyIndexAttribute::GetBufferAttributeIndex() const
{
	return _attributeIndex;
}

//----------------------------------------------------------------------------------------
void ReadOnlyIndexAttribute::SetIndexAttributeInfo(const IIndexAttribute& sourceAttribute)
{
	_dataType = sourceAttribute.GetDataType();
	_indexCount = sourceAttribute.GetIndexCount();
	_performanceHintCpu = sourceAttribute.GetPerformanceHintCpu();
	_performanceHintGpu = sourceAttribute.GetPerformanceHintGpu();
	_dataPersistenceFlags = sourceAttribute.GetDataPersistenceFlags();
	_indexBuffer = IIndexAttribute::GetBoundIndexBuffer(sourceAttribute);
	_boundToIndexBuffer = (_indexBuffer != nullptr);
	_attributeIndex = IIndexAttribute::GetBufferAttributeIndex(sourceAttribute);
}

}} // namespace cobalt::graphics
