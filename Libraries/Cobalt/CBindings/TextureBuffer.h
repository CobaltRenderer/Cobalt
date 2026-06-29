// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Macros.h"
#include "Result.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Cobalt_TextureBuffer_Internal* Cobalt_TextureBuffer;

// Enumerations
typedef enum
{
	Cobalt_ImageFormat_R,
	Cobalt_ImageFormat_RG,
	Cobalt_ImageFormat_RGB,
	Cobalt_ImageFormat_RGBA,
	Cobalt_ImageFormat_BGR,
	Cobalt_ImageFormat_BGRA,
	Cobalt_ImageFormat_X,
	Cobalt_ImageFormat_XY,
	Cobalt_ImageFormat_XYZ,
	Cobalt_ImageFormat_XYZW,
	Cobalt_ImageFormat_Depth,
	Cobalt_ImageFormat_DepthAndStencil,
} Cobalt_ImageFormat;

typedef enum
{
	Cobalt_DataFormat_Int8,
	Cobalt_DataFormat_Int16,
	Cobalt_DataFormat_Int32,
	Cobalt_DataFormat_UInt8,
	Cobalt_DataFormat_UInt16,
	Cobalt_DataFormat_UInt32,
	Cobalt_DataFormat_Norm8,
	Cobalt_DataFormat_Norm16,
	Cobalt_DataFormat_UNorm8,
	Cobalt_DataFormat_UNorm16,
	Cobalt_DataFormat_Float16,
	Cobalt_DataFormat_Float32,
	Cobalt_DataFormat_DXT1,
	Cobalt_DataFormat_DXT3,
	Cobalt_DataFormat_DXT5,
	Cobalt_DataFormat_BPTC,
	Cobalt_DataFormat_ETC2,
	Cobalt_DataFormat_ASTC4x4,
	Cobalt_DataFormat_ASTC5x5,
	Cobalt_DataFormat_ASTC6x6,
	Cobalt_DataFormat_ASTC8x8,
	Cobalt_DataFormat_DepthUNorm16,
	Cobalt_DataFormat_DepthUNorm24,
	Cobalt_DataFormat_DepthUNorm24StencilUInt8,
	Cobalt_DataFormat_DepthFloat32,
	Cobalt_DataFormat_DepthFloat32StencilUInt8,
} Cobalt_DataFormat;

typedef enum
{
	Cobalt_SourceImageFormat_R,
	Cobalt_SourceImageFormat_RG,
	Cobalt_SourceImageFormat_RGB,
	Cobalt_SourceImageFormat_RGBA,
	Cobalt_SourceImageFormat_BGR,
	Cobalt_SourceImageFormat_BGRA,
	Cobalt_SourceImageFormat_X,
	Cobalt_SourceImageFormat_XY,
	Cobalt_SourceImageFormat_XYZ,
	Cobalt_SourceImageFormat_XYZW,
} Cobalt_SourceImageFormat;

typedef enum
{
	Cobalt_SourceDataFormat_Int8,
	Cobalt_SourceDataFormat_Int16,
	Cobalt_SourceDataFormat_Int32,
	Cobalt_SourceDataFormat_UInt8,
	Cobalt_SourceDataFormat_UInt16,
	Cobalt_SourceDataFormat_UInt32,
	Cobalt_SourceDataFormat_Norm8,
	Cobalt_SourceDataFormat_Norm16,
	Cobalt_SourceDataFormat_Norm32,
	Cobalt_SourceDataFormat_UNorm8,
	Cobalt_SourceDataFormat_UNorm16,
	Cobalt_SourceDataFormat_UNorm32,
	Cobalt_SourceDataFormat_Float16,
	Cobalt_SourceDataFormat_Float32,
	Cobalt_SourceDataFormat_DXT1,
	Cobalt_SourceDataFormat_DXT3,
	Cobalt_SourceDataFormat_DXT5,
	Cobalt_SourceDataFormat_BPTC,
	Cobalt_SourceDataFormat_ETC2,
	Cobalt_SourceDataFormat_ASTC4x4,
	Cobalt_SourceDataFormat_ASTC5x5,
	Cobalt_SourceDataFormat_ASTC6x6,
	Cobalt_SourceDataFormat_ASTC8x8,
} Cobalt_SourceDataFormat;

typedef enum
{
	Cobalt_CubeMapFace_PositiveX = 0,
	Cobalt_CubeMapFace_NegativeX = 1,
	Cobalt_CubeMapFace_PositiveY = 2,
	Cobalt_CubeMapFace_NegativeY = 3,
	Cobalt_CubeMapFace_PositiveZ = 4,
	Cobalt_CubeMapFace_NegativeZ = 5,
} Cobalt_CubeMapFace;

typedef enum
{
	Cobalt_SampleCount_1 = 1,
	Cobalt_SampleCount_2 = 2,
	Cobalt_SampleCount_4 = 4,
	Cobalt_SampleCount_8 = 8,
	Cobalt_SampleCount_16 = 16,
	Cobalt_SampleCount_32 = 32,
} Cobalt_SampleCount;

typedef enum
{
	Cobalt_TextureUsageFlags_Default = 0x00000000,
	Cobalt_TextureUsageFlags_ShaderInput = 0x00000001,
	Cobalt_TextureUsageFlags_FrameBufferOutput = 0x00000002,
	Cobalt_TextureUsageFlags_MultiSampleResolve = 0x00000004,
} Cobalt_TextureUsageFlags;

typedef enum
{
	Cobalt_TexturePerformanceHint_Default = 0x00000000,
	Cobalt_TexturePerformanceHint_ReadNever = 0x00000001,
	Cobalt_TexturePerformanceHint_ReadRarely = 0x00000002,
	Cobalt_TexturePerformanceHint_ReadOften = 0x00000004,
	Cobalt_TexturePerformanceHint_ReadFlagsMask = 0x000000FF,
	Cobalt_TexturePerformanceHint_WriteNever = 0x00000100,
	Cobalt_TexturePerformanceHint_WriteRarely = 0x00000200,
	Cobalt_TexturePerformanceHint_WriteOften = 0x00000400,
	Cobalt_TexturePerformanceHint_WriteFlagsMask = 0x0000FF00,
} Cobalt_TexturePerformanceHint;

typedef enum
{
	Cobalt_TextureDataPersistenceFlags_PersistAlways = 0x00000000,
	Cobalt_TextureDataPersistenceFlags_InvalidateExistingDataOnWrite = 0x000000001,
	Cobalt_TextureDataPersistenceFlags_InvalidateExistingDataAfterDrawComplete = 0x000000002,
} Cobalt_TextureDataPersistenceFlags;

// Usage methods
COBALT_FUNCTION_EXPORT void Cobalt_TextureBuffer_SetUsageFlags(Cobalt_TextureBuffer texture, Cobalt_TextureUsageFlags usageFlags);
COBALT_FUNCTION_EXPORT void Cobalt_TextureBuffer_SetPerformanceHints(Cobalt_TextureBuffer texture, Cobalt_TexturePerformanceHint performanceHintCpu, Cobalt_TexturePerformanceHint performanceHintGpu);
COBALT_FUNCTION_EXPORT void Cobalt_TextureBuffer_SetDataPersistenceFlags(Cobalt_TextureBuffer texture, Cobalt_TextureDataPersistenceFlags dataPersistenceFlags);

#ifdef __cplusplus
}
#endif
