// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Macros.h"
#include "Renderer.h"
#include "Result.h"
#include "TextureBuffer.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Cobalt_GraphicsDevice_Internal* Cobalt_GraphicsDevice;

// Enumerations
typedef enum
{
	Cobalt_DeviceType_Discrete,
	Cobalt_DeviceType_Integrated,
	Cobalt_DeviceType_Software,
	Cobalt_DeviceType_Unknown,
} Cobalt_DeviceType;

typedef enum
{
	Cobalt_MemoryType_Dedicated,
	Cobalt_MemoryType_Shared,
} Cobalt_MemoryType;

typedef enum
{
	Cobalt_Feature_AnisotropicFiltering,
	Cobalt_Feature_GeometryShaders,
	Cobalt_Feature_ComputeShaders,
	Cobalt_Feature_MeshShaders,
	Cobalt_Feature_DepthBiasClamp,
	Cobalt_Feature_IndirectDraw,
	Cobalt_Feature_IndirectMultiDrawNative,
	Cobalt_Feature_InstanceOffset,
	Cobalt_Feature_PolygonWireframeFillMode,
	Cobalt_Feature_ResourceArrays,
	Cobalt_Feature_ShaderArraysOfArrays,
	Cobalt_Feature_SeparateBlendModePerTarget,
	Cobalt_Feature_SeparateTextureSamplers,
	Cobalt_Feature_TextureCubeArray,
	Cobalt_Feature_MipmapLevelBias,
} Cobalt_Feature;

typedef enum
{
	Cobalt_DepthRange_ZeroToOne,
	Cobalt_DepthRange_NegativeOneToOne,
} Cobalt_DepthRange;

typedef enum
{
	Cobalt_Vendor_AMD,
	Cobalt_Vendor_ImgTec,
	Cobalt_Vendor_Nvidia,
	Cobalt_Vendor_ARM,
	Cobalt_Vendor_Microsoft,
	Cobalt_Vendor_Qualcomm,
	Cobalt_Vendor_Intel,
	Cobalt_Vendor_Apple,
	Cobalt_Vendor_Mesa,
	Cobalt_Vendor_Vivante,
	Cobalt_Vendor_VeriSilicon,
	Cobalt_Vendor_Unknown = 9999,
} Cobalt_Vendor;

// Structures
typedef struct Cobalt_ImageLimits
{
	uint32_t maxImageDimensionTexture1D;
	uint32_t maxImageDimensionTexture2D;
	uint32_t maxImageDimensionTexture3D;
	uint32_t maxImageDimensionTextureCube;
	uint32_t maxImageArraySizeTexture1D;
	uint32_t maxImageArraySizeTexture2D;
	int maxSamplerAnisotropicFilteringLevel;
} Cobalt_ImageLimits;

typedef struct Cobalt_ShaderLimits
{
	uint32_t maxVertexShaderInputAttributes;
	uint32_t maxVertexShaderOutputComponents;
	uint32_t maxGeometryShaderInputComponents;
	uint32_t maxGeometryShaderOutputComponents;
	uint32_t maxGeometryShaderOutputVertices;
	uint32_t maxGeometryShaderTotalOutputComponents;
	uint32_t maxFragmentShaderInputComponents;
} Cobalt_ShaderLimits;

typedef struct Cobalt_DrawLimits
{
	uint32_t maxVertexCountPerDraw;
	uint32_t maxIndexValue;
	uint32_t maxTextureResourcesPerDraw;
	uint32_t maxResourcesPerDraw;
} Cobalt_DrawLimits;

typedef struct Cobalt_FrameBufferLimits
{
	uint32_t maxFrameBufferWidth;
	uint32_t maxFrameBufferHeight;
	uint32_t maxFrameBufferColorAttachments;
	Cobalt_DepthRange depthRange;
} Cobalt_FrameBufferLimits;

