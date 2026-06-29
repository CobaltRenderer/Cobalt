// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "Direct3DRenderableNode.h"
#include "Direct3DDataArray.h"
#include "Direct3DRenderer.h"
#include <Internal/RendererSupport/KnownDynamicCast.h>
#include <Internal/RendererSupport/UnicodeConversion.h>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
Direct3DRenderableNode::Direct3DRenderableNode(cobalt::logging::ILogger* log, Direct3DRenderer* renderer)
: Direct3DStateContainer(log), _log(log), _renderer(renderer), _indexAttributeBound(false), _primitiveModeSet(false), _vertexCountSet(false), _instanceCountSet(false), _indexBuffer(nullptr), _generatedBufferViews(false)
{
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
void Direct3DRenderableNode::Delete()
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

	// Release ownership of the draw count buffer if it exists. This will queue the object for deletion, but not remove it
	// right away. This will allow any currently drawing frame to complete as intended, meaning our raw pointer to the
	// buffer remains valid for draw purposes, but the buffer will now be released along with this renderable node when the
	// deletion process runs.
	_indirectDrawInternalDrawCountBufferOwningPointer.reset();

	// Queue this object for deletion
	_renderer->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
// Binding methods
//----------------------------------------------------------------------------------------
SuccessToken Direct3DRenderableNode::BindVertexAttribute(IVertexAttribute& vertexAttribute, VertexAttributeId shaderAttributeID)
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
	DXGI_FORMAT dataType;
	if (!GetNativeAttributeDataType(vertexAttribute.GetDataType(), vertexAttribute.GetAttributeElementCount(), dataType))
	{
		_log->Error("Failed to convert vertex attribute type of {0} with element count {1} to a native Direct3D data type, for vertex attribute with shader ID {2}.", vertexAttribute.GetDataType(), vertexAttribute.GetAttributeElementCount(), shaderAttributeID);
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
	auto* vertexBuffer = KnownDynamicCast<Direct3DVertexBuffer*>(vertexBufferGeneric);
	if (!vertexBuffer->IsAllocated())
	{
		_log->Error("Attempted to bind a vertex attribute from a buffer which hasn't been allocated");
		return false;
	}

	// Bind the vertex attribute to the slot
	VertexAttributeInfo attributeInfo = {};
	attributeInfo.vertexBuffer = vertexBuffer;
	attributeInfo.info = vertexBuffer->GetVertexAttributeInfo(vertexAttributeIndex);
	attributeInfo.dataType = dataType;
	attributeInfo.attributeID = shaderAttributeID;
	_vertexAttributes[VertexAttributeIndex].push_back(attributeInfo);

	// Set the vertex count to the vertex attribute count if appropriate
	if (!_vertexCountSet && !_indexAttributeBound)
	{
		_buildState.vertexCount = attributeInfo.info->vertexCount;
		UpdateTotalVertexCount();
	}
	return true;
}

//----------------------------------------------------------------------------------------
SuccessToken Direct3DRenderableNode::BindVertexInstanceAttribute(IVertexAttribute& vertexAttribute, VertexAttributeId shaderAttributeID)
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
	DXGI_FORMAT dataType;
	if (!GetNativeAttributeDataType(vertexAttribute.GetDataType(), vertexAttribute.GetAttributeElementCount(), dataType))
	{
		_log->Error("Failed to convert vertex instance attribute type of {0} to a native Direct3D data type, for vertex instance attribute with shader ID {1}.", vertexAttribute.GetDataType(), shaderAttributeID);
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
	auto* vertexBuffer = KnownDynamicCast<Direct3DVertexBuffer*>(vertexBufferGeneric);
	if (!vertexBuffer->IsAllocated())
	{
		_log->Error("Attempted to bind a vertex instance attribute from a buffer which hasn't been allocated");
		return false;
	}

	// Bind the vertex attribute to the slot
	bool firstInstanceAttribute = _vertexAttributes[VertexInstanceAttributeIndex].empty();
	VertexAttributeInfo attributeInfo = {};
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
SuccessToken Direct3DRenderableNode::BindIndexAttribute(IIndexAttribute& indexAttribute)
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

	// Determine the Direct3D data type to use for the index data
	DXGI_FORMAT dataType;
	switch (indexAttribute.GetDataType())
	{
	case IIndexAttribute::DataType::UInt16:
		dataType = DXGI_FORMAT_R16_UINT;
		break;
	case IIndexAttribute::DataType::UInt32:
		dataType = DXGI_FORMAT_R32_UINT;
		break;
	default:
		_log->Error("Failed to convert index attribute type of {0} to a native Direct3D data type.", indexAttribute.GetDataType());
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
	auto* indexBuffer = KnownDynamicCast<Direct3DIndexBuffer*>(indexBufferGeneric);
	if (!indexBuffer->IsAllocated())
	{
		_log->Error("Attempted to bind an index attribute from a buffer which hasn't been allocated");
		return false;
	}

	// Bind the index attribute
	IndexAttributeInfo attributeInfo = {};
	attributeInfo.indexBuffer = indexBuffer;
	attributeInfo.info = indexBuffer->GetIndexAttributeInfo(indexAttributeIndex);
	attributeInfo.dataType = dataType;
	_indexAttribute = attributeInfo;
	_indexAttributeBound = true;

	// Set the vertex count to the index count if appropriate
	if (!_vertexCountSet)
	{
		_buildState.vertexCount = attributeInfo.info->indexCount;
		UpdateTotalVertexCount();
	}
	return true;
}

//----------------------------------------------------------------------------------------
constexpr bool Direct3DRenderableNode::GetNativeAttributeDataType(IVertexAttribute::DataType dataType, size_t attributeElementCount, DXGI_FORMAT& nativeType)
{
	switch (dataType)
	{
	case IVertexAttribute::DataType::Int8:
		switch (attributeElementCount)
		{
		case 1:
			nativeType = DXGI_FORMAT_R8_SINT;
			return true;
		case 2:
			nativeType = DXGI_FORMAT_R8G8_SINT;
			return true;
		case 3:
		case 4:
			nativeType = DXGI_FORMAT_R8G8B8A8_SINT;
			return true;
		default:
			return false;
		}
	case IVertexAttribute::DataType::Int16:
		switch (attributeElementCount)
		{
		case 1:
			nativeType = DXGI_FORMAT_R16_SINT;
			return true;
		case 2:
			nativeType = DXGI_FORMAT_R16G16_SINT;
			return true;
		case 3:
		case 4:
			nativeType = DXGI_FORMAT_R16G16B16A16_SINT;
			return true;
		default:
			return false;
		}
	case IVertexAttribute::DataType::Int32:
		switch (attributeElementCount)
		{
		case 1:
			nativeType = DXGI_FORMAT_R32_SINT;
			return true;
		case 2:
			nativeType = DXGI_FORMAT_R32G32_SINT;
			return true;
		case 3:
			nativeType = DXGI_FORMAT_R32G32B32_SINT;
			return true;
		case 4:
			nativeType = DXGI_FORMAT_R32G32B32A32_SINT;
			return true;
		default:
			return false;
		}
	case IVertexAttribute::DataType::UInt8:
		switch (attributeElementCount)
		{
		case 1:
			nativeType = DXGI_FORMAT_R8_UINT;
			return true;
		case 2:
			nativeType = DXGI_FORMAT_R8G8_UINT;
			return true;
		case 3:
		case 4:
			nativeType = DXGI_FORMAT_R8G8B8A8_UINT;
			return true;
		default:
			return false;
		}
	case IVertexAttribute::DataType::UInt16:
		switch (attributeElementCount)
		{
		case 1:
			nativeType = DXGI_FORMAT_R16_UINT;
			return true;
		case 2:
			nativeType = DXGI_FORMAT_R16G16_UINT;
			return true;
		case 3:
		case 4:
			nativeType = DXGI_FORMAT_R16G16B16A16_UINT;
			return true;
		default:
			return false;
		}
	case IVertexAttribute::DataType::UInt32:
		switch (attributeElementCount)
		{
		case 1:
			nativeType = DXGI_FORMAT_R32_UINT;
			return true;
		case 2:
			nativeType = DXGI_FORMAT_R32G32_UINT;
			return true;
		case 3:
			nativeType = DXGI_FORMAT_R32G32B32_UINT;
			return true;
		case 4:
			nativeType = DXGI_FORMAT_R32G32B32A32_UINT;
			return true;
		default:
			return false;
		}
	case IVertexAttribute::DataType::Norm8:
		switch (attributeElementCount)
		{
		case 1:
			nativeType = DXGI_FORMAT_R8_SNORM;
			return true;
		case 2:
			nativeType = DXGI_FORMAT_R8G8_SNORM;
			return true;
		case 3:
		case 4:
			nativeType = DXGI_FORMAT_R8G8B8A8_SNORM;
			return true;
		default:
			return false;
		}
	case IVertexAttribute::DataType::Norm16:
		switch (attributeElementCount)
		{
		case 1:
			nativeType = DXGI_FORMAT_R16_SNORM;
			return true;
		case 2:
			nativeType = DXGI_FORMAT_R16G16_SNORM;
			return true;
		case 3:
		case 4:
			nativeType = DXGI_FORMAT_R16G16B16A16_SNORM;
			return true;
		default:
			return false;
		}
	case IVertexAttribute::DataType::UNorm8:
		switch (attributeElementCount)
		{
		case 1:
			nativeType = DXGI_FORMAT_R8_UNORM;
			return true;
		case 2:
			nativeType = DXGI_FORMAT_R8G8_UNORM;
			return true;
		case 3:
		case 4:
			nativeType = DXGI_FORMAT_R8G8B8A8_UNORM;
			return true;
		default:
			return false;
		}
	case IVertexAttribute::DataType::UNorm16:
		switch (attributeElementCount)
		{
		case 1:
			nativeType = DXGI_FORMAT_R16_UNORM;
			return true;
		case 2:
			nativeType = DXGI_FORMAT_R16G16_UNORM;
			return true;
		case 3:
		case 4:
			nativeType = DXGI_FORMAT_R16G16B16A16_UNORM;
			return true;
		default:
			return false;
		}
	case IVertexAttribute::DataType::Float16:
		switch (attributeElementCount)
		{
		case 1:
			nativeType = DXGI_FORMAT_R16_FLOAT;
			return true;
		case 2:
			nativeType = DXGI_FORMAT_R16G16_FLOAT;
			return true;
		case 3:
		case 4:
			nativeType = DXGI_FORMAT_R16G16B16A16_FLOAT;
			return true;
		default:
			return false;
		}
	case IVertexAttribute::DataType::Float32:
		switch (attributeElementCount)
		{
		case 1:
			nativeType = DXGI_FORMAT_R32_FLOAT;
			return true;
		case 2:
			nativeType = DXGI_FORMAT_R32G32_FLOAT;
			return true;
		case 3:
			nativeType = DXGI_FORMAT_R32G32B32_FLOAT;
			return true;
		case 4:
			nativeType = DXGI_FORMAT_R32G32B32A32_FLOAT;
			return true;
		default:
			return false;
		}
	// This can't be supported, because Direct3D doesn't provide a DXGI_FORMAT_R10G10B10A2_SINT type.
	//case IVertexAttribute::DataType::A2B10G10R10Int:
	// This can't be supported, because OpenGL doesn't support it.
	//case IVertexAttribute::DataType::A2B10G10R10UInt:
	//	switch (attributeElementCount)
	//	{
	//	case 1:
	//		nativeType = DXGI_FORMAT_R10G10B10A2_UINT;
	//		return true;
	//	default:
	//		return false;
	//	}
	// This can't be supported, because Direct3D doesn't provide a DXGI_FORMAT_R10G10B10A2_SNORM type.
	//case IVertexAttribute::DataType::A2B10G10R10Norm:
	case IVertexAttribute::DataType::A2B10G10R10UNorm:
		switch (attributeElementCount)
		{
		case 1:
			nativeType = DXGI_FORMAT_R10G10B10A2_UNORM;
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
SuccessToken Direct3DRenderableNode::SetPrimitiveMode(PrimitiveMode primitiveMode, bool primitiveRestartEnabled, bool adjacencyEnabled)
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
		_primitiveModeNative = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
		_primitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
		break;
	case PrimitiveMode::Lines:
		_primitiveModeNative = (!adjacencyEnabled ? D3D_PRIMITIVE_TOPOLOGY_LINELIST : D3D_PRIMITIVE_TOPOLOGY_LINELIST_ADJ);
		_primitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		break;
	case PrimitiveMode::Triangles:
		_primitiveModeNative = (!adjacencyEnabled ? D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST : D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ);
		_primitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		break;
	case PrimitiveMode::LineStrip:
		_primitiveModeNative = (!adjacencyEnabled ? D3D_PRIMITIVE_TOPOLOGY_LINESTRIP : D3D_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ);
		_primitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		break;
	case PrimitiveMode::TriangleStrip:
		_primitiveModeNative = (!adjacencyEnabled ? D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP : D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ);
		_primitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
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
SuccessToken Direct3DRenderableNode::SetVertexCount(size_t vertexCount, size_t vertexBufferOffset, size_t indexBufferOffset, ptrdiff_t indexValueOffset)
{
	_buildState.nativeObjectsCurrent = false;
	_buildState.vertexCount = vertexCount;
	_buildState.vertexBufferOffset = vertexBufferOffset;
	_buildState.indexBufferOffset = indexBufferOffset;
	_buildState.indexValueOffset = indexValueOffset;
	_buildState.indirectDrawSet = false;
	_vertexCountSet = true;
	UpdateTotalVertexCount();
	FlagDrawStateNotCurrent();
	return true;
}

//----------------------------------------------------------------------------------------
SuccessToken Direct3DRenderableNode::SetInstanceMode(uint32_t instanceCount, uint32_t instanceOffset)
{
	_buildState.instanceCount = instanceCount;
	_buildState.instanceOffset = instanceOffset;
	_buildState.indirectDrawSet = false;
	_instanceCountSet = true;
	UpdateTotalVertexCount();
	FlagDrawStateNotCurrent();
	return true;
}

//----------------------------------------------------------------------------------------
SuccessToken Direct3DRenderableNode::SetIndirectDraw(size_t drawCount, IDataArray* sourceDataArray, size_t arrayOffsetInBytes, size_t arrayStrideInBytes)
{
	// Ensure that a data array has been created to hold the draw count value
	if (_indirectDrawInternalDrawCountBuffer == nullptr)
	{
		_indirectDrawInternalDrawCountBufferOwningPointer = _renderer->CreateDataArray();
		_indirectDrawInternalDrawCountBuffer = _indirectDrawInternalDrawCountBufferOwningPointer.get();
		_indirectDrawInternalDrawCountBuffer->SetUsageFlags(IDataArray::UsageFlags::IndirectDrawSource);
		_indirectDrawInternalDrawCountBuffer->SetBufferLayout(4, 1);
		if (!_indirectDrawInternalDrawCountBuffer->AllocateMemory())
		{
			_log->Error("Failed to allocate internal indirect draw count buffer");
			_indirectDrawInternalDrawCountBufferOwningPointer.reset();
			return false;
		}
	}

	// Update the draw count in the internal draw count buffer
	UINT drawCountAsUInt = (UINT)drawCount;
	if (!_indirectDrawInternalDrawCountBuffer->QueueDataUpdate(&drawCountAsUInt, sizeof(drawCountAsUInt)))
	{
		_log->Error("Failed to update internal indirect draw count buffer");
		return false;
	}

	// Pass the internal draw count buffer in as the source buffer for the draw count
	return SetIndirectDraw(drawCount, _indirectDrawInternalDrawCountBuffer, 0, sourceDataArray, arrayOffsetInBytes, arrayStrideInBytes);
}

//----------------------------------------------------------------------------------------
SuccessToken Direct3DRenderableNode::SetIndirectDraw(size_t maxDrawCount, IDataArray* drawCountSourceCounter, IDataArray* sourceDataArray, size_t arrayOffsetInBytes, size_t arrayStrideInBytes)
{
	_buildState.nativeObjectsCurrent = false;
	_buildState.vertexCount = 0;
	_buildState.vertexBufferOffset = 0;
	_buildState.indexBufferOffset = 0;
	_buildState.indexValueOffset = 0;
	_buildState.indirectDrawSet = true;
	_buildState.indirectDrawDataArray = KnownDynamicCast<Direct3DDataArray*>(sourceDataArray);
	_buildState.indirectMaxDrawCount = maxDrawCount;
	_buildState.indirectDrawCountSourceDataArray = KnownDynamicCast<Direct3DDataArray*>(drawCountSourceCounter);
	_buildState.indirectDrawCountOffsetInBytes = 0;
	_buildState.indirectDrawArrayOffsetInBytes = arrayOffsetInBytes;
	_buildState.indirectDrawArrayStrideInBytes = (arrayStrideInBytes != 0 ? arrayStrideInBytes : (_indexAttributeBound ? sizeof(IndexedIndirectDrawParams) : sizeof(IndirectDrawParams)));
	_buildState.indirectDrawCountFromBuffer = false;
	UpdateTotalVertexCount();
	FlagDrawStateNotCurrent();
	return true;
}

//----------------------------------------------------------------------------------------
SuccessToken Direct3DRenderableNode::SetIndirectDraw(size_t maxDrawCount, IDataArray* drawCountSourceDataArray, size_t drawCountArrayOffsetInBytes, IDataArray* sourceDataArray, size_t arrayOffsetInBytes, size_t arrayStrideInBytes)
{
	_buildState.nativeObjectsCurrent = false;
	_buildState.vertexCount = 0;
	_buildState.vertexBufferOffset = 0;
	_buildState.indexBufferOffset = 0;
	_buildState.indexValueOffset = 0;
	_buildState.indirectDrawSet = true;
	_buildState.indirectDrawDataArray = KnownDynamicCast<Direct3DDataArray*>(sourceDataArray);
	_buildState.indirectMaxDrawCount = maxDrawCount;
	_buildState.indirectDrawCountSourceDataArray = KnownDynamicCast<Direct3DDataArray*>(drawCountSourceDataArray);
	_buildState.indirectDrawCountOffsetInBytes = drawCountArrayOffsetInBytes;
	_buildState.indirectDrawArrayOffsetInBytes = arrayOffsetInBytes;
	_buildState.indirectDrawArrayStrideInBytes = (arrayStrideInBytes != 0 ? arrayStrideInBytes : (_indexAttributeBound ? sizeof(IndexedIndirectDrawParams) : sizeof(IndirectDrawParams)));
	_buildState.indirectDrawCountFromBuffer = true;
	UpdateTotalVertexCount();
	FlagDrawStateNotCurrent();
	return true;
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
uint32_t Direct3DRenderableNode::GetChildNodeId() const
{
	return _childNodeID;
}

//----------------------------------------------------------------------------------------
void Direct3DRenderableNode::SetChildNodeId(uint32_t childNodeID)
{
	_childNodeID = childNodeID;
}

//----------------------------------------------------------------------------------------
bool Direct3DRenderableNode::SetAsChildNode()
{
	// Validate the current object state
	if (!_primitiveModeSet)
	{
		_log->Error("Attempted to freeze immutable state of a renderable node before the primitive mode was set.");
		return false;
	}

	// If this is a debug build, attempt to flag the node as being added as a child node, so we can do extra validation.
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

	// Calculate the strip cut value
	if (_primitiveRestartEnabled && _indexAttributeBound)
	{
		_stripCutValue = (_indexAttribute.info->dataType == IIndexAttribute::DataType::UInt16) ? D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFF : D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFFFFFF;
	}
	else
	{
		_stripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
	}
	return true;
}

//----------------------------------------------------------------------------------------
void Direct3DRenderableNode::RemoveAsChildNode()
{
	// Flag that this renderable is no longer a child node. Note that the immutable state intentionally remains frozen.
	// There is very little state in this object, and very little overhead from re-creating it. If the user wants to
	// modify an immutable aspect of this renderable, they can generate a new renderable node to represent it.
	_addedAsChildNode.clear(std::memory_order_release);
}

//----------------------------------------------------------------------------------------
void Direct3DRenderableNode::MigrateBuildStateToDrawState()
{
	// Migrate our build state
	if (!IsDrawStateCurrent())
	{
		Direct3DStateContainer::MigrateBuildStateToDrawState();
		bool nativeObjectsCurrent = _drawState.nativeObjectsCurrent;
		_drawState = _buildState;
		_buildState.nativeObjectsCurrent = nativeObjectsCurrent;
		FlagDrawStateCurrent();
	}
}

//----------------------------------------------------------------------------------------
size_t Direct3DRenderableNode::GetTotalVertexCount() const
{
	return _drawState.totalVertexCount;
}

//----------------------------------------------------------------------------------------
void Direct3DRenderableNode::UpdateTotalVertexCount()
{
	size_t totalVertexCount = 0;
	if (_buildState.indirectDrawSet)
	{
		bool usesVariableIndirectDrawCount = (_buildState.indirectDrawSet && (_buildState.indirectDrawCountSourceDataArray != nullptr));
		totalVertexCount = (usesVariableIndirectDrawCount ? _buildState.indirectMaxDrawCount : _buildState.indirectDrawCount);
	}
	else
	{
		totalVertexCount = _buildState.vertexCount * _buildState.instanceCount;
	}
	_buildState.totalVertexCount = totalVertexCount;
}

//----------------------------------------------------------------------------------------
size_t Direct3DRenderableNode::AddResourcesToResidencySet(D3DX12Residency::ResidencySet* residencySet) const
{
	size_t addedTotalInBytes = 0;
	for (const auto& vertexAttribute : _vertexAttributes)
	{
		auto vertexAttributeCount = (uint32_t)vertexAttribute.size();
		for (uint32_t i = 0; i < vertexAttributeCount; ++i)
		{
			auto* residencyObject = vertexAttribute[i].vertexBuffer->GetResidencyObject();
			addedTotalInBytes += residencySet->Insert(residencyObject) ? (size_t)residencyObject->Size : 0;
		}
	}
	if (_indexAttributeBound)
	{
		auto* residencyObject = _indexAttribute.indexBuffer->GetResidencyObject();
		addedTotalInBytes += residencySet->Insert(residencyObject) ? (size_t)residencyObject->Size : 0;
	}
	return addedTotalInBytes;
}

//----------------------------------------------------------------------------------------
// Render methods
//----------------------------------------------------------------------------------------
void Direct3DRenderableNode::Draw(ID3D12GraphicsCommandList* commandList, Direct3DShaderProgram* shaderProgram)
{
	// Build our index and vertex buffer views if required
	if (!_generatedBufferViews || !_drawState.nativeObjectsCurrent)
	{
		// Build our set of vertex buffer views
		shaderProgram->GetDefaultVertexBufferViews(_vertexBufferViews);
		for (auto& vertexAttribute : _vertexAttributes)
		{
			auto vertexAttributeCount = (uint32_t)vertexAttribute.size();
			for (uint32_t i = 0; i < vertexAttributeCount; ++i)
			{
				// Retrieve information on the target vertex attribute
				const VertexAttributeInfo& info = vertexAttribute[i];
				UINT slotNumber = shaderProgram->GetVertexAttributeSlot(info.attributeID);
				ID3D12Resource* buffer = info.vertexBuffer->GetNativeBuffer();

				// Update the vertex buffer view for this attribute
				D3D12_VERTEX_BUFFER_VIEW& vertexBufferView = _vertexBufferViews[slotNumber];
				vertexBufferView.BufferLocation = buffer->GetGPUVirtualAddress() + info.info->bufferStartPosInBytes + (_drawState.vertexBufferOffset * info.info->bufferStrideInBytes);
				vertexBufferView.StrideInBytes = (UINT)info.info->bufferStrideInBytes;
				vertexBufferView.SizeInBytes = (UINT)((info.info->vertexCount - _drawState.vertexBufferOffset) * info.info->bufferStrideInBytes);

				// Add this buffer to our set of buffers to transition if required
				if (!_generatedBufferViews && info.vertexBuffer->HasBufferAlias())
				{
					_buffersToTransitionBarrier.push_back(info.vertexBuffer->GetBufferWrapper());
					_hasBuffersToTransition = true;
				}
			}
		}

		// Create our index buffer view
		if (_indexAttributeBound)
		{
			// Update the index buffer view
			_indexBufferView.BufferLocation = _indexAttribute.indexBuffer->GetNativeBuffer()->GetGPUVirtualAddress() + _indexAttribute.info->bufferStartPosInBytes + (_drawState.indexBufferOffset * _indexAttribute.info->dataTypeByteSize);
			_indexBufferView.Format = _indexAttribute.dataType;
			_indexBufferView.SizeInBytes = (UINT)((_indexAttribute.info->indexCount - _drawState.indexBufferOffset) * _indexAttribute.info->dataTypeByteSize);

			// Add this buffer to our set of buffers to transition if required
			if (!_generatedBufferViews && _indexAttribute.indexBuffer->HasBufferAlias())
			{
				_buffersToTransitionBarrier.push_back(_indexAttribute.indexBuffer->GetBufferWrapper());
				_hasBuffersToTransition = true;
			}
		}

		// Create the command signature for indirect drawing if required
		if (_drawState.indirectDrawSet)
		{
			D3D12_INDIRECT_ARGUMENT_DESC indirectArgumentDescription = {};
			indirectArgumentDescription.Type = (_indexAttributeBound ? D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED : D3D12_INDIRECT_ARGUMENT_TYPE_DRAW);
			D3D12_COMMAND_SIGNATURE_DESC commandSignatureDescription = {};
			commandSignatureDescription.pArgumentDescs = &indirectArgumentDescription;
			commandSignatureDescription.NumArgumentDescs = 1;
			commandSignatureDescription.ByteStride = (UINT)_drawState.indirectDrawArrayStrideInBytes;
			_renderer->GetDevice()->CreateCommandSignature(&commandSignatureDescription, nullptr, IID_PPV_ARGS(&_indirectDrawCommandSignature));
		}

		// Flag that we've now generated our buffer views
		_generatedBufferViews = true;
		_drawState.nativeObjectsCurrent = true;
	}

	// Transition any attached vertex and index buffers to the required resource state. We only do this step for index
	// and vertex buffers which are aliased as texel arrays, as they may have last been bound as an unordered access
	// view, and we need to transition them back to their "default" state here, which is suitable for using as their
	// primary buffer type.
	if (_hasBuffersToTransition)
	{
		for (auto* bufferWrapper : _buffersToTransitionBarrier)
		{
			if (bufferWrapper->lastResourceState != bufferWrapper->defaultResourceState)
			{
				auto resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(bufferWrapper->buffer, bufferWrapper->lastResourceState, bufferWrapper->defaultResourceState);
				commandList->ResourceBarrier(1, &resourceBarrier);
				bufferWrapper->lastResourceState = bufferWrapper->defaultResourceState;
			}
		}
	}

	// Bind the vertex buffers
	commandList->IASetVertexBuffers(0, (UINT)_vertexBufferViews.size(), _vertexBufferViews.data());

	// Bind the index buffer if required
	if (_indexAttributeBound)
	{
		commandList->IASetIndexBuffer(&_indexBufferView);
	}

	// Set the primitive topology mode
	commandList->IASetPrimitiveTopology(_primitiveModeNative);

	// Perform the draw operation for this object
	if (_drawState.indirectDrawSet)
	{
		// Retrieve the buffers to use for the indirect draw operation
		auto countBufferLastResourceState = (_drawState.indirectDrawCountFromBuffer ? _drawState.indirectDrawCountSourceDataArray->GetLastResourceState() : D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		auto* countBuffer = (_drawState.indirectDrawCountFromBuffer ? _drawState.indirectDrawCountSourceDataArray->GetNativeBuffer() : _drawState.indirectDrawCountSourceDataArray->GetCounterBuffer());
		auto indirectDrawBufferLastResourceState = _drawState.indirectDrawDataArray->GetLastResourceState();
		auto* indirectDrawBuffer = _drawState.indirectDrawDataArray->GetNativeBuffer();

		// Transition the buffers to the required resource state for reading
		if ((countBuffer != indirectDrawBuffer) && (countBufferLastResourceState != D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT))
		{
			auto countBarrierAcquire = CD3DX12_RESOURCE_BARRIER::Transition(countBuffer, countBufferLastResourceState, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
			commandList->ResourceBarrier(1, &countBarrierAcquire);
		}
		if (indirectDrawBufferLastResourceState != D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT)
		{
			auto indirectDrawBarrierAcquire = CD3DX12_RESOURCE_BARRIER::Transition(indirectDrawBuffer, indirectDrawBufferLastResourceState, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
			commandList->ResourceBarrier(1, &indirectDrawBarrierAcquire);
		}

		// Perform the indirect draw operation
		commandList->ExecuteIndirect(_indirectDrawCommandSignature.Get(), (UINT)_drawState.indirectMaxDrawCount, indirectDrawBuffer, _drawState.indirectDrawArrayOffsetInBytes, countBuffer, _drawState.indirectDrawCountOffsetInBytes);

		// Transition the buffers to the last resource state if required
		if (indirectDrawBufferLastResourceState != D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT)
		{
			auto indirectDrawBarrierRelease = CD3DX12_RESOURCE_BARRIER::Transition(indirectDrawBuffer, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, indirectDrawBufferLastResourceState);
			commandList->ResourceBarrier(1, &indirectDrawBarrierRelease);
		}
		if ((countBuffer != indirectDrawBuffer) && (countBufferLastResourceState != D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT))
		{
			auto countBarrierRelease = CD3DX12_RESOURCE_BARRIER::Transition(countBuffer, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, countBufferLastResourceState);
			commandList->ResourceBarrier(1, &countBarrierRelease);
		}
	}
	else if (_indexAttributeBound)
	{
		commandList->DrawIndexedInstanced((UINT)_drawState.vertexCount, (UINT)_drawState.instanceCount, 0, (INT)_drawState.indexValueOffset, (UINT)_drawState.instanceOffset);
	}
	else
	{
		commandList->DrawInstanced((UINT)_drawState.vertexCount, (UINT)_drawState.instanceCount, 0, (UINT)_drawState.instanceOffset);
	}
}

//----------------------------------------------------------------------------------------
void Direct3DRenderableNode::GetPipelineStateSettingsForRenderable(D3D12_PRIMITIVE_TOPOLOGY_TYPE& primitiveTopology, D3D12_INDEX_BUFFER_STRIP_CUT_VALUE& stripCutValue, std::vector<VertexAttributeId>& vertexAttributeIDs, std::vector<VertexAttributeId>& instanceAttributeIDs, std::vector<DXGI_FORMAT>& attributeDataTypes)
{
	// Determine the required sizes of each vector and resize them to fit
	size_t vertexAttributeCount = _vertexAttributes[VertexAttributeIndex].size();
	size_t instanceAttributeCount = _vertexAttributes[VertexInstanceAttributeIndex].size();
	size_t attributeCount = vertexAttributeCount + instanceAttributeCount;
	vertexAttributeIDs.resize(vertexAttributeCount);
	instanceAttributeIDs.resize(instanceAttributeCount);
	attributeDataTypes.resize(attributeCount);

	// Return the set of known vertex attributes
	for (size_t i = 0; i < vertexAttributeCount; ++i)
	{
		const auto& attributeEntry = _vertexAttributes[VertexAttributeIndex][i];
		vertexAttributeIDs[i] = attributeEntry.attributeID;
		attributeDataTypes[i] = attributeEntry.dataType;
	}

	// Return the set of known instance attributes
	for (size_t i = 0; i < instanceAttributeCount; ++i)
	{
		const auto& attributeEntry = _vertexAttributes[VertexInstanceAttributeIndex][i];
		instanceAttributeIDs[i] = attributeEntry.attributeID;
		attributeDataTypes[vertexAttributeCount + i] = attributeEntry.dataType;
	}

	// Return the primitive topology information
	primitiveTopology = _primitiveTopologyType;
	stripCutValue = _stripCutValue;
}

//----------------------------------------------------------------------------------------
Direct3DShaderProgram::GlobalConstantBufferBindingInfo& Direct3DRenderableNode::GetGlobalConstantBufferBindingInfo()
{
	return _globalConstantBufferBindingInfo;
}

//----------------------------------------------------------------------------------------
// Debug methods
//----------------------------------------------------------------------------------------
void Direct3DRenderableNode::SetDebugName(const Marshal::In<std::string>& name)
{
	// String is stored as wide string because render markers require a wide string
	_buildState.debugName = UTF8ToUTF16(name);
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
const std::wstring& Direct3DRenderableNode::DebugName() const
{
	return _drawState.debugName;
}

} // namespace cobalt::graphics
