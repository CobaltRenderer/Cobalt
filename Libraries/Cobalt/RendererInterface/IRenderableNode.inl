// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "IIndexAttribute.h"
#include "IVertexAttribute.h"
namespace cobalt { namespace graphics {

//----------------------------------------------------------------------------------------
// Binding methods
//----------------------------------------------------------------------------------------
bool IRenderableNode::GetBoundVertexBuffer(const IVertexAttribute& vertexAttribute, IVertexBuffer*& vertexBuffer, size_t& attributeIndex) const
{
	// Ensure the specified attribute has been bound to a buffer
	if (!vertexAttribute.IsBoundToBuffer())
	{
		return false;
	}

	// Return the bound buffer and attribute index for the specified attribute
	vertexBuffer = vertexAttribute.GetBoundVertexBuffer();
	attributeIndex = vertexAttribute.GetBufferAttributeIndex();
	return true;
}

//----------------------------------------------------------------------------------------
bool IRenderableNode::GetBoundVertexInstanceBuffer(const IVertexAttribute& vertexAttribute, IVertexBuffer*& vertexBuffer, size_t& attributeIndex) const
{
	// Ensure the specified attribute has been bound to a buffer
	if (!vertexAttribute.IsBoundToBuffer())
	{
		return false;
	}

	// Return the bound buffer and attribute index for the specified attribute
	vertexBuffer = vertexAttribute.GetBoundVertexBuffer();
	attributeIndex = vertexAttribute.GetBufferAttributeIndex();
	return true;
}

//----------------------------------------------------------------------------------------
bool IRenderableNode::GetBoundIndexBuffer(const IIndexAttribute& indexAttribute, IIndexBuffer*& indexBuffer, size_t& attributeIndex) const
{
	// Ensure the specified attribute has been bound to a buffer
	if (!indexAttribute.IsBoundToBuffer())
	{
		return false;
	}

	// Return the bound buffer and attribute index for the specified attribute
	indexBuffer = indexAttribute.GetBoundIndexBuffer();
	attributeIndex = indexAttribute.GetBufferAttributeIndex();
	return true;
}

}} // namespace cobalt::graphics
