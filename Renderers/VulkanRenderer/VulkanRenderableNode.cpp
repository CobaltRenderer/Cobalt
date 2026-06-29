// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "VulkanRenderableNode.h"
#include "VulkanDataArray.h"
#include "VulkanHeaders.h"
#include "VulkanRenderer.h"
#include <Internal/RendererSupport/KnownDynamicCast.h>
#include <cstring>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
VulkanRenderableNode::VulkanRenderableNode(cobalt::logging::ILogger* log, VulkanRenderer* renderer)
: VulkanStateContainer(log)
{
	_log = log;
	_renderer = renderer;

	_indexAttributeBound = false;
	_primitiveModeSet = false;
	_vertexCountSet = false;
	_instanceCountSet = false;
	_generatedBufferViews = false;
	_primitiveModeNative = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	_primitiveMode = PrimitiveMode::Triangles;
	_primitiveModeSet = false;
	_primitiveRestartEnabled = false;
	_vertexBindingArrayGenerated = false;

	_immutableStateFrozen = false;
	_buildState.vertexBufferOffset = 0;
	_buildState.indexBufferOffset = 0;
	_buildState.indexValueOffset = 0;
	_buildState.instanceCount = 1;
	_buildState.instanceOffset = 0;
	_buildState.nativeObjectsCurrent = false;
	_drawState.nativeObjectsCurrent = false;
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void VulkanRenderableNode::Delete()
{
	// If API debug logging is enabled, log a warning if this node is still marked as a child node. Note that this may
	// not be a problem. It's possible that the parent node is also being deleted or removed from the tree. In this
	// case, explicitly removing each child node would be unnecessary. It's also possible the entire renderer is being
	// shut down and the scene content is being dumped. The parent node may not even be attached to the scene tree
	// currently. In any of these cases, this warning can be ignored.
	if (_renderer->DebugLoggingEnabled())
	{
		if (_addedAsChildNode.test_and_set(std::memory_order_acquire))
		{
			_log->Warning("Deleting a renderable node that is currently added as a child. If the containing node isn't also being removed before the next frame, a crash is likely if this object is drawn.");
		}
		_addedAsChildNode.clear(std::memory_order_release);
	}

	// Queue this object for deletion
	_renderer->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
// Binding methods
//----------------------------------------------------------------------------------------
SuccessToken VulkanRenderableNode::BindVertexAttribute(IVertexAttribute& vertexAttribute, VertexAttributeId shaderAttributeID)
{
	// Validate the current object state
	if (_immutableStateFrozen)
	{
		_log->Error("Attempted to modify immutable state of a renderable node after it has already been frozen");
		return false;
	}

	// Ensure a valid vertex attribute ID has been supplied
	if (shaderAttributeID == VertexAttributeId::Null)
	{
		_log->Warning("Attempted to bind a vertex attribute with invalid shader ID {0}", shaderAttributeID);
		return false;
	}

	// Ensure a vertex attribute isn't already bound to the target slot
	for (const auto& i : _vertexAttributes[VertexAttributeIndex])
	{
		if (i.attributeID == shaderAttributeID)
		{
			_log->Error("Attempted to bind a vertex attribute with shader ID {0} when an attribute has already been bound to that position.", shaderAttributeID);
			return false;
		}
	}

	// Decode the attribute datatype
	VkFormat dataType;
	if (!GetNativeAttributeDataType(vertexAttribute.GetDataType(), vertexAttribute.GetAttributeElementCount(), dataType))
	{
		_log->Error("Failed to convert vertex attribute type of {0} with element count {1} to a native Vulkan data type, for vertex attribute with shader ID {2}.", vertexAttribute.GetDataType(), vertexAttribute.GetAttributeElementCount(), shaderAttributeID);
		return false;
	}

	// Attempt to retrieve the vertex buffer
	IVertexBuffer* vertexBufferGeneric;
	size_t vertexAttributeIndex;
	if (!GetBoundVertexBuffer(vertexAttribute, vertexBufferGeneric, vertexAttributeIndex))
	{
		_log->Error("Failed to locate bound vertex buffer for vertex attribute with shader ID {0}", shaderAttributeID);
		return false;
	}
	auto* vertexBuffer = KnownDynamicCast<VulkanVertexBuffer*>(vertexBufferGeneric);
	if (!vertexBuffer->IsAllocated())
	{
		_log->Error("Attempted to bind a vertex attribute from a buffer which hasn't been allocated");
		return false;
	}

	// Bind the vertex attribute to the slot
	VertexAttributeInfo attributeInfo{};
	attributeInfo.vertexBuffer = vertexBuffer;
	attributeInfo.info = vertexBuffer->GetVertexAttributeInfo(vertexAttributeIndex);
	attributeInfo.dataType = dataType;
	attributeInfo.attributeID = shaderAttributeID;
	_vertexAttributes[VertexAttributeIndex].push_back(attributeInfo);

	// Set the vertex count to the vertex attribute count if appropriate
	if (!_vertexCountSet && !_indexAttributeBound)
	{
		_buildState.vertexCount = attributeInfo.info->vertexCount;
	}
	return true;
}

//----------------------------------------------------------------------------------------
SuccessToken VulkanRenderableNode::BindVertexInstanceAttribute(IVertexAttribute& vertexAttribute, VertexAttributeId shaderAttributeID)
{
	// Validate the current object state
	if (_immutableStateFrozen)
	{
		_log->Error("Attempted to modify immutable state of a renderable node after it has already been frozen");
		return false;
	}

	// Ensure a valid vertex attribute ID has been supplied
	if (shaderAttributeID == VertexAttributeId::Null)
	{
		_log->Warning("Attempted to bind a vertex instance attribute with invalid shader ID {0}", shaderAttributeID);
		return false;
	}

	// Ensure a vertex attribute isn't already bound to the target slot
	for (const auto& i : _vertexAttributes[VertexInstanceAttributeIndex])
	{
		if (i.attributeID == shaderAttributeID)
		{
			_log->Error("Attempted to bind a vertex instance attribute with shader ID {0} when an attribute has already been bound to that position.", shaderAttributeID);
			return false;
		}
	}

	// Decode the attribute datatype
	VkFormat dataType;
	if (!GetNativeAttributeDataType(vertexAttribute.GetDataType(), vertexAttribute.GetAttributeElementCount(), dataType))
	{
		_log->Error("Failed to convert vertex instance attribute type of {0} to a native Vulkan data type, for vertex instance attribute with shader ID {1}.", vertexAttribute.GetDataType(), shaderAttributeID);
		return false;
	}

	// Attempt to retrieve the vertex buffer
	IVertexBuffer* vertexBufferGeneric;
	size_t vertexAttributeIndex;
	if (!GetBoundVertexBuffer(vertexAttribute, vertexBufferGeneric, vertexAttributeIndex))
	{
		_log->Error("Failed to locate bound vertex buffer for vertex instance attribute with shader ID {0}", shaderAttributeID);
		return false;
	}
	auto* vertexBuffer = KnownDynamicCast<VulkanVertexBuffer*>(vertexBufferGeneric);
	if (!vertexBuffer->IsAllocated())
	{
		_log->Error("Attempted to bind a vertex instance attribute from a buffer which hasn't been allocated");
		return false;
	}

	// Bind the vertex attribute to the slot
	bool firstInstanceAttribute = _vertexAttributes[VertexInstanceAttributeIndex].empty();
	VertexAttributeInfo attributeInfo{};
	attributeInfo.vertexBuffer = vertexBuffer;
	attributeInfo.info = vertexBuffer->GetVertexAttributeInfo(vertexAttributeIndex);
	attributeInfo.dataType = dataType;
	attributeInfo.attributeID = shaderAttributeID;
	_vertexAttributes[VertexInstanceAttributeIndex].push_back(attributeInfo);

	// Set the instance count to the vertex instance attribute count if appropriate
	if (!_instanceCountSet && firstInstanceAttribute)
	{
		_buildState.instanceCount = (uint32_t)attributeInfo.info->vertexCount;
	}
	return true;
}

//----------------------------------------------------------------------------------------
SuccessToken VulkanRenderableNode::BindIndexAttribute(IIndexAttribute& indexAttribute)
{
	// Validate the current object state
	if (_immutableStateFrozen)
	{
		_log->Error("Attempted to modify immutable state of a renderable node after it has already been frozen");
		return false;
	}

	// Ensure an index attribute isn't already bound
	if (_indexAttributeBound)
	{
		_log->Error("Attempted to bind an index attribute when an index attribute has already been bound.");
		return false;
	}

	// Attempt to retrieve the index buffer
	IIndexBuffer* indexBufferGeneric;
	size_t indexAttributeIndex;
	if (!GetBoundIndexBuffer(indexAttribute, indexBufferGeneric, indexAttributeIndex))
	{
		_log->Error("Failed to locate bound index buffer for index attribute");
		return false;
	}
	auto* indexBuffer = KnownDynamicCast<VulkanIndexBuffer*>(indexBufferGeneric);
	if (!indexBuffer->IsAllocated())
	{
		_log->Error("Attempted to bind an index attribute from a buffer which hasn't been allocated");
		return false;
	}

	// Bind the index attribute
	IndexAttributeInfo attributeInfo{};
	attributeInfo.indexBuffer = indexBuffer;
	attributeInfo.info = indexBuffer->GetIndexAttributeInfo(indexAttributeIndex);
	auto type = indexAttribute.GetDataType();
	if (type == IIndexAttribute::DataType::UInt16)
	{
		attributeInfo.dataType = VK_INDEX_TYPE_UINT16;
	}
	else if (type == IIndexAttribute::DataType::UInt32)
	{
		attributeInfo.dataType = VK_INDEX_TYPE_UINT32;
	}
	_indexAttribute = attributeInfo;
	_indexAttributeBound = true;

	// Set the vertex count to the index count if appropriate
	if (!_vertexCountSet)
	{
		_buildState.vertexCount = attributeInfo.info->indexCount;
	}
	return true;
}

//----------------------------------------------------------------------------------------
constexpr bool VulkanRenderableNode::GetNativeAttributeDataType(IVertexAttribute::DataType dataType, size_t attributeElementCount, VkFormat& nativeType)
{
	switch (dataType)
	{
	case IVertexAttribute::DataType::Int8:
		switch (attributeElementCount)
		{
		case 1:
			nativeType = VK_FORMAT_R8_SINT;
			return true;
		case 2:
			nativeType = VK_FORMAT_R8G8_SINT;
			return true;
		case 3:
		case 4:
			nativeType = VK_FORMAT_R8G8B8A8_SINT;
			return true;
		default:
			return false;
		}
	case IVertexAttribute::DataType::Int16:
		switch (attributeElementCount)
		{
		case 1:
			nativeType = VK_FORMAT_R16_SINT;
			return true;
		case 2:
			nativeType = VK_FORMAT_R16G16_SINT;
			return true;
		case 3:
		case 4:
			nativeType = VK_FORMAT_R16G16B16A16_SINT;
			return true;
		default:
			return false;
		}
	case IVertexAttribute::DataType::Int32:
		switch (attributeElementCount)
		{
		case 1:
			nativeType = VK_FORMAT_R32_SINT;
			return true;
		case 2:
			nativeType = VK_FORMAT_R32G32_SINT;
			return true;
		case 3:
			nativeType = VK_FORMAT_R32G32B32_SINT;
			return true;
		case 4:
			nativeType = VK_FORMAT_R32G32B32A32_SINT;
			return true;
		default:
			return false;
		}
	case IVertexAttribute::DataType::UInt8:
		switch (attributeElementCount)
		{
		case 1:
			nativeType = VK_FORMAT_R8_UINT;
			return true;
		case 2:
			nativeType = VK_FORMAT_R8G8_UINT;
			return true;
		case 3:
		case 4:
			nativeType = VK_FORMAT_R8G8B8A8_UINT;
			return true;
		default:
			return false;
		}
	case IVertexAttribute::DataType::UInt16:
		switch (attributeElementCount)
		{
		case 1:
			nativeType = VK_FORMAT_R16_UINT;
			return true;
		case 2:
			nativeType = VK_FORMAT_R16G16_UINT;
			return true;
		case 3:
		case 4:
			nativeType = VK_FORMAT_R16G16B16A16_UINT;
			return true;
		default:
			return false;
		}
	case IVertexAttribute::DataType::UInt32:
		switch (attributeElementCount)
		{
		case 1:
			nativeType = VK_FORMAT_R32_UINT;
			return true;
		case 2:
			nativeType = VK_FORMAT_R32G32_UINT;
			return true;
		case 3:
			nativeType = VK_FORMAT_R32G32B32_UINT;
			return true;
		case 4:
			nativeType = VK_FORMAT_R32G32B32A32_UINT;
			return true;
		default:
			return false;
		}
	case IVertexAttribute::DataType::Norm8:
		switch (attributeElementCount)
		{
		case 1:
			nativeType = VK_FORMAT_R8_SNORM;
			return true;
		case 2:
			nativeType = VK_FORMAT_R8G8_SNORM;
			return true;
		case 3:
		case 4:
			nativeType = VK_FORMAT_R8G8B8A8_SNORM;
			return true;
		default:
			return false;
		}
	case IVertexAttribute::DataType::Norm16:
		switch (attributeElementCount)
		{
		case 1:
			nativeType = VK_FORMAT_R16_SNORM;
			return true;
		case 2:
			nativeType = VK_FORMAT_R16G16_SNORM;
			return true;
		case 3:
		case 4:
			nativeType = VK_FORMAT_R16G16B16A16_SNORM;
			return true;
		default:
			return false;
		}
	case IVertexAttribute::DataType::UNorm8:
		switch (attributeElementCount)
		{
		case 1:
			nativeType = VK_FORMAT_R8_UNORM;
			return true;
		case 2:
			nativeType = VK_FORMAT_R8G8_UNORM;
			return true;
		case 3:
		case 4:
			nativeType = VK_FORMAT_R8G8B8A8_UNORM;
			return true;
		default:
			return false;
		}
	case IVertexAttribute::DataType::UNorm16:
		switch (attributeElementCount)
		{
		case 1:
			nativeType = VK_FORMAT_R16_UNORM;
			return true;
		case 2:
			nativeType = VK_FORMAT_R16G16_UNORM;
			return true;
		case 3:
		case 4:
			nativeType = VK_FORMAT_R16G16B16A16_UNORM;
			return true;
		default:
			return false;
		}
	case IVertexAttribute::DataType::Float16:
		switch (attributeElementCount)
		{
		case 1:
			nativeType = VK_FORMAT_R16_SFLOAT;
			return true;
		case 2:
			nativeType = VK_FORMAT_R16G16_SFLOAT;
			return true;
		case 3:
		case 4:
			nativeType = VK_FORMAT_R16G16B16A16_SFLOAT;
			return true;
		default:
			return false;
		}
	case IVertexAttribute::DataType::Float32:
		switch (attributeElementCount)
		{
		case 1:
			nativeType = VK_FORMAT_R32_SFLOAT;
			return true;
		case 2:
			nativeType = VK_FORMAT_R32G32_SFLOAT;
			return true;
		case 3:
			nativeType = VK_FORMAT_R32G32B32_SFLOAT;
			return true;
		case 4:
			nativeType = VK_FORMAT_R32G32B32A32_SFLOAT;
			return true;
		default:
			return false;
		}
	// This can't be supported, because Direct3D and OpenGL don't support it.
	//case IVertexAttribute::DataType::A2B10G10R10Int:
	//	switch (attributeElementCount)
	//	{
	//	case 1:
	//		nativeType = VK_FORMAT_A2B10G10R10_SINT_PACK32;
	//		return true;
	//	default:
	//		return false;
	//	}
	// This can't be supported, because OpenGL doesn't support it.
	//case IVertexAttribute::DataType::A2B10G10R10UInt:
	//	switch (attributeElementCount)
	//	{
	//	case 1:
	//		nativeType = VK_FORMAT_A2B10G10R10_UINT_PACK32;
	//		return true;
	//	default:
	//		return false;
	//	}
	// This can't be supported, because Direct3D doesn't support it.
	//case IVertexAttribute::DataType::A2B10G10R10Norm:
	//	switch (attributeElementCount)
	//	{
	//	case 1:
	//		nativeType = VK_FORMAT_A2B10G10R10_SNORM_PACK32;
	//		return true;
	//	default:
	//		return false;
	//	}
	case IVertexAttribute::DataType::A2B10G10R10UNorm:
		switch (attributeElementCount)
		{
		case 1:
			nativeType = VK_FORMAT_A2B10G10R10_UNORM_PACK32;
			return true;
		default:
			return false;
		}
	}
	return false;
}

//----------------------------------------------------------------------------------------
// Primitive mode methods
//----------------------------------------------------------------------------------------
SuccessToken VulkanRenderableNode::SetPrimitiveMode(PrimitiveMode primitiveMode, bool primitiveRestartEnabled, bool adjacencyEnabled)
{
	// Validate the current object state
	if (_immutableStateFrozen)
	{
		_log->Error("Attempted to modify immutable state of a renderable node after it has already been frozen");
		return false;
	}

	// Set the specified primitive mode
	_primitiveMode = primitiveMode;
	switch (_primitiveMode)
	{
	case PrimitiveMode::Points:
		if (adjacencyEnabled)
		{
			_log->Error("Unsupported use of adjacency info with points");
			return false;
		}
		_primitiveModeNative = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		break;
	case PrimitiveMode::Lines:
		_primitiveModeNative = (!adjacencyEnabled ? VK_PRIMITIVE_TOPOLOGY_LINE_LIST : VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY);
		break;
	case PrimitiveMode::Triangles:
		_primitiveModeNative = (!adjacencyEnabled ? VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST : VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY);
		break;
	case PrimitiveMode::LineStrip:
		_primitiveModeNative = (!adjacencyEnabled ? VK_PRIMITIVE_TOPOLOGY_LINE_STRIP : VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY);
		break;
	case PrimitiveMode::TriangleStrip:
		_primitiveModeNative = (!adjacencyEnabled ? VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP : VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY);
		break;
	default:
		_log->Error("Attempted to set renderable primitive mode to unsupported value of {0}", primitiveMode);
		return false;
	}

	// Set the specified primitive restart settings
	_primitiveRestartEnabled = primitiveRestartEnabled;
	_primitiveModeSet = true;
	return true;
}

//----------------------------------------------------------------------------------------
SuccessToken VulkanRenderableNode::SetVertexCount(size_t vertexCount, size_t vertexBufferOffset, size_t indexBufferOffset, ptrdiff_t indexValueOffset)
{
	_buildState.nativeObjectsCurrent = false;
	_buildState.vertexCount = vertexCount;
	_buildState.vertexBufferOffset = vertexBufferOffset;
	_buildState.indexBufferOffset = indexBufferOffset;
	_buildState.indexValueOffset = indexValueOffset;
	_buildState.indirectDrawSet = false;
	_vertexCountSet = true;
	FlagDrawStateNotCurrent();
	return true;
}

//----------------------------------------------------------------------------------------
SuccessToken VulkanRenderableNode::SetInstanceMode(uint32_t instanceCount, uint32_t instanceOffset)
{
	_buildState.instanceCount = instanceCount;
	_buildState.instanceOffset = instanceOffset;
	_buildState.indirectDrawSet = false;
	_instanceCountSet = true;
	FlagDrawStateNotCurrent();
	return true;
}

//----------------------------------------------------------------------------------------
SuccessToken VulkanRenderableNode::SetIndirectDraw(size_t drawCount, IDataArray* sourceDataArray, size_t arrayOffsetInBytes, size_t arrayStrideInBytes)
{
	_buildState.nativeObjectsCurrent = false;
	_buildState.vertexCount = 0;
	_buildState.vertexBufferOffset = 0;
	_buildState.indexBufferOffset = 0;
	_buildState.indexValueOffset = 0;
	_buildState.indirectDrawSet = true;
	_buildState.indirectDrawDataArray = KnownDynamicCast<VulkanDataArray*>(sourceDataArray);
	_buildState.indirectDrawCount = drawCount;
	_buildState.indirectDrawArrayOffsetInBytes = arrayOffsetInBytes;
	_buildState.indirectDrawArrayStrideInBytes = (arrayStrideInBytes != 0 ? arrayStrideInBytes : (_indexAttributeBound ? sizeof(IndexedIndirectDrawParams) : sizeof(IndirectDrawParams)));
	FlagDrawStateNotCurrent();
	return true;
}

//----------------------------------------------------------------------------------------
SuccessToken VulkanRenderableNode::SetIndirectDraw(size_t maxDrawCount, IDataArray* drawCountSourceCounter, IDataArray* sourceDataArray, size_t arrayOffsetInBytes, size_t arrayStrideInBytes)
{
	_buildState.nativeObjectsCurrent = false;
	_buildState.vertexCount = 0;
	_buildState.vertexBufferOffset = 0;
	_buildState.indexBufferOffset = 0;
	_buildState.indexValueOffset = 0;
	_buildState.indirectDrawSet = true;
	_buildState.indirectDrawDataArray = KnownDynamicCast<VulkanDataArray*>(sourceDataArray);
	_buildState.indirectMaxDrawCount = maxDrawCount;
	_buildState.indirectDrawCountSourceDataArray = KnownDynamicCast<VulkanDataArray*>(drawCountSourceCounter);
	_buildState.indirectDrawCountOffsetInBytes = 0;
	_buildState.indirectDrawArrayOffsetInBytes = arrayOffsetInBytes;
	_buildState.indirectDrawArrayStrideInBytes = (arrayStrideInBytes != 0 ? arrayStrideInBytes : (_indexAttributeBound ? sizeof(IndexedIndirectDrawParams) : sizeof(IndirectDrawParams)));
	_buildState.indirectDrawCountFromBuffer = false;
	FlagDrawStateNotCurrent();
	return true;
}

//----------------------------------------------------------------------------------------
SuccessToken VulkanRenderableNode::SetIndirectDraw(size_t maxDrawCount, IDataArray* drawCountSourceDataArray, size_t drawCountArrayOffsetInBytes, IDataArray* sourceDataArray, size_t arrayOffsetInBytes, size_t arrayStrideInBytes)
{
	_buildState.nativeObjectsCurrent = false;
	_buildState.vertexCount = 0;
	_buildState.vertexBufferOffset = 0;
	_buildState.indexBufferOffset = 0;
	_buildState.indexValueOffset = 0;
	_buildState.indirectDrawSet = true;
	_buildState.indirectDrawDataArray = KnownDynamicCast<VulkanDataArray*>(sourceDataArray);
	_buildState.indirectMaxDrawCount = maxDrawCount;
	_buildState.indirectDrawCountSourceDataArray = KnownDynamicCast<VulkanDataArray*>(drawCountSourceDataArray);
	_buildState.indirectDrawCountOffsetInBytes = drawCountArrayOffsetInBytes;
	_buildState.indirectDrawArrayOffsetInBytes = arrayOffsetInBytes;
	_buildState.indirectDrawArrayStrideInBytes = (arrayStrideInBytes != 0 ? arrayStrideInBytes : (_indexAttributeBound ? sizeof(IndexedIndirectDrawParams) : sizeof(IndirectDrawParams)));
	_buildState.indirectDrawCountFromBuffer = true;

	FlagDrawStateNotCurrent();
	return true;
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
uint32_t VulkanRenderableNode::GetChildNodeId() const
{
	return _childNodeID;
}

//----------------------------------------------------------------------------------------
void VulkanRenderableNode::SetChildNodeId(uint32_t childNodeID)
{
	_childNodeID = childNodeID;
}

//----------------------------------------------------------------------------------------
bool VulkanRenderableNode::SetAsChildNode()
{
	// Validate the current object state
	if (!_primitiveModeSet)
	{
		_log->Error("Attempted to freeze immutable state of a renderable node before the primitive mode was set.");
		return false;
	}

	// If API debug logging is enabled, attempt to flag the node as being added as a child node, so we can do extra
	// validation.
	if (_renderer->DebugLoggingEnabled())
	{
		if (_addedAsChildNode.test_and_set(std::memory_order_acquire))
		{
			_log->Error("Attempted to add a renderable node as a child of more than one node.");
			return false;
		}
	}

	// Now that we're marked as having been a child node, freeze our immutable state.
	_immutableStateFrozen = true;
	return true;
}

//----------------------------------------------------------------------------------------
void VulkanRenderableNode::RemoveAsChildNode()
{
	// Flag that this renderable is no longer a child node. Note that the immutable state intentionally remains frozen.
	// There is very little state in this object, and very little overhead from re-creating it. If the user wants to
	// modify an immutable aspect of this renderable, they can generate a new renderable node to represent it.
	_addedAsChildNode.clear(std::memory_order_release);
}

//----------------------------------------------------------------------------------------
void VulkanRenderableNode::MigrateBuildStateToDrawState()
{
	// Migrate our build state
	if (!IsDrawStateCurrent())
	{
		VulkanStateContainer::MigrateBuildStateToDrawState();
		bool nativeObjectsCurrent = _drawState.nativeObjectsCurrent;
		_drawState = _buildState;
		_buildState.nativeObjectsCurrent = nativeObjectsCurrent;
		FlagDrawStateCurrent();
	}
}

//----------------------------------------------------------------------------------------
// Render methods
//----------------------------------------------------------------------------------------
bool VulkanRenderableNode::UsesDrawCountFromBuffer() const
{
	return (_drawState.indirectDrawCountSourceDataArray != nullptr);
}

//----------------------------------------------------------------------------------------
void VulkanRenderableNode::Draw(VkCommandBuffer& commandBuffer, VulkanShaderProgram* shaderProgram)
{
	// Build a combined vertex binding array if required
	if (!_vertexBindingArrayGenerated || !_drawState.nativeObjectsCurrent)
	{
		size_t vertexAttributeCount = shaderProgram->GetVertexAttributeCount();
		_vertexAttributeBufferArray.resize(vertexAttributeCount, (_renderer->NullDescriptorFeatureMissingOrBroken() ? _renderer->GetNullDescriptorFallbackVertexBuffer() : VK_NULL_HANDLE));
		_vertexAttributeOffsetArray.resize(vertexAttributeCount, 0);

		// Bind vertex data
		for (const auto& attribute : _vertexAttributes[VertexAttributeIndex])
		{
			VkBuffer buffer = attribute.vertexBuffer->GetNativeBuffer();
			VkDeviceSize offset = attribute.info->bufferStartPosInBytes + (attribute.info->bufferStrideInBytes * _drawState.vertexBufferOffset);
			size_t order = shaderProgram->GetVertexAttributeLocation(attribute.attributeID);
			_vertexAttributeBufferArray[order] = buffer;
			_vertexAttributeOffsetArray[order] = offset;
		}

		// Bind instanced vertex data
		for (const auto& attribute : _vertexAttributes[VertexInstanceAttributeIndex])
		{
			VkBuffer buffer = attribute.vertexBuffer->GetNativeBuffer();
			VkDeviceSize offset = attribute.info->bufferStartPosInBytes + (attribute.info->bufferStrideInBytes * _drawState.vertexBufferOffset);
			size_t order = shaderProgram->GetVertexAttributeLocation(attribute.attributeID);
			_vertexAttributeBufferArray[order] = buffer;
			_vertexAttributeOffsetArray[order] = offset;
		}

		// Flag that we've now generated our buffer views
		_vertexBindingArrayGenerated = true;
		_drawState.nativeObjectsCurrent = true;
	}

	// Bind the vertex buffers
	vkCmdBindVertexBuffers(commandBuffer, 0, (uint32_t)_vertexAttributeBufferArray.size(), _vertexAttributeBufferArray.data(), _vertexAttributeOffsetArray.data());

	// Bind the index buffer if required
	if (_indexAttributeBound)
	{
		vkCmdBindIndexBuffer(commandBuffer, _indexAttribute.indexBuffer->GetNativeBuffer(), _indexAttribute.info->dataTypeByteSize * _drawState.indexBufferOffset, _indexAttribute.dataType);
	}

	// Perform the draw operation for this object
	if (_drawState.indirectDrawSet)
	{
		bool drawCountFromBuffer = (_drawState.indirectDrawCountSourceDataArray != nullptr);
		const auto& extensionInfo = _renderer->GetExtensionInfo();
		if (drawCountFromBuffer && extensionInfo.extensionLoaded_VK_KHR_draw_indirect_count)
		{
			auto drawCountBuffer = (_drawState.indirectDrawCountFromBuffer ? _drawState.indirectDrawCountSourceDataArray->GetNativeBuffer() : _drawState.indirectDrawCountSourceDataArray->GetNativeCounterBuffer());
			if (_indexAttributeBound)
			{
				extensionInfo.vkCmdDrawIndexedIndirectCountKHR(commandBuffer, _drawState.indirectDrawDataArray->GetNativeBuffer(), _drawState.indirectDrawArrayOffsetInBytes, drawCountBuffer, _drawState.indirectDrawCountOffsetInBytes, (uint32_t)_drawState.indirectMaxDrawCount, (uint32_t)_drawState.indirectDrawArrayStrideInBytes);
			}
			else
			{
				extensionInfo.vkCmdDrawIndirectCountKHR(commandBuffer, _drawState.indirectDrawDataArray->GetNativeBuffer(), _drawState.indirectDrawArrayOffsetInBytes, drawCountBuffer, _drawState.indirectDrawCountOffsetInBytes, (uint32_t)_drawState.indirectMaxDrawCount, (uint32_t)_drawState.indirectDrawArrayStrideInBytes);
			}
		}
		else
		{
			// If we need to retrieve the draw count from a memory buffer, retrieve it now. Note that this will require
			// a CPU round trip and therefore will create a pipeline stall. Ideally the VK_KHR_draw_indirect_count
			// extension is available on this device and will handle this more efficiently using the above methods, but
			// if not we fall back to this slower, but working approach. The calling application is made aware of this
			// inefficient fallback being employed by a lack of the IndirectMultiDrawNative feature. Note that the
			// renderer itself has already taken care of queue synchronization for us prior to this call, so we can now
			// just map the buffer here.
			if (drawCountFromBuffer)
			{
				// Map the draw count buffer
				auto drawCountBuffer = (_drawState.indirectDrawCountFromBuffer ? _drawState.indirectDrawCountSourceDataArray->GetNativeAllocation() : _drawState.indirectDrawCountSourceDataArray->GetNativeCounterAllocation());
				uint8_t* mappedBufferPointer = nullptr;
				_renderer->GetMemoryManager()->MapBufferMemory(drawCountBuffer, mappedBufferPointer);

				// Read the draw count from the buffer. Note that we use memcpy here rather than a simple cast to avoid
				// potentially misaligned memory access. While this should be legal on all platforms we care about, it's
				// technically undefined behaviour so we avoid it.
				uint32_t drawCount = 0;
				std::memcpy(&drawCount, mappedBufferPointer + _drawState.indirectDrawCountOffsetInBytes, sizeof(drawCount));

				// Unmap the draw count buffer
				_renderer->GetMemoryManager()->UnmapBufferMemory(drawCountBuffer);

				// Update the current draw count value
				_drawState.indirectDrawCount = drawCount;
			}

			// Perform the indirect draw operation
			if (_indexAttributeBound)
			{
				vkCmdDrawIndexedIndirect(commandBuffer, _drawState.indirectDrawDataArray->GetNativeBuffer(), _drawState.indirectDrawArrayOffsetInBytes, (uint32_t)_drawState.indirectDrawCount, (uint32_t)_drawState.indirectDrawArrayStrideInBytes);
			}
			else
			{
				vkCmdDrawIndirect(commandBuffer, _drawState.indirectDrawDataArray->GetNativeBuffer(), _drawState.indirectDrawArrayOffsetInBytes, (uint32_t)_drawState.indirectDrawCount, (uint32_t)_drawState.indirectDrawArrayStrideInBytes);
			}
		}
	}
	else if (_indexAttributeBound)
	{
		vkCmdDrawIndexed(commandBuffer, (uint32_t)_drawState.vertexCount, _drawState.instanceCount, 0, (int32_t)_drawState.indexValueOffset, _drawState.instanceOffset);
	}
	else
	{
		vkCmdDraw(commandBuffer, (uint32_t)_drawState.vertexCount, _drawState.instanceCount, 0, _drawState.instanceOffset);
	}
}

//----------------------------------------------------------------------------------------
void VulkanRenderableNode::GetPipelineStateSettingsForRenderable(VkPrimitiveTopology& primitiveTopology, bool& primitiveRestartEnabled, std::vector<int>& vertexAttributeIDs, std::vector<int>& instanceAttributeIDs, std::vector<VkFormat>& attributeDataTypes, std::vector<size_t>& attributeStrideInBytes)
{
	// Determine the required sizes of each vector and resize them to fit
	size_t vertexAttributeCount = _vertexAttributes[VertexAttributeIndex].size();
	size_t instanceAttributeCount = _vertexAttributes[VertexInstanceAttributeIndex].size();
	size_t attributeCount = vertexAttributeCount + instanceAttributeCount;
	vertexAttributeIDs.resize(vertexAttributeCount);
	instanceAttributeIDs.resize(instanceAttributeCount);
	attributeDataTypes.resize(attributeCount);
	attributeStrideInBytes.resize(attributeCount);

	// Return the set of known vertex attributes
	for (size_t i = 0; i < vertexAttributeCount; ++i)
	{
		const auto& attributeEntry = _vertexAttributes[VertexAttributeIndex][i];
		vertexAttributeIDs[i] = (int)attributeEntry.attributeID;
		attributeDataTypes[i] = attributeEntry.dataType;
		attributeStrideInBytes[i] = attributeEntry.info->bufferStrideInBytes;
	}

	// Return the set of known instance attributes
	for (size_t i = 0; i < instanceAttributeCount; ++i)
	{
		const auto& attributeEntry = _vertexAttributes[VertexInstanceAttributeIndex][i];
		instanceAttributeIDs[i] = (int)attributeEntry.attributeID;
		attributeDataTypes[vertexAttributeCount + i] = attributeEntry.dataType;
		attributeStrideInBytes[vertexAttributeCount + i] = attributeEntry.info->bufferStrideInBytes;
	}

	// Return the primitive topology information
	primitiveTopology = _primitiveModeNative;
	primitiveRestartEnabled = _primitiveRestartEnabled;
}

//----------------------------------------------------------------------------------------
VulkanShaderProgram::GlobalConstantBufferBindingInfo& VulkanRenderableNode::GetGlobalConstantBufferBindingInfo(int constantStateIndex)
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
void VulkanRenderableNode::SetDebugName(const Marshal::In<std::string>& name)
{
	_buildState.debugName = name;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
const std::string& VulkanRenderableNode::DebugName() const
{
	return _drawState.debugName;
}

} // namespace cobalt::graphics
