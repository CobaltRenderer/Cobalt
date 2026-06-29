// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Direct3DHeaders.h"
#include "Direct3DStateBuffer.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <Internal/ShaderSupport/ShaderSupport.pkg>
#include <d3dcompiler.h>
#include <string>
#include <unordered_map>
#include <vector>
namespace cobalt::graphics {
class Direct3DRenderer;

class Direct3DShaderProgram : public IShaderProgram
{
public:
	// Constants
	static const uint32_t ShaderStageCount = 4;

	// Structures
	struct GlobalConstantBufferBindingInfo
	{
		bool hasBindings;
		bool bindingSet[ShaderStageCount];
		uint32_t pageNumbers[ShaderStageCount];
	};
	struct GlobalConstantBufferBuildingSession
	{
		bool buffersExist[ShaderStageCount];
		uint32_t nextPageNumbers[ShaderStageCount];
		uint32_t lastWrittenPageBlockIndex[ShaderStageCount];
		Direct3DStateBuffer* stateBuffers[ShaderStageCount];
	};
	struct ResourceArrayBindPointEntry
	{
		ShaderStage stage = {};
		UINT readOnlyBindPoint = 0xFFFFFFFF;
		UINT readWriteBindPoint = 0xFFFFFFFF;
		bool hasReadOnlyBinding = false;
		bool hasReadWriteBinding = false;
		D3D_SHADER_INPUT_TYPE type = {};
	};
	struct SamplerBindPointEntry
	{
		ShaderStage stage = {};
		UINT bindPoint = 0xFFFFFFFF;
	};
	struct TextureBindPointEntry
	{
		ShaderStage stage = {};
		UINT bindPoint = 0xFFFFFFFF;
		UINT combinedSamplerBindPoint = 0xFFFFFFFF;
	};

public:
	// Constructors
	Direct3DShaderProgram(cobalt::logging::ILogger* log, Direct3DRenderer* renderer);
	~Direct3DShaderProgram();

	// Initialization methods
	void Delete() override;

	// Code format methods
	bool IsCodeFormatSupported(CodeFormat format) const override;
	CodeFormat PreferredCodeFormat() const override;

	// Compilation methods
	SuccessToken ConfigureShaderTarget(const ShaderTargetInfoBase& shaderTargetInfo) override;
	SuccessToken LoadShaderStage(ShaderStage stage, const ShaderSourceInfoBase& shaderSourceInfo) override;
	SuccessToken CompileProgram() override;

	// Shader input methods
	std::string GetVertexAttributeName(VertexAttributeId id) const;
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

	// State buffer methods
	SuccessToken LoadStateBufferLayoutFromShader(StateBufferId stateBufferId, IStateBufferLayout* stateBufferLayout) const override;

	// Shader binding methods
	bool IsComputeShader() const;
	bool CreateInputLayout(const std::vector<D3D11_INPUT_ELEMENT_DESC>& inputDescription, ID3D11InputLayout** inputLayout);
	void BindShaderProgram(ID3D11DeviceContext1* context, bool bindLegacyConstantBuffers);
	void UnbindAllShaderResources(ID3D11DeviceContext1* context) const;
	void BindConstantBuffer(IStateBuffer* stateBuffer, StateBufferId stateBufferId, uint32_t stateBufferPageNo, ID3D11DeviceContext1* context) const;
	void UnbindConstantBuffer(IStateBuffer* stateBuffer, StateBufferId stateBufferId, uint32_t stateBufferPageNo, ID3D11DeviceContext1* context) const;
	void GetBindPointsForTexture(TextureId textureId, const TextureBindPointEntry*& bindPoints, size_t& bindPointCount) const;
	void GetBindPointsForSampler(SamplerId samplerId, const SamplerBindPointEntry*& bindPoints, size_t& bindPointCount) const;
	void GetBindPointsForResourceArray(ResourceArrayId resourceArrayId, const ResourceArrayBindPointEntry*& bindPoints, size_t& bindPointCount) const;

	// State value methods
	void UpdateStateValue(StateValueId stateId, const uint8_t* data, size_t dataSizeInBytes, const size_t* arrayIndices, size_t arrayIndexCount);
	void SetConstantValue(StateValueId stateId, const uint8_t* data, size_t dataSizeInBytes, const size_t* arrayIndices, size_t arrayIndexCount);
	void FlushDirtyGlobalConstantBuffers(ID3D11DeviceContext* context);
	void PushGlobalConstantBufferState();
	void PopGlobalConstantBufferState();
	void RestoreGlobalConstantBufferBaseline();

