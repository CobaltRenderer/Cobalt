// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "VulkanHeaders.h"
#include "VulkanInstanceData.h"
#include "VulkanMemoryManager.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <chrono>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <variant>
#include <vector>
namespace cobalt::graphics {
class ITextureBindingInfo;
class ISamplerBindingInfo;
class StateBufferBindingInfo;
class ResourceArrayBindingInfo;
class VulkanDefaultState;
class VulkanFrameBuffer;
class VulkanFrameBufferOutput;
class VulkanIndexBuffer;
class VulkanProgramNode;
class VulkanRenderPassNode;
class VulkanRenderableNode;
class VulkanShaderProgram;
class VulkanDataArray;
class VulkanDataArrayOutput;
class VulkanTexelArray;
class VulkanTexelArrayOutput;
class VulkanStateBuffer;
class VulkanStateBufferLayout;
class VulkanStateGroupNode;
class VulkanTextureBuffer1D;
class VulkanTextureBuffer1DArray;
class VulkanTextureBuffer2D;
class VulkanTextureBuffer2DArray;
class VulkanTextureBuffer3D;
class VulkanTextureBufferCube;
class VulkanTextureBufferCubeArray;
class VulkanTextureSampler1D;
class VulkanTextureSampler1DArray;
class VulkanTextureSampler2D;
class VulkanTextureSampler2DArray;
class VulkanTextureSampler3D;
class VulkanTextureSamplerCube;
class VulkanTextureSamplerCubeArray;
class VulkanVertexBuffer;

class VulkanRenderer : public IRenderer
{
public:
	// Structures
	struct ExtensionInfo
	{
		bool extensionLoaded_VK_KHR_draw_indirect_count = false; // NOLINT
		PFN_vkCmdDrawIndexedIndirectCountKHR vkCmdDrawIndexedIndirectCountKHR = nullptr;
		PFN_vkCmdDrawIndirectCountKHR vkCmdDrawIndirectCountKHR = nullptr;
	};
	struct CompletionToken
	{
		// Could all be replaced with std::atomic_flag with notify/wait in C++20
		std::mutex mutex;
		bool complete = false;
		std::condition_variable notifyOnComplete;
	};

public:
	// Constructors
	VulkanRenderer(cobalt::logging::ILogger::unique_ptr log, std::shared_ptr<VulkanInstanceData> instanceData, VkPhysicalDevice physicalDevice, const VkPhysicalDeviceFeatures& deviceFeatures, const std::set<std::string>& deviceExtensions, const std::set<IGraphicsDevice::Feature>& enabledFeatures, const std::set<Options>& enabledOptions, bool nullDescriptorFeatureMissingOrBroken, uint32_t minVertexElementStride);

	// Initialization methods
	SuccessToken Initialize(const WindowSystemInfoBase& windowSystemInfo, InitializationFlags flags) override;
	void Delete() override;

	// Geometry buffer methods
	IVertexBuffer* CreateVertexBufferInternal() override;
	IIndexBuffer* CreateIndexBufferInternal() override;

	// Image buffer methods
	ITextureBuffer1D* CreateTextureBuffer1DInternal() override;
	ITextureBuffer2D* CreateTextureBuffer2DInternal() override;
	ITextureBuffer3D* CreateTextureBuffer3DInternal() override;
	ITextureBufferCube* CreateTextureBufferCubeInternal() override;
	ITextureBuffer1DArray* CreateTextureBuffer1DArrayInternal() override;
	ITextureBuffer2DArray* CreateTextureBuffer2DArrayInternal() override;
	ITextureBufferCubeArray* CreateTextureBufferCubeArrayInternal() override;

	// Image sampler methods
	ITextureSampler1D* CreateTextureSampler1DInternal() override;
	ITextureSampler2D* CreateTextureSampler2DInternal() override;
	ITextureSampler3D* CreateTextureSampler3DInternal() override;
	ITextureSamplerCube* CreateTextureSamplerCubeInternal() override;
	ITextureSampler1DArray* CreateTextureSampler1DArrayInternal() override;
	ITextureSampler2DArray* CreateTextureSampler2DArrayInternal() override;
	ITextureSamplerCubeArray* CreateTextureSamplerCubeArrayInternal() override;

	// Data array methods
	IDataArray* CreateDataArrayInternal() override;
	IDataArrayOutput* CreateDataArrayOutputInternal() override;
	ITexelArray* CreateTexelArrayInternal() override;
	ITexelArrayOutput* CreateTexelArrayOutputInternal() override;

