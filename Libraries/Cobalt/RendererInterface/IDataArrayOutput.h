// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Deleter.h"
#include "SuccessToken.h"
#include <memory>
namespace cobalt { namespace graphics {

class IDataArrayOutput
{
public:
	// Typedefs
	typedef std::unique_ptr<IDataArrayOutput, Deleter<IDataArrayOutput>> unique_ptr;

public:
	// Initialization methods
	virtual void Delete() = 0;

	// Configuration methods
	virtual void SetDetachAfterCapture(bool state = true) = 0;
	virtual void SetArrayCaptureRegion(size_t captureEntryCount, size_t bufferOffset = 0, bool captureCounterValue = false) = 0;

	// Data methods
	virtual bool HasCapturedOutput() const = 0;
	virtual bool HasCapturedCounterValue() const = 0;
	virtual void ClearCapturedOutput() = 0;
	virtual size_t GetEntryCount() const = 0;
	virtual size_t GetEntrySizeInBytes() const = 0;
	virtual SuccessToken ReadBufferData(void* targetBuffer, size_t targetBufferSizeInBytes) const = 0;
	virtual SuccessToken ReadCounterValue(uint32_t& counterValue) const = 0;

protected:
	// Constructors
	~IDataArrayOutput() = default;
};

}} // namespace cobalt::graphics
