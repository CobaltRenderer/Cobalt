// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "VulkanHeaders.h"
#include "VulkanRenderer.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <variant>
#include <vector>
namespace cobalt::graphics {
class VulkanDataArray;
class VulkanIndexBuffer;
class VulkanTexelArray;
class VulkanTextureBuffer1D;
class VulkanTextureBuffer1DArray;
class VulkanTextureBuffer2D;
class VulkanTextureBuffer2DArray;
class VulkanTextureBuffer3D;
class VulkanTextureBufferCube;
class VulkanTextureBufferCubeArray;
class VulkanVertexBuffer;

class VulkanTransferBatch : public ITransferBatch
{
public:
	// Typedefs
	using QueueOperationObjectTypes = std::variant<VulkanVertexBuffer*, VulkanIndexBuffer*, VulkanTextureBuffer1D*, VulkanTextureBuffer2D*, VulkanTextureBuffer3D*, VulkanTextureBufferCube*, VulkanTextureBuffer1DArray*, VulkanTextureBuffer2DArray*, VulkanTextureBufferCubeArray*, VulkanDataArray*, VulkanTexelArray*>;

public:
	// Constructors
	VulkanTransferBatch(cobalt::logging::ILogger* log, VulkanRenderer* renderer, StartTiming startTiming, EndTiming endTiming);
	~VulkanTransferBatch();

	// Initialization methods
	void Delete() override;

	// Submission methods
	SuccessToken SubmitBatch() override;
	bool IsSubmitted() const override;
	bool IsComplete() const override;
	void WaitForComplete() const override;

	// Operation methods
	void CreateTemporaryTransferBuffer(size_t bufferSizeInBytes, VkBuffer& buffer, VmaAllocation& bufferAllocation);
	void AddInitializeOperation(QueueOperationObjectTypes object);
	void AddTransferOperation(std::function<void(VkCommandBuffer)>&& transferOperation);
	void AddFinalizeOperation(QueueOperationObjectTypes object);

private:
	// Structures
	struct OperationEntry
	{
		QueueOperationObjectTypes object;
		void* objectPointerRaw = nullptr;
	};
	struct TransferBufferAllocation
	{
		size_t bufferSizeInBytes;
		VkBuffer buffer;
		VmaAllocation bufferAllocation;
	};

private:
	logging::ILogger* _log;
	VulkanRenderer* _renderer;
	StartTiming _startTiming;
	EndTiming _endTiming;
	mutable std::mutex _accessMutex;
	mutable std::condition_variable _submissionStateChanged;
	bool _batchSubmitted = false;
	bool _batchDetachedFromOwner = false;
	VkCommandBuffer _commandBuffer = VK_NULL_HANDLE;
	VkCommandPool _commandPool = VK_NULL_HANDLE;
	VkQueue _targetQueue = VK_NULL_HANDLE;
	VkFence _fence = VK_NULL_HANDLE;
	std::shared_ptr<VulkanRenderer::CompletionToken> _submitCompleteToken;
	std::vector<OperationEntry> _initializeOperations;
	std::vector<std::function<void(VkCommandBuffer)>> _transferOperations;
	std::vector<OperationEntry> _finalizeOperations;
	std::vector<TransferBufferAllocation> _transferBufferAllocations;
};

} // namespace cobalt::graphics