	// Batch methods
	ITransferBatch* CreateTransferBatchInternal(ITransferBatch::StartTiming startTiming, ITransferBatch::EndTiming endTiming) override;

	// Frame buffer methods
	IFrameBuffer* CreateFrameBufferInternal() override;
	IFrameBufferOutput* CreateFrameBufferOutputInternal() override;

	// State buffer methods
	IStateBuffer* CreateStateBufferInternal() override;
	IStateBufferLayout* CreateStateBufferLayoutInternal() override;

	// Render tree node methods
	IRenderPassNode* CreateRenderPassNodeInternal() override;
	IProgramNode* CreateProgramNodeInternal() override;
	IStateGroupNode* CreateStateGroupNodeInternal() override;
	IRenderableNode* CreateRenderableNodeInternal() override;
	IDefaultState* CreateDefaultStateInternal() override;

	// Program methods
	IShaderProgram* CreateShaderProgramInternal() override;

	// Object modification/deletion methods
	template<class T>
	void FlagObjectModified(T* object);
	template<class T>
	void DeleteObject(T* object);

	// Scene content methods
	void SetRenderPasses(IRenderPassNode* const* childNodes, size_t childNodeCount, const int32_t* childNodeSortOrder = nullptr) override;
	void SetRenderPasses(IRenderPassNode::unique_ptr const* childNodes, size_t childNodeCount, const int32_t* childNodeSortOrder = nullptr) override;
	void RemoveAllRenderPasses() override;

	// Render methods
	void StartNewFrame() override;
	void WaitForDrawComplete() const override;
	void WaitForOutputCaptureComplete() const override;
	void WaitForDeferredDeletionComplete() const override;
	void AddCurrentFrameBufferOutput(VulkanFrameBufferOutput* frameBufferOutput);
	void AddCurrentDataArrayOutput(VulkanDataArrayOutput* resourceBufferOutput);
	void AddCurrentDataArray(VulkanDataArray* resourceBuffer);
	void AddCurrentTexelArrayOutput(VulkanTexelArrayOutput* resourceBufferOutput);
	void AddCurrentTexelArray(VulkanTexelArray* resourceBuffer);

	// Instance and device methods
	bool NullDescriptorFeatureMissingOrBroken() const;
	VkInstance GetInstance() const;
	VkPhysicalDevice GetPhysicalDevice() const;
	const VkPhysicalDeviceProperties& GetPhysicalDeviceProperties() const;
	VkDevice GetDevice() const;
	const ExtensionInfo& GetExtensionInfo() const;
	uint32_t GetMinVertexElementStride() const;
	bool PrimitiveRestartSupported() const;

	// Memory methods
	VulkanMemoryManager* GetMemoryManager() const;

	// Queue methods
	uint32_t GetTransferQueueFamily() const;
	uint32_t GetBatchTransferQueueFamily() const;
	uint32_t GetGraphicsQueueFamily() const;
	uint32_t GetPresentQueueFamily() const;
	bool IsTransferQueueSharedWithGraphics() const;
	VkQueue GetGraphicsQueue() const;
	VkQueue GetPresentQueue() const;
	VkCommandBuffer GetBuildCommandBuffer();
	VkCommandBuffer GetBatchCommandBuffer(ITransferBatch::EndTiming endTiming, VkCommandPool& commandPool, VkQueue& targetQueue);
	void FreeBatchCommandBuffer(VkCommandBuffer commandBuffer, VkCommandPool commandPool);
	void SubmitBuildCommandBuffer(VkCommandBuffer commandBuffer);
	void SubmitBatchCommandBuffer(VkCommandBuffer commandBuffer, VkFence completionFence, VkQueue targetQueue, ITransferBatch::StartTiming startTiming, std::shared_ptr<CompletionToken> submitCompleteToken);
	void CreatePersistentUploadBuffer(size_t bufferSizeInBytes, VkBuffer& buffer, VmaAllocation& bufferAllocation);
	void CreatePersistentReadbackBuffer(size_t bufferSizeInBytes, VkBuffer& buffer, VmaAllocation& bufferAllocation);
	void CreateTemporaryUploadBuffer(size_t bufferSizeInBytes, VkBuffer& buffer, VmaAllocation& bufferAllocation, bool keepUntilNextFrame);
	template<class T>
	void ScheduleGraphicsQueueAcquireOperation(T* object);
	template<class T>
	void ScheduleGraphicsQueueReleaseOperation(T* object);
	template<class T>
	bool CancelPendingGraphicsQueueAcquireOperation(T* object);
	void IncrementDetachedTransferBatchCount();
	void DecrementDetachedTransferBatchCount();

