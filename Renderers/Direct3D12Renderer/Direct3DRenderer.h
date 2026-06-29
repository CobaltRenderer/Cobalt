// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "BindingHelpers.h"
#include "Direct3DHeaders.h"
// This needs to be after Direct3DHeaders.h so we have our defines for Windows.h set properly
#include "D3D12MemAlloc.h"
#include "Direct3DCommandListPool.h"
#include "Direct3DHeapManager.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <dxgi1_5.h>
#include <memory>
#include <mutex>
#include <variant>
#include <vector>
// We need to define initguid.h before dxgidebug.h here in order to have the GUID values defined for us
#include <initguid.h>
// And this comment line is to keep our include header order rules in clang format happy
#include <dxgidebug.h>
namespace cobalt::graphics {
class Direct3DDefaultState;
class Direct3DFrameBuffer;
class Direct3DFrameBufferOutput;
class Direct3DIndexBuffer;
class Direct3DProgramNode;
class Direct3DRenderPassNode;
class Direct3DRenderableNode;
class Direct3DShaderProgram;
class Direct3DDataArray;
class Direct3DDataArrayOutput;
class Direct3DTexelArray;
class Direct3DTexelArrayOutput;
class Direct3DStateBuffer;
class Direct3DStateBufferLayout;
class Direct3DStateGroupNode;
class Direct3DTextureBuffer1D;
class Direct3DTextureBuffer1DArray;
class Direct3DTextureBuffer2D;
class Direct3DTextureBuffer2DArray;
class Direct3DTextureBuffer3D;
class Direct3DTextureBufferCube;
class Direct3DTextureBufferCubeArray;
class Direct3DTextureSampler1D;
class Direct3DTextureSampler1DArray;
class Direct3DTextureSampler2D;
class Direct3DTextureSampler2DArray;
class Direct3DTextureSampler3D;
class Direct3DTextureSamplerCube;
class Direct3DTextureSamplerCubeArray;
class Direct3DVertexBuffer;

class Direct3DRenderer : public IRenderer
{
public:
	// Constructors
	Direct3DRenderer(cobalt::logging::ILogger::unique_ptr log, Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory, Microsoft::WRL::ComPtr<IDXGIAdapter3> adapter, const Marshal::In<std::set<IGraphicsDevice::Feature>>& enabledFeatures, const Marshal::In<std::set<Options>>& enabledOptions);

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
	void AddCurrentFrameBufferOutput(Direct3DFrameBufferOutput* frameBufferOutput);
	void AddCurrentDataArrayOutput(Direct3DDataArrayOutput* resourceBufferOutput);
	void AddCurrentDataArray(Direct3DDataArray* resourceBuffer);
	void AddCurrentTexelArrayOutput(Direct3DTexelArrayOutput* resourceBufferOutput);
	void AddCurrentTexelArray(Direct3DTexelArray* resourceBuffer);

	// Resource methods
	ID3D12Device* GetDevice() const;
	void EnsureHeapExists(Direct3DHeapManager::ResourceType type) const;
	std::unique_ptr<DescriptorHandle> AllocateDescriptor(Direct3DHeapManager::ResourceType type) const;
	CommandListHandle GetBuildCommandList() const;
	ID3D12CommandQueue* GetBuildCommandQueue() const;
	ID3D12CommandQueue* GetDrawCommandQueue() const;
	D3DX12Residency::ResidencyManager& ResidencyManager() const;
	D3D12MA::Allocator& MemoryManager() const;
	void AllocateDrawCommandList();
	void SubmitDrawCommandList();
	void CreatePersistentUploadBuffer(size_t bufferSizeInBytes, Microsoft::WRL::ComPtr<ID3D12Resource>& bufferPointer, D3D12MA::Allocation*& bufferAllocation);
	void CreatePersistentReadbackBuffer(size_t bufferSizeInBytes, Microsoft::WRL::ComPtr<ID3D12Resource>& bufferPointer, D3D12MA::Allocation*& bufferAllocation);
	void CreateTemporaryUploadBuffer(size_t bufferSizeInBytes, ID3D12Resource*& buffer, bool keepUntilNextFrame);
	void TrackTemporaryUploadBufferUntilNextFrame(Microsoft::WRL::ComPtr<ID3D12Resource>& buffer, std::shared_ptr<D3D12MA::Allocation>& bufferAllocation);
	void ExtendTransferBufferLifetimeToNextFrame(ID3D12Resource* buffer);

