// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Direct3DFrameBuffer.h"
#include "Direct3DHeaders.h"
#include "Direct3DShaderProgram.h"
#include "Direct3DStateContainer.h"
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <mutex>
#include <vector>
namespace cobalt::graphics {
class Direct3DRenderer;
class Direct3DRenderableNode;

class Direct3DStateGroupNode : public Direct3DStateContainer<IStateGroupNode>
{
public:
	// Constructors
	Direct3DStateGroupNode(cobalt::logging::ILogger* log, Direct3DRenderer* renderer);

	// Initialization methods
	void Delete() override;

	// Child node methods
	void AddChildNode(IRenderableNode* childNode) override;
	void AddChildNodes(IRenderableNode* const* childNodes, size_t childNodeCount) override;
	void AddChildNodes(IRenderableNode::unique_ptr const* childNodes, size_t childNodeCount) override;
	void RemoveChildNode(IRenderableNode* childNode) override;
	void RemoveChildNodes(IRenderableNode* const* childNodes, size_t childNodeCount) override;
	void RemoveChildNodes(IRenderableNode::unique_ptr const* childNodes, size_t childNodeCount) override;
	void RemoveAllChildNodes() override;
	void SetChildNodes(IRenderableNode* const* childNodes, size_t childNodeCount) override;
	void SetChildNodes(IRenderableNode::unique_ptr const* childNodes, size_t childNodeCount) override;
	const std::vector<Direct3DRenderableNode*>& GetChildNodes(size_t stateBucketIndex) const;

	// Compute methods
	bool HasComputeTask() const;
	V3UInt32 GetComputeThreadGroupCounts() const;
	void SetComputeTask(const V3UInt32& threadGroupCounts) override;
	void RemoveComputeTask() override;
	Direct3DShaderProgram::GlobalConstantBufferBindingInfo& GetGlobalConstantBufferBindingInfo();

	// Depth state methods
	void SetDepthTestEnabled(bool state) override;
	void SetDepthWriteEnabled(bool state) override;
	void SetDepthComparisonFunction(DepthComparisonFunction comparisonTest) override;
	void SetDepthBias(float constantFactor, float slopeFactor, float clamp) override;
	void ClearDepthBias() override;

	// Stencil state methods
	void SetStencilTestEnabled(bool state, uint32_t compareMask, uint32_t writeMask) override;
	void SetStencilOperation(StencilTargetFace targetFace, StencilComparisonFunction comparisonTest, StencilOperation passOperation, StencilOperation failOperation, StencilOperation depthFailOperation) override;
	void SetStencilReferenceValue(uint32_t referenceValue) override;

	// Rasterization state methods
	void SetPolygonFillMode(PolygonFillMode fillMode) override;
	void SetPolygonCullMode(PolygonCullMode cullMode) override;
	void SetPolygonWindingOrder(PolygonWindingOrder windingOrder) override;

	// Blend state methods
	void SetBlendEnabled(bool state) override;
	void SetBlendMode(BlendOperation blendOperationRGB, BlendFactor blendFactorSourceRGB, BlendFactor blendFactorDestinationRGB, BlendOperation blendOperationA, BlendFactor blendFactorSourceA, BlendFactor blendFactorDestinationA) override;
	void SetBlendMode(IFrameBuffer::AttachmentType type, size_t index, BlendOperation blendOperationRGB, BlendFactor blendFactorSourceRGB, BlendFactor blendFactorDestinationRGB, BlendOperation blendOperationA, BlendFactor blendFactorSourceA, BlendFactor blendFactorDestinationA) override;

	// Build state methods
	size_t GetStateBucketCount() const;
	void SetFrameBufferCount(int count);
	void ClearFrameBufferEntry(int frameBufferIndex);
	void ClearAllFrameBufferEntries();
	void MigrateBuildStateToDrawState();
	void ApplyFixedState(size_t stateBucketIndex, int frameBufferIndex, ID3D12GraphicsCommandList* commandList, Direct3DShaderProgram* shaderProgram, Direct3DFrameBuffer* framebuffer);

	// Debug methods
	void SetDebugName(const Marshal::In<std::string>& name) override;
	const std::wstring& DebugName() const;

private:
	// Structures
	struct BlendState
	{
		size_t index;
		IFrameBuffer::AttachmentType type;
		BlendOperation blendOperationRGB;
		BlendFactor blendFactorSourceRGB;
		BlendFactor blendFactorDestinationRGB;
		BlendOperation blendOperationA;
		BlendFactor blendFactorSourceA;
		BlendFactor blendFactorDestinationA;
		D3D12_BLEND_OP nativeBlendOperationRGB;
		D3D12_BLEND nativeBlendFactorSourceRGB;
		D3D12_BLEND nativeBlendFactorDestinationRGB;
		D3D12_BLEND_OP nativeBlendOperationA;
		D3D12_BLEND nativeBlendFactorSourceA;
		D3D12_BLEND nativeBlendFactorDestinationA;
	};
	struct StencilFaceState
	{
		StencilComparisonFunction comparisonTest;
		StencilOperation passOperation;
		StencilOperation failOperation;
		StencilOperation depthFailOperation;
	};
	struct PipelineStateBucket
	{
		// Note that these are logically std::vector<bool>, but we want to avoid the std::vector<bool> specialization.
		std::vector<uint8_t> attributeTypeKnown;
		std::vector<uint8_t> attributeIsInstanced;

		std::vector<DXGI_FORMAT> attributeDataType;
		std::vector<Direct3DRenderableNode*> drawChildNodes;
		std::vector<Direct3DRenderableNode*> buildChildNodes;
		std::vector<uint32_t> removedChildNodeIDs;
		std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementDescriptions;

