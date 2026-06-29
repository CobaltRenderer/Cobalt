// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "VulkanFrameBuffer.h"
#include "VulkanHeaders.h"
#include "VulkanNode.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <mutex>
#include <vector>
namespace cobalt::graphics {
class VulkanRenderer;
class VulkanProgramNode;
class VulkanDefaultState;

class VulkanRenderPassNode : public VulkanNode<IRenderPassNode>
{
public:
	// Structures
	struct ChildNodeEntry
	{
		VulkanProgramNode* node;
		VulkanDefaultState* defaultState;
		int frameBufferIndex;
		int defaultStateIndex;
	};

public:
	// Constructors
	VulkanRenderPassNode(cobalt::logging::ILogger* log, VulkanRenderer* renderer);
	~VulkanRenderPassNode();

	// Initialization methods
	void Delete() override;

	// Child node methods
	void AddChildNode(IProgramNode* childNode, IDefaultState* defaultState) override;
	void AddChildNodes(IProgramNode* const* childNodes, size_t childNodeCount, IDefaultState* const* defaultState) override;
	void AddChildNodes(IProgramNode::unique_ptr const* childNodes, size_t childNodeCount, IDefaultState::unique_ptr const* defaultState) override;
	void RemoveChildNode(IProgramNode* childNode) override;
	void RemoveChildNodes(IProgramNode* const* childNodes, size_t childNodeCount) override;
	void RemoveChildNodes(IProgramNode::unique_ptr const* childNodes, size_t childNodeCount) override;
	void RemoveAllChildNodes() override;
	void SetChildNodes(IProgramNode* const* childNodes, size_t childNodeCount, IDefaultState* const* defaultState) override;
	void SetChildNodes(IProgramNode::unique_ptr const* childNodes, size_t childNodeCount, IDefaultState::unique_ptr const* defaultState) override;
	const std::vector<ChildNodeEntry>& GetChildNodes() const;

	// Framebuffer methods
	VulkanFrameBuffer* GetFrameBuffer() const;
	void BindFrameBuffer(IFrameBuffer* frameBuffer) override;
	void SetAttachmentLoadStoreBehavior(IFrameBuffer::AttachmentType type, size_t index, AttachmentLoadBehavior loadBehavior, AttachmentStoreBehavior storeBehavior) override;
	void SetAttachmentClearData(IFrameBuffer::AttachmentType type, size_t index, const V4Float32& data) override;
	void SetAttachmentClearData(IFrameBuffer::AttachmentType type, size_t index, const V4Int32& data) override;
	void SetAttachmentClearData(IFrameBuffer::AttachmentType type, size_t index, const V4UInt32& data) override;
	void RemoveAttachmentClearData(IFrameBuffer::AttachmentType type, size_t index) override;
	void EnableAttachmentMultiSamplingResolution(IFrameBuffer::AttachmentType attachmentType, size_t attachmentIndex, size_t resolveAttachmentIndex) override;
	void DisableAttachmentMultiSamplingResolution(IFrameBuffer::AttachmentType attachmentType, size_t attachmentIndex) override;

	// Enabled state methods
	bool IsEnabled() const;
	void SetIsEnabled(bool state) override;

	// Build state methods
	void MigrateBuildStateToDrawState();
	void ApplyFixedState(VkCommandBuffer commandBuffer);
	void BeginRenderPass(VkCommandBuffer commandBuffer);
	void EndRenderPass(VkCommandBuffer commandBuffer);

	// Debug methods
	void SetDebugName(const Marshal::In<std::string>& name) override;
	const std::string& DebugName() const;

private:
	// Structures
	struct AttachmentState
	{
		V4Float32 clearDataFloat = {};
		V4Int32 clearDataInt = {};
		V4UInt32 clearDataUInt = {};
		VulkanFrameBuffer::AttachmentFormat clearDataFormat = VulkanFrameBuffer::AttachmentFormat::Float;
		IFrameBuffer::AttachmentType type = {};
		size_t index = 0;
		size_t multiSampleResolveTargetIndex = 0;
		AttachmentLoadBehavior loadBehavior = AttachmentLoadBehavior::LoadExistingData;
		AttachmentStoreBehavior storeBehavior = AttachmentStoreBehavior::StoreFinalData;
		IFrameBuffer::AttachmentType multiSampleResolveTargetType = {};
		bool multiSampleResolutionEnabled = false;
		bool clearDataDefined = false;
	};
	struct MutableState
	{
		std::vector<ChildNodeEntry> childNodes;
		std::vector<AttachmentState> attachmentState;
		VulkanFrameBuffer* framebuffer;
		std::string debugName = "RenderPass";
		bool nodeEnabled;
	};

private:
	// Child node methods
	void RemoveChildNodes(const std::unordered_set<IProgramNode*>& childNodes);

	// Framebuffer methods
	void UpdateRenderPassObject();
	void UpdateRenderPassBeginInfo();

private:
	mutable std::mutex _childNodeMutex;
	MutableState _drawState = {};
	MutableState _buildState = {};
	VkRenderPassBeginInfo _renderPassBeginInfo = {};
	std::vector<VkClearValue> _renderTargetClearValues;
	cobalt::logging::ILogger* _log;
	VulkanRenderer* _renderer;
	int _previousFramebufferLastUpdateToken = 0;
	int _previousViewportLastUpdateToken = 0;
	VkRenderPass _renderPass = VK_NULL_HANDLE;
	bool _renderPassObjectCurrent = false;
	bool _renderPassBeginInfoCurrent = false;
};

} // namespace cobalt::graphics
