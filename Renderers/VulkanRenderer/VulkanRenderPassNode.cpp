// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "VulkanRenderPassNode.h"
#include "VulkanDefaultState.h"
#include "VulkanFrameBuffer.h"
#include "VulkanProgramNode.h"
#include "VulkanRenderer.h"
#include <Internal/RendererSupport/KnownDynamicCast.h>
#include <array>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
VulkanRenderPassNode::VulkanRenderPassNode(cobalt::logging::ILogger* log, VulkanRenderer* renderer)
{
	_log = log;
	_renderer = renderer;

	_buildState.framebuffer = nullptr;
	_buildState.nodeEnabled = true;
}

//----------------------------------------------------------------------------------------
VulkanRenderPassNode::~VulkanRenderPassNode()
{
	// Delete the existing render pass object
	if (_renderPass != VK_NULL_HANDLE)
	{
		VkDevice device = _renderer->GetDevice();
		vkDestroyRenderPass(device, _renderPass, nullptr);
		_renderPass = VK_NULL_HANDLE;
	}
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void VulkanRenderPassNode::Delete()
{
	_renderer->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
// Child node methods
//----------------------------------------------------------------------------------------
void VulkanRenderPassNode::AddChildNode(IProgramNode* childNode, IDefaultState* defaultState)
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	ChildNodeEntry entry{};
	entry.node = KnownDynamicCast<VulkanProgramNode*>(childNode);
	entry.defaultState = KnownDynamicCast<VulkanDefaultState*>(defaultState);
	entry.frameBufferIndex = entry.node->UpdateFrameBufferAssociation(-1, _buildState.framebuffer);
	entry.defaultStateIndex = entry.node->UpdateDefaultStateAssociation(-1, entry.defaultState);
	_buildState.childNodes.push_back(entry);
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void VulkanRenderPassNode::AddChildNodes(IProgramNode* const* childNodes, size_t childNodeCount, IDefaultState* const* defaultState)
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
			entry.node = KnownDynamicCast<VulkanProgramNode*>(*(childNodes++));
			entry.defaultState = KnownDynamicCast<VulkanDefaultState*>(*(defaultState++));
			entry.frameBufferIndex = entry.node->UpdateFrameBufferAssociation(-1, _buildState.framebuffer);
			entry.defaultStateIndex = entry.node->UpdateDefaultStateAssociation(-1, entry.defaultState);
		}
	}
	else
	{
		for (size_t i = currentChildCount; i < newChildCount; ++i)
		{
			auto& entry = _buildState.childNodes[i];
			entry.node = KnownDynamicCast<VulkanProgramNode*>(*(childNodes++));
			entry.defaultState = nullptr;
			entry.frameBufferIndex = entry.node->UpdateFrameBufferAssociation(-1, _buildState.framebuffer);
			entry.defaultStateIndex = entry.node->UpdateDefaultStateAssociation(-1, entry.defaultState);
		}
	}
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void VulkanRenderPassNode::AddChildNodes(IProgramNode::unique_ptr const* childNodes, size_t childNodeCount, IDefaultState::unique_ptr const* defaultState)
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
			entry.node = KnownDynamicCast<VulkanProgramNode*>((childNodes++)->get());
			entry.defaultState = KnownDynamicCast<VulkanDefaultState*>((defaultState++)->get());
			entry.frameBufferIndex = entry.node->UpdateFrameBufferAssociation(-1, _buildState.framebuffer);
			entry.defaultStateIndex = entry.node->UpdateDefaultStateAssociation(-1, entry.defaultState);
		}
	}
	else
	{
		for (size_t i = currentChildCount; i < newChildCount; ++i)
		{
			auto& entry = _buildState.childNodes[i];
			entry.node = KnownDynamicCast<VulkanProgramNode*>((childNodes++)->get());
			entry.defaultState = nullptr;
			entry.frameBufferIndex = entry.node->UpdateFrameBufferAssociation(-1, _buildState.framebuffer);
			entry.defaultStateIndex = entry.node->UpdateDefaultStateAssociation(-1, entry.defaultState);
		}
	}
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void VulkanRenderPassNode::RemoveChildNode(IProgramNode* childNode)
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	for (auto i = _buildState.childNodes.begin(); i != _buildState.childNodes.end(); ++i)
	{
		if (i->node == childNode)
		{
			i->node->UpdateFrameBufferAssociation(i->frameBufferIndex, nullptr);
			i->node->UpdateDefaultStateAssociation(i->defaultStateIndex, nullptr);
			_buildState.childNodes.erase(i);
			lock.unlock();
			FlagDrawStateNotCurrent();
			return;
		}
	}
}

