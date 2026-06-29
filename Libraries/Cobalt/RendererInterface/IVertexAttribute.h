// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "SuccessToken.h"
namespace cobalt { namespace graphics {
class IVertexBuffer;
class IRenderableNode;
class ITransferBatch;
class ReadOnlyVertexAttribute;

class IVertexAttribute
{
public:
	// Friend declarations
	friend class IVertexBuffer;
	friend class IRenderableNode;

public:
	// Enumerations
	enum class DataType
	{
		Int8,
		Int16,
		Int32,
		UInt8,
		UInt16,
		UInt32,
		Norm8,
		Norm16,
		UNorm8,
		UNorm16,
		Float16,
		Float32,
		A2B10G10R10UNorm,
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
	virtual size_t GetVertexCount() const = 0;
	virtual size_t GetAttributeElementCount() const = 0;
	constexpr static inline size_t GetDataTypeByteSize(DataType dataType);

	// Usage methods
	virtual PerformanceHint GetPerformanceHintCpu() const = 0;
	virtual PerformanceHint GetPerformanceHintGpu() const = 0;
	virtual DataPersistenceFlags GetDataPersistenceFlags() const = 0;

	// Binding methods
	virtual bool IsBoundToBuffer() const = 0;
	inline SuccessToken BuildReadOnlyAttribute(ReadOnlyVertexAttribute& targetAttribute) const;

protected:
	// Constructors
	~IVertexAttribute() = default;
	IVertexAttribute() = default;
	IVertexAttribute(const IVertexAttribute&) = default;
	IVertexAttribute& operator=(const IVertexAttribute&) = default;

	// Binding methods
	virtual void SetVertexBufferInfo(IVertexBuffer* vertexArray, size_t attributeIndex) = 0;
	virtual IVertexBuffer* GetBoundVertexBuffer() const = 0;
	virtual size_t GetBufferAttributeIndex() const = 0;
	static inline IVertexBuffer* GetBoundVertexBuffer(const IVertexAttribute& targetAttribute);
	static inline size_t GetBufferAttributeIndex(const IVertexAttribute& targetAttribute);

	// Data methods
	inline bool SetInitialDataInternal(IVertexBuffer* vertexBuffer, size_t attributeIndex, const uint8_t* data, size_t entryCount, size_t entryStrideInBytes);
	inline bool QueueDataUpdateInternal(IVertexBuffer* vertexBuffer, size_t attributeIndex, const uint8_t* data, size_t entryCount, size_t initialVertexNo, size_t entryStrideInBytes, ITransferBatch* transferBatch);
};

}} // namespace cobalt::graphics
#include "IVertexAttribute.inl"
