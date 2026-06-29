// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Deleter.h"
#include "IResourceArray.h"
#include "SuccessToken.h"
#include "VectorTypes.h"
#include <memory>
#include <vector>
namespace cobalt { namespace graphics {
class ITexelArrayOutput;
class ITransferBatch;

class ITexelArray : public IResourceArray
{
public:
	// Enumerations
	enum class ImageFormat
	{
		R,
		RG,
		RGBA,
		X,
		XY,
		XYZW,
	};
	enum class DataFormat
	{
		Int8,
		Int16,
		Int32,
		UInt8,
		UInt16,
		UInt32,
		Norm8,
		UNorm8,
		Float16,
		Float32,
	};
	enum class SourceImageFormat
	{
		R,
		RG,
		RGBA,
		X,
		XY,
		XYZW,
	};
	enum class SourceDataFormat
	{
		Int8,
		Int16,
		Int32,
		UInt8,
		UInt16,
		UInt32,
		Norm8,
		Norm16,
		Norm32,
		UNorm8,
		UNorm16,
		UNorm32,
		Float16,
		Float32,
	};
	enum class UsageFlags : uint32_t
	{
		Default = 0x00000000,
		ShaderInput = 0x00000001,
		ShaderOutput = 0x00000002,
		TransferSource = 0x00000004,
		TransferDestination = 0x00000008,
	};

	// Typedefs
	typedef std::unique_ptr<ITexelArray, Deleter<ITexelArray>> unique_ptr;

public:
	// Initialization methods
	virtual void Delete() = 0;
	virtual SuccessToken AllocateMemory() = 0;
	virtual void SetBufferLayout(ImageFormat imageFormat, DataFormat dataFormat, size_t entryCount) = 0;

	// Usage methods
	virtual void SetUsageFlags(UsageFlags usageFlags) = 0;

	// Initial data methods
	template<class T>
	inline SuccessToken SetInitialData(const std::vector<T>& sourceBuffer);
	template<class T>
	SuccessToken SetInitialData(std::vector<T>&& sourceBuffer) = delete;
	inline SuccessToken SetInitialData(const V1Int8* sourceBuffer, size_t sourceBufferSize);
	inline SuccessToken SetInitialData(const V2Int8* sourceBuffer, size_t sourceBufferSize);
	inline SuccessToken SetInitialData(const V4Int8* sourceBuffer, size_t sourceBufferSize);
	inline SuccessToken SetInitialData(const V1Int16* sourceBuffer, size_t sourceBufferSize);
	inline SuccessToken SetInitialData(const V2Int16* sourceBuffer, size_t sourceBufferSize);
	inline SuccessToken SetInitialData(const V4Int16* sourceBuffer, size_t sourceBufferSize);
	inline SuccessToken SetInitialData(const V1Int32* sourceBuffer, size_t sourceBufferSize);
	inline SuccessToken SetInitialData(const V2Int32* sourceBuffer, size_t sourceBufferSize);
	inline SuccessToken SetInitialData(const V4Int32* sourceBuffer, size_t sourceBufferSize);
	inline SuccessToken SetInitialData(const V1UInt8* sourceBuffer, size_t sourceBufferSize);
	inline SuccessToken SetInitialData(const V2UInt8* sourceBuffer, size_t sourceBufferSize);
	inline SuccessToken SetInitialData(const V4UInt8* sourceBuffer, size_t sourceBufferSize);
	inline SuccessToken SetInitialData(const V1UInt16* sourceBuffer, size_t sourceBufferSize);
	inline SuccessToken SetInitialData(const V2UInt16* sourceBuffer, size_t sourceBufferSize);
	inline SuccessToken SetInitialData(const V4UInt16* sourceBuffer, size_t sourceBufferSize);
	inline SuccessToken SetInitialData(const V1UInt32* sourceBuffer, size_t sourceBufferSize);
	inline SuccessToken SetInitialData(const V2UInt32* sourceBuffer, size_t sourceBufferSize);
	inline SuccessToken SetInitialData(const V4UInt32* sourceBuffer, size_t sourceBufferSize);
	inline SuccessToken SetInitialData(const V1Norm8* sourceBuffer, size_t sourceBufferSize);
	inline SuccessToken SetInitialData(const V2Norm8* sourceBuffer, size_t sourceBufferSize);
	inline SuccessToken SetInitialData(const V4Norm8* sourceBuffer, size_t sourceBufferSize);
	inline SuccessToken SetInitialData(const V1Norm16* sourceBuffer, size_t sourceBufferSize);
	inline SuccessToken SetInitialData(const V2Norm16* sourceBuffer, size_t sourceBufferSize);
	inline SuccessToken SetInitialData(const V4Norm16* sourceBuffer, size_t sourceBufferSize);
	inline SuccessToken SetInitialData(const V1UNorm8* sourceBuffer, size_t sourceBufferSize);
	inline SuccessToken SetInitialData(const V2UNorm8* sourceBuffer, size_t sourceBufferSize);
	inline SuccessToken SetInitialData(const V4UNorm8* sourceBuffer, size_t sourceBufferSize);
	inline SuccessToken SetInitialData(const V1UNorm16* sourceBuffer, size_t sourceBufferSize);
	inline SuccessToken SetInitialData(const V2UNorm16* sourceBuffer, size_t sourceBufferSize);
	inline SuccessToken SetInitialData(const V4UNorm16* sourceBuffer, size_t sourceBufferSize);
	inline SuccessToken SetInitialData(const V1Float16* sourceBuffer, size_t sourceBufferSize);
	inline SuccessToken SetInitialData(const V2Float16* sourceBuffer, size_t sourceBufferSize);
	inline SuccessToken SetInitialData(const V4Float16* sourceBuffer, size_t sourceBufferSize);
	inline SuccessToken SetInitialData(const V1Float32* sourceBuffer, size_t sourceBufferSize);
	inline SuccessToken SetInitialData(const V2Float32* sourceBuffer, size_t sourceBufferSize);
	inline SuccessToken SetInitialData(const V4Float32* sourceBuffer, size_t sourceBufferSize);
	virtual SuccessToken SetInitialData(const void* sourceBuffer, size_t sourceBufferSizeInBytes, SourceImageFormat imageFormat, SourceDataFormat dataFormat) = 0;