//----------------------------------------------------------------------------------------
void VulkanRenderPassNode::RemoveChildNodes(IProgramNode* const* childNodes, size_t childNodeCount)
{
	std::unordered_set<IProgramNode*> nodesToRemove;
	for (size_t i = 0; i < childNodeCount; ++i)
	{
		nodesToRemove.insert(*(childNodes++));
	}
	RemoveChildNodes(nodesToRemove);
}

//----------------------------------------------------------------------------------------
void VulkanRenderPassNode::RemoveChildNodes(IProgramNode::unique_ptr const* childNodes, size_t childNodeCount)
{
	std::unordered_set<IProgramNode*> nodesToRemove;
	for (size_t i = 0; i < childNodeCount; ++i)
	{
		nodesToRemove.insert((childNodes++)->get());
	}
	RemoveChildNodes(nodesToRemove);
}

//----------------------------------------------------------------------------------------
void VulkanRenderPassNode::RemoveChildNodes(const std::unordered_set<IProgramNode*>& childNodes)
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
		_buildState.childNodes[readIndex].node->UpdateDefaultStateAssociation(_buildState.childNodes[readIndex].defaultStateIndex, nullptr);
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
				entry.node->UpdateDefaultStateAssociation(entry.defaultStateIndex, nullptr);
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
void VulkanRenderPassNode::RemoveAllChildNodes()
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	for (auto& entry : _buildState.childNodes)
	{
		entry.node->UpdateFrameBufferAssociation(entry.frameBufferIndex, nullptr);
		entry.node->UpdateDefaultStateAssociation(entry.defaultStateIndex, nullptr);
	}
	_buildState.childNodes.clear();
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void VulkanRenderPassNode::SetChildNodes(IProgramNode* const* childNodes, size_t childNodeCount, IDefaultState* const* defaultState)
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	for (auto& entry : _buildState.childNodes)
	{
		entry.node->UpdateFrameBufferAssociation(entry.frameBufferIndex, nullptr);
		entry.node->UpdateDefaultStateAssociation(entry.defaultStateIndex, nullptr);
	}
	_buildState.childNodes.resize(childNodeCount);
	if (defaultState != nullptr)
	{
		for (size_t i = 0; i < childNodeCount; ++i)
		{
			auto& entry = _buildState.childNodes[i];
			entry.node = KnownDynamicCast<VulkanProgramNode*>(*(childNodes++));
			entry.defaultState = KnownDynamicCast<VulkanDefaultState*>(*(defaultState++));
			entry.frameBufferIndex = entry.node->UpdateFrameBufferAssociation(-1, _buildState.framebuffer);
			entry.defaultStateIndex = entry.node->UpdateDefaultStateAssociation(-1, entry.defaultState);
		}
	}
	else
	{
		for (size_t i = 0; i < childNodeCount; ++i)
		{
			auto& entry = _buildState.childNodes[i];
			entry.node = KnownDynamicCast<VulkanProgramNode*>(*(childNodes++));
			entry.defaultState = nullptr;
			entry.frameBufferIndex = entry.node->UpdateFrameBufferAssociation(-1, _buildState.framebuffer);
			entry.defaultStateIndex = entry.node->UpdateDefaultStateAssociation(-1, entry.defaultState);
		}
	}
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void VulkanRenderPassNode::SetChildNodes(IProgramNode::unique_ptr const* childNodes, size_t childNodeCount, IDefaultState::unique_ptr const* defaultState)
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	for (auto& entry : _buildState.childNodes)
	{
		entry.node->UpdateFrameBufferAssociation(entry.frameBufferIndex, nullptr);
		entry.node->UpdateDefaultStateAssociation(entry.defaultStateIndex, nullptr);
	}
	_buildState.childNodes.resize(childNodeCount);
	if (defaultState != nullptr)
	{
		for (size_t i = 0; i < childNodeCount; ++i)
		{
			auto& entry = _buildState.childNodes[i];
			entry.node = KnownDynamicCast<VulkanProgramNode*>((childNodes++)->get());
			entry.defaultState = KnownDynamicCast<VulkanDefaultState*>((defaultState++)->get());
			entry.frameBufferIndex = entry.node->UpdateFrameBufferAssociation(-1, _buildState.framebuffer);
			entry.defaultStateIndex = entry.node->UpdateDefaultStateAssociation(-1, entry.defaultState);
		}
	}
	else
	{
		for (size_t i = 0; i < childNodeCount; ++i)
		{
			auto& entry = _buildState.childNodes[i];
			entry.node = KnownDynamicCast<VulkanProgramNode*>((childNodes++)->get());
			entry.defaultState = nullptr;
			entry.frameBufferIndex = entry.node->UpdateFrameBufferAssociation(-1, _buildState.framebuffer);
			entry.defaultStateIndex = entry.node->UpdateDefaultStateAssociation(-1, entry.defaultState);
		}
	}
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
const std::vector<VulkanRenderPassNode::ChildNodeEntry>& VulkanRenderPassNode::GetChildNodes() const
{
	return _drawState.childNodes;
}

