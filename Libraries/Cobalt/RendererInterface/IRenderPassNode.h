// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Deleter.h"
#include "IDefaultState.h"
#include "IFrameBuffer.h"
#include "IProgramNode.h"
#include "VectorTypes.h"
#include <Cobalt/Marshalling/Marshalling.pkg>
#include <limits>
#include <memory>
namespace cobalt { namespace graphics {
using namespace cobalt::marshalling::operators;
class IProgramNode;

class IRenderPassNode
{
public:
	// Enumerations
	enum class AttachmentLoadBehavior
	{
		LoadExistingData,
		UndefinedInitialData,
	};
	enum class AttachmentStoreBehavior
	{
		StoreFinalData,
		UndefinedFinalData,
	};

	// Typedefs
	typedef std::unique_ptr<IRenderPassNode, Deleter<IRenderPassNode>> unique_ptr;

public:
	// Initialization methods
	virtual void Delete() = 0;

	// Child node methods
	virtual void AddChildNode(IProgramNode* childNode, IDefaultState* defaultState = nullptr) = 0;
	virtual void AddChildNodes(IProgramNode* const* childNodes, size_t childNodeCount, IDefaultState* const* defaultState = nullptr) = 0;
	virtual void AddChildNodes(IProgramNode::unique_ptr const* childNodes, size_t childNodeCount, IDefaultState::unique_ptr const* defaultState = nullptr) = 0;
	virtual void RemoveChildNode(IProgramNode* childNode) = 0;
	virtual void RemoveChildNodes(IProgramNode* const* childNodes, size_t childNodeCount) = 0;
	virtual void RemoveChildNodes(IProgramNode::unique_ptr const* childNodes, size_t childNodeCount) = 0;
	virtual void RemoveAllChildNodes() = 0;
	virtual void SetChildNodes(IProgramNode* const* childNodes, size_t childNodeCount, IDefaultState* const* defaultState = nullptr) = 0;
	virtual void SetChildNodes(IProgramNode::unique_ptr const* childNodes, size_t childNodeCount, IDefaultState::unique_ptr const* defaultState = nullptr) = 0;

	// Framebuffer methods
	virtual void BindFrameBuffer(IFrameBuffer* frameBuffer) = 0;
	virtual void SetAttachmentLoadStoreBehavior(IFrameBuffer::AttachmentType type, size_t index, AttachmentLoadBehavior loadBehavior, AttachmentStoreBehavior storeBehavior) = 0;
	virtual void SetAttachmentClearData(IFrameBuffer::AttachmentType type, size_t index, const V4Float32& data) = 0;
	virtual void SetAttachmentClearData(IFrameBuffer::AttachmentType type, size_t index, const V4Int32& data) = 0;
	virtual void SetAttachmentClearData(IFrameBuffer::AttachmentType type, size_t index, const V4UInt32& data) = 0;
	virtual void RemoveAttachmentClearData(IFrameBuffer::AttachmentType type, size_t index) = 0;
	virtual void EnableAttachmentMultiSamplingResolution(IFrameBuffer::AttachmentType attachmentType, size_t attachmentIndex, size_t resolveAttachmentIndex = std::numeric_limits<size_t>::max()) = 0;
	virtual void DisableAttachmentMultiSamplingResolution(IFrameBuffer::AttachmentType attachmentType, size_t attachmentIndex) = 0;

	// Enabled state methods
	virtual void SetIsEnabled(bool state) = 0;

	// Debug methods
	virtual void SetDebugName(const Marshal::In<std::string>& name) = 0;

protected:
	// Constructors
	~IRenderPassNode() = default;
};

}} // namespace cobalt::graphics
