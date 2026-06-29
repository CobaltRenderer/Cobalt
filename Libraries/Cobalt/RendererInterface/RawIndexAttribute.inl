// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
namespace cobalt { namespace graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
RawIndexAttribute::RawIndexAttribute(DataType dataType, size_t indexCount, PerformanceHint performanceHintCpu, PerformanceHint performanceHintGpu, DataPersistenceFlags dataPersistenceFlags)
: RawIndexAttribute()
{
	_dataType = dataType;
	_indexCount = indexCount;
	_performanceHintCpu = performanceHintCpu;
	_performanceHintGpu = performanceHintGpu;
	_dataPersistenceFlags = dataPersistenceFlags;
}

//----------------------------------------------------------------------------------------
RawIndexAttribute::RawIndexAttribute()
: _dataType(DataType::UInt16)
{}

//----------------------------------------------------------------------------------------
// Type methods
//----------------------------------------------------------------------------------------
IIndexAttribute::DataType RawIndexAttribute::GetDataType() const
{
	return _dataType;
}

//----------------------------------------------------------------------------------------
size_t RawIndexAttribute::GetIndexCount() const
{
	return _indexCount;
}

//----------------------------------------------------------------------------------------
// Usage methods
//----------------------------------------------------------------------------------------
IIndexAttribute::PerformanceHint RawIndexAttribute::GetPerformanceHintCpu() const
{
	return _performanceHintCpu;
}

//----------------------------------------------------------------------------------------
IIndexAttribute::PerformanceHint RawIndexAttribute::GetPerformanceHintGpu() const
{
	return _performanceHintGpu;
}

//----------------------------------------------------------------------------------------
IIndexAttribute::DataPersistenceFlags RawIndexAttribute::GetDataPersistenceFlags() const
{
	return _dataPersistenceFlags;
}

//----------------------------------------------------------------------------------------
// Binding methods
//----------------------------------------------------------------------------------------
void RawIndexAttribute::SetIndexBufferInfo(IIndexBuffer* indexBuffer, size_t attributeIndex)
{
	_indexBuffer = indexBuffer;
	_attributeIndex = attributeIndex;
	_boundToIndexBuffer = true;
}

//----------------------------------------------------------------------------------------
IIndexBuffer* RawIndexAttribute::GetBoundIndexBuffer() const
{
	return _indexBuffer;
}

//----------------------------------------------------------------------------------------
size_t RawIndexAttribute::GetBufferAttributeIndex() const
{
	return _attributeIndex;
}

//----------------------------------------------------------------------------------------
// Binding methods
//----------------------------------------------------------------------------------------
bool RawIndexAttribute::IsBoundToBuffer() const
{
	return _boundToIndexBuffer;
}

//----------------------------------------------------------------------------------------
void RawIndexAttribute::SetIndexAttributeInfo(const IIndexAttribute& sourceAttribute)
{
	_dataType = sourceAttribute.GetDataType();
	_indexCount = sourceAttribute.GetIndexCount();
	_performanceHintCpu = sourceAttribute.GetPerformanceHintCpu();
	_performanceHintGpu = sourceAttribute.GetPerformanceHintGpu();
	_indexBuffer = IIndexAttribute::GetBoundIndexBuffer(sourceAttribute);
	_boundToIndexBuffer = (_indexBuffer != nullptr);
	_attributeIndex = IIndexAttribute::GetBufferAttributeIndex(sourceAttribute);
}

//----------------------------------------------------------------------------------------
// Data methods
//----------------------------------------------------------------------------------------
SuccessToken RawIndexAttribute::SetInitialData(const uint8_t* data, size_t entryCount, size_t entryStrideInBytes)
{
	if (!_boundToIndexBuffer)
	{
		return false;
	}
	return SetInitialDataInternal(_indexBuffer, data, entryCount, entryStrideInBytes);
}

//----------------------------------------------------------------------------------------
SuccessToken RawIndexAttribute::QueueDataUpdate(const uint8_t* data, size_t entryCount, size_t initialIndexNo, size_t entryStrideInBytes, ITransferBatch* transferBatch)
{
	if (!_boundToIndexBuffer)
	{
		return false;
	}
	return QueueDataUpdateInternal(_indexBuffer, data, entryCount, initialIndexNo, entryStrideInBytes, transferBatch);
}

}} // namespace cobalt::graphics
