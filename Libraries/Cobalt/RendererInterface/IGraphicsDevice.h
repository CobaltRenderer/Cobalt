// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "IRenderer.h"
#include <Cobalt/Marshalling/Marshalling.pkg>
#include <set>
#include <string>
namespace cobalt { namespace graphics {
using namespace cobalt::marshalling::operators;

class IGraphicsDevice
{
public:
	// Enumerations
	enum class DeviceType
	{
		Discrete,
		Integrated,
		Software,
		Unknown,
	};
	enum class MemoryType
	{
		Dedicated,
		Shared,
	};
	enum class Feature
	{
		AnisotropicFiltering,
		GeometryShaders,
		ComputeShaders,
		MeshShaders,
		DepthBiasClamp,
		IndirectDraw,
		IndirectMultiDrawNative,
		InstanceOffset,
		PolygonWireframeFillMode,
		ResourceArrays,
		ShaderArraysOfArrays,
		SeparateBlendModePerTarget,
		SeparateTextureSamplers,
		TextureCubeArray,
		MipmapLevelBias,
	};
	enum class DepthRange
	{
		ZeroToOne,
		NegativeOneToOne,
	};
	enum class Vendor
	{
		AMD,
		ImgTec,
		Nvidia,
		ARM,
		Microsoft,
		Qualcomm,
		Intel,
		Apple,
		Mesa,
		Vivante,
		VeriSilicon,
		Unknown = 9999,
	};

	// Structures
	struct ImageLimits
	{
		uint32_t maxImageDimensionTexture1D;
		uint32_t maxImageDimensionTexture2D;
		uint32_t maxImageDimensionTexture3D;
		uint32_t maxImageDimensionTextureCube;
		uint32_t maxImageArraySizeTexture1D;
		uint32_t maxImageArraySizeTexture2D;
		int maxSamplerAnisotropicFilteringLevel;
	};
	struct ShaderLimits
	{
		uint32_t maxVertexShaderInputAttributes;
		uint32_t maxVertexShaderOutputComponents;
		uint32_t maxGeometryShaderInputComponents;
		uint32_t maxGeometryShaderOutputComponents;
		uint32_t maxGeometryShaderOutputVertices;
		uint32_t maxGeometryShaderTotalOutputComponents;
		uint32_t maxFragmentShaderInputComponents;
	};
	struct DrawLimits
	{
		uint32_t maxVertexCountPerDraw;
		uint32_t maxIndexValue;
		uint32_t maxTextureResourcesPerDraw;
		uint32_t maxResourcesPerDraw;
	};
	struct FrameBufferLimits
	{
		uint32_t maxFrameBufferWidth;
		uint32_t maxFrameBufferHeight;
		uint32_t maxFrameBufferColorAttachments;
		DepthRange depthRange;
	};
	struct DataBufferLimits
	{
		uint32_t maxStateBufferPageSize;

		uint32_t stateBufferAlignmentFloatOrInt;
		uint32_t stateBufferAlignmentVector4f;
		uint32_t stateBufferAlignmentMatrix4f;
		uint32_t stateBufferAlignmentArrayWhole;
		uint32_t stateBufferAlignmentArrayStride;
		uint32_t stateBufferAlignmentStruct;
	};

public:
	// Info methods
	virtual DeviceType GetDeviceType() const = 0;
	virtual Vendor GetVendor() const = 0;
	virtual Marshal::Ret<std::string> GetVendorName() const = 0;
	virtual Marshal::Ret<std::string> GetDeviceName() const = 0;
	virtual Marshal::Ret<std::string> GetDriverInfo() const = 0;
	virtual size_t GetMemorySizeInBytes(MemoryType memoryType) const = 0;

	// Limit methods
	virtual ImageLimits GetImageLimits() const = 0;
	virtual ShaderLimits GetShaderLimits() const = 0;
	virtual DrawLimits GetDrawLimits() const = 0;
	virtual FrameBufferLimits GetFrameBufferLimits() const = 0;
	virtual DataBufferLimits GetDataBufferLimits() const = 0;

	// Feature methods
	virtual bool IsFeatureSupported(Feature feature) const = 0;
	virtual bool AreAllFeaturesSupported(const Marshal::In<std::set<Feature>>& featureSet) const = 0;
	virtual Marshal::Ret<std::set<Feature>> GetAllSupportedFeatures() const = 0;
	virtual bool IsTextureFormatSupported(ITextureBuffer::ImageFormat imageFormat, ITextureBuffer::DataFormat dataFormat) const = 0;

	// Renderer methods
	virtual IRenderer::unique_ptr CreateRenderer(const Marshal::In<std::set<Feature>>& enabledFeatures, const Marshal::In<std::set<IRenderer::Options>>& enabledOptions) = 0;

protected:
	// Constructors
	~IGraphicsDevice() = default;
};

}} // namespace cobalt::graphics
