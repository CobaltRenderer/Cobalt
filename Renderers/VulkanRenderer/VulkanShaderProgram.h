// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "VulkanHeaders.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <Internal/ShaderSupport/ShaderSupport.pkg>
#include <memory>
#include <string>
#include <unordered_map>
namespace cobalt::graphics {
class VulkanRenderer;
class VulkanDataArray;
class VulkanStateBuffer;
class VulkanStateBufferLayout;

class VulkanShaderProgram : public IShaderProgram
{
public:
	// Constants
	static const uint32_t ShaderStageCount = 4;

	// Structures
	struct GlobalConstantBufferBindingInfo
	{
		bool hasBindings;
		size_t descriptorIndex;
		bool bindingSet[ShaderStageCount];
		uint32_t pageNumbers[ShaderStageCount];
	};
	struct GlobalStateBufferBuildingSession
	{
		bool buffersExist[ShaderStageCount];
		uint32_t nextPageNumbers[ShaderStageCount];
		uint32_t lastWrittenPageBlockIndex[ShaderStageCount];
		VulkanStateBuffer* stateBuffers[ShaderStageCount];
	};
	struct StateBufferBindPointEntry
	{
		IShaderCode::Stage stageMask = {};
		uint32_t bindPoint = 0xFFFFFFFF;
	};
	struct ResourceArrayBindPointEntry
	{
		IShaderCode::Stage stageMask = {};
		uint32_t bindPoint = 0xFFFFFFFF;
		uint32_t counterBindPoint = 0xFFFFFFFF;
		bool writeable = false;
	};
	struct SamplerBindPointEntry
	{
		IShaderCode::Stage stageMask = {};
		uint32_t bindPoint = 0xFFFFFFFF;
	};
	struct TextureBindPointEntry
	{
		IShaderCode::Stage stageMask = {};
		uint32_t bindPoint = 0xFFFFFFFF;
		VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	};

public:
	// Constructors
	VulkanShaderProgram(cobalt::logging::ILogger* log, VulkanRenderer* renderer);
	~VulkanShaderProgram();

	// Initialization methods
	void Delete() override;

	// Code format methods
	bool IsCodeFormatSupported(CodeFormat format) const override;
	CodeFormat PreferredCodeFormat() const override;

	// Compilation methods
	SuccessToken ConfigureShaderTarget(const ShaderTargetInfoBase& shaderTargetInfo) override;
	SuccessToken LoadShaderStage(ShaderStage stage, const ShaderSourceInfoBase& shaderSourceInfo) override;
	SuccessToken CompileProgram() override;
	VkShaderModule GetShaderModule(ShaderStage stage) const;
	std::string GetShaderEntryPointName(ShaderStage stage) const;

	// Shader input methods
	bool VertexAttributeExists(const Marshal::In<std::string>& name) const override;
	bool StateValueExists(const Marshal::In<std::string>& name) const override;
	bool TextureExists(const Marshal::In<std::string>& name) const override;
	bool SamplerExists(const Marshal::In<std::string>& name) const override;
	bool StateBufferExists(const Marshal::In<std::string>& name) const override;
	bool ResourceArrayExists(const Marshal::In<std::string>& name) const override;
	VertexAttributeId GetVertexAttributeId(const Marshal::In<std::string>& name) const override;
	StateValueId GetStateValueId(const Marshal::In<std::string>& name) const override;
	TextureId GetTextureId(const Marshal::In<std::string>& name) const override;
	SamplerId GetSamplerId(const Marshal::In<std::string>& name) const override;
	StateBufferId GetStateBufferId(const Marshal::In<std::string>& name) const override;
	ResourceArrayId GetResourceArrayId(const Marshal::In<std::string>& name) const override;
	size_t GetVertexAttributeLocation(VertexAttributeId id);

	// State buffer methods
	SuccessToken LoadStateBufferLayoutFromShader(StateBufferId stateBufferID, IStateBufferLayout* stateBufferLayout) const override;

	// Shader binding methods
	size_t GetVertexAttributeCount() const;
	const std::vector<VkVertexInputBindingDescription>& GetVertexBindingDescriptions() const;
	const std::vector<VkVertexInputAttributeDescription>& GetVertexAttributeDescriptions() const;
	void GetBindPointsForStateBuffer(StateBufferId stateBufferId, const StateBufferBindPointEntry*& bindPoints, size_t& bindPointCount) const;
	void GetBindPointsForTexture(TextureId textureId, const TextureBindPointEntry*& bindPoints, size_t& bindPointCount) const;
	void GetBindPointsForSampler(SamplerId samplerId, const SamplerBindPointEntry*& bindPoints, size_t& bindPointCount) const;
	void GetBindPointsForResourceArray(ResourceArrayId resourceArrayId, const ResourceArrayBindPointEntry*& bindPoints, size_t& bindPointCount) const;
	void BindNullDescriptorFallbacks(size_t setIndex) const;

