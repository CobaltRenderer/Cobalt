// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Deleter.h"
#include "ITexelArray.h"
#include "SuccessToken.h"
#include <memory>
#include <vector>
namespace cobalt { namespace graphics {

class ITexelArrayOutput
{
public:
	// Typedefs
	typedef std::unique_ptr<ITexelArrayOutput, Deleter<ITexelArrayOutput>> unique_ptr;

public:
	// Initialization methods
	virtual void Delete() = 0;

	// Configuration methods
	virtual void SetDetachAfterCapture(bool state = true) = 0;
	virtual void SetArrayCaptureRegion(size_t captureEntryCount, size_t bufferOffset = 0) = 0;

	// Data methods
	virtual bool HasCapturedOutput() const = 0;
	virtual void ClearCapturedOutput() = 0;
	virtual size_t GetEntryCount() const = 0;
	virtual ITexelArray::SourceImageFormat GetOptimalImageFormat() const = 0;
	virtual ITexelArray::SourceDataFormat GetOptimalDataFormat() const = 0;
	template<class T>
	inline SuccessToken ReadBufferData(std::vector<T>& targetBuffer) const;
	inline SuccessToken ReadBufferData(V1Int8* targetBuffer, size_t targetBufferSize) const;
	inline SuccessToken ReadBufferData(V2Int8* targetBuffer, size_t targetBufferSize) const;
	inline SuccessToken ReadBufferData(V4Int8* targetBuffer, size_t targetBufferSize) const;
	inline SuccessToken ReadBufferData(V1Int16* targetBuffer, size_t targetBufferSize) const;
	inline SuccessToken ReadBufferData(V2Int16* targetBuffer, size_t targetBufferSize) const;
	inline SuccessToken ReadBufferData(V4Int16* targetBuffer, size_t targetBufferSize) const;
	inline SuccessToken ReadBufferData(V1Int32* targetBuffer, size_t targetBufferSize) const;
	inline SuccessToken ReadBufferData(V2Int32* targetBuffer, size_t targetBufferSize) const;
	inline SuccessToken ReadBufferData(V4Int32* targetBuffer, size_t targetBufferSize) const;
	inline SuccessToken ReadBufferData(V1UInt8* targetBuffer, size_t targetBufferSize) const;
	inline SuccessToken ReadBufferData(V2UInt8* targetBuffer, size_t targetBufferSize) const;
	inline SuccessToken ReadBufferData(V4UInt8* targetBuffer, size_t targetBufferSize) const;
	inline SuccessToken ReadBufferData(V1UInt16* targetBuffer, size_t targetBufferSize) const;
	inline SuccessToken ReadBufferData(V2UInt16* targetBuffer, size_t targetBufferSize) const;
	inline SuccessToken ReadBufferData(V4UInt16* targetBuffer, size_t targetBufferSize) const;
	inline SuccessToken ReadBufferData(V1UInt32* targetBuffer, size_t targetBufferSize) const;
	inline SuccessToken ReadBufferData(V2UInt32* targetBuffer, size_t targetBufferSize) const;
	inline SuccessToken ReadBufferData(V4UInt32* targetBuffer, size_t targetBufferSize) const;
	inline SuccessToken ReadBufferData(V1Float16* targetBuffer, size_t targetBufferSize) const;
	inline SuccessToken ReadBufferData(V2Float16* targetBuffer, size_t targetBufferSize) const;
	inline SuccessToken ReadBufferData(V4Float16* targetBuffer, size_t targetBufferSize) const;
	inline SuccessToken ReadBufferData(V1Float32* targetBuffer, size_t targetBufferSize) const;
	inline SuccessToken ReadBufferData(V2Float32* targetBuffer, size_t targetBufferSize) const;
	inline SuccessToken ReadBufferData(V4Float32* targetBuffer, size_t targetBufferSize) const;
	virtual SuccessToken ReadBufferData(void* targetBuffer, size_t targetBufferSizeInBytes, ITexelArray::SourceImageFormat imageFormat, ITexelArray::SourceDataFormat dataFormat) const = 0;

protected:
	// Constructors
	~ITexelArrayOutput() = default;
};

}} // namespace cobalt::graphics
#include "ITexelArrayOutput.inl"
