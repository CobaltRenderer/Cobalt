// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "Direct3DRenderableNode.h"
#include "Direct3DDataArray.h"
#include "Direct3DRenderer.h"
#include <Internal/RendererSupport/KnownDynamicCast.h>
#include <Internal/RendererSupport/UnicodeConversion.h>
#include <list>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
Direct3DRenderableNode::Direct3DRenderableNode(cobalt::logging::ILogger* log, Direct3DRenderer* renderer)
: Direct3DStateContainer(log), _log(log), _renderer(renderer), _indexAttributeBound(false), _primitiveModeSet(false), _vertexCountSet(false), _instanceCountSet(false), _generatedInputLayout(false), _indexBuffer(nullptr)
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
		_buildState.instanceCount = attributeInfo.info->vertexCount;
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

	// Determine the Direct3D primitive render mode to use
	switch (primitiveMode)
	{
	case PrimitiveMode::Points:
		if (adjacencyEnabled)
		{
			_log->Error("Unsupported use of adjacency info with points");
			return false;
		}
		_primitiveModeNative = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
		break;
	case PrimitiveMode::Lines:
		_primitiveModeNative = (!adjacencyEnabled ? D3D_PRIMITIVE_TOPOLOGY_LINELIST : D3D_PRIMITIVE_TOPOLOGY_LINELIST_ADJ);
		break;
	case PrimitiveMode::Triangles:
		_primitiveModeNative = (!adjacencyEnabled ? D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST : D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ);
		break;
	case PrimitiveMode::LineStrip:
		_primitiveModeNative = (!adjacencyEnabled ? D3D_PRIMITIVE_TOPOLOGY_LINESTRIP : D3D_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ);
		break;
	case PrimitiveMode::TriangleStrip:
		_primitiveModeNative = (!adjacencyEnabled ? D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP : D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ);
		break;
	default:
		_log->Error("Attempted to set renderable primitive mode to unsupported value of {0}", primitiveMode);
		return false;
	}
	_primitiveMode = primitiveMode;

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
	FlagDrawStateNotCurrent();
	return true;
}

