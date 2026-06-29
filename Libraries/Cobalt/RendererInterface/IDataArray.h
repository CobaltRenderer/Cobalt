// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Deleter.h"
#include "IResourceArray.h"
#include "SuccessToken.h"
#include <memory>
namespace cobalt { namespace graphics {
class IDataArrayOutput;
class ITransferBatch;

class IDataArray : public IResourceArray
{
public:
	// Enumerations
	enum class UsageFlags : uint32_t
	{
		Default = 0x00000000,
		ShaderInput = 0x00000001,
		ShaderOutput = 0x00000002,
		TransferSource = 0x00000004,
		TransferDestination = 0x00000008,
		IndirectDrawSource = 0x00000010,
		IndirectDrawCountSource = 0x00000020,
		AtomicOperations = 0x00000040,
	};

	// Typedefs
	typedef std::unique_ptr<IDataArray, Deleter<IDataArray>> unique_ptr;

public:
	// Initialization methods
	virtual void Delete() = 0;
	virtual SuccessToken AllocateMemory() = 0;
	virtual void SetBufferLayout(size_t entryStrideInBytes, size_t entryCount, bool hasCounter = false, uint32_t counterResetValue = 0) = 0;

	// Usage methods
	virtual void SetUsageFlags(UsageFlags usageFlags) = 0;

	// Initial data methods
	virtual SuccessToken SetInitialData(const void* sourceBuffer, size_t sourceBufferSizeInBytes) = 0;

	// Data update methods
	virtual SuccessToken QueueDataUpdate(const void* sourceBuffer, size_t sourceBufferSizeInBytes, size_t targetBufferOffsetInBytes = 0, ITransferBatch* transferBatch = nullptr) = 0;
	virtual void UpdateCounterResetValue(uint32_t counterResetValue) = 0;

	// Data transfer methods
	virtual SuccessToken QueueDataTransfer(IDataArray* targetBuffer, size_t transferCountInBytes, size_t sourceBufferOffsetInBytes = 0, size_t targetBufferOffsetInBytes = 0, ITransferBatch* transferBatch = nullptr) = 0;

	// Output capture methods
	virtual void AddOutputCaptureTarget(IDataArrayOutput* captureTarget) = 0;
	virtual void RemoveOutputCaptureTarget(IDataArrayOutput* captureTarget) = 0;

protected:
	// Constructors
	~IDataArray() = default;
};

}} // namespace cobalt::graphics
#include "IDataArray.inl"
