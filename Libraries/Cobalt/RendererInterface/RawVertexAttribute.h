// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "IVertexAttribute.h"
namespace cobalt { namespace graphics {
class IVertexBuffer;

class RawVertexAttribute : public IVertexAttribute
{
public:
	// Constructors
	inline RawVertexAttribute();
	inline RawVertexAttribute(DataType dataType, size_t elementCount, size_t vertexCount, PerformanceHint performanceHintCpu, PerformanceHint performanceHintGpu, DataPersistenceFlags dataPersistenceFlags = DataPersistenceFlags::PersistAlways);

	// Type methods
	inline DataType GetDataType() const override;
	inline size_t GetVertexCount() const override;
	inline size_t GetAttributeElementCount() const override;

	// Usage methods
	inline PerformanceHint GetPerformanceHintCpu() const override;
	inline PerformanceHint GetPerformanceHintGpu() const override;
	inline DataPersistenceFlags GetDataPersistenceFlags() const override;

	// Binding methods
	inline bool IsBoundToBuffer() const override;

	// Data methods
	inline SuccessToken SetInitialData(const uint8_t* data, size_t entryCount, size_t entryStrideInBytes);
	inline SuccessToken QueueDataUpdate(const uint8_t* data, size_t entryCount, size_t initialVertexNo, size_t entryStrideInBytes, ITransferBatch* transferBatch = nullptr);

protected:
	// Binding methods
	inline void SetVertexBufferInfo(IVertexBuffer* vertexBuffer, size_t attributeIndex) override;
	inline IVertexBuffer* GetBoundVertexBuffer() const override;
	inline size_t GetBufferAttributeIndex() const override;
	inline void SetVertexAttributeInfo(const IVertexAttribute& sourceAttribute);

private:
	DataType _dataType;
	size_t _vertexCount = 0;
	size_t _elementCount = 0;
	PerformanceHint _performanceHintCpu = PerformanceHint::Default;
	PerformanceHint _performanceHintGpu = PerformanceHint::Default;
	DataPersistenceFlags _dataPersistenceFlags = {};
	bool _boundToVertexBuffer = false;
	IVertexBuffer* _vertexBuffer = nullptr;
	size_t _attributeIndex = 0;
};

}} // namespace cobalt::graphics
#include "RawVertexAttribute.inl"