	// Texture and sampler methods
	float GetMaxTextureSamplerAnisotropy() const;

	// Null descriptor fallback methods
	VkBuffer GetNullDescriptorFallbackVertexBuffer() const;
	VkBuffer GetNullDescriptorFallbackUniformBuffer() const;
	VkBuffer GetNullDescriptorFallbackStorageBuffer() const;
	VkBufferView GetNullDescriptorFallbackTexelBufferView(VkFormat format, bool writeable) const;
	VkImageView GetNullDescriptorFallbackTextureView(VkImageViewType viewType) const;
	VkSampler GetNullDescriptorFallbackSampler() const;

	// Settings methods
	bool DebugLoggingEnabled() const;

private:
	// Structures
	struct RenderPassEntry
	{
		struct Sorter
		{
			inline bool operator()(const RenderPassEntry& first, const RenderPassEntry& second)
			{
				return (first.sortIndex < second.sortIndex);
			}
		};

		VulkanRenderPassNode* renderPassNode;
		int sortIndex;
	};
	struct TransferBufferAllocation
	{
		size_t bufferSizeInBytes;
		VkBuffer buffer;
		VmaAllocation bufferAllocation;
	};
	struct DelayedTransferEntry
	{
		VkQueue targetQueue;
		VkCommandBuffer commandBuffer;
		VkFence completionFence;
		std::shared_ptr<CompletionToken> submitCompleteToken;
	};
	struct NullDescriptorFallbackBufferInfo
	{
		VkBuffer buffer = VK_NULL_HANDLE;
		VmaAllocation allocation = {};
	};
	struct NullDescriptorFallbackTextureInfo
	{
		VkImage image = VK_NULL_HANDLE;
		VmaAllocation allocation = {};
		VkImageView imageView = VK_NULL_HANDLE;
	};
	using PendingGraphicsUseObjectTypes = std::variant<VulkanVertexBuffer*, VulkanIndexBuffer*, VulkanTextureBuffer1D*, VulkanTextureBuffer2D*, VulkanTextureBuffer3D*, VulkanTextureBufferCube*, VulkanTextureBuffer1DArray*, VulkanTextureBuffer2DArray*, VulkanTextureBufferCubeArray*, VulkanDataArray*, VulkanTexelArray*>;
	struct MutableState
	{
		using AllObjectTypes = std::variant<VulkanVertexBuffer*, VulkanIndexBuffer*, VulkanTextureBuffer1D*, VulkanTextureBuffer2D*, VulkanTextureBuffer3D*, VulkanTextureBufferCube*, VulkanTextureBuffer1DArray*, VulkanTextureBuffer2DArray*, VulkanTextureBufferCubeArray*, VulkanTextureSampler1D*, VulkanTextureSampler2D*, VulkanTextureSampler3D*, VulkanTextureSamplerCube*, VulkanTextureSampler1DArray*, VulkanTextureSampler2DArray*, VulkanTextureSamplerCubeArray*, VulkanFrameBuffer*, VulkanFrameBufferOutput*, VulkanStateBuffer*, VulkanDataArray*, VulkanTexelArray*, VulkanDataArrayOutput*, VulkanTexelArrayOutput*, VulkanStateBufferLayout*, VulkanShaderProgram*, VulkanRenderPassNode*, VulkanProgramNode*, VulkanStateGroupNode*, VulkanRenderableNode*, VulkanDefaultState*>;
		using MigrateStateObjectTypes = std::variant<VulkanVertexBuffer*, VulkanIndexBuffer*, VulkanTextureBuffer1D*, VulkanTextureBuffer2D*, VulkanTextureBuffer3D*, VulkanTextureBufferCube*, VulkanTextureBuffer1DArray*, VulkanTextureBuffer2DArray*, VulkanTextureBufferCubeArray*, VulkanTextureSampler1D*, VulkanTextureSampler2D*, VulkanTextureSampler3D*, VulkanTextureSamplerCube*, VulkanTextureSampler1DArray*, VulkanTextureSampler2DArray*, VulkanTextureSamplerCubeArray*, VulkanFrameBuffer*, VulkanFrameBufferOutput*, VulkanStateBuffer*, VulkanDataArray*, VulkanTexelArray*, VulkanDataArrayOutput*, VulkanTexelArrayOutput*, VulkanDefaultState*>;
		using BufferUpdateObjectTypes = std::variant<VulkanVertexBuffer*, VulkanIndexBuffer*, VulkanTextureBuffer1D*, VulkanTextureBuffer2D*, VulkanTextureBuffer3D*, VulkanTextureBufferCube*, VulkanTextureBuffer1DArray*, VulkanTextureBuffer2DArray*, VulkanTextureBufferCubeArray*, VulkanStateBuffer*, VulkanDataArray*, VulkanTexelArray*>;
		using BufferTransferObjectTypes = std::variant<VulkanDataArray*, VulkanTexelArray*>;

