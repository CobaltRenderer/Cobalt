// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "OpenGLStateGroupNode.h"
#include "OpenGLDebug.h"
#include "OpenGLRenderableNode.h"
#include "OpenGLRenderer.h"
#include <Internal/RendererSupport/KnownDynamicCast.h>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
OpenGLStateGroupNode::OpenGLStateGroupNode(cobalt::logging::ILogger* log, OpenGLRenderer* renderer)
: OpenGLStateContainer(log), _renderer(renderer), _log(log)
{
	_hasModifiedChildNodes = false;
	_buildState.depthTestEnabled = true;
	_buildState.depthWriteEnabled = true;
	_buildState.depthBiasEnabled = false;
	_buildState.depthBiasConstantFactor = 0.0f;
	_buildState.depthBiasSlopeFactor = 0.0f;
	_buildState.depthBiasClamp = 0.0f;
	_buildState.depthComparisonTest = IStateGroupNode::DepthComparisonFunction::Less;
	_buildState.nativeDepthComparisonTest = GetNativeDepthComparisonFunction(_buildState.depthComparisonTest);
	_buildState.polygonFillMode = PolygonFillMode::Solid;
	_buildState.polygonCullMode = PolygonCullMode::None;
	_buildState.polygonWindingOrder = PolygonWindingOrder::CounterClockwise;
	_buildState.blendEnabled = false;
	_buildState.sharedBlendState = BuildBlendState(BlendOperation::Add, BlendFactor::One, BlendFactor::Zero, BlendOperation::Add, BlendFactor::One, BlendFactor::Zero);
	_buildState.stencilEnabled = false;
	_buildState.stencilCompareMask = 0;
	_buildState.stencilWriteMask = 0;
	_buildState.stencilReferenceValue = 0;
	_buildState.stencilFrontFace = BuildStencilFaceState(StencilComparisonFunction::Always, IStateGroupNode::StencilOperation::Keep, IStateGroupNode::StencilOperation::Keep, IStateGroupNode::StencilOperation::Keep);
	_buildState.stencilBackFace = _buildState.stencilFrontFace;
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void OpenGLStateGroupNode::Delete()
{
	_renderer->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
// Child node methods
//----------------------------------------------------------------------------------------
void OpenGLStateGroupNode::AddChildNode(IRenderableNode* childNode)
{
	// Ensure the specified node is in a valid state to be added as a child node
	auto* childNodeResolved = KnownDynamicCast<OpenGLRenderableNode*>(childNode);
	if (!childNodeResolved->SetAsChildNode())
	{
		_log->Error("Failed to add renderable node as child");
		return;
	}

	// Add the child node
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	auto childNodeID = (uint32_t)_buildChildNodes.size();
	_buildChildNodes.push_back(childNodeResolved);
	_hasModifiedChildNodes = true;
	lock.unlock();
	childNodeResolved->SetChildNodeId(childNodeID);

	// Flag that the build state has been modified
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void OpenGLStateGroupNode::AddChildNodes(IRenderableNode* const* childNodes, size_t childNodeCount)
{
	// Add each of the specified child nodes to this group node
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	size_t currentChildCount = _buildChildNodes.size();
	size_t newChildCount = currentChildCount + childNodeCount;
	_buildChildNodes.resize(newChildCount);
	for (size_t i = currentChildCount; i < newChildCount; ++i)
	{
		// Ensure the specified node is in a valid state to be added as a child node
		auto* childNodeResolved = KnownDynamicCast<OpenGLRenderableNode*>(*(childNodes++));
		if (!childNodeResolved->SetAsChildNode())
		{
			_log->Error("Failed to add renderable node as child");
			continue;
		}

		// Add the child node
		auto childNodeID = (uint32_t)i;
		_buildChildNodes[i] = childNodeResolved;
		_hasModifiedChildNodes = true;
		childNodeResolved->SetChildNodeId(childNodeID);
	}
	lock.unlock();

	// Flag that the build state has been modified
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void OpenGLStateGroupNode::AddChildNodes(IRenderableNode::unique_ptr const* childNodes, size_t childNodeCount)
{
	// Add each of the specified child nodes to this group node
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	size_t currentChildCount = _buildChildNodes.size();
	size_t newChildCount = currentChildCount + childNodeCount;
	_buildChildNodes.resize(newChildCount);
	for (size_t i = currentChildCount; i < newChildCount; ++i)
	{
		// Ensure the specified node is in a valid state to be added as a child node
		auto* childNodeResolved = KnownDynamicCast<OpenGLRenderableNode*>((childNodes++)->get());
		if (!childNodeResolved->SetAsChildNode())
		{
			_log->Error("Failed to add renderable node as child");
			continue;
		}

		// Add the child node
		auto childNodeID = (uint32_t)i;
		_buildChildNodes[i] = childNodeResolved;
		_hasModifiedChildNodes = true;
		childNodeResolved->SetChildNodeId(childNodeID);
	}
	lock.unlock();

	// Flag that the build state has been modified
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void OpenGLStateGroupNode::RemoveChildNode(IRenderableNode* childNode)
{
	// Retrieve the ID of the specified child node
	auto* childNodeResolved = KnownDynamicCast<OpenGLRenderableNode*>(childNode);
	auto childNodeID = childNodeResolved->GetChildNodeId();

	// Attempt to remove the specified child node
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	if (!RemoveChildNode(childNodeResolved, childNodeID))
	{
		_log->Error("Failed to remove renderable child node with ID {0}.", childNodeID);
		return;
	}
	lock.unlock();

	// Notify the removed renderable that it is no longer being held as child nodes. This is only used for extra debug
	// checks to detect incorrect scene tree usage.
	if (_renderer->DebugLoggingEnabled())
	{
		childNodeResolved->RemoveAsChildNode();
	}

	// Flag that the build state has been modified
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
bool OpenGLStateGroupNode::RemoveChildNode(OpenGLRenderableNode* childNode, uint32_t childNodeID)
{
	// Ensure the specified child node ID is valid
	if (childNodeID >= (uint32_t)_buildChildNodes.size())
	{
		_log->Error("Tried to remove renderable child node with ID {0}, but only {1} child nodes are present.", childNodeID, (uint32_t)_buildChildNodes.size());
		return false;
	}

	// Ensure the specified child node is in the slot it claims to be assigned to
	OpenGLRenderableNode* actualChildNodeInSlot = _buildChildNodes[childNodeID];
	if (actualChildNodeInSlot != childNode)
	{
		_log->Error("Tried to remove renderable child node with ID {0}, but a different node was present in that slot.", childNodeID);
		return false;
	}

	// Clear the child node slot, and mark it as removed. Note that we do a sorted insertion here inside the lock. We
	// could instead leave the list of removed nodes unsorted, and sort it once on the render thread, which is more
	// efficient overall. Both methods have been profiled. The two approaches were equivalent for small numbers of
	// removals or large numbers of removals which occur in the same order of addition, with 500000 objects removed per
	// frame. When those 500000 objects were removed in reverse order of addition, sorting on the render thread was
	// about 20% more efficient overall, however the amount of additional time taken on the render thread doubled. In
	// the interests of minimizing overhead on the render thread for large object removals, to maintain a more steady
	// framerate, we opt of the additional work on the calling application thread here.
	_removedChildNodeIDs.insert(std::upper_bound(_removedChildNodeIDs.begin(), _removedChildNodeIDs.end(), childNodeID), childNodeID);
	_buildChildNodes[childNodeID] = nullptr;
	_hasModifiedChildNodes = true;
	return true;
}

//----------------------------------------------------------------------------------------
void OpenGLStateGroupNode::RemoveChildNodes(IRenderableNode* const* childNodes, size_t childNodeCount)
{
	// Remove each of the specified child nodes from this group node
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	for (size_t i = 0; i < childNodeCount; ++i)
	{
		// Retrieve the ID of the specified child node
		auto* childNodeResolved = KnownDynamicCast<OpenGLRenderableNode*>(*(childNodes++));
		auto childNodeID = childNodeResolved->GetChildNodeId();

		// Attempt to remove the specified child node
		if (!RemoveChildNode(childNodeResolved, childNodeID))
		{
			_log->Error("Failed to remove renderable child node with ID {0}.", childNodeID);
			continue;
		}

		// Notify the removed renderable that it is no longer being held as child nodes. This is only used for extra
		// debug checks to detect incorrect scene tree usage.
		if (_renderer->DebugLoggingEnabled())
		{
			childNodeResolved->RemoveAsChildNode();
		}
	}
	lock.unlock();

	// Flag that the build state has been modified
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void OpenGLStateGroupNode::RemoveChildNodes(IRenderableNode::unique_ptr const* childNodes, size_t childNodeCount)
{
	// Remove each of the specified child nodes from this group node
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	for (size_t i = 0; i < childNodeCount; ++i)
	{
		// Retrieve the ID of the specified child node
		auto* childNodeResolved = KnownDynamicCast<OpenGLRenderableNode*>((childNodes++)->get());
		auto childNodeID = childNodeResolved->GetChildNodeId();

		// Attempt to remove the specified child node
		if (!RemoveChildNode(childNodeResolved, childNodeID))
		{
			_log->Error("Failed to remove renderable child node with ID {0}.", childNodeID);
			continue;
		}

		// Notify the removed renderable that it is no longer being held as child nodes. This is only used for extra
		// debug checks to detect incorrect scene tree usage.
		if (_renderer->DebugLoggingEnabled())
		{
			childNodeResolved->RemoveAsChildNode();
		}
	}
	lock.unlock();

	// Flag that the build state has been modified
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void OpenGLStateGroupNode::RemoveAllChildNodes()
{
	// Notify removed renderables that they are no longer being held as child nodes. This is only used for extra debug
	// checks to detect incorrect scene tree usage.
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	if (_renderer->DebugLoggingEnabled())
	{
		for (auto* childNode : _buildChildNodes)
		{
			if (childNode != nullptr)
			{
				childNode->RemoveAsChildNode();
			}
		}
	}

	// Erase all child nodes
	_buildChildNodes.clear();
	_hasModifiedChildNodes = true;
	_removedChildNodeIDs.clear();
	lock.unlock();

	// Flag that the build state has been modified
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void OpenGLStateGroupNode::SetChildNodes(IRenderableNode* const* childNodes, size_t childNodeCount)
{
	// Notify removed renderables that they are no longer being held as child nodes. This is only used for extra debug
	// checks to detect incorrect scene tree usage.
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	if (_renderer->DebugLoggingEnabled())
	{
		for (auto* childNode : _buildChildNodes)
		{
			if (childNode != nullptr)
			{
				childNode->RemoveAsChildNode();
			}
		}
	}

	// Add each of the specified child nodes to this group node, replacing any existing child node content.
	_hasModifiedChildNodes = true;
	_removedChildNodeIDs.clear();
	_buildChildNodes.resize(childNodeCount);
	for (size_t i = 0; i < childNodeCount; ++i)
	{
		// Ensure the specified node is in a valid state to be added as a child node
		auto* childNodeResolved = KnownDynamicCast<OpenGLRenderableNode*>(*(childNodes++));
		if (!childNodeResolved->SetAsChildNode())
		{
			_log->Error("Failed to add renderable node as child");
			continue;
		}

		// Add the child node
		auto childNodeID = (uint32_t)i;
		_buildChildNodes[i] = childNodeResolved;
		_hasModifiedChildNodes = true;
		childNodeResolved->SetChildNodeId(childNodeID);
	}
	lock.unlock();

	// Flag that the build state has been modified
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void OpenGLStateGroupNode::SetChildNodes(IRenderableNode::unique_ptr const* childNodes, size_t childNodeCount)
{
	// Notify removed renderables that they are no longer being held as child nodes. This is only used for extra debug
	// checks to detect incorrect scene tree usage.
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	if (_renderer->DebugLoggingEnabled())
	{
		for (auto* childNode : _buildChildNodes)
		{
			if (childNode != nullptr)
			{
				childNode->RemoveAsChildNode();
			}
		}
	}

	// Add each of the specified child nodes to this group node, replacing any existing child node content.
	_hasModifiedChildNodes = true;
	_removedChildNodeIDs.clear();
	_buildChildNodes.resize(childNodeCount);
	for (size_t i = 0; i < childNodeCount; ++i)
	{
		// Ensure the specified node is in a valid state to be added as a child node
		auto* childNodeResolved = KnownDynamicCast<OpenGLRenderableNode*>((childNodes++)->get());
		if (!childNodeResolved->SetAsChildNode())
		{
			_log->Error("Failed to add renderable node as child");
			continue;
		}

		// Add the child node
		auto childNodeID = (uint32_t)i;
		_buildChildNodes[i] = childNodeResolved;
		_hasModifiedChildNodes = true;
		childNodeResolved->SetChildNodeId(childNodeID);
	}
	lock.unlock();

	// Flag that the build state has been modified
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
const std::vector<OpenGLRenderableNode*>& OpenGLStateGroupNode::GetChildNodes() const
{
	return _drawChildNodes;
}

//----------------------------------------------------------------------------------------
// Compute methods
//----------------------------------------------------------------------------------------
bool OpenGLStateGroupNode::HasComputeTask() const
{
	return _drawState.computeTaskDefined;
}

//----------------------------------------------------------------------------------------
V3UInt32 OpenGLStateGroupNode::GetComputeThreadGroupCounts() const
{
	return _drawState.computeThreadGroupCounts;
}

//----------------------------------------------------------------------------------------
void OpenGLStateGroupNode::SetComputeTask(const V3UInt32& threadGroupCounts)
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	_buildState.computeTaskDefined = true;
	_buildState.computeThreadGroupCounts = threadGroupCounts;
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void OpenGLStateGroupNode::RemoveComputeTask()
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	_buildState.computeTaskDefined = false;
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
// Depth state methods
//----------------------------------------------------------------------------------------
void OpenGLStateGroupNode::SetDepthTestEnabled(bool state)
{
	_buildState.depthTestEnabled = state;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void OpenGLStateGroupNode::SetDepthWriteEnabled(bool state)
{
	_buildState.depthWriteEnabled = state;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void OpenGLStateGroupNode::SetDepthComparisonFunction(DepthComparisonFunction comparisonTest)
{
	_buildState.depthComparisonTest = comparisonTest;
	_buildState.nativeDepthComparisonTest = GetNativeDepthComparisonFunction(comparisonTest);
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void OpenGLStateGroupNode::SetDepthBias(float constantFactor, float slopeFactor, float clamp)
{
	_buildState.depthBiasEnabled = true;
	_buildState.depthBiasConstantFactor = constantFactor;
	_buildState.depthBiasSlopeFactor = slopeFactor;
	_buildState.depthBiasClamp = clamp;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void OpenGLStateGroupNode::ClearDepthBias()
{
	_buildState.depthBiasEnabled = false;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
// Stencil state methods
//----------------------------------------------------------------------------------------
void OpenGLStateGroupNode::SetStencilTestEnabled(bool state, uint32_t compareMask, uint32_t writeMask)
{
	_buildState.stencilEnabled = state;
	_buildState.stencilCompareMask = compareMask;
	_buildState.stencilWriteMask = writeMask;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void OpenGLStateGroupNode::SetStencilOperation(StencilTargetFace targetFace, StencilComparisonFunction comparisonTest, StencilOperation passOperation, StencilOperation failOperation, StencilOperation depthFailOperation)
{
	if ((targetFace == IStateGroupNode::StencilTargetFace::FrontFace) || (targetFace == IStateGroupNode::StencilTargetFace::FrontAndBackFace))
	{
		_buildState.stencilFrontFace = BuildStencilFaceState(comparisonTest, passOperation, failOperation, depthFailOperation);
	}
	if ((targetFace == IStateGroupNode::StencilTargetFace::BackFace) || (targetFace == IStateGroupNode::StencilTargetFace::FrontAndBackFace))
	{
		_buildState.stencilBackFace = BuildStencilFaceState(comparisonTest, passOperation, failOperation, depthFailOperation);
	}
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void OpenGLStateGroupNode::SetStencilReferenceValue(uint32_t referenceValue)
{
	_buildState.stencilReferenceValue = referenceValue;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
// Rasterization state methods
//----------------------------------------------------------------------------------------
void OpenGLStateGroupNode::SetPolygonFillMode(PolygonFillMode fillMode)
{
	_buildState.polygonFillMode = fillMode;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void OpenGLStateGroupNode::SetPolygonCullMode(PolygonCullMode cullMode)
{
	_buildState.polygonCullMode = cullMode;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void OpenGLStateGroupNode::SetPolygonWindingOrder(PolygonWindingOrder windingOrder)
{
	_buildState.polygonWindingOrder = windingOrder;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
// Blend state methods
//----------------------------------------------------------------------------------------
void OpenGLStateGroupNode::SetBlendEnabled(bool state)
{
	_buildState.blendEnabled = state;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void OpenGLStateGroupNode::SetBlendMode(BlendOperation blendOperationRGB, BlendFactor blendFactorSourceRGB, BlendFactor blendFactorDestinationRGB, BlendOperation blendOperationA, BlendFactor blendFactorSourceA, BlendFactor blendFactorDestinationA)
{
	BlendState blendState = BuildBlendState(blendOperationRGB, blendFactorSourceRGB, blendFactorDestinationRGB, blendOperationA, blendFactorSourceA, blendFactorDestinationA);
	_buildState.sharedBlendState = blendState;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void OpenGLStateGroupNode::SetBlendMode(IFrameBuffer::AttachmentType type, size_t index, BlendOperation blendOperationRGB, BlendFactor blendFactorSourceRGB, BlendFactor blendFactorDestinationRGB, BlendOperation blendOperationA, BlendFactor blendFactorSourceA, BlendFactor blendFactorDestinationA)
{
	BlendState blendState = BuildBlendState(blendOperationRGB, blendFactorSourceRGB, blendFactorDestinationRGB, blendOperationA, blendFactorSourceA, blendFactorDestinationA);
	blendState.type = type;
	blendState.index = index;
	blendState.nativeBufferType = AttachmentTypeToBufferType(type);
	blendState.nativeBufferIndex = (GLuint)index;

	bool foundEntry = false;
	for (auto& entry : _buildState.attachmentTypeBlendState)
	{
		if ((entry.type == type) && (entry.index == index))
		{
			entry = blendState;
			foundEntry = true;
			break;
		}
	}
	if (!foundEntry)
	{
		_buildState.attachmentTypeBlendState.push_back(blendState);
	}
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
void OpenGLStateGroupNode::MigrateBuildStateToDrawState()
{
	// Migrate our build state
	if (!IsDrawStateCurrent())
	{
		// Transfer basic build state
		OpenGLStateContainer::MigrateBuildStateToDrawState();
		_drawState = _buildState;

		// Transfer child node state only if it has changed. We do this for efficiency. This implementation gives us
		// extremely fast addition and removal of individual nodes, and the vectors will size themselves naturally to
		// have enough capacity to end up allocation free.
		if (_hasModifiedChildNodes)
		{
			if (!_removedChildNodeIDs.empty())
			{
				// Retrieve the index of the first child node which was removed. Note that the list of removed child
				// node indices has been pre-sorted from lowest to highest.
				uint32_t firstRemovedChildNodeIndex = _removedChildNodeIDs.front();

				// Transfer the remaining valid child node entries to our draw array
				uint32_t nextWriteIndex = 0;
				uint32_t nextReadIndex = 0;
				auto removedChildNodeCount = (uint32_t)_removedChildNodeIDs.size();
				uint32_t newChildNodeCount = (uint32_t)_buildChildNodes.size() - removedChildNodeCount;
				_drawChildNodes.resize(newChildNodeCount);
				uint32_t removedChildNodeSearchIndex = 0;
				while (removedChildNodeSearchIndex < removedChildNodeCount)
				{
					uint32_t removedIndex = _removedChildNodeIDs[removedChildNodeSearchIndex++];
					while ((removedIndex == nextReadIndex) && (removedChildNodeSearchIndex < removedChildNodeCount))
					{
						removedIndex = _removedChildNodeIDs[removedChildNodeSearchIndex++];
						++nextReadIndex;
					}
					std::copy(_buildChildNodes.data() + nextReadIndex, _buildChildNodes.data() + removedIndex, _drawChildNodes.data() + nextWriteIndex);
					nextWriteIndex += removedIndex - nextReadIndex;
					nextReadIndex = removedIndex + 1;
				}
				std::copy(_buildChildNodes.data() + nextReadIndex, _buildChildNodes.data() + _buildChildNodes.size(), _drawChildNodes.data() + nextWriteIndex);
				_removedChildNodeIDs.clear();

				// Since we had removed entries, transfer our compacted child node entries back to our build array.
				_buildChildNodes.resize(newChildNodeCount);
				std::copy(_drawChildNodes.data(), _drawChildNodes.data() + newChildNodeCount, _buildChildNodes.data());

				// Update child node IDs for all relocated nodes in the buffer
				for (auto i = firstRemovedChildNodeIndex; i < newChildNodeCount; ++i)
				{
					_buildChildNodes[i]->SetChildNodeId(i);
				}
			}
			else
			{
				// Transfer our child node entries to our draw array
				auto newChildNodeCount = (uint32_t)_buildChildNodes.size();
				_drawChildNodes.resize(newChildNodeCount);
				std::copy(_buildChildNodes.data(), _buildChildNodes.data() + newChildNodeCount, _drawChildNodes.data());
			}
			_hasModifiedChildNodes = false;
		}

		// Flag that our draw state is now current
		FlagDrawStateCurrent();
	}

	// Migrate build state in our child nodes
	for (OpenGLRenderableNode* entry : _drawChildNodes)
	{
		entry->MigrateBuildStateToDrawState();
	}
}

//----------------------------------------------------------------------------------------
void OpenGLStateGroupNode::ApplyFixedState(OpenGLFrameBuffer* framebuffer)
{
	// Set the blend state
	if (_drawState.blendEnabled)
	{
		glEnable(GL_BLEND);
		glBlendEquationSeparate(_drawState.sharedBlendState.nativeBlendOperationRGB, _drawState.sharedBlendState.nativeBlendOperationA);
		glBlendFuncSeparate(_drawState.sharedBlendState.nativeBlendFactorSourceRGB, _drawState.sharedBlendState.nativeBlendFactorDestinationRGB, _drawState.sharedBlendState.nativeBlendFactorSourceA, _drawState.sharedBlendState.nativeBlendFactorDestinationA);

		for (const auto& blendState : _drawState.attachmentTypeBlendState)
		{
#if defined(GL_VERSION_4_0)
			glBlendEquationSeparatei(blendState.nativeBufferIndex, blendState.nativeBlendOperationRGB, blendState.nativeBlendOperationA);
			glBlendFuncSeparatei(blendState.nativeBufferIndex, blendState.nativeBlendFactorSourceRGB, blendState.nativeBlendFactorDestinationRGB, blendState.nativeBlendFactorSourceA, blendState.nativeBlendFactorDestinationA);
#elif defined(GL_ARB_draw_buffers_blend)
			glBlendEquationSeparateiARB(blendState.nativeBufferIndex, blendState.nativeBlendOperationRGB, blendState.nativeBlendOperationA);
			glBlendFuncSeparateiARB(blendState.nativeBufferIndex, blendState.nativeBlendFactorSourceRGB, blendState.nativeBlendFactorDestinationRGB, blendState.nativeBlendFactorSourceA, blendState.nativeBlendFactorDestinationA);
#endif
		}
	}
	else
	{
		glDisable(GL_BLEND);
	}

	// Enable or disable depth writes
	bool hasBoundDepthBuffer = framebuffer->HasBoundDepthBuffer();
	glDepthMask((hasBoundDepthBuffer && _drawState.depthWriteEnabled) ? GL_TRUE : GL_FALSE);

	// Set the depth test state. Note that we need to check if the framebuffer has a depth buffer attached here too,
	// rather than just taking the depth test state directly. There's a subtle but important reason we do this. Depth
	// testing is enabled by default, but not all framebuffers may have a depth buffer attached. As per the OpenGL spec,
	// if no depth buffer is attached the behaviour is the same as if depth testing is disabled, so it might appear that
	// we don't need to care if depth testing is enabled when no depth buffer is present. When binding directly to a
	// window surface however, we don't have full control over the framebuffer format. A perfectly valid OpenGL driver
	// implementation isn't required to provide any pixel formats for the surface which don't include a depth buffer. We
	// may request a depth buffer with 0 bits in a call to ChoosePixelFormat, but the selected format can include a
	// depth buffer regardless. We have encountered this case with AMD drivers. To deal with this possibility, we need
	// to explicitly disable depth testing where a depth buffer is not present, otherwise it may be active on an
	// uninitialized buffer when binding to window surfaces.
	if (hasBoundDepthBuffer && _drawState.depthTestEnabled)
	{
		glEnable(GL_DEPTH_TEST);
	}
	else
	{
		glDisable(GL_DEPTH_TEST);
	}

	// Set the depth test operation
	glDepthFunc(_drawState.nativeDepthComparisonTest);

	// Set depth bias state
	if (_drawState.depthBiasEnabled)
	{
		glEnable(GL_POLYGON_OFFSET_FILL);
		glEnable(GL_POLYGON_OFFSET_LINE);
		glEnable(GL_POLYGON_OFFSET_POINT);
#ifdef GL_EXT_polygon_offset_clamp
		if (GLAD_GL_EXT_polygon_offset_clamp != 0)
		{
			glPolygonOffsetClampEXT(_drawState.depthBiasSlopeFactor, _drawState.depthBiasConstantFactor, _drawState.depthBiasClamp);
		}
		else
#endif
		{
			glPolygonOffset(_drawState.depthBiasSlopeFactor, _drawState.depthBiasConstantFactor);
		}
	}
	else
	{
		glDisable(GL_POLYGON_OFFSET_FILL);
		glDisable(GL_POLYGON_OFFSET_LINE);
		glDisable(GL_POLYGON_OFFSET_POINT);
	}

	// Configure stencil test settings based on the supplied state
	if (_drawState.stencilEnabled)
	{
		glEnable(GL_STENCIL_TEST);
		glStencilMaskSeparate(GL_FRONT_AND_BACK, _drawState.stencilWriteMask);
		glStencilFuncSeparate(GL_FRONT, _drawState.stencilFrontFace.nativeComparisonTest, _drawState.stencilReferenceValue, _drawState.stencilCompareMask);
		glStencilOpSeparate(GL_FRONT, _drawState.stencilFrontFace.nativeFailOperation, _drawState.stencilFrontFace.nativeDepthFailOperation, _drawState.stencilFrontFace.nativePassOperation);
		glStencilFuncSeparate(GL_BACK, _drawState.stencilBackFace.nativeComparisonTest, _drawState.stencilReferenceValue, _drawState.stencilCompareMask);
		glStencilOpSeparate(GL_BACK, _drawState.stencilBackFace.nativeFailOperation, _drawState.stencilBackFace.nativeDepthFailOperation, _drawState.stencilBackFace.nativePassOperation);
	}
	else
	{
		glDisable(GL_STENCIL_TEST);
	}

	// Set the polygon winding order
	glFrontFace(_drawState.polygonWindingOrder == PolygonWindingOrder::Clockwise ? GL_CW : GL_CCW);

	// Set the polygon cull mode
	if (_drawState.polygonCullMode == PolygonCullMode::None)
	{
		glDisable(GL_CULL_FACE);
	}
	else
	{
		glEnable(GL_CULL_FACE);
		glCullFace(_drawState.polygonCullMode == PolygonCullMode::Back ? GL_BACK : GL_FRONT);
	}

	// Set the polygon fill mode
	glPolygonMode(GL_FRONT_AND_BACK, (_drawState.polygonFillMode == IStateGroupNode::PolygonFillMode::Solid ? GL_FILL : GL_LINE));

	// Configure multisampling
	auto sampleCount = framebuffer->GetSampleCount();
	if (sampleCount == ITextureBuffer::SampleCount::SampleCount1)
	{
		glDisable(GL_MULTISAMPLE);
	}
	else
	{
		glEnable(GL_MULTISAMPLE);
	}
	CheckGLError(_log);
}

//----------------------------------------------------------------------------------------
OpenGLStateGroupNode::BlendState OpenGLStateGroupNode::BuildBlendState(BlendOperation blendOperationRGB, BlendFactor blendFactorSourceRGB, BlendFactor blendFactorDestinationRGB, BlendOperation blendOperationA, BlendFactor blendFactorSourceA, BlendFactor blendFactorDestinationA)
{
	BlendState blendState = {};
	blendState.blendOperationRGB = blendOperationRGB;
	blendState.blendFactorSourceRGB = blendFactorSourceRGB;
	blendState.blendFactorDestinationRGB = blendFactorDestinationRGB;
	blendState.blendOperationA = blendOperationA;
	blendState.blendFactorSourceA = blendFactorSourceA;
	blendState.blendFactorDestinationA = blendFactorDestinationA;
	blendState.nativeBlendOperationRGB = GetNativeBlendOperation(blendState.blendOperationRGB);
	blendState.nativeBlendFactorSourceRGB = GetNativeBlendFactor(blendState.blendFactorSourceRGB);
	blendState.nativeBlendFactorDestinationRGB = GetNativeBlendFactor(blendState.blendFactorDestinationRGB);
	blendState.nativeBlendOperationA = GetNativeBlendOperation(blendState.blendOperationA);
	blendState.nativeBlendFactorSourceA = GetNativeBlendFactor(blendState.blendFactorSourceA);
	blendState.nativeBlendFactorDestinationA = GetNativeBlendFactor(blendState.blendFactorDestinationA);
	return blendState;
}

//----------------------------------------------------------------------------------------
OpenGLStateGroupNode::StencilFaceState OpenGLStateGroupNode::BuildStencilFaceState(StencilComparisonFunction comparisonTest, StencilOperation passOperation, StencilOperation failOperation, StencilOperation depthFailOperation)
{
	StencilFaceState stencilState = {};
	stencilState.comparisonTest = comparisonTest;
	stencilState.passOperation = passOperation;
	stencilState.failOperation = failOperation;
	stencilState.depthFailOperation = depthFailOperation;
	stencilState.nativeComparisonTest = GetNativeStencilComparisonFunction(comparisonTest);
	stencilState.nativePassOperation = GetNativeStencilOperation(passOperation);
	stencilState.nativeFailOperation = GetNativeStencilOperation(failOperation);
	stencilState.nativeDepthFailOperation = GetNativeStencilOperation(depthFailOperation);
	return stencilState;
}

//----------------------------------------------------------------------------------------
constexpr GLenum OpenGLStateGroupNode::GetNativeBlendOperation(BlendOperation operation)
{
	switch (operation)
	{
	case IStateGroupNode::BlendOperation::Add:
		return GL_FUNC_ADD;
	case IStateGroupNode::BlendOperation::Subtract:
		return GL_FUNC_SUBTRACT;
	case IStateGroupNode::BlendOperation::ReverseSubtract:
		return GL_FUNC_REVERSE_SUBTRACT;
	case IStateGroupNode::BlendOperation::Min:
		return GL_MIN;
	case IStateGroupNode::BlendOperation::Max:
		return GL_MAX;
	}
	UNREACHABLE();
	return {};
}

//----------------------------------------------------------------------------------------
constexpr GLenum OpenGLStateGroupNode::GetNativeBlendFactor(BlendFactor factor)
{
	switch (factor)
	{
	case IStateGroupNode::BlendFactor::Zero:
		return GL_ZERO;
	case IStateGroupNode::BlendFactor::One:
		return GL_ONE;
	case IStateGroupNode::BlendFactor::SourceColor:
		return GL_SRC_COLOR;
	case IStateGroupNode::BlendFactor::OneMinusSourceColor:
		return GL_ONE_MINUS_SRC_COLOR;
	case IStateGroupNode::BlendFactor::DestinationColor:
		return GL_DST_COLOR;
	case IStateGroupNode::BlendFactor::OneMinusDestinationColor:
		return GL_ONE_MINUS_DST_COLOR;
	case IStateGroupNode::BlendFactor::SourceAlpha:
		return GL_SRC_ALPHA;
	case IStateGroupNode::BlendFactor::OneMinusSourceAlpha:
		return GL_ONE_MINUS_SRC_ALPHA;
	case IStateGroupNode::BlendFactor::DestinationAlpha:
		return GL_DST_ALPHA;
	case IStateGroupNode::BlendFactor::OneMinusDestinationAlpha:
		return GL_ONE_MINUS_DST_ALPHA;
	}
	UNREACHABLE();
	return {};
}

//----------------------------------------------------------------------------------------
constexpr GLenum OpenGLStateGroupNode::GetNativeDepthComparisonFunction(DepthComparisonFunction function)
{
	switch (function)
	{
	case DepthComparisonFunction::Never:
		return GL_NEVER;
	case DepthComparisonFunction::Equal:
		return GL_EQUAL;
	case DepthComparisonFunction::NotEqual:
		return GL_NOTEQUAL;
	case DepthComparisonFunction::Less:
		return GL_LESS;
	case DepthComparisonFunction::LessOrEqual:
		return GL_LEQUAL;
	case DepthComparisonFunction::Greater:
		return GL_GREATER;
	case DepthComparisonFunction::GreaterOrEqual:
		return GL_GEQUAL;
	case DepthComparisonFunction::Always:
		return GL_ALWAYS;
	}
	UNREACHABLE();
	return {};
}

//----------------------------------------------------------------------------------------
constexpr GLenum OpenGLStateGroupNode::GetNativeStencilComparisonFunction(StencilComparisonFunction function)
{
	switch (function)
	{
	case StencilComparisonFunction::Never:
		return GL_NEVER;
	case StencilComparisonFunction::Equal:
		return GL_EQUAL;
	case StencilComparisonFunction::NotEqual:
		return GL_NOTEQUAL;
	case StencilComparisonFunction::Less:
		return GL_LESS;
	case StencilComparisonFunction::LessOrEqual:
		return GL_LEQUAL;
	case StencilComparisonFunction::Greater:
		return GL_GREATER;
	case StencilComparisonFunction::GreaterOrEqual:
		return GL_GEQUAL;
	case StencilComparisonFunction::Always:
		return GL_ALWAYS;
	}
	UNREACHABLE();
	return {};
}

//----------------------------------------------------------------------------------------
constexpr GLenum OpenGLStateGroupNode::GetNativeStencilOperation(StencilOperation operation)
{
	switch (operation)
	{
	case StencilOperation::Keep:
		return GL_KEEP;
	case StencilOperation::Zero:
		return GL_ZERO;
	case StencilOperation::Replace:
		return GL_REPLACE;
	case StencilOperation::IncrementAndClamp:
		return GL_INCR;
	case StencilOperation::DecrementAndClamp:
		return GL_DECR;
	case StencilOperation::IncrementAndWrap:
		return GL_INCR_WRAP;
	case StencilOperation::DecrementAndWrap:
		return GL_DECR_WRAP;
	case StencilOperation::Invert:
		return GL_INVERT;
	}
	UNREACHABLE();
	return {};
}

//----------------------------------------------------------------------------------------
constexpr GLenum OpenGLStateGroupNode::AttachmentTypeToBufferType(IFrameBuffer::AttachmentType type)
{
	switch (type)
	{
	case IFrameBuffer::AttachmentType::Color:
		return GL_COLOR;
	case IFrameBuffer::AttachmentType::Depth:
		return GL_DEPTH;
	}
	UNREACHABLE();
	return {};
}

//----------------------------------------------------------------------------------------
// Debug methods
//----------------------------------------------------------------------------------------
void OpenGLStateGroupNode::SetDebugName(const Marshal::In<std::string>& name)
{
	_buildState.debugName = name;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
const std::string& OpenGLStateGroupNode::DebugName() const
{
	return _drawState.debugName;
}

} // namespace cobalt::graphics