		// Note that this is logically std::vector<bool>, but we want to avoid the std::vector<bool> specialization.
		std::vector<uint8_t> drawNativeObjectsCurrentForBucket;
		std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC> pipelineStateDescriptions;
		std::vector<Microsoft::WRL::ComPtr<ID3D12PipelineState>> pipelineStateObjects;
		std::vector<int> framebufferLastUpdateTokens;

		D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopology = {};
		D3D12_INDEX_BUFFER_STRIP_CUT_VALUE stripCutValue = {};
		bool hasModifiedChildNodes = false;
		bool buildNativeObjectsCurrentForBucket = false;
	};
	struct MutableState
	{
		BlendState sharedBlendState = {};
		StencilFaceState stencilFrontFace = {};
		StencilFaceState stencilBackFace = {};
		V3UInt32 computeThreadGroupCounts = {};
		std::vector<BlendState> attachmentTypeBlendState;
		size_t allocatedFrameBufferSlotCount = 0;
		float depthBiasConstantFactor = 0.0f;
		float depthBiasSlopeFactor = 0.0f;
		float depthBiasClamp = 0.0f;
		DepthComparisonFunction depthComparisonTest = {};
		PolygonFillMode polygonFillMode = {};
		PolygonCullMode polygonCullMode = {};
		PolygonWindingOrder polygonWindingOrder = {};
		uint32_t stencilCompareMask = 0;
		uint32_t stencilWriteMask = 0;
		uint32_t stencilReferenceValue = 0;
		std::wstring debugName = L"StateGroup";
		bool depthTestEnabled = false;
		bool depthWriteEnabled = false;
		bool depthBiasEnabled = false;
		bool blendEnabled = false;
		bool stencilEnabled = false;
		bool computeTaskDefined = false;
	};

private:
	// Child node methods
	bool RemoveChildNode(Direct3DRenderableNode* childNode, uint32_t childNodeID);

	// Build state methods
	PipelineStateBucket& GetMatchingStateBucket(D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopology, D3D12_INDEX_BUFFER_STRIP_CUT_VALUE stripCutValue, const std::vector<VertexAttributeId>& vertexAttributeIDs, const std::vector<VertexAttributeId>& instanceAttributeIDs, const std::vector<DXGI_FORMAT>& attributeDataTypes);
	static PipelineStateBucket* GetMatchingStateBucketInSet(std::vector<PipelineStateBucket>& stateBucketSet, D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopology, D3D12_INDEX_BUFFER_STRIP_CUT_VALUE stripCutValue, const std::vector<VertexAttributeId>& vertexAttributeIDs, const std::vector<VertexAttributeId>& instanceAttributeIDs, const std::vector<DXGI_FORMAT>& attributeDataTypes);
	bool CreateNativeComputePipelineState(PipelineStateBucket& stateBucket, Direct3DShaderProgram* shaderProgram);
	bool CreateNativeGraphicsPipelineState(PipelineStateBucket& stateBucket, int frameBufferIndex, Direct3DShaderProgram* shaderProgram, Direct3DFrameBuffer* framebuffer);
	void CreateNativeInputLayoutDescription(Direct3DShaderProgram* shaderProgram, D3D12_INPUT_LAYOUT_DESC& inputLayoutDescription, std::vector<D3D12_INPUT_ELEMENT_DESC>& inputElementDescriptions, const std::vector<uint8_t>& attributeTypeKnown, const std::vector<uint8_t>& attributeIsInstanced, const std::vector<DXGI_FORMAT>& attributeDataType) const;
	void CreateNativeRasterizerDescription(D3D12_RASTERIZER_DESC& rasterizerDesc) const;
	void CreateNativeBlendDescription(D3D12_BLEND_DESC& desc) const;
	void CreateNativeDepthStencilDescription(D3D12_DEPTH_STENCIL_DESC& depthStencilDescription) const;
	static BlendState BuildBlendState(BlendOperation blendOperationRGB, BlendFactor blendFactorSourceRGB, BlendFactor blendFactorDestinationRGB, BlendOperation blendOperationA, BlendFactor blendFactorSourceA, BlendFactor blendFactorDestinationA);
	static StencilFaceState BuildStencilFaceState(StencilComparisonFunction comparisonTest, StencilOperation passOperation, StencilOperation failOperation, StencilOperation depthFailOperation);
	constexpr static D3D12_BLEND_OP GetNativeBlendOperation(BlendOperation operation);
	constexpr static D3D12_BLEND GetNativeBlendFactor(BlendFactor factor, BlendOperation operation);
	constexpr static D3D12_BLEND GetNativeAlphaBlendFactor(BlendFactor factor, BlendOperation operation);
	constexpr static D3D12_COMPARISON_FUNC GetNativeDepthComparisonFunction(DepthComparisonFunction function);
	constexpr static D3D12_COMPARISON_FUNC GetNativeStencilComparisonFunction(StencilComparisonFunction function);
	constexpr static D3D12_STENCIL_OP GetNativeStencilOperation(StencilOperation operation);

private:
	mutable std::mutex _childNodeMutex;
	MutableState _drawState;
	MutableState _buildState;
	Direct3DShaderProgram::GlobalConstantBufferBindingInfo _globalConstantBufferBindingInfo = {};
	std::vector<PipelineStateBucket> _pipelineStateBuckets;
	std::vector<PipelineStateBucket> _newPipelineStateBuckets;
	PipelineStateBucket _computePipelineStateBucket;
	std::vector<int> _invalidatedFrameBufferEntries;
	cobalt::logging::ILogger* _log;
	Direct3DRenderer* _renderer;
	bool _buildStateChanged;
};

} // namespace cobalt::graphics
