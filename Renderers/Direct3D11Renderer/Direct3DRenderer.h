// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "BindingHelpers.h"
#include "Direct3DHeaders.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <dxgi1_5.h>
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
	Direct3DRenderer(cobalt::logging::ILogger::unique_ptr log, Microsoft::WRL::ComPtr<IDXGIFactory2> dxgiFactory, Microsoft::WRL::ComPtr<IDXGIAdapter2> adapter, const Marshal::In<std::set<IGraphicsDevice::Feature>>& enabledFeatures, const Marshal::In<std::set<Options>>& enabledOptions);

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
	ID3D11Device1* GetDevice() const;

	// Settings methods
	bool DebugLoggingEnabled() const;
	bool UseDeferredBufferCreation() const;
	bool UseLegacyRenderingMethod() const;

	// Feature methods
	bool IsFeaturePresent(DXGI_FEATURE feature) const;

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

		Direct3DRenderPassNode* renderPassNode;
		int sortIndex;
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
		std::vector<AllObjectTypes> deletePendingObjects;
	};

private:
	// Logging methods
	static const char* DebugMessageCategoryToString(DXGI_INFO_QUEUE_MESSAGE_CATEGORY category);
	static const char* DebugMessageCategoryToString(D3D11_MESSAGE_CATEGORY category);
	void DebugMessageCallbackInternal(DXGI_INFO_QUEUE_MESSAGE_CATEGORY category, DXGI_INFO_QUEUE_MESSAGE_SEVERITY severity, DXGI_INFO_QUEUE_MESSAGE_ID messageId, LPCSTR description) const;
	void DebugMessageCallbackInternal(D3D11_MESSAGE_CATEGORY category, D3D11_MESSAGE_SEVERITY severity, D3D11_MESSAGE_ID messageId, LPCSTR description) const;
	void ProcessPendingDebugMessages();

	// Render methods
	void RenderThread();
	void PerformPrepareBuildOperation();
	void PerformRenderOperation();
	void PerformLegacyRenderOperation();
	void PerformSwapOperation();
	void PerformDeleteLastDrawResourcesOperation();
	void PerformDeleteNextDrawResourcesOperation();
	void BindTextures(const std::vector<ITextureBindingInfo*>& bindingEntries, ID3D11Device1* device, ID3D11DeviceContext1* context, Direct3DShaderProgram* program);
	void BindSamplers(const std::vector<ISamplerBindingInfo*>& bindingEntries, ID3D11Device1* device, ID3D11DeviceContext1* context, Direct3DShaderProgram* program);
	void BindStateBuffers(const std::vector<StateBufferBindingInfo*>& bindingEntries, ID3D11Device1* device, ID3D11DeviceContext1* context, Direct3DShaderProgram* program);
	void BindResourceArrays(const std::vector<ResourceArrayBindingInfo*>* defaultBindingEntries, const std::vector<ResourceArrayBindingInfo*>* groupNodeBindingEntries, const std::vector<ResourceArrayBindingInfo*>* renderableBindingEntries, ID3D11Device1* device, ID3D11DeviceContext1* context, Direct3DShaderProgram* program, bool resetCountersForDefaultBindingEntries, bool resetCountersForGroupBindingEntries, bool resetCountersForRenderableBindingEntries, size_t boundRenderTargetCount, bool computeShaderBinding);
	void UpdateResourceBufferEntries(const std::vector<ResourceArrayBindingInfo*>& bindingEntries, ID3D11Device1* device, ID3D11DeviceContext1* context, Direct3DShaderProgram* program, bool performReset, std::vector<ID3D11UnorderedAccessView*>& resourceBufferViews, std::vector<UINT>& resetValues, UINT& lowestBindPoint);
	void UnbindTextures(const std::vector<ITextureBindingInfo*>& bindingEntries, ID3D11Device1* device, ID3D11DeviceContext1* context, Direct3DShaderProgram* program);
	void UnbindSamplers(const std::vector<ISamplerBindingInfo*>& bindingEntries, ID3D11Device1* device, ID3D11DeviceContext1* context, Direct3DShaderProgram* program);
	void UnbindStateBuffers(const std::vector<StateBufferBindingInfo*>& bindingEntries, ID3D11Device1* device, ID3D11DeviceContext1* context, Direct3DShaderProgram* program);

private:
	cobalt::logging::ILogger::unique_ptr _log;
	mutable std::mutex _renderThreadMutex;
	mutable std::mutex _buildStateMutex;
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
	Microsoft::WRL::ComPtr<ID3D11InfoQueue> _debugInfoQueue;
	Microsoft::WRL::ComPtr<IDXGIInfoQueue> _dxgiInfoQueue;
	std::vector<char> _debugMessageBuffer;
	std::vector<Direct3DFrameBuffer*> _boundWindowFramebuffers;
	std::vector<Direct3DFrameBuffer*> _boundTextureFramebuffers;
	std::vector<Direct3DDataArray*> _boundDataArrays;
	std::vector<Direct3DTexelArray*> _boundTexelArrays;
	std::vector<Direct3DFrameBufferOutput*> _capturedFramebufferOutputsInCurrentFrame;
	std::vector<Direct3DDataArrayOutput*> _capturedDataArrayOutputsInCurrentFrame;
	std::vector<Direct3DTexelArrayOutput*> _capturedTexelArrayOutputsInCurrentFrame;
	std::vector<UINT> _resourceBufferResetValues;
	std::vector<ID3D11UnorderedAccessView*> _resourceBufferViews;
	Microsoft::WRL::ComPtr<IDXGIFactory2> _dxgiFactory;
	Microsoft::WRL::ComPtr<IDXGIAdapter2> _adapter;
	Microsoft::WRL::ComPtr<ID3D11Device1> _device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext1> _deviceContext;
	Microsoft::WRL::ComPtr<ID3DUserDefinedAnnotation> _renderAnnotation;
	std::set<Options> _enabledOptions;
	uint32_t _buildIndex;
	uint32_t _drawIndex;
	MutableState _state[2];
};

} // namespace cobalt::graphics
#include "Direct3DRenderer.inl"
