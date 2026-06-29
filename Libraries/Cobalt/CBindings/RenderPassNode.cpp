// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "RenderPassNode.h"
#include <Cobalt/RendererInterface/RendererInterface.pkg>

using namespace cobalt::graphics;

//----------------------------------------------------------------------------------------
// Child node methods
//----------------------------------------------------------------------------------------
void Cobalt_RenderPassNode_AddChildNode(Cobalt_RenderPassNode renderPassNode, Cobalt_ProgramNode childNode, Cobalt_DefaultState defaultState)
{
	auto _this = reinterpret_cast<IRenderPassNode*>(renderPassNode);
	auto _node = reinterpret_cast<IProgramNode*>(childNode);
	auto _state = reinterpret_cast<IDefaultState*>(defaultState);

	_this->AddChildNode(_node, _state);
}

//----------------------------------------------------------------------------------------
void Cobalt_RenderPassNode_AddChildNodes(Cobalt_RenderPassNode renderPassNode, const Cobalt_ProgramNode* childNodes, size_t childNodeCount, const Cobalt_DefaultState* defaultState)
{
	auto _this = reinterpret_cast<IRenderPassNode*>(renderPassNode);
	auto _nodes = reinterpret_cast<IProgramNode* const*>(childNodes);
	auto _states = reinterpret_cast<IDefaultState* const*>(defaultState);

	_this->AddChildNodes(_nodes, childNodeCount, _states);
}

//----------------------------------------------------------------------------------------
void Cobalt_RenderPassNode_RemoveChildNode(Cobalt_RenderPassNode renderPassNode, Cobalt_ProgramNode childNode)
{
	auto _this = reinterpret_cast<IRenderPassNode*>(renderPassNode);
	auto _node = reinterpret_cast<IProgramNode*>(childNode);

	_this->RemoveChildNode(_node);
}

//----------------------------------------------------------------------------------------
void Cobalt_RenderPassNode_RemoveChildNodes(Cobalt_RenderPassNode renderPassNode, const Cobalt_ProgramNode* childNodes, size_t childNodeCount)
{
	auto _this = reinterpret_cast<IRenderPassNode*>(renderPassNode);
	auto _nodes = reinterpret_cast<IProgramNode* const*>(childNodes);

	_this->RemoveChildNodes(_nodes, childNodeCount);
}

//----------------------------------------------------------------------------------------
void Cobalt_RenderPassNode_RemoveAllChildNodes(Cobalt_RenderPassNode renderPassNode)
{
	auto _this = reinterpret_cast<IRenderPassNode*>(renderPassNode);

	_this->RemoveAllChildNodes();
}

//----------------------------------------------------------------------------------------
void Cobalt_RenderPassNode_SetChildNodes(Cobalt_RenderPassNode renderPassNode, const Cobalt_ProgramNode* childNodes, size_t childNodeCount, const Cobalt_DefaultState* defaultState)
{
	auto _this = reinterpret_cast<IRenderPassNode*>(renderPassNode);
	auto _nodes = reinterpret_cast<IProgramNode* const*>(childNodes);
	auto _states = reinterpret_cast<IDefaultState* const*>(defaultState);

	_this->SetChildNodes(_nodes, childNodeCount, _states);
}

//----------------------------------------------------------------------------------------
// Framebuffer methods
//----------------------------------------------------------------------------------------
void Cobalt_RenderPassNode_BindFrameBuffer(Cobalt_RenderPassNode renderPassNode, Cobalt_FrameBuffer frameBuffer)
{
	auto _this = reinterpret_cast<IRenderPassNode*>(renderPassNode);
	auto _frameBuffer = reinterpret_cast<IFrameBuffer*>(frameBuffer);

	_this->BindFrameBuffer(_frameBuffer);
}

//----------------------------------------------------------------------------------------
void Cobalt_RenderPassNode_SetAttachmentLoadStoreBehavior(Cobalt_RenderPassNode renderPassNode, Cobalt_AttachmentType type, size_t index, Cobalt_AttachmentLoadBehavior loadBehavior, Cobalt_AttachmentStoreBehavior storeBehavior)
{
	auto _this = reinterpret_cast<IRenderPassNode*>(renderPassNode);
	auto _type = (IFrameBuffer::AttachmentType)type;
	auto _loadBehavior = (IRenderPassNode::AttachmentLoadBehavior)loadBehavior;
	auto _storeBehavior = (IRenderPassNode::AttachmentStoreBehavior)storeBehavior;

	_this->SetAttachmentLoadStoreBehavior(_type, index, _loadBehavior, _storeBehavior);
}

