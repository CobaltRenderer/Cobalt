// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "Direct3DStateGroupNode.h"
#include "Direct3DRenderableNode.h"
#include "Direct3DRenderer.h"
#include <Internal/RendererSupport/KnownDynamicCast.h>
#include <Internal/RendererSupport/UnicodeConversion.h>
#include <algorithm>
namespace cobalt::graphics {
using Microsoft::WRL::ComPtr;

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
Direct3DStateGroupNode::Direct3DStateGroupNode(cobalt::logging::ILogger* log, Direct3DRenderer* renderer)
: Direct3DStateContainer(log), _renderer(renderer), _log(log)
{
	_hasModifiedChildNodes = false;
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
	_buildState.nativeObjectsCurrent = false;
	_drawState.nativeObjectsCurrent = false;
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
void Direct3DStateGroupNode::AddChildNodes(IRenderableNode* const* childNodes, size_t childNodeCount)
{
	// Add each of the specified child nodes to this group node
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	size_t currentChildCount = _buildChildNodes.size();
	size_t newChildCount = currentChildCount + childNodeCount;
	_buildChildNodes.resize(newChildCount);
	for (size_t i = currentChildCount; i < newChildCount; ++i)
	{
		// Ensure the specified node is in a valid state to be added as a child node
		auto* childNodeResolved = KnownDynamicCast<Direct3DRenderableNode*>(*(childNodes++));
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
void Direct3DStateGroupNode::AddChildNodes(IRenderableNode::unique_ptr const* childNodes, size_t childNodeCount)
{
	// Add each of the specified child nodes to this group node
	std::unique_lock<std::mutex> lock(_childNodeMutex);
	size_t currentChildCount = _buildChildNodes.size();
	size_t newChildCount = currentChildCount + childNodeCount;
	_buildChildNodes.resize(newChildCount);
	for (size_t i = currentChildCount; i < newChildCount; ++i)
	{
		// Ensure the specified node is in a valid state to be added as a child node
		auto* childNodeResolved = KnownDynamicCast<Direct3DRenderableNode*>((childNodes++)->get());
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
	// Ensure the specified child node ID is valid
	if (childNodeID >= (uint32_t)_buildChildNodes.size())
	{
		_log->Error("Tried to remove renderable child node with ID {0}, but only {1} child nodes are present.", childNodeID, (uint32_t)_buildChildNodes.size());
		return false;
	}

	// Ensure the specified child node is in the slot it claims to be assigned to
	Direct3DRenderableNode* actualChildNodeInSlot = _buildChildNodes[childNodeID];
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
void Direct3DStateGroupNode::SetChildNodes(IRenderableNode* const* childNodes, size_t childNodeCount)
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
		auto* childNodeResolved = KnownDynamicCast<Direct3DRenderableNode*>(*(childNodes++));
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
void Direct3DStateGroupNode::SetChildNodes(IRenderableNode::unique_ptr const* childNodes, size_t childNodeCount)
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
		auto* childNodeResolved = KnownDynamicCast<Direct3DRenderableNode*>((childNodes++)->get());
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
const std::vector<Direct3DRenderableNode*>& Direct3DStateGroupNode::GetChildNodes() const
{
	return _drawChildNodes;
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
	_buildState.nativeObjectsCurrent = false;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void Direct3DStateGroupNode::SetDepthWriteEnabled(bool state)
{
	_buildState.depthWriteEnabled = state;
	_buildState.nativeObjectsCurrent = false;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void Direct3DStateGroupNode::SetDepthComparisonFunction(DepthComparisonFunction comparisonTest)
{
	_buildState.depthComparisonTest = comparisonTest;
	_buildState.nativeObjectsCurrent = false;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void Direct3DStateGroupNode::SetDepthBias(float constantFactor, float slopeFactor, float clamp)
{
	_buildState.depthBiasEnabled = true;
	_buildState.depthBiasConstantFactor = constantFactor;
	_buildState.depthBiasSlopeFactor = slopeFactor;
	_buildState.depthBiasClamp = clamp;
	_buildState.nativeObjectsCurrent = false;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void Direct3DStateGroupNode::ClearDepthBias()
{
	_buildState.depthBiasEnabled = false;
	_buildState.nativeObjectsCurrent = false;
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
	_buildState.nativeObjectsCurrent = false;
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
	_buildState.nativeObjectsCurrent = false;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void Direct3DStateGroupNode::SetStencilReferenceValue(uint32_t referenceValue)
{
	_buildState.stencilReferenceValue = referenceValue;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
// Rasterization state methods
//----------------------------------------------------------------------------------------
void Direct3DStateGroupNode::SetPolygonFillMode(PolygonFillMode fillMode)
{
	_buildState.polygonFillMode = fillMode;
	_buildState.nativeObjectsCurrent = false;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void Direct3DStateGroupNode::SetPolygonCullMode(PolygonCullMode cullMode)
{
	_buildState.polygonCullMode = cullMode;
	_buildState.nativeObjectsCurrent = false;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void Direct3DStateGroupNode::SetPolygonWindingOrder(PolygonWindingOrder windingOrder)
{
	_buildState.polygonWindingOrder = windingOrder;
	_buildState.nativeObjectsCurrent = false;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
// Blend state methods
//----------------------------------------------------------------------------------------
void Direct3DStateGroupNode::SetBlendEnabled(bool state)
{
	_buildState.blendEnabled = state;
	_buildState.nativeObjectsCurrent = false;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
void Direct3DStateGroupNode::SetBlendMode(BlendOperation blendOperationRGB, BlendFactor blendFactorSourceRGB, BlendFactor blendFactorDestinationRGB, BlendOperation blendOperationA, BlendFactor blendFactorSourceA, BlendFactor blendFactorDestinationA)
{
	BlendState blendState = BuildBlendState(blendOperationRGB, blendFactorSourceRGB, blendFactorDestinationRGB, blendOperationA, blendFactorSourceA, blendFactorDestinationA);
	_buildState.sharedBlendState = blendState;
	_buildState.nativeObjectsCurrent = false;
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
	_buildState.nativeObjectsCurrent = false;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
void Direct3DStateGroupNode::MigrateBuildStateToDrawState()
{
	// Migrate our build state
	if (!IsDrawStateCurrent())
	{
		// Transfer basic build state
		Direct3DStateContainer::MigrateBuildStateToDrawState();
		bool nativeObjectsCurrent = _drawState.nativeObjectsCurrent;
		_drawState = _buildState;
		_buildState.nativeObjectsCurrent = nativeObjectsCurrent;

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
				size_t nextWriteIndex = 0;
				size_t nextReadIndex = 0;
				auto removedChildNodeCount = _removedChildNodeIDs.size();
				size_t newChildNodeCount = _buildChildNodes.size() - removedChildNodeCount;
				_drawChildNodes.resize(newChildNodeCount);
				size_t removedChildNodeSearchIndex = 0;
				while (removedChildNodeSearchIndex < removedChildNodeCount)
				{
					size_t removedIndex = _removedChildNodeIDs[removedChildNodeSearchIndex++];
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
				auto newChildNodeCount = _buildChildNodes.size();
				_drawChildNodes.resize(newChildNodeCount);
				std::copy(_buildChildNodes.data(), _buildChildNodes.data() + newChildNodeCount, _drawChildNodes.data());
			}
			_hasModifiedChildNodes = false;
		}

		// Flag that our draw state is now current
		FlagDrawStateCurrent();
	}

	// Migrate build state in our child nodes
	for (Direct3DRenderableNode* entry : _drawChildNodes)
	{
		entry->MigrateBuildStateToDrawState();
	}
}

//----------------------------------------------------------------------------------------
void Direct3DStateGroupNode::ApplyFixedState(ID3D11Device1* device, ID3D11DeviceContext1* context, Direct3DFrameBuffer* framebuffer)
{
	// If this is a compute node, there's nothing to do here, so abort any further processing.
	if (_drawState.computeTaskDefined)
	{
		return;
	}

	// Update our native state objects if required
	if (!_drawState.nativeObjectsCurrent)
	{
		if (!CreateNativeRasterizerStateObject(device, _nativeRasterizerState, false))
		{
			_log->Error("Failed to create rasterizer state object");
		}
		if (!CreateNativeBlendStateObject(device, _nativeBlendState))
		{
			_log->Error("Failed to create blend state object");
		}
		if (!CreateNativeDepthStencilStateObject(device, _nativeDepthStencilState))
		{
			_log->Error("Failed to create depth stencil state object");
		}
		_drawState.nativeObjectsCurrent = true;
		_nativeRasterizerStateAntialiasingCreated = false;
	}

	// Create our multisampling rasterizer state object if required
	bool multiSamplingEnabled = (framebuffer->GetSampleCount() != ITextureBuffer::SampleCount::SampleCount1);
	if (multiSamplingEnabled && !_nativeRasterizerStateAntialiasingCreated)
	{
		if (!CreateNativeRasterizerStateObject(device, _nativeRasterizerStateAntialiasing, true))
		{
			_log->Error("Failed to create rasterizer state object");
		}
		_nativeRasterizerStateAntialiasingCreated = true;
	}

	// Bind the current rasterizer state
	context->RSSetState(multiSamplingEnabled ? _nativeRasterizerStateAntialiasing.Get() : _nativeRasterizerState.Get());

	// Bind the current blend state
	context->OMSetBlendState(_nativeBlendState.Get(), nullptr, 0xFFFFFFFF);

	// Bind the current depth and stencil state
	context->OMSetDepthStencilState(_nativeDepthStencilState.Get(), _drawState.stencilReferenceValue);
}

//----------------------------------------------------------------------------------------
bool Direct3DStateGroupNode::CreateNativeRasterizerStateObject(ID3D11Device1* device, ComPtr<ID3D11RasterizerState>& rasterizerState, bool multisamplingEnabled) const
{
	D3D11_FILL_MODE fillModeNative;
	switch (_drawState.polygonFillMode)
	{
	default:
	case PolygonFillMode::Solid:
		fillModeNative = D3D11_FILL_SOLID;
		break;
	case PolygonFillMode::Wireframe:
		fillModeNative = D3D11_FILL_WIREFRAME;
		break;
	}

	D3D11_CULL_MODE cullModeNative;
	switch (_drawState.polygonCullMode)
	{
	default:
	case PolygonCullMode::None:
		cullModeNative = D3D11_CULL_NONE;
		break;
	case PolygonCullMode::Front:
		cullModeNative = D3D11_CULL_FRONT;
		break;
	case PolygonCullMode::Back:
		cullModeNative = D3D11_CULL_BACK;
		break;
	}

	// Setup rasterizer state
	D3D11_RASTERIZER_DESC rasterizerDesc = {};
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
	rasterizerDesc.ScissorEnable = TRUE;

	// Although the documentation is confusing, these settings affect the line drawing algorithm used regardless of
	// whether a multisampled framebuffer is bound or not. As a result, we need to configure this setting differently
	// depending on whether multisampling is currently active or not.
	if (multisamplingEnabled)
	{
		rasterizerDesc.MultisampleEnable = TRUE;
		rasterizerDesc.AntialiasedLineEnable = FALSE;
	}
	else
	{
		rasterizerDesc.MultisampleEnable = FALSE;
		rasterizerDesc.AntialiasedLineEnable = FALSE;
	}

	// Create the rasterizer state object
	HRESULT createRasterizerStateReturn = device->CreateRasterizerState(&rasterizerDesc, &rasterizerState);
	if (FAILED(createRasterizerStateReturn))
	{
		_log->Error("CreateRasterizerState failed with error code {0}", createRasterizerStateReturn);
		return false;
	}
	return true;
}

//----------------------------------------------------------------------------------------
bool Direct3DStateGroupNode::CreateNativeBlendStateObject(ID3D11Device1* device, ComPtr<ID3D11BlendState>& blendState) const
{
	if (_drawState.blendEnabled)
	{
		D3D11_BLEND_DESC desc = {};
		D3D11_RENDER_TARGET_BLEND_DESC blendDesc = {};
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
		blendDesc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		desc.RenderTarget[0] = blendDesc;
		desc.AlphaToCoverageEnable = FALSE;
		desc.IndependentBlendEnable = FALSE;

		for (const auto& targetBlendState : _drawState.attachmentTypeBlendState)
		{
			desc.IndependentBlendEnable = TRUE;
			D3D11_RENDER_TARGET_BLEND_DESC blendDescTemp = {};

			blendDescTemp.BlendEnable = TRUE;
			blendDescTemp.SrcBlend = targetBlendState.nativeBlendFactorSourceRGB;
			blendDescTemp.DestBlend = targetBlendState.nativeBlendFactorDestinationRGB;
			blendDescTemp.BlendOp = targetBlendState.nativeBlendOperationRGB;

			blendDescTemp.SrcBlendAlpha = GetNativeAlphaBlendFactor(targetBlendState.blendFactorSourceA, targetBlendState.blendOperationA);
			blendDescTemp.DestBlendAlpha = GetNativeAlphaBlendFactor(targetBlendState.blendFactorDestinationA, targetBlendState.blendOperationA);
			blendDescTemp.BlendOpAlpha = targetBlendState.nativeBlendOperationA;
			blendDescTemp.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

			desc.RenderTarget[targetBlendState.index] = blendDescTemp;
		}
		HRESULT createBlendStateReturn = device->CreateBlendState(&desc, &blendState);
		if (FAILED(createBlendStateReturn))
		{
			_log->Error("CreateBlendState failed with error code {0}", createBlendStateReturn);
			return false;
		}
	}
	else
	{
		D3D11_BLEND_DESC desc = {};
		D3D11_RENDER_TARGET_BLEND_DESC blendDesc = {};
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
		blendDesc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		desc.RenderTarget[0] = blendDesc;
		desc.AlphaToCoverageEnable = FALSE;
		desc.IndependentBlendEnable = FALSE;

		HRESULT createBlendStateReturn = device->CreateBlendState(&desc, &blendState);
		if (FAILED(createBlendStateReturn))
		{
			_log->Error("CreateBlendState failed with error code {0}", createBlendStateReturn);
			return false;
		}
	}

	return true;
}

//----------------------------------------------------------------------------------------
bool Direct3DStateGroupNode::CreateNativeDepthStencilStateObject(ID3D11Device1* device, ComPtr<ID3D11DepthStencilState>& depthStencilState) const
{
	// Configure depth test settings based on the supplied state
	D3D11_DEPTH_STENCIL_DESC depthStencilStateDesc = {};
	depthStencilStateDesc.DepthEnable = (_drawState.depthTestEnabled ? TRUE : FALSE);
	if (_drawState.depthWriteEnabled)
	{
		depthStencilStateDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	}
	else
	{
		depthStencilStateDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	}
	depthStencilStateDesc.DepthFunc = GetNativeDepthComparisonFunction(_drawState.depthComparisonTest);

	// Configure stencil test settings based on the supplied state
	if (_drawState.stencilEnabled)
	{
		depthStencilStateDesc.StencilEnable = TRUE;
		depthStencilStateDesc.StencilReadMask = UINT8(_drawState.stencilCompareMask);
		depthStencilStateDesc.StencilWriteMask = UINT8(_drawState.stencilWriteMask);
		depthStencilStateDesc.FrontFace.StencilFailOp = GetNativeStencilOperation(_drawState.stencilFrontFace.failOperation);
		depthStencilStateDesc.FrontFace.StencilDepthFailOp = GetNativeStencilOperation(_drawState.stencilFrontFace.depthFailOperation);
		depthStencilStateDesc.FrontFace.StencilPassOp = GetNativeStencilOperation(_drawState.stencilFrontFace.passOperation);
		depthStencilStateDesc.FrontFace.StencilFunc = GetNativeStencilComparisonFunction(_drawState.stencilFrontFace.comparisonTest);
		depthStencilStateDesc.BackFace.StencilFailOp = GetNativeStencilOperation(_drawState.stencilBackFace.failOperation);
		depthStencilStateDesc.BackFace.StencilDepthFailOp = GetNativeStencilOperation(_drawState.stencilBackFace.depthFailOperation);
		depthStencilStateDesc.BackFace.StencilPassOp = GetNativeStencilOperation(_drawState.stencilBackFace.passOperation);
		depthStencilStateDesc.BackFace.StencilFunc = GetNativeStencilComparisonFunction(_drawState.stencilBackFace.comparisonTest);
	}
	else
	{
		depthStencilStateDesc.StencilEnable = FALSE;
	}

	// Attempt to create the depth/stencil state object
	HRESULT createDepthStencilStateReturn = device->CreateDepthStencilState(&depthStencilStateDesc, &depthStencilState);
	if (FAILED(createDepthStencilStateReturn))
	{
		_log->Error("CreateDepthStencilState failed with error code {0}", createDepthStencilStateReturn);
		return false;
	}
	return true;
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
constexpr D3D11_BLEND_OP Direct3DStateGroupNode::GetNativeBlendOperation(BlendOperation operation)
{
	switch (operation)
	{
	case IStateGroupNode::BlendOperation::Add:
		return D3D11_BLEND_OP::D3D11_BLEND_OP_ADD;
	case IStateGroupNode::BlendOperation::Subtract:
		return D3D11_BLEND_OP::D3D11_BLEND_OP_SUBTRACT;
	case IStateGroupNode::BlendOperation::ReverseSubtract:
		return D3D11_BLEND_OP::D3D11_BLEND_OP_REV_SUBTRACT;
	case IStateGroupNode::BlendOperation::Min:
		return D3D11_BLEND_OP::D3D11_BLEND_OP_MIN;
	case IStateGroupNode::BlendOperation::Max:
		return D3D11_BLEND_OP::D3D11_BLEND_OP_MAX;
	}
	return D3D11_BLEND_OP::D3D11_BLEND_OP_ADD;
}

//----------------------------------------------------------------------------------------
constexpr D3D11_BLEND Direct3DStateGroupNode::GetNativeBlendFactor(BlendFactor factor, BlendOperation operation)
{
	// If the blend operation is Min or Max, we need to set our blend factors to 1, otherwise we trigger debug warnings.
	if ((operation == BlendOperation::Min) || (operation == BlendOperation::Max))
	{
		return D3D11_BLEND::D3D11_BLEND_ONE;
	}

	// Map to the native blend factor value
	switch (factor)
	{
	case IStateGroupNode::BlendFactor::Zero:
		return D3D11_BLEND::D3D11_BLEND_ZERO;
	case IStateGroupNode::BlendFactor::One:
		return D3D11_BLEND::D3D11_BLEND_ONE;
	case IStateGroupNode::BlendFactor::SourceColor:
		return D3D11_BLEND::D3D11_BLEND_SRC_COLOR;
	case IStateGroupNode::BlendFactor::OneMinusSourceColor:
		return D3D11_BLEND::D3D11_BLEND_INV_SRC_COLOR;
	case IStateGroupNode::BlendFactor::DestinationColor:
		return D3D11_BLEND::D3D11_BLEND_DEST_COLOR;
	case IStateGroupNode::BlendFactor::OneMinusDestinationColor:
		return D3D11_BLEND::D3D11_BLEND_INV_DEST_COLOR;
	case IStateGroupNode::BlendFactor::SourceAlpha:
		return D3D11_BLEND::D3D11_BLEND_SRC_ALPHA;
	case IStateGroupNode::BlendFactor::OneMinusSourceAlpha:
		return D3D11_BLEND::D3D11_BLEND_INV_SRC_ALPHA;
	case IStateGroupNode::BlendFactor::DestinationAlpha:
		return D3D11_BLEND::D3D11_BLEND_DEST_ALPHA;
	case IStateGroupNode::BlendFactor::OneMinusDestinationAlpha:
		return D3D11_BLEND::D3D11_BLEND_INV_DEST_ALPHA;
	}
	return D3D11_BLEND::D3D11_BLEND_ZERO;
}

//----------------------------------------------------------------------------------------
constexpr D3D11_BLEND Direct3DStateGroupNode::GetNativeAlphaBlendFactor(BlendFactor factor, BlendOperation operation)
{
	switch (factor)
	{
	case IStateGroupNode::BlendFactor::SourceColor:
		return D3D11_BLEND::D3D11_BLEND_SRC_ALPHA;
	case IStateGroupNode::BlendFactor::OneMinusSourceColor:
		return D3D11_BLEND::D3D11_BLEND_INV_SRC_ALPHA;
	case IStateGroupNode::BlendFactor::DestinationColor:
		return D3D11_BLEND::D3D11_BLEND_DEST_ALPHA;
	case IStateGroupNode::BlendFactor::OneMinusDestinationColor:
		return D3D11_BLEND::D3D11_BLEND_INV_DEST_ALPHA;
	}
	return GetNativeBlendFactor(factor, operation);
}

//----------------------------------------------------------------------------------------
constexpr D3D11_COMPARISON_FUNC Direct3DStateGroupNode::GetNativeDepthComparisonFunction(DepthComparisonFunction function)
{
	switch (function)
	{
	case DepthComparisonFunction::Never:
		return D3D11_COMPARISON_NEVER;
	case DepthComparisonFunction::Equal:
		return D3D11_COMPARISON_EQUAL;
	case DepthComparisonFunction::NotEqual:
		return D3D11_COMPARISON_NOT_EQUAL;
	case DepthComparisonFunction::Less:
		return D3D11_COMPARISON_LESS;
	case DepthComparisonFunction::LessOrEqual:
		return D3D11_COMPARISON_LESS_EQUAL;
	case DepthComparisonFunction::Greater:
		return D3D11_COMPARISON_GREATER;
	case DepthComparisonFunction::GreaterOrEqual:
		return D3D11_COMPARISON_GREATER_EQUAL;
	case DepthComparisonFunction::Always:
		return D3D11_COMPARISON_ALWAYS;
	}
	return D3D11_COMPARISON_NEVER;
}

//----------------------------------------------------------------------------------------
constexpr D3D11_COMPARISON_FUNC Direct3DStateGroupNode::GetNativeStencilComparisonFunction(StencilComparisonFunction function)
{
	switch (function)
	{
	case StencilComparisonFunction::Never:
		return D3D11_COMPARISON_NEVER;
	case StencilComparisonFunction::Equal:
		return D3D11_COMPARISON_EQUAL;
	case StencilComparisonFunction::NotEqual:
		return D3D11_COMPARISON_NOT_EQUAL;
	case StencilComparisonFunction::Less:
		return D3D11_COMPARISON_LESS;
	case StencilComparisonFunction::LessOrEqual:
		return D3D11_COMPARISON_LESS_EQUAL;
	case StencilComparisonFunction::Greater:
		return D3D11_COMPARISON_GREATER;
	case StencilComparisonFunction::GreaterOrEqual:
		return D3D11_COMPARISON_GREATER_EQUAL;
	case StencilComparisonFunction::Always:
		return D3D11_COMPARISON_ALWAYS;
	}
	return D3D11_COMPARISON_NEVER;
}

//----------------------------------------------------------------------------------------
constexpr D3D11_STENCIL_OP Direct3DStateGroupNode::GetNativeStencilOperation(StencilOperation operation)
{
	switch (operation)
	{
	case StencilOperation::Keep:
		return D3D11_STENCIL_OP_KEEP;
	case StencilOperation::Zero:
		return D3D11_STENCIL_OP_ZERO;
	case StencilOperation::Replace:
		return D3D11_STENCIL_OP_REPLACE;
	case StencilOperation::IncrementAndClamp:
		return D3D11_STENCIL_OP_INCR_SAT;
	case StencilOperation::DecrementAndClamp:
		return D3D11_STENCIL_OP_DECR_SAT;
	case StencilOperation::IncrementAndWrap:
		return D3D11_STENCIL_OP_INCR;
	case StencilOperation::DecrementAndWrap:
		return D3D11_STENCIL_OP_DECR;
	case StencilOperation::Invert:
		return D3D11_STENCIL_OP_INVERT;
	}
	return D3D11_STENCIL_OP_KEEP;
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
