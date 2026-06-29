// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "IVertexAttribute.h"
#include <initializer_list>
#include <vector>
namespace cobalt { namespace graphics {
class IVertexBuffer;

template<class T>
class VertexAttribute : public IVertexAttribute
{
public:
	// Constructors
	inline VertexAttribute();
	inline VertexAttribute(size_t vertexCount, PerformanceHint performanceHintCpu, PerformanceHint performanceHintGpu, DataPersistenceFlags dataPersistenceFlags = DataPersistenceFlags::PersistAlways);

	// Type methods
	DataType GetDataType() const override;
	size_t GetVertexCount() const override;
	size_t GetAttributeElementCount() const override;

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
	inline SuccessToken QueueDataUpdate(const T* data, size_t entryCount, size_t initialVertexNo = 0, size_t entryStrideInBytes = sizeof(T), ITransferBatch* transferBatch = nullptr);

protected:
	// Binding methods
	void SetVertexBufferInfo(IVertexBuffer* vertexBuffer, size_t attributeIndex) override;
	IVertexBuffer* GetBoundVertexBuffer() const override;
	size_t GetBufferAttributeIndex() const override;

private:
	DataType _dataType = {};
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
#include "VertexAttribute.inl"
