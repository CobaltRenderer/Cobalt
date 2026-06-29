// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "BindingHelpers.h"
#include "VulkanIndexBuffer.h"
#include "VulkanShaderProgram.h"
#include "VulkanStateContainer.h"
#include "VulkanVertexBuffer.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <atomic>
#include <vector>
namespace cobalt::graphics {
class VulkanRenderer;
class VulkanDataArray;

class VulkanRenderableNode : public VulkanStateContainer<IRenderableNode>
{
public:
	// Constructors
	VulkanRenderableNode(cobalt::logging::ILogger* log, VulkanRenderer* renderer);

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

	// Render methods
	bool UsesDrawCountFromBuffer() const;
	void Draw(VkCommandBuffer& commandBuffer, VulkanShaderProgram* shaderProgram);
	void GetPipelineStateSettingsForRenderable(VkPrimitiveTopology& primitiveTopology, bool& primitiveRestartEnabled, std::vector<int>& vertexAttributeIDs, std::vector<int>& instanceAttributeIDs, std::vector<VkFormat>& attributeDataTypes, std::vector<size_t>& attributeStrideInBytes);
	VulkanShaderProgram::GlobalConstantBufferBindingInfo& GetGlobalConstantBufferBindingInfo(int constantStateIndex);

	// Debug methods
	void SetDebugName(const Marshal::In<std::string>& name) override;
	const std::string& DebugName() const;

private:
	// Constants
	static const uint32_t VertexAttributeTypeCount = 2;
	static const uint32_t VertexAttributeIndex = 0;
	static const uint32_t VertexInstanceAttributeIndex = 1;

private:
	// Structures
	struct VertexAttributeInfo
	{
		VulkanVertexBuffer* vertexBuffer = nullptr;
		const VulkanVertexBuffer::VertexAttributeInfo* info = nullptr;
		VertexAttributeId attributeID = VertexAttributeId::Null;
		VkFormat dataType = VK_FORMAT_UNDEFINED;
	};
	struct IndexAttributeInfo
	{
		VulkanIndexBuffer* indexBuffer = nullptr;
		const VulkanIndexBuffer::IndexAttributeInfo* info = nullptr;
		VkIndexType dataType = {};
	};
	struct MutableState
	{
		std::string debugName = "Renderable";
		size_t vertexCount;
		size_t vertexBufferOffset;
		size_t indexBufferOffset;
		ptrdiff_t indexValueOffset;
		VulkanDataArray* indirectDrawDataArray = nullptr;
		VulkanDataArray* indirectDrawCountSourceDataArray = nullptr;
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
	constexpr static bool GetNativeAttributeDataType(IVertexAttribute::DataType dataType, size_t attributeElementCount, VkFormat& nativeType);

private:
	MutableState _drawState = {};
	MutableState _buildState = {};
	cobalt::logging::ILogger* _log;
	VulkanRenderer* _renderer;
	VulkanShaderProgram::GlobalConstantBufferBindingInfo _globalConstantBufferBindingInfo = {};
	IndexAttributeInfo _indexAttribute = {};
	std::vector<VulkanShaderProgram::GlobalConstantBufferBindingInfo> _globalConstantBufferBindingInfoWithDefaults;
	std::vector<VertexAttributeInfo> _vertexAttributes[VertexAttributeTypeCount];
	std::vector<VkBuffer> _vertexAttributeBufferArray;
	std::vector<VkDeviceSize> _vertexAttributeOffsetArray;
	int _globalConstantBufferBindingInfoWithDefaultsSize = 0;
	PrimitiveMode _primitiveMode = {};
	VkPrimitiveTopology _primitiveModeNative = {};
	std::atomic_flag _addedAsChildNode = ATOMIC_FLAG_INIT;
	uint32_t _childNodeID = {};
	bool _indexAttributeBound;
	bool _generatedBufferViews;
	bool _primitiveModeSet;
	bool _primitiveRestartEnabled;
	bool _vertexCountSet;
	bool _instanceCountSet;
	bool _immutableStateFrozen;
	bool _vertexBindingArrayGenerated;
};

} // namespace cobalt::graphics
