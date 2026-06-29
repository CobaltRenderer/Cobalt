// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "OpenGLRenderableNode.h"
#include <Internal/RendererSupport/KnownDynamicCast.h>
#ifdef GL_VERSION_4_3
#include "OpenGLDataArray.h"
#endif
#include "OpenGLDebug.h"
#include "OpenGLHeaders.h"
#include "OpenGLRenderer.h"
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
OpenGLRenderableNode::OpenGLRenderableNode(cobalt::logging::ILogger* log, OpenGLRenderer* renderer)
: OpenGLStateContainer(log), _log(log), _renderer(renderer), _indexAttributeBound(false), _primitiveModeSet(false), _vertexCountSet(false), _instanceCountSet(false)
{
	_immutableStateFrozen = false;
	_buildState.vertexBufferOffset = 0;
	_buildState.indexBufferOffset = 0;
	_buildState.indexValueOffset = 0;
	_buildState.instanceCount = 1;
	_buildState.instanceOffset = 0;
	_buildState.nativeObjectsInvalidated = false;
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void OpenGLRenderableNode::Delete()
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
void OpenGLRenderableNode::ExtractVertexArrayIDs(std::vector<OpenGLRenderer::ContainerObjectsPendingRelease>& releaseList) const
{
	// Add any vertex array object IDs for contexts which are still valid to the free list
	for (size_t i = 0; i < _vertexArrayObjectEntries.size(); ++i)
	{
		const auto& entry = _vertexArrayObjectEntries[i];
		if (entry.vertexArrayGenerated && releaseList[i].slotAllocated && (entry.renderingContextGenerationIndex == releaseList[i].generationIndex))
		{
			releaseList[i].vertexArrayObjects->push_back(entry.vertexArrayID);
		}
	}
}

//----------------------------------------------------------------------------------------
// Binding methods
//----------------------------------------------------------------------------------------
SuccessToken OpenGLRenderableNode::BindVertexAttribute(IVertexAttribute& vertexAttribute, VertexAttributeId shaderAttributeID)
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
		if (i.attributeID == (GLint)shaderAttributeID)
		{
			_log->Error("Attempted to bind a vertex attribute with shader ID {0} when an attribute has already been bound to that position.", shaderAttributeID);
			return false;
		}
	}

	// Decode the attribute datatype
	GLenum dataType;
	bool normalizeType;
	auto elementCount = vertexAttribute.GetAttributeElementCount();
	if ((elementCount < 1) || (elementCount > 4) || !GetNativeAttributeDataType(vertexAttribute.GetDataType(), dataType, normalizeType))
	{
		_log->Error("Failed to convert vertex attribute type of {0} with element count {1} to a native OpenGL data type, for vertex attribute with shader ID {2}.", vertexAttribute.GetDataType(), elementCount, shaderAttributeID);
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
	auto* vertexBuffer = KnownDynamicCast<OpenGLVertexBuffer*>(vertexBufferGeneric);
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
	attributeInfo.normalizeType = normalizeType;
	attributeInfo.attributeID = (GLint)shaderAttributeID;
	_vertexAttributes[VertexAttributeIndex].push_back(attributeInfo);

	// Set the vertex count to the vertex attribute count if appropriate
	if (!_vertexCountSet && !_indexAttributeBound)
	{
		_buildState.vertexCount = attributeInfo.info->vertexCount;
	}
	return true;
}

//----------------------------------------------------------------------------------------
SuccessToken OpenGLRenderableNode::BindVertexInstanceAttribute(IVertexAttribute& vertexAttribute, VertexAttributeId shaderAttributeID)
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
		if (i.attributeID == (GLint)shaderAttributeID)
		{
			_log->Error("Attempted to bind a vertex instance attribute with shader ID {0} when an attribute has already been bound to that position.", shaderAttributeID);
			return false;
		}
	}

	// Decode the attribute datatype
	GLenum dataType;
	bool normalizeType;
	if (!GetNativeAttributeDataType(vertexAttribute.GetDataType(), dataType, normalizeType))
	{
		_log->Error("Failed to convert vertex instance attribute type of {0} to a native OpenGL data type, for vertex instance attribute with shader ID {1}.", vertexAttribute.GetDataType(), shaderAttributeID);
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
	auto* vertexBuffer = KnownDynamicCast<OpenGLVertexBuffer*>(vertexBufferGeneric);
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
	attributeInfo.normalizeType = normalizeType;
	attributeInfo.attributeID = (GLint)shaderAttributeID;
	_vertexAttributes[VertexInstanceAttributeIndex].push_back(attributeInfo);

	// Set the instance count to the vertex instance attribute count if appropriate
	if (!_instanceCountSet && firstInstanceAttribute)
	{
		_buildState.instanceCount = attributeInfo.info->vertexCount;
	}
	return true;
}

