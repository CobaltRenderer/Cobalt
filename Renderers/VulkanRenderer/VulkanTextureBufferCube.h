// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "VulkanTextureBuffer.h"
namespace cobalt::graphics {

class VulkanTextureBufferCube : public VulkanTextureBuffer<ITextureBufferCube, V2UInt32>
{
public:
	// Constructors
	inline VulkanTextureBufferCube(cobalt::logging::ILogger* log, VulkanRenderer* renderer);

	// Initialization methods
	inline void Delete() final;

protected:
	// Build state methods
	inline void FlagObjectModified() final;
	inline void ScheduleGraphicsQueueAcquireOperation() final;
	inline bool CancelPendingGraphicsQueueAcquireOperation() final;
	inline void AddBatchTransferInitializeOperation(VulkanTransferBatch* transferBatch) final;
	inline void AddBatchTransferFinalizeOperation(VulkanTransferBatch* transferBatch) final;
	inline void PopulateImageCreateInfo(VkImageCreateInfo& imageCreateInfo, VkFormat nativeFormat, VkImageUsageFlags usageFlags, VkImageCreateFlags createFlags) const final;
	inline void PopulateImageViewCreateInfo(VkImageViewCreateInfo& imageViewCreateInfo, VkImage image, VkImageAspectFlagBits aspectFlags, VkFormat nativeFormat) const final;
	inline void PopulateImageCopyRegion(const InitialDataEntry& entry, VkImageAspectFlagBits aspectFlags, VkBufferImageCopy& imageCopyRegion) const final;
	inline void PopulateImageCopyRegion(const PendingWrite& entry, VkImageAspectFlagBits aspectFlags, VkBufferImageCopy& imageCopyRegion) const final;
};

} // namespace cobalt::graphics
#include "VulkanTextureBufferCube.inl"
