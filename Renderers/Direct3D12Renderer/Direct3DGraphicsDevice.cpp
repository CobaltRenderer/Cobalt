// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "Direct3DGraphicsDevice.h"
#include "Direct3DRenderer.h"
#include <Internal/RendererSupport/UnicodeConversion.h>
#include <Internal/RendererSupport/VendorName.h>
#include <algorithm>
#include <sstream>
#include <utility>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
Direct3DGraphicsDevice::Direct3DGraphicsDevice(cobalt::logging::ILogger* log, Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory, Microsoft::WRL::ComPtr<IDXGIAdapter3> adapter, const DXGI_ADAPTER_DESC2& adapterDescription)
: _log(log), _dxgiFactory(std::move(dxgiFactory)), _adapter(std::move(adapter)), _adapterDescription(adapterDescription)
{
	// Add our available features to the supported feature set
	_supportedFeatureSet.insert(Feature::AnisotropicFiltering);
	_supportedFeatureSet.insert(Feature::GeometryShaders);
	_supportedFeatureSet.insert(Feature::ComputeShaders);
	_supportedFeatureSet.insert(Feature::DepthBiasClamp);
	_supportedFeatureSet.insert(Feature::IndirectDraw);
	_supportedFeatureSet.insert(Feature::IndirectMultiDrawNative);
	_supportedFeatureSet.insert(Feature::InstanceOffset);
	_supportedFeatureSet.insert(Feature::MipmapLevelBias);
	_supportedFeatureSet.insert(Feature::PolygonWireframeFillMode);
	_supportedFeatureSet.insert(Feature::ResourceArrays);
	_supportedFeatureSet.insert(Feature::ShaderArraysOfArrays);
	_supportedFeatureSet.insert(Feature::SeparateBlendModePerTarget);
	_supportedFeatureSet.insert(Feature::SeparateTextureSamplers);
	_supportedFeatureSet.insert(Feature::TextureCubeArray);
}

