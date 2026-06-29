// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "VulkanHeaders.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <atomic>
#include <mutex>
namespace cobalt::graphics {
class VulkanRenderer;
class VulkanTransferBatch;

template<class InterfaceType, class DimensionVectorType>
class VulkanTextureBuffer : public InterfaceType
{
public:
	// Constructors
	VulkanTextureBuffer(cobalt::logging::ILogger* log, VulkanRenderer* renderer);
	~VulkanTextureBuffer();

	// Initialization methods
	SuccessToken AllocateMemory() final;

	// Usage methods
	typename InterfaceType::UsageFlags GetUsageFlags() const;
	typename InterfaceType::PerformanceHint GetPerformanceHintCpu() const;
	typename InterfaceType::PerformanceHint GetPerformanceHintGpu() const;
	typename InterfaceType::DataPersistenceFlags GetDataPersistenceFlags() const;
	void SetUsageFlags(typename InterfaceType::UsageFlags usageFlags) final;
	void SetPerformanceHints(typename InterfaceType::PerformanceHint performanceHintCpu, typename InterfaceType::PerformanceHint performanceHintGpu) final;
	void SetDataPersistenceFlags(typename InterfaceType::DataPersistenceFlags dataPersistenceFlags) final;

	// Format methods
	void SetTextureFormat(typename InterfaceType::ImageFormat imageFormat, typename InterfaceType::DataFormat dataFormat) final;
	void GetTextureFormatInfo(ITextureBuffer::ImageFormat& imageFormat, ITextureBuffer::DataFormat& dataFormat, size_t& elementCount, size_t& elementSizeInBytes, size_t& pixelOffsetInBytes, size_t& pixelStrideInBytes, bool stencilComponent) const;
	using InterfaceType::SetTextureDimensions;
	// NOLINTNEXTLINE(hicpp-use-override, modernize-use-override)
	virtual void SetTextureDimensions(uint32_t faceLength, int mipmapLevelCount) final;
	// NOLINTNEXTLINE(hicpp-use-override, modernize-use-override)
	virtual void SetTextureDimensions(uint32_t faceLength, size_t arraySize, int mipmapLevelCount) final;
	// NOLINTNEXTLINE(hicpp-use-override, modernize-use-override)
	virtual void SetTextureDimensions(const DimensionVectorType& imageDimensions, int mipmapLevelCount) final;
	// NOLINTNEXTLINE(hicpp-use-override, modernize-use-override)
	virtual void SetTextureDimensions(const DimensionVectorType& imageDimensions, size_t arraySize, int mipmapLevelCount) final;
	typename InterfaceType::SampleCount GetSampleCount() const;
	// NOLINTNEXTLINE(hicpp-use-override, modernize-use-override)
	virtual bool IsSampleCountSupported(typename InterfaceType::ImageFormat imageFormat, typename InterfaceType::DataFormat dataFormat, typename InterfaceType::SampleCount sampleCount) const final;
	// NOLINTNEXTLINE(hicpp-use-override, modernize-use-override)
	virtual void SetSampleCount(typename InterfaceType::SampleCount sampleCount) final;
	static VkSampleCountFlagBits GetNativeSampleCountFromSampleCount(typename InterfaceType::SampleCount sampleCount);
	typename InterfaceType::ImageFormat AllocatedImageFormat() const final;
	typename InterfaceType::DataFormat AllocatedDataFormat() const final;
	size_t ArraySize() const;
	int MipmapLevelCount() const final;
	DimensionVectorType MipmapLevelDimensions(int mipmapLevel) const final;

