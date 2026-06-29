// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "OpenGLDebug.h"
#include "OpenGLRenderer.h"
#include "OpenGLTransferBatch.h"
#include <Internal/TextureFormatConversion/TextureFormatConversion.pkg>
#include <Internal/RendererSupport/KnownDynamicCast.h>
#include <cmath>
#include <utility>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::OpenGLTextureBuffer(cobalt::logging::ILogger* log, OpenGLRenderer* renderer, GLenum textureTarget)
: _log(log), _renderer(renderer), _textureTarget(textureTarget), _usageFlags(InterfaceType::UsageFlags::Default), _performanceHintCpu(InterfaceType::PerformanceHint::Default), _performanceHintGpu(InterfaceType::PerformanceHint::Default), _dataPersistenceFlags(InterfaceType::DataPersistenceFlags::PersistAlways)
{}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::~OpenGLTextureBuffer()
{
	ReleaseMemory();
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
SuccessToken OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::AllocateMemory()
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
	if (!GetFormatNative(_requestedImageFormat, _requestedDataFormat, _internalFormat))
	{
		_log->Error("Failed to allocate memory for texture. The combination of image format {0} and data format {1} is not supported by this renderer.", _requestedImageFormat, _requestedDataFormat);
		return false;
	}
	_imageFormat = _requestedImageFormat;
	_dataFormat = _requestedDataFormat;

	// Record whether we've selected a compressed texture format or not
	_isCompressedTexture = InterfaceType::IsCompressedTextureFormat(_dataFormat);

	// Now that we know the final image and data formats that are being used, perform format conversion on any initial
	// data as required, and populate the native format flags.
	for (InitialDataEntry& entry : _initialData)
	{
		// Adjust our target image format to allow for the fact that we can't directly upload data in some native formats
		typename InterfaceType::ImageFormat selectedImageFormat;
		typename InterfaceType::DataFormat selectedDataFormat;
		AdjustSourceDataTargetFormat(entry.imageFormat, entry.dataFormat, _imageFormat, _dataFormat, selectedImageFormat, selectedDataFormat);

		// Convert the data format if required. Note that in all cases, we copy the input data here now, even if
		// conversion isn't required. We need to do this, as we defer the buffer creation to the render thread here, and
		// the caller is free to release this memory after this method returns.
		if (!ConvertDataFormat(entry.data, entry.dataSizeInBytes, entry.imageFormat, entry.dataFormat, selectedImageFormat, selectedDataFormat, entry.convertedData, entry.convertedStencilData))
		{
			_log->Error("AllocateMemory failed to convert initial data");
			return false;
		}
		entry.data = entry.convertedData.data();
		entry.dataSizeInBytes = entry.convertedData.size();

		// Populate the native format flags
		entry.nativeImageFormat = (_isCompressedTexture ? (GLenum)_internalFormat : GetImageFormatNative(selectedImageFormat, selectedDataFormat, false));
		entry.nativeDataFormat = GetDataFormatNative(selectedDataFormat, false);
		if (_imageFormat == InterfaceType::ImageFormat::DepthAndStencil)
		{
			entry.stencilNativeImageFormat = GetImageFormatNative(selectedImageFormat, selectedDataFormat, true);
			entry.stencilNativeDataFormat = GetDataFormatNative(selectedDataFormat, true);
		}
	}

	// Flag that memory has been allocated, and set the creation of the native buffer object as pending.
	_isMemoryAllocated = true;
	_nativeBufferCreationPending = true;

	// Flag that the build state has been modified, and return true.
	FlagBuildStateModified();
	return true;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
void OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::ReleaseMemory()
{
	// Delete the texture
	if (_isMemoryAllocated)
	{
		glDeleteTextures(1, &_textureNo);
		CheckGLError(Log());
		_isMemoryAllocated = false;
	}
}

//----------------------------------------------------------------------------------------
// Usage methods
//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
typename InterfaceType::UsageFlags OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::GetUsageFlags() const
{
	return _usageFlags;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
typename InterfaceType::PerformanceHint OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::GetPerformanceHintCpu() const
{
	return _performanceHintCpu;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
typename InterfaceType::PerformanceHint OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::GetPerformanceHintGpu() const
{
	return _performanceHintGpu;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
typename InterfaceType::DataPersistenceFlags OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::GetDataPersistenceFlags() const
{
	return _dataPersistenceFlags;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
void OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::SetUsageFlags(typename InterfaceType::UsageFlags usageFlags)
{
	_usageFlags = usageFlags;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
void OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::SetPerformanceHints(typename InterfaceType::PerformanceHint performanceHintCpu, typename InterfaceType::PerformanceHint performanceHintGpu)
{
	_performanceHintCpu = performanceHintCpu;
	_performanceHintGpu = performanceHintGpu;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
void OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::SetDataPersistenceFlags(typename InterfaceType::DataPersistenceFlags dataPersistenceFlags)
{
	_dataPersistenceFlags = dataPersistenceFlags;
}

//----------------------------------------------------------------------------------------
// Format methods
//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
void OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::SetTextureFormat(typename InterfaceType::ImageFormat imageFormat, typename InterfaceType::DataFormat dataFormat)
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
void OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::SetTextureDimensions(uint32_t faceLength, int mipmapLevelCount)
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
void OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::SetTextureDimensions(uint32_t faceLength, size_t arraySize, int mipmapLevelCount)
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
void OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::SetTextureDimensions(const DimensionVectorType& imageDimensions, int mipmapLevelCount)
{
	SetTextureDimensions(imageDimensions, 1, mipmapLevelCount);
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
void OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::SetTextureDimensions(const DimensionVectorType& imageDimensions, size_t arraySize, int mipmapLevelCount)
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
typename InterfaceType::SampleCount OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::GetSampleCount() const
{
	return _sampleCount;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
bool OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::IsSampleCountSupported(typename InterfaceType::ImageFormat imageFormat, typename InterfaceType::DataFormat dataFormat, typename InterfaceType::SampleCount sampleCount) const
{
	// A single sample texture uses the normal texture allocation path. If the requested format maps to a native OpenGL
	// internal format, there is no multisample support to verify.
	GLsizei nativeSampleCount = GetNativeSampleCountFromSampleCount(sampleCount);
	if (nativeSampleCount == 1)
	{
		return true;
	}

	// Resolve the requested Cobalt format to the native texture format that would actually be allocated.
	GLint internalFormat = {};
	if (!GetFormatNative(imageFormat, dataFormat, internalFormat))
	{
		return false;
	}

	// Query the exact sample counts supported for this texture target and internal format. This is more precise than
	// the global GL_MAX_*_SAMPLES limits, which only report broad device limits and don't guarantee a specific
	// format/sample combination is valid.
	return Renderer()->RenderThreadInvokeSync([&] {
#ifdef GL_VERSION_4_3
		// Retrieve the set of supported sample counts for the target format
		GLint supportedSampleCountCount = 0;
		glGetInternalformativ(GL_TEXTURE_2D_MULTISAMPLE, (GLenum)internalFormat, GL_NUM_SAMPLE_COUNTS, 1, &supportedSampleCountCount);
		if (supportedSampleCountCount <= 0)
		{
			return false;
		}
		std::vector<GLint> supportedSampleCounts((size_t)supportedSampleCountCount, 0);
		glGetInternalformativ(GL_TEXTURE_2D_MULTISAMPLE, (GLenum)internalFormat, GL_SAMPLES, supportedSampleCountCount, supportedSampleCounts.data());

		// Return true if the requested sample count is supported
		for (GLint supportedSampleCount : supportedSampleCounts)
		{
			if (supportedSampleCount == nativeSampleCount)
			{
				return true;
			}
		}
		return false;
#else
#ifdef GL_ARB_internalformat_query
		// On OpenGL 3.3, prefer GL_ARB_internalformat_query when it is exposed. This gives us the same exact
		// format/sample list as the OpenGL 4.3 path.
		if (GLAD_GL_ARB_internalformat_query != 0)
		{
			// Retrieve the set of supported sample counts for the target format
			GLint supportedSampleCountCount = 0;
			glGetInternalformativ(GL_TEXTURE_2D_MULTISAMPLE, (GLenum)internalFormat, GL_NUM_SAMPLE_COUNTS, 1, &supportedSampleCountCount);
			if (supportedSampleCountCount <= 0)
			{
				return false;
			}
			std::vector<GLint> supportedSampleCounts((size_t)supportedSampleCountCount);
			glGetInternalformativ(GL_TEXTURE_2D_MULTISAMPLE, (GLenum)internalFormat, GL_SAMPLES, supportedSampleCountCount, supportedSampleCounts.data());

			// Return true if the requested sample count is supported
			for (GLint supportedSampleCount : supportedSampleCounts)
			{
				if (supportedSampleCount == nativeSampleCount)
				{
					return true;
				}
			}
			return false;
		}
#endif

		// Retrieve the overall sample limits for the device
		GLint maxSupportedSampleCount = 1;
		if ((imageFormat == InterfaceType::ImageFormat::Depth) || (imageFormat == InterfaceType::ImageFormat::DepthAndStencil))
		{
			glGetIntegerv(GL_MAX_DEPTH_TEXTURE_SAMPLES, &maxSupportedSampleCount);
		}
		else
		{
			glGetIntegerv(GL_MAX_COLOR_TEXTURE_SAMPLES, &maxSupportedSampleCount);
		}
		// Return true if the requested sample count is supported
		return (nativeSampleCount <= maxSupportedSampleCount);
#endif
	});
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
void OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::SetSampleCount(typename InterfaceType::SampleCount sampleCount)
{
	_sampleCount = sampleCount;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
GLsizei OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::GetNativeSampleCountFromSampleCount(typename InterfaceType::SampleCount sampleCount)
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
typename InterfaceType::ImageFormat OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::AllocatedImageFormat() const
{
	return _imageFormat;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
typename InterfaceType::DataFormat OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::AllocatedDataFormat() const
{
	return _dataFormat;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
bool OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::IsCompressedTexture() const
{
	return _isCompressedTexture;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
size_t OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::ArraySize() const
{
	return _arraySize;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
int OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::MipmapLevelCount() const
{
	return _mipmapLevels;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
DimensionVectorType OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::MipmapLevelDimensions(int mipmapLevel) const
{
	return _mipmapDimensions[mipmapLevel];
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
int OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::CalculateMipmapLevelCount(const V1UInt32& imageDimensions)
{
	return 1 + (int)std::floor(std::log2(imageDimensions.X()));
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
int OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::CalculateMipmapLevelCount(const V2UInt32& imageDimensions)
{
	return 1 + (int)std::floor(std::log2(std::max(imageDimensions.X(), imageDimensions.Y())));
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
int OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::CalculateMipmapLevelCount(const V3UInt32& imageDimensions)
{
	return 1 + (int)std::floor(std::log2(std::max({imageDimensions.X(), imageDimensions.Y(), imageDimensions.Z()})));
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
V1UInt32 OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::CalculateMipmapLevelDimensions(const V1UInt32& parentLevelDimensions)
{
	return V1UInt32(std::max(1u, (parentLevelDimensions.X() / 2)));
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
V2UInt32 OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::CalculateMipmapLevelDimensions(const V2UInt32& parentLevelDimensions)
{
	return V2UInt32(std::max(1u, (parentLevelDimensions.X() / 2)), std::max(1u, (parentLevelDimensions.Y() / 2)));
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
V3UInt32 OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::CalculateMipmapLevelDimensions(const V3UInt32& parentLevelDimensions)
{
	return V3UInt32(std::max(1u, (parentLevelDimensions.X() / 2)), std::max(1u, (parentLevelDimensions.Y() / 2)), std::max(1u, (parentLevelDimensions.Z() / 2)));
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
uint32_t OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::GetSmallestImageDimension(const V1UInt32& imageDimensions)
{
	return imageDimensions.X();
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
uint32_t OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::GetSmallestImageDimension(const V2UInt32& imageDimensions)
{
	return std::min(imageDimensions.X(), imageDimensions.Y());
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
uint32_t OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::GetSmallestImageDimension(const V3UInt32& imageDimensions)
{
	return std::min({imageDimensions.X(), imageDimensions.Y(), imageDimensions.Z()});
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
bool OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::RegionIsWithinImageDimensions(const V1UInt32& imageDimensions, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels)
{
	return (imageOffsetInPixels.X() < imageDimensions.X()) && ((imageOffsetInPixels.X() + imageRegionInPixels.X()) <= imageDimensions.X());
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
bool OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::RegionIsWithinImageDimensions(const V2UInt32& imageDimensions, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels)
{
	return (imageOffsetInPixels.X() < imageDimensions.X()) && ((imageOffsetInPixels.X() + imageRegionInPixels.X()) <= imageDimensions.X()) && (imageOffsetInPixels.Y() < imageDimensions.Y()) && ((imageOffsetInPixels.Y() + imageRegionInPixels.Y()) <= imageDimensions.Y());
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
bool OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::RegionIsWithinImageDimensions(const V3UInt32& imageDimensions, const V3UInt32& imageOffsetInPixels, const V3UInt32& imageRegionInPixels)
{
	return (imageOffsetInPixels.X() < imageDimensions.X()) && ((imageOffsetInPixels.X() + imageRegionInPixels.X()) <= imageDimensions.X()) && (imageOffsetInPixels.Y() < imageDimensions.Y()) && ((imageOffsetInPixels.Y() + imageRegionInPixels.Y()) <= imageDimensions.Y()) && (imageOffsetInPixels.Z() < imageDimensions.Z()) && ((imageOffsetInPixels.Z() + imageRegionInPixels.Z()) <= imageDimensions.Z());
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
std::string OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::ImageDimensionAsString(const V1UInt32& imageDimensions)
{
	return "[" + std::to_string(imageDimensions.X()) + "]";
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
std::string OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::ImageDimensionAsString(const V2UInt32& imageDimensions)
{
	return "[" + std::to_string(imageDimensions.X()) + "," + std::to_string(imageDimensions.Y()) + "]";
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
std::string OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::ImageDimensionAsString(const V3UInt32& imageDimensions)
{
	return "[" + std::to_string(imageDimensions.X()) + "," + std::to_string(imageDimensions.Y()) + "," + std::to_string(imageDimensions.Z()) + "]";
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
constexpr bool OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::GetFormatNative(typename InterfaceType::ImageFormat requestedImageFormat, typename InterfaceType::DataFormat requestedDataFormat, GLint& internalFormat)
{
	switch (requestedDataFormat)
	{
	case InterfaceType::DataFormat::Int8:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::R:
		case InterfaceType::ImageFormat::X:
			internalFormat = GL_R8I;
			return true;
		case InterfaceType::ImageFormat::RG:
		case InterfaceType::ImageFormat::XY:
			internalFormat = GL_RG8I;
			return true;
		case InterfaceType::ImageFormat::RGB:
		case InterfaceType::ImageFormat::XYZ:
		case InterfaceType::ImageFormat::BGR:
			internalFormat = GL_RGB8I;
			return true;
		case InterfaceType::ImageFormat::RGBA:
		case InterfaceType::ImageFormat::XYZW:
		case InterfaceType::ImageFormat::BGRA:
			internalFormat = GL_RGBA8I;
			return true;
		}
		break;
	case InterfaceType::DataFormat::Int16:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::R:
		case InterfaceType::ImageFormat::X:
			internalFormat = GL_R16I;
			return true;
		case InterfaceType::ImageFormat::RG:
		case InterfaceType::ImageFormat::XY:
			internalFormat = GL_RG16I;
			return true;
		case InterfaceType::ImageFormat::RGB:
		case InterfaceType::ImageFormat::XYZ:
		case InterfaceType::ImageFormat::BGR:
			internalFormat = GL_RGB16I;
			return true;
		case InterfaceType::ImageFormat::RGBA:
		case InterfaceType::ImageFormat::XYZW:
		case InterfaceType::ImageFormat::BGRA:
			internalFormat = GL_RGBA16I;
			return true;
		}
		break;
	case InterfaceType::DataFormat::Int32:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::R:
		case InterfaceType::ImageFormat::X:
			internalFormat = GL_R32I;
			return true;
		case InterfaceType::ImageFormat::RG:
		case InterfaceType::ImageFormat::XY:
			internalFormat = GL_RG32I;
			return true;
		case InterfaceType::ImageFormat::RGB:
		case InterfaceType::ImageFormat::XYZ:
		case InterfaceType::ImageFormat::BGR:
			internalFormat = GL_RGB32I;
			return true;
		case InterfaceType::ImageFormat::RGBA:
		case InterfaceType::ImageFormat::XYZW:
		case InterfaceType::ImageFormat::BGRA:
			internalFormat = GL_RGBA32I;
			return true;
		}
		break;
	case InterfaceType::DataFormat::UInt8:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::R:
		case InterfaceType::ImageFormat::X:
			internalFormat = GL_R8UI;
			return true;
		case InterfaceType::ImageFormat::RG:
		case InterfaceType::ImageFormat::XY:
			internalFormat = GL_RG8UI;
			return true;
		case InterfaceType::ImageFormat::RGB:
		case InterfaceType::ImageFormat::XYZ:
		case InterfaceType::ImageFormat::BGR:
			internalFormat = GL_RGB8UI;
			return true;
		case InterfaceType::ImageFormat::RGBA:
		case InterfaceType::ImageFormat::XYZW:
		case InterfaceType::ImageFormat::BGRA:
			internalFormat = GL_RGBA8UI;
			return true;
		}
		break;
	case InterfaceType::DataFormat::UInt16:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::R:
		case InterfaceType::ImageFormat::X:
			internalFormat = GL_R16UI;
			return true;
		case InterfaceType::ImageFormat::RG:
		case InterfaceType::ImageFormat::XY:
			internalFormat = GL_RG16UI;
			return true;
		case InterfaceType::ImageFormat::RGB:
		case InterfaceType::ImageFormat::XYZ:
		case InterfaceType::ImageFormat::BGR:
			internalFormat = GL_RGB16UI;
			return true;
		case InterfaceType::ImageFormat::RGBA:
		case InterfaceType::ImageFormat::XYZW:
		case InterfaceType::ImageFormat::BGRA:
			internalFormat = GL_RGBA16UI;
			return true;
		}
		break;
	case InterfaceType::DataFormat::UInt32:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::R:
		case InterfaceType::ImageFormat::X:
			internalFormat = GL_R32UI;
			return true;
		case InterfaceType::ImageFormat::RG:
		case InterfaceType::ImageFormat::XY:
			internalFormat = GL_RG32UI;
			return true;
		case InterfaceType::ImageFormat::RGB:
		case InterfaceType::ImageFormat::XYZ:
		case InterfaceType::ImageFormat::BGR:
			internalFormat = GL_RGB32UI;
			return true;
		case InterfaceType::ImageFormat::RGBA:
		case InterfaceType::ImageFormat::XYZW:
		case InterfaceType::ImageFormat::BGRA:
			internalFormat = GL_RGBA32UI;
			return true;
		}
		break;
	case InterfaceType::DataFormat::Norm8:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::R:
		case InterfaceType::ImageFormat::X:
			internalFormat = GL_R8_SNORM;
			return true;
		case InterfaceType::ImageFormat::RG:
		case InterfaceType::ImageFormat::XY:
			internalFormat = GL_RG8_SNORM;
			return true;
		case InterfaceType::ImageFormat::RGB:
		case InterfaceType::ImageFormat::XYZ:
		case InterfaceType::ImageFormat::BGR:
			internalFormat = GL_RGB8_SNORM;
			return true;
		case InterfaceType::ImageFormat::RGBA:
		case InterfaceType::ImageFormat::XYZW:
		case InterfaceType::ImageFormat::BGRA:
			internalFormat = GL_RGBA8_SNORM;
			return true;
		}
		break;
	case InterfaceType::DataFormat::Norm16:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::R:
		case InterfaceType::ImageFormat::X:
			internalFormat = GL_R16_SNORM;
			return true;
		case InterfaceType::ImageFormat::RG:
		case InterfaceType::ImageFormat::XY:
			internalFormat = GL_RG16_SNORM;
			return true;
		case InterfaceType::ImageFormat::RGB:
		case InterfaceType::ImageFormat::XYZ:
		case InterfaceType::ImageFormat::BGR:
			internalFormat = GL_RGB16_SNORM;
			return true;
		case InterfaceType::ImageFormat::RGBA:
		case InterfaceType::ImageFormat::XYZW:
		case InterfaceType::ImageFormat::BGRA:
			internalFormat = GL_RGBA16_SNORM;
			return true;
		}
		break;
	case InterfaceType::DataFormat::UNorm8:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::R:
		case InterfaceType::ImageFormat::X:
			internalFormat = GL_R8;
			return true;
		case InterfaceType::ImageFormat::RG:
		case InterfaceType::ImageFormat::XY:
			internalFormat = GL_RG8;
			return true;
		case InterfaceType::ImageFormat::RGB:
		case InterfaceType::ImageFormat::XYZ:
		case InterfaceType::ImageFormat::BGR:
			internalFormat = GL_RGB8;
			return true;
		case InterfaceType::ImageFormat::RGBA:
		case InterfaceType::ImageFormat::XYZW:
		case InterfaceType::ImageFormat::BGRA:
			internalFormat = GL_RGBA8;
			return true;
		}
		break;
	case InterfaceType::DataFormat::UNorm16:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::R:
		case InterfaceType::ImageFormat::X:
			internalFormat = GL_R16;
			return true;
		case InterfaceType::ImageFormat::RG:
		case InterfaceType::ImageFormat::XY:
			internalFormat = GL_RG16;
			return true;
		case InterfaceType::ImageFormat::RGB:
		case InterfaceType::ImageFormat::XYZ:
		case InterfaceType::ImageFormat::BGR:
			internalFormat = GL_RGB16;
			return true;
		case InterfaceType::ImageFormat::RGBA:
		case InterfaceType::ImageFormat::XYZW:
		case InterfaceType::ImageFormat::BGRA:
			internalFormat = GL_RGBA16;
			return true;
		}
		break;
	case InterfaceType::DataFormat::Float16:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::R:
		case InterfaceType::ImageFormat::X:
			internalFormat = GL_R16F;
			return true;
		case InterfaceType::ImageFormat::RG:
		case InterfaceType::ImageFormat::XY:
			internalFormat = GL_RG16F;
			return true;
		case InterfaceType::ImageFormat::RGB:
		case InterfaceType::ImageFormat::XYZ:
		case InterfaceType::ImageFormat::BGR:
			internalFormat = GL_RGB16F;
			return true;
		case InterfaceType::ImageFormat::RGBA:
		case InterfaceType::ImageFormat::XYZW:
		case InterfaceType::ImageFormat::BGRA:
			internalFormat = GL_RGBA16F;
			return true;
		}
		break;
	case InterfaceType::DataFormat::Float32:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::R:
		case InterfaceType::ImageFormat::X:
			internalFormat = GL_R32F;
			return true;
		case InterfaceType::ImageFormat::RG:
		case InterfaceType::ImageFormat::XY:
			internalFormat = GL_RG32F;
			return true;
		case InterfaceType::ImageFormat::RGB:
		case InterfaceType::ImageFormat::XYZ:
		case InterfaceType::ImageFormat::BGR:
			internalFormat = GL_RGB32F;
			return true;
		case InterfaceType::ImageFormat::RGBA:
		case InterfaceType::ImageFormat::XYZW:
		case InterfaceType::ImageFormat::BGRA:
			internalFormat = GL_RGBA32F;
			return true;
		}
		break;
	case InterfaceType::DataFormat::DXT1:
#ifdef GL_EXT_texture_compression_s3tc
		if (GLAD_GL_EXT_texture_compression_s3tc != 0)
		{
			switch (requestedImageFormat)
			{
			case InterfaceType::ImageFormat::RGB:
				internalFormat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
				return true;
			case InterfaceType::ImageFormat::RGBA:
				internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
				return true;
			}
		}
#endif
		break;
	case InterfaceType::DataFormat::DXT3:
#ifdef GL_EXT_texture_compression_s3tc
		if (GLAD_GL_EXT_texture_compression_s3tc != 0)
		{
			switch (requestedImageFormat)
			{
			case InterfaceType::ImageFormat::RGBA:
				internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
				return true;
			}
		}
#endif
		break;
	case InterfaceType::DataFormat::DXT5:
#ifdef GL_EXT_texture_compression_s3tc
		if (GLAD_GL_EXT_texture_compression_s3tc != 0)
		{
			switch (requestedImageFormat)
			{
			case InterfaceType::ImageFormat::RGBA:
				internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
				return true;
			}
		}
#endif
		break;
	case InterfaceType::DataFormat::BPTC:
	{
#if defined(GL_VERSION_4_2) || defined(GL_ARB_texture_compression_bptc)
		bool bptcSupported = false;
#ifdef GL_VERSION_4_2
		bptcSupported = bptcSupported || (GLAD_GL_VERSION_4_2 != 0);
#endif
#ifdef GL_ARB_texture_compression_bptc
		bptcSupported = bptcSupported || (GLAD_GL_ARB_texture_compression_bptc != 0);
#endif
		if (bptcSupported)
		{
			switch (requestedImageFormat)
			{
			case InterfaceType::ImageFormat::RGBA:
				internalFormat = GL_COMPRESSED_RGBA_BPTC_UNORM_ARB;
				return true;
			}
		}
#endif
		break;
	}
	case InterfaceType::DataFormat::ETC2:
#if defined(GL_VERSION_4_3) || defined(GL_ARB_ES3_compatibility)
#ifndef GL_VERSION_4_3
		if (GLAD_GL_ARB_ES3_compatibility != 0)
#endif
		{
			switch (requestedImageFormat)
			{
			case InterfaceType::ImageFormat::RGB:
				internalFormat = GL_COMPRESSED_RGB8_ETC2;
				return true;
			case InterfaceType::ImageFormat::RGBA:
				internalFormat = GL_COMPRESSED_RGBA8_ETC2_EAC;
				return true;
			}
		}
#endif
		break;
	case InterfaceType::DataFormat::ASTC4x4:
	case InterfaceType::DataFormat::ASTC5x5:
	case InterfaceType::DataFormat::ASTC6x6:
	case InterfaceType::DataFormat::ASTC8x8:
	{
#if defined(GL_KHR_texture_compression_astc_ldr) || defined(GL_KHR_texture_compression_astc_hdr)
		bool astcSupported = false;
#ifdef GL_KHR_texture_compression_astc_ldr
		astcSupported = astcSupported || (GLAD_GL_KHR_texture_compression_astc_ldr != 0);
#endif
#ifdef GL_KHR_texture_compression_astc_hdr
		astcSupported = astcSupported || (GLAD_GL_KHR_texture_compression_astc_hdr != 0);
#endif
		if (astcSupported)
		{
			switch (requestedImageFormat)
			{
			case InterfaceType::ImageFormat::RGBA:
				switch (requestedDataFormat)
				{
				case InterfaceType::DataFormat::ASTC4x4:
					internalFormat = GL_COMPRESSED_RGBA_ASTC_4x4_KHR;
					return true;
				case InterfaceType::DataFormat::ASTC5x5:
					internalFormat = GL_COMPRESSED_RGBA_ASTC_5x5_KHR;
					return true;
				case InterfaceType::DataFormat::ASTC6x6:
					internalFormat = GL_COMPRESSED_RGBA_ASTC_6x6_KHR;
					return true;
				case InterfaceType::DataFormat::ASTC8x8:
					internalFormat = GL_COMPRESSED_RGBA_ASTC_8x8_KHR;
					return true;
				}
				break;
			}
		}
#endif
		break;
	}
	case InterfaceType::DataFormat::DepthUNorm16:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::Depth:
			internalFormat = GL_DEPTH_COMPONENT16;
			return true;
		}
		break;
	case InterfaceType::DataFormat::DepthUNorm24:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::Depth:
			internalFormat = GL_DEPTH_COMPONENT24;
			return true;
		}
		break;
	case InterfaceType::DataFormat::DepthUNorm24StencilUInt8:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::DepthAndStencil:
			internalFormat = GL_DEPTH24_STENCIL8;
			return true;
		}
		break;
	case InterfaceType::DataFormat::DepthFloat32:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::Depth:
			internalFormat = GL_DEPTH_COMPONENT32F;
			return true;
		}
		break;
	case InterfaceType::DataFormat::DepthFloat32StencilUInt8:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::DepthAndStencil:
			internalFormat = GL_DEPTH32F_STENCIL8;
			return true;
		}
		break;
	}
	return false;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
constexpr bool OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::IsIntegerDataFormat(typename InterfaceType::DataFormat dataFormat)
{
	switch (dataFormat)
	{
	case InterfaceType::DataFormat::Int8:
	case InterfaceType::DataFormat::Int16:
	case InterfaceType::DataFormat::Int32:
	case InterfaceType::DataFormat::UInt8:
	case InterfaceType::DataFormat::UInt16:
	case InterfaceType::DataFormat::UInt32:
		return true;
	default:
		return false;
	}
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
constexpr GLenum OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::GetImageFormatNative(typename InterfaceType::ImageFormat imageFormat, typename InterfaceType::DataFormat dataFormat, bool stencilComponent)
{
	bool integerDataFormat = IsIntegerDataFormat(dataFormat);
	switch (imageFormat)
	{
	case InterfaceType::ImageFormat::R:
	case InterfaceType::ImageFormat::X:
		return integerDataFormat ? GL_RED_INTEGER : GL_RED;
	case InterfaceType::ImageFormat::RG:
	case InterfaceType::ImageFormat::XY:
		return integerDataFormat ? GL_RG_INTEGER : GL_RG;
	case InterfaceType::ImageFormat::RGB:
	case InterfaceType::ImageFormat::XYZ:
		return integerDataFormat ? GL_RGB_INTEGER : GL_RGB;
	case InterfaceType::ImageFormat::RGBA:
	case InterfaceType::ImageFormat::XYZW:
		return integerDataFormat ? GL_RGBA_INTEGER : GL_RGBA;
	case InterfaceType::ImageFormat::BGR:
		return integerDataFormat ? GL_BGR_INTEGER : GL_BGR;
	case InterfaceType::ImageFormat::BGRA:
		return integerDataFormat ? GL_BGRA_INTEGER : GL_BGRA;
	case InterfaceType::ImageFormat::Depth:
	case InterfaceType::ImageFormat::DepthAndStencil:
		if (stencilComponent)
		{
			return GL_STENCIL_INDEX;
		}
		return GL_DEPTH_COMPONENT;
	}
	UNREACHABLE();
	return {};
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
constexpr GLenum OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::GetDataFormatNative(typename InterfaceType::DataFormat dataFormat, bool stencilComponent)
{
	switch (dataFormat)
	{
	case InterfaceType::DataFormat::Int8:
		return GL_BYTE;
	case InterfaceType::DataFormat::Int16:
		return GL_SHORT;
	case InterfaceType::DataFormat::Int32:
		return GL_INT;
	case InterfaceType::DataFormat::UInt8:
	case InterfaceType::DataFormat::UNorm8:
		return GL_UNSIGNED_BYTE;
	case InterfaceType::DataFormat::UInt16:
	case InterfaceType::DataFormat::UNorm16:
		return GL_UNSIGNED_SHORT;
	case InterfaceType::DataFormat::UInt32:
		return GL_UNSIGNED_INT;
	case InterfaceType::DataFormat::Float16:
		return GL_HALF_FLOAT;
	case InterfaceType::DataFormat::Float32:
		return GL_FLOAT;
	case InterfaceType::DataFormat::Norm8:
	case InterfaceType::DataFormat::Norm16:
		// OpenGL doesn't support signed normalized integer formats as an input, so we don't provide a value for
		// these types here. This case won't be encountered, as we strip out these formats and force conversion to
		// another type.
	case InterfaceType::DataFormat::DXT1:
	case InterfaceType::DataFormat::DXT3:
	case InterfaceType::DataFormat::DXT5:
	case InterfaceType::DataFormat::BPTC:
	case InterfaceType::DataFormat::ETC2:
	case InterfaceType::DataFormat::ASTC4x4:
	case InterfaceType::DataFormat::ASTC5x5:
	case InterfaceType::DataFormat::ASTC6x6:
	case InterfaceType::DataFormat::ASTC8x8:
		// This value is meaningless for compressed texture formats, as these formats don't require an element data
		// format to be specified anywhere.
		return GL_NONE;
	case InterfaceType::DataFormat::DepthUNorm16:
	case InterfaceType::DataFormat::DepthUNorm24:
	case InterfaceType::DataFormat::DepthUNorm24StencilUInt8:
	case InterfaceType::DataFormat::DepthFloat32:
	case InterfaceType::DataFormat::DepthFloat32StencilUInt8:
		// In the unusual case someone wants to write to a depth buffer to set initial state, rather than just bind
		// it to a framebuffer and clear it, we only want values passed in as floats.
		if (stencilComponent)
		{
			return GL_UNSIGNED_BYTE;
		}
		return GL_FLOAT;
	}
	UNREACHABLE();
	return {};
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
constexpr void OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::GetImageAllocationFormatNative(typename InterfaceType::ImageFormat imageFormat, typename InterfaceType::DataFormat dataFormat, GLenum& nativeImageFormat, GLenum& nativeDataFormat)
{
	// glTexImage* validates the source format and type even when no source data is supplied. Match the source format
	// family to the texture storage format so mutable-storage allocation works for integer and depth/stencil textures.
	if (imageFormat == InterfaceType::ImageFormat::DepthAndStencil)
	{
		nativeImageFormat = GL_DEPTH_STENCIL;
		switch (dataFormat)
		{
		case InterfaceType::DataFormat::DepthUNorm24StencilUInt8:
			nativeDataFormat = GL_UNSIGNED_INT_24_8;
			return;
		case InterfaceType::DataFormat::DepthFloat32StencilUInt8:
			nativeDataFormat = GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
			return;
		default:
			UNREACHABLE();
			nativeDataFormat = GL_UNSIGNED_INT_24_8;
			return;
		}
	}
	nativeImageFormat = GetImageFormatNative(imageFormat, dataFormat, false);
	nativeDataFormat = GetDataFormatNative(dataFormat, false);
	if (nativeDataFormat == GL_NONE)
	{
		// Normal upload paths convert signed normalized data through Float32, but mutable empty allocation still needs
		// a legal source type even though no source data is supplied.
		if (dataFormat == InterfaceType::DataFormat::Norm8)
		{
			nativeDataFormat = GL_BYTE;
		}
		else if (dataFormat == InterfaceType::DataFormat::Norm16)
		{
			nativeDataFormat = GL_SHORT;
		}
		else
		{
			nativeDataFormat = GL_UNSIGNED_BYTE;
		}
	}
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
void OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::AdjustSourceDataTargetFormat(typename InterfaceType::SourceImageFormat sourceImageFormat, typename InterfaceType::SourceDataFormat sourceDataFormat, typename InterfaceType::ImageFormat targetImageFormat, typename InterfaceType::DataFormat targetDataFormat, typename InterfaceType::ImageFormat& newTargetImageFormat, typename InterfaceType::DataFormat& newTargetDataFormat)
{
	// Initialize the output format settings
	newTargetImageFormat = targetImageFormat;
	newTargetDataFormat = targetDataFormat;

	// Perform any format adjustments that may be required
	switch (sourceDataFormat)
	{
	case InterfaceType::SourceDataFormat::Norm8:
	case InterfaceType::SourceDataFormat::Norm16:
		// OpenGL doesn't support signed normalized integer formats as an input data type, so we switch to Float32
		// instead, as it can represent the original values without loss.
		newTargetDataFormat = InterfaceType::DataFormat::Float32;
		break;
	}
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
constexpr void OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::GetOptimalSourceFormat(typename InterfaceType::ImageFormat imageFormat, typename InterfaceType::DataFormat dataFormat, typename InterfaceType::ImageFormat& selectedImageFormat, typename InterfaceType::DataFormat& selectedDataFormat, typename InterfaceType::SourceImageFormat& sourceImageFormat, typename InterfaceType::SourceDataFormat& sourceDataFormat, GLenum& nativeDataFormat, GLenum& nativeDataType, bool stencilComponent)
{
	// Default the selected image settings to the actual storage format. OpenGL doesn't allow reading in all native
	// storage formats directly, so we may have to change this below.
	selectedImageFormat = imageFormat;
	selectedDataFormat = dataFormat;

	// Set the initial native source format settings based on the image format
	nativeDataFormat = GetImageFormatNative(imageFormat, dataFormat, stencilComponent);
	nativeDataType = GetDataFormatNative(dataFormat, stencilComponent);

	// Determine the source image format to use
	switch (imageFormat)
	{
	case InterfaceType::ImageFormat::R:
		sourceImageFormat = InterfaceType::SourceImageFormat::R;
		break;
	case InterfaceType::ImageFormat::RG:
		sourceImageFormat = InterfaceType::SourceImageFormat::RG;
		break;
	case InterfaceType::ImageFormat::RGB:
		sourceImageFormat = InterfaceType::SourceImageFormat::RGB;
		break;
	case InterfaceType::ImageFormat::RGBA:
		sourceImageFormat = InterfaceType::SourceImageFormat::RGBA;
		break;
	case InterfaceType::ImageFormat::BGR:
		sourceImageFormat = InterfaceType::SourceImageFormat::BGR;
		break;
	case InterfaceType::ImageFormat::BGRA:
		sourceImageFormat = InterfaceType::SourceImageFormat::BGRA;
		break;
	case InterfaceType::ImageFormat::X:
	case InterfaceType::ImageFormat::Depth:
	case InterfaceType::ImageFormat::DepthAndStencil:
		sourceImageFormat = InterfaceType::SourceImageFormat::X;
		break;
	case InterfaceType::ImageFormat::XY:
		sourceImageFormat = InterfaceType::SourceImageFormat::XY;
		break;
	case InterfaceType::ImageFormat::XYZ:
		sourceImageFormat = InterfaceType::SourceImageFormat::XYZ;
		break;
	case InterfaceType::ImageFormat::XYZW:
		sourceImageFormat = InterfaceType::SourceImageFormat::XYZW;
		break;
	default:
		UNREACHABLE();
		break;
	}

	// Determine the source data format to use
	switch (dataFormat)
	{
	case InterfaceType::DataFormat::Int8:
		sourceDataFormat = InterfaceType::SourceDataFormat::Int8;
		break;
	case InterfaceType::DataFormat::Int16:
		sourceDataFormat = InterfaceType::SourceDataFormat::Int16;
		break;
	case InterfaceType::DataFormat::Int32:
		sourceDataFormat = InterfaceType::SourceDataFormat::Int32;
		break;
	case InterfaceType::DataFormat::UInt8:
		sourceDataFormat = InterfaceType::SourceDataFormat::UInt8;
		break;
	case InterfaceType::DataFormat::UInt16:
		sourceDataFormat = InterfaceType::SourceDataFormat::UInt16;
		break;
	case InterfaceType::DataFormat::UInt32:
		sourceDataFormat = InterfaceType::SourceDataFormat::UInt32;
		break;
	case InterfaceType::DataFormat::Norm8:
	case InterfaceType::DataFormat::Norm16:
		// OpenGL doesn't support signed normalized integer formats as an input data type, so we switch to Float32
		// instead, as it can represent the original values without loss.
		selectedDataFormat = InterfaceType::DataFormat::Float32;
		nativeDataType = GetDataFormatNative(selectedDataFormat, stencilComponent);
		sourceDataFormat = InterfaceType::SourceDataFormat::Float32;
		break;
	case InterfaceType::DataFormat::UNorm8:
		sourceDataFormat = InterfaceType::SourceDataFormat::UNorm8;
		break;
	case InterfaceType::DataFormat::UNorm16:
		sourceDataFormat = InterfaceType::SourceDataFormat::UNorm16;
		break;
	case InterfaceType::DataFormat::Float16:
		sourceDataFormat = InterfaceType::SourceDataFormat::Float16;
		break;
	case InterfaceType::DataFormat::Float32:
		sourceDataFormat = InterfaceType::SourceDataFormat::Float32;
		break;
	case InterfaceType::DataFormat::DXT1:
		sourceDataFormat = InterfaceType::SourceDataFormat::DXT1;
		break;
	case InterfaceType::DataFormat::DXT3:
		sourceDataFormat = InterfaceType::SourceDataFormat::DXT3;
		break;
	case InterfaceType::DataFormat::DXT5:
		sourceDataFormat = InterfaceType::SourceDataFormat::DXT5;
		break;
	case InterfaceType::DataFormat::BPTC:
		sourceDataFormat = InterfaceType::SourceDataFormat::BPTC;
		break;
	case InterfaceType::DataFormat::ETC2:
		sourceDataFormat = InterfaceType::SourceDataFormat::ETC2;
		break;
	case InterfaceType::DataFormat::ASTC4x4:
		sourceDataFormat = InterfaceType::SourceDataFormat::ASTC4x4;
		break;
	case InterfaceType::DataFormat::ASTC5x5:
		sourceDataFormat = InterfaceType::SourceDataFormat::ASTC5x5;
		break;
	case InterfaceType::DataFormat::ASTC6x6:
		sourceDataFormat = InterfaceType::SourceDataFormat::ASTC6x6;
		break;
	case InterfaceType::DataFormat::ASTC8x8:
		sourceDataFormat = InterfaceType::SourceDataFormat::ASTC8x8;
		break;
	case InterfaceType::DataFormat::DepthUNorm24StencilUInt8:
	case InterfaceType::DataFormat::DepthFloat32StencilUInt8:
		if (!stencilComponent)
		{
			selectedDataFormat = InterfaceType::DataFormat::DepthFloat32;
			sourceDataFormat = InterfaceType::SourceDataFormat::Float32;
		}
		else
		{
			selectedDataFormat = InterfaceType::DataFormat::UInt8;
			sourceDataFormat = InterfaceType::SourceDataFormat::UInt8;
		}
		break;
	case InterfaceType::DataFormat::DepthUNorm16:
	case InterfaceType::DataFormat::DepthUNorm24:
	case InterfaceType::DataFormat::DepthFloat32:
		selectedDataFormat = InterfaceType::DataFormat::DepthFloat32;
		sourceDataFormat = InterfaceType::SourceDataFormat::Float32;
		break;
	default:
		UNREACHABLE();
		break;
	}
}

//----------------------------------------------------------------------------------------
// Initial data methods
//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
SuccessToken OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::SetInitialData(const void* sourceBuffer, size_t sourceBufferSizeInBytes, typename InterfaceType::SourceImageFormat imageFormat, typename InterfaceType::SourceDataFormat dataFormat, typename InterfaceType::CubeMapFace targetFace, int mipmapLevel)
{
	auto arrayIndexFinal = (size_t)targetFace;
	return SetInitialData(sourceBuffer, sourceBufferSizeInBytes, imageFormat, dataFormat, arrayIndexFinal, mipmapLevel);
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
SuccessToken OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::SetInitialData(const void* sourceBuffer, size_t sourceBufferSizeInBytes, typename InterfaceType::SourceImageFormat imageFormat, typename InterfaceType::SourceDataFormat dataFormat, typename InterfaceType::CubeMapFace targetFace, size_t arrayIndex, int mipmapLevel)
{
	auto arrayIndexFinal = (arrayIndex * 6) + (size_t)targetFace;
	return SetInitialData(sourceBuffer, sourceBufferSizeInBytes, imageFormat, dataFormat, arrayIndexFinal, mipmapLevel);
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
SuccessToken OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::SetInitialData(const void* sourceBuffer, size_t sourceBufferSizeInBytes, typename InterfaceType::SourceImageFormat imageFormat, typename InterfaceType::SourceDataFormat dataFormat, int mipmapLevel)
{
	return SetInitialData(sourceBuffer, sourceBufferSizeInBytes, imageFormat, dataFormat, 0, mipmapLevel);
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
SuccessToken OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::SetInitialData(const void* sourceBuffer, size_t sourceBufferSizeInBytes, typename InterfaceType::SourceImageFormat imageFormat, typename InterfaceType::SourceDataFormat dataFormat, size_t arrayIndex, int mipmapLevel)
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

	// Capture the supplied initial data, performing format conversion if required.
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
SuccessToken OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::QueueDataUpdate(const void* sourceBuffer, size_t sourceBufferSizeInBytes, typename InterfaceType::SourceImageFormat imageFormat, typename InterfaceType::SourceDataFormat dataFormat, typename InterfaceType::CubeMapFace targetFace, int mipmapLevel, const DimensionVectorType& imageOffsetInPixels, const DimensionVectorType& imageRegionInPixels, ITransferBatch* transferBatch)
{
	auto arrayIndexFinal = (size_t)targetFace;
	return QueueDataUpdate(sourceBuffer, sourceBufferSizeInBytes, imageFormat, dataFormat, arrayIndexFinal, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
SuccessToken OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::QueueDataUpdate(const void* sourceBuffer, size_t sourceBufferSizeInBytes, typename InterfaceType::SourceImageFormat imageFormat, typename InterfaceType::SourceDataFormat dataFormat, typename InterfaceType::CubeMapFace targetFace, size_t arrayIndex, int mipmapLevel, const DimensionVectorType& imageOffsetInPixels, const DimensionVectorType& imageRegionInPixels, ITransferBatch* transferBatch)
{
	auto arrayIndexFinal = (arrayIndex * 6) + (size_t)targetFace;
	return QueueDataUpdate(sourceBuffer, sourceBufferSizeInBytes, imageFormat, dataFormat, arrayIndexFinal, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
SuccessToken OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::QueueDataUpdate(const void* sourceBuffer, size_t sourceBufferSizeInBytes, typename InterfaceType::SourceImageFormat imageFormat, typename InterfaceType::SourceDataFormat dataFormat, int mipmapLevel, const DimensionVectorType& imageOffsetInPixels, const DimensionVectorType& imageRegionInPixels, ITransferBatch* transferBatch)
{
	return QueueDataUpdate(sourceBuffer, sourceBufferSizeInBytes, imageFormat, dataFormat, 0, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
SuccessToken OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::QueueDataUpdate(const void* sourceBuffer, size_t sourceBufferSizeInBytes, typename InterfaceType::SourceImageFormat imageFormat, typename InterfaceType::SourceDataFormat dataFormat, size_t arrayIndex, int mipmapLevel, const DimensionVectorType& imageOffsetInPixels, const DimensionVectorType& imageRegionInPixels, ITransferBatch* transferBatch)
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
	auto* transferBatchResolved = KnownDynamicCast<OpenGLTransferBatch*>(transferBatch);
	if ((transferBatchResolved != nullptr) && transferBatchResolved->IsSubmitted())
	{
		_log->Error("Attempted to queue a transfer using a transfer batch that has already been submitted");
		return false;
	}

	// Adjust our target image format to allow for the fact that we can't directly upload data in some native formats
	typename InterfaceType::ImageFormat selectedImageFormat;
	typename InterfaceType::DataFormat selectedDataFormat;
	AdjustSourceDataTargetFormat(imageFormat, dataFormat, _imageFormat, _dataFormat, selectedImageFormat, selectedDataFormat);

	// Capture the supplied update settings and data, performing format conversion if required.
	PendingWrite pendingWrite(arrayIndex, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatchResolved);
	if (!ConvertDataFormat(sourceBuffer, sourceBufferSizeInBytes, imageFormat, dataFormat, selectedImageFormat, selectedDataFormat, pendingWrite.data, pendingWrite.convertedStencilData))
	{
		_log->Error("QueueDataUpdate failed to convert source data");
		return false;
	}

	// If a transfer batch has been supplied, increment the usage count.
	if (transferBatchResolved != nullptr)
	{
		transferBatchResolved->IncrementUsageCount();
	}

	// Populate the native format flags
	pendingWrite.nativeImageFormat = (_isCompressedTexture ? (GLenum)_internalFormat : GetImageFormatNative(selectedImageFormat, selectedDataFormat, false));
	pendingWrite.nativeDataFormat = GetDataFormatNative(selectedDataFormat, false);
	if (_imageFormat == InterfaceType::ImageFormat::DepthAndStencil)
	{
		pendingWrite.stencilNativeImageFormat = GetImageFormatNative(selectedImageFormat, selectedDataFormat, true);
		pendingWrite.stencilNativeDataFormat = GetDataFormatNative(selectedDataFormat, true);
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
bool OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::ConvertDataFormat(const void* sourceBuffer, size_t sourceBufferSizeInBytes, typename InterfaceType::SourceImageFormat sourceImageFormat, typename InterfaceType::SourceDataFormat sourceDataFormat, typename InterfaceType::ImageFormat targetImageFormat, typename InterfaceType::DataFormat targetDataFormat, std::vector<uint8_t>& targetBuffer, std::vector<uint8_t>& stencilTargetBuffer) const
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
	bool result;
	if (targetImageFormat == InterfaceType::ImageFormat::DepthAndStencil)
	{
		result = TextureFormatConversion::ConvertTextureInputData(sourceBuffer, sourceBufferSizeInBytes, sourceImageFormat, sourceDataFormat, targetImageFormat, InterfaceType::DataFormat::DepthFloat32, targetBuffer, sourceDataFormatError, targetDataFormatError);
		result &= TextureFormatConversion::ConvertTextureInputData(sourceBuffer, sourceBufferSizeInBytes, sourceImageFormat, sourceDataFormat, targetImageFormat, InterfaceType::DataFormat::UInt8, stencilTargetBuffer, sourceDataFormatError, targetDataFormatError);
	}
	else
	{
		result = TextureFormatConversion::ConvertTextureInputData(sourceBuffer, sourceBufferSizeInBytes, sourceImageFormat, sourceDataFormat, targetImageFormat, targetDataFormat, targetBuffer, sourceDataFormatError, targetDataFormatError);
	}

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
void OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::MigrateBuildStateToDrawState()
{
	std::swap(_buildIndex, _drawIndex);
	_stateModified.clear(std::memory_order_relaxed);
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
GLuint OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::GetTextureNo() const
{
	return _textureNo;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
void OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::CreateNativeBuffer()
{
	// If this texture has been defined as a multisample texture, override the texture target.
	if (_sampleCount != InterfaceType::SampleCount::SampleCount1)
	{
		_textureTarget = GL_TEXTURE_2D_MULTISAMPLE;
	}

	// Generate the texture number
	glGenTextures(1, &_textureNo);
	CheckGLError(Log());

	// Bind the texture target
	glBindTexture(_textureTarget, _textureNo);
	CheckGLError(Log());

	// Set the default texture filter modes to GL_NEAREST. We need to do this here, as GL_TEXTURE_MIN_FILTER defaults to
	// GL_NEAREST_MIPMAP_LINEAR, and since we don't have mipmapping defined by default, OpenGL considers this texture to
	// be incomplete at this point. These filter modes are overridden by our texture samplers when binding the texture
	// to a shader resource, but if this texture is used as a framebuffer target instead, no texture sampler is bound,
	// and we therefore have to ensure we have the default values here set correctly, otherwise the texture won't
	// function correctly as a framebuffer output. Note that we don't need to assign these values in the case of a
	// multisample target, and in fact OpenGL flags an error if we do, as described in the "Errors" note under section
	// 8.11 of the OpenGL 4.3 Core specification. Multisample targets don't support filtering at all, and are therefore
	// considered complete without needing to adjust filtering parameters.
	if (_sampleCount == InterfaceType::SampleCount::SampleCount1)
	{
		glTexParameteri(_textureTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(_textureTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		CheckGLError(Log());
	}

	// Set the unpack alignment to 1 byte, so that rows of pixel data are considered to be tightly packed. We don't
	// want any packing to be performed. We've opted to leave the default setting of 4 globally, but this could
	// potentially be done once globally instead.
	GLint originalUnpackAlignment;
	glGetIntegerv(GL_UNPACK_ALIGNMENT, &originalUnpackAlignment);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// Attempt to create the texture object
	if (!CreateTextureObject(_imageFormat, _dataFormat, _internalFormat, _initialData))
	{
		_log->Error("Failed to create texture object.");
		glDeleteTextures(1, &_textureNo);
		CheckGLError(Log());
		_textureNo = 0;
	}

	// Restore the original unpack alignment
	glPixelStorei(GL_UNPACK_ALIGNMENT, originalUnpackAlignment);

	// Release any memory currently held by the initial data array
	_initialData.clear();
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
void OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::CompletePendingDataWrites()
{
	// If our native buffer is pending creation, create it now.
	if (_nativeBufferCreationPending)
	{
		CreateNativeBuffer();
		_nativeBufferCreationPending = false;
	}

	// Complete any pending data writes for this texture
	std::vector<PendingWrite>& pendingWrites = _state[_drawIndex].pendingWrites;
	if (!pendingWrites.empty())
	{
		// Split pending writes into those that are ready to run now, and those that must remain queued until their
		// batch has been submitted.
		std::vector<PendingWrite> readyWrites;
		std::vector<PendingWrite> deferredWrites;
		readyWrites.reserve(pendingWrites.size());
		deferredWrites.reserve(pendingWrites.size());
		for (PendingWrite& write : pendingWrites)
		{
			if ((write.transferBatch != nullptr) && !write.transferBatch->IsSubmitted())
			{
				deferredWrites.push_back(std::move(write));
				continue;
			}
			readyWrites.push_back(std::move(write));
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
		if (readyWrites.empty())
		{
			return;
		}

		// Bind the target texture
		glBindTexture(_textureTarget, _textureNo);

		// Set the unpack alignment to 1 byte, so that rows of pixel data are considered to be tightly packed. We don't
		// want any packing to be performed. We've opted to leave the default setting of 4 globally, but this could
		// potentially be done once globally instead.
		GLint originalUnpackAlignment;
		glGetIntegerv(GL_UNPACK_ALIGNMENT, &originalUnpackAlignment);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		// Perform all pending writes
		for (const PendingWrite& write : readyWrites)
		{
			// Perform the pending write
			if (!CompletePendingDataWrite(write))
			{
				_log->Error("CompletePendingDataWrite failed for texture object.");
			}

			// If a transfer batch has been supplied, decrement the usage count.
			if (write.transferBatch != nullptr)
			{
				write.transferBatch->DecrementUsageCount();
			}
		}

		// Restore the original unpack alignment
		glPixelStorei(GL_UNPACK_ALIGNMENT, originalUnpackAlignment);
	}
	CheckGLError(Log());
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
void OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::FlagBuildStateModified()
{
	if (!_stateModified.test_and_set(std::memory_order_acquire))
	{
		FlagObjectModified();
	}
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
OpenGLRenderer* OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::Renderer() const
{
	return _renderer;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
cobalt::logging::ILogger* OpenGLTextureBuffer<InterfaceType, DimensionVectorType>::Log() const
{
	return _log;
}

} // namespace cobalt::graphics
