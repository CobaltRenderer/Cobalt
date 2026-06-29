// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "OpenGLHeaders.h"
#include "OpenGLStateBufferLayout.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <Internal/ShaderSupport/ShaderSupport.pkg>
#include <unordered_map>
#include <vector>
namespace cobalt::graphics {
class OpenGLRenderer;
class IStateValueInfo;

class OpenGLShaderProgram : public IShaderProgram
{
public:
	// Constructors
	OpenGLShaderProgram(cobalt::logging::ILogger* log, OpenGLRenderer* renderer);
	~OpenGLShaderProgram();

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
	bool GetUniformLocation(StateValueId stateId, const size_t* arrayIndices, size_t arrayIndexCount, GLint& location) const;
	void GetBindPointForResourceArray(ResourceArrayId bufferId, GLuint& bufferBindingPoint, GLuint& counterBindingPoint, int& textureUnitNo, bool& writeable) const;
	void GetBindPointForStateBuffer(StateBufferId bufferId, GLuint& bufferBindingPoint) const;

	// State buffer methods
	SuccessToken LoadStateBufferLayoutFromShader(StateBufferId stateBufferId, IStateBufferLayout* stateBufferLayout) const override;

	// Shader binding methods
	void BindShaderProgram();

	// State value methods
	void PushGlobalConstantBufferState(const std::vector<IStateValueInfo*>* valueEntries);
	void PopGlobalConstantBufferState();
	void RestoreGlobalConstantBufferBaseline();

private:
	// Structures
	struct ShaderInputParameterInfo
	{
		std::string name;
		GLint location = {};
		GLenum nativeDataType = {};
		size_t dataSizeInBytes = 0;
	};
	struct ShaderBlockInfo
	{
		ShaderStage stage = ShaderStage(-1);
		CodeFormat format = CodeFormat(-1);
		std::vector<uint8_t> code;
		std::string entryPointName;
		std::string codeAsHLSLCached;
		std::string codeAsGLSLCached;
	};
	struct GlobalUniformInfo
	{
		std::string name;
		std::vector<std::unique_ptr<IStateValueInfo>> defaultValues;
		GLint location = {};
		IStateBufferLayout::DataType dataType = IStateBufferLayout::DataType(-1);
		uint32_t elementWidth = 0;
		uint32_t elementHeight = 0;
		GLenum nativeDataType = {};
		size_t arraySize = 0;
		std::vector<size_t> arrayIndices;
		std::vector<size_t> flattenedArrayIndices;
	};
	struct MergedGlobalUniformInfo
	{
		std::string name;
		std::vector<size_t> arraySizes;
		std::vector<size_t> underlyingUniformIndices;
		std::vector<GLint> underlyingUniformLocationsAsFlattenedArray;
	};
	struct StateBufferUniformInfo
	{
		std::string name;
		IStateBufferLayout::DataType dataType = IStateBufferLayout::DataType(-1);
		uint32_t elementWidth = 0;
		uint32_t elementHeight = 0;
		GLenum nativeDataType = {};
		size_t bufferOffset = 0;
		size_t arraySize = 0;
		size_t arrayStride = 0;
		std::vector<size_t> arrayIndices;
	};
	struct MergedStateBufferUniformInfo
	{
		std::string name;
		IStateBufferLayout::DataType dataType = IStateBufferLayout::DataType(-1);
		uint32_t elementWidth = 0;
		uint32_t elementHeight = 0;
		GLenum nativeDataType = {};
		size_t bufferOffset = 0;
		size_t arraySize = 0;
		size_t arrayStride = 0;
		std::vector<size_t> leadingArraySizes;
		std::vector<size_t> leadingArrayStrides;
		std::vector<size_t> uniformIndices;
	};
	struct StateBufferLayoutInfo
	{
		size_t uniformCount = 0;
		std::vector<GLint> uniformIndices;
		std::vector<GLint> uniformTypes;
		std::vector<GLint> uniformNameLengths;
		std::vector<GLint> uniformBlockIndexes;
		std::vector<GLint> uniformOffsets;
		std::vector<GLint> uniformArraySizes;
		std::vector<GLint> uniformArrayStrides;
		std::vector<GLint> uniformMatrixStrides;
		std::vector<GLint> uniformMatrixIsRowMajors;
		std::vector<std::string> uniformNames;
		std::vector<StateBufferUniformInfo> uniforms;
		std::vector<MergedStateBufferUniformInfo> mergedUniforms;
		std::unordered_map<std::string, size_t> uniformNameToMergedIndex;
	};
	struct ResourceBufferVariableInfo
	{
		std::string name;
		GLenum type = {};
		GLint blockIndex = 0;
		GLint bufferOffsetInBytes = 0;
		GLint arraySize = 0;
		GLint arrayStride = 0;
		GLint matrixStride = 0;
		bool isRowMajor = false;
	};
	struct ResourceBufferInfo
	{
		std::string name;
		ResourceArrayId id = ResourceArrayId::Null;
		GLuint bindingPoint = 0;
		GLint texelArrayTextureUnitNo = 0;
		bool writeableResource = false;
		GLint minimumBufferSize = 0;
		std::vector<ResourceBufferVariableInfo> variables;
		bool isCounterBuffer = false;
		std::string owningBufferName;
		bool hasCounterBuffer = false;
		ResourceArrayId counterBufferId = ResourceArrayId::Null;
	};

private:
	// Compilation methods
	bool CompileProgramInternal();
	GLenum ShaderStageToGLEnum(ShaderStage stage) const;
	bool CompileShaderBlock(ShaderBlockInfo& blockInfo, bool hasGeometryStage, GLuint& shaderID);
	void ReleaseMemory();