	// Initial data methods
	using InterfaceType::SetInitialData;
	// NOLINTNEXTLINE(hicpp-use-override, modernize-use-override)
	virtual SuccessToken SetInitialData(const void* sourceBuffer, size_t sourceBufferSizeInBytes, typename InterfaceType::SourceImageFormat imageFormat, typename InterfaceType::SourceDataFormat dataFormat, typename InterfaceType::CubeMapFace targetFace, int mipmapLevel) final;
	// NOLINTNEXTLINE(hicpp-use-override, modernize-use-override)
	virtual SuccessToken SetInitialData(const void* sourceBuffer, size_t sourceBufferSizeInBytes, typename InterfaceType::SourceImageFormat imageFormat, typename InterfaceType::SourceDataFormat dataFormat, typename InterfaceType::CubeMapFace targetFace, size_t arrayIndex, int mipmapLevel) final;
	// NOLINTNEXTLINE(hicpp-use-override, modernize-use-override)
	virtual SuccessToken SetInitialData(const void* sourceBuffer, size_t sourceBufferSizeInBytes, typename InterfaceType::SourceImageFormat imageFormat, typename InterfaceType::SourceDataFormat dataFormat, int mipmapLevel) final;
	// NOLINTNEXTLINE(hicpp-use-override, modernize-use-override)
	virtual SuccessToken SetInitialData(const void* sourceBuffer, size_t sourceBufferSizeInBytes, typename InterfaceType::SourceImageFormat imageFormat, typename InterfaceType::SourceDataFormat dataFormat, size_t arrayIndex, int mipmapLevel) final;

	// Data update methods
	using InterfaceType::QueueDataUpdate;
	// NOLINTNEXTLINE(hicpp-use-override, modernize-use-override)
	virtual SuccessToken QueueDataUpdate(const void* sourceBuffer, size_t sourceBufferSizeInBytes, typename InterfaceType::SourceImageFormat imageFormat, typename InterfaceType::SourceDataFormat dataFormat, typename InterfaceType::CubeMapFace targetFace, int mipmapLevel, const DimensionVectorType& imageOffsetInPixels, const DimensionVectorType& imageRegionInPixels, ITransferBatch* transferBatch) final;
	// NOLINTNEXTLINE(hicpp-use-override, modernize-use-override)
	virtual SuccessToken QueueDataUpdate(const void* sourceBuffer, size_t sourceBufferSizeInBytes, typename InterfaceType::SourceImageFormat imageFormat, typename InterfaceType::SourceDataFormat dataFormat, typename InterfaceType::CubeMapFace targetFace, size_t arrayIndex, int mipmapLevel, const DimensionVectorType& imageOffsetInPixels, const DimensionVectorType& imageRegionInPixels, ITransferBatch* transferBatch) final;
	// NOLINTNEXTLINE(hicpp-use-override, modernize-use-override)
	virtual SuccessToken QueueDataUpdate(const void* sourceBuffer, size_t sourceBufferSizeInBytes, typename InterfaceType::SourceImageFormat imageFormat, typename InterfaceType::SourceDataFormat dataFormat, int mipmapLevel, const DimensionVectorType& imageOffsetInPixels, const DimensionVectorType& imageRegionInPixels, ITransferBatch* transferBatch) final;
	// NOLINTNEXTLINE(hicpp-use-override, modernize-use-override)
	virtual SuccessToken QueueDataUpdate(const void* sourceBuffer, size_t sourceBufferSizeInBytes, typename InterfaceType::SourceImageFormat imageFormat, typename InterfaceType::SourceDataFormat dataFormat, size_t arrayIndex, int mipmapLevel, const DimensionVectorType& imageOffsetInPixels, const DimensionVectorType& imageRegionInPixels, ITransferBatch* transferBatch) final;
	void CompletePendingDataWrites(VkCommandBuffer commandBuffer);

	// Build state methods
	void MigrateBuildStateToDrawState();
	void PerformGraphicsQueueAcquireOperation(VkCommandBuffer commandBuffer);
	void PerformGraphicsQueueReleaseOperation(VkCommandBuffer commandBuffer);
	bool PerformTransferQueueAcquireOperation(VkCommandBuffer commandBuffer, bool canDiscardCurrentContent);
	bool PerformTransferQueueReleaseOperation(VkCommandBuffer commandBuffer);
	VkImage GetImage();
	VkImageView GetImageView();
	VkFormat GetNativeFormat();
	VkImageLayout GetDefaultImageLayout();

protected:
	// Structures
	struct PendingWrite
	{
		PendingWrite(size_t arrayIndex, int mipmapLevel, DimensionVectorType imageOffsetInPixels, DimensionVectorType imageRegionInPixels)
		: arrayIndex(arrayIndex), mipmapLevel(mipmapLevel), imageOffsetInPixels(imageOffsetInPixels), imageRegionInPixels(imageRegionInPixels)
		{}