	// State value methods
	void UpdateStateValue(StateValueId stateId, const uint8_t* data, size_t dataSizeInBytes, const size_t* arrayIndices, size_t arrayIndexCount);
	void SetConstantValue(StateValueId stateId, const uint8_t* data, size_t dataSizeInBytes, const size_t* arrayIndices, size_t arrayIndexCount);
	void PushGlobalStateBufferState();
	void PopGlobalStateBufferState();
	void RestoreGlobalStateBufferBaseline();

	// Global constant buffer session methods
	void ResetGlobalConstantBufferState();
	void BeginGlobalConstantBufferBuildingSession(VkCommandBuffer commandBuffer, GlobalStateBufferBuildingSession& stateInfo);
	void CompleteGlobalConstantBufferBuildingSession(VkCommandBuffer commandBuffer, GlobalStateBufferBuildingSession& stateInfo);
	void GenerateGlobalConstantBufferBindings(VkCommandBuffer commandBuffer, GlobalStateBufferBuildingSession& stateInfo, GlobalConstantBufferBindingInfo& bindingInfo);
	void ApplyGlobalConstantBufferBindings(VkCommandBuffer commandBuffer, const GlobalConstantBufferBindingInfo& bindingInfo, VkPipelineLayout pipelineLayout);

	// Descriptor methods
	void ClearDescriptorSets();
	size_t AllocateBindingDescriptorSet();
	size_t AllocateGlobalStateDescriptorSet();
	void CopyDescriptorSet(size_t sourceDescriptorSetIndex, size_t targetDescriptorSetIndex);
	void BindDescriptorSet(VkCommandBuffer commandBuffer, size_t index, VkPipelineLayout pipelineLayout);
	VkDescriptorSet GetDescriptorSet(size_t setIndex) const;
	const std::vector<VkDescriptorSetLayout>& GetDescriptorSetLayouts() const;

private:
	// Constants
	static const size_t DescriptorPoolPageSize = 256;

	// Structures
	struct ShaderInputParameterInfo
	{
		std::string name;
		size_t shaderResourceIndex = 0;
	};
	struct ShaderResourceInfo
	{
		IShaderCode::Resource resourceInfo[ShaderStageCount];
	};
	struct StateBufferEntryInfo
	{
		bool defined[ShaderStageCount] = {false};
		std::string attributeName;
		StateValueId attributeID = StateValueId::Null;
		IStateBufferLayout::DataType type = IStateBufferLayout::DataType::Null;
		//size_t width = 0;
		//size_t height = 0;
		std::vector<size_t> arraySizes;
		std::vector<size_t> arrayStrides[ShaderStageCount];
		std::vector<size_t> leadingArraySizes;
		std::vector<size_t> leadingArrayStrides[ShaderStageCount];
		IShaderCode::Resource resourceEntries[ShaderStageCount];
		size_t arraySize = 0;
		size_t arrayStrideInBytes[ShaderStageCount] = {0};
		size_t bufferOffset[ShaderStageCount] = {0};
		//##TODO## Review the usage of these fields and if they still need to be here
		size_t entrySizeInBytes = 0;
		size_t dataSizeInBytes = 0;
	};
	struct StateBufferInfo
	{
		std::string name;
		StateBufferId id = {};
		size_t shaderResourceIndex = 0;
		std::vector<StateBufferBindPointEntry> bindPoints;
		size_t totalBufferSize = 0;
		std::vector<StateBufferEntryInfo> uniformBufferEntries;
		IShaderCode::Resource resourceInfo[ShaderStageCount];
	};
	struct GlobalStateBufferInfo
	{
		bool bufferExists = false;
		bool isDirty = true;
		std::vector<unsigned char> bufferContents;
		std::vector<std::vector<unsigned char>> bufferStack;
		size_t bufferStackEntries = 0;
		StateBufferInfo bufferInfo;
		std::unique_ptr<VulkanStateBuffer> stateBuffer;
		uint32_t allocatedPageCount = 0;
	};
	struct ResourceBufferInfo
	{
		std::string name;
		ResourceArrayId id = {};
		size_t shaderResourceIndex = 0;
		std::vector<ResourceArrayBindPointEntry> bindPoints;
		bool isDataArray = false;
		bool hasAttachedCounterBuffer = false;
		bool isAttachedDataArrayCounter = false;
		size_t counterBufferIndex = 0;
	};
	struct ShaderTextureInfo
	{
		std::string name;
		TextureId id = {};
		size_t shaderResourceIndex = 0;
		VkImageViewType imageViewType = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
		std::vector<TextureBindPointEntry> bindPoints;
	};
	struct ShaderSamplerInfo
	{
		std::string name;
		SamplerId id = {};
		size_t shaderResourceIndex = 0;
		std::vector<SamplerBindPointEntry> bindPoints;
	};
	struct PendingDescriptorWriteInfo
	{
		size_t pageNumber;
		size_t descriptorIndex;
		VulkanStateBuffer* stateBuffer;
		int binding;
	};
	struct ShaderBlockInfo
	{
		IShaderCode::unique_ptr code;
		std::string entryPointName;
	};

private:
	// Initialization methods
	void ReleaseMemory();

	// Shader binding methods
	void BuildNullDescriptorFallbacks() const;

