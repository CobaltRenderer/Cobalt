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
class VulkanDataArrayOutput;

class VulkanDataArray : public IDataArray
{
public:
	// Constructors
	VulkanDataArray(cobalt::logging::ILogger* log, VulkanRenderer* renderer);
	~VulkanDataArray();

	// Initialization methods
	void Delete() override;
	SuccessToken AllocateMemory() override;
	void SetBufferLayout(size_t entryStrideInBytes, size_t entryCount, bool hasCounter, uint32_t counterResetValue) override;

	// Usage methods
	void SetUsageFlags(UsageFlags usageFlags) override;
	void SetPerformanceHints(PerformanceHint performanceHintCpu, PerformanceHint performanceHintGpu) override;
	void SetDataPersistenceFlags(DataPersistenceFlags dataPersistenceFlags) override;

	// Initial data methods
	SuccessToken SetInitialData(const void* sourceBuffer, size_t sourceBufferSizeInBytes) override;

	// Data update methods
	SuccessToken QueueDataUpdate(const void* sourceBuffer, size_t sourceBufferSizeInBytes, size_t targetBufferOffsetInBytes, ITransferBatch* transferBatch) override;
	void UpdateCounterResetValue(uint32_t counterResetValue) override;

	// Data transfer methods
	SuccessToken QueueDataTransfer(IDataArray* targetBuffer, size_t transferCountInBytes, size_t sourceBufferOffsetInBytes, size_t targetBufferOffsetInBytes, ITransferBatch* transferBatch) override;

	// Output capture methods
	bool HasCaptureTargets() const;
	void AddOutputCaptureTarget(IDataArrayOutput* captureTarget) override;
	void RemoveOutputCaptureTarget(IDataArrayOutput* captureTarget) override;
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
	bool HasCounter() const;
	void ResetCounter(VkCommandBuffer commandBuffer);
	VkBuffer GetNativeBuffer() const;
	VkBuffer GetNativeCounterBuffer() const;
	VmaAllocation GetNativeAllocation() const;
	VmaAllocation GetNativeCounterAllocation() const;
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
		size_t targetBufferPos = 0;
		size_t uploadBufferSizeInBytes = 0;
		VulkanTransferBatch* transferBatch;
	};
	struct PendingTransfer
	{
		explicit PendingTransfer(VulkanDataArray* targetBuffer, VulkanTransferBatch* transferBatch)
		: targetBuffer(targetBuffer), transferBatch(transferBatch)
		{}

		VulkanDataArray* targetBuffer;
		VulkanTransferBatch* transferBatch;
		size_t transferCountInBytes = 0;
		size_t sourceBufferPosInBytes = 0;
		size_t targetBufferPosInBytes = 0;
	};
	struct MutableState
	{
		std::vector<PendingWrite> pendingWrites;
		std::vector<PendingTransfer> pendingTransfers;
		bool updatedCounterResetValue = false;
		uint32_t newCounterResetValue = 0;
		std::vector<VulkanDataArrayOutput*> captureTargets;
	};

private:
	// Initialization methods
	void ReleaseMemory();

	// Build state methods
	void CompletePendingDataWrite(VkCommandBuffer commandBuffer, const PendingWrite& pendingWrite);
	void CompletePendingDataTransfer(VkCommandBuffer commandBuffer, const PendingTransfer& pendingTransfer);
	void FlagBuildStateModified();

private:
	cobalt::logging::ILogger* _log;
	mutable std::mutex _accessMutex;
	VulkanRenderer* _renderer;
	VkBuffer _resourceBuffer = VK_NULL_HANDLE;
	VmaAllocation _resourceBufferAllocation = {};
	VkBuffer _counterBuffer = VK_NULL_HANDLE;
	VmaAllocation _counterBufferAllocation = {};
	VkBuffer _counterUploadBuffer = VK_NULL_HANDLE;
	VmaAllocation _counterUploadBufferAllocation = {};
	VkBuffer _captureDataStagingBuffer = VK_NULL_HANDLE;
	VmaAllocation _captureDataStagingBufferAllocation = {};
	VkBuffer _captureCounterStagingBuffer = VK_NULL_HANDLE;
	VmaAllocation _captureCounterStagingBufferAllocation = {};
	bool _bufferLayoutSet = false;
	size_t _structureEntryCount = 0;
	size_t _structureStrideInBytes = 0;
	size_t _totalBufferSizeInBytes = 0;
	bool _hasCounter = false;
	uint32_t _counterResetValue = 0;
	bool _bufferCreated = false;
	bool _initialDataSet = false;
	const uint8_t* _initialData = nullptr;
	size_t _initialDataSizeInBytes = 0;
	bool _addedAsCurrent = false;
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
