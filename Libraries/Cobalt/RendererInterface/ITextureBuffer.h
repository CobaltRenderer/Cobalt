// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "SuccessToken.h"
#include "VectorTypes.h"
#include <cstdint>
namespace cobalt { namespace graphics {

class ITextureBuffer
{
public:
	// Enumerations
	enum class ImageFormat
	{
		R,
		RG,
		RGB,
		RGBA,
		BGR,
		BGRA,
		X,
		XY,
		XYZ,
		XYZW,
		Depth,
		DepthAndStencil,
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
		Norm16,
		UNorm8,
		UNorm16,
		Float16,
		Float32,
		DXT1,
		DXT3,
		DXT5,
		BPTC,
		ETC2,
		ASTC4x4,
		ASTC5x5,
		ASTC6x6,
		ASTC8x8,
		DepthUNorm16,
		DepthUNorm24,
		DepthUNorm24StencilUInt8,
		DepthFloat32,
		DepthFloat32StencilUInt8,
	};
	enum class SourceImageFormat
	{
		R,
		RG,
		RGB,
		RGBA,
		BGR,
		BGRA,
		X,
		XY,
		XYZ,
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
		DXT1,
		DXT3,
		DXT5,
		BPTC,
		ETC2,
		ASTC4x4,
		ASTC5x5,
		ASTC6x6,
		ASTC8x8,
	};
	enum class CubeMapFace : uint32_t
	{
		PositiveX = 0,
		NegativeX = 1,
		PositiveY = 2,
		NegativeY = 3,
		PositiveZ = 4,
		NegativeZ = 5,
	};
	enum class SampleCount : uint32_t
	{
		SampleCount1 = 1,
		SampleCount2 = 2,
		SampleCount4 = 4,
		SampleCount8 = 8,
		SampleCount16 = 16,
		SampleCount32 = 32,
	};
	enum class UsageFlags : uint32_t
	{
		Default = 0x00000000,
		ShaderInput = 0x00000001,
		FrameBufferOutput = 0x00000002,
		MultiSampleResolve = 0x00000004,
	};
	enum class PerformanceHint : uint32_t
	{
		Default = 0x00000000,
		ReadNever = 0x00000001,
		ReadRarely = 0x00000002,
		ReadOften = 0x00000004,
		ReadFlagsMask = 0x000000FF,
		WriteNever = 0x00000100,
		WriteRarely = 0x00000200,
		WriteOften = 0x00000400,
		WriteFlagsMask = 0x0000FF00,
	};
	enum class DataPersistenceFlags
	{
		PersistAlways = 0x00000000,
		InvalidateExistingDataOnWrite = 0x000000001,
		InvalidateExistingDataAfterDrawComplete = 0x000000002,
	};

public:
	// Usage methods
	virtual void SetUsageFlags(UsageFlags usageFlags) = 0;
	virtual void SetPerformanceHints(PerformanceHint performanceHintCpu, PerformanceHint performanceHintGpu) = 0;
	virtual void SetDataPersistenceFlags(DataPersistenceFlags dataPersistenceFlags) = 0;

	// Size methods
	inline constexpr static size_t ElementCountPerPixelFromFormat(SourceImageFormat imageFormat);
	inline constexpr static size_t ElementCountPerPixelFromFormat(ImageFormat imageFormat);
	inline constexpr static size_t ByteSizePerElementFromFormat(SourceDataFormat dataFormat);
	inline constexpr static size_t ByteSizePerElementFromFormat(DataFormat dataFormat);
	inline constexpr static bool IsCompressedTextureFormat(SourceDataFormat dataFormat);
	inline constexpr static bool IsCompressedTextureFormat(DataFormat dataFormat);
	inline static V2UInt32 CellDimensionsInPixelsFromFormat(SourceDataFormat dataFormat);
	inline static V2UInt32 CellDimensionsInPixelsFromFormat(DataFormat dataFormat);
	inline constexpr static size_t CellSizeInBytesFromFormat(SourceImageFormat imageFormat, SourceDataFormat dataFormat);
	inline constexpr static size_t CellSizeInBytesFromFormat(ImageFormat imageFormat, DataFormat dataFormat);

	// Format methods
	inline constexpr static bool ImageFormatsAreBinaryEquivalent(SourceImageFormat sourceImageFormat, ImageFormat imageFormat);
	inline constexpr static bool DataFormatsAreBinaryEquivalent(SourceDataFormat sourceDataFormat, DataFormat dataFormat);

protected:
	// Constructors
	~ITextureBuffer() = default;
};

}} // namespace cobalt::graphics
#include "ITextureBuffer.inl"