		std::vector<RenderPassEntry> renderPasses;
		std::vector<MigrateStateObjectTypes> migrateStatePendingObjects;
		std::vector<BufferUpdateObjectTypes> bufferUpdatePendingObjects;
		std::vector<BufferTransferObjectTypes> bufferTransferPendingObjects;
		std::vector<TransferBufferAllocation> transferBufferAllocations;
		std::vector<AllObjectTypes> deletePendingObjects;
	};

private:
	// Render methods
	void RenderThread();
	void PerformPrepareBuildOperation();
	void PerformRenderOperation();
	void PerformDeleteLastDrawResourcesOperation();
	void PerformDeleteNextDrawResourcesOperation();
	void PerformSwapOperation();
	void ExecutePendingDelayedTransferCommands();
	void BindTextures(const std::vector<ITextureBindingInfo*>& bindingEntries, VulkanShaderProgram* program, size_t setIndex);
	void BindSamplers(const std::vector<ISamplerBindingInfo*>& bindingEntries, VulkanShaderProgram* program, size_t setIndex);
	void BindStateBuffers(const std::vector<StateBufferBindingInfo*>& bindingEntries, VulkanShaderProgram* program, size_t setIndex);
	void BindResourceArrays(const std::vector<ResourceArrayBindingInfo*>& bindingEntries, VulkanShaderProgram* program, size_t setIndex, VkCommandBuffer commandBuffer, bool performReset);

	// Queue methods
	void CreatePersistentTransferBuffer(size_t bufferSizeInBytes, VkBuffer& buffer, VmaAllocation& bufferAllocation, bool isUploadBuffer);

	// Null descriptor fallback methods
	bool CreateNullDescriptorFallbackBufferIfRequired() const;
	size_t GetNullDescriptorFallbackTexelSizeInBytes(VkFormat format) const;
	void DestroyNullDescriptorFallbackResources();

private:
	logging::ILogger::unique_ptr _log;

	std::shared_ptr<VulkanInstanceData> _instanceData = nullptr;
	VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
	VkDevice _device = VK_NULL_HANDLE;
	VkPhysicalDeviceProperties _physicalDeviceProperties = {};
	std::unique_ptr<VulkanMemoryManager> _memoryManager = nullptr;
	VkPhysicalDeviceFeatures _deviceFeatures = {};
	std::set<std::string> _deviceExtensions;
	std::set<IGraphicsDevice::Feature> _enabledFeatures;
	std::set<Options> _enabledOptions;
	uint32_t _minVertexElementStride;
	bool _indirectDrawCountFromBufferFallbackActive = false;

	mutable std::mutex _renderThreadMutex;
	mutable std::mutex _buildStateMutex;
	mutable std::mutex _heapAllocationMutex;
	mutable std::mutex _pendingGraphicsQueueTransferMutex;
	mutable std::condition_variable _notifyRenderThreadTaskPending;
	mutable std::condition_variable _notifyRenderThreadTaskComplete;
	std::condition_variable _notifyRenderThreadStopped;
	bool _renderThreadActive = false;
	bool _frameAdvanceInProgress = false;
	bool _buildingInProgress = false;
	bool _drawingInProgress = false;
	bool _drawingComplete = false;
	bool _buildToDrawRequestPending = false;
	bool _renderRequestPending = false;
	bool _swapRequestPending = false;
	bool _deleteLastDrawResourcesRequestPending = false;
	mutable bool _earlyDeleteNextDrawResourcesRequestPending = false;
	bool _enableDebugLogging = false;
	bool _useRenderMarkers = false;
	bool _nullDescriptorFeatureMissingOrBroken = false;
	bool _primitiveRestartSupported = false;
	ExtensionInfo _extensionInfo;