typedef struct Cobalt_DataBufferLimits
{
	uint32_t maxStateBufferPageSize;
	uint32_t stateBufferAlignmentFloatOrInt;
	uint32_t stateBufferAlignmentVector4f;
	uint32_t stateBufferAlignmentMatrix4f;
	uint32_t stateBufferAlignmentArrayWhole;
	uint32_t stateBufferAlignmentArrayStride;
	uint32_t stateBufferAlignmentStruct;
} Cobalt_DataBufferLimits;

// Info methods
COBALT_FUNCTION_EXPORT void Cobalt_GraphicsDevice_GetDeviceType(Cobalt_GraphicsDevice device, Cobalt_DeviceType* deviceType);
COBALT_FUNCTION_EXPORT void Cobalt_GraphicsDevice_GetVendor(Cobalt_GraphicsDevice device, Cobalt_Vendor* vendor);
COBALT_FUNCTION_EXPORT void Cobalt_GraphicsDevice_GetVendorName(Cobalt_GraphicsDevice device, char* vendorName, size_t* vendorNameLength);
COBALT_FUNCTION_EXPORT void Cobalt_GraphicsDevice_GetDeviceName(Cobalt_GraphicsDevice device, char* deviceName, size_t* deviceNameLength);
COBALT_FUNCTION_EXPORT void Cobalt_GraphicsDevice_GetDriverInfo(Cobalt_GraphicsDevice device, char* driverInfo, size_t* driverInfoLength);
COBALT_FUNCTION_EXPORT size_t Cobalt_GraphicsDevice_GetMemorySizeInBytes(Cobalt_GraphicsDevice device, Cobalt_MemoryType memoryType);

// Limit methods
COBALT_FUNCTION_EXPORT void Cobalt_GraphicsDevice_GetImageLimits(Cobalt_GraphicsDevice device, Cobalt_ImageLimits* imageLimits);
COBALT_FUNCTION_EXPORT void Cobalt_GraphicsDevice_GetShaderLimits(Cobalt_GraphicsDevice device, Cobalt_ShaderLimits* shaderLimits);
COBALT_FUNCTION_EXPORT void Cobalt_GraphicsDevice_GetDrawLimits(Cobalt_GraphicsDevice device, Cobalt_DrawLimits* drawLimits);
COBALT_FUNCTION_EXPORT void Cobalt_GraphicsDevice_GetFrameBufferLimits(Cobalt_GraphicsDevice device, Cobalt_FrameBufferLimits* frameBufferLimits);
COBALT_FUNCTION_EXPORT void Cobalt_GraphicsDevice_GetDataBufferLimits(Cobalt_GraphicsDevice device, Cobalt_DataBufferLimits* dataBufferLimits);

// Feature methods
COBALT_FUNCTION_EXPORT char Cobalt_GraphicsDevice_IsFeatureSupported(Cobalt_GraphicsDevice device, Cobalt_Feature feature);
COBALT_FUNCTION_EXPORT char Cobalt_GraphicsDevice_AreAllFeaturesSupported(Cobalt_GraphicsDevice device, const Cobalt_Feature* featureSet, size_t featureSetLength);
COBALT_FUNCTION_EXPORT void Cobalt_GraphicsDevice_GetAllSupportedFeatures(Cobalt_GraphicsDevice device, Cobalt_Feature* featureSet, size_t* featureSetLength);
COBALT_FUNCTION_EXPORT char Cobalt_GraphicsDevice_IsTextureFormatSupported(Cobalt_GraphicsDevice device, Cobalt_ImageFormat imageFormat, Cobalt_DataFormat dataFormat);

// Renderer methods
COBALT_FUNCTION_EXPORT Cobalt_Result Cobalt_GraphicsDevice_CreateRenderer(Cobalt_GraphicsDevice device, const Cobalt_Feature* enabledFeatures, size_t enabledFeaturesLength, const Cobalt_RendererOption* enabledOptions, size_t enabledOptionsLength, Cobalt_Renderer* renderer);

#ifdef __cplusplus
}
#endif
