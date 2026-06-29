// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "IIndexAttribute.h"
// Actual include guard with base outside the guard due to circular inl file dependency
#ifndef INCLUDED_COBALT_READONLYINDEXATTRIBUTE_H
#define INCLUDED_COBALT_READONLYINDEXATTRIBUTE_H
namespace cobalt { namespace graphics {
class IIndexBuffer;

class ReadOnlyIndexAttribute : public IIndexAttribute
{
public:
	// Friend declarations
	friend class IIndexAttribute;

public:
	// Constructors
	inline ReadOnlyIndexAttribute();

	// Type methods
	inline DataType GetDataType() const override;
	inline size_t GetIndexCount() const override;

	// Usage methods
	inline PerformanceHint GetPerformanceHintCpu() const override;
	inline PerformanceHint GetPerformanceHintGpu() const override;
	inline DataPersistenceFlags GetDataPersistenceFlags() const override;

	// Binding methods
	inline bool IsBoundToBuffer() const override;

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
#include "ReadOnlyIndexAttribute.inl"
#endif
