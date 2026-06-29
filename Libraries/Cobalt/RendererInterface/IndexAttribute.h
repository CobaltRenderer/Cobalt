// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "IIndexAttribute.h"
#include <initializer_list>
#include <vector>
namespace cobalt { namespace graphics {
class IIndexBuffer;

template<class T>
class IndexAttribute : public IIndexAttribute
{
public:
	// Constructors
	inline IndexAttribute();
	inline IndexAttribute(size_t indexCount, PerformanceHint performanceHintCpu, PerformanceHint performanceHintGpu, DataPersistenceFlags dataPersistenceFlags = DataPersistenceFlags::PersistAlways);

	// Type methods
	DataType GetDataType() const override;
	size_t GetIndexCount() const override;
	static inline T GetPrimitiveRestartValue();

	// Usage methods
	PerformanceHint GetPerformanceHintCpu() const override;
	PerformanceHint GetPerformanceHintGpu() const override;
	DataPersistenceFlags GetDataPersistenceFlags() const override;

	// Binding methods
	bool IsBoundToBuffer() const override;

	// Data methods
	inline SuccessToken SetInitialData(const T* data, size_t entryCount, size_t entryStrideInBytes = sizeof(T));
	inline SuccessToken SetInitialData(const std::vector<T>& data);
	inline SuccessToken SetInitialData(std::vector<T>&& data) = delete;
	inline SuccessToken SetInitialData(std::initializer_list<T> data);
	inline SuccessToken QueueDataUpdate(const T* data, size_t entryCount, size_t initialIndexNo = 0, size_t entryStrideInBytes = sizeof(T), ITransferBatch* transferBatch = nullptr);

protected:
	// Binding methods
	void SetIndexBufferInfo(IIndexBuffer* indexBuffer, size_t attributeIndex) override;
	IIndexBuffer* GetBoundIndexBuffer() const override;
	size_t GetBufferAttributeIndex() const override;

private:
	DataType _dataType = {};
	size_t _indexCount = 0;
	PerformanceHint _performanceHintCpu = PerformanceHint::Default;
	PerformanceHint _performanceHintGpu = PerformanceHint::Default;
	DataPersistenceFlags _dataPersistenceFlags = {};
	bool _boundToIndexBuffer = false;
	IIndexBuffer* _indexBuffer = nullptr;
	size_t _attributeIndex = 0;
};

}} // namespace cobalt::graphics
#include "IndexAttribute.inl"
