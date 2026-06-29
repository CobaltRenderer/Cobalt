// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "Direct3DRenderer.h"
#include "Direct3DTransferBatch.h"
#include <Internal/TextureFormatConversion/TextureFormatConversion.pkg>
#include <Internal/RendererSupport/KnownDynamicCast.h>
#include <cmath>
#include <utility>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::Direct3DTextureBuffer(cobalt::logging::ILogger* log, Direct3DRenderer* renderer)
: _log(log), _renderer(renderer), _usageFlags(InterfaceType::UsageFlags::Default), _performanceHintCpu(InterfaceType::PerformanceHint::Default), _performanceHintGpu(InterfaceType::PerformanceHint::Default), _dataPersistenceFlags(InterfaceType::DataPersistenceFlags::PersistAlways)
{}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
SuccessToken Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::AllocateMemory()
{
	// Ensure the buffer is in a state where memory can be allocated
	if (_isMemoryAllocated)
	{
		_log->Error("Attempted to allocate memory for texture that has already been allocated.");
		return false;
	}
	if (!_formatSet)
	{
		_log->Error("Attempted to allocate memory for texture before the texture format has been set.");
		return false;
	}
	if (_mipmapLevels <= 0)
	{
		_log->Error("Attempted to allocate memory for texture before the texture dimensions have been set.");
		return false;
	}
	if (!_initialData.empty())
	{
		size_t requiredInitialDataEntries = _initialData.size();
		size_t suppliedInitialDataEntries = 0;
		for (size_t i = 0; i < requiredInitialDataEntries; ++i)
		{
			suppliedInitialDataEntries += (_initialData[i].data != nullptr ? 1 : 0);
		}
		if (requiredInitialDataEntries != suppliedInitialDataEntries)
		{
			_log->Error("Attempted to allocate memory for texture buffer with partial initial data. If any initial data is supplied, it must be provided for all mipmap levels and array slices. Only {0} entries have been set, but {1} entries are required.", suppliedInitialDataEntries, requiredInitialDataEntries);
			return false;
		}
	}

	// Calculate the internal data format to use
	if (!GetFormatNative(_requestedImageFormat, _requestedDataFormat, _imageFormat, _dataFormat, _internalFormat, _elementCount, _elementSizeInBytes, _pixelOffsetInBytes, _pixelStrideInBytes))
	{
		_log->Error("Failed to allocate memory for texture. The combination of image format {0} and data format {1} is not supported by this renderer.", _requestedImageFormat, _requestedDataFormat);
		return false;
	}

	// Now that we know the final image and data formats that are being used, perform format conversion on any initial
	// data as required.
	for (InitialDataEntry& entry : _initialData)
	{
		if (!InterfaceType::ImageFormatsAreBinaryEquivalent(entry.imageFormat, _imageFormat) || !InterfaceType::DataFormatsAreBinaryEquivalent(entry.dataFormat, _dataFormat))
		{
			if (!ConvertDataFormat(entry.data, entry.dataSizeInBytes, entry.imageFormat, entry.dataFormat, _imageFormat, _dataFormat, entry.convertedData))
			{
				_log->Error("AllocateMemory failed to convert initial data");
				return false;
			}
			entry.data = entry.convertedData.data();
			entry.dataSizeInBytes = entry.convertedData.size();
		}
	}

	// Calculate various settings and flags to use for the buffer
	UINT bindFlags;
	UINT cpuFlags;
	D3D11_USAGE usageType;
	CalculateBufferSettings(GetUsageFlags(), _imageFormat, GetPerformanceHintCpu(), GetPerformanceHintGpu(), _isInitialDataSet, bindFlags, cpuFlags, usageType);

	// Attempt to create the texture object
	if (!CreateTextureObject(_imageFormat, _dataFormat, _internalFormat, bindFlags, cpuFlags, usageType, _initialData))
	{
		_log->Error("Failed to create texture object.");
		return false;
	}
	_isMemoryAllocated = true;

	// Release any memory currently held by the initial data array
	_initialData.clear();

	// Flag that the build state has been modified, and return true.
	FlagBuildStateModified();
	return true;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
void Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::ReleaseMemory()
{
	// Delete the texture
	if (_isMemoryAllocated)
	{
		ReleaseMemoryInternal();
		_isMemoryAllocated = false;
	}
}

//----------------------------------------------------------------------------------------
// Usage methods
//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
typename InterfaceType::UsageFlags Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::GetUsageFlags() const
{
	return _usageFlags;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
typename InterfaceType::PerformanceHint Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::GetPerformanceHintCpu() const
{
	return _performanceHintCpu;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
typename InterfaceType::PerformanceHint Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::GetPerformanceHintGpu() const
{
	return _performanceHintGpu;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
typename InterfaceType::DataPersistenceFlags Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::GetDataPersistenceFlags() const
{
	return _dataPersistenceFlags;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
void Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::SetUsageFlags(typename InterfaceType::UsageFlags usageFlags)
{
	_usageFlags = usageFlags;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
void Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::SetPerformanceHints(typename InterfaceType::PerformanceHint performanceHintCpu, typename InterfaceType::PerformanceHint performanceHintGpu)
{
	_performanceHintCpu = performanceHintCpu;
	_performanceHintGpu = performanceHintGpu;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
void Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::SetDataPersistenceFlags(typename InterfaceType::DataPersistenceFlags dataPersistenceFlags)
{
	_dataPersistenceFlags = dataPersistenceFlags;
}

//----------------------------------------------------------------------------------------
// Format methods
//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
void Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::SetTextureFormat(typename InterfaceType::ImageFormat imageFormat, typename InterfaceType::DataFormat dataFormat)
{
	// Ensure the texture format can only be set once. This ensures any initial data that has been set will be in the
	// correct format, and also prevents attempts to modify this state after the texture has been created.
	if (_formatSet)
	{
		Log()->Error("Attempted to set texture format more than once. The original format has not been changed.");
		return;
	}

	// Set the requested image format
	_requestedImageFormat = imageFormat;
	_requestedDataFormat = dataFormat;
	_formatSet = true;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
void Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::GetTextureFormatInfo(ITextureBuffer::ImageFormat& imageFormat, ITextureBuffer::DataFormat& dataFormat, size_t& elementCount, size_t& elementSizeInBytes, size_t& pixelOffsetInBytes, size_t& pixelStrideInBytes, bool stencilComponent) const
{
	imageFormat = _imageFormat;
	dataFormat = _dataFormat;
	elementCount = _elementCount;
	if (_dataFormat == ITextureBuffer::DataFormat::DepthUNorm24)
	{
		pixelStrideInBytes = 4;
		elementSizeInBytes = 3;
		pixelOffsetInBytes = 0;
	}
	else if (_dataFormat == ITextureBuffer::DataFormat::DepthUNorm24StencilUInt8)
	{
		pixelStrideInBytes = 4;
		if (!stencilComponent)
		{
			elementSizeInBytes = 3;
			pixelOffsetInBytes = 0;
		}
		else
		{
			elementSizeInBytes = 1;
			pixelOffsetInBytes = 3;
		}
	}
	else if (_dataFormat == ITextureBuffer::DataFormat::DepthFloat32StencilUInt8)
	{
		pixelStrideInBytes = 8;
		if (!stencilComponent)
		{
			elementSizeInBytes = 4;
			pixelOffsetInBytes = 0;
		}
		else
		{
			elementSizeInBytes = 1;
			pixelOffsetInBytes = 4;
		}
	}
	else
	{
		elementSizeInBytes = _elementSizeInBytes;
		pixelOffsetInBytes = _pixelOffsetInBytes;
		pixelStrideInBytes = _pixelStrideInBytes;
	}
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
DXGI_FORMAT Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::GetNativeTextureFormat() const
{
	return _internalFormat;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
void Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::SetTextureDimensions(uint32_t faceLength, int mipmapLevelCount)
{
	// RJS - As of 2019-10-01, clang-tidy gets confused by this if statement, even with C++17 support enabled.
#ifndef __clang_analyzer__
	if constexpr (DimensionVectorType::Dimensions == 2)
	{
		DimensionVectorType imageDimensions(faceLength, faceLength);
		SetTextureDimensions(imageDimensions, 6, mipmapLevelCount);
	}
#endif
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
void Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::SetTextureDimensions(uint32_t faceLength, size_t arraySize, int mipmapLevelCount)
{
	// RJS - As of 2019-10-01, clang-tidy gets confused by this if statement, even with C++17 support enabled.
#ifndef __clang_analyzer__
	if constexpr (DimensionVectorType::Dimensions == 2)
	{
		DimensionVectorType imageDimensions(faceLength, faceLength);
		SetTextureDimensions(imageDimensions, 6 * arraySize, mipmapLevelCount);
	}
#endif
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
void Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::SetTextureDimensions(const DimensionVectorType& imageDimensions, int mipmapLevelCount)
{
	SetTextureDimensions(imageDimensions, 1, mipmapLevelCount);
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
void Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::SetTextureDimensions(const DimensionVectorType& imageDimensions, size_t arraySize, int mipmapLevelCount)
{
	// Ensure the texture dimensions can only be set once. This ensures any initial data that has been set will be
	// correct format, and also prevents attempts to modify this state after the texture has been created.
	if (_mipmapLevels > 0)
	{
		Log()->Error("Attempted to set texture dimensions more than once. The original dimensions have not been changed.");
		return;
	}

	// Ensure the specified array size and image dimensions are valid
	if (arraySize <= 0)
	{
		Log()->Error("Attempted to set texture array size of {0}. A texture must have an array size of at least 1.", arraySize);
		return;
	}
	uint32_t smallestImageDimension = GetSmallestImageDimension(imageDimensions);
	if (smallestImageDimension <= 0)
	{
		Log()->Error("Attempted to set texture dimensions with at least one dimension set to {0}. All texture dimensions must be 1 or greater.", smallestImageDimension);
		return;
	}

	// Set the number of mipmap levels for this texture, generating a full set of levels if a count of 0 is specified.
	if (mipmapLevelCount == 0)
	{
		mipmapLevelCount = CalculateMipmapLevelCount(imageDimensions);
	}
	_mipmapLevels = mipmapLevelCount;

	// Set the array size for this buffer
	_arraySize = arraySize;

	// Store the dimensions for each mipmap level of the image
	DimensionVectorType mipmapLevelDimensions = imageDimensions;
	_mipmapDimensions.resize(_mipmapLevels);
	_mipmapDimensions[0] = mipmapLevelDimensions;
	for (int i = 1; i < _mipmapLevels; ++i)
	{
		mipmapLevelDimensions = CalculateMipmapLevelDimensions(mipmapLevelDimensions);
		_mipmapDimensions[i] = mipmapLevelDimensions;
	}
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
typename InterfaceType::SampleCount Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::GetSampleCount() const
{
	return _sampleCount;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
bool Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::IsSampleCountSupported(typename InterfaceType::ImageFormat imageFormat, typename InterfaceType::DataFormat dataFormat, typename InterfaceType::SampleCount sampleCount) const
{
	// Resolve the requested format to the native texture format that would actually be allocated
	typename InterfaceType::ImageFormat selectedImageFormat = {};
	typename InterfaceType::DataFormat selectedDataFormat = {};
	DXGI_FORMAT nativeFormat = {};
	size_t elementCount = {};
	size_t elementSizeInBytes = {};
	size_t pixelOffsetInBytes = {};
	size_t pixelStrideInBytes = {};
	if (!GetFormatNative(imageFormat, dataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes))
	{
		return false;
	}

	// Query Direct3D for the broad capabilities exposed by the native format
	UINT formatSupport = {};
	if (FAILED(Renderer()->GetDevice()->CheckFormatSupport(nativeFormat, &formatSupport)))
	{
		return false;
	}

	// Calculate the capabilities required to use this as a two dimensional framebuffer attachment
	UINT nativeSampleCount = GetNativeSampleCountFromSampleCount(sampleCount);
	UINT requiredFormatSupport = D3D11_FORMAT_SUPPORT_TEXTURE2D;
	if ((selectedImageFormat == InterfaceType::ImageFormat::Depth) || (selectedImageFormat == InterfaceType::ImageFormat::DepthAndStencil))
	{
		requiredFormatSupport |= D3D11_FORMAT_SUPPORT_DEPTH_STENCIL;
	}
	else
	{
		requiredFormatSupport |= D3D11_FORMAT_SUPPORT_RENDER_TARGET;
		if (nativeSampleCount > 1)
		{
			requiredFormatSupport |= D3D11_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET;
		}
	}
	if ((formatSupport & requiredFormatSupport) != requiredFormatSupport)
	{
		return false;
	}

	// Single-sample textures are supported if the format can be used as the requested framebuffer attachment type
	if (nativeSampleCount <= 1)
	{
		return true;
	}

	// Query support for the exact multisample count requested by the caller
	UINT qualityLevels = {};
	return SUCCEEDED(Renderer()->GetDevice()->CheckMultisampleQualityLevels(nativeFormat, nativeSampleCount, &qualityLevels)) && (qualityLevels > 0);
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
void Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::SetSampleCount(typename InterfaceType::SampleCount sampleCount)
{
	_sampleCount = sampleCount;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
UINT Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::GetNativeSampleCountFromSampleCount(typename InterfaceType::SampleCount sampleCount)
{
	switch (sampleCount)
	{
	case InterfaceType::SampleCount::SampleCount1:
		return 1;
	case InterfaceType::SampleCount::SampleCount2:
		return 2;
	case InterfaceType::SampleCount::SampleCount4:
		return 4;
	case InterfaceType::SampleCount::SampleCount8:
		return 8;
	case InterfaceType::SampleCount::SampleCount16:
		return 16;
	case InterfaceType::SampleCount::SampleCount32:
		return 32;
	}
	UNREACHABLE();
	return {};
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
typename InterfaceType::ImageFormat Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::AllocatedImageFormat() const
{
	return _imageFormat;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
typename InterfaceType::DataFormat Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::AllocatedDataFormat() const
{
	return _dataFormat;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
size_t Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::ArraySize() const
{
	return _arraySize;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
int Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::MipmapLevelCount() const
{
	return _mipmapLevels;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
DimensionVectorType Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::MipmapLevelDimensions(int mipmapLevel) const
{
	return _mipmapDimensions[mipmapLevel];
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
int Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::CalculateMipmapLevelCount(const V1UInt32& imageDimensions)
{
	return 1 + (int)std::floor(std::log2(imageDimensions.X()));
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
int Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::CalculateMipmapLevelCount(const V2UInt32& imageDimensions)
{
	return 1 + (int)std::floor(std::log2(std::max(imageDimensions.X(), imageDimensions.Y())));
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
int Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::CalculateMipmapLevelCount(const V3UInt32& imageDimensions)
{
	return 1 + (int)std::floor(std::log2(std::max({imageDimensions.X(), imageDimensions.Y(), imageDimensions.Z()})));
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
V1UInt32 Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::CalculateMipmapLevelDimensions(const V1UInt32& parentLevelDimensions)
{
	return V1UInt32(std::max(1u, (parentLevelDimensions.X() / 2)));
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
V2UInt32 Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::CalculateMipmapLevelDimensions(const V2UInt32& parentLevelDimensions)
{
	return V2UInt32(std::max(1u, (parentLevelDimensions.X() / 2)), std::max(1u, (parentLevelDimensions.Y() / 2)));
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
V3UInt32 Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::CalculateMipmapLevelDimensions(const V3UInt32& parentLevelDimensions)
{
	return V3UInt32(std::max(1u, (parentLevelDimensions.X() / 2)), std::max(1u, (parentLevelDimensions.Y() / 2)), std::max(1u, (parentLevelDimensions.Z() / 2)));
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
uint32_t Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::GetSmallestImageDimension(const V1UInt32& imageDimensions)
{
	return imageDimensions.X();
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
uint32_t Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::GetSmallestImageDimension(const V2UInt32& imageDimensions)
{
	return std::min(imageDimensions.X(), imageDimensions.Y());
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
uint32_t Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::GetSmallestImageDimension(const V3UInt32& imageDimensions)
{
	return std::min({imageDimensions.X(), imageDimensions.Y(), imageDimensions.Z()});
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
bool Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::RegionIsWithinImageDimensions(const V1UInt32& imageDimensions, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels)
{
	return (imageOffsetInPixels.X() < imageDimensions.X()) && ((imageOffsetInPixels.X() + imageRegionInPixels.X()) <= imageDimensions.X());
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
bool Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::RegionIsWithinImageDimensions(const V2UInt32& imageDimensions, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels)
{
	return (imageOffsetInPixels.X() < imageDimensions.X()) && ((imageOffsetInPixels.X() + imageRegionInPixels.X()) <= imageDimensions.X()) && (imageOffsetInPixels.Y() < imageDimensions.Y()) && ((imageOffsetInPixels.Y() + imageRegionInPixels.Y()) <= imageDimensions.Y());
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
bool Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::RegionIsWithinImageDimensions(const V3UInt32& imageDimensions, const V3UInt32& imageOffsetInPixels, const V3UInt32& imageRegionInPixels)
{
	return (imageOffsetInPixels.X() < imageDimensions.X()) && ((imageOffsetInPixels.X() + imageRegionInPixels.X()) <= imageDimensions.X()) && (imageOffsetInPixels.Y() < imageDimensions.Y()) && ((imageOffsetInPixels.Y() + imageRegionInPixels.Y()) <= imageDimensions.Y()) && (imageOffsetInPixels.Z() < imageDimensions.Z()) && ((imageOffsetInPixels.Z() + imageRegionInPixels.Z()) <= imageDimensions.Z());
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
std::string Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::ImageDimensionAsString(const V1UInt32& imageDimensions)
{
	return "[" + std::to_string(imageDimensions.X()) + "]";
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
std::string Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::ImageDimensionAsString(const V2UInt32& imageDimensions)
{
	return "[" + std::to_string(imageDimensions.X()) + "," + std::to_string(imageDimensions.Y()) + "]";
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
std::string Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::ImageDimensionAsString(const V3UInt32& imageDimensions)
{
	return "[" + std::to_string(imageDimensions.X()) + "," + std::to_string(imageDimensions.Y()) + "," + std::to_string(imageDimensions.Z()) + "]";
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
constexpr bool Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::GetFormatNative(typename InterfaceType::ImageFormat requestedImageFormat, typename InterfaceType::DataFormat requestedDataFormat, typename InterfaceType::ImageFormat& selectedImageFormat, typename InterfaceType::DataFormat& selectedDataFormat, DXGI_FORMAT& nativeFormat, size_t& elementCount, size_t& elementSizeInBytes, size_t& pixelOffsetInBytes, size_t& pixelStrideInBytes, bool setElementParams)
{
	selectedImageFormat = requestedImageFormat;
	selectedDataFormat = requestedDataFormat;
	if (!setElementParams)
	{
		elementCount = InterfaceType::ElementCountPerPixelFromFormat(requestedImageFormat);
		elementSizeInBytes = InterfaceType::ByteSizePerElementFromFormat(requestedDataFormat);
		pixelStrideInBytes = elementSizeInBytes * elementCount;
		pixelOffsetInBytes = 0;
	}

	switch (requestedDataFormat)
	{
	case InterfaceType::DataFormat::Int8:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::R:
		case InterfaceType::ImageFormat::X:
			nativeFormat = DXGI_FORMAT_R8_SINT;
			return true;
		case InterfaceType::ImageFormat::RG:
		case InterfaceType::ImageFormat::XY:
			nativeFormat = DXGI_FORMAT_R8G8_SINT;
			return true;
		case InterfaceType::ImageFormat::RGB:
		case InterfaceType::ImageFormat::BGR:
			pixelStrideInBytes += elementSizeInBytes;
			return GetFormatNative(InterfaceType::ImageFormat::RGBA, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes, true);
		case InterfaceType::ImageFormat::XYZ:
			pixelStrideInBytes += elementSizeInBytes;
			return GetFormatNative(InterfaceType::ImageFormat::XYZW, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes, true);
		case InterfaceType::ImageFormat::RGBA:
		case InterfaceType::ImageFormat::XYZW:
			nativeFormat = DXGI_FORMAT_R8G8B8A8_SINT;
			return true;
		case InterfaceType::ImageFormat::BGRA:
			return GetFormatNative(InterfaceType::ImageFormat::RGBA, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes, true);
		}
		break;
	case InterfaceType::DataFormat::Int16:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::R:
		case InterfaceType::ImageFormat::X:
			nativeFormat = DXGI_FORMAT_R16_SINT;
			return true;
		case InterfaceType::ImageFormat::RG:
		case InterfaceType::ImageFormat::XY:
			nativeFormat = DXGI_FORMAT_R16G16_SINT;
			return true;
		case InterfaceType::ImageFormat::RGB:
		case InterfaceType::ImageFormat::BGR:
			pixelStrideInBytes += elementSizeInBytes;
			return GetFormatNative(InterfaceType::ImageFormat::RGBA, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes, true);
		case InterfaceType::ImageFormat::XYZ:
			pixelStrideInBytes += elementSizeInBytes;
			return GetFormatNative(InterfaceType::ImageFormat::XYZW, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes, true);
		case InterfaceType::ImageFormat::RGBA:
		case InterfaceType::ImageFormat::XYZW:
			nativeFormat = DXGI_FORMAT_R16G16B16A16_SINT;
			return true;
		case InterfaceType::ImageFormat::BGRA:
			return GetFormatNative(InterfaceType::ImageFormat::RGBA, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes, true);
		}
		break;
	case InterfaceType::DataFormat::Int32:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::R:
		case InterfaceType::ImageFormat::X:
			nativeFormat = DXGI_FORMAT_R32_SINT;
			return true;
		case InterfaceType::ImageFormat::RG:
		case InterfaceType::ImageFormat::XY:
			nativeFormat = DXGI_FORMAT_R32G32_SINT;
			return true;
		case InterfaceType::ImageFormat::RGB:
		case InterfaceType::ImageFormat::XYZ:
			nativeFormat = DXGI_FORMAT_R32G32B32_SINT;
			return true;
		case InterfaceType::ImageFormat::RGBA:
		case InterfaceType::ImageFormat::XYZW:
			nativeFormat = DXGI_FORMAT_R32G32B32A32_SINT;
			return true;
		case InterfaceType::ImageFormat::BGR:
			return GetFormatNative(InterfaceType::ImageFormat::RGB, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes, true);
		case InterfaceType::ImageFormat::BGRA:
			return GetFormatNative(InterfaceType::ImageFormat::RGBA, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes, true);
		}
		break;
	case InterfaceType::DataFormat::UInt8:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::R:
		case InterfaceType::ImageFormat::X:
			nativeFormat = DXGI_FORMAT_R8_UINT;
			return true;
		case InterfaceType::ImageFormat::RG:
		case InterfaceType::ImageFormat::XY:
			nativeFormat = DXGI_FORMAT_R8G8_UINT;
			return true;
		case InterfaceType::ImageFormat::RGB:
		case InterfaceType::ImageFormat::BGR:
			pixelStrideInBytes += elementSizeInBytes;
			return GetFormatNative(InterfaceType::ImageFormat::RGBA, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes, true);
		case InterfaceType::ImageFormat::XYZ:
			pixelStrideInBytes += elementSizeInBytes;
			return GetFormatNative(InterfaceType::ImageFormat::XYZW, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes, true);
		case InterfaceType::ImageFormat::RGBA:
		case InterfaceType::ImageFormat::XYZW:
			nativeFormat = DXGI_FORMAT_R8G8B8A8_UINT;
			return true;
		case InterfaceType::ImageFormat::BGRA:
			return GetFormatNative(InterfaceType::ImageFormat::RGBA, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes, true);
		}
		break;
	case InterfaceType::DataFormat::UInt16:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::R:
		case InterfaceType::ImageFormat::X:
			nativeFormat = DXGI_FORMAT_R16_UINT;
			return true;
		case InterfaceType::ImageFormat::RG:
		case InterfaceType::ImageFormat::XY:
			nativeFormat = DXGI_FORMAT_R16G16_UINT;
			return true;
		case InterfaceType::ImageFormat::RGB:
		case InterfaceType::ImageFormat::BGR:
			pixelStrideInBytes += elementSizeInBytes;
			return GetFormatNative(InterfaceType::ImageFormat::RGBA, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes, true);
		case InterfaceType::ImageFormat::XYZ:
			pixelStrideInBytes += elementSizeInBytes;
			return GetFormatNative(InterfaceType::ImageFormat::XYZW, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes, true);
		case InterfaceType::ImageFormat::RGBA:
		case InterfaceType::ImageFormat::XYZW:
			nativeFormat = DXGI_FORMAT_R16G16B16A16_UINT;
			return true;
		case InterfaceType::ImageFormat::BGRA:
			return GetFormatNative(InterfaceType::ImageFormat::RGBA, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes, true);
		}
		break;
	case InterfaceType::DataFormat::UInt32:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::R:
		case InterfaceType::ImageFormat::X:
			nativeFormat = DXGI_FORMAT_R32_UINT;
			return true;
		case InterfaceType::ImageFormat::RG:
		case InterfaceType::ImageFormat::XY:
			nativeFormat = DXGI_FORMAT_R32G32_UINT;
			return true;
		case InterfaceType::ImageFormat::RGB:
		case InterfaceType::ImageFormat::XYZ:
			nativeFormat = DXGI_FORMAT_R32G32B32_UINT;
			return true;
		case InterfaceType::ImageFormat::RGBA:
		case InterfaceType::ImageFormat::XYZW:
			nativeFormat = DXGI_FORMAT_R32G32B32A32_UINT;
			return true;
		case InterfaceType::ImageFormat::BGR:
			return GetFormatNative(InterfaceType::ImageFormat::RGB, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes, true);
		case InterfaceType::ImageFormat::BGRA:
			return GetFormatNative(InterfaceType::ImageFormat::RGBA, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes, true);
		}
		break;
	case InterfaceType::DataFormat::Norm8:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::R:
		case InterfaceType::ImageFormat::X:
			nativeFormat = DXGI_FORMAT_R8_SNORM;
			return true;
		case InterfaceType::ImageFormat::RG:
		case InterfaceType::ImageFormat::XY:
			nativeFormat = DXGI_FORMAT_R8G8_SNORM;
			return true;
		case InterfaceType::ImageFormat::RGB:
		case InterfaceType::ImageFormat::BGR:
			pixelStrideInBytes += elementSizeInBytes;
			return GetFormatNative(InterfaceType::ImageFormat::RGBA, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes, true);
		case InterfaceType::ImageFormat::XYZ:
			pixelStrideInBytes += elementSizeInBytes;
			return GetFormatNative(InterfaceType::ImageFormat::XYZW, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes, true);
		case InterfaceType::ImageFormat::RGBA:
		case InterfaceType::ImageFormat::XYZW:
			nativeFormat = DXGI_FORMAT_R8G8B8A8_SNORM;
			return true;
		case InterfaceType::ImageFormat::BGRA:
			return GetFormatNative(InterfaceType::ImageFormat::RGBA, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes, true);
		}
		break;
	case InterfaceType::DataFormat::Norm16:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::R:
		case InterfaceType::ImageFormat::X:
			nativeFormat = DXGI_FORMAT_R16_SNORM;
			return true;
		case InterfaceType::ImageFormat::RG:
		case InterfaceType::ImageFormat::XY:
			nativeFormat = DXGI_FORMAT_R16G16_SNORM;
			return true;
		case InterfaceType::ImageFormat::RGB:
		case InterfaceType::ImageFormat::BGR:
			pixelStrideInBytes += elementSizeInBytes;
			return GetFormatNative(InterfaceType::ImageFormat::RGBA, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes, true);
		case InterfaceType::ImageFormat::XYZ:
			pixelStrideInBytes += elementSizeInBytes;
			return GetFormatNative(InterfaceType::ImageFormat::XYZW, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes, true);
		case InterfaceType::ImageFormat::RGBA:
		case InterfaceType::ImageFormat::XYZW:
			nativeFormat = DXGI_FORMAT_R16G16B16A16_SNORM;
			return true;
		case InterfaceType::ImageFormat::BGRA:
			return GetFormatNative(InterfaceType::ImageFormat::RGBA, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes, true);
		}
		break;
	case InterfaceType::DataFormat::UNorm8:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::R:
		case InterfaceType::ImageFormat::X:
			nativeFormat = DXGI_FORMAT_R8_UNORM;
			return true;
		case InterfaceType::ImageFormat::RG:
		case InterfaceType::ImageFormat::XY:
			nativeFormat = DXGI_FORMAT_R8G8_UNORM;
			return true;
		case InterfaceType::ImageFormat::RGB:
			pixelStrideInBytes += elementSizeInBytes;
			return GetFormatNative(InterfaceType::ImageFormat::RGBA, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes, true);
		case InterfaceType::ImageFormat::XYZ:
			pixelStrideInBytes += elementSizeInBytes;
			return GetFormatNative(InterfaceType::ImageFormat::XYZW, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes, true);
		case InterfaceType::ImageFormat::RGBA:
		case InterfaceType::ImageFormat::XYZW:
			nativeFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
			return true;
		case InterfaceType::ImageFormat::BGR:
			pixelStrideInBytes += elementSizeInBytes;
			nativeFormat = DXGI_FORMAT_B8G8R8X8_UNORM;
			return true;
		case InterfaceType::ImageFormat::BGRA:
			nativeFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
			return true;
		}
		break;
	case InterfaceType::DataFormat::UNorm16:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::R:
		case InterfaceType::ImageFormat::X:
			nativeFormat = DXGI_FORMAT_R16_UNORM;
			return true;
		case InterfaceType::ImageFormat::RG:
		case InterfaceType::ImageFormat::XY:
			nativeFormat = DXGI_FORMAT_R16G16_UNORM;
			return true;
		case InterfaceType::ImageFormat::RGB:
		case InterfaceType::ImageFormat::BGR:
			pixelStrideInBytes += elementSizeInBytes;
			return GetFormatNative(InterfaceType::ImageFormat::RGBA, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes, true);
		case InterfaceType::ImageFormat::XYZ:
			pixelStrideInBytes += elementSizeInBytes;
			return GetFormatNative(InterfaceType::ImageFormat::XYZW, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes, true);
		case InterfaceType::ImageFormat::RGBA:
		case InterfaceType::ImageFormat::XYZW:
			nativeFormat = DXGI_FORMAT_R16G16B16A16_UNORM;
			return true;
		case InterfaceType::ImageFormat::BGRA:
			return GetFormatNative(InterfaceType::ImageFormat::RGBA, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes, true);
		}
		break;
	case InterfaceType::DataFormat::Float16:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::R:
		case InterfaceType::ImageFormat::X:
			nativeFormat = DXGI_FORMAT_R16_FLOAT;
			return true;
		case InterfaceType::ImageFormat::RG:
		case InterfaceType::ImageFormat::XY:
			nativeFormat = DXGI_FORMAT_R16G16_FLOAT;
			return true;
		case InterfaceType::ImageFormat::RGB:
		case InterfaceType::ImageFormat::BGR:
			pixelStrideInBytes += elementSizeInBytes;
			return GetFormatNative(InterfaceType::ImageFormat::RGBA, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes, true);
		case InterfaceType::ImageFormat::XYZ:
			pixelStrideInBytes += elementSizeInBytes;
			return GetFormatNative(InterfaceType::ImageFormat::XYZW, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes, true);
		case InterfaceType::ImageFormat::RGBA:
		case InterfaceType::ImageFormat::XYZW:
			nativeFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
			return true;
		case InterfaceType::ImageFormat::BGRA:
			return GetFormatNative(InterfaceType::ImageFormat::RGBA, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes, true);
		}
		break;
	case InterfaceType::DataFormat::Float32:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::R:
		case InterfaceType::ImageFormat::X:
			nativeFormat = DXGI_FORMAT_R32_FLOAT;
			return true;
		case InterfaceType::ImageFormat::RG:
		case InterfaceType::ImageFormat::XY:
			nativeFormat = DXGI_FORMAT_R32G32_FLOAT;
			return true;
		case InterfaceType::ImageFormat::RGB:
		case InterfaceType::ImageFormat::XYZ:
			nativeFormat = DXGI_FORMAT_R32G32B32_FLOAT;
			return true;
		case InterfaceType::ImageFormat::RGBA:
		case InterfaceType::ImageFormat::XYZW:
			nativeFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
			return true;
		case InterfaceType::ImageFormat::BGR:
		case InterfaceType::ImageFormat::BGRA:
			return GetFormatNative(InterfaceType::ImageFormat::RGBA, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes, true);
		}
		break;
	case InterfaceType::DataFormat::DXT1:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::RGB:
		case InterfaceType::ImageFormat::RGBA:
			nativeFormat = DXGI_FORMAT_BC1_UNORM;
			return true;
		}
		break;
	case InterfaceType::DataFormat::DXT3:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::RGBA:
			nativeFormat = DXGI_FORMAT_BC2_UNORM;
			return true;
		}
		break;
	case InterfaceType::DataFormat::DXT5:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::RGBA:
			nativeFormat = DXGI_FORMAT_BC3_UNORM;
			return true;
		}
		break;
	case InterfaceType::DataFormat::BPTC:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::RGBA:
			nativeFormat = DXGI_FORMAT_BC7_UNORM;
			return true;
		}
		break;
	case InterfaceType::DataFormat::DepthUNorm16:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::Depth:
			nativeFormat = DXGI_FORMAT_D16_UNORM;
			return true;
		}
		break;
	case InterfaceType::DataFormat::DepthUNorm24:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::Depth:
			nativeFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
			return true;
		}
		break;
	case InterfaceType::DataFormat::DepthUNorm24StencilUInt8:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::DepthAndStencil:
			nativeFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
			return true;
		}
		break;
	case InterfaceType::DataFormat::DepthFloat32:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::Depth:
			nativeFormat = DXGI_FORMAT_D32_FLOAT;
			return true;
		}
		break;
	case InterfaceType::DataFormat::DepthFloat32StencilUInt8:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::DepthAndStencil:
			nativeFormat = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
			return true;
		}
		break;
	}
	return false;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
constexpr void Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::CalculateBufferSettings(typename InterfaceType::UsageFlags usageFlags, typename InterfaceType::ImageFormat imageFormat, typename InterfaceType::PerformanceHint performanceHintCpu, typename InterfaceType::PerformanceHint performanceHintGpu, bool initialDataSet, UINT& bindFlags, UINT& cpuFlags, D3D11_USAGE& usageType)
{
	// Calculate the bind flags for the texture
	bindFlags = (usageFlags == InterfaceType::UsageFlags::Default) ? D3D11_BIND_SHADER_RESOURCE : 0;
	bindFlags |= ((usageFlags & InterfaceType::UsageFlags::FrameBufferOutput) != InterfaceType::UsageFlags::Default) ? ((imageFormat == InterfaceType::ImageFormat::Depth) || (imageFormat == InterfaceType::ImageFormat::DepthAndStencil) ? D3D11_BIND_DEPTH_STENCIL : D3D11_BIND_RENDER_TARGET) : 0;
	bindFlags |= ((usageFlags & InterfaceType::UsageFlags::ShaderInput) != InterfaceType::UsageFlags::Default) ? D3D11_BIND_SHADER_RESOURCE : 0;

	// Determine the usage flag to use when defining the Direct3D buffer
	if (performanceHintCpu == InterfaceType::PerformanceHint::Default)
	{
		if (initialDataSet)
		{
			usageType = D3D11_USAGE_IMMUTABLE;
			cpuFlags = 0;
		}
		else
		{
			usageType = D3D11_USAGE_DEFAULT;
			cpuFlags = 0;
		}
	}
	else
	{
		if (((performanceHintCpu & InterfaceType::PerformanceHint::WriteFlagsMask) == InterfaceType::PerformanceHint::WriteNever) && ((performanceHintGpu & InterfaceType::PerformanceHint::WriteFlagsMask) == InterfaceType::PerformanceHint::WriteNever))
		{
			usageType = D3D11_USAGE_IMMUTABLE;
			cpuFlags = 0;
		}
		else
		{
			usageType = D3D11_USAGE_DEFAULT;
			cpuFlags = 0;
		}
	}
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
constexpr UINT Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::CalculateSysMemPitch(typename InterfaceType::ImageFormat imageFormat, typename InterfaceType::DataFormat dataFormat, unsigned int width)
{
	// This calculation is accurate for all texture types, including compressed texture formats.
	V2UInt32 cellDimensions = InterfaceType::CellDimensionsInPixelsFromFormat(dataFormat);
	size_t cellSizeInBytes = InterfaceType::CellSizeInBytesFromFormat(imageFormat, dataFormat);
	UINT sysMemPitch = (UINT)cellSizeInBytes * ((width + (cellDimensions.X() - 1)) / cellDimensions.X());
	return sysMemPitch;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
constexpr void Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::CalculateSysMemPitchAndDepthPitch(typename InterfaceType::ImageFormat imageFormat, typename InterfaceType::DataFormat dataFormat, unsigned int width, unsigned int height, UINT& sysMemPitch, UINT& depthPitch)
{
	// This calculation is accurate for all texture types, including compressed texture formats.
	V2UInt32 cellDimensions = InterfaceType::CellDimensionsInPixelsFromFormat(dataFormat);
	size_t cellSizeInBytes = InterfaceType::CellSizeInBytesFromFormat(imageFormat, dataFormat);
	sysMemPitch = (UINT)cellSizeInBytes * ((width + (cellDimensions.X() - 1)) / cellDimensions.X());
	depthPitch = sysMemPitch * ((height + (cellDimensions.Y() - 1)) / cellDimensions.Y());
}

//----------------------------------------------------------------------------------------
// Initial data methods
//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
SuccessToken Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::SetInitialData(const void* sourceBuffer, size_t sourceBufferSizeInBytes, typename InterfaceType::SourceImageFormat imageFormat, typename InterfaceType::SourceDataFormat dataFormat, typename InterfaceType::CubeMapFace targetFace, int mipmapLevel)
{
	auto arrayIndexFinal = (size_t)targetFace;
	return SetInitialData(sourceBuffer, sourceBufferSizeInBytes, imageFormat, dataFormat, arrayIndexFinal, mipmapLevel);
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
SuccessToken Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::SetInitialData(const void* sourceBuffer, size_t sourceBufferSizeInBytes, typename InterfaceType::SourceImageFormat imageFormat, typename InterfaceType::SourceDataFormat dataFormat, typename InterfaceType::CubeMapFace targetFace, size_t arrayIndex, int mipmapLevel)
{
	auto arrayIndexFinal = (arrayIndex * 6) + (size_t)targetFace;
	return SetInitialData(sourceBuffer, sourceBufferSizeInBytes, imageFormat, dataFormat, arrayIndexFinal, mipmapLevel);
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
SuccessToken Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::SetInitialData(const void* sourceBuffer, size_t sourceBufferSizeInBytes, typename InterfaceType::SourceImageFormat imageFormat, typename InterfaceType::SourceDataFormat dataFormat, int mipmapLevel)
{
	return SetInitialData(sourceBuffer, sourceBufferSizeInBytes, imageFormat, dataFormat, 0, mipmapLevel);
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
SuccessToken Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::SetInitialData(const void* sourceBuffer, size_t sourceBufferSizeInBytes, typename InterfaceType::SourceImageFormat imageFormat, typename InterfaceType::SourceDataFormat dataFormat, size_t arrayIndex, int mipmapLevel)
{
	// Ensure we're in a valid state to perform this operation
	if (_isMemoryAllocated)
	{
		_log->Error("SetInitialData was called after memory has already been allocated.");
		return false;
	}
	if (!_formatSet)
	{
		_log->Error("SetInitialData was called before the image format has been set.");
		return false;
	}
	if (_mipmapLevels <= 0)
	{
		_log->Error("SetInitialData was called before the image dimensions have been set.");
		return false;
	}
	if (mipmapLevel >= _mipmapLevels)
	{
		_log->Error("SetInitialData was called with a mipmap level of {0}, but the highest defined mipmap level is {1}.", mipmapLevel, _mipmapLevels);
		return false;
	}
	if (arrayIndex >= _arraySize)
	{
		_log->Error("SetInitialData was called with an array index of {0}, but the buffer only has an array size of {1}.", arrayIndex, _arraySize);
		return false;
	}

	// If no initial data has been set before, resize the initial data array now.
	if (_initialData.empty())
	{
		_initialData.resize(_mipmapLevels * _arraySize);
	}

	// Ensure initial data hasn't already been provided for the target image
	size_t initialDataArrayIndex = (arrayIndex * _mipmapLevels) + mipmapLevel;
	if (_initialData[initialDataArrayIndex].data != nullptr)
	{
		_log->Error("SetInitialData was called with a mipmap level of {0} and an array index of {1}, but initial data has already been provided for that location.", mipmapLevel, arrayIndex);
		return false;
	}

	// Capture the supplied initial data
	InitialDataEntry initialDataEntry(imageFormat, dataFormat, arrayIndex, mipmapLevel);
	initialDataEntry.data = sourceBuffer;
	initialDataEntry.dataSizeInBytes = sourceBufferSizeInBytes;

	// Store the initial data for use when the buffer is allocated
	_initialData[initialDataArrayIndex] = std::move(initialDataEntry);
	return true;
}

//----------------------------------------------------------------------------------------
// Data update methods
//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
SuccessToken Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::QueueDataUpdate(const void* sourceBuffer, size_t sourceBufferSizeInBytes, typename InterfaceType::SourceImageFormat imageFormat, typename InterfaceType::SourceDataFormat dataFormat, typename InterfaceType::CubeMapFace targetFace, int mipmapLevel, const DimensionVectorType& imageOffsetInPixels, const DimensionVectorType& imageRegionInPixels, ITransferBatch* transferBatch)
{
	auto arrayIndexFinal = (size_t)targetFace;
	return QueueDataUpdate(sourceBuffer, sourceBufferSizeInBytes, imageFormat, dataFormat, arrayIndexFinal, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
SuccessToken Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::QueueDataUpdate(const void* sourceBuffer, size_t sourceBufferSizeInBytes, typename InterfaceType::SourceImageFormat imageFormat, typename InterfaceType::SourceDataFormat dataFormat, typename InterfaceType::CubeMapFace targetFace, size_t arrayIndex, int mipmapLevel, const DimensionVectorType& imageOffsetInPixels, const DimensionVectorType& imageRegionInPixels, ITransferBatch* transferBatch)
{
	auto arrayIndexFinal = (arrayIndex * 6) + (size_t)targetFace;
	return QueueDataUpdate(sourceBuffer, sourceBufferSizeInBytes, imageFormat, dataFormat, arrayIndexFinal, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
SuccessToken Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::QueueDataUpdate(const void* sourceBuffer, size_t sourceBufferSizeInBytes, typename InterfaceType::SourceImageFormat imageFormat, typename InterfaceType::SourceDataFormat dataFormat, int mipmapLevel, const DimensionVectorType& imageOffsetInPixels, const DimensionVectorType& imageRegionInPixels, ITransferBatch* transferBatch)
{
	return QueueDataUpdate(sourceBuffer, sourceBufferSizeInBytes, imageFormat, dataFormat, 0, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
SuccessToken Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::QueueDataUpdate(const void* sourceBuffer, size_t sourceBufferSizeInBytes, typename InterfaceType::SourceImageFormat imageFormat, typename InterfaceType::SourceDataFormat dataFormat, size_t arrayIndex, int mipmapLevel, const DimensionVectorType& imageOffsetInPixels, const DimensionVectorType& imageRegionInPixels, ITransferBatch* transferBatch)
{
	// Ensure we're in a valid state to perform this operation
	if (!_isMemoryAllocated)
	{
		_log->Error("QueueDataUpdate was called before memory has been allocated.");
		return false;
	}
	if (mipmapLevel >= _mipmapLevels)
	{
		_log->Error("QueueDataUpdate was called with a mipmap level of {0}, but the highest defined mipmap level is {1}.", mipmapLevel, _mipmapLevels);
		return false;
	}
	if (arrayIndex >= _arraySize)
	{
		_log->Error("QueueDataUpdate was called with an array start index of {0}, but the buffer only has an array size of {1}.", arrayIndex, _arraySize);
		return false;
	}
	if ((GetPerformanceHintCpu() & ITextureBuffer::PerformanceHint::WriteFlagsMask) == ITextureBuffer::PerformanceHint::WriteNever)
	{
		_log->Error("QueueDataUpdate was called for a texture that can't be modified.");
		return false;
	}

	// Ensure the write region is within the bounds of the buffer
	if (!RegionIsWithinImageDimensions(MipmapLevelDimensions(mipmapLevel), imageOffsetInPixels, imageRegionInPixels))
	{
		_log->Error("Attempted texture write at image offset {0} with region size {1}, which exceeds the image dimensions of {2} for mipmap level {3}.", ImageDimensionAsString(imageOffsetInPixels), ImageDimensionAsString(imageRegionInPixels), ImageDimensionAsString(MipmapLevelDimensions(mipmapLevel)), mipmapLevel);
		return false;
	}

	// If a transfer batch has been supplied, ensure it hasn't already been submitted.
	auto* transferBatchResolved = KnownDynamicCast<Direct3DTransferBatch*>(transferBatch);
	if ((transferBatchResolved != nullptr) && transferBatchResolved->IsSubmitted())
	{
		_log->Error("Attempted to queue a transfer using a transfer batch that has already been submitted");
		return false;
	}

	// Capture the supplied update settings and data, performing format conversion if required.
	PendingWrite pendingWrite(arrayIndex, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatchResolved);
	if (!ConvertDataFormat(sourceBuffer, sourceBufferSizeInBytes, imageFormat, dataFormat, _imageFormat, _dataFormat, pendingWrite.data))
	{
		_log->Error("QueueDataUpdate failed to convert source data");
		return false;
	}

	// If a transfer batch has been supplied, increment the usage count.
	if (transferBatchResolved != nullptr)
	{
		transferBatchResolved->IncrementUsageCount();
	}

	// Queue a task to update the buffer with the supplied data
	std::unique_lock<std::mutex> lock(_buildStateMutex);
	_state[_buildIndex].pendingWrites.push_back(std::move(pendingWrite));
	lock.unlock();
	FlagBuildStateModified();
	return true;
}

//----------------------------------------------------------------------------------------
// Data conversion methods
//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
bool Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::ConvertDataFormat(const void* sourceBuffer, size_t sourceBufferSizeInBytes, typename InterfaceType::SourceImageFormat sourceImageFormat, typename InterfaceType::SourceDataFormat sourceDataFormat, typename InterfaceType::ImageFormat targetImageFormat, typename InterfaceType::DataFormat targetDataFormat, std::vector<uint8_t>& targetBuffer) const
{
	// If the source and target image and data formats match, directly copy the source data to the target buffer.
	if (InterfaceType::ImageFormatsAreBinaryEquivalent(sourceImageFormat, targetImageFormat) && InterfaceType::DataFormatsAreBinaryEquivalent(sourceDataFormat, targetDataFormat))
	{
		targetBuffer.assign(reinterpret_cast<const uint8_t*>(sourceBuffer), reinterpret_cast<const uint8_t*>(sourceBuffer) + sourceBufferSizeInBytes);
		return true;
	}

	// If either the source or target data formats are compressed texture formats, but the formats aren't matching,
	// abort any further processing. While we could theoretically convert between or to/from compressed texture formats
	// at runtime, compressed texture formats are optimized for decompression speed, while compression can be very slow.
	// Compressed textures are intended to be generated ahead of time, so we leave it up to the caller to handle
	// generating compressed texture data.
	if (InterfaceType::IsCompressedTextureFormat(sourceDataFormat) || InterfaceType::IsCompressedTextureFormat(targetDataFormat))
	{
		_log->Error("Unable to convert image data from image format {0} and data format {1} to image format {2} and data format {3}. Conversion of compressed texture formats is not supported.", sourceImageFormat, sourceDataFormat, targetImageFormat, targetDataFormat);
		return false;
	}

	// Convert the data to the required format
	bool sourceDataFormatError = false;
	bool targetDataFormatError = false;
	bool result = TextureFormatConversion::ConvertTextureInputData(sourceBuffer, sourceBufferSizeInBytes, sourceImageFormat, sourceDataFormat, targetImageFormat, targetDataFormat, targetBuffer, sourceDataFormatError, targetDataFormatError);
	if (targetDataFormatError)
	{
		_log->Critical("Data conversion wasn't handled for texture with image format {0} and data format {1}", targetImageFormat, targetDataFormat);
	}
	if (sourceDataFormatError)
	{
		_log->Error("Attempted to write to texture with incompatible or unsupported image format {0} and data format {1}", sourceImageFormat, sourceDataFormat);
	}
	return result;
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
void Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::MigrateBuildStateToDrawState()
{
	std::swap(_buildIndex, _drawIndex);
	_stateModified.clear(std::memory_order_relaxed);
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
ID3D11ShaderResourceView* Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::GetShaderResourceView(ID3D11Device1* device)
{
	if (!_createdResourceView)
	{
		if (FAILED(device->CreateShaderResourceView(GetTextureAsResource(), nullptr, &_resourceView)))
		{
			_log->Error("Failed to create shader resource view for texture");
			return nullptr;
		}
		_createdResourceView = true;
	}
	return _resourceView.Get();
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
void Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::CompletePendingDataWrites(ID3D11Device1* device, ID3D11DeviceContext1* context)
{
	// Retrieve the set of pending writes
	std::vector<PendingWrite>& pendingWrites = _state[_drawIndex].pendingWrites;
	if (pendingWrites.empty())
	{
		return;
	}

	// Split pending writes into those that are ready to run now, and those that must remain queued until their batch
	// has been submitted.
	std::vector<PendingWrite> deferredWrites;
	deferredWrites.reserve(pendingWrites.size());
	for (PendingWrite& write : pendingWrites)
	{
		if ((write.transferBatch != nullptr) && !write.transferBatch->IsSubmitted())
		{
			deferredWrites.push_back(std::move(write));
			continue;
		}

		CompletePendingDataWrite(write, device, context);

		// If a transfer batch has been supplied, decrement the usage count.
		if (write.transferBatch != nullptr)
		{
			write.transferBatch->DecrementUsageCount();
		}
	}
	pendingWrites.clear();

	// Re-queue any deferred writes onto the build state so they remain pending for a later frame.
	if (!deferredWrites.empty())
	{
		std::unique_lock<std::mutex> lock(_buildStateMutex);
		auto& buildPendingWrites = _state[_buildIndex].pendingWrites;
		for (PendingWrite& write : deferredWrites)
		{
			buildPendingWrites.push_back(std::move(write));
		}
		lock.unlock();
		FlagBuildStateModified();
	}
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
void Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::FlagBuildStateModified()
{
	if (!_stateModified.test_and_set(std::memory_order_acquire))
	{
		FlagObjectModified();
	}
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
Direct3DRenderer* Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::Renderer() const
{
	return _renderer;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
cobalt::logging::ILogger* Direct3DTextureBuffer<InterfaceType, DimensionVectorType>::Log() const
{
	return _log;
}

} // namespace cobalt::graphics
