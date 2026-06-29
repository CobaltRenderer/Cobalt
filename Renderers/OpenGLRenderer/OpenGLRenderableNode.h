// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "BindingHelpers.h"
#include "OpenGLIndexBuffer.h"
#include "OpenGLRenderer.h"
#include "OpenGLShaderProgram.h"
#include "OpenGLStateContainer.h"
#include "OpenGLVertexBuffer.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <atomic>
#include <vector>
namespace cobalt::graphics {
class OpenGLDataArray;

class OpenGLRenderableNode : public OpenGLStateContainer<IRenderableNode>
{
public:
	// Constructors
	OpenGLRenderableNode(cobalt::logging::ILogger* log, OpenGLRenderer* renderer);

	// Initialization methods
	void Delete() override;
	void ExtractVertexArrayIDs(std::vector<OpenGLRenderer::ContainerObjectsPendingRelease>& releaseList) const;

	// Binding methods
	SuccessToken BindVertexAttribute(IVertexAttribute& vertexAttribute, VertexAttributeId shaderAttributeID) override;
	SuccessToken BindVertexInstanceAttribute(IVertexAttribute& vertexAttribute, VertexAttributeId shaderAttributeID) override;
	SuccessToken BindIndexAttribute(IIndexAttribute& indexAttribute) override;

	// Primitive mode methods
	SuccessToken SetPrimitiveMode(PrimitiveMode primitiveMode, bool primitiveRestartEnabled, bool adjacencyEnabled) override;
	SuccessToken SetVertexCount(size_t vertexCount, size_t vertexBufferOffset, size_t indexBufferOffset, ptrdiff_t indexValueOffset) override;
	SuccessToken SetInstanceMode(uint32_t instanceCount, uint32_t instanceOffset) override;
	SuccessToken SetIndirectDraw(size_t drawCount, IDataArray* sourceDataArray, size_t arrayOffsetInBytes, size_t arrayStrideInBytes) override;
	SuccessToken SetIndirectDraw(size_t maxDrawCount, IDataArray* drawCountSourceCounter, IDataArray* sourceDataArray, size_t arrayOffsetInBytes, size_t arrayStrideInBytes) override;
	SuccessToken SetIndirectDraw(size_t maxDrawCount, IDataArray* drawCountSourceDataArray, size_t drawCountArrayOffsetInBytes, IDataArray* sourceDataArray, size_t arrayOffsetInBytes, size_t arrayStrideInBytes) override;

	// Build state methods
	uint32_t GetChildNodeId() const;
	void SetChildNodeId(uint32_t childNodeID);
	bool SetAsChildNode();
	void RemoveAsChildNode();
	void MigrateBuildStateToDrawState();

	// Render methods
	void Draw(OpenGLShaderProgram* shaderProgram, size_t renderingContextIndex, size_t renderingContextGenerationIndex);

	// Debug methods
	void SetDebugName(const Marshal::In<std::string>& name) override;
	const std::string& DebugName() const;

private:
	// Constants
	static const uint32_t VertexAttributeTypeCount = 2;
	static const uint32_t VertexAttributeIndex = 0;
	static const uint32_t VertexInstanceAttributeIndex = 1;

	// Structures
	struct VertexAttributeInfo
	{
		OpenGLVertexBuffer* vertexBuffer = nullptr;
		const OpenGLVertexBuffer::VertexAttributeInfo* info = nullptr;
		GLenum dataType = GLenum(0);
		GLint attributeID = 0;
		bool normalizeType = false;
	};
	struct IndexAttributeInfo
	{
		OpenGLIndexBuffer* indexBuffer = nullptr;
		const OpenGLIndexBuffer::IndexAttributeInfo* info = nullptr;
		GLenum dataType = GLenum(0);
		GLenum primitiveMode = GLenum(0);
	};
	struct MutableState
	{
		std::string debugName = "Renderable";
		size_t vertexCount = 0;
		size_t vertexBufferOffset = 0;
		size_t indexBufferOffset = 0;
		ptrdiff_t indexValueOffset = 0;
		size_t instanceCount = 0;
		size_t instanceOffset = 0;
		OpenGLDataArray* indirectDrawDataArray = nullptr;
		OpenGLDataArray* indirectDrawCountSourceDataArray = nullptr;
		size_t indirectDrawCountOffsetInBytes;
		size_t indirectDrawArrayOffsetInBytes;
		size_t indirectDrawArrayStrideInBytes;
		size_t indirectDrawCount;
		size_t indirectMaxDrawCount;
		bool nativeObjectsInvalidated = true;
		bool indirectDrawSet = false;
		bool indirectDrawCountFromBuffer = false;
	};
	struct VertexArrayObjectData
	{
		GLuint vertexArrayID = 0;
		bool vertexArrayGenerated = false;
		bool vertexArrayInvalidated = false;
		size_t renderingContextGenerationIndex = 0;
	};

private:
	// Binding methods
	constexpr static bool GetNativeAttributeDataType(IVertexAttribute::DataType dataType, GLenum& nativeType, bool& normalizeType);

	// Render methods
	void ReleaseVertexArray(size_t renderingContextIndex);

private:
	MutableState _drawState = {};
	MutableState _buildState = {};
	IndexAttributeInfo _indexAttribute = {};
	std::vector<VertexAttributeInfo> _vertexAttributes[VertexAttributeTypeCount];
	std::vector<VertexArrayObjectData> _vertexArrayObjectEntries;
	cobalt::logging::ILogger* _log;
	OpenGLRenderer* _renderer;
	void* _indexBufferByteOffset = {};
	PrimitiveMode _primitiveMode = {};
	GLenum _primitiveModeNative = {};
	std::atomic_flag _addedAsChildNode = ATOMIC_FLAG_INIT;
	uint32_t _childNodeID = 0;
	bool _indexAttributeBound;
	bool _primitiveModeSet;
	bool _primitiveRestartEnabled = false;
	bool _vertexCountSet;
	bool _instanceCountSet;
	bool _immutableStateFrozen;
};

} // namespace cobalt::graphics
