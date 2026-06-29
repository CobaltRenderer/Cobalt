// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "SuccessToken.h"
namespace cobalt { namespace graphics {
class IIndexBuffer;
class IRenderableNode;
class ITransferBatch;
class ReadOnlyIndexAttribute;

class IIndexAttribute
{
public:
	// Friend declarations
	friend class IIndexBuffer;
	friend class IRenderableNode;

public:
	// Enumerations
	enum class DataType
	{
		UInt16,
		UInt32,
	};
	enum class PerformanceHint : uint32_t
	{
		Default = 0x00000000,
		ReadNever = 0x00000001,
		ReadRarely = 0x00000002,
		ReadOften = 0x00000004,
		ReadFlagsMask = 0x000000FF,
		WriteNever = 0x00000100,
		WriteRarely = 0x00000200,
		WriteOften = 0x00000400,
		WriteFlagsMask = 0x0000FF00,
	};
	enum class DataPersistenceFlags
	{
		PersistAlways = 0x00000000,
		InvalidateExistingDataOnWrite = 0x000000001,
		InvalidateExistingDataAfterDrawComplete = 0x000000002,
	};

public:
	// Type methods
	virtual DataType GetDataType() const = 0;
	virtual size_t GetIndexCount() const = 0;
	constexpr static inline size_t GetDataTypeByteSize(DataType dataType);

	// Usage methods
	virtual PerformanceHint GetPerformanceHintCpu() const = 0;
	virtual PerformanceHint GetPerformanceHintGpu() const = 0;
	virtual DataPersistenceFlags GetDataPersistenceFlags() const = 0;

	// Binding methods
	virtual bool IsBoundToBuffer() const = 0;
	inline SuccessToken BuildReadOnlyAttribute(ReadOnlyIndexAttribute& targetAttribute) const;

protected:
	// Constructors
	~IIndexAttribute() = default;
	IIndexAttribute() = default;
	IIndexAttribute(const IIndexAttribute&) = default;
	IIndexAttribute& operator=(const IIndexAttribute&) = default;

	// Binding methods
	virtual void SetIndexBufferInfo(IIndexBuffer* indexBuffer, size_t attributeIndex) = 0;
	virtual IIndexBuffer* GetBoundIndexBuffer() const = 0;
	virtual size_t GetBufferAttributeIndex() const = 0;
	static inline IIndexBuffer* GetBoundIndexBuffer(const IIndexAttribute& targetAttribute);
	static inline size_t GetBufferAttributeIndex(const IIndexAttribute& targetAttribute);

	// Data methods
	inline bool SetInitialDataInternal(IIndexBuffer* indexBuffer, const uint8_t* data, size_t entryCount, size_t entryStrideInBytes);
	inline bool QueueDataUpdateInternal(IIndexBuffer* indexBuffer, const uint8_t* data, size_t entryCount, size_t initialIndexNo, size_t entryStrideInBytes, ITransferBatch* transferBatch);
};

}} // namespace cobalt::graphics
#include "IIndexAttribute.inl"