	uint32_t _graphicsQueueFamilyIndex = {};
	uint32_t _transferQueueFamilyIndex = {};
	uint32_t _batchTransferQueueFamilyIndex = {};
	uint32_t _computeQueueFamilyIndex = {};
	uint32_t _presentQueueFamilyIndex = {};
	VkQueue _graphicsQueue = VK_NULL_HANDLE;
	VkQueue _transferQueue = VK_NULL_HANDLE;
	VkQueue _batchTransferQueue = VK_NULL_HANDLE;
	VkQueue _computeQueue = VK_NULL_HANDLE;
	VkQueue _presentQueue = VK_NULL_HANDLE;
	VkCommandPool _graphicsCommandPool = VK_NULL_HANDLE;
	VkCommandPool _transferCommandPool = VK_NULL_HANDLE;
	VkCommandPool _batchTransferCommandPool = VK_NULL_HANDLE;
	VkCommandPool _computeCommandPool = VK_NULL_HANDLE;
	bool _separateComputeQueue = false;
	std::atomic<bool> _frameStartTransferCommandsInProgress = false;
	std::vector<VkCommandBuffer> _transferCommandBuffers;
	std::vector<VkCommandBuffer> _batchTransferCommandBuffers;
	std::vector<DelayedTransferEntry> _delayedTransferCommandBuffers;
	std::vector<VkCommandBuffer> _delayedTransferCommandBuffersNoFence;
	std::vector<VkPipelineStageFlags> _waitStageMaskBuffer;
	mutable std::mutex _transferBufferMutex;
	mutable std::mutex _queueMutex;

	mutable std::mutex _transferPoolMutex;
	std::atomic_flag _transferPoolLocked = ATOMIC_FLAG_INIT;
	std::condition_variable _transferPoolLockReleased;

	mutable NullDescriptorFallbackBufferInfo _nullDescriptorFallbackBuffer;
	mutable std::unordered_map<uint64_t, VkBufferView> _nullDescriptorFallbackTexelBufferViews;
	mutable std::unordered_map<VkImageViewType, NullDescriptorFallbackTextureInfo> _nullDescriptorFallbackTextures;
	mutable VkSampler _nullDescriptorFallbackSampler = VK_NULL_HANDLE;

	bool _captureTargetsPresent = false;
	std::vector<PendingGraphicsUseObjectTypes> _pendingGraphicsQueueAcquireOperations;
	std::vector<PendingGraphicsUseObjectTypes> _pendingGraphicsQueueReleaseOperations;
	std::vector<VulkanFrameBuffer*> _boundWindowFramebuffers;
	std::vector<VulkanFrameBuffer*> _boundTextureFramebuffers;
	std::vector<VulkanDataArray*> _boundDataArrays;
	std::vector<VulkanTexelArray*> _boundTexelArrays;
	std::vector<VulkanFrameBufferOutput*> _capturedFramebufferOutputsInCurrentFrame;
	std::vector<VulkanDataArrayOutput*> _capturedDataArrayOutputsInCurrentFrame;
	std::vector<VulkanTexelArrayOutput*> _capturedTexelArrayOutputsInCurrentFrame;

	VkFence _drawFence = VK_NULL_HANDLE;
	VkFence _transferCommandsCompleteFence = VK_NULL_HANDLE;
	VkSemaphore _drawPrepFinishedSemaphore = VK_NULL_HANDLE;
	VkSemaphore _drawCompleteSemaphore = VK_NULL_HANDLE;
	VkSemaphore _computeStartSemaphore = VK_NULL_HANDLE;
	VkSemaphore _computeEndSemaphore = VK_NULL_HANDLE;
	std::vector<VkSemaphore> _graphicsQueueReleaseCompleteSemaphores;
	std::vector<VkSemaphore> _presentSemaphores;
	std::vector<VkSemaphore> _drawWaitSemaphores;
	std::vector<VkPipelineStageFlags> _drawWaitStages;

	std::vector<VkCommandBuffer> _commandBuffers;
	std::vector<VkCommandBuffer> _computeCommandBuffers;
	VkCommandBuffer _graphicsQueueReleaseCommandBuffer = VK_NULL_HANDLE;
	uint32_t _currentCommandBufferIndex = 0;
	const static int DefaultCommandBufferCount = 2;
	std::atomic<uint32_t> _detachedTransferBatchCount = 0;
	std::condition_variable _detachedTransferBatchCountReachedZero;

	PFN_vkCmdBeginDebugUtilsLabelEXT _pfnCmdBeginDebugUtilsLabelEXT = nullptr;
	PFN_vkCmdEndDebugUtilsLabelEXT _pfnCmdEndDebugUtilsLabelEXT = nullptr;
	PFN_vkCmdInsertDebugUtilsLabelEXT _pfnCmdInsertDebugUtilsLabelEXT = nullptr;

	uint32_t _buildIndex;
	uint32_t _drawIndex;
	MutableState _state[2];
};

} // namespace cobalt::graphics
#include "VulkanRenderer.inl"
