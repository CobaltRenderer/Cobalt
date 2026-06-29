// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Deleter.h"
#include "IFrameBuffer.h"
#include "IRenderableNode.h"
#include "IStateContainer.h"
#include <Cobalt/Marshalling/Marshalling.pkg>
#include <limits>
#include <memory>
namespace cobalt { namespace graphics {
using namespace cobalt::marshalling::operators;
class IRenderableNode;

class IStateGroupNode : public IStateContainer
{
public:
	// Enumerations
	enum class PolygonFillMode
	{
		Solid,
		Wireframe,
	};
	enum class PolygonCullMode
	{
		None,
		Front,
		Back,
	};
	enum class PolygonWindingOrder
	{
		Clockwise,
		CounterClockwise,
	};
	enum class DepthComparisonFunction
	{
		Never,
		Equal,
		NotEqual,
		Less,
		LessOrEqual,
		Greater,
		GreaterOrEqual,
		Always,
	};
	enum class StencilTargetFace
	{
		FrontFace,
		BackFace,
		FrontAndBackFace,
	};
	enum class StencilComparisonFunction
	{
		Never,
		Equal,
		NotEqual,
		Less,
		LessOrEqual,
		Greater,
		GreaterOrEqual,
		Always,
	};
	enum class StencilOperation
	{
		Keep,
		Zero,
		Replace,
		IncrementAndClamp,
		DecrementAndClamp,
		IncrementAndWrap,
		DecrementAndWrap,
		Invert,
	};
	enum class BlendOperation
	{
		Add,
		Subtract,
		ReverseSubtract,
		Min,
		Max,
	};
	enum class BlendFactor
	{
		Zero,
		One,
		SourceColor,
		OneMinusSourceColor,
		DestinationColor,
		OneMinusDestinationColor,
		SourceAlpha,
		OneMinusSourceAlpha,
		DestinationAlpha,
		OneMinusDestinationAlpha,
	};

	// Typedefs
	typedef std::unique_ptr<IStateGroupNode, Deleter<IStateGroupNode>> unique_ptr;

public:
	// Initialization methods
	virtual void Delete() = 0;

	// Child node methods
	virtual void AddChildNode(IRenderableNode* childNode) = 0;
	virtual void AddChildNodes(IRenderableNode* const* childNodes, size_t childNodeCount) = 0;
	virtual void AddChildNodes(IRenderableNode::unique_ptr const* childNodes, size_t childNodeCount) = 0;
	virtual void RemoveChildNode(IRenderableNode* childNode) = 0;
	virtual void RemoveChildNodes(IRenderableNode* const* childNodes, size_t childNodeCount) = 0;
	virtual void RemoveChildNodes(IRenderableNode::unique_ptr const* childNodes, size_t childNodeCount) = 0;
	virtual void RemoveAllChildNodes() = 0;
	virtual void SetChildNodes(IRenderableNode* const* childNodes, size_t childNodeCount) = 0;
	virtual void SetChildNodes(IRenderableNode::unique_ptr const* childNodes, size_t childNodeCount) = 0;

	// Compute methods
	virtual void SetComputeTask(const V3UInt32& threadGroupCounts) = 0;
	virtual void RemoveComputeTask() = 0;

	// Depth state methods
	virtual void SetDepthTestEnabled(bool state) = 0;
	virtual void SetDepthWriteEnabled(bool state) = 0;
	virtual void SetDepthComparisonFunction(DepthComparisonFunction comparisonTest) = 0;
	virtual void SetDepthBias(float constantFactor, float slopeFactor, float clamp = 0.0f) = 0;
	virtual void ClearDepthBias() = 0;

	// Stencil state methods
	virtual void SetStencilTestEnabled(bool state, uint32_t compareMask = std::numeric_limits<uint32_t>::max(), uint32_t writeMask = std::numeric_limits<uint32_t>::max()) = 0;
	virtual void SetStencilOperation(StencilTargetFace targetFace, StencilComparisonFunction comparisonTest, StencilOperation passOperation, StencilOperation failOperation, StencilOperation depthFailOperation) = 0;
	virtual void SetStencilReferenceValue(uint32_t referenceValue) = 0;

	// Rasterization state methods
	virtual void SetPolygonFillMode(PolygonFillMode fillMode) = 0;
	virtual void SetPolygonCullMode(PolygonCullMode cullMode) = 0;
	virtual void SetPolygonWindingOrder(PolygonWindingOrder windingOrder) = 0;

	// Blend state methods
	virtual void SetBlendEnabled(bool state) = 0;
	virtual void SetBlendMode(BlendOperation blendOperationRGB, BlendFactor blendFactorSourceRGB, BlendFactor blendFactorDestinationRGB, BlendOperation blendOperationA, BlendFactor blendFactorSourceA, BlendFactor blendFactorDestinationA) = 0;
	virtual void SetBlendMode(IFrameBuffer::AttachmentType type, size_t index, BlendOperation blendOperationRGB, BlendFactor blendFactorSourceRGB, BlendFactor blendFactorDestinationRGB, BlendOperation blendOperationA, BlendFactor blendFactorSourceA, BlendFactor blendFactorDestinationA) = 0;

	// Debug methods
	virtual void SetDebugName(const Marshal::In<std::string>& name) = 0;

protected:
	// Constructors
	~IStateGroupNode() = default;
};

}} // namespace cobalt::graphics
