// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Deleter.h"
#include "ITextureBuffer.h"
#include <memory>
#include <vector>
namespace cobalt { namespace graphics {
class ITransferBatch;

class ITextureBuffer1D : public ITextureBuffer
{
public:
	// Typedefs
	typedef std::unique_ptr<ITextureBuffer1D, Deleter<ITextureBuffer1D>> unique_ptr;

public:
	// Initialization methods
	virtual void Delete() = 0;
	virtual SuccessToken AllocateMemory() = 0;

	// Format methods
	virtual void SetTextureFormat(ImageFormat imageFormat, DataFormat dataFormat) = 0;
	virtual void SetTextureDimensions(const V1UInt32& imageDimensions, int mipmapLevelCount = 1) = 0;
	virtual ImageFormat AllocatedImageFormat() const = 0;
	virtual DataFormat AllocatedDataFormat() const = 0;
	virtual int MipmapLevelCount() const = 0;
	virtual V1UInt32 MipmapLevelDimensions(int mipmapLevel) const = 0;

	// Initial data methods
	template<class T>
	inline SuccessToken SetInitialData(const std::vector<T>& sourceBuffer, int mipmapLevel = 0);
	template<class T>
	SuccessToken SetInitialData(std::vector<T>&& sourceBuffer, int mipmapLevel = 0) = delete;
	inline SuccessToken SetInitialData(const V1Int8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V2Int8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V3Int8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V4Int8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V1Int16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V2Int16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V3Int16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V4Int16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V1Int32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V2Int32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V3Int32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V4Int32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V1UInt8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V2UInt8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V3UInt8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V4UInt8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V1UInt16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V2UInt16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V3UInt16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V4UInt16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V1UInt32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V2UInt32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V3UInt32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V4UInt32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V1Norm8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V2Norm8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V3Norm8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V4Norm8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V1Norm16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V2Norm16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V3Norm16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V4Norm16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V1Norm32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V2Norm32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V3Norm32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V4Norm32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V1UNorm8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V2UNorm8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V3UNorm8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V4UNorm8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V1UNorm16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V2UNorm16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V3UNorm16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V4UNorm16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V1UNorm32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V2UNorm32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V3UNorm32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V4UNorm32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V1Float16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V2Float16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V3Float16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V4Float16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V1Float32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V2Float32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V3Float32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	inline SuccessToken SetInitialData(const V4Float32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0);
	virtual SuccessToken SetInitialData(const void* sourceBuffer, size_t sourceBufferSizeInBytes, SourceImageFormat imageFormat, SourceDataFormat dataFormat, int mipmapLevel = 0) = 0;

	// Data update methods
	template<class T>
	inline SuccessToken QueueDataUpdate(const std::vector<T>& sourceBuffer, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V1Int8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V2Int8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V3Int8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V4Int8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V1Int16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V2Int16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V3Int16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V4Int16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V1Int32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V2Int32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V3Int32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V4Int32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V1UInt8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V2UInt8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V3UInt8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V4UInt8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V1UInt16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V2UInt16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V3UInt16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V4UInt16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V1UInt32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V2UInt32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V3UInt32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V4UInt32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V1Norm8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V2Norm8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V3Norm8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V4Norm8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V1Norm16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V2Norm16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V3Norm16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V4Norm16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V1Norm32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V2Norm32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V3Norm32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V4Norm32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V1UNorm8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V2UNorm8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V3UNorm8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V4UNorm8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V1UNorm16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V2UNorm16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V3UNorm16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V4UNorm16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V1UNorm32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V2UNorm32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V3UNorm32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V4UNorm32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V1Float16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V2Float16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V3Float16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V4Float16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V1Float32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V2Float32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V3Float32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V4Float32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr);
	virtual SuccessToken QueueDataUpdate(const void* sourceBuffer, size_t sourceBufferSizeInBytes, SourceImageFormat imageFormat, SourceDataFormat dataFormat, int mipmapLevel = 0, const V1UInt32& imageOffsetInPixels = V1UInt32(0), const V1UInt32& imageRegionInPixels = V1UInt32(0), ITransferBatch* transferBatch = nullptr) = 0;

protected:
	// Constructors
	~ITextureBuffer1D() = default;
};

}} // namespace cobalt::graphics
#include "ITextureBuffer1D.inl"