	// Data update methods
	template<class T>
	inline SuccessToken QueueDataUpdate(const std::vector<T>& sourceBuffer, size_t targetBufferOffset = 0, ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V1Int8* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset = 0, ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V2Int8* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset = 0, ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V4Int8* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset = 0, ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V1Int16* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset = 0, ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V2Int16* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset = 0, ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V4Int16* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset = 0, ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V1Int32* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset = 0, ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V2Int32* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset = 0, ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V4Int32* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset = 0, ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V1UInt8* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset = 0, ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V2UInt8* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset = 0, ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V4UInt8* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset = 0, ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V1UInt16* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset = 0, ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V2UInt16* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset = 0, ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V4UInt16* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset = 0, ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V1UInt32* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset = 0, ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V2UInt32* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset = 0, ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V4UInt32* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset = 0, ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V1Norm8* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset = 0, ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V2Norm8* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset = 0, ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V4Norm8* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset = 0, ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V1Norm16* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset = 0, ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V2Norm16* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset = 0, ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V4Norm16* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset = 0, ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V1UNorm8* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset = 0, ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V2UNorm8* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset = 0, ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V4UNorm8* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset = 0, ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V1UNorm16* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset = 0, ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V2UNorm16* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset = 0, ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V4UNorm16* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset = 0, ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V1Float16* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset = 0, ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V2Float16* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset = 0, ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V4Float16* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset = 0, ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V1Float32* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset = 0, ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V2Float32* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset = 0, ITransferBatch* transferBatch = nullptr);
	inline SuccessToken QueueDataUpdate(const V4Float32* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset = 0, ITransferBatch* transferBatch = nullptr);
	virtual SuccessToken QueueDataUpdate(const void* sourceBuffer, size_t sourceBufferSizeInBytes, SourceImageFormat imageFormat, SourceDataFormat dataFormat, size_t targetBufferOffset = 0, ITransferBatch* transferBatch = nullptr) = 0;

	// Data transfer methods
	virtual SuccessToken QueueDataTransfer(ITexelArray* targetBuffer, size_t transferCount, size_t sourceBufferOffset = 0, size_t targetBufferOffset = 0, ITransferBatch* transferBatch = nullptr) = 0;

	// Output capture methods
	virtual void AddOutputCaptureTarget(ITexelArrayOutput* captureTarget) = 0;
	virtual void RemoveOutputCaptureTarget(ITexelArrayOutput* captureTarget) = 0;

	// Size methods
	inline constexpr static size_t ElementCountPerPixelFromFormat(SourceImageFormat imageFormat);
	inline constexpr static size_t ElementCountPerPixelFromFormat(ImageFormat imageFormat);
	inline constexpr static size_t ByteSizePerElementFromFormat(SourceDataFormat dataFormat);
	inline constexpr static size_t ByteSizePerElementFromFormat(DataFormat dataFormat);

	// Format methods
	inline constexpr static bool ImageFormatsAreBinaryEquivalent(SourceImageFormat sourceImageFormat, ImageFormat imageFormat);
	inline constexpr static bool ImageFormatsAreBinaryEquivalent(ImageFormat sourceImageFormat, ImageFormat imageFormat);
	inline constexpr static bool DataFormatsAreBinaryEquivalent(SourceDataFormat sourceDataFormat, DataFormat dataFormat);
	inline constexpr static bool DataFormatsAreBinaryEquivalent(DataFormat sourceDataFormat, DataFormat dataFormat);

protected:
	// Constructors
	~ITexelArray() = default;
};

}} // namespace cobalt::graphics
#include "ITexelArray.inl"