//----------------------------------------------------------------------------------------
// Info methods
//----------------------------------------------------------------------------------------
Direct3DGraphicsDevice::DeviceType Direct3DGraphicsDevice::GetDeviceType() const
{
	// If the adapter indicates it has no dedicated video memory, or if it's from Intel, consider it to be an integrated
	// graphics device.
	//##TODO## Research if there's a more reliable way to determine this
	bool isIntegratedDevice = (_adapterDescription.DedicatedVideoMemory == 0) || (GetVendor() == Vendor::Intel);

	// If the adapter indicates it's a software device (IE, the WARP layer), mark it as software.
	bool isSoftwareDevice = ((_adapterDescription.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0);

	// Return the detected device type to the caller
	if (isSoftwareDevice)
	{
		return DeviceType::Software;
	}
	if (isIntegratedDevice)
	{
		return DeviceType::Integrated;
	}
	return DeviceType::Discrete;
}

//----------------------------------------------------------------------------------------
IGraphicsDevice::Vendor Direct3DGraphicsDevice::GetVendor() const
{
	return VendorCodeToVendor(_adapterDescription.VendorId);
}

//----------------------------------------------------------------------------------------
Marshal::Ret<std::string> Direct3DGraphicsDevice::GetVendorName() const
{
	return VendorToVendorName(GetVendor());
}

//----------------------------------------------------------------------------------------
Marshal::Ret<std::string> Direct3DGraphicsDevice::GetDeviceName() const
{
	return UTF16ToUTF8(&_adapterDescription.Description[0]);
}

//----------------------------------------------------------------------------------------
Marshal::Ret<std::string> Direct3DGraphicsDevice::GetDriverInfo() const
{
	// Attempt to obtain the driver version information
	LARGE_INTEGER version = {};
	if (FAILED(_adapter->CheckInterfaceSupport(__uuidof(IDXGIDevice), &version)))
	{
		return "Unknown";
	}

	// Return the driver version as a string
	const uint16_t major = HIWORD(version.HighPart);
	const uint16_t minor = LOWORD(version.HighPart);
	const uint16_t patch = HIWORD(version.LowPart);
	const uint16_t build = LOWORD(version.LowPart);
	std::ostringstream stringBuilder;
	stringBuilder << major << "." << minor << "." << patch << "." << build;
	return stringBuilder.str();
}

//----------------------------------------------------------------------------------------
size_t Direct3DGraphicsDevice::GetMemorySizeInBytes(MemoryType memoryType) const
{
	switch (memoryType)
	{
	case IGraphicsDevice::MemoryType::Dedicated:
		return (size_t)_adapterDescription.DedicatedVideoMemory + (size_t)_adapterDescription.DedicatedSystemMemory;
	case IGraphicsDevice::MemoryType::Shared:
		return (size_t)_adapterDescription.SharedSystemMemory;
	}
	return 0;
}

//----------------------------------------------------------------------------------------
// Limit methods
//----------------------------------------------------------------------------------------
Direct3DGraphicsDevice::ImageLimits Direct3DGraphicsDevice::GetImageLimits() const
{
	ImageLimits imageLimits = {};
	imageLimits.maxImageDimensionTexture1D = D3D12_REQ_TEXTURE1D_U_DIMENSION;
	imageLimits.maxImageDimensionTexture2D = D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION;
	imageLimits.maxImageDimensionTexture3D = D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
	imageLimits.maxImageDimensionTextureCube = D3D12_REQ_TEXTURECUBE_DIMENSION;
	imageLimits.maxImageArraySizeTexture1D = D3D12_REQ_TEXTURE1D_ARRAY_AXIS_DIMENSION;
	imageLimits.maxImageArraySizeTexture2D = D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
	imageLimits.maxSamplerAnisotropicFilteringLevel = D3D12_REQ_MAXANISOTROPY;
	return imageLimits;
}

//----------------------------------------------------------------------------------------
Direct3DGraphicsDevice::ShaderLimits Direct3DGraphicsDevice::GetShaderLimits() const
{
	ShaderLimits shaderLimits = {};
	shaderLimits.maxVertexShaderInputAttributes = D3D12_VS_INPUT_REGISTER_COUNT;
	shaderLimits.maxVertexShaderOutputComponents = D3D12_VS_OUTPUT_REGISTER_COUNT * D3D12_VS_OUTPUT_REGISTER_COMPONENTS;
	shaderLimits.maxGeometryShaderInputComponents = D3D12_GS_INPUT_REGISTER_COUNT * D3D12_GS_INPUT_REGISTER_COMPONENTS;
	shaderLimits.maxGeometryShaderOutputComponents = D3D12_GS_OUTPUT_REGISTER_COUNT * D3D12_GS_OUTPUT_REGISTER_COMPONENTS;
	shaderLimits.maxGeometryShaderOutputVertices = D3D12_GS_MAX_OUTPUT_VERTEX_COUNT_ACROSS_INSTANCES;
	shaderLimits.maxGeometryShaderTotalOutputComponents = D3D12_GS_MAX_OUTPUT_VERTEX_COUNT_ACROSS_INSTANCES * D3D12_GS_OUTPUT_REGISTER_COMPONENTS;
	shaderLimits.maxFragmentShaderInputComponents = D3D12_PS_INPUT_REGISTER_COUNT * D3D12_PS_INPUT_REGISTER_COMPONENTS;
	return shaderLimits;
}

//----------------------------------------------------------------------------------------
Direct3DGraphicsDevice::DrawLimits Direct3DGraphicsDevice::GetDrawLimits() const
{
	DrawLimits drawLimits = {};
	drawLimits.maxVertexCountPerDraw = (uint32_t)((1ull << D3D12_REQ_DRAW_VERTEX_COUNT_2_TO_EXP) - 1ull);
	drawLimits.maxIndexValue = (uint32_t)((1ull << D3D12_REQ_DRAWINDEXED_INDEX_COUNT_2_TO_EXP) - 1ull);

	drawLimits.maxResourcesPerDraw = D3D12_COMMONSHADER_INPUT_RESOURCE_REGISTER_COUNT;
	drawLimits.maxTextureResourcesPerDraw = drawLimits.maxResourcesPerDraw;
	return drawLimits;
}

//----------------------------------------------------------------------------------------
Direct3DGraphicsDevice::FrameBufferLimits Direct3DGraphicsDevice::GetFrameBufferLimits() const
{
	FrameBufferLimits frameBufferLimits = {};
	frameBufferLimits.maxFrameBufferWidth = D3D12_VIEWPORT_BOUNDS_MAX;
	frameBufferLimits.maxFrameBufferHeight = D3D12_VIEWPORT_BOUNDS_MAX;
	frameBufferLimits.maxFrameBufferColorAttachments = D3D12_PS_OUTPUT_REGISTER_COUNT;
	frameBufferLimits.depthRange = IGraphicsDevice::DepthRange::ZeroToOne;
	return frameBufferLimits;
}

//----------------------------------------------------------------------------------------
Direct3DGraphicsDevice::DataBufferLimits Direct3DGraphicsDevice::GetDataBufferLimits() const
{
	DataBufferLimits limits = {};

	// See https://stackoverflow.com/questions/18237406/d3d11-req-constant-buffer-element-count-actual-definition#:~:text=In%20DirectX%2011.1%2C%20constant%20buffers,of%20up%20to%204096%20elements.
	limits.maxStateBufferPageSize = D3D12_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * sizeof(V4Float32);

	// See https://docs.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-packing-rules
	limits.stateBufferAlignmentFloatOrInt = 4;
	limits.stateBufferAlignmentMatrix4f = 16;
	limits.stateBufferAlignmentVector4f = 16;
	limits.stateBufferAlignmentStruct = 16;
	limits.stateBufferAlignmentArrayWhole = 16;
	limits.stateBufferAlignmentArrayStride = 16;

	return limits;
}

//----------------------------------------------------------------------------------------
// Feature methods
//----------------------------------------------------------------------------------------
bool Direct3DGraphicsDevice::IsFeatureSupported(Feature feature) const
{
	return (_supportedFeatureSet.find(feature) != _supportedFeatureSet.end());
}

//----------------------------------------------------------------------------------------
bool Direct3DGraphicsDevice::AreAllFeaturesSupported(const Marshal::In<std::set<Feature>>& featureSet) const
{
	auto featureSetResolved = featureSet.Get();
	return std::includes(_supportedFeatureSet.begin(), _supportedFeatureSet.end(), featureSetResolved.begin(), featureSetResolved.end());
}

//----------------------------------------------------------------------------------------
Marshal::Ret<std::set<Direct3DGraphicsDevice::Feature>> Direct3DGraphicsDevice::GetAllSupportedFeatures() const
{
	return _supportedFeatureSet;
}

//----------------------------------------------------------------------------------------
bool Direct3DGraphicsDevice::IsTextureFormatSupported(ITextureBuffer::ImageFormat imageFormat, ITextureBuffer::DataFormat dataFormat) const
{
	// All non-compressed formats are supported either natively or via conversion, so return true for all of them.
	if (!ITextureBuffer::IsCompressedTextureFormat(dataFormat))
	{
		return true;
	}

	// If this is a compressed texture format, return true if it's supported.
	switch (dataFormat)
	{
	case ITextureBuffer::DataFormat::DXT1:
		return (imageFormat == ITextureBuffer::ImageFormat::RGB) || (imageFormat == ITextureBuffer::ImageFormat::RGBA);
	case ITextureBuffer::DataFormat::DXT3:
	case ITextureBuffer::DataFormat::DXT5:
	case ITextureBuffer::DataFormat::BPTC:
		return imageFormat == ITextureBuffer::ImageFormat::RGBA;
	default:
		return false;
	}
}

//----------------------------------------------------------------------------------------
// Renderer methods
//----------------------------------------------------------------------------------------
IRenderer::unique_ptr Direct3DGraphicsDevice::CreateRenderer(const Marshal::In<std::set<Feature>>& enabledFeatures, const Marshal::In<std::set<IRenderer::Options>>& enabledOptions)
{
	return IRenderer::unique_ptr(new Direct3DRenderer(_log->CloneLogger(), _dxgiFactory, _adapter, enabledFeatures.Get(), enabledOptions.Get()));
}

} // namespace cobalt::graphics
