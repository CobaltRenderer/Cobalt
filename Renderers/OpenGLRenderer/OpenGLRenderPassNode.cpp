// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "OpenGLRenderPassNode.h"
#include "OpenGLDebug.h"
#include "OpenGLDefaultState.h"
#include "OpenGLProgramNode.h"
#include "OpenGLRenderer.h"
#include <Internal/RendererSupport/KnownDynamicCast.h>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
OpenGLRenderPassNode::OpenGLRenderPassNode(cobalt::logging::ILogger* log, OpenGLRenderer* renderer)
: _renderer(renderer), _log(log)
{
	_buildState.framebuffer = nullptr;
	_buildState.nodeEnabled = true;
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void OpenGLRenderPassNode::Delete()
{
	_renderer->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
// Child node methods
//----------------------------------------------------------------------------------------
void OpenGLRenderPassNode::AddChildNode(IProgramNode* childNode, IDefaultState* defaultState)
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	ChildNodeEntry entry{};
	entry.node = KnownDynamicCast<OpenGLProgramNode*>(childNode);
	entry.defaultState = KnownDynamicCast<OpenGLDefaultState*>(defaultState);
	_buildState.childNodes.push_back(entry);
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void OpenGLRenderPassNode::AddChildNodes(IProgramNode* const* childNodes, size_t childNodeCount, IDefaultState* const* defaultState)
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
			entry.node = KnownDynamicCast<OpenGLProgramNode*>(*(childNodes++));
			entry.defaultState = KnownDynamicCast<OpenGLDefaultState*>(*(defaultState++));
		}
	}
	else
	{
		for (size_t i = currentChildCount; i < newChildCount; ++i)
		{
			auto& entry = _buildState.childNodes[i];
			entry.node = KnownDynamicCast<OpenGLProgramNode*>(*(childNodes++));
			entry.defaultState = nullptr;
		}
	}
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void OpenGLRenderPassNode::AddChildNodes(IProgramNode::unique_ptr const* childNodes, size_t childNodeCount, IDefaultState::unique_ptr const* defaultState)
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
			entry.node = KnownDynamicCast<OpenGLProgramNode*>((childNodes++)->get());
			entry.defaultState = KnownDynamicCast<OpenGLDefaultState*>((defaultState++)->get());
		}
	}
	else
	{
		for (size_t i = currentChildCount; i < newChildCount; ++i)
		{
			auto& entry = _buildState.childNodes[i];
			entry.node = KnownDynamicCast<OpenGLProgramNode*>((childNodes++)->get());
			entry.defaultState = nullptr;
		}
	}
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void OpenGLRenderPassNode::RemoveChildNode(IProgramNode* childNode)
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	for (auto i = _buildState.childNodes.begin(); i != _buildState.childNodes.end(); ++i)
	{
		if (i->node == childNode)
		{
			_buildState.childNodes.erase(i);
			lock.unlock();
			FlagDrawStateNotCurrent();
			return;
		}
	}
}

//----------------------------------------------------------------------------------------
void OpenGLRenderPassNode::RemoveChildNodes(IProgramNode* const* childNodes, size_t childNodeCount)
{
	std::unordered_set<IProgramNode*> nodesToRemove;
	for (size_t i = 0; i < childNodeCount; ++i)
	{
		nodesToRemove.insert(*(childNodes++));
	}
	RemoveChildNodes(nodesToRemove);
}

//----------------------------------------------------------------------------------------
void OpenGLRenderPassNode::RemoveChildNodes(IProgramNode::unique_ptr const* childNodes, size_t childNodeCount)
{
	std::unordered_set<IProgramNode*> nodesToRemove;
	for (size_t i = 0; i < childNodeCount; ++i)
	{
		nodesToRemove.insert((childNodes++)->get());
	}
	RemoveChildNodes(nodesToRemove);
}

