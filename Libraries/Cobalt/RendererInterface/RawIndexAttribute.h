// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "IIndexAttribute.h"
namespace cobalt { namespace graphics {
class IIndexBuffer;

class RawIndexAttribute : public IIndexAttribute
{
public:
	// Constructors
	inline RawIndexAttribute();
	inline RawIndexAttribute(DataType dataType, size_t indexCount, PerformanceHint performanceHintCpu, PerformanceHint performanceHintGpu, DataPersistenceFlags dataPersistenceFlags = DataPersistenceFlags::PersistAlways);

	// Type methods
	inline DataType GetDataType() const override;
	inline size_t GetIndexCount() const override;

	// Usage methods
	inline PerformanceHint GetPerformanceHintCpu() const override;
	inline PerformanceHint GetPerformanceHintGpu() const override;
	inline DataPersistenceFlags GetDataPersistenceFlags() const override;

	// Binding methods
	inline bool IsBoundToBuffer() const override;

	// Data methods
	inline SuccessToken SetInitialData(const uint8_t* data, size_t entryCount, size_t entryStrideInBytes);
	inline SuccessToken QueueDataUpdate(const uint8_t* data, size_t entryCount, size_t initialIndexNo, size_t entryStrideInBytes, ITransferBatch* transferBatch = nullptr);

protected:
	// Binding methods
	inline void SetIndexBufferInfo(IIndexBuffer* indexBuffer, size_t attributeIndex) override;
	inline IIndexBuffer* GetBoundIndexBuffer() const override;
	inline size_t GetBufferAttributeIndex() const override;
	inline void SetIndexAttributeInfo(const IIndexAttribute& sourceAttribute);

private:
	DataType _dataType;
	size_t _indexCount = 0;
	PerformanceHint _performanceHintCpu = PerformanceHint::Default;
	PerformanceHint _performanceHintGpu = PerformanceHint::Default;
	DataPersistenceFlags _dataPersistenceFlags = {};
	bool _boundToIndexBuffer = false;
	IIndexBuffer* _indexBuffer = nullptr;
	size_t _attributeIndex = 0;
};

}} // namespace cobalt::graphics
#include "RawIndexAttribute.inl"
