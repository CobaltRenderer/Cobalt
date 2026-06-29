// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "VulkanFrameBuffer.h"
#include "VulkanHeaders.h"
#include "VulkanShaderProgram.h"
#include "VulkanStateContainer.h"
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <mutex>
#include <vector>
namespace cobalt::graphics {
class VulkanRenderer;
class VulkanRenderableNode;

class VulkanStateGroupNode : public VulkanStateContainer<IStateGroupNode>
{
public:
	// Constructors
	VulkanStateGroupNode(cobalt::logging::ILogger* log, VulkanRenderer* renderer);
	~VulkanStateGroupNode();

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
	const std::vector<VulkanRenderableNode*>& GetChildNodes(size_t stateBucketIndex) const;

	// Compute methods
	bool HasComputeTask() const;
	V3UInt32 GetComputeThreadGroupCounts() const;
	void SetComputeTask(const V3UInt32& threadGroupCounts) override;
	void RemoveComputeTask() override;

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
	VkPipelineLayout GetPipelineLayout(int stateBucketIndex, int frameBufferIndex);

	// Build state methods
	int GetStateBucketCount() const;
	void SetFrameBufferCount(int count);
	void ClearFrameBufferEntry(int frameBufferIndex);
	void ClearAllFrameBufferEntries();
	void MigrateBuildStateToDrawState();
	void ApplyFixedState(int stateBucketIndex, int frameBufferIndex, VkCommandBuffer& commandBuffer, VulkanShaderProgram* shaderProgram, VulkanFrameBuffer* framebuffer);

	// Render methods
	VulkanShaderProgram::GlobalConstantBufferBindingInfo& GetGlobalConstantBufferBindingInfo(int constantStateIndex);

	// Debug methods
	void SetDebugName(const Marshal::In<std::string>& name) override;
	const std::string& DebugName() const;

private:
	// Structures
	struct BlendState
	{
		IFrameBuffer::AttachmentType type;
		size_t index;
		BlendOperation blendOperationRGB;
		BlendFactor blendFactorSourceRGB;
		BlendFactor blendFactorDestinationRGB;
		BlendOperation blendOperationA;
		BlendFactor blendFactorSourceA;
		BlendFactor blendFactorDestinationA;
		VkBlendOp nativeBlendOperationRGB;
		VkBlendFactor nativeBlendFactorSourceRGB;
		VkBlendFactor nativeBlendFactorDestinationRGB;
		VkBlendOp nativeBlendOperationA;
		VkBlendFactor nativeBlendFactorSourceA;
		VkBlendFactor nativeBlendFactorDestinationA;
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

		std::vector<VkFormat> attributeDataType;
		std::vector<size_t> attributeStrideInBytes;
		std::vector<VulkanRenderableNode*> drawChildNodes;
		std::vector<VulkanRenderableNode*> buildChildNodes;
		std::vector<uint32_t> removedChildNodeIDs;

		// Note that this is logically std::vector<bool>, but we want to avoid the std::vector<bool> specialization.
		std::vector<uint8_t> drawNativeObjectsCurrentForBucket;

		std::vector<VkPipeline> pipelineStateObjects;
		std::vector<VkPipelineLayout> pipelineLayoutObjects;
		std::vector<int> viewportLastUpdateTokens;
		std::vector<int> framebufferLastUpdateTokens;
		std::vector<VkBuffer> nullVertexBufferBindings;
		std::vector<VkDeviceSize> nullVertexBufferOffsets;

