// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
namespace cobalt { namespace graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
RawVertexAttribute::RawVertexAttribute(DataType dataType, size_t elementCount, size_t vertexCount, PerformanceHint performanceHintCpu, PerformanceHint performanceHintGpu, DataPersistenceFlags dataPersistenceFlags)
: RawVertexAttribute()
{
	_dataType = dataType;
	_vertexCount = vertexCount;
	_elementCount = elementCount;
	_performanceHintCpu = performanceHintCpu;
	_performanceHintGpu = performanceHintGpu;
	_dataPersistenceFlags = dataPersistenceFlags;
}

//----------------------------------------------------------------------------------------
RawVertexAttribute::RawVertexAttribute()
: _dataType(DataType::UInt8), _elementCount(1)
{}

//----------------------------------------------------------------------------------------
// Type methods
//----------------------------------------------------------------------------------------
IVertexAttribute::DataType RawVertexAttribute::GetDataType() const
{
	return _dataType;
}

//----------------------------------------------------------------------------------------
size_t RawVertexAttribute::GetVertexCount() const
{
	return _vertexCount;
}

//----------------------------------------------------------------------------------------
size_t RawVertexAttribute::GetAttributeElementCount() const
{
	return _elementCount;
}

//----------------------------------------------------------------------------------------
// Usage methods
//----------------------------------------------------------------------------------------
IVertexAttribute::PerformanceHint RawVertexAttribute::GetPerformanceHintCpu() const
{
	return _performanceHintCpu;
}

//----------------------------------------------------------------------------------------
IVertexAttribute::PerformanceHint RawVertexAttribute::GetPerformanceHintGpu() const
{
	return _performanceHintGpu;
}

//----------------------------------------------------------------------------------------
IVertexAttribute::DataPersistenceFlags RawVertexAttribute::GetDataPersistenceFlags() const
{
	return _dataPersistenceFlags;
}

//----------------------------------------------------------------------------------------
// Binding methods
//----------------------------------------------------------------------------------------
void RawVertexAttribute::SetVertexBufferInfo(IVertexBuffer* vertexBuffer, size_t attributeIndex)
{
	_vertexBuffer = vertexBuffer;
	_attributeIndex = attributeIndex;
	_boundToVertexBuffer = true;
}

//----------------------------------------------------------------------------------------
IVertexBuffer* RawVertexAttribute::GetBoundVertexBuffer() const
{
	return _vertexBuffer;
}

//----------------------------------------------------------------------------------------
size_t RawVertexAttribute::GetBufferAttributeIndex() const
{
	return _attributeIndex;
}

//----------------------------------------------------------------------------------------
// Binding methods
//----------------------------------------------------------------------------------------
bool RawVertexAttribute::IsBoundToBuffer() const
{
	return _boundToVertexBuffer;
}

//----------------------------------------------------------------------------------------
void RawVertexAttribute::SetVertexAttributeInfo(const IVertexAttribute& sourceAttribute)
{
	_dataType = sourceAttribute.GetDataType();
	_vertexCount = sourceAttribute.GetVertexCount();
	_elementCount = sourceAttribute.GetAttributeElementCount();
	_performanceHintCpu = sourceAttribute.GetPerformanceHintCpu();
	_performanceHintGpu = sourceAttribute.GetPerformanceHintGpu();
	_vertexBuffer = IVertexAttribute::GetBoundVertexBuffer(sourceAttribute);
	_boundToVertexBuffer = (_vertexBuffer != nullptr);
	_attributeIndex = IVertexAttribute::GetBufferAttributeIndex(sourceAttribute);
}

//----------------------------------------------------------------------------------------
// Data methods
//----------------------------------------------------------------------------------------
SuccessToken RawVertexAttribute::SetInitialData(const uint8_t* data, size_t entryCount, size_t entryStrideInBytes)
{
	if (!_boundToVertexBuffer)
	{
		return false;
	}
	return SetInitialDataInternal(_vertexBuffer, _attributeIndex, data, entryCount, entryStrideInBytes);
}

//----------------------------------------------------------------------------------------
SuccessToken RawVertexAttribute::QueueDataUpdate(const uint8_t* data, size_t entryCount, size_t initialVertexNo, size_t entryStrideInBytes, ITransferBatch* transferBatch)
{
	if (!_boundToVertexBuffer)
	{
		return false;
	}
	return QueueDataUpdateInternal(_vertexBuffer, _attributeIndex, data, entryCount, initialVertexNo, entryStrideInBytes, transferBatch);
}

}} // namespace cobalt::graphics
