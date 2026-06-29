// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "VulkanHeaders.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
namespace cobalt::graphics {
class VulkanRenderer;

class VulkanMemoryManager
{
public:
	// Constructors
	VulkanMemoryManager(cobalt::logging::ILogger* log, VulkanRenderer* renderer);
	~VulkanMemoryManager();

	// Allocator methods
	VmaAllocator Allocator() const;

	// Buffer methods
	bool CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags memoryAllocationFlags, VkBuffer& buffer, VmaAllocation& allocation);
	bool CreateMappedBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags memoryAllocationFlags, VkBuffer& buffer, VmaAllocation& allocation, void*& allocationPtr);
	void RecordCopyBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void DestroyBuffer(VkBuffer buffer, VmaAllocation allocation);

	// Image methods
	bool RecordTransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkFormat format, uint32_t mipmapLevels, uint32_t arraySize, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t sourceQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, uint32_t destinationQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED);
	void DestroyImage(VkImage image, VmaAllocation allocation);
	void RecordCopyImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImage dstImage, uint32_t width, uint32_t height, VkImageAspectFlags aspect);
	void RecordCopyImageToBuffer(VkCommandBuffer commandBuffer, VkImage image, VkBuffer buffer, uint32_t width, uint32_t height, int offsetX, int offsetY, VkImageAspectFlags aspect);

	// Memory methods
	bool MapBufferMemory(VmaAllocation allocation, uint8_t*& data);
	void UnmapBufferMemory(VmaAllocation allocation);

private:
	logging::ILogger* _log;
	VulkanRenderer* _renderer;
	VmaAllocator _allocator = {};
};

} // namespace cobalt::graphics
