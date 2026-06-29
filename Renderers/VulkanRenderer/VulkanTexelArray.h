// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "VulkanHeaders.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <atomic>
#include <mutex>
#include <vector>
namespace cobalt::graphics {
class VulkanRenderer;
class VulkanTransferBatch;
class VulkanTexelArrayOutput;

class VulkanTexelArray : public ITexelArray
{
public:
	// Constructors
	VulkanTexelArray(cobalt::logging::ILogger* log, VulkanRenderer* renderer);
	~VulkanTexelArray();

	// Initialization methods
	void Delete() override;
	SuccessToken AllocateMemory() override;
	bool AllocateAsAliasForBuffer(VkBuffer buffer, size_t bufferSizeInBytes);
	void SetBufferLayout(ImageFormat imageFormat, DataFormat dataFormat, size_t entryCount) override;

	// Usage methods
	void SetUsageFlags(UsageFlags usageFlags) override;
	void SetPerformanceHints(PerformanceHint performanceHintCpu, PerformanceHint performanceHintGpu) override;
	void SetDataPersistenceFlags(DataPersistenceFlags dataPersistenceFlags) override;

	// Initial data methods
	SuccessToken SetInitialData(const void* sourceBuffer, size_t sourceBufferSizeInBytes, SourceImageFormat imageFormat, SourceDataFormat dataFormat) override;

	// Data update methods
	SuccessToken QueueDataUpdate(const void* sourceBuffer, size_t sourceBufferSizeInBytes, SourceImageFormat imageFormat, SourceDataFormat dataFormat, size_t targetBufferOffset, ITransferBatch* transferBatch) override;

	// Data transfer methods
	SuccessToken QueueDataTransfer(ITexelArray* targetBuffer, size_t transferCount, size_t sourceBufferOffset, size_t targetBufferOffset, ITransferBatch* transferBatch) override;

	// Output capture methods
	bool HasCaptureTargets() const;
	void AddOutputCaptureTarget(ITexelArrayOutput* captureTarget) override;
	void RemoveOutputCaptureTarget(ITexelArrayOutput* captureTarget) override;
	void CaptureDataBufferOutput(VkCommandBuffer commandBuffer);
	void CompleteCaptureDataBufferOutput();

	// Build state methods
	void MigrateBuildStateToDrawState();
	void CompletePendingDataWrites(VkCommandBuffer commandBuffer);
	void CompletePendingDataTransfers(VkCommandBuffer commandBuffer);
	void PerformGraphicsQueueAcquireOperation(VkCommandBuffer commandBuffer);
	void PerformGraphicsQueueReleaseOperation(VkCommandBuffer commandBuffer);
	bool PerformTransferQueueAcquireOperation(VkCommandBuffer commandBuffer, bool canDiscardCurrentContent);
	bool PerformTransferQueueReleaseOperation(VkCommandBuffer commandBuffer);
	VkBufferView GetBufferView();
	VkBuffer GetNativeBuffer() const;
	size_t GetBufferSizeInBytes() const;
	void AddAsCurrentBuffer();

private:
	// Structures
	struct PendingWrite
	{
		explicit PendingWrite(VulkanTransferBatch* transferBatch)
		: transferBatch(transferBatch)
		{}

		VkBuffer uploadBuffer = {};
		size_t targetBufferPosInBytes = 0;
		size_t uploadBufferSizeInBytes = 0;
		std::vector<uint8_t> data;
		VulkanTransferBatch* transferBatch;
	};
	struct PendingTransfer
	{
		explicit PendingTransfer(VulkanTexelArray* targetBuffer, VulkanTransferBatch* transferBatch)
		: targetBuffer(targetBuffer), transferBatch(transferBatch)
		{}

		VulkanTexelArray* targetBuffer;
		VulkanTransferBatch* transferBatch;
		size_t transferCountInBytes = 0;
		size_t sourceBufferPosInBytes = 0;
		size_t targetBufferPosInBytes = 0;
	};
	struct MutableState
	{
		std::vector<PendingWrite> pendingWrites;
		std::vector<PendingTransfer> pendingTransfers;
		std::vector<VulkanTexelArrayOutput*> captureTargets;
	};

private:
	// Initialization methods
	void ReleaseMemory();

	// Data conversion methods
	bool ConvertDataFormat(const void* sourceBuffer, size_t sourceBufferSizeInBytes, SourceImageFormat sourceImageFormat, SourceDataFormat sourceDataFormat, ImageFormat targetImageFormat, DataFormat targetDataFormat, std::vector<uint8_t>& targetBuffer) const;

	// Format methods
	constexpr static bool GetFormatNative(ImageFormat requestedImageFormat, DataFormat requestedDataFormat, VkFormat& nativeFormat);

	// Build state methods
	void CompletePendingDataWrite(VkCommandBuffer commandBuffer, const PendingWrite& pendingWrite);
	void CompletePendingDataTransfer(VkCommandBuffer commandBuffer, const PendingTransfer& pendingTransfer);
	void FlagBuildStateModified();

private:
	cobalt::logging::ILogger* _log;
	mutable std::mutex _accessMutex;
	VulkanRenderer* _renderer;
	bool _bufferLayoutSet = false;
	size_t _structureEntryCount = 0;
	size_t _structureStrideInBytes = 0;
	size_t _totalBufferSizeInBytes = 0;
	ImageFormat _imageFormat = {};
	DataFormat _dataFormat = {};
	VkFormat _nativeFormat = {};
	bool _bufferCreated = false;
	VkBuffer _buffer = VK_NULL_HANDLE;
	bool _bufferIsAlias = false;
	VmaAllocation _bufferAllocation = {};
	VkBuffer _captureDataStagingBuffer = VK_NULL_HANDLE;
	VmaAllocation _captureDataStagingBufferAllocation = {};
	VkBufferView _bufferView = VK_NULL_HANDLE;
	bool _createdReadOnlyView = false;
	bool _createdReadWriteView = false;
	bool _initialDataSet = false;
	bool _addedAsCurrent = false;
	const uint8_t* _initialData = nullptr;
	size_t _initialDataSizeInBytes = 0;
	SourceImageFormat _initialDataImageFormat = {};
	SourceDataFormat _initialDataDataFormat = {};
	UsageFlags _usageFlags = UsageFlags::Default;
	PerformanceHint _performanceHintCpu = PerformanceHint::Default;
	PerformanceHint _performanceHintGpu = PerformanceHint::Default;
	DataPersistenceFlags _dataPersistenceFlags = DataPersistenceFlags::PersistAlways;
	std::vector<uint8_t> _initialDataBuffer;
	std::atomic<uint32_t> _transferQueueUseCount = 0;
	std::mutex _buildStateMutex;
	std::atomic_flag _stateModified = ATOMIC_FLAG_INIT;
	uint32_t _buildIndex;
	uint32_t _drawIndex;
	MutableState _state[2];
};

} // namespace cobalt::graphics
