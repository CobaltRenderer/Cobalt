// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "OpenGLFrameBuffer.h"
#include "OpenGLNode.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <mutex>
#include <vector>
namespace cobalt::graphics {
class OpenGLRenderer;
class OpenGLProgramNode;
class OpenGLDefaultState;

class OpenGLRenderPassNode : public OpenGLNode<IRenderPassNode>
{
public:
	// Structures
	struct ChildNodeEntry
	{
		OpenGLProgramNode* node;
		OpenGLDefaultState* defaultState;
	};

public:
	// Constructors
	OpenGLRenderPassNode(cobalt::logging::ILogger* log, OpenGLRenderer* renderer);

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
	OpenGLFrameBuffer* GetFrameBuffer() const;
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
	void ApplyFixedState(OpenGLFrameBuffer* frameBuffer);
	void PerformUnbindOperations();

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
		size_t index = {};
		size_t multiSampleResolveTargetIndex = 0;
		IFrameBuffer::AttachmentType type = {};
		GLenum nativeBufferType = {};
		GLuint nativeBufferIndex = {};
		OpenGLFrameBuffer::AttachmentFormat clearDataFormat = OpenGLFrameBuffer::AttachmentFormat::Float;
		OpenGLFrameBuffer::AttachmentFormat actualFormat = OpenGLFrameBuffer::AttachmentFormat::Float;
		IFrameBuffer::AttachmentType multiSampleResolveTargetType = {};
		bool clearDataDefined = false;
		bool clearDataTypeMatched = false;
		bool multiSampleResolutionEnabled = false;
	};
	struct MutableState
	{
		std::vector<ChildNodeEntry> childNodes;
		std::vector<AttachmentState> attachmentState;
		OpenGLFrameBuffer* framebuffer = {};
		std::string debugName = "RenderPass";
		bool nodeEnabled = false;
	};

private:
	// Child node methods
	void RemoveChildNodes(const std::unordered_set<IProgramNode*>& childNodes);

	// Framebuffer methods
	constexpr static GLenum AttachmentTypeToBufferType(IFrameBuffer::AttachmentType type);
	AttachmentState& RetrieveOrCreateAttachmentState(IFrameBuffer::AttachmentType type, size_t index);

private:
	mutable std::mutex _childNodeMutex;
	MutableState _drawState;
	MutableState _buildState;
	cobalt::logging::ILogger* _log;
	OpenGLRenderer* _renderer;
	bool _hasMultiSampleResolveTargets = false;
};

} // namespace cobalt::graphics
