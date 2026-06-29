// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#define VMA_IMPLEMENTATION
#include "VulkanMemoryManager.h"
#include "VulkanHeaders.h"
#include "VulkanRenderer.h"
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
VulkanMemoryManager::VulkanMemoryManager(cobalt::logging::ILogger* log, VulkanRenderer* renderer)
{
	_log = log;
	_renderer = renderer;

	// Use Vulkan Memory Allocator to create and destroy buffers and images
	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.instance = _renderer->GetInstance();
	allocatorInfo.physicalDevice = _renderer->GetPhysicalDevice();
	allocatorInfo.device = _renderer->GetDevice();
	vmaCreateAllocator(&allocatorInfo, &_allocator);
}

//----------------------------------------------------------------------------------------
VulkanMemoryManager::~VulkanMemoryManager()
{
	vmaDestroyAllocator(_allocator);
}

//----------------------------------------------------------------------------------------
// Allocator methods
//----------------------------------------------------------------------------------------
VmaAllocator VulkanMemoryManager::Allocator() const
{
	return _allocator;
}

//----------------------------------------------------------------------------------------
// Buffer methods
//----------------------------------------------------------------------------------------
bool VulkanMemoryManager::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags memoryAllocationFlags, VkBuffer& buffer, VmaAllocation& allocation)
{
	// Buffer information
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferInfo.flags = 0;

	// Allocate memory
	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = memoryUsage;
	allocInfo.flags = memoryAllocationFlags;
	VkResult result = vmaCreateBuffer(_allocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr);
	if (result != VK_SUCCESS)
	{
		_log->Error("Could not create buffer with size {0} and usage {1}. vmaCreateBuffer returned {2}.", size, usage, result);
		return false;
	}
	return true;
}

//----------------------------------------------------------------------------------------
bool VulkanMemoryManager::CreateMappedBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags memoryAllocationFlags, VkBuffer& buffer, VmaAllocation& allocation, void*& allocationPtr)
{
	// Buffer information
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferInfo.flags = 0;

	// Allocate memory
	VmaAllocationCreateInfo allocCreateInfo = {};
	allocCreateInfo.usage = memoryUsage;
	allocCreateInfo.flags = memoryAllocationFlags | VMA_ALLOCATION_CREATE_MAPPED_BIT;
	VmaAllocationInfo allocInfo;
	VkResult result = vmaCreateBuffer(_allocator, &bufferInfo, &allocCreateInfo, &buffer, &allocation, &allocInfo);
	if (result != VK_SUCCESS)
	{
		_log->Error("Could not create mapped buffer with size {0} and usage {1}. vmaCreateBuffer returned {2}.", size, usage, result);
		return false;
	}
	allocationPtr = allocInfo.pMappedData;
	return true;
}

//----------------------------------------------------------------------------------------
void VulkanMemoryManager::RecordCopyBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	// Setup copy buffer command
	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
}

//----------------------------------------------------------------------------------------
void VulkanMemoryManager::DestroyBuffer(VkBuffer buffer, VmaAllocation allocation)
{
	vmaDestroyBuffer(_allocator, buffer, allocation);
}