	// Global constant buffer session methods
	void ResetGlobalConstantBufferState();
	void BeginGlobalConstantBufferBuildingSession(GlobalConstantBufferBuildingSession& stateInfo, ID3D11DeviceContext1* context);
	void CompleteGlobalConstantBufferBuildingSession(GlobalConstantBufferBuildingSession& stateInfo, ID3D11DeviceContext1* context);
	void GenerateGlobalConstantBufferBindings(GlobalConstantBufferBuildingSession& stateInfo, GlobalConstantBufferBindingInfo& bindingInfo, ID3D11DeviceContext1* context);
	void ApplyGlobalConstantBufferBindings(const GlobalConstantBufferBindingInfo& bindingInfo, ID3D11DeviceContext1* context);

private:
	// Constants
	//##TODO## These constants duplicate ones in IShaderCode
	static constexpr const char* GlobalConstantBufferName = "$Globals";

	// Structures
	struct ShaderInputParameterInfo
	{
		std::string name;
		size_t index = 0;
		DXGI_FORMAT format = {};
	};
	struct ShaderBlockInfo
	{
		ShaderStage stage = {};
		CodeFormat format = {};
		std::vector<uint8_t> code;
		std::string entryPointName;
	};
	struct BindingPointInfo
	{
		UINT registerNo;
		UINT registerCount;
	};
	struct ConstantBufferEntryInfo
	{
		bool defined = false;
		std::string attributeName;
		StateValueId attributeID = StateValueId::Null;
		bool hasDefaultValue = false;
		std::vector<size_t> defaultValueOffsets;
		std::vector<std::vector<uint8_t>> defaultValues;
		IStateBufferLayout::DataType type = IStateBufferLayout::DataType::Null;
		size_t width = 0;
		size_t height = 0;
		std::vector<size_t> arraySizes;
		std::vector<size_t> arrayStrides;
		std::vector<size_t> leadingArraySizes;
		std::vector<size_t> leadingArrayStrides;
		size_t arraySize = 0;
		size_t arrayStrideInBytes = 0;
		size_t bufferOffset = 0;
		//##TODO## Review the usage of these fields and if they still need to be here
		size_t entrySizeInBytes = 0;
		size_t dataSizeInBytes = 0;
	};
	struct ConstantBufferInfo
	{
		std::string name;
		uint32_t totalBufferSize = 0;
		bool bindingPointPresent[ShaderStageCount] = {};
		BindingPointInfo bindingPoints[ShaderStageCount] = {};
		std::vector<ConstantBufferEntryInfo> uniformBufferEntries;
	};
	struct GlobalConstantBufferInfo
	{
		bool bufferExists = false;
		bool isDirty = true;
		Microsoft::WRL::ComPtr<ID3D11Buffer> nativeBuffer;
		std::vector<uint8_t> bufferContents;
		std::vector<std::vector<uint8_t>> bufferStack;
		uint32_t bufferStackEntries = 0;
		ConstantBufferInfo bufferInfo;
		std::unique_ptr<Direct3DStateBuffer> stateBuffer;
		uint32_t allocatedPageCount = 0;
	};
	struct ResourceBufferInfo
	{
		std::string name;
		ResourceArrayId id = {};
		std::vector<ResourceArrayBindPointEntry> bindPoints;
	};
	struct ShaderTextureInfo
	{
		std::string name;
		TextureId id = {};
		std::vector<TextureBindPointEntry> bindPoints;
	};
	struct ShaderSamplerInfo
	{
		std::string name;
		std::string combinedSamplerFullName;
		SamplerId id = {};
		std::vector<SamplerBindPointEntry> bindPoints;
	};

private:
	// Compilation methods
	bool CompileShaderBlock(ShaderBlockInfo& blockInfo, int& shaderID);
	std::string GetShaderModelVersionString(ShaderStage stage, unsigned int shaderModelVersionMajor, unsigned int shaderModelVersionMinor) const;
	void ReleaseMemory();

	// Shader input methods
	constexpr static DXGI_FORMAT GetVertexShaderInputNativeDataFormat(const D3D11_SIGNATURE_PARAMETER_DESC& paramDesc);

