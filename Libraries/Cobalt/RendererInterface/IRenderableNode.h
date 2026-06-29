// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Deleter.h"
#include "IStateContainer.h"
#include "SuccessToken.h"
#include <Cobalt/Marshalling/Marshalling.pkg>
#include <memory>
namespace cobalt { namespace graphics {
using namespace cobalt::marshalling::operators;
class IVertexAttribute;
class IIndexAttribute;
class IVertexBuffer;
class IIndexBuffer;

class IRenderableNode : public IStateContainer
{
public:
	// Enumerations
	enum class PrimitiveMode
	{
		Points,
		Lines,
		Triangles,
		LineStrip,
		TriangleStrip,
	};

	// Structures
	struct IndirectDrawParams
	{
		uint32_t vertexCount;
		uint32_t instanceCount;
		uint32_t firstVertex;
		uint32_t firstInstance;
	};
	struct IndexedIndirectDrawParams
	{
		uint32_t indexCount;
		uint32_t instanceCount;
		uint32_t firstIndex;
		int32_t vertexOffset;
		uint32_t firstInstance;
	};

	// Typedefs
	typedef std::unique_ptr<IRenderableNode, Deleter<IRenderableNode>> unique_ptr;

public:
	// Initialization methods
	virtual void Delete() = 0;

	// Binding methods
	virtual SuccessToken BindVertexAttribute(IVertexAttribute& vertexAttribute, VertexAttributeId shaderAttributeID) = 0;
	virtual SuccessToken BindVertexInstanceAttribute(IVertexAttribute& vertexAttribute, VertexAttributeId shaderAttributeID) = 0;
	virtual SuccessToken BindIndexAttribute(IIndexAttribute& indexAttribute) = 0;

	// Primitive mode methods
	virtual SuccessToken SetPrimitiveMode(PrimitiveMode primitiveMode, bool primitiveRestartEnabled = false, bool adjacencyEnabled = false) = 0;
	virtual SuccessToken SetVertexCount(size_t vertexCount, size_t vertexBufferOffset = 0, size_t indexBufferOffset = 0, ptrdiff_t indexValueOffset = 0) = 0;
	virtual SuccessToken SetInstanceMode(uint32_t instanceCount, uint32_t instanceOffset = 0) = 0;
	virtual SuccessToken SetIndirectDraw(size_t drawCount, IDataArray* sourceDataArray, size_t arrayOffsetInBytes = 0, size_t arrayStrideInBytes = 0) = 0;
	virtual SuccessToken SetIndirectDraw(size_t maxDrawCount, IDataArray* drawCountSourceCounter, IDataArray* sourceDataArray, size_t arrayOffsetInBytes = 0, size_t arrayStrideInBytes = 0) = 0;
	virtual SuccessToken SetIndirectDraw(size_t maxDrawCount, IDataArray* drawCountSourceDataArray, size_t drawCountArrayOffsetInBytes, IDataArray* sourceDataArray, size_t arrayOffsetInBytes = 0, size_t arrayStrideInBytes = 0) = 0;

	// Debug methods
	virtual void SetDebugName(const Marshal::In<std::string>& name) = 0;

protected:
	// Constructors
	~IRenderableNode() = default;

	// Binding methods
	inline bool GetBoundVertexBuffer(const IVertexAttribute& vertexAttribute, IVertexBuffer*& vertexBuffer, size_t& attributeIndex) const;
	inline bool GetBoundVertexInstanceBuffer(const IVertexAttribute& vertexAttribute, IVertexBuffer*& vertexBuffer, size_t& attributeIndex) const;
	inline bool GetBoundIndexBuffer(const IIndexAttribute& indexAttribute, IIndexBuffer*& indexBuffer, size_t& attributeIndex) const;
};

}} // namespace cobalt::graphics
#include "IRenderableNode.inl"
