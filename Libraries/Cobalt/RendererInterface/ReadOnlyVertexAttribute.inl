// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
namespace cobalt { namespace graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
ReadOnlyVertexAttribute::ReadOnlyVertexAttribute()
: _dataType(DataType::UInt8), _vertexCount(0), _elementCount(1), _dataPersistenceFlags(DataPersistenceFlags::PersistAlways)
{}

//----------------------------------------------------------------------------------------
// Type methods
//----------------------------------------------------------------------------------------
IVertexAttribute::DataType ReadOnlyVertexAttribute::GetDataType() const
{
	return _dataType;
}

//----------------------------------------------------------------------------------------
size_t ReadOnlyVertexAttribute::GetVertexCount() const
{
	return _vertexCount;
}

//----------------------------------------------------------------------------------------
size_t ReadOnlyVertexAttribute::GetAttributeElementCount() const
{
	return _elementCount;
}

//----------------------------------------------------------------------------------------
// Usage methods
//----------------------------------------------------------------------------------------
IVertexAttribute::PerformanceHint ReadOnlyVertexAttribute::GetPerformanceHintCpu() const
{
	return _performanceHintCpu;
}

//----------------------------------------------------------------------------------------
IVertexAttribute::PerformanceHint ReadOnlyVertexAttribute::GetPerformanceHintGpu() const
{
	return _performanceHintGpu;
}

//----------------------------------------------------------------------------------------
IVertexAttribute::DataPersistenceFlags ReadOnlyVertexAttribute::GetDataPersistenceFlags() const
{
	return _dataPersistenceFlags;
}

//----------------------------------------------------------------------------------------
// Binding methods
//----------------------------------------------------------------------------------------
bool ReadOnlyVertexAttribute::IsBoundToBuffer() const
{
	return _boundToVertexBuffer;
}

//----------------------------------------------------------------------------------------
void ReadOnlyVertexAttribute::SetVertexBufferInfo(IVertexBuffer* vertexBuffer, size_t attributeIndex)
{
	_vertexBuffer = vertexBuffer;
	_attributeIndex = attributeIndex;
	_boundToVertexBuffer = true;
}

//----------------------------------------------------------------------------------------
IVertexBuffer* ReadOnlyVertexAttribute::GetBoundVertexBuffer() const
{
	return _vertexBuffer;
}

//----------------------------------------------------------------------------------------
size_t ReadOnlyVertexAttribute::GetBufferAttributeIndex() const
{
	return _attributeIndex;
}

//----------------------------------------------------------------------------------------
void ReadOnlyVertexAttribute::SetVertexAttributeInfo(const IVertexAttribute& sourceAttribute)
{
	_dataType = sourceAttribute.GetDataType();
	_vertexCount = sourceAttribute.GetVertexCount();
	_elementCount = sourceAttribute.GetAttributeElementCount();
	_performanceHintCpu = sourceAttribute.GetPerformanceHintCpu();
	_performanceHintGpu = sourceAttribute.GetPerformanceHintGpu();
	_dataPersistenceFlags = sourceAttribute.GetDataPersistenceFlags();
	_vertexBuffer = IVertexAttribute::GetBoundVertexBuffer(sourceAttribute);
	_boundToVertexBuffer = (_vertexBuffer != nullptr);
	_attributeIndex = IVertexAttribute::GetBufferAttributeIndex(sourceAttribute);
}

}} // namespace cobalt::graphics