	// State buffer methods
	void ReadStateBufferLayoutInfoFromShader(StateBufferId stateBufferId, StateBufferLayoutInfo& layoutInfo) const;
	constexpr static bool StateBufferEntryNativeTypeToBufferLayoutType(GLint nativeType, IStateBufferLayout::DataType& type, uint32_t& elementWidth, uint32_t& elementHeight);
	std::unique_ptr<IStateValueInfo> GetGlobalUniformCurrentValue(GLenum nativeDataType, GLint location, StateValueId stateId, const size_t* arrayIndices, size_t arrayIndexCount) const;

private:
	cobalt::logging::ILogger* _log;
	OpenGLRenderer* _renderer;
	ShaderTargetInfoOpenGL _shaderTargetInfo;
	bool _shaderCompiled;
	GLuint _programID = {};
	int _baseBindingNo = 0;
	int _baseBindingNoConversionTemp = 0;
	int _baseBindingNoConversionTemp2 = 0;
	int _baseBindingNoForGlobalBufferReflection = 0;
	int _userClipPlaneCount = 0;
	int _maxUserClipPlaneCount = 0;
	std::unordered_map<ShaderStage, ShaderBlockInfo> _shaderBlocks;
	std::vector<IShaderCode::Resource> _shaderResourceInfo;
	std::vector<IShaderCode::Resource> _shaderResourceInfoConversionTemp;
	std::vector<IShaderCode::Resource> _shaderResourceInfoConversionTemp2;
	std::vector<IShaderCode::Resource> _shaderResourceInfoForGlobalBufferReflection;
	std::unordered_map<std::string, size_t> _uniformNameToMergedIndex;
	std::unordered_map<std::string, GLint> _attributeNameToID;
	std::unordered_map<std::string, GLint> _stateBufferNameToID;
	std::unordered_map<std::string, GLint> _resourceBufferNameToID;
	std::unordered_map<std::string, GLint> _textureNameToID;
	std::unordered_map<StateBufferId, std::string> _stateBufferIDToName;
	std::vector<std::pair<GLint, GLint>> _textureSamplerAssociations;
	std::vector<GlobalUniformInfo> _globalUniforms;
	std::vector<MergedGlobalUniformInfo> _mergedGlobalUniforms;
	std::unordered_map<StateBufferId, StateBufferLayoutInfo> _stateBufferLayoutInfo;
	std::vector<const std::vector<IStateValueInfo*>*> _stateBufferStack;
	std::vector<IStateValueInfo*> _uniformDefaultStateList;
	std::vector<ShaderInputParameterInfo> _attributeNameList;
	std::vector<ResourceBufferInfo> _resourceBufferList;
	std::vector<GLuint> _stateBufferBindPoints;
	uint32_t _stateBufferStackEntries = 0;
};

} // namespace cobalt::graphics
