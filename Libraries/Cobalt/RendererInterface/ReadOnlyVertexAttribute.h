// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "IVertexAttribute.h"
// Actual include guard with base outside the guard due to circular inl file dependency
#ifndef INCLUDED_COBALT_READONLYVERTEXATTRIBUTE_H
#define INCLUDED_COBALT_READONLYVERTEXATTRIBUTE_H
namespace cobalt { namespace graphics {
class IVertexBuffer;

class ReadOnlyVertexAttribute : public IVertexAttribute
{
public:
	// Friend declarations
	friend class IVertexAttribute;

public:
	// Constructors
	inline ReadOnlyVertexAttribute();

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
#include "ReadOnlyVertexAttribute.inl"
#endif
