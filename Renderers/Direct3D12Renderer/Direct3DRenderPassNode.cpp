// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "Direct3DRenderPassNode.h"
#include "Direct3DDefaultState.h"
#include "Direct3DFrameBuffer.h"
#include "Direct3DProgramNode.h"
#include "Direct3DRenderer.h"
#include <Internal/RendererSupport/KnownDynamicCast.h>
#include <Internal/RendererSupport/UnicodeConversion.h>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
Direct3DRenderPassNode::Direct3DRenderPassNode(cobalt::logging::ILogger* log, Direct3DRenderer* renderer)
: _renderer(renderer), _log(log)
{
	_buildState.framebuffer = nullptr;
	_buildState.nodeEnabled = true;
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void Direct3DRenderPassNode::Delete()
{
	_renderer->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
// Child node methods
//----------------------------------------------------------------------------------------
void Direct3DRenderPassNode::AddChildNode(IProgramNode* childNode, IDefaultState* defaultState)
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	ChildNodeEntry entry{};
	entry.node = KnownDynamicCast<Direct3DProgramNode*>(childNode);
	entry.defaultState = KnownDynamicCast<Direct3DDefaultState*>(defaultState);
	entry.frameBufferIndex = entry.node->UpdateFrameBufferAssociation(-1, _buildState.framebuffer);
	_buildState.childNodes.push_back(entry);
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void Direct3DRenderPassNode::AddChildNodes(IProgramNode* const* childNodes, size_t childNodeCount, IDefaultState* const* defaultState)
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	size_t currentChildCount = _buildState.childNodes.size();
	size_t newChildCount = currentChildCount + childNodeCount;
	_buildState.childNodes.resize(newChildCount);
	if (defaultState != nullptr)
	{
		for (size_t i = currentChildCount; i < newChildCount; ++i)
		{
			auto& entry = _buildState.childNodes[i];
			entry.node = KnownDynamicCast<Direct3DProgramNode*>(*(childNodes++));
			entry.defaultState = KnownDynamicCast<Direct3DDefaultState*>(*(defaultState++));
			entry.frameBufferIndex = entry.node->UpdateFrameBufferAssociation(-1, _buildState.framebuffer);
		}
	}
	else
	{
		for (size_t i = currentChildCount; i < newChildCount; ++i)
		{
			auto& entry = _buildState.childNodes[i];
			entry.node = KnownDynamicCast<Direct3DProgramNode*>(*(childNodes++));
			entry.defaultState = nullptr;
			entry.frameBufferIndex = entry.node->UpdateFrameBufferAssociation(-1, _buildState.framebuffer);
		}
	}
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void Direct3DRenderPassNode::AddChildNodes(IProgramNode::unique_ptr const* childNodes, size_t childNodeCount, IDefaultState::unique_ptr const* defaultState)
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	size_t currentChildCount = _buildState.childNodes.size();
	size_t newChildCount = currentChildCount + childNodeCount;
	_buildState.childNodes.resize(newChildCount);
	if (defaultState != nullptr)
	{
		for (size_t i = currentChildCount; i < newChildCount; ++i)
		{
			auto& entry = _buildState.childNodes[i];
			entry.node = KnownDynamicCast<Direct3DProgramNode*>((childNodes++)->get());
			entry.defaultState = KnownDynamicCast<Direct3DDefaultState*>((defaultState++)->get());
			entry.frameBufferIndex = entry.node->UpdateFrameBufferAssociation(-1, _buildState.framebuffer);
		}
	}
	else
	{
		for (size_t i = currentChildCount; i < newChildCount; ++i)
		{
			auto& entry = _buildState.childNodes[i];
			entry.node = KnownDynamicCast<Direct3DProgramNode*>((childNodes++)->get());
			entry.defaultState = nullptr;
			entry.frameBufferIndex = entry.node->UpdateFrameBufferAssociation(-1, _buildState.framebuffer);
		}
	}
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void Direct3DRenderPassNode::RemoveChildNode(IProgramNode* childNode)
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	for (auto i = _buildState.childNodes.begin(); i != _buildState.childNodes.end(); ++i)
	{
		if (i->node == childNode)
		{
			i->node->UpdateFrameBufferAssociation(i->frameBufferIndex, nullptr);
			_buildState.childNodes.erase(i);
			lock.unlock();
			FlagDrawStateNotCurrent();
			return;
		}
	}
}

//----------------------------------------------------------------------------------------
void Direct3DRenderPassNode::RemoveChildNodes(IProgramNode* const* childNodes, size_t childNodeCount)
{
	std::unordered_set<IProgramNode*> nodesToRemove;
	for (size_t i = 0; i < childNodeCount; ++i)
	{
		nodesToRemove.insert(*(childNodes++));
	}
	RemoveChildNodes(nodesToRemove);
}

//----------------------------------------------------------------------------------------
void Direct3DRenderPassNode::RemoveChildNodes(IProgramNode::unique_ptr const* childNodes, size_t childNodeCount)
{
	std::unordered_set<IProgramNode*> nodesToRemove;
	for (size_t i = 0; i < childNodeCount; ++i)
	{
		nodesToRemove.insert((childNodes++)->get());
	}
	RemoveChildNodes(nodesToRemove);
}

//----------------------------------------------------------------------------------------
void Direct3DRenderPassNode::RemoveChildNodes(const std::unordered_set<IProgramNode*>& childNodes)
{
	// Skip leading entries in the child array until we find the first entry to remove. This helps improve performance
	// when the elements to remove are towards the end of the child array.
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	size_t currentChildCount = _buildState.childNodes.size();
	size_t readIndex = 0;
	while ((readIndex < currentChildCount) && childNodes.find(_buildState.childNodes[readIndex].node) == childNodes.end())
	{
		++readIndex;
	}

	// Start copying remaining entries in-place in the array, shuffling all the remaining entries forward, until we
	// reach the end of the array or remove all the target entries. The early abort when we've removed all target
	// entries helps improve performance when the elements to remove are towards the start of the child array.
	size_t writeIndex = readIndex;
	if (readIndex < currentChildCount)
	{
		_buildState.childNodes[readIndex].node->UpdateFrameBufferAssociation(_buildState.childNodes[readIndex].frameBufferIndex, nullptr);
		++readIndex;
		size_t childCountToRemove = childNodes.size();
		size_t removedChildNodeCount = 1; // Start at 1 because we skip over the entry we identified in the loop above
		while (readIndex < currentChildCount)
		{
			auto& entry = _buildState.childNodes[readIndex];
			if (childNodes.find(entry.node) == childNodes.end())
			{
				_buildState.childNodes[writeIndex++] = _buildState.childNodes[readIndex];
			}
			else
			{
				entry.node->UpdateFrameBufferAssociation(entry.frameBufferIndex, nullptr);
				if (++removedChildNodeCount == childCountToRemove)
				{
					++readIndex;
					break;
				}
			}
			++readIndex;
		}
	}

	// Shuffle forward any remaining entries in the array now that we've removed all the target entries
	while (readIndex < currentChildCount)
	{
		_buildState.childNodes[writeIndex++] = _buildState.childNodes[readIndex++];
	}

	// Resize the array to trim off the dead entries at the end
	_buildState.childNodes.resize(writeIndex);

	// Flag the draw state as no longer being current
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void Direct3DRenderPassNode::RemoveAllChildNodes()
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	for (auto& entry : _buildState.childNodes)
	{
		entry.node->UpdateFrameBufferAssociation(entry.frameBufferIndex, nullptr);
	}
	_buildState.childNodes.clear();
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void Direct3DRenderPassNode::SetChildNodes(IProgramNode* const* childNodes, size_t childNodeCount, IDefaultState* const* defaultState)
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	for (auto& entry : _buildState.childNodes)
	{
		entry.node->UpdateFrameBufferAssociation(entry.frameBufferIndex, nullptr);
	}
	_buildState.childNodes.resize(childNodeCount);
	if (defaultState != nullptr)
	{
		for (size_t i = 0; i < childNodeCount; ++i)
		{
			auto& entry = _buildState.childNodes[i];
			entry.node = KnownDynamicCast<Direct3DProgramNode*>(*(childNodes++));
			entry.defaultState = KnownDynamicCast<Direct3DDefaultState*>(*(defaultState++));
			entry.frameBufferIndex = entry.node->UpdateFrameBufferAssociation(-1, _buildState.framebuffer);
		}
	}
	else
	{
		for (size_t i = 0; i < childNodeCount; ++i)
		{
			auto& entry = _buildState.childNodes[i];
			entry.node = KnownDynamicCast<Direct3DProgramNode*>(*(childNodes++));
			entry.defaultState = nullptr;
			entry.frameBufferIndex = entry.node->UpdateFrameBufferAssociation(-1, _buildState.framebuffer);
		}
	}
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void Direct3DRenderPassNode::SetChildNodes(IProgramNode::unique_ptr const* childNodes, size_t childNodeCount, IDefaultState::unique_ptr const* defaultState)
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	for (auto& entry : _buildState.childNodes)
	{
		entry.node->UpdateFrameBufferAssociation(entry.frameBufferIndex, nullptr);
	}
	_buildState.childNodes.resize(childNodeCount);
	if (defaultState != nullptr)
	{
		for (size_t i = 0; i < childNodeCount; ++i)
		{
			auto& entry = _buildState.childNodes[i];
			entry.node = KnownDynamicCast<Direct3DProgramNode*>((childNodes++)->get());
			entry.defaultState = KnownDynamicCast<Direct3DDefaultState*>((defaultState++)->get());
			entry.frameBufferIndex = entry.node->UpdateFrameBufferAssociation(-1, _buildState.framebuffer);
		}
	}
	else
	{
		for (size_t i = 0; i < childNodeCount; ++i)
		{
			auto& entry = _buildState.childNodes[i];
			entry.node = KnownDynamicCast<Direct3DProgramNode*>((childNodes++)->get());
			entry.defaultState = nullptr;
			entry.frameBufferIndex = entry.node->UpdateFrameBufferAssociation(-1, _buildState.framebuffer);
		}
	}
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
const std::vector<Direct3DRenderPassNode::ChildNodeEntry>& Direct3DRenderPassNode::GetChildNodes() const
{
	return _drawState.childNodes;
}

//----------------------------------------------------------------------------------------
// Framebuffer methods
//----------------------------------------------------------------------------------------
Direct3DFrameBuffer* Direct3DRenderPassNode::GetFrameBuffer() const
{
	return _drawState.framebuffer;
}

//----------------------------------------------------------------------------------------
void Direct3DRenderPassNode::BindFrameBuffer(IFrameBuffer* frameBuffer)
{
	_buildState.framebuffer = KnownDynamicCast<Direct3DFrameBuffer*>(frameBuffer);
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	for (auto& childNodeEntry : _buildState.childNodes)
	{
		childNodeEntry.frameBufferIndex = childNodeEntry.node->UpdateFrameBufferAssociation(childNodeEntry.frameBufferIndex, _buildState.framebuffer);
	}
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void Direct3DRenderPassNode::SetAttachmentLoadStoreBehavior(IFrameBuffer::AttachmentType type, size_t index, AttachmentLoadBehavior loadBehavior, AttachmentStoreBehavior storeBehavior)
{
	// There's nothing we can do under the Direct3D 12 API to optimize around attachment load/store behaviour, so we
	// ignore the flags passed through here.
}

//----------------------------------------------------------------------------------------
void Direct3DRenderPassNode::SetAttachmentClearData(IFrameBuffer::AttachmentType type, size_t index, const V4Float32& data)
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	bool foundEntry = false;
	for (auto& attachmentState : _buildState.attachmentState)
	{
		if ((attachmentState.type == type) && (attachmentState.index == index))
		{
			attachmentState.clearData = data;
			attachmentState.clearDataDefined = true;
			foundEntry = true;
			break;
		}
	}
	if (!foundEntry)
	{
		AttachmentState attachmentState;
		attachmentState.type = type;
		attachmentState.index = index;
		attachmentState.clearDataDefined = true;
		attachmentState.clearData = data;
		_buildState.attachmentState.push_back(attachmentState);
	}
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void Direct3DRenderPassNode::SetAttachmentClearData(IFrameBuffer::AttachmentType type, size_t index, const V4Int32& data)
{
	SetAttachmentClearData(type, index, V4Float32((float)data.X(), (float)data.Y(), (float)data.Z(), (float)data.W()));
}

//----------------------------------------------------------------------------------------
void Direct3DRenderPassNode::SetAttachmentClearData(IFrameBuffer::AttachmentType type, size_t index, const V4UInt32& data)
{
	SetAttachmentClearData(type, index, V4Float32((float)data.X(), (float)data.Y(), (float)data.Z(), (float)data.W()));
}

//----------------------------------------------------------------------------------------
void Direct3DRenderPassNode::RemoveAttachmentClearData(IFrameBuffer::AttachmentType type, size_t index)
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	bool foundEntry = false;
	for (auto attachmentStateIterator = _buildState.attachmentState.begin(); attachmentStateIterator != _buildState.attachmentState.end(); ++attachmentStateIterator)
	{
		auto& attachmentState = *attachmentStateIterator;
		if ((attachmentState.type == type) && (attachmentState.index == index))
		{
			attachmentState.clearDataDefined = false;
			if (!attachmentState.clearDataDefined && !attachmentState.multiSampleResolutionEnabled)
			{
				_buildState.attachmentState.erase(attachmentStateIterator);
			}
			foundEntry = true;
			break;
		}
	}
	if (!foundEntry)
	{
		return;
	}
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void Direct3DRenderPassNode::EnableAttachmentMultiSamplingResolution(IFrameBuffer::AttachmentType attachmentType, size_t attachmentIndex, size_t resolveAttachmentIndex)
{
	if (attachmentType != IFrameBuffer::AttachmentType::Color)
	{
		_log->Error("Cannot resolve multisample buffer for type {0} and index {1} as multisample resolution is currently only supported for color attachments.", attachmentType, attachmentIndex);
		return;
	}

	// If no resolve attachment index has been specified, default to the index of the attachment itself.
	if (resolveAttachmentIndex == std::numeric_limits<size_t>::max())
	{
		resolveAttachmentIndex = attachmentIndex;
	}

	std::unique_lock<std::mutex> lock(_childNodeMutex);
	bool foundEntry = false;
	for (auto& attachmentState : _buildState.attachmentState)
	{
		if ((attachmentState.type == attachmentType) && (attachmentState.index == attachmentIndex))
		{
			attachmentState.multiSampleResolveTargetType = attachmentType;
			attachmentState.multiSampleResolveTargetIndex = resolveAttachmentIndex;
			attachmentState.multiSampleResolutionEnabled = true;
			foundEntry = true;
			break;
		}
	}
	if (!foundEntry)
	{
		AttachmentState attachmentState;
		attachmentState.type = attachmentType;
		attachmentState.index = attachmentIndex;
		attachmentState.multiSampleResolveTargetType = attachmentType;
		attachmentState.multiSampleResolveTargetIndex = resolveAttachmentIndex;
		attachmentState.multiSampleResolutionEnabled = true;
		_buildState.attachmentState.push_back(attachmentState);
	}
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void Direct3DRenderPassNode::DisableAttachmentMultiSamplingResolution(IFrameBuffer::AttachmentType attachmentType, size_t attachmentIndex)
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	bool foundEntry = false;
	for (auto attachmentStateIterator = _buildState.attachmentState.begin(); attachmentStateIterator != _buildState.attachmentState.end(); ++attachmentStateIterator)
	{
		auto& attachmentState = *attachmentStateIterator;
		if ((attachmentState.type == attachmentType) && (attachmentState.index == attachmentIndex))
		{
			attachmentState.multiSampleResolutionEnabled = false;
			if (!attachmentState.clearDataDefined && !attachmentState.multiSampleResolutionEnabled)
			{
				_buildState.attachmentState.erase(attachmentStateIterator);
			}
			foundEntry = true;
			break;
		}
	}
	if (!foundEntry)
	{
		return;
	}
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
// Enabled state methods
//----------------------------------------------------------------------------------------
bool Direct3DRenderPassNode::IsEnabled() const
{
	return _drawState.nodeEnabled;
}

//----------------------------------------------------------------------------------------
void Direct3DRenderPassNode::SetIsEnabled(bool state)
{
	_buildState.nodeEnabled = state;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
void Direct3DRenderPassNode::MigrateBuildStateToDrawState()
{
	// Migrate our build state
	if (!IsDrawStateCurrent())
	{
		_drawState = _buildState;
		FlagDrawStateCurrent();
	}

	// Record if we have any multisample resolve targets defined
	_hasMultiSampleResolveTargets = false;
	for (const auto& entry : _drawState.attachmentState)
	{
		if (entry.multiSampleResolutionEnabled)
		{
			_hasMultiSampleResolveTargets = true;
			break;
		}
	}

	// Migrate build state in our child nodes
	for (const ChildNodeEntry& entry : _drawState.childNodes)
	{
		if (entry.defaultState != nullptr)
		{
			entry.defaultState->MigrateBuildStateToDrawState();
		}
		entry.node->MigrateBuildStateToDrawState();
	}
}

//----------------------------------------------------------------------------------------
void Direct3DRenderPassNode::ApplyFixedState(ID3D12GraphicsCommandList* commandList)
{
	// Clear each render target that has clear data set
	bool hasDepthClearData = false;
	bool hasStencilClearData = false;
	float depthClearData = 0;
	uint32_t stencilClearData = 0;
	for (const auto& entry : _drawState.attachmentState)
	{
		if (!entry.clearDataDefined)
		{
			continue;
		}
		switch (entry.type)
		{
		case IFrameBuffer::AttachmentType::Color:
			_drawState.framebuffer->ClearRenderView(commandList, entry.index, entry.clearData);
			break;
		case IFrameBuffer::AttachmentType::Depth:
			depthClearData = entry.clearData.X();
			hasDepthClearData = true;
			break;
		case IFrameBuffer::AttachmentType::Stencil:
			stencilClearData = (uint32_t)entry.clearData.X();
			hasStencilClearData = true;
			break;
		}
	}
	if (hasDepthClearData || hasStencilClearData)
	{
		_drawState.framebuffer->ClearDepthStencilView(commandList, depthClearData, stencilClearData);
	}
}

//----------------------------------------------------------------------------------------
void Direct3DRenderPassNode::PerformUnbindOperations(ID3D12GraphicsCommandList* commandList)
{
	// If we don't have any multisample resolve targets defined, abort any further processing.
	if (!_hasMultiSampleResolveTargets)
	{
		return;
	}

	// Perform multisample resolve operations where required
	for (const auto& attachmentState : _drawState.attachmentState)
	{
		// If this attachment doesn't have a multisample resolution operation enabled, skip it.
		if (!attachmentState.multiSampleResolutionEnabled)
		{
			continue;
		}

		// Resolve the multisampled framebuffer attachment to the target texture
		_drawState.framebuffer->ResolveMultiSamplingAttachmentToTexture(commandList, attachmentState.type, attachmentState.index, attachmentState.multiSampleResolveTargetIndex);
	}
}

//----------------------------------------------------------------------------------------
// Debug methods
//----------------------------------------------------------------------------------------
void Direct3DRenderPassNode::SetDebugName(const Marshal::In<std::string>& name)
{
	// String is stored as wide string because render markers require a wide string
	_buildState.debugName = UTF8ToUTF16(name);
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
const std::wstring& Direct3DRenderPassNode::DebugName() const
{
	return _drawState.debugName;
}

} // namespace cobalt::graphics