	// Settings methods
	bool DebugLoggingEnabled() const;

	// Feature methods
	bool IsFeaturePresent(DXGI_FEATURE feature) const;
	void HighestSupportedShaderModel(unsigned int& targetShaderModelMajor, unsigned int& targetShaderModelMinor) const;

private:
	// Constants
	static const size_t MinTargetIndicesPerCommandList = 1000000;
	static const size_t MinResidencySetTargetInBytes = size_t{1024} * 1024 * 1024;

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

		Direct3DRenderPassNode* renderPassNode;
		int sortIndex;
	};
	struct TransferBufferAllocation
	{
		TransferBufferAllocation() = default;
		TransferBufferAllocation(const TransferBufferAllocation&) = delete;
		TransferBufferAllocation& operator=(const TransferBufferAllocation&) = delete;
		TransferBufferAllocation(TransferBufferAllocation&&) noexcept = default;
		TransferBufferAllocation& operator=(TransferBufferAllocation&&) noexcept = default;
		~TransferBufferAllocation()
		{
			// D3D12MA requires releasing the resource before the allocation.
			// https://gpuopen-librariesandsdks.github.io/D3D12MemoryAllocator/html/quick_start.html#quick_start_resource_reference_counting
			buffer.Reset();
			bufferAllocation.reset();
		}

		size_t bufferSizeInBytes = 0;
		Microsoft::WRL::ComPtr<ID3D12Resource> buffer;
		std::shared_ptr<D3D12MA::Allocation> bufferAllocation;
	};
	struct MutableState
	{
		using AllObjectTypes = std::variant<Direct3DVertexBuffer*, Direct3DIndexBuffer*, Direct3DTextureBuffer1D*, Direct3DTextureBuffer2D*, Direct3DTextureBuffer3D*, Direct3DTextureBufferCube*, Direct3DTextureBuffer1DArray*, Direct3DTextureBuffer2DArray*, Direct3DTextureBufferCubeArray*, Direct3DTextureSampler1D*, Direct3DTextureSampler2D*, Direct3DTextureSampler3D*, Direct3DTextureSamplerCube*, Direct3DTextureSampler1DArray*, Direct3DTextureSampler2DArray*, Direct3DTextureSamplerCubeArray*, Direct3DFrameBuffer*, Direct3DFrameBufferOutput*, Direct3DStateBuffer*, Direct3DDataArray*, Direct3DTexelArray*, Direct3DDataArrayOutput*, Direct3DTexelArrayOutput*, Direct3DStateBufferLayout*, Direct3DShaderProgram*, Direct3DRenderPassNode*, Direct3DProgramNode*, Direct3DStateGroupNode*, Direct3DRenderableNode*, Direct3DDefaultState*>;
		using MigrateStateObjectTypes = std::variant<Direct3DVertexBuffer*, Direct3DIndexBuffer*, Direct3DTextureBuffer1D*, Direct3DTextureBuffer2D*, Direct3DTextureBuffer3D*, Direct3DTextureBufferCube*, Direct3DTextureBuffer1DArray*, Direct3DTextureBuffer2DArray*, Direct3DTextureBufferCubeArray*, Direct3DTextureSampler1D*, Direct3DTextureSampler2D*, Direct3DTextureSampler3D*, Direct3DTextureSamplerCube*, Direct3DTextureSampler1DArray*, Direct3DTextureSampler2DArray*, Direct3DTextureSamplerCubeArray*, Direct3DFrameBuffer*, Direct3DFrameBufferOutput*, Direct3DStateBuffer*, Direct3DDataArray*, Direct3DTexelArray*, Direct3DDataArrayOutput*, Direct3DTexelArrayOutput*, Direct3DDefaultState*>;
		using BufferUpdateObjectTypes = std::variant<Direct3DVertexBuffer*, Direct3DIndexBuffer*, Direct3DTextureBuffer1D*, Direct3DTextureBuffer2D*, Direct3DTextureBuffer3D*, Direct3DTextureBufferCube*, Direct3DTextureBuffer1DArray*, Direct3DTextureBuffer2DArray*, Direct3DTextureBufferCubeArray*, Direct3DStateBuffer*, Direct3DDataArray*, Direct3DTexelArray*>;
		using BufferTransferObjectTypes = std::variant<Direct3DDataArray*, Direct3DTexelArray*>;

