// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Direct3DFrameBuffer.h"
#include "Direct3DNode.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <mutex>
#include <unordered_set>
#include <vector>
namespace cobalt::graphics {
class Direct3DRenderer;
class Direct3DProgramNode;
class Direct3DDefaultState;

class Direct3DRenderPassNode : public Direct3DNode<IRenderPassNode>
{
public:
	// Structures
	struct ChildNodeEntry
	{
		Direct3DProgramNode* node;
		Direct3DDefaultState* defaultState;
	};

public:
	// Constructors
	Direct3DRenderPassNode(cobalt::logging::ILogger* log, Direct3DRenderer* renderer);

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
	Direct3DFrameBuffer* GetFrameBuffer() const;
	void BindFrameBuffer(IFrameBuffer* frameBuffer) override;
	void SetAttachmentLoadStoreBehavior(IFrameBuffer::AttachmentType type, size_t index, AttachmentLoadBehavior loadBehavior, AttachmentStoreBehavior storeBehavior) override;
	void SetAttachmentClearData(IFrameBuffer::AttachmentType type, size_t index, const V4Float32& data) override;
	void SetAttachmentClearData(IFrameBuffer::AttachmentType type, size_t index, const V4Int32& data) override;
	void SetAttachmentClearData(IFrameBuffer::AttachmentType type, size_t index, const V4UInt32& data) override;
	void RemoveAttachmentClearData(IFrameBuffer::AttachmentType type, size_t index) override;
	void EnableAttachmentMultiSamplingResolution(IFrameBuffer::AttachmentType attachmentType, size_t attachmentIndex, size_t resolveAttachmentIndex) override;
	void DisableAttachmentMultiSamplingResolution(IFrameBuffer::AttachmentType attachmentType, size_t attachmentIndex) override;

	// Enabled state methods
	void SetIsEnabled(bool state) override;
	bool IsEnabled() const;

	// Build state methods
	void MigrateBuildStateToDrawState();
	void ApplyFixedState(ID3D11Device1* device, ID3D11DeviceContext1* context);
	void PerformUnbindOperations(ID3D11DeviceContext1* context);

	// Debug methods
	void SetDebugName(const Marshal::In<std::string>& name) override;
	const std::wstring& DebugName() const;

private:
	// Structures
	struct AttachmentState
	{
		V4Float32 clearData = {};
		size_t index = {};
		size_t multiSampleResolveTargetIndex = 0;
		IFrameBuffer::AttachmentType type = {};
		IFrameBuffer::AttachmentType multiSampleResolveTargetType = {};
		bool clearDataDefined = false;
		bool multiSampleResolutionEnabled = false;
	};
	struct MutableState
	{
		std::vector<ChildNodeEntry> childNodes;
		std::vector<AttachmentState> attachmentState;
		Direct3DFrameBuffer* framebuffer = {};
		std::wstring debugName = L"RenderPass";
		bool nodeEnabled = false;
	};

private:
	// Child node methods
	void RemoveChildNodes(const std::unordered_set<IProgramNode*>& childNodes);

private:
	mutable std::mutex _childNodeMutex;
	MutableState _drawState;
	MutableState _buildState;
	cobalt::logging::ILogger* _log;
	Direct3DRenderer* _renderer;
	bool _hasMultiSampleResolveTargets = false;
};

} // namespace cobalt::graphics
