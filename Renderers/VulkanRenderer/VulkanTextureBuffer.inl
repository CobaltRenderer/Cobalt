// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "VulkanRenderer.h"
#include "VulkanTransferBatch.h"
#include <Internal/TextureFormatConversion/TextureFormatConversion.pkg>
#include <Internal/RendererSupport/KnownDynamicCast.h>
#include <cmath>
#include <cstring>
#include <utility>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
VulkanTextureBuffer<InterfaceType, DimensionVectorType>::VulkanTextureBuffer(cobalt::logging::ILogger* log, VulkanRenderer* renderer)
: _log(log), _renderer(renderer), _usageFlags(InterfaceType::UsageFlags::Default), _performanceHintCpu(InterfaceType::PerformanceHint::Default), _performanceHintGpu(InterfaceType::PerformanceHint::Default), _dataPersistenceFlags(InterfaceType::DataPersistenceFlags::PersistAlways)
{}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
VulkanTextureBuffer<InterfaceType, DimensionVectorType>::~VulkanTextureBuffer()
{
	ReleaseMemory();
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
SuccessToken VulkanTextureBuffer<InterfaceType, DimensionVectorType>::AllocateMemory()
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
	VkImageUsageFlags nativeUsageFlags;
	VkImageCreateFlags nativeCreateFlags;
	CalculateBufferSettings(_usageFlags, _imageFormat, _dataFormat, _performanceHintCpu, _performanceHintGpu, nativeUsageFlags, nativeCreateFlags);

	// Populate the VkImageCreateInfo structure
	VkImageCreateInfo imageInfo;
	PopulateImageCreateInfo(imageInfo, _internalFormat, nativeUsageFlags, nativeCreateFlags);

	// Verify that the requested image sample count is supported for this format before attempting to create the image.
	// Vulkan Specification, Chapter 12 "Resources", VUID-VkImageCreateInfo-samples-02258:
	// https://docs.vulkan.org/spec/latest/chapters/resources.html#VUID-VkImageCreateInfo-samples-02258
	// The requested VkImageCreateInfo::samples value must be present in the format's supported image sample counts.
	VkImageFormatProperties imageFormatProperties = {};
	VkResult imageFormatPropertiesResult = vkGetPhysicalDeviceImageFormatProperties(_renderer->GetPhysicalDevice(), imageInfo.format, imageInfo.imageType, imageInfo.tiling, imageInfo.usage, imageInfo.flags, &imageFormatProperties);
	if (imageFormatPropertiesResult == VK_ERROR_FORMAT_NOT_SUPPORTED)
	{
		_log->Error("Failed to allocate memory for texture. The requested image configuration is not supported for native format {0}.", imageInfo.format);
		return false;
	}
	if (imageFormatPropertiesResult != VK_SUCCESS)
	{
		_log->Error("Failed to query image format properties for native format {0}. vkGetPhysicalDeviceImageFormatProperties returned {1}.", imageInfo.format, imageFormatPropertiesResult);
		return false;
	}
	if ((imageFormatProperties.sampleCounts & imageInfo.samples) == 0)
	{
		_log->Error("Failed to allocate memory for texture. Requested sample count {0} is not supported for native format {1}. Supported counts mask is {2}.", imageInfo.samples, imageInfo.format, imageFormatProperties.sampleCounts);
		return false;
	}

	// Attempt to create the texture object
	VulkanMemoryManager* memoryManager = _renderer->GetMemoryManager();
	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	VkResult vmaCreateImageResult = vmaCreateImage(memoryManager->Allocator(), &imageInfo, &allocInfo, &_image, &_imageAllocation, nullptr);
	if (vmaCreateImageResult != VK_SUCCESS)
	{
		_log->Error("vmaCreateImage failed with error code {0}", vmaCreateImageResult);
		return false;
	}
	VkImageLayout initialImageLayout = imageInfo.initialLayout;
	_imageBufferCreated = true;

	// Determine the aspect flags to use when performing writes to the texture buffer, and when constructing the image
	// view for texture binding in shaders.
	if (_imageFormat == InterfaceType::ImageFormat::DepthAndStencil)
	{
		_aspectFlags = VkImageAspectFlagBits(VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
	}
	else if (_imageFormat == InterfaceType::ImageFormat::Depth)
	{
		_aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
	}
	else
	{
		_aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	// Populate the VkImageViewCreateInfo structure
	VkImageViewCreateInfo viewInfo;
	PopulateImageViewCreateInfo(viewInfo, _image, _aspectFlags, _internalFormat);

	// Attempt to create an image view for the texture object
	VkResult vkCreateImageViewResult = vkCreateImageView(_renderer->GetDevice(), &viewInfo, nullptr, &_imageView);
	if (vkCreateImageViewResult != VK_SUCCESS)
	{
		_log->Error("vkCreateImageView failed with error code {0}", vkCreateImageViewResult);
		return false;
	}
	_imageViewCreated = true;

	// Determine the default image layout to use
	_defaultImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	if ((_usageFlags != InterfaceType::UsageFlags::Default) && ((_usageFlags & InterfaceType::UsageFlags::ShaderInput) == InterfaceType::UsageFlags::Default))
	{
		if ((_imageFormat == InterfaceType::ImageFormat::Depth) || (_imageFormat == InterfaceType::ImageFormat::DepthAndStencil))
		{
			_defaultImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		}
		else
		{
			_defaultImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		}
	}

	// If initial data has been provided for this texture, prepare an upload task for it now. We also handle setting the
	// initial layout state for the texture here.
	if (!_initialData.empty())
	{
		// Allocate a new command buffer
		VkCommandBuffer commandBuffer = _renderer->GetBuildCommandBuffer();

		// Prepare a command buffer for the transfer commands, and transition the texture into the correct state to
		// receive an upload.
		memoryManager->RecordTransitionImageLayout(commandBuffer, _image, _internalFormat, _mipmapLevels, (uint32_t)_arraySize, initialImageLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		initialImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

		// Copy each supplied initial data entry into staging buffers, and queue transfers from those staging buffers to
		// the texture.
		for (const InitialDataEntry& entry : _initialData)
		{
			// Create a staging buffer
			VkBuffer stagingBuffer;
			VmaAllocation stagingBufferAllocation;
			_renderer->CreateTemporaryUploadBuffer(entry.dataSizeInBytes, stagingBuffer, stagingBufferAllocation, false);

			// Copy the supplied data to the staging buffer
			uint8_t* stagingBufferMappedData;
			memoryManager->MapBufferMemory(stagingBufferAllocation, stagingBufferMappedData);
			std::memcpy(stagingBufferMappedData, entry.data, entry.dataSizeInBytes);
			memoryManager->UnmapBufferMemory(stagingBufferAllocation);

			// Copy the staging buffer to the texture buffer
			VkBufferImageCopy copyRegion;
			PopulateImageCopyRegion(entry, _aspectFlags, copyRegion);
			vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, _image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
		}

		// Release any memory currently held by the initial data array
		_initialData.clear();

		// If the current graphics device has separate graphics and transfer queues, which is the preferred model, we
		// need to do a queue family release operation here. If the same queue is being used for both graphics and
		// transfers however, there's nothing to do. The buffer will remain in VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL and
		// an image layout transition will be performed if required later.
		if (!_renderer->IsTransferQueueSharedWithGraphics())
		{
			// Perform a queue family release operation to transition this resource from the transfer queue to the
			// graphics queue. Note that we deliberately don't update the "_initialImageLayout" member here. As defined
			// in the Vulkan 1.1 specification under "6.7.4. Queue Family Ownership Transfer", if you want to perform an
			// image layout transition as part of the queue family release operation, "the values of oldLayout and
			// newLayout in the release operation's memory barrier must be equal to values of oldLayout and newLayout in
			// the acquire operation's memory barrier. Although the image layout transition is submitted twice, it will
			// only be executed once". This matches our use case here, so we deliberately leave the _initialImageLayout
			// variable with a stale value, so the image layout transition we request in the release operation below,
			// will be repeated in the acquire operation which is executed next.
			VkImageMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.pNext = nullptr;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = 0;
			barrier.oldLayout = initialImageLayout;
			barrier.newLayout = _defaultImageLayout;
			barrier.srcQueueFamilyIndex = _renderer->GetTransferQueueFamily();
			barrier.dstQueueFamilyIndex = _renderer->GetGraphicsQueueFamily();
			barrier.image = _image;
			barrier.subresourceRange.aspectMask = _aspectFlags;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = _mipmapLevels;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = (uint32_t)_arraySize;
			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

			// Schedule an acquire operation on the graphics queue for the buffer
			ScheduleGraphicsQueueAcquireOperation();
		}

		// Submit the command buffer
		_renderer->SubmitBuildCommandBuffer(commandBuffer);
	}

	// Record the initial image layout state
	_currentImageLayout = initialImageLayout;

	// Flag that the build state has been modified, and return true.
	FlagBuildStateModified();
	_isMemoryAllocated = true;
	return true;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
void VulkanTextureBuffer<InterfaceType, DimensionVectorType>::ReleaseMemory()
{
	// Destroy the image view
	if (_imageViewCreated)
	{
		vkDestroyImageView(_renderer->GetDevice(), _imageView, nullptr);
		_imageView = VK_NULL_HANDLE;
		_imageViewCreated = false;
	}

	// Destroy the image buffer
	if (_imageBufferCreated)
	{
		vmaDestroyImage(_renderer->GetMemoryManager()->Allocator(), _image, _imageAllocation);
		_image = VK_NULL_HANDLE;
		_imageBufferCreated = false;
	}
}

//----------------------------------------------------------------------------------------
// Usage methods
//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
typename InterfaceType::UsageFlags VulkanTextureBuffer<InterfaceType, DimensionVectorType>::GetUsageFlags() const
{
	return _usageFlags;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
typename InterfaceType::PerformanceHint VulkanTextureBuffer<InterfaceType, DimensionVectorType>::GetPerformanceHintCpu() const
{
	return _performanceHintCpu;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
typename InterfaceType::PerformanceHint VulkanTextureBuffer<InterfaceType, DimensionVectorType>::GetPerformanceHintGpu() const
{
	return _performanceHintGpu;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
typename InterfaceType::DataPersistenceFlags VulkanTextureBuffer<InterfaceType, DimensionVectorType>::GetDataPersistenceFlags() const
{
	return _dataPersistenceFlags;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
void VulkanTextureBuffer<InterfaceType, DimensionVectorType>::SetUsageFlags(typename InterfaceType::UsageFlags usageFlags)
{
	_usageFlags = usageFlags;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
void VulkanTextureBuffer<InterfaceType, DimensionVectorType>::SetPerformanceHints(typename InterfaceType::PerformanceHint performanceHintCpu, typename InterfaceType::PerformanceHint performanceHintGpu)
{
	_performanceHintCpu = performanceHintCpu;
	_performanceHintGpu = performanceHintGpu;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
void VulkanTextureBuffer<InterfaceType, DimensionVectorType>::SetDataPersistenceFlags(typename InterfaceType::DataPersistenceFlags dataPersistenceFlags)
{
	_dataPersistenceFlags = dataPersistenceFlags;
}

//----------------------------------------------------------------------------------------
// Format methods
//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
void VulkanTextureBuffer<InterfaceType, DimensionVectorType>::SetTextureFormat(typename InterfaceType::ImageFormat imageFormat, typename InterfaceType::DataFormat dataFormat)
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
void VulkanTextureBuffer<InterfaceType, DimensionVectorType>::GetTextureFormatInfo(ITextureBuffer::ImageFormat& imageFormat, ITextureBuffer::DataFormat& dataFormat, size_t& elementCount, size_t& elementSizeInBytes, size_t& pixelOffsetInBytes, size_t& pixelStrideInBytes, bool stencilComponent) const
{
	imageFormat = _imageFormat;
	dataFormat = _dataFormat;
	elementCount = _elementCount;
	// See Chapter 39 "Copy Commands" in the Vulkan spec for info on format rules when accessing depth/stencil
	// components from a combined format:
	// https://docs.vulkan.org/spec/latest/chapters/copies.html
	// Copies between a buffer and the depth or stencil aspect of an image use separate aspect formats rather than the
	// base interleaved image format, so we apply overrides here to set the stride, offset and size correctly.
	if (_dataFormat == ITextureBuffer::DataFormat::DepthUNorm24)
	{
		elementSizeInBytes = 3;
		pixelStrideInBytes = 4;
		pixelOffsetInBytes = 0;
	}
	else if (_dataFormat == ITextureBuffer::DataFormat::DepthUNorm24StencilUInt8)
	{
		if (!stencilComponent)
		{
			// For D24/S8, the depth aspect is treated as VK_FORMAT_X8_D24_UNORM_PACK32
			elementSizeInBytes = 3;
			pixelStrideInBytes = 4;
			pixelOffsetInBytes = 0;
		}
		else
		{
			// For D24/S8, the stencil aspect is treated as VK_FORMAT_S8_UINT.
			elementSizeInBytes = 1;
			pixelStrideInBytes = 1;
			pixelOffsetInBytes = 0;
		}
	}
	else if (_dataFormat == ITextureBuffer::DataFormat::DepthFloat32StencilUInt8)
	{
		if (!stencilComponent)
		{
			// For D32/S8, the depth aspect is treated as VK_FORMAT_D32_SFLOAT.
			elementSizeInBytes = 4;
			pixelStrideInBytes = 4;
			pixelOffsetInBytes = 0;
		}
		else
		{
			// For D32/S8, the stencil aspect is treated as VK_FORMAT_S8_UINT.
			elementSizeInBytes = 1;
			pixelStrideInBytes = 1;
			pixelOffsetInBytes = 0;
		}
	}
	else
	{
		elementSizeInBytes = _elementSizeInBytes;
		pixelStrideInBytes = _pixelStrideInBytes;
		pixelOffsetInBytes = _pixelOffsetInBytes;
	}
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
void VulkanTextureBuffer<InterfaceType, DimensionVectorType>::SetTextureDimensions(uint32_t faceLength, int mipmapLevelCount)
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
void VulkanTextureBuffer<InterfaceType, DimensionVectorType>::SetTextureDimensions(uint32_t faceLength, size_t arraySize, int mipmapLevelCount)
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
void VulkanTextureBuffer<InterfaceType, DimensionVectorType>::SetTextureDimensions(const DimensionVectorType& imageDimensions, int mipmapLevelCount)
{
	SetTextureDimensions(imageDimensions, 1, mipmapLevelCount);
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
void VulkanTextureBuffer<InterfaceType, DimensionVectorType>::SetTextureDimensions(const DimensionVectorType& imageDimensions, size_t arraySize, int mipmapLevelCount)
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
typename InterfaceType::SampleCount VulkanTextureBuffer<InterfaceType, DimensionVectorType>::GetSampleCount() const
{
	return _sampleCount;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
bool VulkanTextureBuffer<InterfaceType, DimensionVectorType>::IsSampleCountSupported(typename InterfaceType::ImageFormat imageFormat, typename InterfaceType::DataFormat dataFormat, typename InterfaceType::SampleCount sampleCount) const
{
	// Resolve the requested Cobalt format to the native texture format that would actually be allocated.
	typename InterfaceType::ImageFormat selectedImageFormat = {};
	typename InterfaceType::DataFormat selectedDataFormat = {};
	VkFormat nativeFormat = {};
	size_t elementCount = {};
	size_t elementSizeInBytes = {};
	size_t pixelOffsetInBytes = {};
	size_t pixelStrideInBytes = {};
	if (!GetFormatNative(imageFormat, dataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes))
	{
		return false;
	}

	// Calculate the image flags required for a framebuffer output texture with the requested format.
	VkImageUsageFlags nativeUsageFlags = {};
	VkImageCreateFlags nativeCreateFlags = {};
	CalculateBufferSettings(InterfaceType::UsageFlags::FrameBufferOutput, selectedImageFormat, selectedDataFormat, InterfaceType::PerformanceHint::Default, InterfaceType::PerformanceHint::Default, nativeUsageFlags, nativeCreateFlags);

	// Query Vulkan for the image limits and sample counts supported by this native image configuration.
	VkImageFormatProperties imageFormatProperties = {};
	VkResult imageFormatPropertiesResult = vkGetPhysicalDeviceImageFormatProperties(Renderer()->GetPhysicalDevice(), nativeFormat, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, nativeUsageFlags, nativeCreateFlags, &imageFormatProperties);
	if (imageFormatPropertiesResult != VK_SUCCESS)
	{
		return false;
	}

	// Confirm the requested sample count is available for the assumed single mip level, one layer texture.
	VkSampleCountFlagBits nativeSampleCount = GetNativeSampleCountFromSampleCount(sampleCount);
	return ((imageFormatProperties.sampleCounts & nativeSampleCount) != 0) && (imageFormatProperties.maxMipLevels >= 1) && (imageFormatProperties.maxArrayLayers >= 1) && (imageFormatProperties.maxExtent.width >= 1) && (imageFormatProperties.maxExtent.height >= 1);
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
void VulkanTextureBuffer<InterfaceType, DimensionVectorType>::SetSampleCount(typename InterfaceType::SampleCount sampleCount)
{
	_sampleCount = sampleCount;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
VkSampleCountFlagBits VulkanTextureBuffer<InterfaceType, DimensionVectorType>::GetNativeSampleCountFromSampleCount(typename InterfaceType::SampleCount sampleCount)
{
	switch (sampleCount)
	{
	case InterfaceType::SampleCount::SampleCount1:
		return VK_SAMPLE_COUNT_1_BIT;
	case InterfaceType::SampleCount::SampleCount2:
		return VK_SAMPLE_COUNT_2_BIT;
	case InterfaceType::SampleCount::SampleCount4:
		return VK_SAMPLE_COUNT_4_BIT;
	case InterfaceType::SampleCount::SampleCount8:
		return VK_SAMPLE_COUNT_8_BIT;
	case InterfaceType::SampleCount::SampleCount16:
		return VK_SAMPLE_COUNT_16_BIT;
	case InterfaceType::SampleCount::SampleCount32:
		return VK_SAMPLE_COUNT_32_BIT;
	}
	UNREACHABLE();
	return {};
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
typename InterfaceType::ImageFormat VulkanTextureBuffer<InterfaceType, DimensionVectorType>::AllocatedImageFormat() const
{
	return _imageFormat;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
typename InterfaceType::DataFormat VulkanTextureBuffer<InterfaceType, DimensionVectorType>::AllocatedDataFormat() const
{
	return _dataFormat;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
size_t VulkanTextureBuffer<InterfaceType, DimensionVectorType>::ArraySize() const
{
	return _arraySize;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
int VulkanTextureBuffer<InterfaceType, DimensionVectorType>::MipmapLevelCount() const
{
	return _mipmapLevels;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
DimensionVectorType VulkanTextureBuffer<InterfaceType, DimensionVectorType>::MipmapLevelDimensions(int mipmapLevel) const
{
	return _mipmapDimensions[mipmapLevel];
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
int VulkanTextureBuffer<InterfaceType, DimensionVectorType>::CalculateMipmapLevelCount(const V1UInt32& imageDimensions)
{
	return 1 + (int)std::floor(std::log2(imageDimensions.X()));
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
int VulkanTextureBuffer<InterfaceType, DimensionVectorType>::CalculateMipmapLevelCount(const V2UInt32& imageDimensions)
{
	return 1 + (int)std::floor(std::log2(std::max(imageDimensions.X(), imageDimensions.Y())));
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
int VulkanTextureBuffer<InterfaceType, DimensionVectorType>::CalculateMipmapLevelCount(const V3UInt32& imageDimensions)
{
	return 1 + (int)std::floor(std::log2(std::max({imageDimensions.X(), imageDimensions.Y(), imageDimensions.Z()})));
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
V1UInt32 VulkanTextureBuffer<InterfaceType, DimensionVectorType>::CalculateMipmapLevelDimensions(const V1UInt32& parentLevelDimensions)
{
	return V1UInt32(std::max(1u, (parentLevelDimensions.X() / 2)));
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
V2UInt32 VulkanTextureBuffer<InterfaceType, DimensionVectorType>::CalculateMipmapLevelDimensions(const V2UInt32& parentLevelDimensions)
{
	return V2UInt32(std::max(1u, (parentLevelDimensions.X() / 2)), std::max(1u, (parentLevelDimensions.Y() / 2)));
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
V3UInt32 VulkanTextureBuffer<InterfaceType, DimensionVectorType>::CalculateMipmapLevelDimensions(const V3UInt32& parentLevelDimensions)
{
	return V3UInt32(std::max(1u, (parentLevelDimensions.X() / 2)), std::max(1u, (parentLevelDimensions.Y() / 2)), std::max(1u, (parentLevelDimensions.Z() / 2)));
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
uint32_t VulkanTextureBuffer<InterfaceType, DimensionVectorType>::GetSmallestImageDimension(const V1UInt32& imageDimensions)
{
	return imageDimensions.X();
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
uint32_t VulkanTextureBuffer<InterfaceType, DimensionVectorType>::GetSmallestImageDimension(const V2UInt32& imageDimensions)
{
	return std::min(imageDimensions.X(), imageDimensions.Y());
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
uint32_t VulkanTextureBuffer<InterfaceType, DimensionVectorType>::GetSmallestImageDimension(const V3UInt32& imageDimensions)
{
	return std::min({imageDimensions.X(), imageDimensions.Y(), imageDimensions.Z()});
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
bool VulkanTextureBuffer<InterfaceType, DimensionVectorType>::RegionIsWithinImageDimensions(const V1UInt32& imageDimensions, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels)
{
	return (imageOffsetInPixels.X() < imageDimensions.X()) && ((imageOffsetInPixels.X() + imageRegionInPixels.X()) <= imageDimensions.X());
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
bool VulkanTextureBuffer<InterfaceType, DimensionVectorType>::RegionIsWithinImageDimensions(const V2UInt32& imageDimensions, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels)
{
	return (imageOffsetInPixels.X() < imageDimensions.X()) && ((imageOffsetInPixels.X() + imageRegionInPixels.X()) <= imageDimensions.X()) && (imageOffsetInPixels.Y() < imageDimensions.Y()) && ((imageOffsetInPixels.Y() + imageRegionInPixels.Y()) <= imageDimensions.Y());
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
bool VulkanTextureBuffer<InterfaceType, DimensionVectorType>::RegionIsWithinImageDimensions(const V3UInt32& imageDimensions, const V3UInt32& imageOffsetInPixels, const V3UInt32& imageRegionInPixels)
{
	return (imageOffsetInPixels.X() < imageDimensions.X()) && ((imageOffsetInPixels.X() + imageRegionInPixels.X()) <= imageDimensions.X()) && (imageOffsetInPixels.Y() < imageDimensions.Y()) && ((imageOffsetInPixels.Y() + imageRegionInPixels.Y()) <= imageDimensions.Y()) && (imageOffsetInPixels.Z() < imageDimensions.Z()) && ((imageOffsetInPixels.Z() + imageRegionInPixels.Z()) <= imageDimensions.Z());
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
std::string VulkanTextureBuffer<InterfaceType, DimensionVectorType>::ImageDimensionAsString(const V1UInt32& imageDimensions)
{
	return "[" + std::to_string(imageDimensions.X()) + "]";
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
std::string VulkanTextureBuffer<InterfaceType, DimensionVectorType>::ImageDimensionAsString(const V2UInt32& imageDimensions)
{
	return "[" + std::to_string(imageDimensions.X()) + "," + std::to_string(imageDimensions.Y()) + "]";
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
std::string VulkanTextureBuffer<InterfaceType, DimensionVectorType>::ImageDimensionAsString(const V3UInt32& imageDimensions)
{
	return "[" + std::to_string(imageDimensions.X()) + "," + std::to_string(imageDimensions.Y()) + "," + std::to_string(imageDimensions.Z()) + "]";
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
bool VulkanTextureBuffer<InterfaceType, DimensionVectorType>::IsTextureFormatSupported(VkFormat nativeFormat) const
{
	//##TODO## Cache these
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(_renderer->GetPhysicalDevice(), nativeFormat, &formatProperties);
	VkFormatFeatureFlags targetFlags = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT | VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
	return ((formatProperties.optimalTilingFeatures & targetFlags) != 0);
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
bool VulkanTextureBuffer<InterfaceType, DimensionVectorType>::FallbackIfNotSupported(typename InterfaceType::ImageFormat requestedImageFormat, typename InterfaceType::DataFormat requestedDataFormat, typename InterfaceType::ImageFormat& selectedImageFormat, typename InterfaceType::DataFormat& selectedDataFormat, VkFormat& nativeFormat, size_t& elementCount, size_t& elementSizeInBytes, size_t& pixelOffsetInBytes, size_t& pixelStrideInBytes) const
{
	if (IsTextureFormatSupported(nativeFormat))
	{
		return true;
	}
	return GetFormatNative(requestedImageFormat, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
bool VulkanTextureBuffer<InterfaceType, DimensionVectorType>::GetFormatNative(typename InterfaceType::ImageFormat requestedImageFormat, typename InterfaceType::DataFormat requestedDataFormat, typename InterfaceType::ImageFormat& selectedImageFormat, typename InterfaceType::DataFormat& selectedDataFormat, VkFormat& nativeFormat, size_t& elementCount, size_t& elementSizeInBytes, size_t& pixelOffsetInBytes, size_t& pixelStrideInBytes, bool setElementParams) const
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
			nativeFormat = VK_FORMAT_R8_SINT;
			return FallbackIfNotSupported((requestedImageFormat == InterfaceType::ImageFormat::X ? InterfaceType::ImageFormat::XY : InterfaceType::ImageFormat::RG), requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::RG:
		case InterfaceType::ImageFormat::XY:
			nativeFormat = VK_FORMAT_R8G8_SINT;
			return FallbackIfNotSupported((requestedImageFormat == InterfaceType::ImageFormat::XY ? InterfaceType::ImageFormat::XYZ : InterfaceType::ImageFormat::RGB), requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::RGB:
		case InterfaceType::ImageFormat::XYZ:
			nativeFormat = VK_FORMAT_R8G8B8_SINT;
			return FallbackIfNotSupported((requestedImageFormat == InterfaceType::ImageFormat::XYZ ? InterfaceType::ImageFormat::XYZW : InterfaceType::ImageFormat::RGBA), requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::RGBA:
		case InterfaceType::ImageFormat::XYZW:
			nativeFormat = VK_FORMAT_R8G8B8A8_SINT;
			return FallbackIfNotSupported(requestedImageFormat, InterfaceType::DataFormat::Int16, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::BGR:
			nativeFormat = VK_FORMAT_B8G8R8_SINT;
			return FallbackIfNotSupported(InterfaceType::ImageFormat::BGRA, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::BGRA:
			nativeFormat = VK_FORMAT_B8G8R8A8_SINT;
			return FallbackIfNotSupported(InterfaceType::ImageFormat::RGBA, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		}
		break;
	case InterfaceType::DataFormat::Int16:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::R:
		case InterfaceType::ImageFormat::X:
			nativeFormat = VK_FORMAT_R16_SINT;
			return FallbackIfNotSupported((requestedImageFormat == InterfaceType::ImageFormat::X ? InterfaceType::ImageFormat::XY : InterfaceType::ImageFormat::RG), requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::RG:
		case InterfaceType::ImageFormat::XY:
			nativeFormat = VK_FORMAT_R16G16_SINT;
			return FallbackIfNotSupported((requestedImageFormat == InterfaceType::ImageFormat::XY ? InterfaceType::ImageFormat::XYZ : InterfaceType::ImageFormat::RGB), requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::RGB:
		case InterfaceType::ImageFormat::XYZ:
			nativeFormat = VK_FORMAT_R16G16B16_SINT;
			return FallbackIfNotSupported((requestedImageFormat == InterfaceType::ImageFormat::XYZ ? InterfaceType::ImageFormat::XYZW : InterfaceType::ImageFormat::RGBA), requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::RGBA:
		case InterfaceType::ImageFormat::XYZW:
			nativeFormat = VK_FORMAT_R16G16B16A16_SINT;
			return FallbackIfNotSupported(requestedImageFormat, InterfaceType::DataFormat::Int32, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::BGR:
			return GetFormatNative(InterfaceType::ImageFormat::BGRA, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::BGRA:
			return GetFormatNative(InterfaceType::ImageFormat::RGBA, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		}
		break;
	case InterfaceType::DataFormat::Int32:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::R:
		case InterfaceType::ImageFormat::X:
			nativeFormat = VK_FORMAT_R32_SINT;
			return FallbackIfNotSupported((requestedImageFormat == InterfaceType::ImageFormat::X ? InterfaceType::ImageFormat::XY : InterfaceType::ImageFormat::RG), requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::RG:
		case InterfaceType::ImageFormat::XY:
			nativeFormat = VK_FORMAT_R32G32_SINT;
			return FallbackIfNotSupported((requestedImageFormat == InterfaceType::ImageFormat::XY ? InterfaceType::ImageFormat::XYZ : InterfaceType::ImageFormat::RGB), requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::RGB:
		case InterfaceType::ImageFormat::XYZ:
			nativeFormat = VK_FORMAT_R32G32B32_SINT;
			return FallbackIfNotSupported((requestedImageFormat == InterfaceType::ImageFormat::XYZ ? InterfaceType::ImageFormat::XYZW : InterfaceType::ImageFormat::RGBA), requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::RGBA:
		case InterfaceType::ImageFormat::XYZW:
			nativeFormat = VK_FORMAT_R32G32B32A32_SINT;
			return IsTextureFormatSupported(nativeFormat);
		case InterfaceType::ImageFormat::BGR:
			return GetFormatNative(InterfaceType::ImageFormat::BGRA, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::BGRA:
			return GetFormatNative(InterfaceType::ImageFormat::RGBA, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		}
		break;
	case InterfaceType::DataFormat::UInt8:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::R:
		case InterfaceType::ImageFormat::X:
			nativeFormat = VK_FORMAT_R8_UINT;
			return FallbackIfNotSupported((requestedImageFormat == InterfaceType::ImageFormat::X ? InterfaceType::ImageFormat::XY : InterfaceType::ImageFormat::RG), requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::RG:
		case InterfaceType::ImageFormat::XY:
			nativeFormat = VK_FORMAT_R8G8_UINT;
			return FallbackIfNotSupported((requestedImageFormat == InterfaceType::ImageFormat::XY ? InterfaceType::ImageFormat::XYZ : InterfaceType::ImageFormat::RGB), requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::RGB:
		case InterfaceType::ImageFormat::XYZ:
			nativeFormat = VK_FORMAT_R8G8B8_UINT;
			return FallbackIfNotSupported((requestedImageFormat == InterfaceType::ImageFormat::XYZ ? InterfaceType::ImageFormat::XYZW : InterfaceType::ImageFormat::RGBA), requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::RGBA:
		case InterfaceType::ImageFormat::XYZW:
			nativeFormat = VK_FORMAT_R8G8B8A8_UINT;
			return FallbackIfNotSupported(requestedImageFormat, InterfaceType::DataFormat::UInt16, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::BGR:
			nativeFormat = VK_FORMAT_B8G8R8_UINT;
			return FallbackIfNotSupported(InterfaceType::ImageFormat::BGRA, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::BGRA:
			nativeFormat = VK_FORMAT_B8G8R8A8_UINT;
			return FallbackIfNotSupported(InterfaceType::ImageFormat::RGBA, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		}
		break;
	case InterfaceType::DataFormat::UInt16:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::R:
		case InterfaceType::ImageFormat::X:
			nativeFormat = VK_FORMAT_R16_UINT;
			return FallbackIfNotSupported((requestedImageFormat == InterfaceType::ImageFormat::X ? InterfaceType::ImageFormat::XY : InterfaceType::ImageFormat::RG), requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::RG:
		case InterfaceType::ImageFormat::XY:
			nativeFormat = VK_FORMAT_R16G16_UINT;
			return FallbackIfNotSupported((requestedImageFormat == InterfaceType::ImageFormat::XY ? InterfaceType::ImageFormat::XYZ : InterfaceType::ImageFormat::RGB), requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::RGB:
		case InterfaceType::ImageFormat::XYZ:
			nativeFormat = VK_FORMAT_R16G16B16_UINT;
			return FallbackIfNotSupported((requestedImageFormat == InterfaceType::ImageFormat::XYZ ? InterfaceType::ImageFormat::XYZW : InterfaceType::ImageFormat::RGBA), requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::RGBA:
		case InterfaceType::ImageFormat::XYZW:
			nativeFormat = VK_FORMAT_R16G16B16A16_UINT;
			return FallbackIfNotSupported(requestedImageFormat, InterfaceType::DataFormat::UInt32, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::BGR:
			return GetFormatNative(InterfaceType::ImageFormat::BGRA, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::BGRA:
			return GetFormatNative(InterfaceType::ImageFormat::RGBA, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		}
		break;
	case InterfaceType::DataFormat::UInt32:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::R:
		case InterfaceType::ImageFormat::X:
			nativeFormat = VK_FORMAT_R32_UINT;
			return FallbackIfNotSupported((requestedImageFormat == InterfaceType::ImageFormat::X ? InterfaceType::ImageFormat::XY : InterfaceType::ImageFormat::RG), requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::RG:
		case InterfaceType::ImageFormat::XY:
			nativeFormat = VK_FORMAT_R32G32_UINT;
			return FallbackIfNotSupported((requestedImageFormat == InterfaceType::ImageFormat::XY ? InterfaceType::ImageFormat::XYZ : InterfaceType::ImageFormat::RGB), requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::RGB:
		case InterfaceType::ImageFormat::XYZ:
			nativeFormat = VK_FORMAT_R32G32B32_UINT;
			return FallbackIfNotSupported((requestedImageFormat == InterfaceType::ImageFormat::XYZ ? InterfaceType::ImageFormat::XYZW : InterfaceType::ImageFormat::RGBA), requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::RGBA:
		case InterfaceType::ImageFormat::XYZW:
			nativeFormat = VK_FORMAT_R32G32B32A32_UINT;
			return IsTextureFormatSupported(nativeFormat);
		case InterfaceType::ImageFormat::BGR:
			return GetFormatNative(InterfaceType::ImageFormat::BGRA, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::BGRA:
			return GetFormatNative(InterfaceType::ImageFormat::RGBA, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		}
		break;
	case InterfaceType::DataFormat::Norm8:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::R:
		case InterfaceType::ImageFormat::X:
			nativeFormat = VK_FORMAT_R8_SNORM;
			return FallbackIfNotSupported((requestedImageFormat == InterfaceType::ImageFormat::X ? InterfaceType::ImageFormat::XY : InterfaceType::ImageFormat::RG), requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::RG:
		case InterfaceType::ImageFormat::XY:
			nativeFormat = VK_FORMAT_R8G8_SNORM;
			return FallbackIfNotSupported((requestedImageFormat == InterfaceType::ImageFormat::XY ? InterfaceType::ImageFormat::XYZ : InterfaceType::ImageFormat::RGB), requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::RGB:
		case InterfaceType::ImageFormat::XYZ:
			nativeFormat = VK_FORMAT_R8G8B8_SNORM;
			return FallbackIfNotSupported((requestedImageFormat == InterfaceType::ImageFormat::XYZ ? InterfaceType::ImageFormat::XYZW : InterfaceType::ImageFormat::RGBA), requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::RGBA:
		case InterfaceType::ImageFormat::XYZW:
			nativeFormat = VK_FORMAT_R8G8B8A8_SNORM;
			return FallbackIfNotSupported(requestedImageFormat, InterfaceType::DataFormat::Norm16, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::BGR:
			nativeFormat = VK_FORMAT_B8G8R8_SNORM;
			return FallbackIfNotSupported(InterfaceType::ImageFormat::BGRA, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::BGRA:
			nativeFormat = VK_FORMAT_B8G8R8A8_SNORM;
			return FallbackIfNotSupported(InterfaceType::ImageFormat::RGBA, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		}
		break;
	case InterfaceType::DataFormat::Norm16:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::R:
		case InterfaceType::ImageFormat::X:
			nativeFormat = VK_FORMAT_R16_SNORM;
			return FallbackIfNotSupported((requestedImageFormat == InterfaceType::ImageFormat::X ? InterfaceType::ImageFormat::XY : InterfaceType::ImageFormat::RG), requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::RG:
		case InterfaceType::ImageFormat::XY:
			nativeFormat = VK_FORMAT_R16G16_SNORM;
			return FallbackIfNotSupported((requestedImageFormat == InterfaceType::ImageFormat::XY ? InterfaceType::ImageFormat::XYZ : InterfaceType::ImageFormat::RGB), requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::RGB:
		case InterfaceType::ImageFormat::XYZ:
			nativeFormat = VK_FORMAT_R16G16B16_SNORM;
			return FallbackIfNotSupported((requestedImageFormat == InterfaceType::ImageFormat::XYZ ? InterfaceType::ImageFormat::XYZW : InterfaceType::ImageFormat::RGBA), requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::RGBA:
		case InterfaceType::ImageFormat::XYZW:
			nativeFormat = VK_FORMAT_R16G16B16A16_SNORM;
			return FallbackIfNotSupported(requestedImageFormat, InterfaceType::DataFormat::Float32, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::BGR:
			return GetFormatNative(InterfaceType::ImageFormat::BGRA, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::BGRA:
			return GetFormatNative(InterfaceType::ImageFormat::RGBA, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		}
		break;
	case InterfaceType::DataFormat::UNorm8:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::R:
		case InterfaceType::ImageFormat::X:
			nativeFormat = VK_FORMAT_R8_UNORM;
			return FallbackIfNotSupported((requestedImageFormat == InterfaceType::ImageFormat::X ? InterfaceType::ImageFormat::XY : InterfaceType::ImageFormat::RG), requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::RG:
		case InterfaceType::ImageFormat::XY:
			nativeFormat = VK_FORMAT_R8G8_UNORM;
			return FallbackIfNotSupported((requestedImageFormat == InterfaceType::ImageFormat::XY ? InterfaceType::ImageFormat::XYZ : InterfaceType::ImageFormat::RGB), requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::RGB:
		case InterfaceType::ImageFormat::XYZ:
			nativeFormat = VK_FORMAT_R8G8B8_UNORM;
			return FallbackIfNotSupported((requestedImageFormat == InterfaceType::ImageFormat::XYZ ? InterfaceType::ImageFormat::XYZW : InterfaceType::ImageFormat::RGBA), requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::RGBA:
		case InterfaceType::ImageFormat::XYZW:
			nativeFormat = VK_FORMAT_R8G8B8A8_UNORM;
			return FallbackIfNotSupported(requestedImageFormat, InterfaceType::DataFormat::UNorm16, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::BGR:
			nativeFormat = VK_FORMAT_B8G8R8_UNORM;
			return FallbackIfNotSupported(InterfaceType::ImageFormat::BGRA, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::BGRA:
			nativeFormat = VK_FORMAT_B8G8R8A8_UNORM;
			return FallbackIfNotSupported(InterfaceType::ImageFormat::RGBA, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		}
		break;
	case InterfaceType::DataFormat::UNorm16:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::R:
		case InterfaceType::ImageFormat::X:
			nativeFormat = VK_FORMAT_R16_UNORM;
			return FallbackIfNotSupported((requestedImageFormat == InterfaceType::ImageFormat::X ? InterfaceType::ImageFormat::XY : InterfaceType::ImageFormat::RG), requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::RG:
		case InterfaceType::ImageFormat::XY:
			nativeFormat = VK_FORMAT_R16G16_UNORM;
			return FallbackIfNotSupported((requestedImageFormat == InterfaceType::ImageFormat::XY ? InterfaceType::ImageFormat::XYZ : InterfaceType::ImageFormat::RGB), requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::RGB:
		case InterfaceType::ImageFormat::XYZ:
			nativeFormat = VK_FORMAT_R16G16B16_UNORM;
			return FallbackIfNotSupported((requestedImageFormat == InterfaceType::ImageFormat::XYZ ? InterfaceType::ImageFormat::XYZW : InterfaceType::ImageFormat::RGBA), requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::RGBA:
		case InterfaceType::ImageFormat::XYZW:
			nativeFormat = VK_FORMAT_R16G16B16A16_UNORM;
			return FallbackIfNotSupported(requestedImageFormat, InterfaceType::DataFormat::Float32, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::BGR:
			return GetFormatNative(InterfaceType::ImageFormat::BGRA, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::BGRA:
			return GetFormatNative(InterfaceType::ImageFormat::RGBA, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		}
		break;
	case InterfaceType::DataFormat::Float16:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::R:
		case InterfaceType::ImageFormat::X:
			nativeFormat = VK_FORMAT_R16_SFLOAT;
			return FallbackIfNotSupported((requestedImageFormat == InterfaceType::ImageFormat::X ? InterfaceType::ImageFormat::XY : InterfaceType::ImageFormat::RG), requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::RG:
		case InterfaceType::ImageFormat::XY:
			nativeFormat = VK_FORMAT_R16G16_SFLOAT;
			return FallbackIfNotSupported((requestedImageFormat == InterfaceType::ImageFormat::XY ? InterfaceType::ImageFormat::XYZ : InterfaceType::ImageFormat::RGB), requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::RGB:
		case InterfaceType::ImageFormat::XYZ:
			nativeFormat = VK_FORMAT_R16G16B16_SFLOAT;
			return FallbackIfNotSupported((requestedImageFormat == InterfaceType::ImageFormat::XYZ ? InterfaceType::ImageFormat::XYZW : InterfaceType::ImageFormat::RGBA), requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::RGBA:
		case InterfaceType::ImageFormat::XYZW:
			nativeFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
			return FallbackIfNotSupported(requestedImageFormat, InterfaceType::DataFormat::Float32, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::BGR:
			return GetFormatNative(InterfaceType::ImageFormat::BGRA, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::BGRA:
			return GetFormatNative(InterfaceType::ImageFormat::RGBA, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		}
		break;
	case InterfaceType::DataFormat::Float32:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::R:
		case InterfaceType::ImageFormat::X:
			nativeFormat = VK_FORMAT_R32_SFLOAT;
			return FallbackIfNotSupported((requestedImageFormat == InterfaceType::ImageFormat::X ? InterfaceType::ImageFormat::XY : InterfaceType::ImageFormat::RG), requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::RG:
		case InterfaceType::ImageFormat::XY:
			nativeFormat = VK_FORMAT_R32G32_SFLOAT;
			return FallbackIfNotSupported((requestedImageFormat == InterfaceType::ImageFormat::XY ? InterfaceType::ImageFormat::XYZ : InterfaceType::ImageFormat::RGB), requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::RGB:
		case InterfaceType::ImageFormat::XYZ:
			nativeFormat = VK_FORMAT_R32G32B32_SFLOAT;
			return FallbackIfNotSupported((requestedImageFormat == InterfaceType::ImageFormat::XYZ ? InterfaceType::ImageFormat::XYZW : InterfaceType::ImageFormat::RGBA), requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::RGBA:
		case InterfaceType::ImageFormat::XYZW:
			nativeFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
			return IsTextureFormatSupported(nativeFormat);
		case InterfaceType::ImageFormat::BGR:
			return GetFormatNative(InterfaceType::ImageFormat::BGRA, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		case InterfaceType::ImageFormat::BGRA:
			return GetFormatNative(InterfaceType::ImageFormat::RGBA, requestedDataFormat, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		}
		break;
	case InterfaceType::DataFormat::DXT1:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::RGB:
			nativeFormat = VK_FORMAT_BC1_RGB_UNORM_BLOCK;
			return IsTextureFormatSupported(nativeFormat);
		case InterfaceType::ImageFormat::RGBA:
			nativeFormat = VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
			return IsTextureFormatSupported(nativeFormat);
		}
		break;
	case InterfaceType::DataFormat::DXT3:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::RGBA:
			nativeFormat = VK_FORMAT_BC2_UNORM_BLOCK;
			return IsTextureFormatSupported(nativeFormat);
		}
		break;
	case InterfaceType::DataFormat::DXT5:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::RGBA:
			nativeFormat = VK_FORMAT_BC3_UNORM_BLOCK;
			return IsTextureFormatSupported(nativeFormat);
		}
		break;
	case InterfaceType::DataFormat::BPTC:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::RGBA:
			nativeFormat = VK_FORMAT_BC7_UNORM_BLOCK;
			return IsTextureFormatSupported(nativeFormat);
		}
		break;
	case InterfaceType::DataFormat::ASTC4x4:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::RGBA:
			nativeFormat = VK_FORMAT_ASTC_4x4_UNORM_BLOCK;
			return IsTextureFormatSupported(nativeFormat);
		}
		break;
	case InterfaceType::DataFormat::ASTC5x5:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::RGBA:
			nativeFormat = VK_FORMAT_ASTC_5x5_UNORM_BLOCK;
			return IsTextureFormatSupported(nativeFormat);
		}
		break;
	case InterfaceType::DataFormat::ASTC6x6:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::RGBA:
			nativeFormat = VK_FORMAT_ASTC_6x6_UNORM_BLOCK;
			return IsTextureFormatSupported(nativeFormat);
		}
		break;
	case InterfaceType::DataFormat::ASTC8x8:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::RGBA:
			nativeFormat = VK_FORMAT_ASTC_8x8_UNORM_BLOCK;
			return IsTextureFormatSupported(nativeFormat);
		}
		break;
	case InterfaceType::DataFormat::DepthUNorm16:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::Depth:
			nativeFormat = VK_FORMAT_D16_UNORM;
			return FallbackIfNotSupported(InterfaceType::ImageFormat::Depth, InterfaceType::DataFormat::DepthUNorm24, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		}
		break;
	case InterfaceType::DataFormat::DepthUNorm24:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::Depth:
			nativeFormat = VK_FORMAT_D24_UNORM_S8_UINT;
			return FallbackIfNotSupported(InterfaceType::ImageFormat::Depth, InterfaceType::DataFormat::DepthFloat32, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		}
		break;
	case InterfaceType::DataFormat::DepthUNorm24StencilUInt8:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::DepthAndStencil:
			nativeFormat = VK_FORMAT_D24_UNORM_S8_UINT;
			return FallbackIfNotSupported(InterfaceType::ImageFormat::DepthAndStencil, InterfaceType::DataFormat::DepthFloat32StencilUInt8, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		}
		break;
	case InterfaceType::DataFormat::DepthFloat32:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::Depth:
			nativeFormat = VK_FORMAT_D32_SFLOAT;
			return FallbackIfNotSupported(InterfaceType::ImageFormat::DepthAndStencil, InterfaceType::DataFormat::DepthFloat32StencilUInt8, selectedImageFormat, selectedDataFormat, nativeFormat, elementCount, elementSizeInBytes, pixelOffsetInBytes, pixelStrideInBytes);
		}
		break;
	case InterfaceType::DataFormat::DepthFloat32StencilUInt8:
		switch (requestedImageFormat)
		{
		case InterfaceType::ImageFormat::DepthAndStencil:
			nativeFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;
			return true;
		}
		break;
	}
	return false;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
constexpr void VulkanTextureBuffer<InterfaceType, DimensionVectorType>::CalculateBufferSettings(typename InterfaceType::UsageFlags usageFlags, typename InterfaceType::ImageFormat imageFormat, typename InterfaceType::DataFormat dataFormat, typename InterfaceType::PerformanceHint performanceHintCpu, typename InterfaceType::PerformanceHint performanceHintGpu, VkImageUsageFlags& nativeUsageFlags, VkImageCreateFlags& nativeCreateFlags)
{
	// Calculate the usage flags for the texture
	nativeUsageFlags = {};
	if ((usageFlags & InterfaceType::UsageFlags::FrameBufferOutput) != InterfaceType::UsageFlags::Default)
	{
		// Note that we request VK_IMAGE_USAGE_TRANSFER_SRC_BIT to support framebuffer output capture here
		nativeUsageFlags |= ((imageFormat == InterfaceType::ImageFormat::Depth) || (imageFormat == InterfaceType::ImageFormat::DepthAndStencil)) ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		nativeUsageFlags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}
	if ((usageFlags == InterfaceType::UsageFlags::Default) || ((usageFlags & InterfaceType::UsageFlags::ShaderInput) != InterfaceType::UsageFlags::Default))
	{
		// Note that we request VK_IMAGE_USAGE_TRANSFER_DST_BIT here to support texture uploads
		nativeUsageFlags |= VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}

	// Calculate the creation flags for the texture
	nativeCreateFlags = {};
}

//----------------------------------------------------------------------------------------
// Initial data methods
//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
SuccessToken VulkanTextureBuffer<InterfaceType, DimensionVectorType>::SetInitialData(const void* sourceBuffer, size_t sourceBufferSizeInBytes, typename InterfaceType::SourceImageFormat imageFormat, typename InterfaceType::SourceDataFormat dataFormat, typename InterfaceType::CubeMapFace targetFace, int mipmapLevel)
{
	auto arrayIndexFinal = (size_t)targetFace;
	return SetInitialData(sourceBuffer, sourceBufferSizeInBytes, imageFormat, dataFormat, arrayIndexFinal, mipmapLevel);
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
SuccessToken VulkanTextureBuffer<InterfaceType, DimensionVectorType>::SetInitialData(const void* sourceBuffer, size_t sourceBufferSizeInBytes, typename InterfaceType::SourceImageFormat imageFormat, typename InterfaceType::SourceDataFormat dataFormat, typename InterfaceType::CubeMapFace targetFace, size_t arrayIndex, int mipmapLevel)
{
	auto arrayIndexFinal = (arrayIndex * 6) + (size_t)targetFace;
	return SetInitialData(sourceBuffer, sourceBufferSizeInBytes, imageFormat, dataFormat, arrayIndexFinal, mipmapLevel);
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
SuccessToken VulkanTextureBuffer<InterfaceType, DimensionVectorType>::SetInitialData(const void* sourceBuffer, size_t sourceBufferSizeInBytes, typename InterfaceType::SourceImageFormat imageFormat, typename InterfaceType::SourceDataFormat dataFormat, int mipmapLevel)
{
	return SetInitialData(sourceBuffer, sourceBufferSizeInBytes, imageFormat, dataFormat, 0, mipmapLevel);
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
SuccessToken VulkanTextureBuffer<InterfaceType, DimensionVectorType>::SetInitialData(const void* sourceBuffer, size_t sourceBufferSizeInBytes, typename InterfaceType::SourceImageFormat imageFormat, typename InterfaceType::SourceDataFormat dataFormat, size_t arrayIndex, int mipmapLevel)
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
SuccessToken VulkanTextureBuffer<InterfaceType, DimensionVectorType>::QueueDataUpdate(const void* sourceBuffer, size_t sourceBufferSizeInBytes, typename InterfaceType::SourceImageFormat imageFormat, typename InterfaceType::SourceDataFormat dataFormat, typename InterfaceType::CubeMapFace targetFace, int mipmapLevel, const DimensionVectorType& imageOffsetInPixels, const DimensionVectorType& imageRegionInPixels, ITransferBatch* transferBatch)
{
	auto arrayIndexFinal = (size_t)targetFace;
	return QueueDataUpdate(sourceBuffer, sourceBufferSizeInBytes, imageFormat, dataFormat, arrayIndexFinal, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
SuccessToken VulkanTextureBuffer<InterfaceType, DimensionVectorType>::QueueDataUpdate(const void* sourceBuffer, size_t sourceBufferSizeInBytes, typename InterfaceType::SourceImageFormat imageFormat, typename InterfaceType::SourceDataFormat dataFormat, typename InterfaceType::CubeMapFace targetFace, size_t arrayIndex, int mipmapLevel, const DimensionVectorType& imageOffsetInPixels, const DimensionVectorType& imageRegionInPixels, ITransferBatch* transferBatch)
{
	auto arrayIndexFinal = (arrayIndex * 6) + (size_t)targetFace;
	return QueueDataUpdate(sourceBuffer, sourceBufferSizeInBytes, imageFormat, dataFormat, arrayIndexFinal, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
SuccessToken VulkanTextureBuffer<InterfaceType, DimensionVectorType>::QueueDataUpdate(const void* sourceBuffer, size_t sourceBufferSizeInBytes, typename InterfaceType::SourceImageFormat imageFormat, typename InterfaceType::SourceDataFormat dataFormat, int mipmapLevel, const DimensionVectorType& imageOffsetInPixels, const DimensionVectorType& imageRegionInPixels, ITransferBatch* transferBatch)
{
	return QueueDataUpdate(sourceBuffer, sourceBufferSizeInBytes, imageFormat, dataFormat, 0, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
SuccessToken VulkanTextureBuffer<InterfaceType, DimensionVectorType>::QueueDataUpdate(const void* sourceBuffer, size_t sourceBufferSizeInBytes, typename InterfaceType::SourceImageFormat imageFormat, typename InterfaceType::SourceDataFormat dataFormat, size_t arrayIndex, int mipmapLevel, const DimensionVectorType& imageOffsetInPixels, const DimensionVectorType& imageRegionInPixels, ITransferBatch* transferBatch)
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
	auto* transferBatchResolved = KnownDynamicCast<VulkanTransferBatch*>(transferBatch);
	if ((transferBatchResolved != nullptr) && transferBatchResolved->IsSubmitted())
	{
		_log->Error("Attempted to queue a transfer using a transfer batch that has already been submitted");
		return false;
	}

	// Capture the supplied update settings and data, performing format conversion if required.
	const auto* uploadData = reinterpret_cast<const uint8_t*>(sourceBuffer);
	size_t uploadDataSizeInBytes = sourceBufferSizeInBytes;
	PendingWrite pendingWrite(arrayIndex, mipmapLevel, imageOffsetInPixels, imageRegionInPixels);
	std::vector<uint8_t> convertedData;
	if (!InterfaceType::ImageFormatsAreBinaryEquivalent(imageFormat, _imageFormat) || !InterfaceType::DataFormatsAreBinaryEquivalent(dataFormat, _dataFormat))
	{
		if (!ConvertDataFormat(sourceBuffer, sourceBufferSizeInBytes, imageFormat, dataFormat, _imageFormat, _dataFormat, convertedData))
		{
			_log->Error("QueueDataUpdate failed to convert source data");
			return false;
		}
		uploadData = convertedData.data();
		uploadDataSizeInBytes = convertedData.size();
	}

	// Create a staging buffer
	VkBuffer stagingBuffer;
	VmaAllocation stagingBufferAllocation;
	if (transferBatchResolved != nullptr)
	{
		transferBatchResolved->CreateTemporaryTransferBuffer(uploadDataSizeInBytes, stagingBuffer, stagingBufferAllocation);
	}
	else
	{
		_renderer->CreateTemporaryUploadBuffer(uploadDataSizeInBytes, stagingBuffer, stagingBufferAllocation, true);
	}

	// Copy the supplied data to the staging buffer
	uint8_t* stagingBufferMappedData;
	VulkanMemoryManager* memoryManager = _renderer->GetMemoryManager();
	memoryManager->MapBufferMemory(stagingBufferAllocation, stagingBufferMappedData);
	std::memcpy(stagingBufferMappedData, uploadData, uploadDataSizeInBytes);
	memoryManager->UnmapBufferMemory(stagingBufferAllocation);
	pendingWrite.stagingBuffer = stagingBuffer;

	// Populate the image copy region in advance to make the final transfer faster on the render thread
	PopulateImageCopyRegion(pendingWrite, _aspectFlags, pendingWrite.copyRegion);

	// Queue a task to update the buffer with the supplied data
	if (transferBatchResolved != nullptr)
	{
		// Note that we do all steps unconditionally, even with a shared graphics/transfer queue, as we have image
		// layout transitions to trigger even if there are no queue transfer operations to perform.
		AddBatchTransferInitializeOperation(transferBatchResolved);
		transferBatchResolved->AddTransferOperation([this, pendingWrite](VkCommandBuffer commandBuffer) { CompleteBatchPendingDataWrite(commandBuffer, pendingWrite); });
		AddBatchTransferFinalizeOperation(transferBatchResolved);
	}
	else
	{
		std::unique_lock<std::mutex> lock(_buildStateMutex);
		_state[_buildIndex].pendingWrites.push_back(std::move(pendingWrite));
		lock.unlock();
		FlagBuildStateModified();
	}
	return true;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
void VulkanTextureBuffer<InterfaceType, DimensionVectorType>::CompletePendingDataWrites(VkCommandBuffer commandBuffer)
{
	// If there are no pending writes, but the buffer has a pending image layout state transition from its initial
	// state, perform it now, and abort any further processing.
	bool hasPendingWrites = !_state[_drawIndex].pendingWrites.empty();
	if (!hasPendingWrites && (_currentImageLayout != _defaultImageLayout))
	{
		VulkanMemoryManager* memoryManager = _renderer->GetMemoryManager();
		memoryManager->RecordTransitionImageLayout(commandBuffer, _image, _internalFormat, _mipmapLevels, (uint32_t)_arraySize, _currentImageLayout, _defaultImageLayout);
		_currentImageLayout = _defaultImageLayout;
		return;
	}

	// If there are no pending writes, abort any further processing.
	if (!hasPendingWrites)
	{
		return;
	}

	// Transition the texture into the correct image layout to receive an upload if required
	VulkanMemoryManager* memoryManager = _renderer->GetMemoryManager();
	if (_currentImageLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		memoryManager->RecordTransitionImageLayout(commandBuffer, _image, _internalFormat, _mipmapLevels, (uint32_t)_arraySize, _currentImageLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		_currentImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	}

	// Complete all pending data writes
	for (const PendingWrite& write : _state[_drawIndex].pendingWrites)
	{
		CompletePendingDataWrite(commandBuffer, write);
	}
	_state[_drawIndex].pendingWrites.clear();

	// Transition our buffer back into its default state
	memoryManager->RecordTransitionImageLayout(commandBuffer, _image, _internalFormat, _mipmapLevels, (uint32_t)_arraySize, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, _defaultImageLayout);
	_currentImageLayout = _defaultImageLayout;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
bool VulkanTextureBuffer<InterfaceType, DimensionVectorType>::CompleteBatchPendingDataWrite(VkCommandBuffer commandBuffer, const PendingWrite& entry)
{
	// Transition the texture into the correct image layout to receive an upload if required
	VulkanMemoryManager* memoryManager = _renderer->GetMemoryManager();
	if (_currentImageLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		memoryManager->RecordTransitionImageLayout(commandBuffer, _image, _internalFormat, _mipmapLevels, (uint32_t)_arraySize, _defaultImageLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		_currentImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	}

	// Copy the staging buffer to the texture buffer
	vkCmdCopyBufferToImage(commandBuffer, entry.stagingBuffer, _image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &entry.copyRegion);
	return true;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
bool VulkanTextureBuffer<InterfaceType, DimensionVectorType>::CompletePendingDataWrite(VkCommandBuffer commandBuffer, const PendingWrite& entry)
{
	// Copy the staging buffer to the texture buffer
	vkCmdCopyBufferToImage(commandBuffer, entry.stagingBuffer, _image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &entry.copyRegion);
	return true;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
void VulkanTextureBuffer<InterfaceType, DimensionVectorType>::PerformGraphicsQueueAcquireOperation(VkCommandBuffer commandBuffer)
{
	// Perform an acquire operation on the graphics queue for the buffer
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.pNext = nullptr;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
	barrier.oldLayout = _currentImageLayout;
	barrier.newLayout = _defaultImageLayout;
	barrier.srcQueueFamilyIndex = _renderer->GetTransferQueueFamily();
	barrier.dstQueueFamilyIndex = _renderer->GetGraphicsQueueFamily();
	barrier.image = _image;
	barrier.subresourceRange.aspectMask = _aspectFlags;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = _mipmapLevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = (uint32_t)_arraySize;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	// Now that we've completed the other half of the queue transfer, complete the layout transfer.
	_currentImageLayout = _defaultImageLayout;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
void VulkanTextureBuffer<InterfaceType, DimensionVectorType>::PerformGraphicsQueueReleaseOperation(VkCommandBuffer commandBuffer)
{
	// Perform a release operation on the graphics queue for the buffer. Note that we don't update _currentImageLayout
	// here though, that gets completed on the acquire.
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.pNext = nullptr;
	barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
	barrier.dstAccessMask = 0;
	barrier.oldLayout = _delayedGraphicsQueueReleaseSourceLayout;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.srcQueueFamilyIndex = _renderer->GetGraphicsQueueFamily();
	barrier.dstQueueFamilyIndex = _renderer->GetTransferQueueFamily();
	barrier.image = _image;
	barrier.subresourceRange.aspectMask = _aspectFlags;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = _mipmapLevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = (uint32_t)_arraySize;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
bool VulkanTextureBuffer<InterfaceType, DimensionVectorType>::PerformTransferQueueAcquireOperation(VkCommandBuffer commandBuffer, bool canDiscardCurrentContent)
{
	// Increment the transfer queue usage count for this buffer. If there was already an existing usage, abort any
	// further processing.
	uint32_t previousTransferQueueUseCount = _transferQueueUseCount.fetch_add(1, std::memory_order_acq_rel);
	if (previousTransferQueueUseCount != 0)
	{
		return false;
	}

	// If we have a combined graphics and transfer queue, perform an image layout transition if required, and abort any
	// further processing.
	if (_renderer->IsTransferQueueSharedWithGraphics())
	{
		if (_currentImageLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			VulkanMemoryManager* memoryManager = _renderer->GetMemoryManager();
			memoryManager->RecordTransitionImageLayout(commandBuffer, _image, _internalFormat, _mipmapLevels, (uint32_t)_arraySize, _currentImageLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			_currentImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		}
		return false;
	}

	// If we're discarding the current buffer content, no queue transfer operation is required, so we abort any further
	// processing. Note that the graphics queue release operation will have already been skipped at this point.
	if (canDiscardCurrentContent)
	{
		return false;
	}

	// Since we're transferring ownership to the transfer queue, attempt to remove this from the list of buffers pending
	// an aquire operation on the graphics queue. This can occur where this same buffer is used in a batch transfer,
	// that transfer completes, then before another frame advances it is added to another batch transfer. If the removal
	// succeeds, we know the buffer is currently still owned by the transfer queue, otherwise we perform a transfer
	// queue acquire operation here.
	if (CancelPendingGraphicsQueueAcquireOperation())
	{
		return true;
	}

	// Perform an acquire operation on the transfer queue for the buffer
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.pNext = nullptr;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.oldLayout = _currentImageLayout;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.srcQueueFamilyIndex = _renderer->GetGraphicsQueueFamily();
	barrier.dstQueueFamilyIndex = _renderer->GetTransferQueueFamily();
	barrier.image = _image;
	barrier.subresourceRange.aspectMask = _aspectFlags;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = _mipmapLevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = (uint32_t)_arraySize;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	// Now that we've completed the other half of the queue transfer, complete the layout transfer.
	_delayedGraphicsQueueReleaseSourceLayout = _currentImageLayout;
	_currentImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	return true;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
bool VulkanTextureBuffer<InterfaceType, DimensionVectorType>::PerformTransferQueueReleaseOperation(VkCommandBuffer commandBuffer)
{
	// Decrement the transfer queue usage count for this buffer. If there is at least one user remaining, abort any
	// further processing.
	uint32_t previousTransferQueueUseCount = _transferQueueUseCount.fetch_sub(1, std::memory_order_acq_rel);
	if (previousTransferQueueUseCount != 1)
	{
		return false;
	}

	// If we have a combined graphics and transfer queue, perform an image layout transition if required, and abort any
	// further processing.
	if (_renderer->IsTransferQueueSharedWithGraphics())
	{
		if (_currentImageLayout != _defaultImageLayout)
		{
			VulkanMemoryManager* memoryManager = _renderer->GetMemoryManager();
			memoryManager->RecordTransitionImageLayout(commandBuffer, _image, _internalFormat, _mipmapLevels, (uint32_t)_arraySize, _currentImageLayout, _defaultImageLayout);
			_currentImageLayout = _defaultImageLayout;
		}

		// Note that we deliberately return false here so we don't queue an acquire operation on the graphics queue
		return false;
	}

	// Perform a release operation on the transfer queue for the buffer, and transition the image layout back to its
	// default. Note that we don't update _currentImageLayout here though, that gets completed on the acquire.
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.pNext = nullptr;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = 0;
	barrier.oldLayout = _currentImageLayout;
	barrier.newLayout = _defaultImageLayout;
	barrier.srcQueueFamilyIndex = _renderer->GetTransferQueueFamily();
	barrier.dstQueueFamilyIndex = _renderer->GetGraphicsQueueFamily();
	barrier.image = _image;
	barrier.subresourceRange.aspectMask = _aspectFlags;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = _mipmapLevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = (uint32_t)_arraySize;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	return true;
}

//----------------------------------------------------------------------------------------
// Data conversion methods
//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
bool VulkanTextureBuffer<InterfaceType, DimensionVectorType>::ConvertDataFormat(const void* sourceBuffer, size_t sourceBufferSizeInBytes, typename InterfaceType::SourceImageFormat sourceImageFormat, typename InterfaceType::SourceDataFormat sourceDataFormat, typename InterfaceType::ImageFormat targetImageFormat, typename InterfaceType::DataFormat targetDataFormat, std::vector<uint8_t>& targetBuffer) const
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
void VulkanTextureBuffer<InterfaceType, DimensionVectorType>::MigrateBuildStateToDrawState()
{
	std::swap(_buildIndex, _drawIndex);
	_stateModified.clear(std::memory_order_relaxed);
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
VkImage VulkanTextureBuffer<InterfaceType, DimensionVectorType>::GetImage()
{
	if (!_isMemoryAllocated)
	{
		_log->Error("Texture has not been allocated");
		return VK_NULL_HANDLE;
	}
	return _image;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
VkImageView VulkanTextureBuffer<InterfaceType, DimensionVectorType>::GetImageView()
{
	if (!_isMemoryAllocated)
	{
		_log->Error("Texture has not been allocated");
		return VK_NULL_HANDLE;
	}
	return _imageView;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
VkFormat VulkanTextureBuffer<InterfaceType, DimensionVectorType>::GetNativeFormat()
{
	if (!_isMemoryAllocated)
	{
		_log->Error("Texture has not been allocated");
		return VK_FORMAT_UNDEFINED;
	}
	return _internalFormat;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
VkImageLayout VulkanTextureBuffer<InterfaceType, DimensionVectorType>::GetDefaultImageLayout()
{
	return _defaultImageLayout;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
void VulkanTextureBuffer<InterfaceType, DimensionVectorType>::FlagBuildStateModified()
{
	if (!_stateModified.test_and_set(std::memory_order_acquire))
	{
		FlagObjectModified();
	}
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
VulkanRenderer* VulkanTextureBuffer<InterfaceType, DimensionVectorType>::Renderer() const
{
	return _renderer;
}

//----------------------------------------------------------------------------------------
template<class InterfaceType, class DimensionVectorType>
cobalt::logging::ILogger* VulkanTextureBuffer<InterfaceType, DimensionVectorType>::Log() const
{
	return _log;
}

} // namespace cobalt::graphics
