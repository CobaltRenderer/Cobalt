// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "RenderableNode.h"
#include <Cobalt/RendererInterface/RendererInterface.pkg>

using namespace cobalt::graphics;

//----------------------------------------------------------------------------------------
// Binding methods
//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_RenderableNode_BindVertexAttribute(Cobalt_RenderableNode renderableNode, Cobalt_VertexAttribute attribute, uint32_t shaderAttributeID)
{
	auto _this = reinterpret_cast<IRenderableNode*>(renderableNode);
	auto _attribute = reinterpret_cast<IVertexAttribute*>(attribute);

	return _this->BindVertexAttribute(*_attribute, (VertexAttributeId)shaderAttributeID) ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_RenderableNode_BindVertexInstanceAttribute(Cobalt_RenderableNode renderableNode, Cobalt_VertexAttribute attribute, uint32_t shaderAttributeID)
{
	auto _this = reinterpret_cast<IRenderableNode*>(renderableNode);
	auto _attribute = reinterpret_cast<IVertexAttribute*>(attribute);

	return _this->BindVertexInstanceAttribute(*_attribute, (VertexAttributeId)shaderAttributeID) ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_RenderableNode_BindIndexAttribute(Cobalt_RenderableNode renderableNode, Cobalt_IndexAttribute attribute)
{
	auto _this = reinterpret_cast<IRenderableNode*>(renderableNode);
	auto _attribute = reinterpret_cast<IIndexAttribute*>(attribute);

	return _this->BindIndexAttribute(*_attribute) ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
// Primitive mode methods
//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_RenderableNode_SetPrimitiveMode(Cobalt_RenderableNode renderableNode, Cobalt_PrimitiveMode primitiveMode, char primitiveRestartEnabled, char adjacencyEnabled)
{
	auto _this = reinterpret_cast<IRenderableNode*>(renderableNode);

	return _this->SetPrimitiveMode((IRenderableNode::PrimitiveMode)primitiveMode, primitiveRestartEnabled != 0, adjacencyEnabled != 0) ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_RenderableNode_SetVertexCount(Cobalt_RenderableNode renderableNode, size_t vertexCount, size_t vertexBufferOffset, size_t indexBufferOffset, ptrdiff_t indexValueOffset)
{
	auto _this = reinterpret_cast<IRenderableNode*>(renderableNode);

	return _this->SetVertexCount(vertexCount, vertexBufferOffset, indexBufferOffset, indexValueOffset) ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_RenderableNode_SetInstanceMode(Cobalt_RenderableNode renderableNode, uint32_t instanceCount, uint32_t instanceOffset)
{
	auto _this = reinterpret_cast<IRenderableNode*>(renderableNode);

	return _this->SetInstanceMode(instanceCount, instanceOffset) ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_RenderableNode_SetIndirectDraw(Cobalt_RenderableNode renderableNode, size_t drawCount, Cobalt_DataArray sourceDataArray, size_t arrayOffsetInBytes, size_t arrayStrideInBytes)
{
	auto _this = reinterpret_cast<IRenderableNode*>(renderableNode);

	return _this->SetIndirectDraw(drawCount, reinterpret_cast<IDataArray*>(sourceDataArray), arrayOffsetInBytes, arrayStrideInBytes) ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_RenderableNode_SetIndirectDrawCounter(Cobalt_RenderableNode renderableNode, size_t maxDrawCount, Cobalt_DataArray drawCountSourceCounter, Cobalt_DataArray sourceDataArray, size_t arrayOffsetInBytes, size_t arrayStrideInBytes)
{
	auto _this = reinterpret_cast<IRenderableNode*>(renderableNode);

	return _this->SetIndirectDraw(maxDrawCount, reinterpret_cast<IDataArray*>(drawCountSourceCounter), reinterpret_cast<IDataArray*>(sourceDataArray), arrayOffsetInBytes, arrayStrideInBytes) ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_RenderableNode_SetIndirectDrawCounterWithOffset(Cobalt_RenderableNode renderableNode, size_t maxDrawCount, Cobalt_DataArray drawCountSourceDataArray, size_t drawCountArrayOffsetInBytes, Cobalt_DataArray sourceDataArray, size_t arrayOffsetInBytes, size_t arrayStrideInBytes)
{
	auto _this = reinterpret_cast<IRenderableNode*>(renderableNode);

	return _this->SetIndirectDraw(maxDrawCount, reinterpret_cast<IDataArray*>(drawCountSourceDataArray), drawCountArrayOffsetInBytes, reinterpret_cast<IDataArray*>(sourceDataArray), arrayOffsetInBytes, arrayStrideInBytes) ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
// Debug methods
//----------------------------------------------------------------------------------------
void Cobalt_RenderableNode_SetDebugName(Cobalt_RenderableNode renderableNode, const char* name, size_t nameLength)
{
	auto _this = reinterpret_cast<IRenderableNode*>(renderableNode);
	auto _name = std::string(name, nameLength);

	_this->SetDebugName(_name);
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void Cobalt_RenderableNode_Delete(Cobalt_RenderableNode renderableNode)
{
	auto _this = reinterpret_cast<IRenderableNode*>(renderableNode);

	_this->Delete();
}