		VkPrimitiveTopology primitiveTopology = {};
		bool primitiveRestartEnabled = false;
		bool hasModifiedChildNodes = false;
		bool buildNativeObjectsCurrentForBucket = false;
	};
	struct MutableState
	{
		BlendState sharedBlendState;
		StencilFaceState stencilFrontFace = {};
		StencilFaceState stencilBackFace = {};
		V3UInt32 computeThreadGroupCounts = {};
		std::vector<BlendState> attachmentTypeBlendState;
		size_t allocatedFrameBufferSlotCount = 0;
		float depthBiasConstantFactor;
		float depthBiasSlopeFactor;
		float depthBiasClamp;
		DepthComparisonFunction depthComparisonTest;
		PolygonFillMode polygonFillMode;
		PolygonCullMode polygonCullMode;
		PolygonWindingOrder polygonWindingOrder;
		uint32_t stencilCompareMask;
		uint32_t stencilWriteMask;
		uint32_t stencilReferenceValue;
		std::string debugName = "StateGroup";
		bool depthTestEnabled;
		bool depthWriteEnabled;
		bool depthBiasEnabled;
		bool blendEnabled;
		bool stencilEnabled = false;
		bool computeTaskDefined = false;
	};

private:
	// Child node methods
	bool RemoveChildNode(VulkanRenderableNode* childNode, uint32_t childNodeID);

	// State methods
	bool CreateNativeComputePipelineState(PipelineStateBucket& stateBucket, VulkanShaderProgram* shaderProgram);
	bool CreateNativeGraphicsPipelineState(PipelineStateBucket& stateBucket, int frameBufferIndex, VulkanShaderProgram* shaderProgram, VulkanFrameBuffer* framebuffer);
	void DeleteNativePipelineState(PipelineStateBucket& stateBucket, int frameBufferIndex);
	PipelineStateBucket& GetMatchingStateBucket(VkPrimitiveTopology primitiveTopology, bool primitiveRestartEnabled, const std::vector<int>& vertexAttributeIDs, const std::vector<int>& instanceAttributeIDs, const std::vector<VkFormat>& attributeDataTypes, const std::vector<size_t>& attributeStrideInBytes);
	static PipelineStateBucket* GetMatchingStateBucketInSet(std::vector<PipelineStateBucket>& stateBucketSet, VkPrimitiveTopology primitiveTopology, bool primitiveRestartEnabled, const std::vector<int>& vertexAttributeIDs, const std::vector<int>& instanceAttributeIDs, const std::vector<VkFormat>& attributeDataTypes, const std::vector<size_t>& attributeStrideInBytes);

	// Build state methods
	static BlendState BuildBlendState(BlendOperation blendOperationRGB, BlendFactor blendFactorSourceRGB, BlendFactor blendFactorDestinationRGB, BlendOperation blendOperationA, BlendFactor blendFactorSourceA, BlendFactor blendFactorDestinationA);
	static StencilFaceState BuildStencilFaceState(StencilComparisonFunction comparisonTest, StencilOperation passOperation, StencilOperation failOperation, StencilOperation depthFailOperation);
	constexpr static VkBlendOp GetNativeBlendOperation(BlendOperation operation);
	constexpr static VkBlendFactor GetNativeBlendFactor(BlendFactor factor);
	constexpr static VkCompareOp GetNativeDepthComparisonFunction(DepthComparisonFunction function);
	constexpr static VkCompareOp GetNativeStencilComparisonFunction(StencilComparisonFunction function);
	constexpr static VkStencilOp GetNativeStencilOperation(StencilOperation operation);

private:
	mutable std::mutex _childNodeMutex;
	MutableState _drawState = {};
	MutableState _buildState = {};
	VulkanShaderProgram::GlobalConstantBufferBindingInfo _globalConstantBufferBindingInfo = {};
	std::vector<VulkanShaderProgram::GlobalConstantBufferBindingInfo> _globalConstantBufferBindingInfoWithDefaults;
	std::vector<PipelineStateBucket> _pipelineStateBuckets;
	std::vector<PipelineStateBucket> _newPipelineStateBuckets;
	PipelineStateBucket _computePipelineStateBucket;
	std::vector<int> _invalidatedFrameBufferEntries;
	cobalt::logging::ILogger* _log;
	VulkanRenderer* _renderer;
	int _globalConstantBufferBindingInfoWithDefaultsSize = 0;
	bool _buildStateChanged;
};

} // namespace cobalt::graphics