//----------------------------------------------------------------------------------------
// Image methods
//----------------------------------------------------------------------------------------
bool VulkanMemoryManager::RecordTransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkFormat format, uint32_t mipmapLevels, uint32_t arraySize, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t sourceQueueFamilyIndex, uint32_t destinationQueueFamilyIndex)
{
	// Specify image barrier to transition image
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = sourceQueueFamilyIndex;
	barrier.dstQueueFamilyIndex = destinationQueueFamilyIndex;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mipmapLevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = arraySize;

	// Select resource flags based on layout
	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL || oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL || format == VK_FORMAT_D32_SFLOAT)
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT)
		{
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}
	else
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	// Select access masks based on layout
	//##TODO## These barriers and source stages could be optimised
	//##TODO## Good resources for better understanding Vulkan barriers/synchronization:
	//http://themaister.net/blog/2019/08/14/yet-another-blog-explaining-vulkan-synchronization/
	//https://blog.imaginationtech.com/vulkan-synchronisation-and-graphics-compute-graphics-hazards-part-i
	//https://blog.imaginationtech.com/vulkan-synchronisation-and-graphics-compute-graphics-hazards-part-2
	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;
	if ((oldLayout == VK_IMAGE_LAYOUT_UNDEFINED) && (newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL))
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if ((oldLayout == VK_IMAGE_LAYOUT_UNDEFINED) && (newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR))
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	else if ((oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) && (newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL))
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if ((oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) && (newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL))
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if ((oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) && (newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL))
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if ((oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) && (newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL))
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if ((oldLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) && (newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL))
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if ((oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) && (newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR))
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	else if ((oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) && (newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL))
	{
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if ((oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) && (newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL))
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	else if ((oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) && (newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL))
	{
		barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		sourceStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if ((oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) && (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL))
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	}
	else if ((oldLayout == VK_IMAGE_LAYOUT_UNDEFINED) && (newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL))
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if ((oldLayout == VK_IMAGE_LAYOUT_UNDEFINED) && (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL))
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else if ((oldLayout == VK_IMAGE_LAYOUT_GENERAL) && (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL))
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else if ((oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) && (newLayout == VK_IMAGE_LAYOUT_GENERAL))
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else if ((oldLayout == VK_IMAGE_LAYOUT_UNDEFINED) && (newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL))
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	else
	{
		_log->Error("Unsupported image layout transition");
		return false;
	}

	vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	return true;
}

//----------------------------------------------------------------------------------------
void VulkanMemoryManager::DestroyImage(VkImage image, VmaAllocation allocation)
{
	vmaDestroyImage(_allocator, image, allocation);
}

//----------------------------------------------------------------------------------------
void VulkanMemoryManager::RecordCopyImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImage dstImage, uint32_t width, uint32_t height, VkImageAspectFlags aspect)
{
	// Copy entire image to another image
	VkImageCopy region = {};
	region.srcSubresource.aspectMask = aspect;
	region.srcSubresource.mipLevel = 0;
	region.srcSubresource.baseArrayLayer = 0;
	region.srcSubresource.layerCount = 1;
	region.srcOffset = {0, 0, 0};
	region.dstSubresource.aspectMask = aspect;
	region.dstSubresource.mipLevel = 0;
	region.dstSubresource.baseArrayLayer = 0;
	region.dstSubresource.layerCount = 1;
	region.dstOffset = {0, 0, 0};
	region.extent = {width, height, 1};
	vkCmdCopyImage(commandBuffer, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

//----------------------------------------------------------------------------------------
void VulkanMemoryManager::RecordCopyImageToBuffer(VkCommandBuffer commandBuffer, VkImage image, VkBuffer buffer, uint32_t width, uint32_t height, int offsetX, int offsetY, VkImageAspectFlags aspect)
{
	// Copy entire image to buffer
	VkBufferImageCopy region = {};
	region.bufferOffset = 0;
	region.bufferImageHeight = 0;
	region.bufferRowLength = 0;
	region.imageSubresource.aspectMask = aspect;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = {offsetX, offsetY, 0};
	region.imageExtent = {width, height, 1};
	vkCmdCopyImageToBuffer(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer, 1, &region);
}

//----------------------------------------------------------------------------------------
// Memory methods
//----------------------------------------------------------------------------------------
bool VulkanMemoryManager::MapBufferMemory(VmaAllocation allocation, uint8_t*& data)
{
	// Map the memory buffer
	VkResult vmaMapMemoryReturn = vmaMapMemory(_allocator, allocation, reinterpret_cast<void**>(&data));
	if (vmaMapMemoryReturn != VK_SUCCESS)
	{
		_log->Error("vmaMapMemory with error code {0}", vmaMapMemoryReturn);
		data = nullptr;
		return false;
	}

	// If the memory is not HOST_COHERENT, invalidate it so the CPU sees the latest GPU writes.
	VkResult vmaInvalidateAllocationReturn = vmaInvalidateAllocation(_allocator, allocation, 0, VK_WHOLE_SIZE);
	if (vmaInvalidateAllocationReturn != VK_SUCCESS)
	{
		_log->Warning("vmaInvalidateAllocation with error code {0}. Mapped memory may contain stale data.", vmaInvalidateAllocationReturn);
	}
	return true;
}

//----------------------------------------------------------------------------------------
void VulkanMemoryManager::UnmapBufferMemory(VmaAllocation allocation)
{
	vmaUnmapMemory(_allocator, allocation);
}

} // namespace cobalt::graphics