//----------------------------------------------------------------------------------------
void Cobalt_RenderPassNode_SetAttachmentClearDataF(Cobalt_RenderPassNode renderPassNode, Cobalt_AttachmentType type, size_t index, const float data[4])
{
	auto _this = reinterpret_cast<IRenderPassNode*>(renderPassNode);
	auto _type = (IFrameBuffer::AttachmentType)type;

	_this->SetAttachmentClearData(_type, index, V4Float32(data[0], data[1], data[2], data[3]));
}

//----------------------------------------------------------------------------------------
void Cobalt_RenderPassNode_SetAttachmentClearDataI(Cobalt_RenderPassNode renderPassNode, Cobalt_AttachmentType type, size_t index, const int32_t data[4])
{
	auto _this = reinterpret_cast<IRenderPassNode*>(renderPassNode);
	auto _type = (IFrameBuffer::AttachmentType)type;

	_this->SetAttachmentClearData(_type, index, V4Int32(data[0], data[1], data[2], data[3]));
}

//----------------------------------------------------------------------------------------
void Cobalt_RenderPassNode_SetAttachmentClearDataU(Cobalt_RenderPassNode renderPassNode, Cobalt_AttachmentType type, size_t index, const uint32_t data[4])
{
	auto _this = reinterpret_cast<IRenderPassNode*>(renderPassNode);
	auto _type = (IFrameBuffer::AttachmentType)type;

	_this->SetAttachmentClearData(_type, index, V4UInt32(data[0], data[1], data[2], data[3]));
}

//----------------------------------------------------------------------------------------
void Cobalt_RenderPassNode_RemoveAttachmentClearData(Cobalt_RenderPassNode renderPassNode, Cobalt_AttachmentType type, size_t index)
{
	auto _this = reinterpret_cast<IRenderPassNode*>(renderPassNode);
	auto _type = (IFrameBuffer::AttachmentType)type;

	_this->RemoveAttachmentClearData(_type, index);
}

//----------------------------------------------------------------------------------------
void Cobalt_RenderPassNode_EnableAttachmentMultiSamplingResolution(Cobalt_RenderPassNode renderPassNode, Cobalt_AttachmentType type, size_t index, size_t resolveAttachmentIndex)
{
	auto _this = reinterpret_cast<IRenderPassNode*>(renderPassNode);
	auto _type = (IFrameBuffer::AttachmentType)type;

	_this->EnableAttachmentMultiSamplingResolution(_type, index, resolveAttachmentIndex);
}

//----------------------------------------------------------------------------------------
void Cobalt_RenderPassNode_DisableAttachmentMultiSamplingResolution(Cobalt_RenderPassNode renderPassNode, Cobalt_AttachmentType type, size_t index)
{
	auto _this = reinterpret_cast<IRenderPassNode*>(renderPassNode);
	auto _type = (IFrameBuffer::AttachmentType)type;

	_this->DisableAttachmentMultiSamplingResolution(_type, index);
}

//----------------------------------------------------------------------------------------
// Enabled state methods
//----------------------------------------------------------------------------------------
void Cobalt_RenderPassNode_SetIsEnabled(Cobalt_RenderPassNode renderPassNode, char state)
{
	auto _this = reinterpret_cast<IRenderPassNode*>(renderPassNode);

	_this->SetIsEnabled(state != 0);
}

//----------------------------------------------------------------------------------------
// Debug methods
//----------------------------------------------------------------------------------------
void Cobalt_RenderPassNode_SetDebugName(Cobalt_RenderPassNode renderPassNode, const char* name, size_t nameLength)
{
	auto _this = reinterpret_cast<IRenderPassNode*>(renderPassNode);
	auto _name = std::string(name, nameLength);

	_this->SetDebugName(_name);
}

//----------------------------------------------------------------------------------------
void Cobalt_RenderPassNode_Delete(Cobalt_RenderPassNode renderPassNode)
{
	auto _this = reinterpret_cast<IRenderPassNode*>(renderPassNode);

	_this->Delete();
}