		std::vector<RenderPassEntry> renderPasses;
		std::vector<MigrateStateObjectTypes> migrateStatePendingObjects;
		std::vector<BufferUpdateObjectTypes> bufferUpdatePendingObjects;
		std::vector<BufferTransferObjectTypes> bufferTransferPendingObjects;
		std::vector<TransferBufferAllocation> transferBufferAllocations;
		std::vector<AllObjectTypes> deletePendingObjects;
	};

private:
	// Logging methods
	static const char* DebugMessageCategoryToString(DXGI_INFO_QUEUE_MESSAGE_CATEGORY category);
	static const char* DebugMessageCategoryToString(D3D12_MESSAGE_CATEGORY category);
	void DebugMessageCallbackInternal(DXGI_INFO_QUEUE_MESSAGE_CATEGORY category, DXGI_INFO_QUEUE_MESSAGE_SEVERITY severity, DXGI_INFO_QUEUE_MESSAGE_ID messageId, LPCSTR description) const;
	void DebugMessageCallbackInternal(D3D12_MESSAGE_CATEGORY category, D3D12_MESSAGE_SEVERITY severity, D3D12_MESSAGE_ID messageId, LPCSTR description) const;
	static void CALLBACK DebugMessageCallback(D3D12_MESSAGE_CATEGORY category, D3D12_MESSAGE_SEVERITY severity, D3D12_MESSAGE_ID messageId, LPCSTR description, void* context);
	void ProcessPendingDebugMessages();

	// Render methods
	void VideoMemoryBudgetChangeWorkerThread();
	void RenderThread();
	void EmitBeginEvent(ID3D12GraphicsCommandList* commandList, uint32_t color, const std::wstring& name) const;
	void EmitEndEvent(ID3D12GraphicsCommandList* commandList) const;
	void PerformPrepareBuildOperation();
	void PerformRenderOperation();
	void PerformSwapOperation();
	void PerformDeleteLastDrawResourcesOperation();
	void PerformDeleteNextDrawResourcesOperation();
	void BindTextures(ID3D12GraphicsCommandList* commandList, const std::vector<ITextureBindingInfo*>& bindingEntries, Direct3DShaderProgram* program, bool computeShaderBinding);
	void BindSamplers(ID3D12GraphicsCommandList* commandList, const std::vector<ISamplerBindingInfo*>& bindingEntries, Direct3DShaderProgram* program, bool computeShaderBinding);
	void BindStateBuffers(ID3D12GraphicsCommandList* commandList, const std::vector<StateBufferBindingInfo*>& bindingEntries, Direct3DShaderProgram* program, bool computeShaderBinding);
	void BindResourceArrays(ID3D12GraphicsCommandList* commandList, const std::vector<ResourceArrayBindingInfo*>& bindingEntries, Direct3DShaderProgram* program, bool performReset, bool computeShaderBinding);
	void UnbindTextures(ID3D12GraphicsCommandList* commandList, const std::vector<ITextureBindingInfo*>& bindingEntries, Direct3DShaderProgram* program, bool computeShaderBinding);
	void UnbindSamplers(ID3D12GraphicsCommandList* commandList, const std::vector<ISamplerBindingInfo*>& bindingEntries, Direct3DShaderProgram* program, bool computeShaderBinding);
	void UnbindStateBuffers(ID3D12GraphicsCommandList* commandList, const std::vector<StateBufferBindingInfo*>& bindingEntries, Direct3DShaderProgram* program, bool computeShaderBinding);
	void UnbindResourceArrays(ID3D12GraphicsCommandList* commandList, const std::vector<ResourceArrayBindingInfo*>& bindingEntries, Direct3DShaderProgram* program, bool computeShaderBinding);

