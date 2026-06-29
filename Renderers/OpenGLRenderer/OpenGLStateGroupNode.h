// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "OpenGLFrameBuffer.h"
#include "OpenGLStateContainer.h"
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <mutex>
#include <vector>
namespace cobalt::graphics {
class OpenGLRenderer;
class OpenGLRenderableNode;

class OpenGLStateGroupNode : public OpenGLStateContainer<IStateGroupNode>
{
public:
	// Constructors
	OpenGLStateGroupNode(cobalt::logging::ILogger* log, OpenGLRenderer* renderer);

	// Initialization methods
	void Delete() override;

	// Child node methods
	void AddChildNode(IRenderableNode* childNode) override;
	void AddChildNodes(IRenderableNode* const* childNodes, size_t childNodeCount) override;
	void AddChildNodes(IRenderableNode::unique_ptr const* childNodes, size_t childNodeCount) override;
	void RemoveChildNode(IRenderableNode* childNode) override;
	void RemoveChildNodes(IRenderableNode* const* childNodes, size_t childNodeCount) override;
	void RemoveChildNodes(IRenderableNode::unique_ptr const* childNodes, size_t childNodeCount) override;
	void RemoveAllChildNodes() override;
	void SetChildNodes(IRenderableNode* const* childNodes, size_t childNodeCount) override;
	void SetChildNodes(IRenderableNode::unique_ptr const* childNodes, size_t childNodeCount) override;
	const std::vector<OpenGLRenderableNode*>& GetChildNodes() const;

	// Compute methods
	bool HasComputeTask() const;
	V3UInt32 GetComputeThreadGroupCounts() const;
	void SetComputeTask(const V3UInt32& threadGroupCounts) override;
	void RemoveComputeTask() override;

	// Depth state methods
	void SetDepthTestEnabled(bool state) override;
	void SetDepthWriteEnabled(bool state) override;
	void SetDepthComparisonFunction(DepthComparisonFunction comparisonTest) override;
	void SetDepthBias(float constantFactor, float slopeFactor, float clamp) override;
	void ClearDepthBias() override;

	// Stencil state methods
	void SetStencilTestEnabled(bool state, uint32_t compareMask, uint32_t writeMask) override;
	void SetStencilOperation(StencilTargetFace targetFace, StencilComparisonFunction comparisonTest, StencilOperation passOperation, StencilOperation failOperation, StencilOperation depthFailOperation) override;
	void SetStencilReferenceValue(uint32_t referenceValue) override;

	// Rasterization state methods
	void SetPolygonFillMode(PolygonFillMode fillMode) override;
	void SetPolygonCullMode(PolygonCullMode cullMode) override;
	void SetPolygonWindingOrder(PolygonWindingOrder windingOrder) override;

	// Blend state methods
	void SetBlendEnabled(bool state) override;
	void SetBlendMode(BlendOperation blendOperationRGB, BlendFactor blendFactorSourceRGB, BlendFactor blendFactorDestinationRGB, BlendOperation blendOperationA, BlendFactor blendFactorSourceA, BlendFactor blendFactorDestinationA) override;
	void SetBlendMode(IFrameBuffer::AttachmentType type, size_t index, BlendOperation blendOperationRGB, BlendFactor blendFactorSourceRGB, BlendFactor blendFactorDestinationRGB, BlendOperation blendOperationA, BlendFactor blendFactorSourceA, BlendFactor blendFactorDestinationA) override;

	// Build state methods
	void MigrateBuildStateToDrawState();
	void ApplyFixedState(OpenGLFrameBuffer* framebuffer);

	// Debug methods
	void SetDebugName(const Marshal::In<std::string>& name) override;
	const std::string& DebugName() const;

private:
	// Structures
	struct BlendState
	{
		size_t index;
		IFrameBuffer::AttachmentType type;
		GLenum nativeBufferType;
		GLuint nativeBufferIndex;
		BlendOperation blendOperationRGB;
		BlendFactor blendFactorSourceRGB;
		BlendFactor blendFactorDestinationRGB;
		BlendOperation blendOperationA;
		BlendFactor blendFactorSourceA;
		BlendFactor blendFactorDestinationA;
		GLenum nativeBlendOperationRGB;
		GLenum nativeBlendFactorSourceRGB;
		GLenum nativeBlendFactorDestinationRGB;
		GLenum nativeBlendOperationA;
		GLenum nativeBlendFactorSourceA;
		GLenum nativeBlendFactorDestinationA;
	};
	struct StencilFaceState
	{
		StencilComparisonFunction comparisonTest;
		StencilOperation passOperation;
		StencilOperation failOperation;
		StencilOperation depthFailOperation;
		GLenum nativeComparisonTest;
		GLenum nativePassOperation;
		GLenum nativeFailOperation;
		GLenum nativeDepthFailOperation;
	};
	struct MutableState
	{
		BlendState sharedBlendState = {};
		StencilFaceState stencilFrontFace = {};
		StencilFaceState stencilBackFace = {};
		V3UInt32 computeThreadGroupCounts = {};
		std::vector<BlendState> attachmentTypeBlendState;
		float depthBiasConstantFactor = 0.0f;
		float depthBiasSlopeFactor = 0.0f;
		float depthBiasClamp = 0.0f;
		DepthComparisonFunction depthComparisonTest = {};
		GLenum nativeDepthComparisonTest = {};
		PolygonFillMode polygonFillMode = {};
		PolygonCullMode polygonCullMode = {};
		PolygonWindingOrder polygonWindingOrder = {};
		uint32_t stencilCompareMask = 0;
		uint32_t stencilWriteMask = 0;
		uint32_t stencilReferenceValue = 0;
		std::string debugName = "StateGroup";
		bool depthTestEnabled = false;
		bool depthWriteEnabled = false;
		bool depthBiasEnabled = false;
		bool blendEnabled = false;
		bool stencilEnabled = false;
		bool computeTaskDefined = false;
	};

private:
	// Child node methods
	bool RemoveChildNode(OpenGLRenderableNode* childNode, uint32_t childNodeID);

	// State methods
	static BlendState BuildBlendState(BlendOperation blendOperationRGB, BlendFactor blendFactorSourceRGB, BlendFactor blendFactorDestinationRGB, BlendOperation blendOperationA, BlendFactor blendFactorSourceA, BlendFactor blendFactorDestinationA);
	static StencilFaceState BuildStencilFaceState(StencilComparisonFunction comparisonTest, StencilOperation passOperation, StencilOperation failOperation, StencilOperation depthFailOperation);
	constexpr static GLenum GetNativeBlendOperation(BlendOperation operation);
	constexpr static GLenum GetNativeBlendFactor(BlendFactor factor);
	constexpr static GLenum GetNativeDepthComparisonFunction(DepthComparisonFunction function);
	constexpr static GLenum GetNativeStencilComparisonFunction(StencilComparisonFunction function);
	constexpr static GLenum GetNativeStencilOperation(StencilOperation operation);
	constexpr static GLenum AttachmentTypeToBufferType(IFrameBuffer::AttachmentType type);

private:
	cobalt::logging::ILogger* _log;
	OpenGLRenderer* _renderer;
	mutable std::mutex _childNodeMutex;
	bool _hasModifiedChildNodes;
	std::vector<OpenGLRenderableNode*> _drawChildNodes;
	std::vector<OpenGLRenderableNode*> _buildChildNodes;
	std::vector<uint32_t> _removedChildNodeIDs;
	MutableState _drawState;
	MutableState _buildState;
};

} // namespace cobalt::graphics
