// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "IVertexBuffer.h"
#include "ReadOnlyVertexAttribute.h"
#include <type_traits>
namespace cobalt { namespace graphics {

//----------------------------------------------------------------------------------------
// Type methods
//----------------------------------------------------------------------------------------
constexpr size_t IVertexAttribute::GetDataTypeByteSize(DataType dataType)
{
	switch (dataType)
	{
	case DataType::Int8:
		return 1;
	case DataType::Int16:
		return 2;
	case DataType::Int32:
		return 4;
	case DataType::UInt8:
		return 1;
	case DataType::UInt16:
		return 2;
	case DataType::UInt32:
		return 4;
	case DataType::Norm8:
		return 1;
	case DataType::Norm16:
		return 2;
	case DataType::UNorm8:
		return 1;
	case DataType::UNorm16:
		return 2;
	case DataType::Float16:
		return 2;
	case DataType::Float32:
		return 4;
	case DataType::A2B10G10R10UNorm:
		return 4;
	}
	return 0;
}

//----------------------------------------------------------------------------------------
// Data methods
//----------------------------------------------------------------------------------------
bool IVertexAttribute::SetInitialDataInternal(IVertexBuffer* vertexBuffer, size_t attributeIndex, const uint8_t* data, size_t entryCount, size_t entryStrideInBytes)
{
	return vertexBuffer->SetInitialData(attributeIndex, data, entryCount, entryStrideInBytes);
}

//----------------------------------------------------------------------------------------
bool IVertexAttribute::QueueDataUpdateInternal(IVertexBuffer* vertexBuffer, size_t attributeIndex, const uint8_t* data, size_t entryCount, size_t initialVertexNo, size_t entryStrideInBytes, ITransferBatch* transferBatch)
{
	return vertexBuffer->QueueDataUpdate(attributeIndex, data, entryCount, initialVertexNo, entryStrideInBytes, transferBatch);
}

//----------------------------------------------------------------------------------------
// Binding methods
//----------------------------------------------------------------------------------------
SuccessToken IVertexAttribute::BuildReadOnlyAttribute(ReadOnlyVertexAttribute& targetAttribute) const
{
	// Ensure that we are currently bound to a buffer
	if (!IsBoundToBuffer())
	{
		return false;
	}

	// Populate the read only attribute with data from this attribute, and return true to the caller.
	targetAttribute.SetVertexAttributeInfo(*this);
	return true;
}

//----------------------------------------------------------------------------------------
IVertexBuffer* IVertexAttribute::GetBoundVertexBuffer(const IVertexAttribute& targetAttribute)
{
	return targetAttribute.GetBoundVertexBuffer();
}

//----------------------------------------------------------------------------------------
size_t IVertexAttribute::GetBufferAttributeIndex(const IVertexAttribute& targetAttribute)
{
	return targetAttribute.GetBufferAttributeIndex();
}

//----------------------------------------------------------------------------------------
// Enumeration operators
//----------------------------------------------------------------------------------------
inline IVertexAttribute::PerformanceHint operator|(IVertexAttribute::PerformanceHint left, IVertexAttribute::PerformanceHint right)
{
	return (IVertexAttribute::PerformanceHint)((std::underlying_type<IVertexAttribute::PerformanceHint>::type)left | (std::underlying_type<IVertexAttribute::PerformanceHint>::type)right);
}

//----------------------------------------------------------------------------------------
inline IVertexAttribute::PerformanceHint& operator|=(IVertexAttribute::PerformanceHint& left, IVertexAttribute::PerformanceHint right)
{
	left = (left | right);
	return left;
}

//----------------------------------------------------------------------------------------
inline IVertexAttribute::PerformanceHint operator&(IVertexAttribute::PerformanceHint left, IVertexAttribute::PerformanceHint right)
{
	return (IVertexAttribute::PerformanceHint)((std::underlying_type<IVertexAttribute::PerformanceHint>::type)left & (std::underlying_type<IVertexAttribute::PerformanceHint>::type)right);
}

//----------------------------------------------------------------------------------------
inline IVertexAttribute::PerformanceHint& operator&=(IVertexAttribute::PerformanceHint& left, IVertexAttribute::PerformanceHint right)
{
	left = (left & right);
	return left;
}

//----------------------------------------------------------------------------------------
inline IVertexAttribute::DataPersistenceFlags operator|(IVertexAttribute::DataPersistenceFlags left, IVertexAttribute::DataPersistenceFlags right)
{
	return (IVertexAttribute::DataPersistenceFlags)((std::underlying_type<IVertexAttribute::DataPersistenceFlags>::type)left | (std::underlying_type<IVertexAttribute::DataPersistenceFlags>::type)right);
}

//----------------------------------------------------------------------------------------
inline IVertexAttribute::DataPersistenceFlags& operator|=(IVertexAttribute::DataPersistenceFlags& left, IVertexAttribute::DataPersistenceFlags right)
{
	left = (left | right);
	return left;
}

//----------------------------------------------------------------------------------------
inline IVertexAttribute::DataPersistenceFlags operator&(IVertexAttribute::DataPersistenceFlags left, IVertexAttribute::DataPersistenceFlags right)
{
	return (IVertexAttribute::DataPersistenceFlags)((std::underlying_type<IVertexAttribute::DataPersistenceFlags>::type)left & (std::underlying_type<IVertexAttribute::DataPersistenceFlags>::type)right);
}

//----------------------------------------------------------------------------------------
inline IVertexAttribute::DataPersistenceFlags& operator&=(IVertexAttribute::DataPersistenceFlags& left, IVertexAttribute::DataPersistenceFlags right)
{
	left = (left & right);
	return left;
}

}} // namespace cobalt::graphics
