// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Deleter.h"
#include "SuccessToken.h"
#include "VectorTypes.h"
#include <memory>
namespace cobalt { namespace graphics {
class IVertexAttribute;
class ITransferBatch;
class ITexelArray;

class IVertexBuffer
{
public:
	// Friend declarations
	friend class IVertexAttribute;

	// Typedefs
	typedef std::unique_ptr<IVertexBuffer, Deleter<IVertexBuffer>> unique_ptr;

public:
	// Initialization methods
	virtual void Delete() = 0;
	virtual SuccessToken AllocateMemory() = 0;
	virtual SuccessToken AllocateMemoryWithAlias(ITexelArray* texelArray) = 0;

	// Binding methods
	virtual SuccessToken BindVertexAttribute(IVertexAttribute& vertexAttribute) = 0;
	virtual SuccessToken BindVertexAttributeManualLayout(IVertexAttribute& vertexAttribute, size_t bufferOffsetInBytes, size_t bufferStrideInBytes) = 0;

	// Data methods
	virtual SuccessToken SetRawInitialData(const uint8_t* data, size_t dataSizeInBytes) = 0;
	virtual SuccessToken QueueRawDataUpdate(const uint8_t* data, size_t dataSizeInBytes, size_t bufferOffsetInBytes, ITransferBatch* transferBatch = nullptr) = 0;

protected:
	// Constructors
	~IVertexBuffer() = default;

	// Binding methods
	inline void AttachVertexAttributeToThisArray(IVertexAttribute& vertexAttribute, size_t attributeIndex);

	// Data methods
	virtual SuccessToken SetInitialData(size_t attributeIndex, const uint8_t* data, size_t entryCount, size_t entryStrideInBytes) = 0;
	virtual SuccessToken QueueDataUpdate(size_t attributeIndex, const uint8_t* data, size_t entryCount, size_t initialVertexNo, size_t entryStrideInBytes, ITransferBatch* transferBatch) = 0;
};

}} // namespace cobalt::graphics
#include "IVertexBuffer.inl"