	// Resource methods
	void CreatePersistentTransferBuffer(size_t bufferSizeInBytes, Microsoft::WRL::ComPtr<ID3D12Resource>& bufferPointer, D3D12MA::Allocation*& bufferAllocation, bool isUploadBuffer);

private:
	cobalt::logging::ILogger::unique_ptr _log;
	mutable std::mutex _renderThreadMutex;
	mutable std::mutex _buildStateMutex;
	mutable std::mutex _heapAllocationMutex;
	mutable std::mutex _cachedFeatureMutex;
	mutable std::condition_variable _notifyRenderThreadTaskPending;
	mutable std::condition_variable _notifyRenderThreadTaskComplete;
	std::condition_variable _notifyRenderThreadStopped;
	std::condition_variable _notifyVideoMemoryBudgetChangeWorkerStopped;
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
	bool _captureTargetsPresent = false;
	bool _useRenderMarkers = false;
	Microsoft::WRL::ComPtr<ID3D12InfoQueue> _debugInfoQueue;
	Microsoft::WRL::ComPtr<ID3D12InfoQueue1> _debugInfoQueue1;
	Microsoft::WRL::ComPtr<IDXGIInfoQueue> _dxgiInfoQueue;
	DWORD _debugMessageCallbackCookie = 0;
	std::vector<char> _debugMessageBuffer;
	std::vector<Direct3DFrameBuffer*> _boundWindowFramebuffers;
	std::vector<Direct3DFrameBuffer*> _boundTextureFramebuffers;
	std::vector<Direct3DDataArray*> _boundDataArrays;
	std::vector<Direct3DTexelArray*> _boundTexelArrays;
	std::vector<Direct3DFrameBufferOutput*> _capturedFramebufferOutputsInCurrentFrame;
	std::vector<Direct3DDataArrayOutput*> _capturedDataArrayOutputsInCurrentFrame;
	std::vector<Direct3DTexelArrayOutput*> _capturedTexelArrayOutputsInCurrentFrame;
	std::unique_ptr<Direct3DHeapManager> _heapManager;
	std::unique_ptr<Direct3DCommandListPool> _buildCommandListPool;
	std::unique_ptr<D3DX12Residency::ResidencyManager> _residencyManager;
	D3D12MA::Allocator* _memoryManager = nullptr;
	mutable std::mutex _transferBufferMutex;
	Microsoft::WRL::ComPtr<IDXGIFactory4> _dxgiFactory;
	Microsoft::WRL::ComPtr<IDXGIAdapter3> _adapter;
	Microsoft::WRL::ComPtr<ID3D12Device> _device;
	Microsoft::WRL::ComPtr<ID3D12Fence> _buildFence;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> _drawCommandQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> _buildCommandQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> _drawCommandAllocator;
	Microsoft::WRL::ComPtr<ID3D12Fence> _drawFence;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> _drawCommandList;
	D3DX12Residency::ResidencySet* _drawResidencySet = nullptr;
	size_t _totalVerticesLastFrame = 0;
	size_t _videoMemoryBudget = 0;
	HANDLE _buildFenceEvent = INVALID_HANDLE_VALUE;
	HANDLE _drawFenceEvent = INVALID_HANDLE_VALUE;
	HANDLE _videoMemoryBudgetShutdownEvent = INVALID_HANDLE_VALUE;
	bool _videoMemoryBudgetChangeWorkerThreadActive = false;
	mutable bool _cachedMaxShaderModel = false;
	mutable unsigned int _maxShaderModelMajor = 0;
	mutable unsigned int _maxShaderModelMinor = 0;
	std::set<Options> _enabledOptions;
	unsigned int _buildIndex;
	unsigned int _drawIndex;
	MutableState _state[2];
};

} // namespace cobalt::graphics
#include "Direct3DRenderer.inl"
