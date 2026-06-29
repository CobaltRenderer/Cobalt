// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "Direct3DStateGroupNode.h"
#include "Direct3DRenderableNode.h"
#include "Direct3DRenderer.h"
#include <Internal/RendererSupport/KnownDynamicCast.h>
#include <Internal/RendererSupport/UnicodeConversion.h>
#include <algorithm>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
Direct3DStateGroupNode::Direct3DStateGroupNode(cobalt::logging::ILogger* log, Direct3DRenderer* renderer)
: Direct3DStateContainer(log), _renderer(renderer), _log(log)
{
	_buildState.depthTestEnabled = true;
	_buildState.depthWriteEnabled = true;
	_buildState.depthBiasEnabled = false;
	_buildState.depthBiasConstantFactor = 0.0f;
	_buildState.depthBiasSlopeFactor = 0.0f;
	_buildState.depthBiasClamp = 0.0f;
	_buildState.depthComparisonTest = IStateGroupNode::DepthComparisonFunction::Less;
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
	_buildStateChanged = false;

	_computePipelineStateBucket.drawNativeObjectsCurrentForBucket.resize(1, 0u);
	_computePipelineStateBucket.pipelineStateObjects.resize(1);
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void Direct3DStateGroupNode::Delete()
{
	_renderer->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
// Child node methods
//----------------------------------------------------------------------------------------
void Direct3DStateGroupNode::AddChildNode(IRenderableNode* childNode)
{
	// Ensure the specified node is in a valid state to be added as a child node
	auto* childNodeResolved = KnownDynamicCast<Direct3DRenderableNode*>(childNode);
	if (!childNodeResolved->SetAsChildNode())
	{
		_log->Error("Failed to add renderable node as child");
	}

	// Retrieve the primitive information we need to match this renderable node to a state bucket
	D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopologyType;
	D3D12_INDEX_BUFFER_STRIP_CUT_VALUE stripCutValue;
	std::vector<VertexAttributeId> vertexAttributeIDs;
	std::vector<VertexAttributeId> instanceAttributeIDs;
	std::vector<DXGI_FORMAT> attributeDataTypes;
	childNodeResolved->GetPipelineStateSettingsForRenderable(primitiveTopologyType, stripCutValue, vertexAttributeIDs, instanceAttributeIDs, attributeDataTypes);

	// Retrieve a state bucket which is compatible with the specified object
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	PipelineStateBucket& stateBucket = GetMatchingStateBucket(primitiveTopologyType, stripCutValue, vertexAttributeIDs, instanceAttributeIDs, attributeDataTypes);

	// Add the child node
	auto childNodeID = (uint32_t)stateBucket.buildChildNodes.size();
	stateBucket.buildChildNodes.push_back(childNodeResolved);
	stateBucket.hasModifiedChildNodes = true;
	lock.unlock();
	childNodeResolved->SetChildNodeId(childNodeID);

	// Flag that the build state has been modified
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void Direct3DStateGroupNode::AddChildNodes(IRenderableNode* const* childNodes, size_t childNodeCount)
{
	// Add each of the specified child nodes to this group node
	D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopologyType;
	D3D12_INDEX_BUFFER_STRIP_CUT_VALUE stripCutValue;
	std::vector<VertexAttributeId> vertexAttributeIDs;
	std::vector<VertexAttributeId> instanceAttributeIDs;
	std::vector<DXGI_FORMAT> attributeDataTypes;
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	for (size_t i = 0; i < childNodeCount; ++i)
	{
		// Ensure the specified node is in a valid state to be added as a child node
		auto* childNodeResolved = KnownDynamicCast<Direct3DRenderableNode*>(*(childNodes++));
		if (!childNodeResolved->SetAsChildNode())
		{
			_log->Error("Failed to add renderable node as child");
			continue;
		}

		// Retrieve the primitive information we need to match this renderable node to a state bucket
		childNodeResolved->GetPipelineStateSettingsForRenderable(primitiveTopologyType, stripCutValue, vertexAttributeIDs, instanceAttributeIDs, attributeDataTypes);

		// Retrieve a state bucket which is compatible with the specified object
		PipelineStateBucket& stateBucket = GetMatchingStateBucket(primitiveTopologyType, stripCutValue, vertexAttributeIDs, instanceAttributeIDs, attributeDataTypes);

		// Add the child node
		auto childNodeID = (uint32_t)stateBucket.buildChildNodes.size();
		stateBucket.buildChildNodes.push_back(childNodeResolved);
		stateBucket.hasModifiedChildNodes = true;
		childNodeResolved->SetChildNodeId(childNodeID);
	}
	lock.unlock();

	// Flag that the build state has been modified
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void Direct3DStateGroupNode::AddChildNodes(IRenderableNode::unique_ptr const* childNodes, size_t childNodeCount)
{
	// Add each of the specified child nodes to this group node
	D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopologyType;
	D3D12_INDEX_BUFFER_STRIP_CUT_VALUE stripCutValue;
	std::vector<VertexAttributeId> vertexAttributeIDs;
	std::vector<VertexAttributeId> instanceAttributeIDs;
	std::vector<DXGI_FORMAT> attributeDataTypes;
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	for (size_t i = 0; i < childNodeCount; ++i)
	{
		// Ensure the specified node is in a valid state to be added as a child node
		auto* childNodeResolved = KnownDynamicCast<Direct3DRenderableNode*>((childNodes++)->get());
		if (!childNodeResolved->SetAsChildNode())
		{
			_log->Error("Failed to add renderable node as child");
			continue;
		}

		// Retrieve the primitive information we need to match this renderable node to a state bucket
		childNodeResolved->GetPipelineStateSettingsForRenderable(primitiveTopologyType, stripCutValue, vertexAttributeIDs, instanceAttributeIDs, attributeDataTypes);

		// Retrieve a state bucket which is compatible with the specified object
		PipelineStateBucket& stateBucket = GetMatchingStateBucket(primitiveTopologyType, stripCutValue, vertexAttributeIDs, instanceAttributeIDs, attributeDataTypes);

		// Add the child node
		auto childNodeID = (uint32_t)stateBucket.buildChildNodes.size();
		stateBucket.buildChildNodes.push_back(childNodeResolved);
		stateBucket.hasModifiedChildNodes = true;
		childNodeResolved->SetChildNodeId(childNodeID);
	}
	lock.unlock();

	// Flag that the build state has been modified
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void Direct3DStateGroupNode::RemoveChildNode(IRenderableNode* childNode)
{
	// Retrieve the ID of the specified child node
	auto* childNodeResolved = KnownDynamicCast<Direct3DRenderableNode*>(childNode);
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
bool Direct3DStateGroupNode::RemoveChildNode(Direct3DRenderableNode* childNode, uint32_t childNodeID)
{
	// Search for the child node in our pipeline state buckets, and remove it if found.
	for (PipelineStateBucket& stateBucket : _pipelineStateBuckets)
	{
		if ((childNodeID < (uint32_t)stateBucket.buildChildNodes.size()) && (stateBucket.buildChildNodes[childNodeID] == childNode))
		{
			// Clear the child node slot, and mark it as removed. Note that we do a sorted insertion here inside the
			// lock. We could instead leave the list of removed nodes unsorted, and sort it once on the render thread,
			// which is more efficient overall. Both methods have been profiled. The two approaches were equivalent for
			// small numbers of removals or large numbers of removals which occur in the same order of addition, with
			// 500000 objects removed per frame. When those 500000 objects were removed in reverse order of addition,
			// sorting on the render thread was about 20% more efficient overall, however the amount of additional time
			// taken on the render thread doubled. In the interests of minimizing overhead on the render thread for
			// large object removals, to maintain a more steady framerate, we opt of the additional work on the calling
			// application thread here.
			stateBucket.removedChildNodeIDs.insert(std::upper_bound(stateBucket.removedChildNodeIDs.begin(), stateBucket.removedChildNodeIDs.end(), childNodeID), childNodeID);
			stateBucket.buildChildNodes[childNodeID] = nullptr;
			stateBucket.hasModifiedChildNodes = true;
			return true;
		}
	}
	for (PipelineStateBucket& stateBucket : _newPipelineStateBuckets)
	{
		if ((childNodeID < (uint32_t)stateBucket.buildChildNodes.size()) && (stateBucket.buildChildNodes[childNodeID] == childNode))
		{
			// Clear the child node slot, and mark it as removed. Note that we do a sorted insertion here inside the
			// lock. We could instead leave the list of removed nodes unsorted, and sort it once on the render thread,
			// which is more efficient overall. Both methods have been profiled. The two approaches were equivalent for
			// small numbers of removals or large numbers of removals which occur in the same order of addition, with
			// 500000 objects removed per frame. When those 500000 objects were removed in reverse order of addition,
			// sorting on the render thread was about 20% more efficient overall, however the amount of additional time
			// taken on the render thread doubled. In the interests of minimizing overhead on the render thread for
			// large object removals, to maintain a more steady framerate, we opt of the additional work on the calling
			// application thread here.
			stateBucket.removedChildNodeIDs.insert(std::upper_bound(stateBucket.removedChildNodeIDs.begin(), stateBucket.removedChildNodeIDs.end(), childNodeID), childNodeID);
			stateBucket.buildChildNodes[childNodeID] = nullptr;
			stateBucket.hasModifiedChildNodes = true;
			return true;
		}
	}

	// Since the specified node doesn't appear to be a current child node, log an error, and return false.
	_log->Error("Tried to remove renderable child node with ID {0}, but it could not be found in the group node.", childNodeID);
	return false;
}

//----------------------------------------------------------------------------------------
void Direct3DStateGroupNode::RemoveChildNodes(IRenderableNode* const* childNodes, size_t childNodeCount)
{
	// Remove each of the specified child nodes from this group node
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	for (size_t i = 0; i < childNodeCount; ++i)
	{
		// Retrieve the ID of the specified child node
		auto* childNodeResolved = KnownDynamicCast<Direct3DRenderableNode*>(*(childNodes++));
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
void Direct3DStateGroupNode::RemoveChildNodes(IRenderableNode::unique_ptr const* childNodes, size_t childNodeCount)
{
	// Remove each of the specified child nodes from this group node
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	for (size_t i = 0; i < childNodeCount; ++i)
	{
		// Retrieve the ID of the specified child node
		auto* childNodeResolved = KnownDynamicCast<Direct3DRenderableNode*>((childNodes++)->get());
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
void Direct3DStateGroupNode::RemoveAllChildNodes()
{
	// Notify removed renderables that they are no longer being held as child nodes. This is only used for extra debug
	// checks to detect incorrect scene tree usage.
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	if (_renderer->DebugLoggingEnabled())
	{
		for (PipelineStateBucket& stateBucket : _pipelineStateBuckets)
		{
			for (auto* childNode : stateBucket.buildChildNodes)
			{
				if (childNode != nullptr)
				{
					childNode->RemoveAsChildNode();
				}
			}
		}
	}

	// Erase all child nodes
	for (PipelineStateBucket& stateBucket : _pipelineStateBuckets)
	{
		stateBucket.buildChildNodes.clear();
		stateBucket.hasModifiedChildNodes = true;
		stateBucket.removedChildNodeIDs.clear();
	}
	lock.unlock();

	// Flag that the build state has been modified
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void Direct3DStateGroupNode::SetChildNodes(IRenderableNode* const* childNodes, size_t childNodeCount)
{
	RemoveAllChildNodes();
	AddChildNodes(childNodes, childNodeCount);
}

//----------------------------------------------------------------------------------------
void Direct3DStateGroupNode::SetChildNodes(IRenderableNode::unique_ptr const* childNodes, size_t childNodeCount)
{
	RemoveAllChildNodes();
	AddChildNodes(childNodes, childNodeCount);
}

//----------------------------------------------------------------------------------------
const std::vector<Direct3DRenderableNode*>& Direct3DStateGroupNode::GetChildNodes(size_t stateBucketIndex) const
{
	return (_drawState.computeTaskDefined ? _computePipelineStateBucket.drawChildNodes : _pipelineStateBuckets[stateBucketIndex].drawChildNodes);
}

//----------------------------------------------------------------------------------------
// Compute methods
//----------------------------------------------------------------------------------------
bool Direct3DStateGroupNode::HasComputeTask() const
{
	return _drawState.computeTaskDefined;
}

//----------------------------------------------------------------------------------------
V3UInt32 Direct3DStateGroupNode::GetComputeThreadGroupCounts() const
{
	return _drawState.computeThreadGroupCounts;
}

//----------------------------------------------------------------------------------------
void Direct3DStateGroupNode::SetComputeTask(const V3UInt32& threadGroupCounts)
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	_buildState.computeTaskDefined = true;
	_buildState.computeThreadGroupCounts = threadGroupCounts;
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void Direct3DStateGroupNode::RemoveComputeTask()
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	_buildState.computeTaskDefined = false;
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
Direct3DShaderProgram::GlobalConstantBufferBindingInfo& Direct3DStateGroupNode::GetGlobalConstantBufferBindingInfo()
{
	return _globalConstantBufferBindingInfo;
}

//----------------------------------------------------------------------------------------
// Depth state methods
//----------------------------------------------------------------------------------------
void Direct3DStateGroupNode::SetDepthTestEnabled(bool state)
{
	_buildState.depthTestEnabled = state;
	_buildStateChanged = true;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void Direct3DStateGroupNode::SetDepthWriteEnabled(bool state)
{
	_buildState.depthWriteEnabled = state;
	_buildStateChanged = true;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void Direct3DStateGroupNode::SetDepthComparisonFunction(DepthComparisonFunction comparisonTest)
{
	_buildState.depthComparisonTest = comparisonTest;
	_buildStateChanged = true;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void Direct3DStateGroupNode::SetDepthBias(float constantFactor, float slopeFactor, float clamp)
{
	_buildState.depthBiasEnabled = true;
	_buildState.depthBiasConstantFactor = constantFactor;
	_buildState.depthBiasSlopeFactor = slopeFactor;
	_buildState.depthBiasClamp = clamp;
	_buildStateChanged = true;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void Direct3DStateGroupNode::ClearDepthBias()
{
	_buildState.depthBiasEnabled = false;
	_buildStateChanged = true;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
// Stencil state methods
//----------------------------------------------------------------------------------------
void Direct3DStateGroupNode::SetStencilTestEnabled(bool state, uint32_t compareMask, uint32_t writeMask)
{
	_buildState.stencilEnabled = state;
	_buildState.stencilCompareMask = compareMask;
	_buildState.stencilWriteMask = writeMask;
	_buildStateChanged = true;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void Direct3DStateGroupNode::SetStencilOperation(StencilTargetFace targetFace, StencilComparisonFunction comparisonTest, StencilOperation passOperation, StencilOperation failOperation, StencilOperation depthFailOperation)
{
	if ((targetFace == IStateGroupNode::StencilTargetFace::FrontFace) || (targetFace == IStateGroupNode::StencilTargetFace::FrontAndBackFace))
	{
		_buildState.stencilFrontFace = BuildStencilFaceState(comparisonTest, passOperation, failOperation, depthFailOperation);
	}
	if ((targetFace == IStateGroupNode::StencilTargetFace::BackFace) || (targetFace == IStateGroupNode::StencilTargetFace::FrontAndBackFace))
	{
		_buildState.stencilBackFace = BuildStencilFaceState(comparisonTest, passOperation, failOperation, depthFailOperation);
	}
	_buildStateChanged = true;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void Direct3DStateGroupNode::SetStencilReferenceValue(uint32_t referenceValue)
{
	_buildState.stencilReferenceValue = referenceValue;
	_buildStateChanged = true;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
// Rasterization state methods
//----------------------------------------------------------------------------------------
void Direct3DStateGroupNode::SetPolygonFillMode(PolygonFillMode fillMode)
{
	_buildState.polygonFillMode = fillMode;
	_buildStateChanged = true;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void Direct3DStateGroupNode::SetPolygonCullMode(PolygonCullMode cullMode)
{
	_buildState.polygonCullMode = cullMode;
	_buildStateChanged = true;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void Direct3DStateGroupNode::SetPolygonWindingOrder(PolygonWindingOrder windingOrder)
{
	_buildState.polygonWindingOrder = windingOrder;
	_buildStateChanged = true;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
// Blend state methods
//----------------------------------------------------------------------------------------
void Direct3DStateGroupNode::SetBlendEnabled(bool state)
{
	_buildState.blendEnabled = state;
	_buildStateChanged = true;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void Direct3DStateGroupNode::SetBlendMode(BlendOperation blendOperationRGB, BlendFactor blendFactorSourceRGB, BlendFactor blendFactorDestinationRGB, BlendOperation blendOperationA, BlendFactor blendFactorSourceA, BlendFactor blendFactorDestinationA)
{
	BlendState blendState = BuildBlendState(blendOperationRGB, blendFactorSourceRGB, blendFactorDestinationRGB, blendOperationA, blendFactorSourceA, blendFactorDestinationA);
	_buildState.sharedBlendState = blendState;
	_buildStateChanged = true;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void Direct3DStateGroupNode::SetBlendMode(IFrameBuffer::AttachmentType type, size_t index, BlendOperation blendOperationRGB, BlendFactor blendFactorSourceRGB, BlendFactor blendFactorDestinationRGB, BlendOperation blendOperationA, BlendFactor blendFactorSourceA, BlendFactor blendFactorDestinationA)
{
	BlendState blendState = BuildBlendState(blendOperationRGB, blendFactorSourceRGB, blendFactorDestinationRGB, blendOperationA, blendFactorSourceA, blendFactorDestinationA);
	blendState.type = type;
	blendState.index = index;

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
	_buildStateChanged = true;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
size_t Direct3DStateGroupNode::GetStateBucketCount() const
{
	return (_drawState.computeTaskDefined ? 1 : _pipelineStateBuckets.size());
}

//----------------------------------------------------------------------------------------
void Direct3DStateGroupNode::ClearFrameBufferEntry(int frameBufferIndex)
{
	// Add this framebuffer entry to the set of invalidated framebuffer entries. Note that we don't need to do anything
	// else at this point, as this only affects the build state so far, and no objects have actually been allocated for
	// the new pipeline state buckets yet.
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	_invalidatedFrameBufferEntries.push_back(frameBufferIndex);
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void Direct3DStateGroupNode::ClearAllFrameBufferEntries()
{
	// Invalidate all framebuffer entries
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	_invalidatedFrameBufferEntries.resize(_buildState.allocatedFrameBufferSlotCount);
	for (size_t i = 0; i < _buildState.allocatedFrameBufferSlotCount; ++i)
	{
		_invalidatedFrameBufferEntries[i] = (int)i;
	}
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void Direct3DStateGroupNode::SetFrameBufferCount(int count)
{
	// Increase the number of allocated framebuffer entries based on the new framebuffer index. By design, this will
	// always be a push back of a single entry to the framebuffer list. We also resize the framebuffer slot arrays for the
	// new pipeline state buckets which have been added in this build phase here. We could entirely delay creation of
	// these arrays until the build state is being migrated, but we want to minimize the work performed during migration,
	// as that step is on the critical path for rendering performance. As a result, we instead perform these allocations
	// on the calling thread during the build phase, even if it means we may have to re-allocate the arrays in some cases.
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	_buildState.allocatedFrameBufferSlotCount = count;
	for (auto& pipelineStateBucket : _newPipelineStateBuckets)
	{
		pipelineStateBucket.drawNativeObjectsCurrentForBucket.resize(_buildState.allocatedFrameBufferSlotCount, 0u);
		pipelineStateBucket.framebufferLastUpdateTokens.resize(_buildState.allocatedFrameBufferSlotCount, 0);
		pipelineStateBucket.pipelineStateDescriptions.resize(_buildState.allocatedFrameBufferSlotCount);
		pipelineStateBucket.pipelineStateObjects.resize(_buildState.allocatedFrameBufferSlotCount);
	}
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void Direct3DStateGroupNode::MigrateBuildStateToDrawState()
{
	// Migrate our build state
	if (!IsDrawStateCurrent())
	{
		// Transfer basic build state
		Direct3DStateContainer::MigrateBuildStateToDrawState();
		bool buildStateChanged = _buildStateChanged;
		bool allocatedFrameBufferSlotCountChanged = _drawState.allocatedFrameBufferSlotCount != _buildState.allocatedFrameBufferSlotCount;
		size_t allocatedFrameBufferSlotCount = _buildState.allocatedFrameBufferSlotCount;
		_drawState = _buildState;
		_buildStateChanged = false;

		// If our allocated framebuffer slot count changed, resize the existing pipeline state bucket buffer entries before
		// we merge them with any new buckets.
		if (allocatedFrameBufferSlotCountChanged)
		{
			for (auto& pipelineStateBucket : _pipelineStateBuckets)
			{
				pipelineStateBucket.drawNativeObjectsCurrentForBucket.resize(allocatedFrameBufferSlotCount, 0u);
				pipelineStateBucket.framebufferLastUpdateTokens.resize(allocatedFrameBufferSlotCount, 0);
				pipelineStateBucket.pipelineStateDescriptions.resize(allocatedFrameBufferSlotCount);
				pipelineStateBucket.pipelineStateObjects.resize(allocatedFrameBufferSlotCount);
			}
		}

		// Free resources for any cleared framebuffer entries
		if (!_invalidatedFrameBufferEntries.empty())
		{
			for (auto frameBufferIndex : _invalidatedFrameBufferEntries)
			{
				for (auto& pipelineStateBucket : _pipelineStateBuckets)
				{
					pipelineStateBucket.drawNativeObjectsCurrentForBucket[frameBufferIndex] = 0u;
					pipelineStateBucket.pipelineStateObjects[frameBufferIndex].Reset();
				}
			}
			_invalidatedFrameBufferEntries.clear();
		}

		// Add new pipeline state buckets to our primary set
		if (!_newPipelineStateBuckets.empty())
		{
			_pipelineStateBuckets.insert(_pipelineStateBuckets.end(), _newPipelineStateBuckets.begin(), _newPipelineStateBuckets.end());
			_newPipelineStateBuckets.clear();
		}

		// Update the current flags for the state associated with each pipeline state bucket
		for (PipelineStateBucket& stateBucket : _pipelineStateBuckets)
		{
			for (size_t i = 0; i < allocatedFrameBufferSlotCount; ++i)
			{
				stateBucket.drawNativeObjectsCurrentForBucket[i] = static_cast<unsigned char>(!buildStateChanged && (stateBucket.drawNativeObjectsCurrentForBucket[i] != 0u) && stateBucket.buildNativeObjectsCurrentForBucket);
			}
			stateBucket.buildNativeObjectsCurrentForBucket = true;
		}

		// Apply pending child node changes to each pipeline state bucket
		for (PipelineStateBucket& stateBucket : _pipelineStateBuckets)
		{
			// Transfer child node state only if it has changed. We do this for efficiency. This implementation gives
			// us extremely fast addition and removal of individual nodes, and the vectors will size themselves
			// naturally to have enough capacity to end up allocation free.
			if (stateBucket.hasModifiedChildNodes)
			{
				if (!stateBucket.removedChildNodeIDs.empty())
				{
					// Retrieve the index of the first child node which was removed. Note that the list of removed child
					// node indices has been pre-sorted from lowest to highest.
					auto firstRemovedChildNodeIndex = stateBucket.removedChildNodeIDs.front();

					// Transfer the remaining valid child node entries to our draw array
					size_t nextWriteIndex = 0;
					size_t nextReadIndex = 0;
					size_t removedChildNodeCount = stateBucket.removedChildNodeIDs.size();
					size_t newChildNodeCount = stateBucket.buildChildNodes.size() - removedChildNodeCount;
					stateBucket.drawChildNodes.resize(newChildNodeCount);
					size_t removedChildNodeSearchIndex = 0;
					while (removedChildNodeSearchIndex < removedChildNodeCount)
					{
						auto removedIndex = stateBucket.removedChildNodeIDs[removedChildNodeSearchIndex++];
						while ((removedIndex == nextReadIndex) && (removedChildNodeSearchIndex < removedChildNodeCount))
						{
							removedIndex = stateBucket.removedChildNodeIDs[removedChildNodeSearchIndex++];
							++nextReadIndex;
						}
						std::copy(stateBucket.buildChildNodes.data() + nextReadIndex, stateBucket.buildChildNodes.data() + removedIndex, stateBucket.drawChildNodes.data() + nextWriteIndex);
						nextWriteIndex += removedIndex - nextReadIndex;
						nextReadIndex = removedIndex + 1;
					}
					std::copy(stateBucket.buildChildNodes.data() + nextReadIndex, stateBucket.buildChildNodes.data() + stateBucket.buildChildNodes.size(), stateBucket.drawChildNodes.data() + nextWriteIndex);
					stateBucket.removedChildNodeIDs.clear();

					// Since we had removed entries, transfer our compacted child node entries back to our build array.
					stateBucket.buildChildNodes.resize(newChildNodeCount);
					std::copy(stateBucket.drawChildNodes.data(), stateBucket.drawChildNodes.data() + newChildNodeCount, stateBucket.buildChildNodes.data());

					// Update child node IDs for all relocated nodes in the buffer
					for (auto i = firstRemovedChildNodeIndex; i < newChildNodeCount; ++i)
					{
						stateBucket.buildChildNodes[i]->SetChildNodeId(i);
					}
				}
				else
				{
					// Transfer our child node entries to our draw array
					size_t newChildNodeCount = stateBucket.buildChildNodes.size();
					stateBucket.drawChildNodes.resize(newChildNodeCount);
					std::copy(stateBucket.buildChildNodes.data(), stateBucket.buildChildNodes.data() + newChildNodeCount, stateBucket.drawChildNodes.data());
				}
				stateBucket.hasModifiedChildNodes = false;
			}
		}

		// Flag that our draw state is now current
		FlagDrawStateCurrent();
	}

	// Migrate build state in our child nodes
	for (PipelineStateBucket& stateBucket : _pipelineStateBuckets)
	{
		for (Direct3DRenderableNode* entry : stateBucket.drawChildNodes)
		{
			entry->MigrateBuildStateToDrawState();
		}
	}
}

//----------------------------------------------------------------------------------------
void Direct3DStateGroupNode::ApplyFixedState(size_t stateBucketIndex, int frameBufferIndex, ID3D12GraphicsCommandList* commandList, Direct3DShaderProgram* shaderProgram, Direct3DFrameBuffer* framebuffer)
{
	// If this is a compute task node, reset the frame buffer index to 0.
	if (_drawState.computeTaskDefined)
	{
		frameBufferIndex = 0;
	}

	// Update our native state objects if required
	PipelineStateBucket& stateBucket = (_drawState.computeTaskDefined ? _computePipelineStateBucket : _pipelineStateBuckets[stateBucketIndex]);
	if ((stateBucket.drawNativeObjectsCurrentForBucket[frameBufferIndex] == 0u) || (!_drawState.computeTaskDefined && (framebuffer->GetFrameBufferObjectLastUpdateToken() != stateBucket.framebufferLastUpdateTokens[frameBufferIndex])))
	{
		// Attempt to build the pipeline state object
		if (_drawState.computeTaskDefined)
		{
			if (!CreateNativeComputePipelineState(stateBucket, shaderProgram))
			{
				_log->Error("Failed to build compute pipeline state for state group node");
				return;
			}
		}
		else
		{
			if (!CreateNativeGraphicsPipelineState(stateBucket, frameBufferIndex, shaderProgram, framebuffer))
			{
				_log->Error("Failed to build graphics pipeline state for state group node");
				return;
			}
		}

		// Flag that our native objects are now current
		stateBucket.drawNativeObjectsCurrentForBucket[frameBufferIndex] = 1u;
		if (!_drawState.computeTaskDefined)
		{
			stateBucket.framebufferLastUpdateTokens[frameBufferIndex] = framebuffer->GetFrameBufferObjectLastUpdateToken();
		}
	}

	// Bind the pipeline state object for this node
	commandList->SetPipelineState(stateBucket.pipelineStateObjects[frameBufferIndex].Get());

	// Set the stencil reference value if required
	if (!_drawState.computeTaskDefined && _drawState.stencilEnabled)
	{
		commandList->OMSetStencilRef(_drawState.stencilReferenceValue);
	}
}

//----------------------------------------------------------------------------------------
Direct3DStateGroupNode::PipelineStateBucket& Direct3DStateGroupNode::GetMatchingStateBucket(D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopology, D3D12_INDEX_BUFFER_STRIP_CUT_VALUE stripCutValue, const std::vector<VertexAttributeId>& vertexAttributeIDs, const std::vector<VertexAttributeId>& instanceAttributeIDs, const std::vector<DXGI_FORMAT>& attributeDataTypes)
{
	// Attempt to locate and update an existing compatible state bucket. We search in our set of new pipeline state
	// buckets first if there are any entries, as this represents buckets that have been added during this build process
	// since the last frame was drawn. As we expect objects being added in the same frame are probably correlated, it
	// makes sense to search the new buckets first in this case, as it's more likely we'll find a match there than in
	// the set of existing buckets.
	PipelineStateBucket* matchingStateBucket = nullptr;
	if (!_newPipelineStateBuckets.empty())
	{
		matchingStateBucket = GetMatchingStateBucketInSet(_newPipelineStateBuckets, primitiveTopology, stripCutValue, vertexAttributeIDs, instanceAttributeIDs, attributeDataTypes);
	}
	if (matchingStateBucket == nullptr)
	{
		matchingStateBucket = GetMatchingStateBucketInSet(_pipelineStateBuckets, primitiveTopology, stripCutValue, vertexAttributeIDs, instanceAttributeIDs, attributeDataTypes);
	}

	// If no existing state bucket was compatible, create a new one now.
	if (matchingStateBucket == nullptr)
	{
		// Create a new state bucket
		PipelineStateBucket stateBucket = {};
		stateBucket.primitiveTopology = primitiveTopology;
		stateBucket.stripCutValue = stripCutValue;
		stateBucket.hasModifiedChildNodes = false;
		stateBucket.buildNativeObjectsCurrentForBucket = false;
		stateBucket.drawNativeObjectsCurrentForBucket.resize(_buildState.allocatedFrameBufferSlotCount, 0u);
		stateBucket.framebufferLastUpdateTokens.resize(_buildState.allocatedFrameBufferSlotCount, 0);
		stateBucket.pipelineStateDescriptions.resize(_buildState.allocatedFrameBufferSlotCount);
		stateBucket.pipelineStateObjects.resize(_buildState.allocatedFrameBufferSlotCount);

		// Calculate the number of known attribute slots
		int highestKnownAttributeID = 0;
		for (auto vertexAttributeID : vertexAttributeIDs)
		{
			highestKnownAttributeID = std::max(highestKnownAttributeID, (int)vertexAttributeID);
		}
		for (auto instanceAttributeID : instanceAttributeIDs)
		{
			highestKnownAttributeID = std::max(highestKnownAttributeID, (int)instanceAttributeID);
		}

		// Populate the instance attribute flags
		stateBucket.attributeTypeKnown.resize(highestKnownAttributeID + 1, 0);
		stateBucket.attributeIsInstanced.resize(highestKnownAttributeID + 1, 0);
		stateBucket.attributeDataType.resize(highestKnownAttributeID + 1, DXGI_FORMAT_UNKNOWN);
		for (size_t i = 0; i < vertexAttributeIDs.size(); ++i)
		{
			auto vertexAttributeIndex = (size_t)vertexAttributeIDs[i];
			stateBucket.attributeTypeKnown[vertexAttributeIndex] = 1;
			stateBucket.attributeDataType[vertexAttributeIndex] = attributeDataTypes[i];
		}
		for (size_t i = 0; i < instanceAttributeIDs.size(); ++i)
		{
			auto instanceAttributeIndex = (size_t)instanceAttributeIDs[i];
			stateBucket.attributeTypeKnown[instanceAttributeIndex] = 1;
			stateBucket.attributeIsInstanced[instanceAttributeIndex] = 1;
			stateBucket.attributeDataType[instanceAttributeIndex] = attributeDataTypes[vertexAttributeIDs.size() + i];
		}

		// Add this new state bucket to the set of state buckets, and set it as the selected state bucket.
		_newPipelineStateBuckets.push_back(std::move(stateBucket));
		matchingStateBucket = &_newPipelineStateBuckets.back();
	}

	// Return the matching state bucket to the caller
	return *matchingStateBucket;
}

//----------------------------------------------------------------------------------------
Direct3DStateGroupNode::PipelineStateBucket* Direct3DStateGroupNode::GetMatchingStateBucketInSet(std::vector<PipelineStateBucket>& stateBucketSet, D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopology, D3D12_INDEX_BUFFER_STRIP_CUT_VALUE stripCutValue, const std::vector<VertexAttributeId>& vertexAttributeIDs, const std::vector<VertexAttributeId>& instanceAttributeIDs, const std::vector<DXGI_FORMAT>& attributeDataTypes)
{
	// Attempt to locate and update an existing compatible state bucket
	for (auto& stateBucket : stateBucketSet)
	{
		// Compare the primitive topology
		if ((stateBucket.primitiveTopology != primitiveTopology) || (stateBucket.stripCutValue != stripCutValue))
		{
			continue;
		}

		// Confirm the vertex attributes aren't marked as instance attributes and match in type
		int knownVertexAttributeCount = (int)stateBucket.attributeTypeKnown.size();
		int highestKnownAttributeID = knownVertexAttributeCount;
		bool foundAttributeClash = false;
		size_t vertexAttributeIDCount = vertexAttributeIDs.size();
		for (size_t i = 0; i < vertexAttributeIDCount; ++i)
		{
			int vertexAttributeIndex = (int)vertexAttributeIDs[i];
			highestKnownAttributeID = std::max(highestKnownAttributeID, vertexAttributeIndex);
			if ((vertexAttributeIndex < knownVertexAttributeCount) && (stateBucket.attributeTypeKnown[vertexAttributeIndex] != 0u) && ((stateBucket.attributeIsInstanced[vertexAttributeIndex] != 0u) || (stateBucket.attributeDataType[vertexAttributeIndex] != attributeDataTypes[i])))
			{
				foundAttributeClash = true;
				break;
			}
		}
		if (foundAttributeClash)
		{
			continue;
		}

		// Confirm the instance attributes aren't marked as vertex attributes and match in type
		for (size_t i = 0; i < instanceAttributeIDs.size(); ++i)
		{
			int vertexAttributeIndex = (int)instanceAttributeIDs[i];
			highestKnownAttributeID = std::max(highestKnownAttributeID, vertexAttributeIndex);
			if ((vertexAttributeIndex < knownVertexAttributeCount) && (stateBucket.attributeTypeKnown[vertexAttributeIndex] != 0u) && ((stateBucket.attributeIsInstanced[vertexAttributeIndex] == 0u) || (stateBucket.attributeDataType[vertexAttributeIndex] != attributeDataTypes[vertexAttributeIDCount + i])))
			{
				foundAttributeClash = true;
				break;
			}
		}
		if (foundAttributeClash)
		{
			continue;
		}

		// Update the instance attribute flags and attribute data types in the state bucket
		if (highestKnownAttributeID >= knownVertexAttributeCount)
		{
			stateBucket.attributeTypeKnown.resize(highestKnownAttributeID + 1, 0);
			stateBucket.attributeIsInstanced.resize(highestKnownAttributeID + 1, 0);
			stateBucket.attributeDataType.resize(highestKnownAttributeID + 1, DXGI_FORMAT_UNKNOWN);
		}
		for (size_t i = 0; i < vertexAttributeIDCount; ++i)
		{
			int vertexAttributeIndex = (int)vertexAttributeIDs[i];
			if (stateBucket.attributeTypeKnown[vertexAttributeIndex] == 0)
			{
				stateBucket.buildNativeObjectsCurrentForBucket = false;
				stateBucket.attributeTypeKnown[vertexAttributeIndex] = 1;
				stateBucket.attributeDataType[vertexAttributeIndex] = attributeDataTypes[i];
			}
		}
		for (size_t i = 0; i < instanceAttributeIDs.size(); ++i)
		{
			int vertexAttributeIndex = (int)instanceAttributeIDs[i];
			if (stateBucket.attributeTypeKnown[vertexAttributeIndex] == 0)
			{
				stateBucket.buildNativeObjectsCurrentForBucket = false;
				stateBucket.attributeTypeKnown[vertexAttributeIndex] = 1;
				stateBucket.attributeIsInstanced[vertexAttributeIndex] = 1;
				stateBucket.attributeDataType[vertexAttributeIndex] = attributeDataTypes[vertexAttributeIDCount + i];
			}
		}

		// Return this state bucket to the caller
		return &stateBucket;
	}
	return nullptr;
}

//----------------------------------------------------------------------------------------
bool Direct3DStateGroupNode::CreateNativeComputePipelineState(PipelineStateBucket& stateBucket, Direct3DShaderProgram* shaderProgram)
{
	// Initialize the pipeline state description with suitable default values
	D3D12_COMPUTE_PIPELINE_STATE_DESC pipelineDescription;
	pipelineDescription = {};
	pipelineDescription.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	pipelineDescription.NodeMask = 0;
	pipelineDescription.CachedPSO = D3D12_CACHED_PIPELINE_STATE{};

	// Load shader program info into the pipeline state description
	ID3DBlob* computeShaderCode = shaderProgram->GetComputeShaderCode();
	pipelineDescription.CS = CD3DX12_SHADER_BYTECODE(computeShaderCode);
	pipelineDescription.pRootSignature = shaderProgram->GetRootSignature();

	// Create the pipeline state object
	HRESULT createComputePipelineStateReturn = _renderer->GetDevice()->CreateComputePipelineState(&pipelineDescription, IID_PPV_ARGS(&stateBucket.pipelineStateObjects[0]));
	if (FAILED(createComputePipelineStateReturn))
	{
		_log->Error("Failed to create compute pipeline state object with error code {0}", createComputePipelineStateReturn);
		return false;
	}
	return true;
}

//----------------------------------------------------------------------------------------
bool Direct3DStateGroupNode::CreateNativeGraphicsPipelineState(PipelineStateBucket& stateBucket, int frameBufferIndex, Direct3DShaderProgram* shaderProgram, Direct3DFrameBuffer* framebuffer)
{
	// Initialize the pipeline state description with suitable default values
	D3D12_GRAPHICS_PIPELINE_STATE_DESC& pipelineDescription = stateBucket.pipelineStateDescriptions[frameBufferIndex];
	pipelineDescription = {};
	pipelineDescription.SampleMask = DefaultSampleMask();
	pipelineDescription.SampleDesc = DefaultSampleDesc();
	pipelineDescription.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	pipelineDescription.NodeMask = 0;
	pipelineDescription.CachedPSO = D3D12_CACHED_PIPELINE_STATE{};

	// Load our own state settings into the pipeline state description
	CreateNativeRasterizerDescription(pipelineDescription.RasterizerState);
	CreateNativeBlendDescription(pipelineDescription.BlendState);
	CreateNativeDepthStencilDescription(pipelineDescription.DepthStencilState);

	// Configure multisampling
	UINT nativeSampleCount;
	UINT sampleQuality;
	auto sampleCount = framebuffer->GetSampleCount(nativeSampleCount, sampleQuality);
	if (sampleCount != ITextureBuffer::SampleCount::SampleCount1)
	{
		pipelineDescription.RasterizerState.MultisampleEnable = TRUE;
		pipelineDescription.RasterizerState.AntialiasedLineEnable = FALSE;
		pipelineDescription.SampleDesc.Count = nativeSampleCount;
		pipelineDescription.SampleDesc.Quality = sampleQuality;
	}

	// Load shader program info into the pipeline state description
	ID3DBlob* vertexShaderCode = shaderProgram->GetVertexShaderCode();
	ID3DBlob* fragmentShaderCode = shaderProgram->GetFragmentShaderCode();
	ID3DBlob* geometryShaderCode = shaderProgram->GetGeometryShaderCode();
	pipelineDescription.VS = (vertexShaderCode != nullptr) ? CD3DX12_SHADER_BYTECODE(vertexShaderCode) : D3D12_SHADER_BYTECODE{};
	pipelineDescription.PS = (fragmentShaderCode != nullptr) ? CD3DX12_SHADER_BYTECODE(fragmentShaderCode) : D3D12_SHADER_BYTECODE{};
	pipelineDescription.DS = D3D12_SHADER_BYTECODE{};
	pipelineDescription.HS = D3D12_SHADER_BYTECODE{};
	pipelineDescription.GS = (geometryShaderCode != nullptr) ? CD3DX12_SHADER_BYTECODE(geometryShaderCode) : D3D12_SHADER_BYTECODE{};
	pipelineDescription.StreamOutput = D3D12_STREAM_OUTPUT_DESC{};
	pipelineDescription.pRootSignature = shaderProgram->GetRootSignature();

	// Load framebuffer info into the pipeline state description
	framebuffer->GetDataFormats(pipelineDescription.RTVFormats, pipelineDescription.NumRenderTargets, pipelineDescription.DSVFormat);

	// Load renderable object info into the pipeline state description
	CreateNativeInputLayoutDescription(shaderProgram, pipelineDescription.InputLayout, stateBucket.inputElementDescriptions, stateBucket.attributeTypeKnown, stateBucket.attributeIsInstanced, stateBucket.attributeDataType);
	pipelineDescription.PrimitiveTopologyType = stateBucket.primitiveTopology;
	pipelineDescription.IBStripCutValue = stateBucket.stripCutValue;

	// Create the pipeline state object
	HRESULT createGraphicsPipelineStateReturn = _renderer->GetDevice()->CreateGraphicsPipelineState(&pipelineDescription, IID_PPV_ARGS(&stateBucket.pipelineStateObjects[frameBufferIndex]));
	if (FAILED(createGraphicsPipelineStateReturn))
	{
		_log->Error("Failed to create graphics pipeline state object with error code {0}", createGraphicsPipelineStateReturn);
		return false;
	}
	return true;
}

//----------------------------------------------------------------------------------------
void Direct3DStateGroupNode::CreateNativeInputLayoutDescription(Direct3DShaderProgram* shaderProgram, D3D12_INPUT_LAYOUT_DESC& inputLayoutDescription, std::vector<D3D12_INPUT_ELEMENT_DESC>& inputElementDescriptions, const std::vector<uint8_t>& attributeTypeKnown, const std::vector<uint8_t>& attributeIsInstanced, const std::vector<DXGI_FORMAT>& attributeDataType) const
{
	// Build the required set of vertex input elements
	size_t knownAttributeCount = attributeIsInstanced.size();
	shaderProgram->GetDefaultInputLayoutDescription(inputElementDescriptions);
	for (size_t i = 0; i < knownAttributeCount; ++i)
	{
		if (attributeTypeKnown[i] != 0)
		{
			D3D12_INPUT_ELEMENT_DESC& descriptionEntry = inputElementDescriptions[i];
			descriptionEntry.Format = attributeDataType[i];
			if (attributeIsInstanced[i] != 0)
			{
				descriptionEntry.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
				descriptionEntry.InstanceDataStepRate = 1;
			}
		}
	}

	// Create an input layout for the bound vertex attributes
	inputLayoutDescription.pInputElementDescs = inputElementDescriptions.data();
	inputLayoutDescription.NumElements = (UINT)inputElementDescriptions.size();
}

//----------------------------------------------------------------------------------------
void Direct3DStateGroupNode::CreateNativeRasterizerDescription(D3D12_RASTERIZER_DESC& rasterizerDesc) const
{
	// Determine the fill mode settings to use
	D3D12_FILL_MODE fillModeNative;
	switch (_drawState.polygonFillMode)
	{
	default:
	case PolygonFillMode::Solid:
		fillModeNative = D3D12_FILL_MODE_SOLID;
		break;
	case PolygonFillMode::Wireframe:
		fillModeNative = D3D12_FILL_MODE_WIREFRAME;
		break;
	}

	// Determine the cull mode settings to use
	D3D12_CULL_MODE cullModeNative;
	switch (_drawState.polygonCullMode)
	{
	default:
	case PolygonCullMode::None:
		cullModeNative = D3D12_CULL_MODE_NONE;
		break;
	case PolygonCullMode::Front:
		cullModeNative = D3D12_CULL_MODE_FRONT;
		break;
	case PolygonCullMode::Back:
		cullModeNative = D3D12_CULL_MODE_BACK;
		break;
	}

	// Populate the rasterizer description state
	rasterizerDesc = {};
	rasterizerDesc.CullMode = cullModeNative;
	if (_drawState.depthBiasEnabled)
	{
		rasterizerDesc.DepthBias = (int)std::ceil(_drawState.depthBiasConstantFactor);
		rasterizerDesc.SlopeScaledDepthBias = _drawState.depthBiasSlopeFactor;
		rasterizerDesc.DepthBiasClamp = _drawState.depthBiasClamp;
	}
	else
	{
		rasterizerDesc.DepthBias = 0;
		rasterizerDesc.SlopeScaledDepthBias = 0.0f;
		rasterizerDesc.DepthBiasClamp = 0.0f;
	}
	rasterizerDesc.DepthClipEnable = TRUE;
	rasterizerDesc.FillMode = fillModeNative;
	rasterizerDesc.FrontCounterClockwise = (_drawState.polygonWindingOrder == PolygonWindingOrder::CounterClockwise ? TRUE : FALSE);
	rasterizerDesc.MultisampleEnable = FALSE;
	rasterizerDesc.AntialiasedLineEnable = FALSE;
}

//----------------------------------------------------------------------------------------
void Direct3DStateGroupNode::CreateNativeBlendDescription(D3D12_BLEND_DESC& desc) const
{
	if (_drawState.blendEnabled)
	{
		desc = {};
		D3D12_RENDER_TARGET_BLEND_DESC blendDesc = {};
		for (auto& i : desc.RenderTarget)
		{
			i = blendDesc;
		}
		blendDesc.BlendEnable = TRUE;
		blendDesc.SrcBlend = _drawState.sharedBlendState.nativeBlendFactorSourceRGB;
		blendDesc.DestBlend = _drawState.sharedBlendState.nativeBlendFactorDestinationRGB;
		blendDesc.BlendOp = _drawState.sharedBlendState.nativeBlendOperationRGB;
		blendDesc.SrcBlendAlpha = GetNativeAlphaBlendFactor(_drawState.sharedBlendState.blendFactorSourceA, _drawState.sharedBlendState.blendOperationA);
		blendDesc.DestBlendAlpha = GetNativeAlphaBlendFactor(_drawState.sharedBlendState.blendFactorDestinationA, _drawState.sharedBlendState.blendOperationA);
		blendDesc.BlendOpAlpha = _drawState.sharedBlendState.nativeBlendOperationA;
		blendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		desc.RenderTarget[0] = blendDesc;
		desc.AlphaToCoverageEnable = FALSE;
		desc.IndependentBlendEnable = FALSE;

		for (const auto& targetBlendState : _drawState.attachmentTypeBlendState)
		{
			desc.IndependentBlendEnable = TRUE;
			D3D12_RENDER_TARGET_BLEND_DESC blendDescTemp = {};

			blendDescTemp.BlendEnable = TRUE;
			blendDescTemp.SrcBlend = targetBlendState.nativeBlendFactorSourceRGB;
			blendDescTemp.DestBlend = targetBlendState.nativeBlendFactorDestinationRGB;
			blendDescTemp.BlendOp = targetBlendState.nativeBlendOperationRGB;

			blendDescTemp.SrcBlendAlpha = GetNativeAlphaBlendFactor(targetBlendState.blendFactorSourceA, targetBlendState.blendOperationA);
			blendDescTemp.DestBlendAlpha = GetNativeAlphaBlendFactor(targetBlendState.blendFactorDestinationA, targetBlendState.blendOperationA);
			blendDescTemp.BlendOpAlpha = targetBlendState.nativeBlendOperationA;
			blendDescTemp.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

			desc.RenderTarget[targetBlendState.index] = blendDescTemp;
		}
	}
	else
	{
		desc = {};
		D3D12_RENDER_TARGET_BLEND_DESC blendDesc = {};
		for (auto& i : desc.RenderTarget)
		{
			i = blendDesc;
		}
		blendDesc.BlendEnable = FALSE;
		blendDesc.SrcBlend = GetNativeBlendFactor(_drawState.sharedBlendState.blendFactorSourceRGB, _drawState.sharedBlendState.blendOperationRGB);
		blendDesc.DestBlend = GetNativeBlendFactor(_drawState.sharedBlendState.blendFactorDestinationRGB, _drawState.sharedBlendState.blendOperationRGB);
		blendDesc.BlendOp = GetNativeBlendOperation(_drawState.sharedBlendState.blendOperationRGB);
		blendDesc.SrcBlendAlpha = GetNativeAlphaBlendFactor(_drawState.sharedBlendState.blendFactorSourceA, _drawState.sharedBlendState.blendOperationA);
		blendDesc.DestBlendAlpha = GetNativeAlphaBlendFactor(_drawState.sharedBlendState.blendFactorDestinationA, _drawState.sharedBlendState.blendOperationA);
		blendDesc.BlendOpAlpha = GetNativeBlendOperation(_drawState.sharedBlendState.blendOperationA);
		blendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		desc.RenderTarget[0] = blendDesc;
		desc.AlphaToCoverageEnable = FALSE;
		desc.IndependentBlendEnable = FALSE;
	}
}

//----------------------------------------------------------------------------------------
void Direct3DStateGroupNode::CreateNativeDepthStencilDescription(D3D12_DEPTH_STENCIL_DESC& depthStencilDescription) const
{
	// Configure depth test settings based on the supplied state
	depthStencilDescription = {};
	depthStencilDescription.DepthEnable = (_drawState.depthTestEnabled ? TRUE : FALSE);
	if (_drawState.depthWriteEnabled)
	{
		depthStencilDescription.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	}
	else
	{
		depthStencilDescription.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	}
	depthStencilDescription.DepthFunc = GetNativeDepthComparisonFunction(_drawState.depthComparisonTest);

	// Configure stencil test settings based on the supplied state
	if (_drawState.stencilEnabled)
	{
		depthStencilDescription.StencilEnable = TRUE;
		depthStencilDescription.StencilReadMask = UINT8(_drawState.stencilCompareMask);
		depthStencilDescription.StencilWriteMask = UINT8(_drawState.stencilWriteMask);
		depthStencilDescription.FrontFace.StencilFailOp = GetNativeStencilOperation(_drawState.stencilFrontFace.failOperation);
		depthStencilDescription.FrontFace.StencilDepthFailOp = GetNativeStencilOperation(_drawState.stencilFrontFace.depthFailOperation);
		depthStencilDescription.FrontFace.StencilPassOp = GetNativeStencilOperation(_drawState.stencilFrontFace.passOperation);
		depthStencilDescription.FrontFace.StencilFunc = GetNativeStencilComparisonFunction(_drawState.stencilFrontFace.comparisonTest);
		depthStencilDescription.BackFace.StencilFailOp = GetNativeStencilOperation(_drawState.stencilBackFace.failOperation);
		depthStencilDescription.BackFace.StencilDepthFailOp = GetNativeStencilOperation(_drawState.stencilBackFace.depthFailOperation);
		depthStencilDescription.BackFace.StencilPassOp = GetNativeStencilOperation(_drawState.stencilBackFace.passOperation);
		depthStencilDescription.BackFace.StencilFunc = GetNativeStencilComparisonFunction(_drawState.stencilBackFace.comparisonTest);
	}
	else
	{
		depthStencilDescription.StencilEnable = FALSE;
	}
}

//----------------------------------------------------------------------------------------
Direct3DStateGroupNode::BlendState Direct3DStateGroupNode::BuildBlendState(BlendOperation blendOperationRGB, BlendFactor blendFactorSourceRGB, BlendFactor blendFactorDestinationRGB, BlendOperation blendOperationA, BlendFactor blendFactorSourceA, BlendFactor blendFactorDestinationA)
{
	BlendState blendState = {};
	blendState.blendOperationRGB = blendOperationRGB;
	blendState.blendFactorSourceRGB = blendFactorSourceRGB;
	blendState.blendFactorDestinationRGB = blendFactorDestinationRGB;
	blendState.blendOperationA = blendOperationA;
	blendState.blendFactorSourceA = blendFactorSourceA;
	blendState.blendFactorDestinationA = blendFactorDestinationA;
	blendState.nativeBlendOperationRGB = GetNativeBlendOperation(blendState.blendOperationRGB);
	blendState.nativeBlendFactorSourceRGB = GetNativeBlendFactor(blendState.blendFactorSourceRGB, blendState.blendOperationRGB);
	blendState.nativeBlendFactorDestinationRGB = GetNativeBlendFactor(blendState.blendFactorDestinationRGB, blendState.blendOperationRGB);
	blendState.nativeBlendOperationA = GetNativeBlendOperation(blendState.blendOperationA);
	blendState.nativeBlendFactorSourceA = GetNativeBlendFactor(blendState.blendFactorSourceA, blendState.blendOperationA);
	blendState.nativeBlendFactorDestinationA = GetNativeBlendFactor(blendState.blendFactorDestinationA, blendState.blendOperationA);
	return blendState;
}

//----------------------------------------------------------------------------------------
Direct3DStateGroupNode::StencilFaceState Direct3DStateGroupNode::BuildStencilFaceState(StencilComparisonFunction comparisonTest, StencilOperation passOperation, StencilOperation failOperation, StencilOperation depthFailOperation)
{
	StencilFaceState stencilState = {};
	stencilState.comparisonTest = comparisonTest;
	stencilState.passOperation = passOperation;
	stencilState.failOperation = failOperation;
	stencilState.depthFailOperation = depthFailOperation;
	return stencilState;
}

//----------------------------------------------------------------------------------------
constexpr D3D12_BLEND_OP Direct3DStateGroupNode::GetNativeBlendOperation(BlendOperation operation)
{
	switch (operation)
	{
	case IStateGroupNode::BlendOperation::Add:
		return D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
	case IStateGroupNode::BlendOperation::Subtract:
		return D3D12_BLEND_OP::D3D12_BLEND_OP_SUBTRACT;
	case IStateGroupNode::BlendOperation::ReverseSubtract:
		return D3D12_BLEND_OP::D3D12_BLEND_OP_REV_SUBTRACT;
	case IStateGroupNode::BlendOperation::Min:
		return D3D12_BLEND_OP::D3D12_BLEND_OP_MIN;
	case IStateGroupNode::BlendOperation::Max:
		return D3D12_BLEND_OP::D3D12_BLEND_OP_MAX;
	}
	return D3D12_BLEND_OP::D3D12_BLEND_OP_ADD;
}

//----------------------------------------------------------------------------------------
constexpr D3D12_BLEND Direct3DStateGroupNode::GetNativeBlendFactor(BlendFactor factor, BlendOperation operation)
{
	// If the blend operation is Min or Max, we need to set our blend factors to 1, otherwise we trigger debug warnings.
	if ((operation == BlendOperation::Min) || (operation == BlendOperation::Max))
	{
		return D3D12_BLEND::D3D12_BLEND_ONE;
	}

	// Map to the native blend factor value
	switch (factor)
	{
	case IStateGroupNode::BlendFactor::Zero:
		return D3D12_BLEND::D3D12_BLEND_ZERO;
	case IStateGroupNode::BlendFactor::One:
		return D3D12_BLEND::D3D12_BLEND_ONE;
	case IStateGroupNode::BlendFactor::SourceColor:
		return D3D12_BLEND::D3D12_BLEND_SRC_COLOR;
	case IStateGroupNode::BlendFactor::OneMinusSourceColor:
		return D3D12_BLEND::D3D12_BLEND_INV_SRC_COLOR;
	case IStateGroupNode::BlendFactor::DestinationColor:
		return D3D12_BLEND::D3D12_BLEND_DEST_COLOR;
	case IStateGroupNode::BlendFactor::OneMinusDestinationColor:
		return D3D12_BLEND::D3D12_BLEND_INV_DEST_COLOR;
	case IStateGroupNode::BlendFactor::SourceAlpha:
		return D3D12_BLEND::D3D12_BLEND_SRC_ALPHA;
	case IStateGroupNode::BlendFactor::OneMinusSourceAlpha:
		return D3D12_BLEND::D3D12_BLEND_INV_SRC_ALPHA;
	case IStateGroupNode::BlendFactor::DestinationAlpha:
		return D3D12_BLEND::D3D12_BLEND_DEST_ALPHA;
	case IStateGroupNode::BlendFactor::OneMinusDestinationAlpha:
		return D3D12_BLEND::D3D12_BLEND_INV_DEST_ALPHA;
	}
	return D3D12_BLEND::D3D12_BLEND_ZERO;
}

//----------------------------------------------------------------------------------------
constexpr D3D12_BLEND Direct3DStateGroupNode::GetNativeAlphaBlendFactor(BlendFactor factor, BlendOperation operation)
{
	switch (factor)
	{
	case IStateGroupNode::BlendFactor::SourceColor:
		return D3D12_BLEND::D3D12_BLEND_SRC_ALPHA;
	case IStateGroupNode::BlendFactor::OneMinusSourceColor:
		return D3D12_BLEND::D3D12_BLEND_INV_SRC_ALPHA;
	case IStateGroupNode::BlendFactor::DestinationColor:
		return D3D12_BLEND::D3D12_BLEND_DEST_ALPHA;
	case IStateGroupNode::BlendFactor::OneMinusDestinationColor:
		return D3D12_BLEND::D3D12_BLEND_INV_DEST_ALPHA;
	}
	return GetNativeBlendFactor(factor, operation);
}

//----------------------------------------------------------------------------------------
constexpr D3D12_COMPARISON_FUNC Direct3DStateGroupNode::GetNativeDepthComparisonFunction(DepthComparisonFunction function)
{
	switch (function)
	{
	case DepthComparisonFunction::Never:
		return D3D12_COMPARISON_FUNC_NEVER;
	case DepthComparisonFunction::Equal:
		return D3D12_COMPARISON_FUNC_EQUAL;
	case DepthComparisonFunction::NotEqual:
		return D3D12_COMPARISON_FUNC_NOT_EQUAL;
	case DepthComparisonFunction::Less:
		return D3D12_COMPARISON_FUNC_LESS;
	case DepthComparisonFunction::LessOrEqual:
		return D3D12_COMPARISON_FUNC_LESS_EQUAL;
	case DepthComparisonFunction::Greater:
		return D3D12_COMPARISON_FUNC_GREATER;
	case DepthComparisonFunction::GreaterOrEqual:
		return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	case DepthComparisonFunction::Always:
		return D3D12_COMPARISON_FUNC_ALWAYS;
	}
	return D3D12_COMPARISON_FUNC_NEVER;
}

//----------------------------------------------------------------------------------------
constexpr D3D12_COMPARISON_FUNC Direct3DStateGroupNode::GetNativeStencilComparisonFunction(StencilComparisonFunction function)
{
	switch (function)
	{
	case StencilComparisonFunction::Never:
		return D3D12_COMPARISON_FUNC_NEVER;
	case StencilComparisonFunction::Equal:
		return D3D12_COMPARISON_FUNC_EQUAL;
	case StencilComparisonFunction::NotEqual:
		return D3D12_COMPARISON_FUNC_NOT_EQUAL;
	case StencilComparisonFunction::Less:
		return D3D12_COMPARISON_FUNC_LESS;
	case StencilComparisonFunction::LessOrEqual:
		return D3D12_COMPARISON_FUNC_LESS_EQUAL;
	case StencilComparisonFunction::Greater:
		return D3D12_COMPARISON_FUNC_GREATER;
	case StencilComparisonFunction::GreaterOrEqual:
		return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	case StencilComparisonFunction::Always:
		return D3D12_COMPARISON_FUNC_ALWAYS;
	}
	return D3D12_COMPARISON_FUNC_NEVER;
}

//----------------------------------------------------------------------------------------
constexpr D3D12_STENCIL_OP Direct3DStateGroupNode::GetNativeStencilOperation(StencilOperation operation)
{
	switch (operation)
	{
	case StencilOperation::Keep:
		return D3D12_STENCIL_OP_KEEP;
	case StencilOperation::Zero:
		return D3D12_STENCIL_OP_ZERO;
	case StencilOperation::Replace:
		return D3D12_STENCIL_OP_REPLACE;
	case StencilOperation::IncrementAndClamp:
		return D3D12_STENCIL_OP_INCR_SAT;
	case StencilOperation::DecrementAndClamp:
		return D3D12_STENCIL_OP_DECR_SAT;
	case StencilOperation::IncrementAndWrap:
		return D3D12_STENCIL_OP_INCR;
	case StencilOperation::DecrementAndWrap:
		return D3D12_STENCIL_OP_DECR;
	case StencilOperation::Invert:
		return D3D12_STENCIL_OP_INVERT;
	}
	return D3D12_STENCIL_OP_KEEP;
}

//----------------------------------------------------------------------------------------
// Debug methods
//----------------------------------------------------------------------------------------
void Direct3DStateGroupNode::SetDebugName(const Marshal::In<std::string>& name)
{
	// String is stored as wide string because render markers require a wide string
	_buildState.debugName = UTF8ToUTF16(name);
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
const std::wstring& Direct3DStateGroupNode::DebugName() const
{
	return _drawState.debugName;
}

} // namespace cobalt::graphics
