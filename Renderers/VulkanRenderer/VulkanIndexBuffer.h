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
class VulkanTexelArray;

class VulkanIndexBuffer : public IIndexBuffer
{
public:
	// Structures
	struct IndexAttributeInfo
	{
		IIndexAttribute::DataType dataType;
		size_t dataTypeByteSize;
		size_t indexCount;
		IIndexAttribute::PerformanceHint performanceHintCpu;
		IIndexAttribute::PerformanceHint performanceHintGpu;
		size_t bufferOffsetInBytes;
		size_t bufferStartPosInBytes;
	};

public:
	// Constructors
	VulkanIndexBuffer(cobalt::logging::ILogger* log, VulkanRenderer* renderer);
	~VulkanIndexBuffer();

	// Initialization methods
	void Delete() override;
	SuccessToken AllocateMemory() override;
	SuccessToken AllocateMemoryWithAlias(ITexelArray* texelArray) override;
	bool IsAllocated();

	// Binding methods
	SuccessToken BindIndexAttribute(IIndexAttribute& indexAttribute) override;
	SuccessToken BindIndexAttributeManualLayout(IIndexAttribute& indexAttribute, size_t bufferOffsetInBytes, size_t bufferStrideInBytes) override;
	const IndexAttributeInfo* GetIndexAttributeInfo(size_t attributeIndex) const;

	// Data methods
	SuccessToken SetRawInitialData(const uint8_t* data, size_t dataSizeInBytes) override;
	SuccessToken QueueRawDataUpdate(const uint8_t* data, size_t dataSizeInBytes, size_t bufferOffsetInBytes, ITransferBatch* transferBatch) override;

	// Build state methods
	void MigrateBuildStateToDrawState();
	void CompletePendingDataWrites(VkCommandBuffer commandBuffer);
	void PerformGraphicsQueueAcquireOperation(VkCommandBuffer commandBuffer);
	void PerformGraphicsQueueReleaseOperation(VkCommandBuffer commandBuffer);
	bool PerformTransferQueueAcquireOperation(VkCommandBuffer commandBuffer, bool canDiscardCurrentContent);
	bool PerformTransferQueueReleaseOperation(VkCommandBuffer commandBuffer);
	VkBuffer GetNativeBuffer() const;

protected:
	// Data methods
	SuccessToken SetInitialData(const uint8_t* data, size_t entryCount, size_t entryStrideInBytes) override;
	SuccessToken QueueDataUpdate(const uint8_t* data, size_t entryCount, size_t initialIndexNo, size_t entryStrideInBytes, ITransferBatch* transferBatch) override;

private:
	// Structures
	struct PendingWrite
	{
		PendingWrite(const IndexAttributeInfo& attributeInfo, size_t initialBufferPosInBytes, size_t dataSizeInBytes)
		: attributeInfo(attributeInfo), entryCount(dataSizeInBytes), initialIndexNo(initialBufferPosInBytes), entryStrideInBytes(1), rawDataWrite(true)
		{}
		PendingWrite(const IndexAttributeInfo& attributeInfo, size_t entryCount, size_t initialIndexNo, size_t entryStrideInBytes)
		: attributeInfo(attributeInfo), entryCount(entryCount), initialIndexNo(initialIndexNo), entryStrideInBytes(entryStrideInBytes), rawDataWrite(false)
		{}

		bool rawDataWrite;
		const IndexAttributeInfo& attributeInfo;
		size_t entryCount;
		size_t initialIndexNo;
		size_t entryStrideInBytes;
		VkBuffer uploadBuffer = {};
		size_t targetBufferPos = 0;
		size_t uploadBufferSizeInBytes = 0;
	};
	struct MutableState
	{
		std::vector<PendingWrite> pendingWrites;
	};

private:
	// Initialization methods
	bool AllocateMemoryInternal(VulkanTexelArray* texelArray);
	void ReleaseMemory();

	// Data methods
	bool QueueDataUpdateInternal(PendingWrite& pendingWrite, const uint8_t* data, size_t uploadBufferSizeInBytes, size_t bufferStartPosInBytes, VulkanTransferBatch* transferBatch);

	// Build State methods
	void WriteDataToMappedBuffer(size_t entryCount, size_t entrySizeInBytes, size_t entryStrideInBytes, const uint8_t* data, uint8_t* mappedMemory);
	void CompletePendingDataWrite(VkCommandBuffer commandBuffer, const PendingWrite& pendingWrite);
	void FlagBuildStateModified();

private:
	cobalt::logging::ILogger* _log;
	VulkanRenderer* _renderer = nullptr;
	VkBuffer _indexBuffer = {};
	VmaAllocation _indexBufferAllocation = {};
	IndexAttributeInfo _indexAttributeInfo = {};
	bool _indexAttributeAdded = false;
	bool _indexBufferCreated = false;
	size_t _totalBufferSizeInBytes = {};
	bool _initialDataSet = false;
	bool _manualBufferLayout = false;
	const uint8_t* _initialData = nullptr;
	size_t _initialDataEntryCount = 0;
	size_t _initialDataEntryStrideInBytes = 0;
	size_t _initialDataEntrySizeInBytes = 0;
	std::atomic<uint32_t> _transferQueueUseCount = 0;
	std::mutex _buildStateMutex;
	std::atomic_flag _stateModified = ATOMIC_FLAG_INIT;
	uint32_t _buildIndex;
	uint32_t _drawIndex;
	MutableState _state[2]{};
};

} // namespace cobalt::graphics
