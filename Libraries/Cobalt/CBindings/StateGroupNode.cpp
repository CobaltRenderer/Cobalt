// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "StateGroupNode.h"
#include <Cobalt/RendererInterface/RendererInterface.pkg>

using namespace cobalt::graphics;

//----------------------------------------------------------------------------------------
// Child node methods
//----------------------------------------------------------------------------------------
void Cobalt_StateGroupNode_AddChildNode(Cobalt_StateGroupNode stateGroupNode, Cobalt_RenderableNode childNode)
{
	auto _this = reinterpret_cast<IStateGroupNode*>(stateGroupNode);
	auto _node = reinterpret_cast<IRenderableNode*>(childNode);

	_this->AddChildNode(_node);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateGroupNode_AddChildNodes(Cobalt_StateGroupNode stateGroupNode, const Cobalt_RenderableNode* childNodes, size_t childNodeCount)
{
	auto _this = reinterpret_cast<IStateGroupNode*>(stateGroupNode);
	auto _nodes = reinterpret_cast<IRenderableNode* const*>(childNodes);

	_this->AddChildNodes(_nodes, childNodeCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateGroupNode_RemoveChildNode(Cobalt_StateGroupNode stateGroupNode, Cobalt_RenderableNode childNode)
{
	auto _this = reinterpret_cast<IStateGroupNode*>(stateGroupNode);
	auto _node = reinterpret_cast<IRenderableNode*>(childNode);

	_this->RemoveChildNode(_node);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateGroupNode_RemoveChildNodes(Cobalt_StateGroupNode stateGroupNode, const Cobalt_RenderableNode* childNodes, size_t childNodeCount)
{
	auto _this = reinterpret_cast<IStateGroupNode*>(stateGroupNode);
	auto _nodes = reinterpret_cast<IRenderableNode* const*>(childNodes);

	_this->RemoveChildNodes(_nodes, childNodeCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateGroupNode_RemoveAllChildNodes(Cobalt_StateGroupNode stateGroupNode)
{
	auto _this = reinterpret_cast<IStateGroupNode*>(stateGroupNode);

	_this->RemoveAllChildNodes();
}

//----------------------------------------------------------------------------------------
void Cobalt_StateGroupNode_SetChildNodes(Cobalt_StateGroupNode stateGroupNode, const Cobalt_RenderableNode* childNodes, size_t childNodeCount)
{
	auto _this = reinterpret_cast<IStateGroupNode*>(stateGroupNode);
	auto _nodes = reinterpret_cast<IRenderableNode* const*>(childNodes);

	_this->SetChildNodes(_nodes, childNodeCount);
}

//----------------------------------------------------------------------------------------
// Compute methods
//----------------------------------------------------------------------------------------
void Cobalt_StateGroupNode_SetComputeTask(Cobalt_StateGroupNode stateGroupNode, const uint32_t threadGroupCounts[3])
{
	auto _this = reinterpret_cast<IStateGroupNode*>(stateGroupNode);

	_this->SetComputeTask(V3UInt32(threadGroupCounts[0], threadGroupCounts[1], threadGroupCounts[2]));
}

//----------------------------------------------------------------------------------------
void Cobalt_StateGroupNode_RemoveComputeTask(Cobalt_StateGroupNode stateGroupNode)
{
	auto _this = reinterpret_cast<IStateGroupNode*>(stateGroupNode);

	_this->RemoveComputeTask();
}

//----------------------------------------------------------------------------------------
// Depth state methods
//----------------------------------------------------------------------------------------
void Cobalt_StateGroupNode_SetDepthTestEnabled(Cobalt_StateGroupNode stateGroupNode, char state)
{
	auto _this = reinterpret_cast<IStateGroupNode*>(stateGroupNode);

	_this->SetDepthTestEnabled(state != 0);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateGroupNode_SetDepthWriteEnabled(Cobalt_StateGroupNode stateGroupNode, char state)
{
	auto _this = reinterpret_cast<IStateGroupNode*>(stateGroupNode);

	_this->SetDepthWriteEnabled(state != 0);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateGroupNode_SetDepthComparisonFunction(Cobalt_StateGroupNode stateGroupNode, Cobalt_DepthComparisonFunction comparisonTest)
{
	auto _this = reinterpret_cast<IStateGroupNode*>(stateGroupNode);

	_this->SetDepthComparisonFunction((IStateGroupNode::DepthComparisonFunction)comparisonTest);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateGroupNode_SetDepthBias(Cobalt_StateGroupNode stateGroupNode, float constantFactor, float slopeFactor, float clamp)
{
	auto _this = reinterpret_cast<IStateGroupNode*>(stateGroupNode);

	_this->SetDepthBias(constantFactor, slopeFactor, clamp);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateGroupNode_ClearDepthBias(Cobalt_StateGroupNode stateGroupNode)
{
	auto _this = reinterpret_cast<IStateGroupNode*>(stateGroupNode);

	_this->ClearDepthBias();
}

//----------------------------------------------------------------------------------------
// Stencil state methods
//----------------------------------------------------------------------------------------
void Cobalt_StateGroupNode_SetStencilTestEnabled(Cobalt_StateGroupNode stateGroupNode, char state, uint32_t compareMask, uint32_t writeMask)
{
	auto _this = reinterpret_cast<IStateGroupNode*>(stateGroupNode);

	_this->SetStencilTestEnabled(state != 0, compareMask, writeMask);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateGroupNode_SetStencilOperation(Cobalt_StateGroupNode stateGroupNode, Cobalt_StencilTargetFace targetFace, Cobalt_StencilComparisonFunction comparisonTest, Cobalt_StencilOperation passOperation, Cobalt_StencilOperation failOperation, Cobalt_StencilOperation depthFailOperation)
{
	auto _this = reinterpret_cast<IStateGroupNode*>(stateGroupNode);

	_this->SetStencilOperation(
	  (IStateGroupNode::StencilTargetFace)targetFace,
	  (IStateGroupNode::StencilComparisonFunction)comparisonTest,
	  (IStateGroupNode::StencilOperation)passOperation,
	  (IStateGroupNode::StencilOperation)failOperation,
	  (IStateGroupNode::StencilOperation)depthFailOperation);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateGroupNode_SetStencilReferenceValue(Cobalt_StateGroupNode stateGroupNode, uint32_t referenceValue)
{
	auto _this = reinterpret_cast<IStateGroupNode*>(stateGroupNode);

	_this->SetStencilReferenceValue(referenceValue);
}

//----------------------------------------------------------------------------------------
// Rasterization state methods
//----------------------------------------------------------------------------------------
void Cobalt_StateGroupNode_SetPolygonFillMode(Cobalt_StateGroupNode stateGroupNode, Cobalt_PolygonFillMode fillMode)
{
	auto _this = reinterpret_cast<IStateGroupNode*>(stateGroupNode);

	_this->SetPolygonFillMode((IStateGroupNode::PolygonFillMode)fillMode);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateGroupNode_SetPolygonCullMode(Cobalt_StateGroupNode stateGroupNode, Cobalt_PolygonCullMode cullMode)
{
	auto _this = reinterpret_cast<IStateGroupNode*>(stateGroupNode);

	_this->SetPolygonCullMode((IStateGroupNode::PolygonCullMode)cullMode);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateGroupNode_SetPolygonWindingOrder(Cobalt_StateGroupNode stateGroupNode, Cobalt_PolygonWindingOrder windingOrder)
{
	auto _this = reinterpret_cast<IStateGroupNode*>(stateGroupNode);

	_this->SetPolygonWindingOrder((IStateGroupNode::PolygonWindingOrder)windingOrder);
}

//----------------------------------------------------------------------------------------
// Blend state methods
//----------------------------------------------------------------------------------------
void Cobalt_StateGroupNode_SetBlendEnabled(Cobalt_StateGroupNode stateGroupNode, char state)
{
	auto _this = reinterpret_cast<IStateGroupNode*>(stateGroupNode);

	_this->SetBlendEnabled(state != 0);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateGroupNode_SetSharedBlendMode(Cobalt_StateGroupNode stateGroupNode, Cobalt_BlendOperation blendOperationRGB, Cobalt_BlendFactor blendFactorSourceRGB, Cobalt_BlendFactor blendFactorDestinationRGB, Cobalt_BlendOperation blendOperationA, Cobalt_BlendFactor blendFactorSourceA, Cobalt_BlendFactor blendFactorDestinationA)
{
	auto _this = reinterpret_cast<IStateGroupNode*>(stateGroupNode);

	_this->SetBlendMode(
	  (IStateGroupNode::BlendOperation)blendOperationRGB,
	  (IStateGroupNode::BlendFactor)blendFactorSourceRGB,
	  (IStateGroupNode::BlendFactor)blendFactorDestinationRGB,
	  (IStateGroupNode::BlendOperation)blendOperationA,
	  (IStateGroupNode::BlendFactor)blendFactorSourceA,
	  (IStateGroupNode::BlendFactor)blendFactorDestinationA);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateGroupNode_SetBlendMode(Cobalt_StateGroupNode stateGroupNode, Cobalt_AttachmentType type, size_t index, Cobalt_BlendOperation blendOperationRGB, Cobalt_BlendFactor blendFactorSourceRGB, Cobalt_BlendFactor blendFactorDestinationRGB, Cobalt_BlendOperation blendOperationA, Cobalt_BlendFactor blendFactorSourceA, Cobalt_BlendFactor blendFactorDestinationA)
{
	auto _this = reinterpret_cast<IStateGroupNode*>(stateGroupNode);

	_this->SetBlendMode(
	  (IFrameBuffer::AttachmentType)type,
	  index,
	  (IStateGroupNode::BlendOperation)blendOperationRGB,
	  (IStateGroupNode::BlendFactor)blendFactorSourceRGB,
	  (IStateGroupNode::BlendFactor)blendFactorDestinationRGB,
	  (IStateGroupNode::BlendOperation)blendOperationA,
	  (IStateGroupNode::BlendFactor)blendFactorSourceA,
	  (IStateGroupNode::BlendFactor)blendFactorDestinationA);
}

//----------------------------------------------------------------------------------------
// Debug methods
//----------------------------------------------------------------------------------------
void Cobalt_StateGroupNode_SetDebugName(Cobalt_StateGroupNode stateGroupNode, const char* name, size_t nameLength)
{
	auto _this = reinterpret_cast<IStateGroupNode*>(stateGroupNode);
	auto _name = std::string(name, nameLength);

	_this->SetDebugName(_name);
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void Cobalt_StateGroupNode_Delete(Cobalt_StateGroupNode stateGroupNode)
{
	auto _this = reinterpret_cast<IStateGroupNode*>(stateGroupNode);

	_this->Delete();
}