	// State value methods
	constexpr static uint32_t ShaderStageToIndex(ShaderStage stage);
	bool LoadConstantBufferInfoForShaderStage(ShaderStage stage, const D3D11_SHADER_DESC& shaderDescription, ID3D11ShaderReflection* shaderReflection);
	bool LoadGlobalConstantBufferInfoForShaderStage(ShaderStage stage, const D3D11_SHADER_DESC& shaderDescription, ID3D11ShaderReflection* shaderReflection, const D3D11_SHADER_BUFFER_DESC& bufferDescription, ID3D11ShaderReflectionConstantBuffer* bufferReflection);
	bool GetResourceBindingDescription(const std::string& resourceName, D3D11_SHADER_INPUT_BIND_DESC& bindingDescription, const D3D11_SHADER_DESC& shaderDescription, ID3D11ShaderReflection* shaderReflection) const;
	bool LoadConstantBufferInfo(ConstantBufferInfo& bufferInfo, std::unordered_map<std::string, StateValueId>& attributeNamesToIDs, const D3D11_SHADER_DESC& shaderDescription, ID3D11ShaderReflection* shaderReflection, const D3D11_SHADER_BUFFER_DESC& bufferDescription, ID3D11ShaderReflectionConstantBuffer* bufferReflection) const;
	bool LoadConstantBufferStructEntry(ConstantBufferInfo& bufferInfo, std::unordered_map<std::string, StateValueId>& attributeNamesToIDs, const std::string& leadingVariableName, const std::vector<size_t>& leadingArraySizes, const std::vector<size_t>& leadingArrayStrides, size_t leadingBufferOffsetInBytes, const D3D11_SHADER_TYPE_DESC& typeDescription, ID3D11ShaderReflectionType* typeReflection, const unsigned char* defaultValue) const;
	bool LoadConstantBufferVariableEntry(ConstantBufferInfo& bufferInfo, std::unordered_map<std::string, StateValueId>& attributeNamesToIDs, const std::string& variableName, const std::vector<size_t>& leadingArraySizes, const std::vector<size_t>& leadingArrayStrides, const D3D11_SHADER_TYPE_DESC& typeDescription, size_t startOffset, const unsigned char* defaultValue) const;
	bool CalculateConstantBufferStructEntrySizeInBytes(size_t& structureSizeInBytes, const D3D11_SHADER_TYPE_DESC& typeDescription, ID3D11ShaderReflectionType* typeReflection) const;
	bool StateBufferEntryNativeTypeToBufferLayoutType(const D3D11_SHADER_TYPE_DESC& typeDescription, IStateBufferLayout::DataType& type, size_t& elementWidth, size_t& elementHeight) const;

private:
	cobalt::logging::ILogger* _log;
	Direct3DRenderer* _renderer;
	ShaderTargetInfoDirect3D _shaderTargetInfo;
	int _baseBindingNo = 0;
	std::unordered_map<ShaderStage, ShaderBlockInfo> _shaderBlocks;
	bool _isComputeShader = false;
	std::vector<IShaderCode::Resource> _shaderResourceInfo;
	Microsoft::WRL::ComPtr<ID3DBlob> _vertexShaderCodeBlob;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> _vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> _pixelShader;
	Microsoft::WRL::ComPtr<ID3D11GeometryShader> _geometryShader;
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> _computeShader;
	std::vector<ShaderInputParameterInfo> _attributeNameList;
	std::unordered_map<std::string, VertexAttributeId> _attributeNameToID;
	std::vector<std::string> _attributeIDToName;
	std::unordered_map<std::string, TextureId> _textureNameToID;
	std::unordered_map<std::string, SamplerId> _samplerNameToID;
	std::unordered_map<std::string, StateValueId> _globalUniformNameToID;
	std::unordered_map<std::string, StateBufferId> _constantBufferNameToID;
	std::unordered_map<std::string, ResourceArrayId> _resourceBufferNameToID;
	std::vector<std::vector<ID3D11ShaderResourceView*>> _resourceClearArray;
	std::vector<std::vector<ID3D11SamplerState*>> _samplerClearArray;
	std::vector<ID3D11UnorderedAccessView*> _uavClearArray;
	std::vector<UINT> _uavCounterClearArray;
	GlobalConstantBufferInfo _globalConstantBuffers[ShaderStageCount];
	std::vector<ConstantBufferInfo> _constantBuffers;
	std::vector<ResourceBufferInfo> _resourceBufferResources;
	std::vector<ShaderTextureInfo> _textureResources;
	std::vector<ShaderSamplerInfo> _samplerResources;
	GlobalConstantBufferBuildingSession _globalConstantBufferBuildingSession{};
};

} // namespace cobalt::graphics
