// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Direct3DFrameBuffer.h"
#include "Direct3DHeaders.h"
#include "Direct3DShaderProgram.h"
#include "Direct3DStateContainer.h"
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <mutex>
#include <vector>
namespace cobalt::graphics {
class Direct3DRenderer;
class Direct3DRenderableNode;

class Direct3DStateGroupNode : public Direct3DStateContainer<IStateGroupNode>
{
public:
	// Constructors
	Direct3DStateGroupNode(cobalt::logging::ILogger* log, Direct3DRenderer* renderer);

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
	const std::vector<Direct3DRenderableNode*>& GetChildNodes() const;

	// Compute methods
	bool HasComputeTask() const;
	V3UInt32 GetComputeThreadGroupCounts() const;
	void SetComputeTask(const V3UInt32& threadGroupCounts) override;
	void RemoveComputeTask() override;
	Direct3DShaderProgram::GlobalConstantBufferBindingInfo& GetGlobalConstantBufferBindingInfo();

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
	void ApplyFixedState(ID3D11Device1* device, ID3D11DeviceContext1* context, Direct3DFrameBuffer* framebuffer);

	// Debug methods
	void SetDebugName(const Marshal::In<std::string>& name) override;
	const std::wstring& DebugName() const;

private:
	// Structures
	struct BlendState
	{
		size_t index;
		IFrameBuffer::AttachmentType type;
		BlendOperation blendOperationRGB;
		BlendFactor blendFactorSourceRGB;
		BlendFactor blendFactorDestinationRGB;
		BlendOperation blendOperationA;
		BlendFactor blendFactorSourceA;
		BlendFactor blendFactorDestinationA;
		D3D11_BLEND_OP nativeBlendOperationRGB;
		D3D11_BLEND nativeBlendFactorSourceRGB;
		D3D11_BLEND nativeBlendFactorDestinationRGB;
		D3D11_BLEND_OP nativeBlendOperationA;
		D3D11_BLEND nativeBlendFactorSourceA;
		D3D11_BLEND nativeBlendFactorDestinationA;
	};
	struct StencilFaceState
	{
		StencilComparisonFunction comparisonTest;
		StencilOperation passOperation;
		StencilOperation failOperation;
		StencilOperation depthFailOperation;
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
		PolygonFillMode polygonFillMode = {};
		PolygonCullMode polygonCullMode = {};
		PolygonWindingOrder polygonWindingOrder = {};
		uint32_t stencilCompareMask = 0;
		uint32_t stencilWriteMask = 0;
		uint32_t stencilReferenceValue = 0;
		std::wstring debugName = L"StateGroup";
		bool depthTestEnabled = false;
		bool depthWriteEnabled = false;
		bool depthBiasEnabled = false;
		bool blendEnabled = false;
		bool stencilEnabled = false;
		bool nativeObjectsCurrent = false;
		bool computeTaskDefined = false;
	};

private:
	// Child node methods
	bool RemoveChildNode(Direct3DRenderableNode* childNode, uint32_t childNodeID);

	// Build state methods
	bool CreateNativeRasterizerStateObject(ID3D11Device1* device, Microsoft::WRL::ComPtr<ID3D11RasterizerState>& rasterizerState, bool multisamplingEnabled) const;
	bool CreateNativeBlendStateObject(ID3D11Device1* device, Microsoft::WRL::ComPtr<ID3D11BlendState>& blendState) const;
	bool CreateNativeDepthStencilStateObject(ID3D11Device1* device, Microsoft::WRL::ComPtr<ID3D11DepthStencilState>& depthStencilState) const;
	static BlendState BuildBlendState(BlendOperation blendOperationRGB, BlendFactor blendFactorSourceRGB, BlendFactor blendFactorDestinationRGB, BlendOperation blendOperationA, BlendFactor blendFactorSourceA, BlendFactor blendFactorDestinationA);
	static StencilFaceState BuildStencilFaceState(StencilComparisonFunction comparisonTest, StencilOperation passOperation, StencilOperation failOperation, StencilOperation depthFailOperation);
	constexpr static D3D11_BLEND_OP GetNativeBlendOperation(BlendOperation operation);
	constexpr static D3D11_BLEND GetNativeBlendFactor(BlendFactor factor, BlendOperation operation);
	constexpr static D3D11_BLEND GetNativeAlphaBlendFactor(BlendFactor factor, BlendOperation operation);
	constexpr static D3D11_COMPARISON_FUNC GetNativeDepthComparisonFunction(DepthComparisonFunction function);
	constexpr static D3D11_COMPARISON_FUNC GetNativeStencilComparisonFunction(StencilComparisonFunction function);
	constexpr static D3D11_STENCIL_OP GetNativeStencilOperation(StencilOperation operation);

private:
	mutable std::mutex _childNodeMutex;
	MutableState _drawState;
	MutableState _buildState;
	Direct3DShaderProgram::GlobalConstantBufferBindingInfo _globalConstantBufferBindingInfo = {};
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> _nativeRasterizerState;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> _nativeRasterizerStateAntialiasing;
	Microsoft::WRL::ComPtr<ID3D11BlendState> _nativeBlendState;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> _nativeDepthStencilState;
	std::vector<Direct3DRenderableNode*> _drawChildNodes;
	std::vector<Direct3DRenderableNode*> _buildChildNodes;
	std::vector<uint32_t> _removedChildNodeIDs;
	cobalt::logging::ILogger* _log;
	Direct3DRenderer* _renderer;
	bool _nativeRasterizerStateAntialiasingCreated = false;
	bool _hasModifiedChildNodes;
};

} // namespace cobalt::graphics
