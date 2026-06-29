// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "VulkanHeaders.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <atomic>
#include <memory>
#include <vector>
namespace cobalt::graphics {
class VulkanRenderer;
class VulkanTransferBatch;
class VulkanTexelArray;

class VulkanVertexBuffer : public IVertexBuffer
{
public:
	// Structures
	struct VertexAttributeInfo
	{
		IVertexAttribute::DataType dataType;
		size_t dataTypeByteSize;
		size_t elementCount;
		size_t vertexCount;
		IVertexAttribute::PerformanceHint performanceHintCpu;
		IVertexAttribute::PerformanceHint performanceHintGpu;
		size_t bufferOffsetInBytes;
		size_t vertexPaddingAtStart;
		size_t vertexPaddingAtEnd;
		size_t bufferStartPosInBytes;
		size_t bufferStrideInBytes;
		bool initialDataSet;
		const unsigned char* initialData;
		size_t initialDataEntryStrideInBytes;
	};

public:
	// Constructors
	VulkanVertexBuffer(cobalt::logging::ILogger* log, VulkanRenderer* renderer);
	~VulkanVertexBuffer();

	// Initialization methods
	void Delete() override;
	SuccessToken AllocateMemory() override;
	SuccessToken AllocateMemoryWithAlias(ITexelArray* texelArray) override;

	// Binding methods
	SuccessToken BindVertexAttribute(IVertexAttribute& vertexAttribute) override;
	SuccessToken BindVertexAttributeManualLayout(IVertexAttribute& vertexAttribute, size_t bufferOffsetInBytes, size_t bufferStrideInBytes) override;
	bool IsAllocated();
	const VertexAttributeInfo* GetVertexAttributeInfo(size_t attributeIndex);
	void PerformGraphicsQueueAcquireOperation(VkCommandBuffer commandBuffer);
	void PerformGraphicsQueueReleaseOperation(VkCommandBuffer commandBuffer);
	bool PerformTransferQueueAcquireOperation(VkCommandBuffer commandBuffer, bool canDiscardCurrentContent);
	bool PerformTransferQueueReleaseOperation(VkCommandBuffer commandBuffer);
	VkBuffer GetNativeBuffer();
	void CompletePendingDataWrites(VkCommandBuffer commandBuffer);

	// Data methods
	SuccessToken SetRawInitialData(const uint8_t* data, size_t dataSizeInBytes) override;
	SuccessToken QueueRawDataUpdate(const uint8_t* data, size_t dataSizeInBytes, size_t bufferOffsetInBytes, ITransferBatch* transferBatch) override;

	// Build state methods
	void MigrateBuildStateToDrawState();

protected:
	// Data methods
	SuccessToken SetInitialData(size_t attributeIndex, const uint8_t* data, size_t entryCount, size_t entryStrideInBytes) override;
	SuccessToken QueueDataUpdate(size_t attributeIndex, const uint8_t* data, size_t entryCount, size_t initialVertexNo, size_t entryStrideInBytes, ITransferBatch* transferBatch) override;

private:
	// Structures
	struct PendingWrite
	{
		PendingWrite(const VertexAttributeInfo& attributeInfo, size_t initialBufferPosInBytes, size_t dataSizeInBytes)
		: attributeInfo(attributeInfo), entryCount(dataSizeInBytes), initialVertexNo(initialBufferPosInBytes), entryStrideInBytes(1), rawDataWrite(true)
		{}
		PendingWrite(const VertexAttributeInfo& attributeInfo, size_t entryCount, size_t initialVertexNo, size_t entryStrideInBytes)
		: attributeInfo(attributeInfo), entryCount(entryCount), initialVertexNo(initialVertexNo), entryStrideInBytes(entryStrideInBytes), rawDataWrite(false)
		{}

		bool rawDataWrite;
		const VertexAttributeInfo& attributeInfo;
		size_t entryCount;
		size_t initialVertexNo;
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

	// Binding methods
	bool BindVertexAttributeInternal(IVertexAttribute& vertexAttribute, bool manualLayout, size_t bufferOffsetInBytes, size_t bufferStrideInBytes);

	// Data methods
	bool QueueDataUpdateInternal(PendingWrite& pendingWrite, const uint8_t* data, size_t uploadBufferSizeInBytes, size_t bufferStartPosInBytes, VulkanTransferBatch* transferBatch);

	// Build State methods
	void WriteDataToMappedBuffer(const PendingWrite& pendingWrite, const uint8_t* data, uint8_t* mappedMemory);
	void CompletePendingDataWrite(VkCommandBuffer commandBuffer, const PendingWrite& pendingWrite);
	void FlagBuildStateModified();

private:
	logging::ILogger* _log;
	VulkanRenderer* _renderer;
	std::vector<VertexAttributeInfo> _vertexAttributeInfo;
	VkBuffer _vertexBuffer = {};
	VmaAllocation _vertexBufferAllocation = {};
	size_t _vertexAttributeCount = 0;
	size_t _totalBufferSizeInBytes = 0;
	bool _vertexBufferCreated = false;
	bool _rawInitialDataSet = false;
	bool _manualBufferLayout = false;
	bool _bufferInterleaved = false;
	std::vector<uint8_t> _initialDataBuffer;
	std::atomic<uint32_t> _transferQueueUseCount = 0;
	uint32_t _buildIndex;
	uint32_t _drawIndex;
	MutableState _state[2]{};
	std::atomic_flag _stateModified = ATOMIC_FLAG_INIT;
	std::mutex _buildStateMutex;
};

} // namespace cobalt::graphics