		size_t arrayIndex = 0;
		int mipmapLevel = 0;
		DimensionVectorType imageOffsetInPixels;
		DimensionVectorType imageRegionInPixels;
		VkBuffer stagingBuffer = {};
		VkBufferImageCopy copyRegion = {};
	};
	struct InitialDataEntry
	{
		InitialDataEntry() = default;
		InitialDataEntry(typename InterfaceType::SourceImageFormat imageFormat, typename InterfaceType::SourceDataFormat dataFormat, size_t arrayIndex, int mipmapLevel)
		: imageFormat(imageFormat), dataFormat(dataFormat), arrayIndex(arrayIndex), mipmapLevel(mipmapLevel)
		{}

		typename InterfaceType::SourceImageFormat imageFormat = InterfaceType::SourceImageFormat::R;
		typename InterfaceType::SourceDataFormat dataFormat = InterfaceType::SourceDataFormat::UInt8;
		size_t arrayIndex = 0;
		int mipmapLevel = 0;
		const void* data = nullptr;
		size_t dataSizeInBytes = 0;
		std::vector<uint8_t> convertedData;
	};

protected:
	// Initialization methods
	void ReleaseMemory();

	// Format methods
	bool IsTextureFormatSupported(VkFormat nativeFormat) const;
	bool FallbackIfNotSupported(typename InterfaceType::ImageFormat requestedImageFormat, typename InterfaceType::DataFormat requestedDataFormat, typename InterfaceType::ImageFormat& selectedImageFormat, typename InterfaceType::DataFormat& selectedDataFormat, VkFormat& nativeFormat, size_t& elementCount, size_t& elementSizeInBytes, size_t& pixelOffsetInBytes, size_t& pixelStrideInBytes) const;
	bool GetFormatNative(typename InterfaceType::ImageFormat requestedImageFormat, typename InterfaceType::DataFormat requestedDataFormat, typename InterfaceType::ImageFormat& selectedImageFormat, typename InterfaceType::DataFormat& selectedDataFormat, VkFormat& nativeFormat, size_t& elementCount, size_t& elementSizeInBytes, size_t& pixelOffsetInBytes, size_t& pixelStrideInBytes, bool setElementParams = false) const;
	constexpr static void CalculateBufferSettings(typename InterfaceType::UsageFlags usageFlags, typename InterfaceType::ImageFormat imageFormat, typename InterfaceType::DataFormat dataFormat, typename InterfaceType::PerformanceHint performanceHintCpu, typename InterfaceType::PerformanceHint performanceHintGpu, VkImageUsageFlags& nativeUsageFlags, VkImageCreateFlags& nativeCreateFlags);

	// Data conversion methods
	bool ConvertDataFormat(const void* sourceBuffer, size_t sourceBufferSizeInBytes, typename InterfaceType::SourceImageFormat sourceImageFormat, typename InterfaceType::SourceDataFormat sourceDataFormat, typename InterfaceType::ImageFormat targetImageFormat, typename InterfaceType::DataFormat targetDataFormat, std::vector<uint8_t>& targetBuffer) const;

