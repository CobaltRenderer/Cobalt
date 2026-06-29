// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
VulkanTextureBuffer3D::VulkanTextureBuffer3D(cobalt::logging::ILogger* log, VulkanRenderer* renderer)
: VulkanTextureBuffer<ITextureBuffer3D, V3UInt32>(log, renderer)
{}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void VulkanTextureBuffer3D::Delete()
{
	Renderer()->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
void VulkanTextureBuffer3D::FlagObjectModified()
{
	Renderer()->FlagObjectModified(this);
}

//----------------------------------------------------------------------------------------
void VulkanTextureBuffer3D::ScheduleGraphicsQueueAcquireOperation()
{
	Renderer()->ScheduleGraphicsQueueAcquireOperation(this);
}

//----------------------------------------------------------------------------------------
bool VulkanTextureBuffer3D::CancelPendingGraphicsQueueAcquireOperation()
{
	return Renderer()->CancelPendingGraphicsQueueAcquireOperation(this);
}

//----------------------------------------------------------------------------------------
void VulkanTextureBuffer3D::AddBatchTransferInitializeOperation(VulkanTransferBatch* transferBatch)
{
	transferBatch->AddInitializeOperation(this);
}

//----------------------------------------------------------------------------------------
void VulkanTextureBuffer3D::AddBatchTransferFinalizeOperation(VulkanTransferBatch* transferBatch)
{
	transferBatch->AddFinalizeOperation(this);
}

//----------------------------------------------------------------------------------------
void VulkanTextureBuffer3D::PopulateImageCreateInfo(VkImageCreateInfo& imageCreateInfo, VkFormat nativeFormat, VkImageUsageFlags usageFlags, VkImageCreateFlags createFlags) const
{
	int mipmapLevels = MipmapLevelCount();
	auto imageDimensions = MipmapLevelDimensions(0);
	imageCreateInfo = {};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.pNext = nullptr;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_3D;
	imageCreateInfo.extent.width = imageDimensions.X();
	imageCreateInfo.extent.height = imageDimensions.Y();
	imageCreateInfo.extent.depth = imageDimensions.Z();
	imageCreateInfo.mipLevels = mipmapLevels;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.format = nativeFormat;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateInfo.usage = usageFlags;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.flags = createFlags;
}

//----------------------------------------------------------------------------------------
void VulkanTextureBuffer3D::PopulateImageViewCreateInfo(VkImageViewCreateInfo& imageViewCreateInfo, VkImage image, VkImageAspectFlagBits aspectFlags, VkFormat nativeFormat) const
{
	int mipmapLevels = MipmapLevelCount();
	imageViewCreateInfo = {};
	imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.pNext = nullptr;
	imageViewCreateInfo.flags = 0;
	imageViewCreateInfo.image = image;
	imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_3D;
	imageViewCreateInfo.format = nativeFormat;
	imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
	imageViewCreateInfo.subresourceRange.levelCount = mipmapLevels;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imageViewCreateInfo.subresourceRange.layerCount = 1;
	imageViewCreateInfo.subresourceRange.aspectMask = aspectFlags;
}

//----------------------------------------------------------------------------------------
void VulkanTextureBuffer3D::PopulateImageCopyRegion(const InitialDataEntry& entry, VkImageAspectFlagBits aspectFlags, VkBufferImageCopy& imageCopyRegion) const
{
	auto imageDimensions = MipmapLevelDimensions(entry.mipmapLevel);
	imageCopyRegion = {};
	imageCopyRegion.bufferOffset = 0;
	imageCopyRegion.bufferImageHeight = 0;
	imageCopyRegion.bufferRowLength = 0;
	imageCopyRegion.imageSubresource.aspectMask = aspectFlags;
	imageCopyRegion.imageSubresource.mipLevel = entry.mipmapLevel;
	imageCopyRegion.imageSubresource.baseArrayLayer = 0;
	imageCopyRegion.imageSubresource.layerCount = 1;
	imageCopyRegion.imageOffset = {0, 0, 0};
	imageCopyRegion.imageExtent = {imageDimensions.X(), imageDimensions.Y(), imageDimensions.Z()};
}

//----------------------------------------------------------------------------------------
void VulkanTextureBuffer3D::PopulateImageCopyRegion(const PendingWrite& entry, VkImageAspectFlagBits aspectFlags, VkBufferImageCopy& imageCopyRegion) const
{
	auto imageDimensions = MipmapLevelDimensions(entry.mipmapLevel);
	auto targetRegionDimensions = ((entry.imageRegionInPixels.X() == 0) || (entry.imageRegionInPixels.Y() == 0) || (entry.imageRegionInPixels.Z() == 0)) ? V3UInt32(imageDimensions.X() - entry.imageOffsetInPixels.X(), imageDimensions.Y() - entry.imageOffsetInPixels.Y(), imageDimensions.Z() - entry.imageOffsetInPixels.Z()) : entry.imageRegionInPixels;
	imageCopyRegion = {};
	imageCopyRegion.bufferOffset = 0;
	imageCopyRegion.bufferImageHeight = 0;
	imageCopyRegion.bufferRowLength = 0;
	imageCopyRegion.imageSubresource.aspectMask = aspectFlags;
	imageCopyRegion.imageSubresource.mipLevel = entry.mipmapLevel;
	imageCopyRegion.imageSubresource.baseArrayLayer = 0;
	imageCopyRegion.imageSubresource.layerCount = 1;
	imageCopyRegion.imageOffset = {(int32_t)entry.imageOffsetInPixels.X(), (int32_t)entry.imageOffsetInPixels.Y(), (int32_t)entry.imageOffsetInPixels.Z()};
	imageCopyRegion.imageExtent = {targetRegionDimensions.X(), targetRegionDimensions.Y(), targetRegionDimensions.Z()};
}

} // namespace cobalt::graphics
