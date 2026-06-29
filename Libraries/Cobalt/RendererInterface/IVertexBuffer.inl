// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "IVertexAttribute.h"
namespace cobalt { namespace graphics {

//----------------------------------------------------------------------------------------
// Binding methods
//----------------------------------------------------------------------------------------
void IVertexBuffer::AttachVertexAttributeToThisArray(IVertexAttribute& vertexAttribute, size_t attributeIndex)
{
	vertexAttribute.SetVertexBufferInfo(this, attributeIndex);
}

}} // namespace cobalt::graphics