//----------------------------------------------------------------------------------------
void OpenGLRenderPassNode::RemoveChildNodes(const std::unordered_set<IProgramNode*>& childNodes)
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
	size_t writeIndex = readIndex++;
	size_t childCountToRemove = childNodes.size();
	size_t removedChildNodeCount = 1; // Start at 1 because we skip over the entry we identified in the loop above
	while (readIndex < currentChildCount)
	{
		if (childNodes.find(_buildState.childNodes[readIndex].node) == childNodes.end())
		{
			_buildState.childNodes[writeIndex++] = _buildState.childNodes[readIndex];
		}
		else
		{
			if (++removedChildNodeCount == childCountToRemove)
			{
				++readIndex;
				break;
			}
		}
		++readIndex;
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
void OpenGLRenderPassNode::RemoveAllChildNodes()
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	_buildState.childNodes.clear();
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void OpenGLRenderPassNode::SetChildNodes(IProgramNode* const* childNodes, size_t childNodeCount, IDefaultState* const* defaultState)
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	_buildState.childNodes.resize(childNodeCount);
	if (defaultState != nullptr)
	{
		for (size_t i = 0; i < childNodeCount; ++i)
		{
			auto& entry = _buildState.childNodes[i];
			entry.node = KnownDynamicCast<OpenGLProgramNode*>(*(childNodes++));
			entry.defaultState = KnownDynamicCast<OpenGLDefaultState*>(*(defaultState++));
		}
	}
	else
	{
		for (size_t i = 0; i < childNodeCount; ++i)
		{
			auto& entry = _buildState.childNodes[i];
			entry.node = KnownDynamicCast<OpenGLProgramNode*>(*(childNodes++));
			entry.defaultState = nullptr;
		}
	}
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void OpenGLRenderPassNode::SetChildNodes(IProgramNode::unique_ptr const* childNodes, size_t childNodeCount, IDefaultState::unique_ptr const* defaultState)
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	_buildState.childNodes.resize(childNodeCount);
	if (defaultState != nullptr)
	{
		for (size_t i = 0; i < childNodeCount; ++i)
		{
			auto& entry = _buildState.childNodes[i];
			entry.node = KnownDynamicCast<OpenGLProgramNode*>((childNodes++)->get());
			entry.defaultState = KnownDynamicCast<OpenGLDefaultState*>((defaultState++)->get());
		}
	}
	else
	{
		for (size_t i = 0; i < childNodeCount; ++i)
		{
			auto& entry = _buildState.childNodes[i];
			entry.node = KnownDynamicCast<OpenGLProgramNode*>((childNodes++)->get());
			entry.defaultState = nullptr;
		}
	}
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
const std::vector<OpenGLRenderPassNode::ChildNodeEntry>& OpenGLRenderPassNode::GetChildNodes() const
{
	return _drawState.childNodes;
}

//----------------------------------------------------------------------------------------
// Framebuffer methods
//----------------------------------------------------------------------------------------
OpenGLFrameBuffer* OpenGLRenderPassNode::GetFrameBuffer() const
{
	return _drawState.framebuffer;
}

//----------------------------------------------------------------------------------------
void OpenGLRenderPassNode::BindFrameBuffer(IFrameBuffer* frameBuffer)
{
	_buildState.framebuffer = KnownDynamicCast<OpenGLFrameBuffer*>(frameBuffer);
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void OpenGLRenderPassNode::SetAttachmentLoadStoreBehavior(IFrameBuffer::AttachmentType type, size_t index, AttachmentLoadBehavior loadBehavior, AttachmentStoreBehavior storeBehavior)
{
	// There's nothing we can do under the OpenGL API to optimize around attachment load/store behaviour, so we ignore
	// the flags passed through here.
}

//----------------------------------------------------------------------------------------
OpenGLRenderPassNode::AttachmentState& OpenGLRenderPassNode::RetrieveOrCreateAttachmentState(IFrameBuffer::AttachmentType type, size_t index)
{
	// Return a reference to the existing attachment state object if present
	for (auto& attachmentState : _buildState.attachmentState)
	{
		if ((attachmentState.type == type) && (attachmentState.index == index))
		{
			return attachmentState;
		}
	}

	// Initialize and return a new attachment state entry
	AttachmentState& attachmentState = _buildState.attachmentState.emplace_back();
	attachmentState.type = type;
	attachmentState.index = index;
	attachmentState.nativeBufferType = AttachmentTypeToBufferType(type);
	attachmentState.nativeBufferIndex = (GLuint)index;
	return attachmentState;
}

//----------------------------------------------------------------------------------------
void OpenGLRenderPassNode::SetAttachmentClearData(IFrameBuffer::AttachmentType type, size_t index, const V4Float32& data)
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	auto& attachmentState = RetrieveOrCreateAttachmentState(type, index);
	attachmentState.clearDataFloat = data;
	attachmentState.clearDataFormat = OpenGLFrameBuffer::AttachmentFormat::Float;
	attachmentState.clearDataDefined = true;
	attachmentState.clearDataTypeMatched = false;
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void OpenGLRenderPassNode::SetAttachmentClearData(IFrameBuffer::AttachmentType type, size_t index, const V4Int32& data)
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	auto& attachmentState = RetrieveOrCreateAttachmentState(type, index);
	attachmentState.clearDataInt = data;
	attachmentState.clearDataFormat = OpenGLFrameBuffer::AttachmentFormat::Int;
	attachmentState.clearDataDefined = true;
	attachmentState.clearDataTypeMatched = false;
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void OpenGLRenderPassNode::SetAttachmentClearData(IFrameBuffer::AttachmentType type, size_t index, const V4UInt32& data)
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	auto& attachmentState = RetrieveOrCreateAttachmentState(type, index);
	attachmentState.clearDataUInt = data;
	attachmentState.clearDataFormat = OpenGLFrameBuffer::AttachmentFormat::UInt;
	attachmentState.clearDataDefined = true;
	attachmentState.clearDataTypeMatched = false;
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void OpenGLRenderPassNode::RemoveAttachmentClearData(IFrameBuffer::AttachmentType type, size_t index)
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);
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
			break;
		}
	}
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void OpenGLRenderPassNode::EnableAttachmentMultiSamplingResolution(IFrameBuffer::AttachmentType attachmentType, size_t attachmentIndex, size_t resolveAttachmentIndex)
{
	// Ensure a colour attachment has been specified, as this is the only type currently supported.
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

	// Record the multisample resolve settings in the attachment state object
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	auto& attachmentState = RetrieveOrCreateAttachmentState(attachmentType, attachmentIndex);
	attachmentState.multiSampleResolveTargetType = attachmentType;
	attachmentState.multiSampleResolveTargetIndex = resolveAttachmentIndex;
	attachmentState.multiSampleResolutionEnabled = true;
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void OpenGLRenderPassNode::DisableAttachmentMultiSamplingResolution(IFrameBuffer::AttachmentType attachmentType, size_t attachmentIndex)
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
constexpr GLenum OpenGLRenderPassNode::AttachmentTypeToBufferType(IFrameBuffer::AttachmentType type)
{
	switch (type)
	{
	case IFrameBuffer::AttachmentType::Color:
		return GL_COLOR;
	case IFrameBuffer::AttachmentType::Depth:
		return GL_DEPTH;
	case IFrameBuffer::AttachmentType::Stencil:
		return GL_STENCIL;
	}
	return GL_COLOR;
}

//----------------------------------------------------------------------------------------
// Enabled state methods
//----------------------------------------------------------------------------------------
bool OpenGLRenderPassNode::IsEnabled() const
{
	return _drawState.nodeEnabled;
}

//----------------------------------------------------------------------------------------
void OpenGLRenderPassNode::SetIsEnabled(bool state)
{
	_buildState.nodeEnabled = state;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
void OpenGLRenderPassNode::MigrateBuildStateToDrawState()
{
	// Migrate our build state
	bool frameBufferChanged = false;
	if (!IsDrawStateCurrent())
	{
		frameBufferChanged = (_drawState.framebuffer != _buildState.framebuffer);
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

	// If the framebuffer changed, discard any resolved clear data entries. Note that this isn't technically required,
	// as these flags will always be false in the build state, but we've chosen to be explicit here that this state
	// needs to be cleared in this case.
	if (frameBufferChanged)
	{
		for (auto& entry : _drawState.attachmentState)
		{
			entry.clearDataTypeMatched = false;
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
void OpenGLRenderPassNode::ApplyFixedState(OpenGLFrameBuffer* frameBuffer)
{
	// Ensure depth writing is enabled if it's previously been disabled
	CheckGLError(_log);
	glDepthMask(GL_TRUE);

	// Ensure stencil writing is enabled if it's previously been disabled
	glStencilMask(0xFFFFFFFF);

	// If the scissor test is enabled, temporarily disable it. The scissor test restricts the region in which the
	// glClearBuffer operation applies, while we want to clear the entire buffer here.
	bool scissorTestEnabled = (glIsEnabled(GL_SCISSOR_TEST) == GL_TRUE);
	if (scissorTestEnabled)
	{
		glDisable(GL_SCISSOR_TEST);
	}

	// Clear each render target that has clear data set
	bool hasDepthClearData = false;
	bool hasStencilClearData = false;
	float depthClearData = 0;
	int stencilClearData = 0;
	for (auto& attachmentState : _drawState.attachmentState)
	{
		// If no clear data is defined, skip this attachment state entry.
		if (!attachmentState.clearDataDefined)
		{
			continue;
		}

		// If the clear data hasn't been matched to the bound texture format yet, do that now. As per the OpenGL spec,
		// the behaviour is undefined if a clear is attempted with mismatched data, such as attempting to clear a buffer
		// which uses floats with int data. Since we allow the data to be supplied in any format and converted, we need
		// to perform the conversion here.
		if (!attachmentState.clearDataTypeMatched)
		{
			auto attachmentFormat = frameBuffer->GetColorAttachmentFormat(attachmentState.index);
			switch (attachmentState.type)
			{
			case IFrameBuffer::AttachmentType::Color:
				attachmentFormat = frameBuffer->GetColorAttachmentFormat(attachmentState.index);
				break;
			case IFrameBuffer::AttachmentType::Depth:
				attachmentFormat = OpenGLFrameBuffer::AttachmentFormat::Float;
				break;
			case IFrameBuffer::AttachmentType::Stencil:
				attachmentFormat = OpenGLFrameBuffer::AttachmentFormat::Int;
				break;
			}
			switch (attachmentFormat)
			{
			case OpenGLFrameBuffer::AttachmentFormat::Int:
				if (attachmentState.clearDataFormat == OpenGLFrameBuffer::AttachmentFormat::Float)
				{
					attachmentState.clearDataInt = V4Int32((int)attachmentState.clearDataFloat.X(), (int)attachmentState.clearDataFloat.Y(), (int)attachmentState.clearDataFloat.Z(), (int)attachmentState.clearDataFloat.W());
				}
				else if (attachmentState.clearDataFormat == OpenGLFrameBuffer::AttachmentFormat::UInt)
				{
					attachmentState.clearDataInt = V4Int32((int)attachmentState.clearDataUInt.X(), (int)attachmentState.clearDataUInt.Y(), (int)attachmentState.clearDataUInt.Z(), (int)attachmentState.clearDataUInt.W());
				}
				break;
			case OpenGLFrameBuffer::AttachmentFormat::UInt:
				if (attachmentState.clearDataFormat == OpenGLFrameBuffer::AttachmentFormat::Float)
				{
					attachmentState.clearDataUInt = V4UInt32((unsigned int)attachmentState.clearDataFloat.X(), (unsigned int)attachmentState.clearDataFloat.Y(), (unsigned int)attachmentState.clearDataFloat.Z(), (unsigned int)attachmentState.clearDataFloat.W());
				}
				else if (attachmentState.clearDataFormat == OpenGLFrameBuffer::AttachmentFormat::Int)
				{
					attachmentState.clearDataUInt = V4UInt32((unsigned int)attachmentState.clearDataInt.X(), (unsigned int)attachmentState.clearDataInt.Y(), (unsigned int)attachmentState.clearDataInt.Z(), (unsigned int)attachmentState.clearDataInt.W());
				}
				break;
			case OpenGLFrameBuffer::AttachmentFormat::Float:
				if (attachmentState.clearDataFormat == OpenGLFrameBuffer::AttachmentFormat::Int)
				{
					attachmentState.clearDataFloat = V4Float32((float)attachmentState.clearDataInt.X(), (float)attachmentState.clearDataInt.Y(), (float)attachmentState.clearDataInt.Z(), (float)attachmentState.clearDataInt.W());
				}
				else if (attachmentState.clearDataFormat == OpenGLFrameBuffer::AttachmentFormat::UInt)
				{
					attachmentState.clearDataFloat = V4Float32((float)attachmentState.clearDataUInt.X(), (float)attachmentState.clearDataUInt.Y(), (float)attachmentState.clearDataUInt.Z(), (float)attachmentState.clearDataUInt.W());
				}
				break;
			}
			attachmentState.actualFormat = attachmentFormat;
			attachmentState.clearDataTypeMatched = true;
		}

		// If the target is a colour buffer, perform the clear operation. For depth and stencil targets we accumulate
		// them and clear them together at the end.
		switch (attachmentState.type)
		{
		case IFrameBuffer::AttachmentType::Color:
		{
			switch (attachmentState.actualFormat)
			{
			case OpenGLFrameBuffer::AttachmentFormat::Int:
				glClearBufferiv(attachmentState.nativeBufferType, attachmentState.nativeBufferIndex, attachmentState.clearDataInt.data());
				break;
			case OpenGLFrameBuffer::AttachmentFormat::UInt:
				glClearBufferuiv(attachmentState.nativeBufferType, attachmentState.nativeBufferIndex, attachmentState.clearDataUInt.data());
				break;
			case OpenGLFrameBuffer::AttachmentFormat::Float:
				glClearBufferfv(attachmentState.nativeBufferType, attachmentState.nativeBufferIndex, attachmentState.clearDataFloat.data());
				break;
			}
			break;
		}
		case IFrameBuffer::AttachmentType::Depth:
			hasDepthClearData = true;
			depthClearData = attachmentState.clearDataFloat.X();
			break;
		case IFrameBuffer::AttachmentType::Stencil:
			hasStencilClearData = true;
			stencilClearData = attachmentState.clearDataInt.X();
			break;
		}
	}

	// Clear the depth and/or stencil buffers if requested
	if (hasDepthClearData || hasStencilClearData)
	{
		if (hasDepthClearData && hasStencilClearData)
		{
			glClearBufferfi(GL_DEPTH_STENCIL, 0, depthClearData, (GLint)stencilClearData);
		}
		else if (hasDepthClearData)
		{
			glClearBufferfv(GL_DEPTH, 0, &depthClearData);
		}
		else if (hasStencilClearData)
		{
			auto stencilClearDataNative = (GLint)stencilClearData;
			glClearBufferiv(GL_STENCIL, 0, &stencilClearDataNative);
		}
	}

	// Restore the scissor test if we disabled it
	if (scissorTestEnabled)
	{
		glEnable(GL_SCISSOR_TEST);
	}
	CheckGLError(_log);
}

//----------------------------------------------------------------------------------------
void OpenGLRenderPassNode::PerformUnbindOperations()
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
		_drawState.framebuffer->ResolveMultiSamplingAttachmentToTexture(attachmentState.type, attachmentState.index, attachmentState.multiSampleResolveTargetIndex);
	}
}

//----------------------------------------------------------------------------------------
// Debug methods
//----------------------------------------------------------------------------------------
void OpenGLRenderPassNode::SetDebugName(const Marshal::In<std::string>& name)
{
	_buildState.debugName = name;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
const std::string& OpenGLRenderPassNode::DebugName() const
{
	return _drawState.debugName;
}

} // namespace cobalt::graphics