	// Build state methods
	virtual void FlagObjectModified() = 0;
	virtual void ScheduleGraphicsQueueAcquireOperation() = 0;
	virtual bool CancelPendingGraphicsQueueAcquireOperation() = 0;
	virtual void AddBatchTransferInitializeOperation(VulkanTransferBatch* transferBatch) = 0;
	virtual void AddBatchTransferFinalizeOperation(VulkanTransferBatch* transferBatch) = 0;
	virtual void PopulateImageCreateInfo(VkImageCreateInfo& imageCreateInfo, VkFormat nativeFormat, VkImageUsageFlags usageFlags, VkImageCreateFlags createFlags) const = 0;
	virtual void PopulateImageViewCreateInfo(VkImageViewCreateInfo& imageViewCreateInfo, VkImage image, VkImageAspectFlagBits aspectFlags, VkFormat nativeFormat) const = 0;
	virtual void PopulateImageCopyRegion(const InitialDataEntry& entry, VkImageAspectFlagBits aspectFlags, VkBufferImageCopy& imageCopyRegion) const = 0;
	virtual void PopulateImageCopyRegion(const PendingWrite& entry, VkImageAspectFlagBits aspectFlags, VkBufferImageCopy& imageCopyRegion) const = 0;
	VulkanRenderer* Renderer() const;
	cobalt::logging::ILogger* Log() const;

private:
	// Structures
	struct MutableState
	{
		std::vector<PendingWrite> pendingWrites;
	};

private:
	// Format methods
	static int CalculateMipmapLevelCount(const V1UInt32& imageDimensions);
	static int CalculateMipmapLevelCount(const V2UInt32& imageDimensions);
	static int CalculateMipmapLevelCount(const V3UInt32& imageDimensions);
	static V1UInt32 CalculateMipmapLevelDimensions(const V1UInt32& parentLevelDimensions);
	static V2UInt32 CalculateMipmapLevelDimensions(const V2UInt32& parentLevelDimensions);
	static V3UInt32 CalculateMipmapLevelDimensions(const V3UInt32& parentLevelDimensions);
	static uint32_t GetSmallestImageDimension(const V1UInt32& imageDimensions);
	static uint32_t GetSmallestImageDimension(const V2UInt32& imageDimensions);
	static uint32_t GetSmallestImageDimension(const V3UInt32& imageDimensions);
	static bool RegionIsWithinImageDimensions(const V1UInt32& imageDimensions, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels);
	static bool RegionIsWithinImageDimensions(const V2UInt32& imageDimensions, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels);
	static bool RegionIsWithinImageDimensions(const V3UInt32& imageDimensions, const V3UInt32& imageOffsetInPixels, const V3UInt32& imageRegionInPixels);
	static std::string ImageDimensionAsString(const V1UInt32& imageDimensions);
	static std::string ImageDimensionAsString(const V2UInt32& imageDimensions);
	static std::string ImageDimensionAsString(const V3UInt32& imageDimensions);

	// Data update methods
	bool CompleteBatchPendingDataWrite(VkCommandBuffer commandBuffer, const PendingWrite& entry);
	bool CompletePendingDataWrite(VkCommandBuffer commandBuffer, const PendingWrite& entry);

	// Build state methods
	void FlagBuildStateModified();

private:
	cobalt::logging::ILogger* _log = nullptr;
	VulkanRenderer* _renderer = nullptr;
	typename InterfaceType::UsageFlags _usageFlags;
	typename InterfaceType::PerformanceHint _performanceHintCpu;
	typename InterfaceType::PerformanceHint _performanceHintGpu;
	typename InterfaceType::DataPersistenceFlags _dataPersistenceFlags;
	typename InterfaceType::SampleCount _sampleCount = InterfaceType::SampleCount::SampleCount1;
	bool _isMemoryAllocated = false;
	bool _imageBufferCreated = false;
	bool _imageViewCreated = false;
	typename InterfaceType::ImageFormat _requestedImageFormat = {};
	typename InterfaceType::DataFormat _requestedDataFormat = {};
	typename InterfaceType::ImageFormat _imageFormat = {};
	typename InterfaceType::DataFormat _dataFormat = {};
	VkFormat _internalFormat = {};
	VkImage _image = {};
	VmaAllocation _imageAllocation = {};
	VkImageView _imageView = {};
	VkImageAspectFlagBits _aspectFlags = {};
	size_t _elementCount = {};
	size_t _elementSizeInBytes = {};
	size_t _pixelOffsetInBytes = {};
	size_t _pixelStrideInBytes = {};
	std::mutex _buildStateMutex;
	std::atomic_flag _stateModified = ATOMIC_FLAG_INIT;
	uint32_t _buildIndex = 0;
	uint32_t _drawIndex = 1;
	MutableState _state[2];
	int _mipmapLevels = 0;
	size_t _arraySize = 0;
	bool _formatSet = false;
	std::vector<DimensionVectorType> _mipmapDimensions;
	std::vector<InitialDataEntry> _initialData;
	VkImageLayout _defaultImageLayout = {};
	VkImageLayout _currentImageLayout = {};
	VkImageLayout _delayedGraphicsQueueReleaseSourceLayout = {};
	std::atomic<uint32_t> _transferQueueUseCount = 0;
	std::mutex _imageStateMutex;
};

} // namespace cobalt::graphics
#include "VulkanTextureBuffer.inl"
