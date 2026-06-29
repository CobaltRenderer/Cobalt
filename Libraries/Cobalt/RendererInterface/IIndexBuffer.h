// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Deleter.h"
#include "SuccessToken.h"
#include "VectorTypes.h"
#include <memory>
namespace cobalt { namespace graphics {
class IIndexAttribute;
class ITransferBatch;
class ITexelArray;

class IIndexBuffer
{
public:
	// Friend declarations
	friend class IIndexAttribute;

	// Typedefs
	typedef std::unique_ptr<IIndexBuffer, Deleter<IIndexBuffer>> unique_ptr;

public:
	// Initialization methods
	virtual void Delete() = 0;
	virtual SuccessToken AllocateMemory() = 0;
	virtual SuccessToken AllocateMemoryWithAlias(ITexelArray* texelArray) = 0;

	// Binding methods
	virtual SuccessToken BindIndexAttribute(IIndexAttribute& indexAttribute) = 0;
	virtual SuccessToken BindIndexAttributeManualLayout(IIndexAttribute& indexAttribute, size_t bufferOffsetInBytes, size_t bufferStrideInBytes) = 0;

	// Data methods
	virtual SuccessToken SetRawInitialData(const uint8_t* data, size_t dataSizeInBytes) = 0;
	virtual SuccessToken QueueRawDataUpdate(const uint8_t* data, size_t dataSizeInBytes, size_t bufferOffsetInBytes, ITransferBatch* transferBatch = nullptr) = 0;

protected:
	// Constructors
	~IIndexBuffer() = default;

	// Binding methods
	inline void AttachIndexAttributeToThisArray(IIndexAttribute& indexAttribute, size_t attributeIndex);

	// Data methods
	virtual SuccessToken SetInitialData(const uint8_t* data, size_t entryCount, size_t entryStrideInBytes) = 0;
	virtual SuccessToken QueueDataUpdate(const uint8_t* data, size_t entryCount, size_t initialIndexNo, size_t entryStrideInBytes, ITransferBatch* transferBatch) = 0;
};

}} // namespace cobalt::graphics
#include "IIndexBuffer.inl"