//----------------------------------------------------------------------------------------
SuccessToken Direct3DRenderableNode::SetIndirectDraw(size_t drawCount, IDataArray* sourceDataArray, size_t arrayOffsetInBytes, size_t arrayStrideInBytes)
{
	_buildState.nativeObjectsCurrent = false;
	_buildState.vertexCount = 0;
	_buildState.vertexBufferOffset = 0;
	_buildState.indexBufferOffset = 0;
	_buildState.indexValueOffset = 0;
	_buildState.indirectDrawSet = true;
	_buildState.indirectDrawDataArray = KnownDynamicCast<Direct3DDataArray*>(sourceDataArray);
	_buildState.indirectDrawCount = drawCount;
	_buildState.indirectDrawArrayOffsetInBytes = arrayOffsetInBytes;
	_buildState.indirectDrawArrayStrideInBytes = (arrayStrideInBytes != 0 ? arrayStrideInBytes : (_indexAttributeBound ? sizeof(IndexedIndirectDrawParams) : sizeof(IndirectDrawParams)));
	FlagDrawStateNotCurrent();
	return true;
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
// Render methods
//----------------------------------------------------------------------------------------
void Direct3DRenderableNode::Draw(Direct3DShaderProgram* shaderProgram, ID3D11DeviceContext1* context)
{
	// Generate the input layout if required
	if (!_generatedInputLayout || !_drawState.nativeObjectsCurrent)
	{
		// Build the required set of vertex input elements
		std::vector<D3D11_INPUT_ELEMENT_DESC> inputDescription;
		std::list<std::string> inputSemanticNames;
		std::vector<ID3D11Buffer*> vertexBufferBuffers;
		std::vector<uint32_t> vertexBufferStrides;
		std::vector<uint32_t> vertexBufferOffsets;
		for (uint32_t attributeTypeIndex = 0; attributeTypeIndex < VertexAttributeTypeCount; ++attributeTypeIndex)
		{
			auto vertexAttributeCount = static_cast<uint32_t>(_vertexAttributes[attributeTypeIndex].size());
			for (uint32_t i = 0; i < vertexAttributeCount; ++i)
			{
				// Retrieve information on the target vertex attribute
				const VertexAttributeInfo& info = _vertexAttributes[attributeTypeIndex][i];
				ID3D11Buffer* buffer = info.vertexBuffer->GetNativeBuffer();

				// Attempt to locate an existing input buffer entry which matches the requirements of this vertex
				// attribute.
				bool foundMatchingBuffer = false;
				uint32_t bufferInputSlot = 0;
				while (!foundMatchingBuffer && (bufferInputSlot < vertexBufferBuffers.size()))
				{
					if ((vertexBufferBuffers[bufferInputSlot] != buffer) || (vertexBufferStrides[bufferInputSlot] != (uint32_t)info.info->bufferStrideInBytes) || (vertexBufferOffsets[bufferInputSlot] != (uint32_t)info.info->bufferBaseStartAddress))
					{
						++bufferInputSlot;
						continue;
					}
					foundMatchingBuffer = true;
				}

				// If no existing input buffer entry could be found, create a new one now.
				if (!foundMatchingBuffer)
				{
					vertexBufferBuffers.push_back(buffer);
					vertexBufferStrides.push_back((uint32_t)info.info->bufferStrideInBytes);
					vertexBufferOffsets.push_back((uint32_t)info.info->bufferBaseStartAddress + (uint32_t)(_drawState.vertexBufferOffset * info.info->bufferStrideInBytes));
				}

				// Add this vertex attribute to the list of defined vertex attributes
				D3D11_INPUT_ELEMENT_DESC descriptionEntry = {};
				inputSemanticNames.push_back(shaderProgram->GetVertexAttributeName(info.attributeID));
				descriptionEntry.SemanticName = inputSemanticNames.back().c_str();
				descriptionEntry.Format = info.dataType;
				descriptionEntry.InputSlot = bufferInputSlot;
				descriptionEntry.AlignedByteOffset = (UINT)(info.info->bufferStartPosInBytes - info.info->bufferBaseStartAddress);
				if (attributeTypeIndex == VertexInstanceAttributeIndex)
				{
					descriptionEntry.InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA;
					descriptionEntry.InstanceDataStepRate = 1;
				}
				else
				{
					descriptionEntry.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
					descriptionEntry.InstanceDataStepRate = 0;
				}
				inputDescription.push_back(descriptionEntry);
			}
		}

		// Attempt to create an input layout for the bound vertex attributes
		if (!shaderProgram->CreateInputLayout(inputDescription, &_inputLayout))
		{
			return;
		}

		// Retrieve the index buffer for efficient lookup during the draw process
		if (_indexAttributeBound)
		{
			_indexBuffer = _indexAttribute.indexBuffer->GetNativeBuffer();
		}

		// Store the defined vertex buffers, and flag that the input layout has been created.
		_vertexBufferBuffers = vertexBufferBuffers;
		_vertexBufferStrides = vertexBufferStrides;
		_vertexBufferOffsets = vertexBufferOffsets;
		_generatedInputLayout = true;
		_drawState.nativeObjectsCurrent = true;
	}

	// Bind the vertex buffers and input layout
	context->IASetVertexBuffers(0, (UINT)_vertexBufferBuffers.size(), _vertexBufferBuffers.data(), _vertexBufferStrides.data(), _vertexBufferOffsets.data());
	context->IASetInputLayout(_inputLayout.Get());

	// Bind the index buffer if required
	if (_indexAttributeBound)
	{
		context->IASetIndexBuffer(_indexBuffer, _indexAttribute.dataType, (UINT)_indexAttribute.info->bufferStartPosInBytes);
	}

	// Set the primitive topology mode
	context->IASetPrimitiveTopology(_primitiveModeNative);

	// Perform the draw operation for this object
	if (_drawState.indirectDrawSet)
	{
		auto* indirectDrawBuffer = _drawState.indirectDrawDataArray->GetNativeBuffer();
		UINT bufferOffset = (UINT)_drawState.indirectDrawArrayOffsetInBytes;
		UINT entryStride = (UINT)_drawState.indirectDrawArrayStrideInBytes;
		if (_indexAttributeBound)
		{
			for (size_t i = 0; i < _drawState.indirectDrawCount; ++i)
			{
				context->DrawIndexedInstancedIndirect(indirectDrawBuffer, bufferOffset);
				bufferOffset += entryStride;
			}
		}
		else
		{
			for (size_t i = 0; i < _drawState.indirectDrawCount; ++i)
			{
				context->DrawInstancedIndirect(indirectDrawBuffer, bufferOffset);
				bufferOffset += entryStride;
			}
		}
	}
	else
	{
		if (_indexAttributeBound)
		{
			if ((_drawState.instanceCount == 1) && (_drawState.instanceOffset == 0))
			{
				context->DrawIndexed((UINT)_drawState.vertexCount, (UINT)_drawState.indexBufferOffset, (INT)_drawState.indexValueOffset);
			}
			else
			{
				context->DrawIndexedInstanced((UINT)_drawState.vertexCount, (UINT)_drawState.instanceCount, (UINT)_drawState.indexBufferOffset, (INT)_drawState.indexValueOffset, (UINT)_drawState.instanceOffset);
			}
		}
		else
		{
			if ((_drawState.instanceCount == 1) && (_drawState.instanceOffset == 0))
			{
				context->Draw((UINT)_drawState.vertexCount, 0);
			}
			else
			{
				context->DrawInstanced((UINT)_drawState.vertexCount, (UINT)_drawState.instanceCount, 0, (UINT)_drawState.instanceOffset);
			}
		}
	}
}

//----------------------------------------------------------------------------------------
bool Direct3DRenderableNode::UsesIndirectMultiDrawWithVariableCount() const
{
	return (_drawState.indirectDrawSet && (_drawState.indirectDrawCountSourceDataArray != nullptr));
}

//----------------------------------------------------------------------------------------
void Direct3DRenderableNode::GetIndirectMultiDrawCountSourceBufferInfo(Direct3DDataArray*& drawCountBuffer, bool& drawCountFromCounter, size_t& drawCountBufferOffsetInBytes) const
{
	drawCountBuffer = _drawState.indirectDrawCountSourceDataArray;
	drawCountFromCounter = !_drawState.indirectDrawCountFromBuffer;
	drawCountBufferOffsetInBytes = _drawState.indirectDrawCountOffsetInBytes;
}

//----------------------------------------------------------------------------------------
void Direct3DRenderableNode::SetCurrentIndirectDrawCount(size_t drawCount)
{
	_drawState.indirectDrawCount = drawCount;
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
