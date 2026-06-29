// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "VulkanStateGroupNode.h"
#include "VulkanRenderableNode.h"
#include "VulkanRenderer.h"
#include <Internal/RendererSupport/KnownDynamicCast.h>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
VulkanStateGroupNode::VulkanStateGroupNode(cobalt::logging::ILogger* log, VulkanRenderer* renderer)
: VulkanStateContainer(log)
{
	_log = log;
	_renderer = renderer;
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
	_computePipelineStateBucket.pipelineLayoutObjects.resize(1);
	_computePipelineStateBucket.pipelineStateObjects.resize(1);
}

//----------------------------------------------------------------------------------------
VulkanStateGroupNode::~VulkanStateGroupNode()
{
	_drawState.computeTaskDefined = false;
	for (auto& stateBucket : _pipelineStateBuckets)
	{
		for (size_t i = 0; i < _buildState.allocatedFrameBufferSlotCount; ++i)
		{
			DeleteNativePipelineState(stateBucket, (int)i);
		}
	}
	DeleteNativePipelineState(_computePipelineStateBucket, 0);
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void VulkanStateGroupNode::Delete()
{
	_renderer->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
// Child node methods
//----------------------------------------------------------------------------------------
void VulkanStateGroupNode::AddChildNode(IRenderableNode* childNode)
{
	// Ensure the specified node is in a valid state to be added as a child node
	auto* childNodeResolved = KnownDynamicCast<VulkanRenderableNode*>(childNode);
	if (!childNodeResolved->SetAsChildNode())
	{
		_log->Error("Failed to add renderable node as child");
	}

	// Retrieve the primitive information we need to match this renderable node to a state bucket
	VkPrimitiveTopology primitiveTopologyType;
	bool primitiveRestartEnabled;
	std::vector<int> vertexAttributeIDs;
	std::vector<int> instanceAttributeIDs;
	std::vector<VkFormat> attributeDataTypes;
	std::vector<size_t> attributeStrideInBytes;
	childNodeResolved->GetPipelineStateSettingsForRenderable(primitiveTopologyType, primitiveRestartEnabled, vertexAttributeIDs, instanceAttributeIDs, attributeDataTypes, attributeStrideInBytes);

	// Retrieve a state bucket which is compatible with the specified object
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	PipelineStateBucket& stateBucket = GetMatchingStateBucket(primitiveTopologyType, primitiveRestartEnabled, vertexAttributeIDs, instanceAttributeIDs, attributeDataTypes, attributeStrideInBytes);

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
void VulkanStateGroupNode::AddChildNodes(IRenderableNode* const* childNodes, size_t childNodeCount)
{
	// Add each of the specified child nodes to this group node
	VkPrimitiveTopology primitiveTopologyType;
	bool primitiveRestartEnabled;
	std::vector<int> vertexAttributeIDs;
	std::vector<int> instanceAttributeIDs;
	std::vector<VkFormat> attributeDataTypes;
	std::vector<size_t> attributeStrideInBytes;
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	for (size_t i = 0; i < childNodeCount; ++i)
	{
		// Ensure the specified node is in a valid state to be added as a child node
		auto* childNodeResolved = KnownDynamicCast<VulkanRenderableNode*>(*(childNodes++));
		if (!childNodeResolved->SetAsChildNode())
		{
			_log->Error("Failed to add renderable node as child");
			continue;
		}

		// Retrieve the primitive information we need to match this renderable node to a state bucket
		childNodeResolved->GetPipelineStateSettingsForRenderable(primitiveTopologyType, primitiveRestartEnabled, vertexAttributeIDs, instanceAttributeIDs, attributeDataTypes, attributeStrideInBytes);

		// Retrieve a state bucket which is compatible with the specified object
		PipelineStateBucket& stateBucket = GetMatchingStateBucket(primitiveTopologyType, primitiveRestartEnabled, vertexAttributeIDs, instanceAttributeIDs, attributeDataTypes, attributeStrideInBytes);

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
void VulkanStateGroupNode::AddChildNodes(IRenderableNode::unique_ptr const* childNodes, size_t childNodeCount)
{
	// Add each of the specified child nodes to this group node
	VkPrimitiveTopology primitiveTopologyType;
	bool primitiveRestartEnabled;
	std::vector<int> vertexAttributeIDs;
	std::vector<int> instanceAttributeIDs;
	std::vector<VkFormat> attributeDataTypes;
	std::vector<size_t> attributeStrideInBytes;
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	for (size_t i = 0; i < childNodeCount; ++i)
	{
		// Ensure the specified node is in a valid state to be added as a child node
		auto* childNodeResolved = KnownDynamicCast<VulkanRenderableNode*>((childNodes++)->get());
		if (!childNodeResolved->SetAsChildNode())
		{
			_log->Error("Failed to add renderable node as child");
			continue;
		}

		// Retrieve the primitive information we need to match this renderable node to a state bucket
		childNodeResolved->GetPipelineStateSettingsForRenderable(primitiveTopologyType, primitiveRestartEnabled, vertexAttributeIDs, instanceAttributeIDs, attributeDataTypes, attributeStrideInBytes);

		// Retrieve a state bucket which is compatible with the specified object
		PipelineStateBucket& stateBucket = GetMatchingStateBucket(primitiveTopologyType, primitiveRestartEnabled, vertexAttributeIDs, instanceAttributeIDs, attributeDataTypes, attributeStrideInBytes);

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
void VulkanStateGroupNode::RemoveChildNode(IRenderableNode* childNode)
{
	// Retrieve the ID of the specified child node
	auto* childNodeResolved = KnownDynamicCast<VulkanRenderableNode*>(childNode);
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
bool VulkanStateGroupNode::RemoveChildNode(VulkanRenderableNode* childNode, uint32_t childNodeID)
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
void VulkanStateGroupNode::RemoveChildNodes(IRenderableNode* const* childNodes, size_t childNodeCount)
{
	// Remove each of the specified child nodes from this group node
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	for (size_t i = 0; i < childNodeCount; ++i)
	{
		// Retrieve the ID of the specified child node
		auto* childNodeResolved = KnownDynamicCast<VulkanRenderableNode*>(*(childNodes++));
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
void VulkanStateGroupNode::RemoveChildNodes(IRenderableNode::unique_ptr const* childNodes, size_t childNodeCount)
{
	// Remove each of the specified child nodes from this group node
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	for (size_t i = 0; i < childNodeCount; ++i)
	{
		// Retrieve the ID of the specified child node
		auto* childNodeResolved = KnownDynamicCast<VulkanRenderableNode*>((childNodes++)->get());
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
void VulkanStateGroupNode::RemoveAllChildNodes()
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
void VulkanStateGroupNode::SetChildNodes(IRenderableNode* const* childNodes, size_t childNodeCount)
{
	RemoveAllChildNodes();
	AddChildNodes(childNodes, childNodeCount);
}

//----------------------------------------------------------------------------------------
void VulkanStateGroupNode::SetChildNodes(IRenderableNode::unique_ptr const* childNodes, size_t childNodeCount)
{
	RemoveAllChildNodes();
	AddChildNodes(childNodes, childNodeCount);
}

//----------------------------------------------------------------------------------------
const std::vector<VulkanRenderableNode*>& VulkanStateGroupNode::GetChildNodes(size_t stateBucketIndex) const
{
	return (_drawState.computeTaskDefined ? _computePipelineStateBucket.drawChildNodes : _pipelineStateBuckets[stateBucketIndex].drawChildNodes);
}

//----------------------------------------------------------------------------------------
// Compute methods
//----------------------------------------------------------------------------------------
bool VulkanStateGroupNode::HasComputeTask() const
{
	return _drawState.computeTaskDefined;
}

//----------------------------------------------------------------------------------------
V3UInt32 VulkanStateGroupNode::GetComputeThreadGroupCounts() const
{
	return _drawState.computeThreadGroupCounts;
}

//----------------------------------------------------------------------------------------
void VulkanStateGroupNode::SetComputeTask(const V3UInt32& threadGroupCounts)
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	_buildState.computeTaskDefined = true;
	_buildState.computeThreadGroupCounts = threadGroupCounts;
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void VulkanStateGroupNode::RemoveComputeTask()
{
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	_buildState.computeTaskDefined = false;
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
// Depth state methods
//----------------------------------------------------------------------------------------
void VulkanStateGroupNode::SetDepthTestEnabled(bool state)
{
	_buildState.depthTestEnabled = state;
	_buildStateChanged = true;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void VulkanStateGroupNode::SetDepthWriteEnabled(bool state)
{
	_buildState.depthWriteEnabled = state;
	_buildStateChanged = true;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void VulkanStateGroupNode::SetDepthComparisonFunction(DepthComparisonFunction comparisonTest)
{
	_buildState.depthComparisonTest = comparisonTest;
	_buildStateChanged = true;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void VulkanStateGroupNode::SetDepthBias(float constantFactor, float slopeFactor, float clamp)
{
	_buildState.depthBiasEnabled = true;
	_buildState.depthBiasConstantFactor = constantFactor;
	_buildState.depthBiasSlopeFactor = slopeFactor;
	_buildState.depthBiasClamp = clamp;
	_buildStateChanged = true;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void VulkanStateGroupNode::ClearDepthBias()
{
	_buildState.depthBiasEnabled = false;
	_buildStateChanged = true;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
// Stencil state methods
//----------------------------------------------------------------------------------------
void VulkanStateGroupNode::SetStencilTestEnabled(bool state, uint32_t compareMask, uint32_t writeMask)
{
	_buildState.stencilEnabled = state;
	_buildState.stencilCompareMask = compareMask;
	_buildState.stencilWriteMask = writeMask;
	_buildStateChanged = true;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void VulkanStateGroupNode::SetStencilOperation(StencilTargetFace targetFace, StencilComparisonFunction comparisonTest, StencilOperation passOperation, StencilOperation failOperation, StencilOperation depthFailOperation)
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
void VulkanStateGroupNode::SetStencilReferenceValue(uint32_t referenceValue)
{
	_buildState.stencilReferenceValue = referenceValue;
	_buildStateChanged = true;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
// Rasterization state methods
//----------------------------------------------------------------------------------------
void VulkanStateGroupNode::SetPolygonFillMode(PolygonFillMode fillMode)
{
	_buildState.polygonFillMode = fillMode;
	_buildStateChanged = true;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void VulkanStateGroupNode::SetPolygonCullMode(PolygonCullMode cullMode)
{
	_buildState.polygonCullMode = cullMode;
	_buildStateChanged = true;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void VulkanStateGroupNode::SetPolygonWindingOrder(PolygonWindingOrder windingOrder)
{
	_buildState.polygonWindingOrder = windingOrder;
	_buildStateChanged = true;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
// Blend state methods
//----------------------------------------------------------------------------------------
void VulkanStateGroupNode::SetBlendEnabled(bool state)
{
	_buildState.blendEnabled = state;
	_buildStateChanged = true;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void VulkanStateGroupNode::SetBlendMode(BlendOperation blendOperationRGB, BlendFactor blendFactorSourceRGB, BlendFactor blendFactorDestinationRGB, BlendOperation blendOperationA, BlendFactor blendFactorSourceA, BlendFactor blendFactorDestinationA)
{
	BlendState blendState = BuildBlendState(blendOperationRGB, blendFactorSourceRGB, blendFactorDestinationRGB, blendOperationA, blendFactorSourceA, blendFactorDestinationA);
	_buildState.sharedBlendState = blendState;
	_buildStateChanged = true;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void VulkanStateGroupNode::SetBlendMode(IFrameBuffer::AttachmentType type, size_t index, BlendOperation blendOperationRGB, BlendFactor blendFactorSourceRGB, BlendFactor blendFactorDestinationRGB, BlendOperation blendOperationA, BlendFactor blendFactorSourceA, BlendFactor blendFactorDestinationA)
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
// State methods
//----------------------------------------------------------------------------------------
VulkanStateGroupNode::PipelineStateBucket& VulkanStateGroupNode::GetMatchingStateBucket(VkPrimitiveTopology primitiveTopology, bool primitiveRestartEnabled, const std::vector<int>& vertexAttributeIDs, const std::vector<int>& instanceAttributeIDs, const std::vector<VkFormat>& attributeDataTypes, const std::vector<size_t>& attributeStrideInBytes)
{
	// Attempt to locate and update an existing compatible state bucket. We search in our set of new pipeline state
	// buckets first if there are any entries, as this represents buckets that have been added during this build process
	// since the last frame was drawn. As we expect objects being added in the same frame are probably correlated, it
	// makes sence to search the new buckets first in this case, as it's more likely we'll find a match there than in
	// the set of existing buckets.
	PipelineStateBucket* matchingStateBucket = nullptr;
	if (!_newPipelineStateBuckets.empty())
	{
		matchingStateBucket = GetMatchingStateBucketInSet(_newPipelineStateBuckets, primitiveTopology, primitiveRestartEnabled, vertexAttributeIDs, instanceAttributeIDs, attributeDataTypes, attributeStrideInBytes);
	}
	if (matchingStateBucket == nullptr)
	{
		matchingStateBucket = GetMatchingStateBucketInSet(_pipelineStateBuckets, primitiveTopology, primitiveRestartEnabled, vertexAttributeIDs, instanceAttributeIDs, attributeDataTypes, attributeStrideInBytes);
	}

	// If no existing state bucket was compatible, create a new one now.
	if (matchingStateBucket == nullptr)
	{
		// Create a new state bucket
		PipelineStateBucket stateBucket{};
		stateBucket.primitiveTopology = primitiveTopology;
		stateBucket.primitiveRestartEnabled = primitiveRestartEnabled;
		stateBucket.hasModifiedChildNodes = false;
		stateBucket.buildNativeObjectsCurrentForBucket = false;
		stateBucket.drawNativeObjectsCurrentForBucket.resize(_buildState.allocatedFrameBufferSlotCount, 0u);
		stateBucket.viewportLastUpdateTokens.resize(_buildState.allocatedFrameBufferSlotCount, 0);
		stateBucket.framebufferLastUpdateTokens.resize(_buildState.allocatedFrameBufferSlotCount, 0);
		stateBucket.pipelineStateObjects.resize(_buildState.allocatedFrameBufferSlotCount);
		stateBucket.pipelineLayoutObjects.resize(_buildState.allocatedFrameBufferSlotCount);

		// Calculate the number of known attribute slots
		int highestKnownAttributeID = 0;
		for (int vertexAttributeID : vertexAttributeIDs)
		{
			highestKnownAttributeID = std::max(highestKnownAttributeID, vertexAttributeID);
		}
		for (int instanceAttributeID : instanceAttributeIDs)
		{
			highestKnownAttributeID = std::max(highestKnownAttributeID, instanceAttributeID);
		}

		// Populate the instance attribute flags
		stateBucket.attributeTypeKnown.resize(highestKnownAttributeID + 1, 0);
		stateBucket.attributeIsInstanced.resize(highestKnownAttributeID + 1, 0);
		stateBucket.attributeDataType.resize(highestKnownAttributeID + 1, VK_FORMAT_UNDEFINED);
		stateBucket.attributeStrideInBytes.resize(highestKnownAttributeID + 1, VK_FORMAT_UNDEFINED);
		for (size_t i = 0; i < vertexAttributeIDs.size(); ++i)
		{
			auto vertexAttributeIndex = (size_t)vertexAttributeIDs[i];
			stateBucket.attributeTypeKnown[vertexAttributeIndex] = 1;
			stateBucket.attributeDataType[vertexAttributeIndex] = attributeDataTypes[i];
			stateBucket.attributeStrideInBytes[vertexAttributeIndex] = attributeStrideInBytes[i];
		}
		for (size_t i = 0; i < instanceAttributeIDs.size(); ++i)
		{
			auto instanceAttributeIndex = (size_t)instanceAttributeIDs[i];
			stateBucket.attributeTypeKnown[instanceAttributeIndex] = 1;
			stateBucket.attributeIsInstanced[instanceAttributeIndex] = 1;
			stateBucket.attributeDataType[instanceAttributeIndex] = attributeDataTypes[vertexAttributeIDs.size() + i];
			stateBucket.attributeStrideInBytes[instanceAttributeIndex] = attributeStrideInBytes[vertexAttributeIDs.size() + i];
		}

		// Add this new state bucket to the set of state buckets, and set it as the selected state bucket.
		_newPipelineStateBuckets.push_back(std::move(stateBucket));
		matchingStateBucket = &_newPipelineStateBuckets.back();
	}

	// Return the matching state bucket to the caller
	return *matchingStateBucket;
}

//----------------------------------------------------------------------------------------
VulkanStateGroupNode::PipelineStateBucket* VulkanStateGroupNode::GetMatchingStateBucketInSet(std::vector<PipelineStateBucket>& stateBucketSet, VkPrimitiveTopology primitiveTopology, bool primitiveRestartEnabled, const std::vector<int>& vertexAttributeIDs, const std::vector<int>& instanceAttributeIDs, const std::vector<VkFormat>& attributeDataTypes, const std::vector<size_t>& attributeStrideInBytes)
{
	// Attempt to locate and update an existing compatible state bucket
	for (auto& stateBucket : stateBucketSet)
	{
		// Compare the primitive topology
		if ((stateBucket.primitiveTopology != primitiveTopology) || (stateBucket.primitiveRestartEnabled != primitiveRestartEnabled))
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
			int vertexAttributeIndex = vertexAttributeIDs[i];
			highestKnownAttributeID = std::max(highestKnownAttributeID, vertexAttributeIndex);
			if ((vertexAttributeIndex < knownVertexAttributeCount) && (stateBucket.attributeTypeKnown[vertexAttributeIndex] != 0u) && ((stateBucket.attributeIsInstanced[vertexAttributeIndex] != 0u) || (stateBucket.attributeDataType[vertexAttributeIndex] != attributeDataTypes[i]) || (stateBucket.attributeStrideInBytes[vertexAttributeIndex] != attributeStrideInBytes[i])))
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
			int vertexAttributeIndex = instanceAttributeIDs[i];
			highestKnownAttributeID = std::max(highestKnownAttributeID, vertexAttributeIndex);
			if ((vertexAttributeIndex < knownVertexAttributeCount) && (stateBucket.attributeTypeKnown[vertexAttributeIndex] != 0u) && ((stateBucket.attributeIsInstanced[vertexAttributeIndex] == 0u) || (stateBucket.attributeDataType[vertexAttributeIndex] != attributeDataTypes[vertexAttributeIDCount + i]) || (stateBucket.attributeStrideInBytes[vertexAttributeIndex] != attributeStrideInBytes[vertexAttributeIDCount + i])))
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
			stateBucket.attributeDataType.resize(highestKnownAttributeID + 1, VK_FORMAT_UNDEFINED);
			stateBucket.attributeStrideInBytes.resize(highestKnownAttributeID + 1, 0);
		}
		for (size_t i = 0; i < vertexAttributeIDCount; ++i)
		{
			int vertexAttributeIndex = (int)vertexAttributeIDs[i];
			if (stateBucket.attributeTypeKnown[vertexAttributeIndex] == 0)
			{
				stateBucket.buildNativeObjectsCurrentForBucket = false;
				stateBucket.attributeTypeKnown[vertexAttributeIndex] = 1;
				stateBucket.attributeDataType[vertexAttributeIndex] = attributeDataTypes[i];
				stateBucket.attributeStrideInBytes[vertexAttributeIndex] = attributeStrideInBytes[i];
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
				stateBucket.attributeStrideInBytes[vertexAttributeIndex] = attributeStrideInBytes[vertexAttributeIDCount + i];
			}
		}

		// Return this state bucket to the caller
		return &stateBucket;
	}
	return nullptr;
}

//----------------------------------------------------------------------------------------
VkPipelineLayout VulkanStateGroupNode::GetPipelineLayout(int stateBucketIndex, int frameBufferIndex)
{
	return (_drawState.computeTaskDefined ? _computePipelineStateBucket.pipelineLayoutObjects[0] : _pipelineStateBuckets[stateBucketIndex].pipelineLayoutObjects[frameBufferIndex]);
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
int VulkanStateGroupNode::GetStateBucketCount() const
{
	return (_drawState.computeTaskDefined ? 1 : (int)_pipelineStateBuckets.size());
}

//----------------------------------------------------------------------------------------
void VulkanStateGroupNode::ClearFrameBufferEntry(int frameBufferIndex)
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
void VulkanStateGroupNode::ClearAllFrameBufferEntries()
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
void VulkanStateGroupNode::SetFrameBufferCount(int count)
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
		pipelineStateBucket.viewportLastUpdateTokens.resize(_buildState.allocatedFrameBufferSlotCount, 0);
		pipelineStateBucket.framebufferLastUpdateTokens.resize(_buildState.allocatedFrameBufferSlotCount, 0);
		pipelineStateBucket.pipelineStateObjects.resize(_buildState.allocatedFrameBufferSlotCount);
		pipelineStateBucket.pipelineLayoutObjects.resize(_buildState.allocatedFrameBufferSlotCount);
	}
	lock.unlock();
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void VulkanStateGroupNode::MigrateBuildStateToDrawState()
{
	// Migrate our build state
	if (!IsDrawStateCurrent())
	{
		// Transfer basic build state
		VulkanStateContainer::MigrateBuildStateToDrawState();
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
				pipelineStateBucket.viewportLastUpdateTokens.resize(allocatedFrameBufferSlotCount, 0);
				pipelineStateBucket.framebufferLastUpdateTokens.resize(allocatedFrameBufferSlotCount, 0);
				pipelineStateBucket.pipelineStateObjects.resize(allocatedFrameBufferSlotCount);
				pipelineStateBucket.pipelineLayoutObjects.resize(allocatedFrameBufferSlotCount);
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
					DeleteNativePipelineState(pipelineStateBucket, frameBufferIndex);
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
				stateBucket.drawNativeObjectsCurrentForBucket[i] = (unsigned char)(!buildStateChanged && (stateBucket.drawNativeObjectsCurrentForBucket[i] != 0u) && stateBucket.buildNativeObjectsCurrentForBucket);
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
					size_t firstRemovedChildNodeIndex = stateBucket.removedChildNodeIDs.front();

					// Transfer the remaining valid child node entries to our draw array
					size_t nextWriteIndex = 0;
					size_t nextReadIndex = 0;
					size_t removedChildNodeCount = stateBucket.removedChildNodeIDs.size();
					size_t newChildNodeCount = stateBucket.buildChildNodes.size() - removedChildNodeCount;
					stateBucket.drawChildNodes.resize(newChildNodeCount);
					size_t removedChildNodeSearchIndex = 0;
					while (removedChildNodeSearchIndex < removedChildNodeCount)
					{
						size_t removedIndex = stateBucket.removedChildNodeIDs[removedChildNodeSearchIndex++];
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
					for (size_t i = firstRemovedChildNodeIndex; i < newChildNodeCount; ++i)
					{
						stateBucket.buildChildNodes[i]->SetChildNodeId((uint32_t)i);
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
		for (VulkanRenderableNode* entry : stateBucket.drawChildNodes)
		{
			entry->MigrateBuildStateToDrawState();
		}
	}
}

//----------------------------------------------------------------------------------------
void VulkanStateGroupNode::ApplyFixedState(int stateBucketIndex, int frameBufferIndex, VkCommandBuffer& commandBuffer, VulkanShaderProgram* shaderProgram, VulkanFrameBuffer* framebuffer)
{
	// If this is a compute task node, reset the frame buffer index to 0.
	if (_drawState.computeTaskDefined)
	{
		frameBufferIndex = 0;
	}

	// Update our native state objects if required
	PipelineStateBucket& stateBucket = (_drawState.computeTaskDefined ? _computePipelineStateBucket : _pipelineStateBuckets[stateBucketIndex]);
	if ((stateBucket.drawNativeObjectsCurrentForBucket[frameBufferIndex] == 0u) || (!_drawState.computeTaskDefined && ((framebuffer->GetViewportLastUpdateToken() != stateBucket.viewportLastUpdateTokens[frameBufferIndex]) || (framebuffer->GetFrameBufferObjectLastUpdateToken() != stateBucket.framebufferLastUpdateTokens[frameBufferIndex]))))
	{
		// Delete the previous pipeline state object for this state bucket and framebuffer
		DeleteNativePipelineState(stateBucket, frameBufferIndex);

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
			stateBucket.viewportLastUpdateTokens[frameBufferIndex] = framebuffer->GetViewportLastUpdateToken();
			stateBucket.framebufferLastUpdateTokens[frameBufferIndex] = framebuffer->GetFrameBufferObjectLastUpdateToken();
		}
	}

	// Bind the pipeline state object for this node
	vkCmdBindPipeline(commandBuffer, (_drawState.computeTaskDefined ? VK_PIPELINE_BIND_POINT_COMPUTE : VK_PIPELINE_BIND_POINT_GRAPHICS), stateBucket.pipelineStateObjects[frameBufferIndex]);

	// Initialize our vertex buffer bindings. We need this to avoid a violation of the Vulkan spec:
	// https://vulkan.lunarg.com/doc/view/1.2.148.0/windows/1.2-extensions/vkspec.html#VUID-vkCmdDrawIndexed-None-04007
	if (!_drawState.computeTaskDefined)
	{
		if (!stateBucket.nullVertexBufferBindings.empty())
		{
			vkCmdBindVertexBuffers(commandBuffer, 0, (uint32_t)stateBucket.nullVertexBufferBindings.size(), stateBucket.nullVertexBufferBindings.data(), stateBucket.nullVertexBufferOffsets.data());
		}
	}
}

//----------------------------------------------------------------------------------------
bool VulkanStateGroupNode::CreateNativeComputePipelineState(PipelineStateBucket& stateBucket, VulkanShaderProgram* shaderProgram)
{
	// Compute shader stage
	auto computeShaderEntryPointName = shaderProgram->GetShaderEntryPointName(VulkanShaderProgram::ShaderStage::Compute);
	VkPipelineShaderStageCreateInfo computeShaderStageInfo = {};
	computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	computeShaderStageInfo.module = shaderProgram->GetShaderModule(VulkanShaderProgram::ShaderStage::Compute);
	computeShaderStageInfo.pName = computeShaderEntryPointName.c_str();

	// Get descriptor set layout
	const auto& descriptorLayouts = shaderProgram->GetDescriptorSetLayouts();

	// Pipeline layout info
	//Note that while it might be expected that push constants in Vulkan and root constants in Direct3D 12 are worth
	//looking into for performance reasons, this doesn't seem to be true in practice. See the end of the discussion on
	//implementing support for this in WebGPU, when performance figures are gathered for a range of hardware:
	//https://github.com/gpuweb/gpuweb/issues/75
	//These results show no improvement, or in some cases regressions when trying to use this feature over standard
	//constant buffers. This is the reason we haven't pursued exposing state values which are backed by push constants.
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = (uint32_t)descriptorLayouts.size();
	pipelineLayoutInfo.pSetLayouts = descriptorLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;

	// Create pipeline layout
	VkResult createComputePipelineLayoutResult = vkCreatePipelineLayout(_renderer->GetDevice(), &pipelineLayoutInfo, nullptr, _computePipelineStateBucket.pipelineLayoutObjects.data());
	if (createComputePipelineLayoutResult != VK_SUCCESS)
	{
		_log->Error("Could not create pipeline layout with error code {0}", createComputePipelineLayoutResult);
		return false;
	}

	// Create pipeline
	VkComputePipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.flags = 0;
	pipelineInfo.stage = computeShaderStageInfo;
	pipelineInfo.layout = stateBucket.pipelineLayoutObjects[0];
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	// Make pipeline
	VkResult createComputePipelinesResult = vkCreateComputePipelines(_renderer->GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, stateBucket.pipelineStateObjects.data());
	if (createComputePipelinesResult != VK_SUCCESS)
	{
		_log->Error("Could not create compute pipeline with error code {0}", createComputePipelinesResult);
		return false;
	}
	return true;
}

//----------------------------------------------------------------------------------------
bool VulkanStateGroupNode::CreateNativeGraphicsPipelineState(PipelineStateBucket& stateBucket, int frameBufferIndex, VulkanShaderProgram* shaderProgram, VulkanFrameBuffer* framebuffer)
{
	// Vertex shader stage
	auto vertexShaderEntryPointName = shaderProgram->GetShaderEntryPointName(VulkanShaderProgram::ShaderStage::Vertex);
	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = shaderProgram->GetShaderModule(VulkanShaderProgram::ShaderStage::Vertex);
	vertShaderStageInfo.pName = vertexShaderEntryPointName.c_str();

	// Fragment shader stage
	auto fragmentShaderEntryPointName = shaderProgram->GetShaderEntryPointName(VulkanShaderProgram::ShaderStage::Fragment);
	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = shaderProgram->GetShaderModule(VulkanShaderProgram::ShaderStage::Fragment);
	fragShaderStageInfo.pName = fragmentShaderEntryPointName.c_str();

	// Shader stage info
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages = {vertShaderStageInfo, fragShaderStageInfo};

	// Add geometry shader if it exists
	std::string geometryShaderEntryPointName;
	if (shaderProgram->GetShaderModule(VulkanShaderProgram::ShaderStage::Geometry) != VK_NULL_HANDLE)
	{
		geometryShaderEntryPointName = shaderProgram->GetShaderEntryPointName(VulkanShaderProgram::ShaderStage::Geometry);
		VkPipelineShaderStageCreateInfo geomShaderStageInfo = {};
		geomShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		geomShaderStageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
		geomShaderStageInfo.module = shaderProgram->GetShaderModule(VulkanShaderProgram::ShaderStage::Geometry);
		geomShaderStageInfo.pName = geometryShaderEntryPointName.c_str();
		shaderStages.push_back(geomShaderStageInfo);
	}

	// Vertex attribute binding information
	auto bindingDescriptions = shaderProgram->GetVertexBindingDescriptions();
	auto attributeDescriptions = shaderProgram->GetVertexAttributeDescriptions();
	for (size_t i = 0; i < bindingDescriptions.size(); ++i)
	{
		if (stateBucket.attributeIsInstanced.size() <= i)
		{
			break;
		}
		if (stateBucket.attributeIsInstanced[i] != 0)
		{
			bindingDescriptions[i].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
		}
		if (stateBucket.attributeTypeKnown[i] != 0)
		{
			bindingDescriptions[i].stride = (uint32_t)stateBucket.attributeStrideInBytes[i];
		}
	}
	for (size_t i = 0; i < attributeDescriptions.size(); ++i)
	{
		if (stateBucket.attributeTypeKnown.size() <= i)
		{
			break;
		}
		if (stateBucket.attributeTypeKnown[i] != 0)
		{
			attributeDescriptions[i].format = stateBucket.attributeDataType[i];
		}
	}

	// Prepare some buffers to clear our vertex buffer bindings
	stateBucket.nullVertexBufferBindings.resize(bindingDescriptions.size(), (_renderer->NullDescriptorFeatureMissingOrBroken() ? _renderer->GetNullDescriptorFallbackVertexBuffer() : VK_NULL_HANDLE));
	stateBucket.nullVertexBufferOffsets.resize(bindingDescriptions.size(), VkDeviceSize(0));

	// Set vertex input info
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = (uint32_t)bindingDescriptions.size();
	vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
	vertexInputInfo.vertexAttributeDescriptionCount = (uint32_t)attributeDescriptions.size();
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	// Assembly input info
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = stateBucket.primitiveTopology;
#ifdef __APPLE__
	// Primitive restart is always active under Metal (through MoltenVK). We force it to true here to avoid warnings
	// from the validation layer. Note that this isn't unique to MoltenVK, as primitive restart is also always active
	// under Direct3D11, and can't be disabled. Things get messy thoguh, as on older macs, the extension is reported
	// as supported, but if primitiveRestartEnable is set to false, a validation error is reported. In newer versions,
	// it appears the extension is not reported as supported, causing device creation to fail if we request it. If we
	// then set primitiveRestartEnable is set to true, we get a validation error, as we're explicitly not allowed to
	// set it to true unless we've requested the extension and enabled primitive restart. If we set it to false though,
	// that previous warning comes back telling us that it's always on and can't be disabled under MoltenVK, so we try
	// our best to handle things to avoid validation warnings here, but we can't always do that, despite the fact that
	// primitive restart is always on either way.
	inputAssembly.primitiveRestartEnable = (_renderer->PrimitiveRestartSupported() ? VK_TRUE : VK_FALSE);
#else
	inputAssembly.primitiveRestartEnable = (stateBucket.primitiveRestartEnabled ? VK_TRUE : VK_FALSE);
#endif

	// Viewport state info
	stateBucket.viewportLastUpdateTokens[frameBufferIndex] = framebuffer->GetViewportLastUpdateToken();
	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = framebuffer->GetViewportDefinition();
	viewportState.scissorCount = 1;
	viewportState.pScissors = framebuffer->GetScissorDefinition();

	// Rasterization info
	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.lineWidth = 1.0f;
	if (_drawState.polygonFillMode == PolygonFillMode::Wireframe)
	{
		rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
	}
	else if (_drawState.polygonFillMode == PolygonFillMode::Solid)
	{
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	}
	else
	{
		UNREACHABLE();
	}
	if (_drawState.polygonCullMode == PolygonCullMode::Back)
	{
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	}
	else if (_drawState.polygonCullMode == PolygonCullMode::Front)
	{
		rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
	}
	else if (_drawState.polygonCullMode == PolygonCullMode::None)
	{
		rasterizer.cullMode = VK_CULL_MODE_NONE;
	}
	else
	{
		UNREACHABLE();
	}
	if (_drawState.polygonWindingOrder == PolygonWindingOrder::Clockwise)
	{
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	}
	else if (_drawState.polygonWindingOrder == PolygonWindingOrder::CounterClockwise)
	{
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	}
	else
	{
		UNREACHABLE();
	}
	if (_drawState.depthBiasEnabled)
	{
		rasterizer.depthBiasEnable = VK_TRUE;
		rasterizer.depthBiasConstantFactor = _drawState.depthBiasConstantFactor;
		rasterizer.depthBiasSlopeFactor = _drawState.depthBiasSlopeFactor;
		rasterizer.depthBiasClamp = _drawState.depthBiasClamp;
	}
	else
	{
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f;
		rasterizer.depthBiasSlopeFactor = 0.0f;
		rasterizer.depthBiasClamp = 0.0f;
	}

	// Configure multisampling
	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = framebuffer->GetSampleCountNative();
	multisampling.minSampleShading = 1.0f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;

	// Create blend attachments for all color attachments
	size_t attachmentCount = framebuffer->GetRenderPassColorAttachmentReferenceCount();
	VkPipelineColorBlendAttachmentState defaultBlend = {};
	defaultBlend.blendEnable = (_drawState.blendEnabled ? VK_TRUE : VK_FALSE);
	defaultBlend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	defaultBlend.srcColorBlendFactor = GetNativeBlendFactor(_drawState.sharedBlendState.blendFactorSourceRGB);
	defaultBlend.dstColorBlendFactor = GetNativeBlendFactor(_drawState.sharedBlendState.blendFactorDestinationRGB);
	defaultBlend.colorBlendOp = GetNativeBlendOperation(_drawState.sharedBlendState.blendOperationRGB);
	defaultBlend.srcAlphaBlendFactor = GetNativeBlendFactor(_drawState.sharedBlendState.blendFactorSourceA);
	defaultBlend.dstAlphaBlendFactor = GetNativeBlendFactor(_drawState.sharedBlendState.blendFactorDestinationA);
	defaultBlend.alphaBlendOp = GetNativeBlendOperation(_drawState.sharedBlendState.blendOperationA);
	std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments(attachmentCount, defaultBlend);
	if (_drawState.blendEnabled)
	{
		for (const BlendState& state : _drawState.attachmentTypeBlendState)
		{
			VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
			int attachmentIndex = framebuffer->GetRenderPassAttachmentIndex(state.type, state.index);
			if (attachmentIndex >= 0)
			{
				colorBlendAttachment.blendEnable = VK_TRUE;
				colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
				colorBlendAttachment.srcColorBlendFactor = state.nativeBlendFactorSourceRGB;
				colorBlendAttachment.dstColorBlendFactor = state.nativeBlendFactorDestinationRGB;
				colorBlendAttachment.colorBlendOp = state.nativeBlendOperationRGB;
				colorBlendAttachment.srcAlphaBlendFactor = state.nativeBlendFactorSourceA;
				colorBlendAttachment.dstAlphaBlendFactor = state.nativeBlendFactorDestinationA;
				colorBlendAttachment.alphaBlendOp = state.nativeBlendOperationA;
				colorBlendAttachments[attachmentIndex] = colorBlendAttachment;
			}
		}
	}

	// Set color blend state
	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = (uint32_t)colorBlendAttachments.size();
	colorBlending.pAttachments = colorBlendAttachments.data();
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	// Configure depth test settings based on the supplied state
	VkPipelineDepthStencilStateCreateInfo depthInfo = {};
	depthInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthInfo.depthTestEnable = (_drawState.depthTestEnabled ? VK_TRUE : VK_FALSE);
	depthInfo.depthWriteEnable = (_drawState.depthWriteEnabled ? VK_TRUE : VK_FALSE);
	depthInfo.depthCompareOp = GetNativeDepthComparisonFunction(_drawState.depthComparisonTest);
	depthInfo.depthBoundsTestEnable = VK_FALSE;
	depthInfo.minDepthBounds = 0.0f;
	depthInfo.maxDepthBounds = 1.0f;

	// Configure stencil test settings based on the supplied state
	if (_drawState.stencilEnabled)
	{
		depthInfo.stencilTestEnable = VK_TRUE;
		depthInfo.front.compareMask = _drawState.stencilCompareMask;
		depthInfo.front.writeMask = _drawState.stencilWriteMask;
		depthInfo.front.failOp = GetNativeStencilOperation(_drawState.stencilFrontFace.failOperation);
		depthInfo.front.depthFailOp = GetNativeStencilOperation(_drawState.stencilFrontFace.depthFailOperation);
		depthInfo.front.passOp = GetNativeStencilOperation(_drawState.stencilFrontFace.passOperation);
		depthInfo.front.compareOp = GetNativeStencilComparisonFunction(_drawState.stencilFrontFace.comparisonTest);
		depthInfo.front.reference = _drawState.stencilReferenceValue;
		depthInfo.back.compareMask = _drawState.stencilCompareMask;
		depthInfo.back.writeMask = _drawState.stencilWriteMask;
		depthInfo.back.failOp = GetNativeStencilOperation(_drawState.stencilBackFace.failOperation);
		depthInfo.back.depthFailOp = GetNativeStencilOperation(_drawState.stencilBackFace.depthFailOperation);
		depthInfo.back.passOp = GetNativeStencilOperation(_drawState.stencilBackFace.passOperation);
		depthInfo.back.compareOp = GetNativeStencilComparisonFunction(_drawState.stencilBackFace.comparisonTest);
		depthInfo.back.reference = _drawState.stencilReferenceValue;
	}
	else
	{
		depthInfo.stencilTestEnable = VK_FALSE;
	}

	// Get descriptor set layout
	const auto& descriptorLayouts = shaderProgram->GetDescriptorSetLayouts();

	// Pipeline layout info
	//Note that while it might be expected that push constants in Vulkan and root constants in Direct3D 12 are worth
	//looking into for performance reasons, this doesn't seem to be true in practice. See the end of the discussion on
	//implementing support for this in WebGPU, when performance figures are gathered for a range of hardware:
	//https://github.com/gpuweb/gpuweb/issues/75
	//These results show no improvement, or in some cases regressions when trying to use this feature over standard
	//constant buffers. This is the reason we haven't pursued exposing state values which are backed by push constants.
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = (uint32_t)descriptorLayouts.size();
	pipelineLayoutInfo.pSetLayouts = descriptorLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;

	// Create pipeline layout
	VkResult createGraphicsPipelineLayoutResult = vkCreatePipelineLayout(_renderer->GetDevice(), &pipelineLayoutInfo, nullptr, &stateBucket.pipelineLayoutObjects[frameBufferIndex]);
	if (createGraphicsPipelineLayoutResult != VK_SUCCESS)
	{
		_log->Error("Could not create pipeline layout with error code {0}", createGraphicsPipelineLayoutResult);
		return false;
	}

	// Create pipeline
	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = (uint32_t)shaderStages.size();
	pipelineInfo.pStages = shaderStages.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pDepthStencilState = &depthInfo;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.layout = stateBucket.pipelineLayoutObjects[frameBufferIndex];
	pipelineInfo.renderPass = framebuffer->GetTemplateRenderPass();
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	// Make pipeline
	VkResult createGraphicsPipelinesResult = vkCreateGraphicsPipelines(_renderer->GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &stateBucket.pipelineStateObjects[frameBufferIndex]);
	if (createGraphicsPipelinesResult != VK_SUCCESS)
	{
		_log->Error("Could not create graphics pipeline with error code {0}", createGraphicsPipelinesResult);
		return false;
	}
	return true;
}

//----------------------------------------------------------------------------------------
void VulkanStateGroupNode::DeleteNativePipelineState(PipelineStateBucket& state, int frameBufferIndex)
{
	if (_drawState.computeTaskDefined)
	{
		frameBufferIndex = 0;
	}
	if (state.pipelineLayoutObjects[frameBufferIndex] != VK_NULL_HANDLE)
	{
		vkDestroyPipelineLayout(_renderer->GetDevice(), state.pipelineLayoutObjects[frameBufferIndex], nullptr);
		state.pipelineLayoutObjects[frameBufferIndex] = VK_NULL_HANDLE;
	}
	if (state.pipelineStateObjects[frameBufferIndex] != VK_NULL_HANDLE)
	{
		vkDestroyPipeline(_renderer->GetDevice(), state.pipelineStateObjects[frameBufferIndex], nullptr);
		state.pipelineStateObjects[frameBufferIndex] = VK_NULL_HANDLE;
	}
}

//----------------------------------------------------------------------------------------
VulkanStateGroupNode::BlendState VulkanStateGroupNode::BuildBlendState(BlendOperation blendOperationRGB, BlendFactor blendFactorSourceRGB, BlendFactor blendFactorDestinationRGB, BlendOperation blendOperationA, BlendFactor blendFactorSourceA, BlendFactor blendFactorDestinationA)
{
	BlendState blendState{};
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
VulkanStateGroupNode::StencilFaceState VulkanStateGroupNode::BuildStencilFaceState(StencilComparisonFunction comparisonTest, StencilOperation passOperation, StencilOperation failOperation, StencilOperation depthFailOperation)
{
	StencilFaceState stencilState = {};
	stencilState.comparisonTest = comparisonTest;
	stencilState.passOperation = passOperation;
	stencilState.failOperation = failOperation;
	stencilState.depthFailOperation = depthFailOperation;
	return stencilState;
}

//----------------------------------------------------------------------------------------
constexpr VkBlendOp VulkanStateGroupNode::GetNativeBlendOperation(BlendOperation operation)
{
	switch (operation)
	{
	case IStateGroupNode::BlendOperation::Add:
		return VK_BLEND_OP_ADD;
	case IStateGroupNode::BlendOperation::Subtract:
		return VK_BLEND_OP_SUBTRACT;
	case IStateGroupNode::BlendOperation::ReverseSubtract:
		return VK_BLEND_OP_REVERSE_SUBTRACT;
	case IStateGroupNode::BlendOperation::Min:
		return VK_BLEND_OP_MIN;
	case IStateGroupNode::BlendOperation::Max:
		return VK_BLEND_OP_MAX;
	}
	UNREACHABLE();
	return VK_BLEND_OP_ADD;
}

//----------------------------------------------------------------------------------------
constexpr VkBlendFactor VulkanStateGroupNode::GetNativeBlendFactor(BlendFactor factor)
{
	switch (factor)
	{
	case IStateGroupNode::BlendFactor::Zero:
		return VK_BLEND_FACTOR_ZERO;
	case IStateGroupNode::BlendFactor::One:
		return VK_BLEND_FACTOR_ONE;
	case IStateGroupNode::BlendFactor::SourceColor:
		return VK_BLEND_FACTOR_SRC_COLOR;
	case IStateGroupNode::BlendFactor::OneMinusSourceColor:
		return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
	case IStateGroupNode::BlendFactor::DestinationColor:
		return VK_BLEND_FACTOR_DST_COLOR;
	case IStateGroupNode::BlendFactor::OneMinusDestinationColor:
		return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
	case IStateGroupNode::BlendFactor::SourceAlpha:
		return VK_BLEND_FACTOR_SRC_ALPHA;
	case IStateGroupNode::BlendFactor::OneMinusSourceAlpha:
		return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	case IStateGroupNode::BlendFactor::DestinationAlpha:
		return VK_BLEND_FACTOR_DST_ALPHA;
	case IStateGroupNode::BlendFactor::OneMinusDestinationAlpha:
		return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
	}
	UNREACHABLE();
	return VK_BLEND_FACTOR_ZERO;
}

//----------------------------------------------------------------------------------------
constexpr VkCompareOp VulkanStateGroupNode::GetNativeDepthComparisonFunction(DepthComparisonFunction function)
{
	switch (function)
	{
	case DepthComparisonFunction::Never:
		return VK_COMPARE_OP_NEVER;
	case DepthComparisonFunction::Equal:
		return VK_COMPARE_OP_EQUAL;
	case DepthComparisonFunction::NotEqual:
		return VK_COMPARE_OP_NOT_EQUAL;
	case DepthComparisonFunction::Less:
		return VK_COMPARE_OP_LESS;
	case DepthComparisonFunction::LessOrEqual:
		return VK_COMPARE_OP_LESS_OR_EQUAL;
	case DepthComparisonFunction::Greater:
		return VK_COMPARE_OP_GREATER;
	case DepthComparisonFunction::GreaterOrEqual:
		return VK_COMPARE_OP_GREATER_OR_EQUAL;
	case DepthComparisonFunction::Always:
		return VK_COMPARE_OP_ALWAYS;
	}
	UNREACHABLE();
	return VK_COMPARE_OP_NEVER;
}

//----------------------------------------------------------------------------------------
constexpr VkCompareOp VulkanStateGroupNode::GetNativeStencilComparisonFunction(StencilComparisonFunction function)
{
	switch (function)
	{
	case StencilComparisonFunction::Never:
		return VK_COMPARE_OP_NEVER;
	case StencilComparisonFunction::Equal:
		return VK_COMPARE_OP_EQUAL;
	case StencilComparisonFunction::NotEqual:
		return VK_COMPARE_OP_NOT_EQUAL;
	case StencilComparisonFunction::Less:
		return VK_COMPARE_OP_LESS;
	case StencilComparisonFunction::LessOrEqual:
		return VK_COMPARE_OP_LESS_OR_EQUAL;
	case StencilComparisonFunction::Greater:
		return VK_COMPARE_OP_GREATER;
	case StencilComparisonFunction::GreaterOrEqual:
		return VK_COMPARE_OP_GREATER_OR_EQUAL;
	case StencilComparisonFunction::Always:
		return VK_COMPARE_OP_ALWAYS;
	}
	UNREACHABLE();
	return VK_COMPARE_OP_NEVER;
}

//----------------------------------------------------------------------------------------
constexpr VkStencilOp VulkanStateGroupNode::GetNativeStencilOperation(StencilOperation operation)
{
	switch (operation)
	{
	case StencilOperation::Keep:
		return VK_STENCIL_OP_KEEP;
	case StencilOperation::Zero:
		return VK_STENCIL_OP_ZERO;
	case StencilOperation::Replace:
		return VK_STENCIL_OP_REPLACE;
	case StencilOperation::IncrementAndClamp:
		return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
	case StencilOperation::DecrementAndClamp:
		return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
	case StencilOperation::IncrementAndWrap:
		return VK_STENCIL_OP_INCREMENT_AND_WRAP;
	case StencilOperation::DecrementAndWrap:
		return VK_STENCIL_OP_DECREMENT_AND_WRAP;
	case StencilOperation::Invert:
		return VK_STENCIL_OP_INVERT;
	}
	UNREACHABLE();
	return VK_STENCIL_OP_KEEP;
}

//----------------------------------------------------------------------------------------
// Render methods
//----------------------------------------------------------------------------------------
VulkanShaderProgram::GlobalConstantBufferBindingInfo& VulkanStateGroupNode::GetGlobalConstantBufferBindingInfo(int constantStateIndex)
{
	if (constantStateIndex < 0)
	{
		return _globalConstantBufferBindingInfo;
	}
	if (constantStateIndex >= _globalConstantBufferBindingInfoWithDefaultsSize)
	{
		_globalConstantBufferBindingInfoWithDefaultsSize = constantStateIndex + 1;
		_globalConstantBufferBindingInfoWithDefaults.resize(_globalConstantBufferBindingInfoWithDefaultsSize);
	}
	return _globalConstantBufferBindingInfoWithDefaults[constantStateIndex];
}

//----------------------------------------------------------------------------------------
// Debug methods
//----------------------------------------------------------------------------------------
void VulkanStateGroupNode::SetDebugName(const Marshal::In<std::string>& name)
{
	_buildState.debugName = name;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
const std::string& VulkanStateGroupNode::DebugName() const
{
	return _drawState.debugName;
}

} // namespace cobalt::graphics