//----------------------------------------------------------------------------------------
SuccessToken OpenGLRenderableNode::BindIndexAttribute(IIndexAttribute& indexAttribute)
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

	// Determine the OpenGL data type to use for the index data
	GLenum dataType;
	switch (indexAttribute.GetDataType())
	{
	case IIndexAttribute::DataType::UInt16:
		dataType = GL_UNSIGNED_SHORT;
		break;
	case IIndexAttribute::DataType::UInt32:
		dataType = GL_UNSIGNED_INT;
		break;
	default:
		_log->Error("Failed to convert index attribute type of {0} to a native OpenGL data type.", indexAttribute.GetDataType());
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
	auto* indexBuffer = KnownDynamicCast<OpenGLIndexBuffer*>(indexBufferGeneric);
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
constexpr bool OpenGLRenderableNode::GetNativeAttributeDataType(IVertexAttribute::DataType dataType, GLenum& nativeType, bool& normalizeType)
{
	normalizeType = false;
	switch (dataType)
	{
	case IVertexAttribute::DataType::Int8:
		nativeType = GL_BYTE;
		return true;
	case IVertexAttribute::DataType::Int16:
		nativeType = GL_SHORT;
		return true;
	case IVertexAttribute::DataType::Int32:
		nativeType = GL_INT;
		return true;
	case IVertexAttribute::DataType::UInt8:
		nativeType = GL_UNSIGNED_BYTE;
		return true;
	case IVertexAttribute::DataType::UInt16:
		nativeType = GL_UNSIGNED_SHORT;
		return true;
	case IVertexAttribute::DataType::UInt32:
		nativeType = GL_UNSIGNED_INT;
		return true;
	case IVertexAttribute::DataType::Norm8:
		nativeType = GL_BYTE;
		normalizeType = true;
		return true;
	case IVertexAttribute::DataType::Norm16:
		nativeType = GL_SHORT;
		normalizeType = true;
		return true;
	case IVertexAttribute::DataType::UNorm8:
		nativeType = GL_UNSIGNED_BYTE;
		normalizeType = true;
		return true;
	case IVertexAttribute::DataType::UNorm16:
		nativeType = GL_UNSIGNED_SHORT;
		normalizeType = true;
		return true;
	case IVertexAttribute::DataType::Float16:
		nativeType = GL_HALF_FLOAT;
		return true;
	case IVertexAttribute::DataType::Float32:
		nativeType = GL_FLOAT;
		return true;
	// This can't be supported, because glVertexAttribIPointer doesn't support packed formats. The glVertexAttribIFormat
	// function has been tested for OpenGL 4.3+ and had the same limitation.
	//case IVertexAttribute::DataType::A2B10G10R10Int:
	//	nativeType = GL_INT_2_10_10_10_REV;
	//	return true;
	//case IVertexAttribute::DataType::A2B10G10R10UInt:
	//	nativeType = GL_UNSIGNED_INT_2_10_10_10_REV;
	//	return true;
	// This can't be supported, because Direct3D doesn't support signed 10:10:10:2 packed formats.
	//case IVertexAttribute::DataType::A2B10G10R10Norm:
	//	nativeType = GL_INT_2_10_10_10_REV;
	//	normalizeType = true;
	//	return true;
	case IVertexAttribute::DataType::A2B10G10R10UNorm:
		nativeType = GL_UNSIGNED_INT_2_10_10_10_REV;
		normalizeType = true;
		return true;
	}
	return false;
}

//----------------------------------------------------------------------------------------
// Primitive mode methods
//----------------------------------------------------------------------------------------
SuccessToken OpenGLRenderableNode::SetPrimitiveMode(PrimitiveMode primitiveMode, bool primitiveRestartEnabled, bool adjacencyEnabled)
{
	// Validate the current object state
	if (_immutableStateFrozen)
	{
		_log->Error("Attempted to modify immutable state of a renderable node after it has already been frozen");
		return false;
	}

	// Determine the OpenGL primitive render mode to use
	switch (primitiveMode)
	{
	case PrimitiveMode::Points:
		if (adjacencyEnabled)
		{
			_log->Error("Unsupported use of adjacency info with points");
			return false;
		}
		_primitiveModeNative = GL_POINTS;
		break;
	case PrimitiveMode::Lines:
		_primitiveModeNative = (!adjacencyEnabled ? GL_LINES : GL_LINES_ADJACENCY);
		break;
	case PrimitiveMode::Triangles:
		_primitiveModeNative = (!adjacencyEnabled ? GL_TRIANGLES : GL_TRIANGLES_ADJACENCY);
		break;
	case PrimitiveMode::LineStrip:
		_primitiveModeNative = (!adjacencyEnabled ? GL_LINE_STRIP : GL_LINE_STRIP_ADJACENCY);
		break;
	case PrimitiveMode::TriangleStrip:
		_primitiveModeNative = (!adjacencyEnabled ? GL_TRIANGLE_STRIP : GL_TRIANGLE_STRIP_ADJACENCY);
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
SuccessToken OpenGLRenderableNode::SetVertexCount(size_t vertexCount, size_t vertexBufferOffset, size_t indexBufferOffset, ptrdiff_t indexValueOffset)
{
	_buildState.nativeObjectsInvalidated = true;
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
SuccessToken OpenGLRenderableNode::SetInstanceMode(uint32_t instanceCount, uint32_t instanceOffset)
{
	_buildState.instanceCount = instanceCount;
	_buildState.instanceOffset = instanceOffset;
	_buildState.indirectDrawSet = false;
	_instanceCountSet = true;
	FlagDrawStateNotCurrent();
	return true;
}

//----------------------------------------------------------------------------------------
SuccessToken OpenGLRenderableNode::SetIndirectDraw(size_t drawCount, IDataArray* sourceDataArray, size_t arrayOffsetInBytes, size_t arrayStrideInBytes)
{
#ifndef GL_VERSION_4_3
	_log->Error("Attempted to configure indirect drawing when that feature is not supported by this renderer");
	return false;
#else
	_buildState.nativeObjectsInvalidated = true;
	_buildState.vertexCount = 0;
	_buildState.vertexBufferOffset = 0;
	_buildState.indexBufferOffset = 0;
	_buildState.indexValueOffset = 0;
	_buildState.indirectDrawSet = true;
	_buildState.indirectDrawDataArray = KnownDynamicCast<OpenGLDataArray*>(sourceDataArray);
	_buildState.indirectDrawCount = drawCount;
	_buildState.indirectDrawArrayOffsetInBytes = arrayOffsetInBytes;
	_buildState.indirectDrawArrayStrideInBytes = (arrayStrideInBytes != 0 ? arrayStrideInBytes : (_indexAttributeBound ? sizeof(IndexedIndirectDrawParams) : sizeof(IndirectDrawParams)));
	FlagDrawStateNotCurrent();
	return true;
#endif
}

//----------------------------------------------------------------------------------------
SuccessToken OpenGLRenderableNode::SetIndirectDraw(size_t maxDrawCount, IDataArray* drawCountSourceCounter, IDataArray* sourceDataArray, size_t arrayOffsetInBytes, size_t arrayStrideInBytes)
{
#ifndef GL_VERSION_4_3
	_log->Error("Attempted to configure indirect drawing when that feature is not supported by this renderer");
	return false;
#else
	_buildState.nativeObjectsInvalidated = true;
	_buildState.vertexCount = 0;
	_buildState.vertexBufferOffset = 0;
	_buildState.indexBufferOffset = 0;
	_buildState.indexValueOffset = 0;
	_buildState.indirectDrawSet = true;
	_buildState.indirectDrawDataArray = KnownDynamicCast<OpenGLDataArray*>(sourceDataArray);
	_buildState.indirectMaxDrawCount = maxDrawCount;
	_buildState.indirectDrawCountSourceDataArray = KnownDynamicCast<OpenGLDataArray*>(drawCountSourceCounter);
	_buildState.indirectDrawCountOffsetInBytes = 0;
	_buildState.indirectDrawArrayOffsetInBytes = arrayOffsetInBytes;
	_buildState.indirectDrawArrayStrideInBytes = (arrayStrideInBytes != 0 ? arrayStrideInBytes : (_indexAttributeBound ? sizeof(IndexedIndirectDrawParams) : sizeof(IndirectDrawParams)));
	_buildState.indirectDrawCountFromBuffer = false;
	FlagDrawStateNotCurrent();
	return true;
#endif
}

//----------------------------------------------------------------------------------------
SuccessToken OpenGLRenderableNode::SetIndirectDraw(size_t maxDrawCount, IDataArray* drawCountSourceDataArray, size_t drawCountArrayOffsetInBytes, IDataArray* sourceDataArray, size_t arrayOffsetInBytes, size_t arrayStrideInBytes)
{
#ifndef GL_VERSION_4_3
	_log->Error("Attempted to configure indirect drawing when that feature is not supported by this renderer");
	return false;
#else
	_buildState.nativeObjectsInvalidated = true;
	_buildState.vertexCount = 0;
	_buildState.vertexBufferOffset = 0;
	_buildState.indexBufferOffset = 0;
	_buildState.indexValueOffset = 0;
	_buildState.indirectDrawSet = true;
	_buildState.indirectDrawDataArray = KnownDynamicCast<OpenGLDataArray*>(sourceDataArray);
	_buildState.indirectMaxDrawCount = maxDrawCount;
	_buildState.indirectDrawCountSourceDataArray = KnownDynamicCast<OpenGLDataArray*>(drawCountSourceDataArray);
	_buildState.indirectDrawCountOffsetInBytes = drawCountArrayOffsetInBytes;
	_buildState.indirectDrawArrayOffsetInBytes = arrayOffsetInBytes;
	_buildState.indirectDrawArrayStrideInBytes = (arrayStrideInBytes != 0 ? arrayStrideInBytes : (_indexAttributeBound ? sizeof(IndexedIndirectDrawParams) : sizeof(IndirectDrawParams)));
	_buildState.indirectDrawCountFromBuffer = true;
	FlagDrawStateNotCurrent();
	return true;
#endif
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
uint32_t OpenGLRenderableNode::GetChildNodeId() const
{
	return _childNodeID;
}

//----------------------------------------------------------------------------------------
void OpenGLRenderableNode::SetChildNodeId(uint32_t childNodeID)
{
	_childNodeID = childNodeID;
}

//----------------------------------------------------------------------------------------
bool OpenGLRenderableNode::SetAsChildNode()
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
void OpenGLRenderableNode::RemoveAsChildNode()
{
	// Flag that this renderable is no longer a child node. Note that the immutable state intentionally remains frozen.
	// There is very little state in this object, and very little overhead from re-creating it. If the user wants to
	// modify an immutable aspect of this renderable, they can generate a new renderable node to represent it.
	_addedAsChildNode.clear(std::memory_order_release);

	// Invalidate our vertex arrays when the parent of this node changes. When the parent changes, the shader program
	// could be changing too, and we can't safely use a VAO between shaders unless all the vertex attributes share the
	// same locations between the shaders, which we don't require. Since changing the parent for a renderable isn't
	// intended to be something done extensively each frame, and the overhead of generating a new VAO is quite low, we
	// invalidate any previously generated vertex array objects here to avoid errors.
	_buildState.nativeObjectsInvalidated = true;
}

//----------------------------------------------------------------------------------------
void OpenGLRenderableNode::MigrateBuildStateToDrawState()
{
	// Migrate our build state
	if (!IsDrawStateCurrent())
	{
		OpenGLStateContainer::MigrateBuildStateToDrawState();
		bool pendingNativeObjectInvalidation = _drawState.nativeObjectsInvalidated;
		_drawState = _buildState;
		_drawState.nativeObjectsInvalidated |= pendingNativeObjectInvalidation;
		_buildState.nativeObjectsInvalidated = false;
		FlagDrawStateCurrent();
	}
}

//----------------------------------------------------------------------------------------
// Render methods
//----------------------------------------------------------------------------------------
void OpenGLRenderableNode::Draw(OpenGLShaderProgram* shaderProgram, size_t renderingContextIndex, size_t renderingContextGenerationIndex)
{
	// Specify whether to use vertex array objects (VAOs) when drawing. Note that under the core profile on OpenGL 3.3
	// or higher, VAOs are mandatory for drawing, so this setting cannot be disabled and still have rendering
	// operational on our renderers. The code for drawing without VAOs enabled however is shown here for reference, and
	// if desired, these code pathways could function if the compatibility profile was selected instead.
	static const bool useVertexArrayObjects = true;

	// Retrieve or allocate the vertex array object entry for the given rendering context index. Vertex Array Objects
	// (VAOs) can't be shared across opengl contexts. We can reuse the underlying resources, as they can be shared, but
	// we need to isolate the VAOs to a single context. We do this by providing a unique index value for each unique
	// context, and isolating the vertex array object state for each context.
	if (_vertexArrayObjectEntries.size() <= renderingContextIndex)
	{
		_vertexArrayObjectEntries.resize(renderingContextIndex + 1);
	}
	auto& vertexArrayObjectEntry = _vertexArrayObjectEntries[renderingContextIndex];

	// If the vertex array objects have been invalidated since the last draw, flag them all as invalidated, and clear
	// the overall invalidation flag.
	if (_drawState.nativeObjectsInvalidated)
	{
		for (auto& entry : _vertexArrayObjectEntries)
		{
			entry.vertexArrayInvalidated = true;
		}
		_drawState.nativeObjectsInvalidated = false;
	}

	// Bind our vertex array object, starting the generation process for it if necessary.
	CheckGLError(_log);
	bool generatingVertexArray = false;
	if (useVertexArrayObjects)
	{
		// If the renderer context generation index doesn't match, the previous vertex array object is already deleted,
		// so we clear the association here. There's no need to release the VAO, since it's already been destroyed.
		if (vertexArrayObjectEntry.renderingContextGenerationIndex != renderingContextGenerationIndex)
		{
			vertexArrayObjectEntry.vertexArrayID = 0;
			vertexArrayObjectEntry.vertexArrayGenerated = false;
		}

		// If the vertex array object hasn't been generated yet, generate an ID for it now.
		if (!vertexArrayObjectEntry.vertexArrayGenerated)
		{
			vertexArrayObjectEntry.vertexArrayID = _renderer->GenerateVertexArrayObject(renderingContextIndex);
			vertexArrayObjectEntry.renderingContextGenerationIndex = renderingContextGenerationIndex;
			vertexArrayObjectEntry.vertexArrayInvalidated = true;
		}

		// If the vertex array object is invalid, either due to it not being generated yet, an object state change, or
		// a render context generation change from being previously associated with a rendering context which has
		// subsequently been deleted, mark that we're generating (or possibly updating) the VAO.
		generatingVertexArray = vertexArrayObjectEntry.vertexArrayInvalidated;

		// Bind the vertex array object
		glBindVertexArray(vertexArrayObjectEntry.vertexArrayID);
		CheckGLError(_log);
	}

	// If we haven't generated a vertex array yet, bind our buffers.
	if (!useVertexArrayObjects || generatingVertexArray)
	{
		// Bind all vertex attributes
		uint32_t currentOpenGLBuffer = 0xFFFFFFFF;
		for (uint32_t attributeTypeIndex = 0; attributeTypeIndex < VertexAttributeTypeCount; ++attributeTypeIndex)
		{
			auto vertexAttributeCount = (uint32_t)_vertexAttributes[attributeTypeIndex].size();
			for (uint32_t i = 0; i < vertexAttributeCount; ++i)
			{
				// Bind the vertex array. Note that this actual binding is NOT stored in the VAO state, however it's not
				// needed, because calls to glVertexAttrib*Pointer reference the underlying resource locations, and ARE
				// stored in tha VAO, so drawing will work without these buffers needing to be "bound" again.
				const VertexAttributeInfo& info = _vertexAttributes[attributeTypeIndex][i];
				auto bufferNo = info.vertexBuffer->GetOpenGLBufferNo();
				if (currentOpenGLBuffer != bufferNo)
				{
					glBindBuffer(GL_ARRAY_BUFFER, bufferNo);
					CheckGLError(_log);
					currentOpenGLBuffer = bufferNo;
				}

				// Bind the vertex attribute
				void* vertexBufferByteOffset = reinterpret_cast<void*>((int64_t)info.info->bufferStartPosInBytes + (_drawState.vertexBufferOffset * info.info->bufferStrideInBytes));
#ifdef GL_VERSION_4_1
				if (info.dataType == GL_DOUBLE)
				{
					glVertexAttribLPointer(info.attributeID, (GLint)info.info->elementCount, info.dataType, (GLsizei)info.info->bufferStrideInBytes, vertexBufferByteOffset);
				}
				else
				{
#endif

					// Note that if we know the target field in the shader is a floating point target (which we could check
					// in the shader program), we could support binding int/uint types as inputs to floats in the shader, if
					// we call glVertexAttribPointer instead of glVertexAttribIPointer in this case. The integer type would
					// be converted to its whole number floating point equivalent. The reason we don't do that here is
					// because Direct3D doesn't appear to support converting integer inputs to floats in this manner,
					// although the documentation on this area in Direct3D is very poor. If a way is found to make this work
					// under Direct3D, support could be added for it here in OpenGL.
					auto elementCount = (GLint)info.info->elementCount;
					if (info.dataType == GL_UNSIGNED_INT_2_10_10_10_REV)
					{
						elementCount = 4;
					}
					if (info.normalizeType || (info.dataType == GL_HALF_FLOAT) || (info.dataType == GL_FLOAT))
					{
						glVertexAttribPointer(info.attributeID, elementCount, info.dataType, (info.normalizeType ? GL_TRUE : GL_FALSE), (GLsizei)info.info->bufferStrideInBytes, vertexBufferByteOffset);
					}
					else
					{
						glVertexAttribIPointer(info.attributeID, elementCount, info.dataType, (GLsizei)info.info->bufferStrideInBytes, vertexBufferByteOffset);
					}
#ifdef GL_VERSION_4_1
				}
#endif
				glEnableVertexAttribArray(info.attributeID);
				CheckGLError(_log);

				// If this is an instance attribute, configure it to advance once per instance instead of once per
				// vertex.
				if (attributeTypeIndex == VertexInstanceAttributeIndex)
				{
					glVertexAttribDivisor(info.attributeID, 1);
				}
				CheckGLError(_log);
			}
		}

		// Bind the index buffer. Note that index buffer bindings ARE stored in the VAO, and don't need to be restored
		// when doing a subsequent draw.
		if (_indexAttributeBound)
		{
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _indexAttribute.indexBuffer->GetOpenGLBufferNo());
			CheckGLError(_log);

			// Pre-generate the byte offset to use when accessing the index buffer
			_indexBufferByteOffset = reinterpret_cast<void*>((int64_t)_indexAttribute.info->bufferStartPosInBytes + (_drawState.indexBufferOffset * _indexAttribute.info->dataTypeByteSize));
		}
	}

	// Enable primitive restart if required
	if (_indexAttributeBound && _primitiveRestartEnabled)
	{
		glEnable(GL_PRIMITIVE_RESTART);
		glPrimitiveRestartIndex((_indexAttribute.dataType == GL_UNSIGNED_SHORT) ? 0xFFFF : 0xFFFFFFFF);
	}
	else
	{
		glDisable(GL_PRIMITIVE_RESTART);
	}
	CheckGLError(_log);

	// Draw the object
	if (_drawState.indirectDrawSet)
	{
#ifdef GL_VERSION_4_3
		// Bind the indirect draw array
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _drawState.indirectDrawDataArray->GetBufferNo());

		// Perform the indirect draw operation
		bool drawCountFromBuffer = (_drawState.indirectDrawCountSourceDataArray != nullptr);
#ifdef GL_ARB_indirect_parameters
		if (drawCountFromBuffer && (GLAD_GL_ARB_indirect_parameters != 0))
		{
			// Bind the draw counter from the source buffer
			glBindBuffer(GL_PARAMETER_BUFFER_ARB, (_drawState.indirectDrawCountFromBuffer ? _drawState.indirectDrawCountSourceDataArray->GetBufferNo() : _drawState.indirectDrawCountSourceDataArray->GetCounterBufferNo()));

			// Perform the indirect draw operation
			if (_indexAttributeBound)
			{
				glMultiDrawElementsIndirectCountARB(_primitiveModeNative, _indexAttribute.dataType, reinterpret_cast<void*>(_drawState.indirectDrawArrayOffsetInBytes), (GLintptr)(_drawState.indirectDrawCountOffsetInBytes), (GLsizei)_drawState.indirectMaxDrawCount, (GLsizei)_drawState.indirectDrawArrayStrideInBytes);
			}
			else
			{
				glMultiDrawArraysIndirectCountARB(_primitiveModeNative, reinterpret_cast<void*>(_drawState.indirectDrawArrayOffsetInBytes), (GLintptr)(_drawState.indirectDrawCountOffsetInBytes), (GLsizei)_drawState.indirectMaxDrawCount, (GLsizei)_drawState.indirectDrawArrayStrideInBytes);
			}
		}
		else
#endif
		{
			// If we need to retrieve the draw count from a memory buffer, retrieve it now. Note that this will require
			// a CPU round trip and therefore will create a pipeline stall. Ideally the GL_ARB_indirect_parameters
			// extension is available on this device and will handle this more efficiently using the above methods, but
			// if not we fall back to this slower, but working approach. The calling application is made aware of this
			// inefficient fallback being employed by a lack of the IndirectMultiDrawNative feature.
			if (drawCountFromBuffer)
			{
				// Read the draw counter value from the buffer
				uint32_t drawCount;
				if (!_drawState.indirectDrawCountFromBuffer)
				{
					drawCount = _drawState.indirectDrawCountSourceDataArray->GetCurrentCounterValue();
				}
				else
				{
					_drawState.indirectDrawCountSourceDataArray->GetCurrentBufferData(_drawState.indirectDrawCountOffsetInBytes, &drawCount, sizeof(drawCount));
				}

				// Update the current draw count value
				_drawState.indirectDrawCount = drawCount;
			}

			// Perform the indirect draw operation
			if (_indexAttributeBound)
			{
				glMultiDrawElementsIndirect(_primitiveModeNative, _indexAttribute.dataType, reinterpret_cast<void*>(_drawState.indirectDrawArrayOffsetInBytes), (GLsizei)_drawState.indirectDrawCount, (GLsizei)_drawState.indirectDrawArrayStrideInBytes);
			}
			else
			{
				glMultiDrawArraysIndirect(_primitiveModeNative, reinterpret_cast<void*>(_drawState.indirectDrawArrayOffsetInBytes), (GLsizei)_drawState.indirectDrawCount, (GLsizei)_drawState.indirectDrawArrayStrideInBytes);
			}
		}
#endif
	}
	else if (_indexAttributeBound)
	{
		if (_drawState.indexValueOffset == 0)
		{
			if ((_drawState.instanceCount == 1) && (_drawState.instanceOffset == 0))
			{
				glDrawRangeElements(_primitiveModeNative, (GLuint)_indexAttribute.info->minIndexValue, (GLuint)_indexAttribute.info->maxIndexValue, (GLsizei)_drawState.vertexCount, _indexAttribute.dataType, _indexBufferByteOffset);
			}
			else
			{
#ifdef GL_VERSION_4_2
				glDrawElementsInstancedBaseInstance(_primitiveModeNative, (GLsizei)_drawState.vertexCount, _indexAttribute.dataType, _indexBufferByteOffset, (GLsizei)_drawState.instanceCount, (GLuint)_drawState.instanceOffset);
#else
				glDrawElementsInstanced(_primitiveModeNative, (GLsizei)_drawState.vertexCount, _indexAttribute.dataType, _indexBufferByteOffset, (GLsizei)_drawState.instanceCount);
#endif
			}
		}
		else
		{
			if ((_drawState.instanceCount == 1) && (_drawState.instanceOffset == 0))
			{
				glDrawRangeElementsBaseVertex(_primitiveModeNative, (GLuint)_indexAttribute.info->minIndexValue, (GLuint)_indexAttribute.info->maxIndexValue, (GLsizei)_drawState.vertexCount, _indexAttribute.dataType, _indexBufferByteOffset, (GLint)_drawState.indexValueOffset);
			}
			else
			{
#ifdef GL_VERSION_4_2
				glDrawElementsInstancedBaseVertexBaseInstance(_primitiveModeNative, (GLsizei)_drawState.vertexCount, _indexAttribute.dataType, _indexBufferByteOffset, (GLsizei)_drawState.instanceCount, (GLint)_drawState.indexValueOffset, (GLuint)_drawState.instanceOffset);
#else
				glDrawElementsInstancedBaseVertex(_primitiveModeNative, (GLsizei)_drawState.vertexCount, _indexAttribute.dataType, _indexBufferByteOffset, (GLsizei)_drawState.instanceCount, (GLint)_drawState.indexValueOffset);
#endif
			}
		}
	}
	else
	{
		if ((_drawState.instanceCount == 1) && (_drawState.instanceOffset == 0))
		{
			glDrawArrays(_primitiveModeNative, 0, (GLsizei)_drawState.vertexCount);
		}
		else
		{
#ifdef GL_VERSION_4_2
			glDrawArraysInstancedBaseInstance(_primitiveModeNative, 0, (GLsizei)_drawState.vertexCount, (GLsizei)_drawState.instanceCount, (GLuint)_drawState.instanceOffset);
#else
			glDrawArraysInstanced(_primitiveModeNative, 0, (GLsizei)_drawState.vertexCount, (GLsizei)_drawState.instanceCount);
#endif
		}
	}
	CheckGLError(_log);

	// Disable primitive restart if required
	if (_indexAttributeBound && _primitiveRestartEnabled)
	{
		glDisable(GL_PRIMITIVE_RESTART);
	}

	// If we're not using vertex arrays, unbind our buffers.
	if (!useVertexArrayObjects)
	{
		// Release the index buffer. Note that in a VAO, this binding is part of the VAO state, and will be unbound
		// automatically.
		if (_indexAttributeBound)
		{
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		}

		// Release the vertex buffer
		for (uint32_t attributeTypeIndex = 0; attributeTypeIndex < VertexAttributeTypeCount; ++attributeTypeIndex)
		{
			for (uint32_t i = 0; i < _vertexAttributes[attributeTypeIndex].size(); ++i)
			{
				const VertexAttributeInfo& info = _vertexAttributes[attributeTypeIndex][i];
				if (attributeTypeIndex == VertexInstanceAttributeIndex)
				{
					glVertexAttribDivisor(info.attributeID, 0);
				}
				glDisableVertexAttribArray(info.attributeID);
			}
		}
	}
	CheckGLError(_log);

	// If we're using vertex array objects, unbind the array now. This will automatically release bindings associated
	// with the VAO, such as GL_ELEMENT_ARRAY_BUFFER and our glVertexAttrib*Pointer calls. Note however that bindings
	// of GL_ARRAY_BUFFER are NOT unbound here. It is considered harmless to leave these active however, with no benefit
	// to doing so. Having buffers bound to GL_ARRAY_BUFFER doesn't actually do anything, it just keeps a reference to
	// the buffer if subsequent binding operations occur without changing it. Since we never actually rely on this
	// though, unbinding them would be pointless busy work.
	if (useVertexArrayObjects)
	{
		glBindVertexArray(0);
	}
	CheckGLError(_log);

	// To be totally clean and explicit, we release our GL_ARRAY_BUFFER binding here.
	if (!useVertexArrayObjects || generatingVertexArray)
	{
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	// If we're using indirect drawing, release the indirect draw buffers. Note that these bindings are NOT part of the
	// VAO state, they're global bindings, so we need to make and clear them each time even if we're drawing using a
	// previously generated VAO. Also note that we've seen clear, reproducable issues on AMD drivers under Linux if we
	// do not unbind these buffers. Theoretically it should be safe to "bleed" these bindings, but in practice, issues
	// occur. Memory barriers/synchronization appears correct, so this is likely to be an AMD driver bug. We leave the
	// unbinding here for everything, because it's rare to do indirect drawing at all, let alone at volume, so there
	// shouldn't be any real world overhead from doing so.
#ifdef GL_VERSION_4_3
	if (_drawState.indirectDrawSet)
	{
#ifdef GL_ARB_indirect_parameters
		bool drawCountFromBuffer = (_drawState.indirectDrawCountSourceDataArray != nullptr);
		if (drawCountFromBuffer && (GLAD_GL_ARB_indirect_parameters != 0))
		{
			glBindBuffer(GL_PARAMETER_BUFFER_ARB, 0);
		}
#endif
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
		CheckGLError(_log);
	}
#endif

	// If we've just created a vertex array for this object, flag that a vertex array has now been generated.
	if (generatingVertexArray)
	{
		vertexArrayObjectEntry.vertexArrayInvalidated = false;
	}
}

//----------------------------------------------------------------------------------------
// Debug methods
//----------------------------------------------------------------------------------------
void OpenGLRenderableNode::SetDebugName(const Marshal::In<std::string>& name)
{
	_buildState.debugName = name;
	FlagDrawStateNotCurrent();
}

//----------------------------------------------------------------------------------------
const std::string& OpenGLRenderableNode::DebugName() const
{
	return _drawState.debugName;
}

} // namespace cobalt::graphics
