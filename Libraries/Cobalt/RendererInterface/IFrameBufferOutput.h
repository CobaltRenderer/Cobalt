// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Deleter.h"
#include "ITextureBuffer.h"
#include "SuccessToken.h"
#include "VectorTypes.h"
#include <memory>
#include <vector>
namespace cobalt { namespace graphics {

class IFrameBufferOutput
{
public:
	// Typedefs
	typedef std::unique_ptr<IFrameBufferOutput, Deleter<IFrameBufferOutput>> unique_ptr;

public:
	// Initialization methods
	virtual void Delete() = 0;

	// Configuration methods
	virtual void SetDetachAfterCapture(bool state = true) = 0;
	virtual void SetFrameBufferCaptureRegion(const V2UInt32& imageOffsetInPixels = V2UInt32(0, 0), const V2UInt32& imageRegionInPixels = V2UInt32(0, 0)) = 0;

	// Data methods
	virtual bool HasCapturedOutput() const = 0;
	virtual void ClearCapturedOutput() = 0;
	virtual V2UInt32 GetImageDimensions() const = 0;
	virtual V2UInt32 GetCroppedImageDimensions(const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels) const = 0;
	virtual ITextureBuffer::SourceImageFormat GetOptimalImageFormat() const = 0;
	virtual ITextureBuffer::SourceDataFormat GetOptimalDataFormat() const = 0;
	template<class T>
	inline SuccessToken ReadBufferData(std::vector<T>& targetBuffer, const V2UInt32& imageOffsetInPixels = V2UInt32(0, 0), const V2UInt32& imageRegionInPixels = V2UInt32(0, 0)) const;
	inline SuccessToken ReadBufferData(V1Int8* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels = V2UInt32(0, 0), const V2UInt32& imageRegionInPixels = V2UInt32(0, 0)) const;
	inline SuccessToken ReadBufferData(V2Int8* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels = V2UInt32(0, 0), const V2UInt32& imageRegionInPixels = V2UInt32(0, 0)) const;
	inline SuccessToken ReadBufferData(V3Int8* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels = V2UInt32(0, 0), const V2UInt32& imageRegionInPixels = V2UInt32(0, 0)) const;
	inline SuccessToken ReadBufferData(V4Int8* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels = V2UInt32(0, 0), const V2UInt32& imageRegionInPixels = V2UInt32(0, 0)) const;
	inline SuccessToken ReadBufferData(V1Int16* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels = V2UInt32(0, 0), const V2UInt32& imageRegionInPixels = V2UInt32(0, 0)) const;
	inline SuccessToken ReadBufferData(V2Int16* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels = V2UInt32(0, 0), const V2UInt32& imageRegionInPixels = V2UInt32(0, 0)) const;
	inline SuccessToken ReadBufferData(V3Int16* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels = V2UInt32(0, 0), const V2UInt32& imageRegionInPixels = V2UInt32(0, 0)) const;
	inline SuccessToken ReadBufferData(V4Int16* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels = V2UInt32(0, 0), const V2UInt32& imageRegionInPixels = V2UInt32(0, 0)) const;
	inline SuccessToken ReadBufferData(V1Int32* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels = V2UInt32(0, 0), const V2UInt32& imageRegionInPixels = V2UInt32(0, 0)) const;
	inline SuccessToken ReadBufferData(V2Int32* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels = V2UInt32(0, 0), const V2UInt32& imageRegionInPixels = V2UInt32(0, 0)) const;
	inline SuccessToken ReadBufferData(V3Int32* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels = V2UInt32(0, 0), const V2UInt32& imageRegionInPixels = V2UInt32(0, 0)) const;
	inline SuccessToken ReadBufferData(V4Int32* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels = V2UInt32(0, 0), const V2UInt32& imageRegionInPixels = V2UInt32(0, 0)) const;
	inline SuccessToken ReadBufferData(V1UInt8* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels = V2UInt32(0, 0), const V2UInt32& imageRegionInPixels = V2UInt32(0, 0)) const;
	inline SuccessToken ReadBufferData(V2UInt8* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels = V2UInt32(0, 0), const V2UInt32& imageRegionInPixels = V2UInt32(0, 0)) const;
	inline SuccessToken ReadBufferData(V3UInt8* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels = V2UInt32(0, 0), const V2UInt32& imageRegionInPixels = V2UInt32(0, 0)) const;
	inline SuccessToken ReadBufferData(V4UInt8* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels = V2UInt32(0, 0), const V2UInt32& imageRegionInPixels = V2UInt32(0, 0)) const;
	inline SuccessToken ReadBufferData(V1UInt16* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels = V2UInt32(0, 0), const V2UInt32& imageRegionInPixels = V2UInt32(0, 0)) const;
	inline SuccessToken ReadBufferData(V2UInt16* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels = V2UInt32(0, 0), const V2UInt32& imageRegionInPixels = V2UInt32(0, 0)) const;
	inline SuccessToken ReadBufferData(V3UInt16* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels = V2UInt32(0, 0), const V2UInt32& imageRegionInPixels = V2UInt32(0, 0)) const;
	inline SuccessToken ReadBufferData(V4UInt16* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels = V2UInt32(0, 0), const V2UInt32& imageRegionInPixels = V2UInt32(0, 0)) const;
	inline SuccessToken ReadBufferData(V1UInt32* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels = V2UInt32(0, 0), const V2UInt32& imageRegionInPixels = V2UInt32(0, 0)) const;
	inline SuccessToken ReadBufferData(V2UInt32* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels = V2UInt32(0, 0), const V2UInt32& imageRegionInPixels = V2UInt32(0, 0)) const;
	inline SuccessToken ReadBufferData(V3UInt32* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels = V2UInt32(0, 0), const V2UInt32& imageRegionInPixels = V2UInt32(0, 0)) const;
	inline SuccessToken ReadBufferData(V4UInt32* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels = V2UInt32(0, 0), const V2UInt32& imageRegionInPixels = V2UInt32(0, 0)) const;
	inline SuccessToken ReadBufferData(V1Float16* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels = V2UInt32(0, 0), const V2UInt32& imageRegionInPixels = V2UInt32(0, 0)) const;
	inline SuccessToken ReadBufferData(V2Float16* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels = V2UInt32(0, 0), const V2UInt32& imageRegionInPixels = V2UInt32(0, 0)) const;
	inline SuccessToken ReadBufferData(V3Float16* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels = V2UInt32(0, 0), const V2UInt32& imageRegionInPixels = V2UInt32(0, 0)) const;
	inline SuccessToken ReadBufferData(V4Float16* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels = V2UInt32(0, 0), const V2UInt32& imageRegionInPixels = V2UInt32(0, 0)) const;
	inline SuccessToken ReadBufferData(V1Float32* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels = V2UInt32(0, 0), const V2UInt32& imageRegionInPixels = V2UInt32(0, 0)) const;
	inline SuccessToken ReadBufferData(V2Float32* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels = V2UInt32(0, 0), const V2UInt32& imageRegionInPixels = V2UInt32(0, 0)) const;
	inline SuccessToken ReadBufferData(V3Float32* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels = V2UInt32(0, 0), const V2UInt32& imageRegionInPixels = V2UInt32(0, 0)) const;
	inline SuccessToken ReadBufferData(V4Float32* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels = V2UInt32(0, 0), const V2UInt32& imageRegionInPixels = V2UInt32(0, 0)) const;
	virtual SuccessToken ReadBufferData(void* targetBuffer, size_t targetBufferSizeInBytes, ITextureBuffer::SourceImageFormat imageFormat, ITextureBuffer::SourceDataFormat dataFormat, const V2UInt32& imageOffsetInPixels = V2UInt32(0, 0), const V2UInt32& imageRegionInPixels = V2UInt32(0, 0)) const = 0;

protected:
	// Constructors
	~IFrameBufferOutput() = default;

	// Data methods
	virtual size_t CalculatePixelCountForRegion(const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels) const = 0;
};

}} // namespace cobalt::graphics
#include "IFrameBufferOutput.inl"
