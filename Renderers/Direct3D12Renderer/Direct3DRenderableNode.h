// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "BindingHelpers.h"
#include "BufferWrapper.h"
#include "Direct3DHeaders.h"
#include "Direct3DIndexBuffer.h"
#include "Direct3DShaderProgram.h"
#include "Direct3DStateContainer.h"
#include "Direct3DVertexBuffer.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <atomic>
#include <dxgiformat.h>
#include <vector>
namespace cobalt::graphics {
class Direct3DRenderer;
class Direct3DDataArray;

class Direct3DRenderableNode : public Direct3DStateContainer<IRenderableNode>
{
public:
	// Constructors
	Direct3DRenderableNode(cobalt::logging::ILogger* log, Direct3DRenderer* renderer);

	// Initialization methods
	void Delete() override;

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
	size_t GetTotalVertexCount() const;
	size_t AddResourcesToResidencySet(D3DX12Residency::ResidencySet* residencySet) const;

	// Render methods
	void Draw(ID3D12GraphicsCommandList* commandList, Direct3DShaderProgram* shaderProgram);
	void GetPipelineStateSettingsForRenderable(D3D12_PRIMITIVE_TOPOLOGY_TYPE& primitiveTopology, D3D12_INDEX_BUFFER_STRIP_CUT_VALUE& stripCutValue, std::vector<VertexAttributeId>& vertexAttributeIDs, std::vector<VertexAttributeId>& instanceAttributeIDs, std::vector<DXGI_FORMAT>& attributeDataTypes);
	Direct3DShaderProgram::GlobalConstantBufferBindingInfo& GetGlobalConstantBufferBindingInfo();

	// Debug methods
	void SetDebugName(const Marshal::In<std::string>& name) override;
	const std::wstring& DebugName() const;

private:
	// Constants
	static const uint32_t VertexAttributeTypeCount = 2;
	static const uint32_t VertexAttributeIndex = 0;
	static const uint32_t VertexInstanceAttributeIndex = 1;

private:
	// Structures
	struct VertexAttributeInfo
	{
		Direct3DVertexBuffer* vertexBuffer = nullptr;
		const Direct3DVertexBuffer::VertexAttributeInfo* info = nullptr;
		DXGI_FORMAT dataType = DXGI_FORMAT_UNKNOWN;
		VertexAttributeId attributeID = VertexAttributeId::Null;
	};
	struct IndexAttributeInfo
	{
		Direct3DIndexBuffer* indexBuffer = nullptr;
		const Direct3DIndexBuffer::IndexAttributeInfo* info = nullptr;
		DXGI_FORMAT dataType = DXGI_FORMAT_UNKNOWN;
	};
	struct MutableState
	{
		std::wstring debugName = L"Renderable";
		size_t vertexCount;
		size_t vertexBufferOffset;
		size_t indexBufferOffset;
		ptrdiff_t indexValueOffset;
		size_t totalVertexCount;
		Direct3DDataArray* indirectDrawDataArray = nullptr;
		Direct3DDataArray* indirectDrawCountSourceDataArray = nullptr;
		size_t indirectDrawCountOffsetInBytes;
		size_t indirectDrawArrayOffsetInBytes;
		size_t indirectDrawArrayStrideInBytes;
		size_t indirectDrawCount;
		size_t indirectMaxDrawCount;
		uint32_t instanceCount;
		uint32_t instanceOffset;
		bool nativeObjectsCurrent = false;
		bool indirectDrawSet = false;
		bool indirectDrawCountFromBuffer = false;
	};

private:
	// Binding methods
	constexpr static bool GetNativeAttributeDataType(IVertexAttribute::DataType dataType, size_t attributeElementCount, DXGI_FORMAT& nativeType);

	// Build state methods
	void UpdateTotalVertexCount();

private:
	MutableState _drawState = {};
	MutableState _buildState = {};
	IndexAttributeInfo _indexAttribute = {};
	Direct3DShaderProgram::GlobalConstantBufferBindingInfo _globalConstantBufferBindingInfo = {};
	D3D12_INPUT_LAYOUT_DESC _inputLayoutDescription = {};
	D3D12_INDEX_BUFFER_VIEW _indexBufferView = {};
	std::vector<VertexAttributeInfo> _vertexAttributes[VertexAttributeTypeCount];
	std::vector<D3D12_INPUT_ELEMENT_DESC> _inputLayoutEntries;
	std::vector<D3D12_VERTEX_BUFFER_VIEW> _vertexBufferViews;
	std::vector<BufferWrapper*> _buffersToTransitionBarrier;
	Microsoft::WRL::ComPtr<ID3D12CommandSignature> _indirectDrawCommandSignature;
	cobalt::graphics::IDataArray::unique_ptr _indirectDrawInternalDrawCountBufferOwningPointer;
	cobalt::graphics::IDataArray* _indirectDrawInternalDrawCountBuffer = nullptr;
	cobalt::logging::ILogger* _log;
	Direct3DRenderer* _renderer;
	ID3D12Resource* _indexBuffer;
	PrimitiveMode _primitiveMode = {};
	D3D_PRIMITIVE_TOPOLOGY _primitiveModeNative = {};
	D3D12_PRIMITIVE_TOPOLOGY_TYPE _primitiveTopologyType = {};
	D3D12_INDEX_BUFFER_STRIP_CUT_VALUE _stripCutValue = {};
	std::atomic_flag _addedAsChildNode = ATOMIC_FLAG_INIT;
	uint32_t _childNodeID = 0;
	bool _indexAttributeBound;
	bool _generatedBufferViews;
	bool _primitiveModeSet;
	bool _primitiveRestartEnabled = false;
	bool _vertexCountSet;
	bool _instanceCountSet;
	bool _immutableStateFrozen;
	bool _hasBuffersToTransition = false;
};

} // namespace cobalt::graphics