	// State value methods
	constexpr static size_t ShaderStageToIndex(ShaderStage stage);
	constexpr ShaderStage ShaderIndexToStage(size_t index);
	constexpr static IStateBufferLayout::DataType ShaderSupportDataTypeToGraphicsDataType(IShaderCode::DataType dataType);
	bool CreateGlobalStateBuffer(ShaderStage stage);
	bool LoadStateBufferInfo(size_t shaderStageIndex, StateBufferInfo& bufferInfo, std::unordered_map<std::string, StateValueId>& attributeNamesToIDs, const IShaderCode::Resource& shaderResource) const;
	bool LoadStateBufferStructEntry(size_t shaderStageIndex, StateBufferInfo& bufferInfo, std::unordered_map<std::string, StateValueId>& attributeNamesToIDs, const std::string& leadingVariableName, const std::vector<size_t>& leadingArraySizes, const std::vector<size_t>& leadingArrayStrides, size_t leadingBufferOffsetInBytes, const IShaderCode::Resource& shaderResource) const;
	bool LoadStateBufferVariableEntry(size_t shaderStageIndex, StateBufferInfo& bufferInfo, std::unordered_map<std::string, StateValueId>& attributeNamesToIDs, const std::string& variableName, const std::vector<size_t>& leadingArraySizes, const std::vector<size_t>& leadingArrayStrides, const IShaderCode::Resource& shaderResource, size_t startOffset) const;
	bool StateBufferEntryNativeTypeToBufferLayoutType(const IShaderCode::Resource& resourceInfo, IStateBufferLayout::DataType& type) const;

	// Compilation methods
	bool CreateShaderModule(IShaderCode* shaderCode, VkShaderModule& shaderModule, ShaderStage shaderStage);

private:
	cobalt::logging::ILogger* _log;
	VulkanRenderer* _renderer;
	ShaderBlockInfo _shaderBlocks[ShaderStageCount];
	VkShaderModule _shaderModules[ShaderStageCount] = {VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE};
	bool _isComputeShader = false;
	std::vector<VkDescriptorBufferInfo> _globalStateDescriptorBufferInfo;
	std::vector<VkWriteDescriptorSet> _globalStateDesriptorSetWrites;
	std::vector<PendingDescriptorWriteInfo> _pendingDescriptorWrites;
	std::vector<VkDescriptorPool> _bindingDescriptorPools;
	std::vector<VkDescriptorPool> _globalStateBindingDescriptorPools;
	std::vector<VkDescriptorPoolSize> _bindingDescriptorPoolSizes;
	VkDescriptorPoolSize _globalStateBindingDescriptorPoolSize = {};
	uint32_t _bindingDescriptorCountPerSet = 0;
	int _baseDescriptorBindingNo = 0;
	size_t _nextBindingDescriptorSetIndex = 0;
	size_t _nextGlobalStateBindingDescriptorSetIndex = 0;
	std::vector<IShaderCode::Resource> _shaderResourceInfo;
	VkDescriptorSetLayout _descriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorSetLayout _globalDescriptorSetLayout = VK_NULL_HANDLE;
	std::vector<VkDescriptorSetLayout> _descriptorSetLayoutArray;
	std::vector<VkDescriptorSetLayout> _globalDescriptorSetLayoutArray;
	std::vector<VkDescriptorSetLayout> _descriptorSetLayouts;
	std::vector<VkDescriptorSet> _bindingDescriptorSets;
	std::vector<VkDescriptorSet> _globalStateDescriptorSets;
	GlobalStateBufferInfo _globalStateBuffers[ShaderStageCount];
	GlobalStateBufferBuildingSession _globalStateBufferBuildingSession = {};
	int _baseBindingNo = 0;
	std::unordered_map<std::string, StateValueId> _stateNameToId;
	std::vector<ShaderInputParameterInfo> _attributeNameList;
	std::unordered_map<std::string, VertexAttributeId> _attributeNameToID;
	std::unordered_map<std::string, TextureId> _textureNameToID;
	std::unordered_map<std::string, SamplerId> _samplerNameToID;
	std::unordered_map<std::string, StateBufferId> _stateBufferNameToID;
	std::unordered_map<std::string, ResourceArrayId> _resourceBufferNameToID;
	std::vector<StateBufferInfo> _stateBuffers;
	std::vector<ResourceBufferInfo> _resourceBufferResources;
	std::vector<ShaderTextureInfo> _textureResources;
	std::vector<ShaderSamplerInfo> _samplerResources;
	std::vector<VkVertexInputBindingDescription> _vertexInputBindingDescriptions;
	std::vector<VkVertexInputAttributeDescription> _vertexInputAttributeDescriptions;
	mutable bool _builtNullDescriptorFallback = false;
	mutable std::vector<VkDescriptorBufferInfo> _nullDescriptorFallbackBufferInfo;
	mutable std::vector<VkDescriptorImageInfo> _nullDescriptorFallbackImageInfo;
	mutable std::vector<VkBufferView> _nullDescriptorFallbackBufferViews;
	mutable std::vector<VkWriteDescriptorSet> _nullDescriptorFallbackDescriptorWrites;
};

} // namespace cobalt::graphics
