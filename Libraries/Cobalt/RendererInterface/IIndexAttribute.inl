// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "IIndexBuffer.h"
#include "ReadOnlyIndexAttribute.h"
#include <type_traits>
namespace cobalt { namespace graphics {

//----------------------------------------------------------------------------------------
// Type methods
//----------------------------------------------------------------------------------------
constexpr size_t IIndexAttribute::GetDataTypeByteSize(DataType dataType)
{
	switch (dataType)
	{
	case DataType::UInt16:
		return sizeof(uint16_t);
	case DataType::UInt32:
		return sizeof(uint32_t);
	}
	return 0;
}

//----------------------------------------------------------------------------------------
// Data methods
//----------------------------------------------------------------------------------------
bool IIndexAttribute::SetInitialDataInternal(IIndexBuffer* indexBuffer, const uint8_t* data, size_t entryCount, size_t entryStrideInBytes)
{
	return indexBuffer->SetInitialData(data, entryCount, entryStrideInBytes);
}

//----------------------------------------------------------------------------------------
bool IIndexAttribute::QueueDataUpdateInternal(IIndexBuffer* indexBuffer, const uint8_t* data, size_t entryCount, size_t initialIndexNo, size_t entryStrideInBytes, ITransferBatch* transferBatch)
{
	return indexBuffer->QueueDataUpdate(data, entryCount, initialIndexNo, entryStrideInBytes, transferBatch);
}

//----------------------------------------------------------------------------------------
// Binding methods
//----------------------------------------------------------------------------------------
SuccessToken IIndexAttribute::BuildReadOnlyAttribute(ReadOnlyIndexAttribute& targetAttribute) const
{
	// Ensure that we are currently bound to a buffer
	if (!IsBoundToBuffer())
	{
		return false;
	}

	// Populate the read only attribute with data from this attribute, and return true to the caller.
	targetAttribute.SetIndexAttributeInfo(*this);
	return true;
}

//----------------------------------------------------------------------------------------
IIndexBuffer* IIndexAttribute::GetBoundIndexBuffer(const IIndexAttribute& targetAttribute)
{
	return targetAttribute.GetBoundIndexBuffer();
}

//----------------------------------------------------------------------------------------
size_t IIndexAttribute::GetBufferAttributeIndex(const IIndexAttribute& targetAttribute)
{
	return targetAttribute.GetBufferAttributeIndex();
}

//----------------------------------------------------------------------------------------
// Enumeration operators
//----------------------------------------------------------------------------------------
inline IIndexAttribute::PerformanceHint operator|(IIndexAttribute::PerformanceHint left, IIndexAttribute::PerformanceHint right)
{
	return (IIndexAttribute::PerformanceHint)((std::underlying_type<IIndexAttribute::PerformanceHint>::type)left | (std::underlying_type<IIndexAttribute::PerformanceHint>::type)right);
}

//----------------------------------------------------------------------------------------
inline IIndexAttribute::PerformanceHint& operator|=(IIndexAttribute::PerformanceHint& left, IIndexAttribute::PerformanceHint right)
{
	left = (left | right);
	return left;
}

//----------------------------------------------------------------------------------------
inline IIndexAttribute::PerformanceHint operator&(IIndexAttribute::PerformanceHint left, IIndexAttribute::PerformanceHint right)
{
	return (IIndexAttribute::PerformanceHint)((std::underlying_type<IIndexAttribute::PerformanceHint>::type)left & (std::underlying_type<IIndexAttribute::PerformanceHint>::type)right);
}

//----------------------------------------------------------------------------------------
inline IIndexAttribute::PerformanceHint& operator&=(IIndexAttribute::PerformanceHint& left, IIndexAttribute::PerformanceHint right)
{
	left = (left & right);
	return left;
}

//----------------------------------------------------------------------------------------
inline IIndexAttribute::DataPersistenceFlags operator|(IIndexAttribute::DataPersistenceFlags left, IIndexAttribute::DataPersistenceFlags right)
{
	return (IIndexAttribute::DataPersistenceFlags)((std::underlying_type<IIndexAttribute::DataPersistenceFlags>::type)left | (std::underlying_type<IIndexAttribute::DataPersistenceFlags>::type)right);
}

//----------------------------------------------------------------------------------------
inline IIndexAttribute::DataPersistenceFlags& operator|=(IIndexAttribute::DataPersistenceFlags& left, IIndexAttribute::DataPersistenceFlags right)
{
	left = (left | right);
	return left;
}

//----------------------------------------------------------------------------------------
inline IIndexAttribute::DataPersistenceFlags operator&(IIndexAttribute::DataPersistenceFlags left, IIndexAttribute::DataPersistenceFlags right)
{
	return (IIndexAttribute::DataPersistenceFlags)((std::underlying_type<IIndexAttribute::DataPersistenceFlags>::type)left & (std::underlying_type<IIndexAttribute::DataPersistenceFlags>::type)right);
}

//----------------------------------------------------------------------------------------
inline IIndexAttribute::DataPersistenceFlags& operator&=(IIndexAttribute::DataPersistenceFlags& left, IIndexAttribute::DataPersistenceFlags right)
{
	left = (left & right);
	return left;
}

}} // namespace cobalt::graphics
