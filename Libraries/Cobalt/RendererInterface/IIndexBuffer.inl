// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "IIndexAttribute.h"
namespace cobalt { namespace graphics {

//----------------------------------------------------------------------------------------
// Binding methods
//----------------------------------------------------------------------------------------
void IIndexBuffer::AttachIndexAttributeToThisArray(IIndexAttribute& indexAttribute, size_t attributeIndex)
{
	indexAttribute.SetIndexBufferInfo(this, attributeIndex);
}

}} // namespace cobalt::graphics