//----------------------------------------------------------------------------------------
// Framebuffer methods
//----------------------------------------------------------------------------------------
VulkanFrameBuffer* VulkanRenderPassNode::GetFrameBuffer() const
{
	return _drawState.framebuffer;
}

//----------------------------------------------------------------------------------------
void VulkanRenderPassNode::BindFrameBuffer(IFrameBuffer* frameBuffer)
{
	_buildState.framebuffer = KnownDynamicCast<VulkanFrameBuffer*>(frameBuffer);
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	for (auto& childNodeEntry : _buildState.childNodes)
	{
		childNodeEntry.frameBufferIndex = childNodeEntry.node->UpdateFrameBufferAssociation(childNodeEntry.frameBufferIndex, _buildState.framebuffer);
	}
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void VulkanRenderPassNode::SetAttachmentLoadStoreBehavior(IFrameBuffer::AttachmentType type, size_t index, AttachmentLoadBehavior loadBehavior, AttachmentStoreBehavior storeBehavior)
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	bool foundEntry = false;
	for (auto& attachmentState : _buildState.attachmentState)
	{
		if ((attachmentState.type == type) && (attachmentState.index == index))
		{
			attachmentState.loadBehavior = loadBehavior;
			attachmentState.storeBehavior = storeBehavior;
			foundEntry = true;
			break;
		}
	}
	if (!foundEntry)
	{
		AttachmentState attachmentState;
		attachmentState.type = type;
		attachmentState.index = index;
		attachmentState.loadBehavior = loadBehavior;
		attachmentState.storeBehavior = storeBehavior;
		_buildState.attachmentState.push_back(attachmentState);
	}
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void VulkanRenderPassNode::SetAttachmentClearData(IFrameBuffer::AttachmentType type, size_t index, const V4Float32& data)
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	bool foundEntry = false;
	for (auto& attachmentState : _buildState.attachmentState)
	{
		if ((attachmentState.type == type) && (attachmentState.index == index))
		{
			attachmentState.clearDataFloat = data;
			attachmentState.clearDataFormat = VulkanFrameBuffer::AttachmentFormat::Float;
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
		attachmentState.clearDataFloat = data;
		attachmentState.clearDataFormat = VulkanFrameBuffer::AttachmentFormat::Float;
		_buildState.attachmentState.push_back(attachmentState);
	}
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void VulkanRenderPassNode::SetAttachmentClearData(IFrameBuffer::AttachmentType type, size_t index, const V4Int32& data)
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	bool foundEntry = false;
	for (auto& attachmentState : _buildState.attachmentState)
	{
		if ((attachmentState.type == type) && (attachmentState.index == index))
		{
			attachmentState.clearDataInt = data;
			attachmentState.clearDataFormat = VulkanFrameBuffer::AttachmentFormat::Int;
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
		attachmentState.clearDataInt = data;
		attachmentState.clearDataFormat = VulkanFrameBuffer::AttachmentFormat::Int;
		_buildState.attachmentState.push_back(attachmentState);
	}
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void VulkanRenderPassNode::SetAttachmentClearData(IFrameBuffer::AttachmentType type, size_t index, const V4UInt32& data)
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	bool foundEntry = false;
	for (auto& attachmentState : _buildState.attachmentState)
	{
		if ((attachmentState.type == type) && (attachmentState.index == index))
		{
			attachmentState.clearDataUInt = data;
			attachmentState.clearDataFormat = VulkanFrameBuffer::AttachmentFormat::UInt;
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
		attachmentState.clearDataUInt = data;
		attachmentState.clearDataFormat = VulkanFrameBuffer::AttachmentFormat::UInt;
		_buildState.attachmentState.push_back(attachmentState);
	}
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void VulkanRenderPassNode::RemoveAttachmentClearData(IFrameBuffer::AttachmentType type, size_t index)
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
void VulkanRenderPassNode::EnableAttachmentMultiSamplingResolution(IFrameBuffer::AttachmentType attachmentType, size_t attachmentIndex, size_t resolveAttachmentIndex)
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
void VulkanRenderPassNode::DisableAttachmentMultiSamplingResolution(IFrameBuffer::AttachmentType attachmentType, size_t attachmentIndex)
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
void VulkanRenderPassNode::UpdateRenderPassObject()
{
	// Delete the existing render pass object
	if (_renderPass != VK_NULL_HANDLE)
	{
		VkDevice device = _renderer->GetDevice();
		vkDestroyRenderPass(device, _renderPass, nullptr);
		_renderPass = VK_NULL_HANDLE;
	}

	// Retrieve the attachments and attachment references from the framebuffer
	auto* framebuffer = _drawState.framebuffer;
	const VkAttachmentDescription* renderPassAttachmentsDefault = framebuffer->GetRenderPassAttachments();
	size_t renderPassAttachmentCount = framebuffer->GetRenderPassAttachmentCount();
	const VkAttachmentReference* renderPassColorAttachmentReferences = framebuffer->GetRenderPassColorAttachmentReferences();
	size_t renderPassColorAttachmentReferenceCount = framebuffer->GetRenderPassColorAttachmentReferenceCount();
	const VkAttachmentReference* renderPassDepthStencilAttachmentReferences = framebuffer->GetRenderPassDepthStencilAttachmentReference();

	// Set correct values for load/store ops in the attachment descriptions for our render pass
	std::vector<VkAttachmentDescription> renderPassAttachments(renderPassAttachmentsDefault, renderPassAttachmentsDefault + renderPassAttachmentCount);
	for (auto& renderPassAttachment : renderPassAttachments)
	{
		renderPassAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		renderPassAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		renderPassAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		renderPassAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
	}
	for (const auto& attachmentState : _drawState.attachmentState)
	{
		// Determine the load operation to use for this attachment
		VkAttachmentLoadOp loadOperation;
		if (attachmentState.clearDataDefined)
		{
			loadOperation = VK_ATTACHMENT_LOAD_OP_CLEAR;
		}
		else if (attachmentState.loadBehavior == IRenderPassNode::AttachmentLoadBehavior::LoadExistingData)
		{
			loadOperation = VK_ATTACHMENT_LOAD_OP_LOAD;
		}
		else
		{
			loadOperation = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		}

		// Determine the store operation to use for this attachment
		VkAttachmentStoreOp storeOperation;
		if (attachmentState.storeBehavior == IRenderPassNode::AttachmentStoreBehavior::StoreFinalData)
		{
			storeOperation = VK_ATTACHMENT_STORE_OP_STORE;
		}
		else
		{
			storeOperation = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		}

		// Set the load and store operations to the desired values for this attachment
		int attachmentIndex = framebuffer->GetRenderPassAttachmentIndex(attachmentState.type, attachmentState.index);
		auto& renderPassAttachment = renderPassAttachments[attachmentIndex];
		if (attachmentState.type == IFrameBuffer::AttachmentType::Stencil)
		{
			renderPassAttachment.stencilLoadOp = loadOperation;
			renderPassAttachment.stencilStoreOp = storeOperation;
		}
		else
		{
			renderPassAttachment.loadOp = loadOperation;
			renderPassAttachment.storeOp = storeOperation;
			if ((attachmentState.type == IFrameBuffer::AttachmentType::Color) && (loadOperation != VK_ATTACHMENT_LOAD_OP_LOAD))
			{
				// Color attachment contents are not being preserved for this pass, so allow Vulkan to discard the
				// previous image contents during the automatic transition into the subpass layout.
				renderPassAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			}
		}
	}

	// Create the subpass description
	VkSubpassDescription subpassDescription = {};
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.colorAttachmentCount = (uint32_t)renderPassColorAttachmentReferenceCount;
	subpassDescription.pColorAttachments = renderPassColorAttachmentReferences;
	subpassDescription.pDepthStencilAttachment = renderPassDepthStencilAttachmentReferences;

	// Fill in multisample target resolve operations where required
	std::vector<VkAttachmentReference> resolveAttachments;
	if (framebuffer->HasResolveTargets() && (framebuffer->GetSampleCount() != ITextureBuffer::SampleCount::SampleCount1))
	{
		for (const auto& attachmentState : _drawState.attachmentState)
		{
			// If this attachment doesn't have a multisample resolution operation enabled, skip it.
			if (!attachmentState.multiSampleResolutionEnabled)
			{
				continue;
			}

			// If no resolve attachments have been defined yet, resize the resolve attachments array, and populate with
			// initial data settings for each color attachment.
			if (resolveAttachments.empty())
			{
				VkAttachmentReference unusedAttachment;
				unusedAttachment.attachment = VK_ATTACHMENT_UNUSED;
				unusedAttachment.layout = VK_IMAGE_LAYOUT_UNDEFINED;
				resolveAttachments.resize(framebuffer->GetRenderPassColorAttachmentReferenceCount(), unusedAttachment);
			}

			// Populate the resolve attachment entry for this target
			auto& resolveAttachmentEntry = resolveAttachments[framebuffer->GetRenderPassColorAttachmentIndex(framebuffer->GetRenderPassAttachmentIndex(attachmentState.type, attachmentState.index))];
			int resolveAttachmentIndex = framebuffer->GetRenderPassResolveAttachmentIndex(attachmentState.multiSampleResolveTargetType, attachmentState.multiSampleResolveTargetIndex);
			resolveAttachmentEntry.attachment = (uint32_t)resolveAttachmentIndex;
			resolveAttachmentEntry.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			renderPassAttachments[resolveAttachmentIndex].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		}
	}
	subpassDescription.pResolveAttachments = (!resolveAttachments.empty() ? resolveAttachments.data() : nullptr);

	// Define the external dependencies around the subpass. Framebuffer outputs are often read back, sampled, or used
	// by another render pass immediately after this render pass completes, so attachment writes need to be made
	// visible outside the subpass before those later operations run.
	std::array<VkSubpassDependency, 2> subpassDependencies = {};
	subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependencies[0].dstSubpass = 0;
	subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT;
	subpassDependencies[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
	subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
	subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	subpassDependencies[1].srcSubpass = 0;
	subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
	subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT;
	subpassDependencies[1].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	// Create the render pass object
	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = (uint32_t)renderPassAttachments.size();
	renderPassCreateInfo.pAttachments = renderPassAttachments.data();
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpassDescription;
	renderPassCreateInfo.dependencyCount = (uint32_t)subpassDependencies.size();
	renderPassCreateInfo.pDependencies = subpassDependencies.data();
	VkResult createRenderPassResult = vkCreateRenderPass(_renderer->GetDevice(), &renderPassCreateInfo, nullptr, &_renderPass);
	if (createRenderPassResult != VK_SUCCESS)
	{
		_log->Error("Could not create render pass with error code {0}", createRenderPassResult);
		return;
	}
}

//----------------------------------------------------------------------------------------
void VulkanRenderPassNode::UpdateRenderPassBeginInfo()
{
	// Define our render pass begin object
	auto frameBufferSizeInPixels = _drawState.framebuffer->GetFrameBufferSizeInPixels();
	_renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	_renderPassBeginInfo.renderPass = _renderPass;
	_renderPassBeginInfo.framebuffer = _drawState.framebuffer->GetFramebuffer();
	_renderPassBeginInfo.renderArea.offset = {0, 0};
	_renderPassBeginInfo.renderArea.extent = {frameBufferSizeInPixels.X(), frameBufferSizeInPixels.Y()};

	// Setup clear colors
	_renderTargetClearValues.resize(_drawState.framebuffer->GetRenderPassAttachmentCount());
	for (const auto& attachmentState : _drawState.attachmentState)
	{
		// If this attachment doesn't have a clear value defined, skip it.
		if (!attachmentState.clearDataDefined)
		{
			continue;
		}

		// Determine the attachment number for the target framebuffer attachment
		int attachmentNo = _drawState.framebuffer->GetRenderPassAttachmentIndex(attachmentState.type, attachmentState.index);
		if (attachmentNo < 0)
		{
			_log->Warning("Render pass attachment of type {0} and index {1} not found", attachmentState.type, attachmentState.index);
			continue;
		}

		// Load the clear value for this attachment into the array of clear values for the render pass
		switch (attachmentState.type)
		{
		case IFrameBuffer::AttachmentType::Color:
		{
			// VkClearColorValue is a union. Integer attachments must be cleared through the integer member so the
			// values are interpreted as integer clear values rather than as the bit pattern of floating point values.
			auto attachmentFormat = _drawState.framebuffer->GetColorAttachmentFormat(attachmentState.index);
			switch (attachmentFormat)
			{
			case VulkanFrameBuffer::AttachmentFormat::Int:
			{
				V4Int32 clearData = {};
				switch (attachmentState.clearDataFormat)
				{
				case VulkanFrameBuffer::AttachmentFormat::Float:
					clearData = V4Int32((int32_t)attachmentState.clearDataFloat.X(), (int32_t)attachmentState.clearDataFloat.Y(), (int32_t)attachmentState.clearDataFloat.Z(), (int32_t)attachmentState.clearDataFloat.W());
					break;
				case VulkanFrameBuffer::AttachmentFormat::UInt:
					clearData = V4Int32((int32_t)attachmentState.clearDataUInt.X(), (int32_t)attachmentState.clearDataUInt.Y(), (int32_t)attachmentState.clearDataUInt.Z(), (int32_t)attachmentState.clearDataUInt.W());
					break;
				case VulkanFrameBuffer::AttachmentFormat::Int:
					clearData = attachmentState.clearDataInt;
					break;
				}
				_renderTargetClearValues[attachmentNo].color.int32[0] = clearData.X();
				_renderTargetClearValues[attachmentNo].color.int32[1] = clearData.Y();
				_renderTargetClearValues[attachmentNo].color.int32[2] = clearData.Z();
				_renderTargetClearValues[attachmentNo].color.int32[3] = clearData.W();
				break;
			}
			case VulkanFrameBuffer::AttachmentFormat::UInt:
			{
				V4UInt32 clearData = {};
				switch (attachmentState.clearDataFormat)
				{
				case VulkanFrameBuffer::AttachmentFormat::Float:
					clearData = V4UInt32((uint32_t)attachmentState.clearDataFloat.X(), (uint32_t)attachmentState.clearDataFloat.Y(), (uint32_t)attachmentState.clearDataFloat.Z(), (uint32_t)attachmentState.clearDataFloat.W());
					break;
				case VulkanFrameBuffer::AttachmentFormat::Int:
					clearData = V4UInt32((uint32_t)attachmentState.clearDataInt.X(), (uint32_t)attachmentState.clearDataInt.Y(), (uint32_t)attachmentState.clearDataInt.Z(), (uint32_t)attachmentState.clearDataInt.W());
					break;
				case VulkanFrameBuffer::AttachmentFormat::UInt:
					clearData = attachmentState.clearDataUInt;
					break;
				}
				_renderTargetClearValues[attachmentNo].color.uint32[0] = clearData.X();
				_renderTargetClearValues[attachmentNo].color.uint32[1] = clearData.Y();
				_renderTargetClearValues[attachmentNo].color.uint32[2] = clearData.Z();
				_renderTargetClearValues[attachmentNo].color.uint32[3] = clearData.W();
				break;
			}
			case VulkanFrameBuffer::AttachmentFormat::Float:
			{
				V4Float32 clearData = {};
				switch (attachmentState.clearDataFormat)
				{
				case VulkanFrameBuffer::AttachmentFormat::Int:
					clearData = V4Float32((float)attachmentState.clearDataInt.X(), (float)attachmentState.clearDataInt.Y(), (float)attachmentState.clearDataInt.Z(), (float)attachmentState.clearDataInt.W());
					break;
				case VulkanFrameBuffer::AttachmentFormat::UInt:
					clearData = V4Float32((float)attachmentState.clearDataUInt.X(), (float)attachmentState.clearDataUInt.Y(), (float)attachmentState.clearDataUInt.Z(), (float)attachmentState.clearDataUInt.W());
					break;
				case VulkanFrameBuffer::AttachmentFormat::Float:
					clearData = attachmentState.clearDataFloat;
					break;
				}
				_renderTargetClearValues[attachmentNo].color.float32[0] = clearData.X();
				_renderTargetClearValues[attachmentNo].color.float32[1] = clearData.Y();
				_renderTargetClearValues[attachmentNo].color.float32[2] = clearData.Z();
				_renderTargetClearValues[attachmentNo].color.float32[3] = clearData.W();
				break;
			}
			}
			break;
		}
		case IFrameBuffer::AttachmentType::Depth:
			if (attachmentState.clearDataFormat == VulkanFrameBuffer::AttachmentFormat::Int)
			{
				_renderTargetClearValues[attachmentNo].depthStencil.depth = (float)attachmentState.clearDataInt.X();
			}
			else if (attachmentState.clearDataFormat == VulkanFrameBuffer::AttachmentFormat::UInt)
			{
				_renderTargetClearValues[attachmentNo].depthStencil.depth = (float)attachmentState.clearDataUInt.X();
			}
			else
			{
				_renderTargetClearValues[attachmentNo].depthStencil.depth = attachmentState.clearDataFloat.X();
			}
			break;
		case IFrameBuffer::AttachmentType::Stencil:
			if (attachmentState.clearDataFormat == VulkanFrameBuffer::AttachmentFormat::Int)
			{
				_renderTargetClearValues[attachmentNo].depthStencil.stencil = (uint32_t)attachmentState.clearDataInt.X();
			}
			else if (attachmentState.clearDataFormat == VulkanFrameBuffer::AttachmentFormat::UInt)
			{
				_renderTargetClearValues[attachmentNo].depthStencil.stencil = attachmentState.clearDataUInt.X();
			}
			else
			{
				_renderTargetClearValues[attachmentNo].depthStencil.stencil = (uint32_t)attachmentState.clearDataFloat.X();
			}
			break;
		}
	}
	_renderPassBeginInfo.clearValueCount = (uint32_t)_renderTargetClearValues.size();
	_renderPassBeginInfo.pClearValues = _renderTargetClearValues.data();
}

//----------------------------------------------------------------------------------------
// Enabled state methods
//----------------------------------------------------------------------------------------
bool VulkanRenderPassNode::IsEnabled() const
{
	return _drawState.nodeEnabled;
}

//----------------------------------------------------------------------------------------
void VulkanRenderPassNode::SetIsEnabled(bool state)
{
	_buildState.nodeEnabled = state;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
void VulkanRenderPassNode::MigrateBuildStateToDrawState()
{
	// Migrate our build state
	if (!IsDrawStateCurrent())
	{
		_drawState = _buildState;
		_renderPassObjectCurrent = false;
		FlagDrawStateCurrent();
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
void VulkanRenderPassNode::ApplyFixedState(VkCommandBuffer commandBuffer)
{
	// If no framebuffer is bound, as will be the case with compute shaders, abort any further processing.
	if (_drawState.framebuffer == nullptr)
	{
		return;
	}

	// Update the render pass object if required
	int currentFramebufferLastUpdateToken = _drawState.framebuffer->GetFrameBufferObjectLastUpdateToken();
	if (!_renderPassObjectCurrent || (_previousFramebufferLastUpdateToken != currentFramebufferLastUpdateToken))
	{
		UpdateRenderPassObject();
		_renderPassObjectCurrent = true;
		_renderPassBeginInfoCurrent = false;
		_previousFramebufferLastUpdateToken = currentFramebufferLastUpdateToken;
	}

	// Update the render pass begin info if required
	int currentViewportLastUpdateToken = _drawState.framebuffer->GetViewportLastUpdateToken();
	if (!_renderPassBeginInfoCurrent || (_previousViewportLastUpdateToken != currentViewportLastUpdateToken))
	{
		UpdateRenderPassBeginInfo();
		_renderPassBeginInfoCurrent = true;
		_previousViewportLastUpdateToken = currentViewportLastUpdateToken;
	}
}

//----------------------------------------------------------------------------------------
void VulkanRenderPassNode::BeginRenderPass(VkCommandBuffer commandBuffer)
{
	// If no framebuffer is bound, as will be the case with compute shaders, abort any further processing.
	if (_drawState.framebuffer == nullptr)
	{
		return;
	}

	// Update the framebuffer in the render pass begin info structure. We need to do this even if the framebuffer hasn't
	// changed, as framebuffers bound to window surfaces have a separate framebuffer object for the front and back
	// buffers.
	_renderPassBeginInfo.framebuffer = _drawState.framebuffer->GetFramebuffer();

	// Begin the render pass
	vkCmdBeginRenderPass(commandBuffer, &_renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

//----------------------------------------------------------------------------------------
void VulkanRenderPassNode::EndRenderPass(VkCommandBuffer commandBuffer)
{
	// If no framebuffer is bound, as will be the case with compute shaders, abort any further processing.
	auto framebuffer = _drawState.framebuffer;
	if (framebuffer == nullptr)
	{
		return;
	}

	// End the render pass
	vkCmdEndRenderPass(commandBuffer);

	// Complete any resolve operations required by the framebuffer. If we have "true" resolve operations, we don't need
	// to do anything here. A resolve target is expressly forbidden in Vulkan when the sample count is set to 1 however,
	// and we want our API to be able to support resolve targets with a sample count of 1. In order to make this
	// possible, we fall back to a simple texture copy in this case.
	if (framebuffer->HasResolveTargets() && (framebuffer->GetSampleCount() == ITextureBuffer::SampleCount::SampleCount1))
	{
		for (const auto& entry : _drawState.attachmentState)
		{
			if (entry.multiSampleResolutionEnabled)
			{
				framebuffer->CompleteRenderPassResolveOperationWithCopy(commandBuffer, entry.type, entry.index, entry.multiSampleResolveTargetType, entry.multiSampleResolveTargetIndex);
			}
		}
	}
}

//----------------------------------------------------------------------------------------
// Debug methods
//----------------------------------------------------------------------------------------
void VulkanRenderPassNode::SetDebugName(const Marshal::In<std::string>& name)
{
	_buildState.debugName = name;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
const std::string& VulkanRenderPassNode::DebugName() const
{
	return _drawState.debugName;
}

} // namespace cobalt::graphics
