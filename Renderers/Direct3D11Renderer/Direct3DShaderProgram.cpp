// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "Direct3DShaderProgram.h"
#include "Direct3DRenderer.h"
#include "Direct3DStateBuffer.h"
#include "Direct3DStateBufferLayout.h"
#include <Internal/ShaderSupport/ShaderSupport.pkg>
#include <Internal/RendererSupport/KnownDynamicCast.h>
#include <algorithm>
#include <cstring>
#include <limits>
#include <memory>
#include <string>
namespace cobalt::graphics {
using Microsoft::WRL::ComPtr;

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
Direct3DShaderProgram::Direct3DShaderProgram(cobalt::logging::ILogger* log, Direct3DRenderer* renderer)
: _log(log), _renderer(renderer)
{
	_resourceClearArray.resize(ShaderStageCount);
	_samplerClearArray.resize(ShaderStageCount);

	// Set our default shader target info
	ShaderTargetInfoDirect3D::Flags shaderTargetFlags = ShaderTargetInfoDirect3D::Flags::None;
	if (_renderer->DebugLoggingEnabled())
	{
		shaderTargetFlags = (ShaderTargetInfoDirect3D::Flags)((unsigned int)ShaderTargetInfoDirect3D::Flags::EnableDebugInfo | (unsigned int)ShaderTargetInfoDirect3D::Flags::SkipOptimization);
	}
	_shaderTargetInfo = ShaderTargetInfoDirect3D(5, 0, shaderTargetFlags);
}

//----------------------------------------------------------------------------------------
Direct3DShaderProgram::~Direct3DShaderProgram()
{
	ReleaseMemory();
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void Direct3DShaderProgram::Delete()
{
	_renderer->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
// Code format methods
//----------------------------------------------------------------------------------------
bool Direct3DShaderProgram::IsCodeFormatSupported(CodeFormat format) const
{
	return (format == CodeFormat::HLSL) || (format == CodeFormat::DXBC) || (format == CodeFormat::SPIRVAssembly) || (format == CodeFormat::SPIRV) || (format == CodeFormat::GLSL);
}

//----------------------------------------------------------------------------------------
Direct3DShaderProgram::CodeFormat Direct3DShaderProgram::PreferredCodeFormat() const
{
	return CodeFormat::HLSL;
}

//----------------------------------------------------------------------------------------
// Compilation methods
//----------------------------------------------------------------------------------------
SuccessToken Direct3DShaderProgram::ConfigureShaderTarget(const ShaderTargetInfoBase& shaderTargetInfo)
{
	// Resolve the structure down to its native type
	auto shaderTargetType = shaderTargetInfo.shaderTarget;
	if (shaderTargetType != ShaderTargetInfoBase::ShaderTarget::Direct3D)
	{
		_log->Error("Attempted to configure shader target using incompatible target type {0}", shaderTargetType);
		return false;
	}
	auto shaderTargetInfoResolved = reinterpret_cast<const ShaderTargetInfoDirect3D*>(&shaderTargetInfo);

	// Load the new shader target settings
	auto previousTargetShaderModelMajor = _shaderTargetInfo.targetShaderModelMajor;
	auto previousTargetShaderModelMinor = _shaderTargetInfo.targetShaderModelMinor;
	_shaderTargetInfo = *shaderTargetInfoResolved;
	if (shaderTargetInfoResolved->targetShaderModelMajor == 0)
	{
		_shaderTargetInfo.targetShaderModelMajor = previousTargetShaderModelMajor;
		_shaderTargetInfo.targetShaderModelMinor = previousTargetShaderModelMinor;
	}
	return true;
}

//----------------------------------------------------------------------------------------
SuccessToken Direct3DShaderProgram::LoadShaderStage(ShaderStage stage, const ShaderSourceInfoBase& shaderSourceInfo)
{
	// Ensure something hasn't already been added for the target shader stage
	if (_shaderBlocks.find(stage) != _shaderBlocks.end())
	{
		_log->Error("Attempted to bind shader code for stage {0} when code has already been bound.", stage);
		return false;
	}

	// Retrieve the supplied shader info
	IShaderCode::Language language = IShaderCode::Language::HLSL;
	IShaderCode::Environment sourceEnvironment = IShaderCode::Environment::General;
	const uint8_t* code = nullptr;
	size_t codeSizeInBytes = 0;
	std::string entryPointName = IShaderCode::StandardShaderEntryPointName;
	auto shaderSourceType = shaderSourceInfo.shaderType;
	if (shaderSourceType == ShaderSourceInfoBase::ShaderType::HLSL)
	{
		auto shaderSourceInfoResolved = reinterpret_cast<const ShaderSourceInfoBaseHLSL*>(&shaderSourceInfo);
		language = IShaderCode::Language::HLSL;
		code = reinterpret_cast<const uint8_t*>(shaderSourceInfoResolved->code);
		codeSizeInBytes = shaderSourceInfoResolved->codeSizeInBytes;
		if (shaderSourceInfoResolved->entryPointFunctionName != nullptr)
		{
			entryPointName = std::string(shaderSourceInfoResolved->entryPointFunctionName, shaderSourceInfoResolved->entryPointFunctionNameSizeInBytes);
		}
	}
	else if (shaderSourceType == ShaderSourceInfoBase::ShaderType::DXBC)
	{
		auto shaderSourceInfoResolved = reinterpret_cast<const ShaderSourceInfoBaseDXBC*>(&shaderSourceInfo);
		code = shaderSourceInfoResolved->code;
		codeSizeInBytes = shaderSourceInfoResolved->codeSizeInBytes;
	}
	else if (shaderSourceType == ShaderSourceInfoBase::ShaderType::SPIRVAssembly)
	{
		auto shaderSourceInfoResolved = reinterpret_cast<const ShaderSourceInfoBaseSPIRVAssembly*>(&shaderSourceInfo);
		language = IShaderCode::Language::SPIRVAssembly;
		sourceEnvironment = IShaderCode::Environment::Vulkan_11;
		code = reinterpret_cast<const uint8_t*>(shaderSourceInfoResolved->code);
		codeSizeInBytes = shaderSourceInfoResolved->codeSizeInBytes;
		if (shaderSourceInfoResolved->entryPointFunctionName != nullptr)
		{
			entryPointName = std::string(shaderSourceInfoResolved->entryPointFunctionName, shaderSourceInfoResolved->entryPointFunctionNameSizeInBytes);
		}
	}
	else if (shaderSourceType == ShaderSourceInfoBase::ShaderType::SPIRV)
	{
		auto shaderSourceInfoResolved = reinterpret_cast<const ShaderSourceInfoBaseSPIRV*>(&shaderSourceInfo);
		language = IShaderCode::Language::SPIRV;
		sourceEnvironment = IShaderCode::Environment::Vulkan_11;
		code = reinterpret_cast<const uint8_t*>(shaderSourceInfoResolved->code);
		codeSizeInBytes = shaderSourceInfoResolved->codeSizeInUnits * sizeof(*shaderSourceInfoResolved->code);
		if (shaderSourceInfoResolved->entryPointFunctionName != nullptr)
		{
			entryPointName = std::string(shaderSourceInfoResolved->entryPointFunctionName, shaderSourceInfoResolved->entryPointFunctionNameSizeInBytes);
		}
	}
	else if (shaderSourceType == ShaderSourceInfoBase::ShaderType::GLSL)
	{
		auto shaderSourceInfoResolved = reinterpret_cast<const ShaderSourceInfoBaseGLSL*>(&shaderSourceInfo);
		language = IShaderCode::Language::GLSL;
		sourceEnvironment = IShaderCode::Environment::OpenGL_43;
		code = reinterpret_cast<const uint8_t*>(shaderSourceInfoResolved->code);
		codeSizeInBytes = shaderSourceInfoResolved->codeSizeInBytes;
	}
	else if (shaderSourceType == ShaderSourceInfoBase::ShaderType::MSL)
	{
		auto shaderSourceInfoResolved = reinterpret_cast<const ShaderSourceInfoBaseMSL*>(&shaderSourceInfo);
		language = IShaderCode::Language::MSL;
		sourceEnvironment = IShaderCode::Environment::General;
		code = reinterpret_cast<const uint8_t*>(shaderSourceInfoResolved->code);
		codeSizeInBytes = shaderSourceInfoResolved->codeSizeInBytes;
		if (shaderSourceInfoResolved->entryPointFunctionName != nullptr)
		{
			entryPointName = std::string(shaderSourceInfoResolved->entryPointFunctionName, shaderSourceInfoResolved->entryPointFunctionNameSizeInBytes);
		}
	}
	else
	{
		_log->Error("Incompatible shader source of type {0} supplied for shader stage {1}.", shaderSourceType, stage);
		return false;
	}

	// Ensure that actual data has been provided for the shader code
	if ((codeSizeInBytes == 0) || (code == nullptr))
	{
		_log->Error("Attempted to provide a shader with no code for stage {0}.", stage);
		return false;
	}

	// Determine the final code format and code data to use for the shader, performing any necessary conversions.
	CodeFormat finalCodeFormat;
	std::vector<uint8_t> finalCodeBlock;
	if ((shaderSourceType == ShaderSourceInfoBase::ShaderType::HLSL) || (shaderSourceType == ShaderSourceInfoBase::ShaderType::DXBC) || (shaderSourceType == ShaderSourceInfoBase::ShaderType::DXIL))
	{
		finalCodeFormat = (shaderSourceType == ShaderSourceInfoBase::ShaderType::HLSL ? CodeFormat::HLSL : (shaderSourceType == ShaderSourceInfoBase::ShaderType::DXBC ? CodeFormat::DXBC : CodeFormat::DXIL));
		finalCodeBlock.assign(code, code + codeSizeInBytes);
	}
	else
	{
		// Map the shader stage value
		IShaderCode::Stage shaderStage = IShaderCode::Stage::Vertex;
		if (stage == ShaderStage::Vertex)
		{
			shaderStage = IShaderCode::Stage::Vertex;
		}
		else if (stage == ShaderStage::Geometry)
		{
			shaderStage = IShaderCode::Stage::Geometry;
		}
		else if (stage == ShaderStage::Fragment)
		{
			shaderStage = IShaderCode::Stage::Fragment;
		}
		else if (stage == ShaderStage::Compute)
		{
			shaderStage = IShaderCode::Stage::Compute;
		}
		else
		{
			_log->Error("Attempted to bind shader for unsupported stage {0}.", stage);
			return false;
		}

		// Attempt to convert the code to HLSL
		std::string shaderCodeAsString;
		auto shaderCode = IShaderCode::Create(_log->GetLoggerChildScope("ShaderSupport"));
		if (!shaderCode->LoadCode(language, shaderStage, sourceEnvironment, IShaderCode::Environment::General, code, codeSizeInBytes, entryPointName, _baseBindingNo, _shaderResourceInfo))
		{
			_log->Error("Failed to import shader code for stage {0}", shaderStage);
			return false;
		}
		if (!shaderCode->ExportCodeAsHLSL(shaderCodeAsString, _shaderTargetInfo.targetShaderModelMajor, _shaderTargetInfo.targetShaderModelMinor))
		{
			_log->Error("Failed to convert shader code for stage {0}", shaderStage);
			return false;
		}
		finalCodeFormat = CodeFormat::HLSL;
		finalCodeBlock.assign(reinterpret_cast<const uint8_t*>(shaderCodeAsString.data()), reinterpret_cast<const uint8_t*>(shaderCodeAsString.data()) + shaderCodeAsString.size());
	}

	// Store information on the provided shader code block
	ShaderBlockInfo& blockInfo = _shaderBlocks[stage];
	blockInfo.stage = stage;
	blockInfo.format = finalCodeFormat;
	blockInfo.code = std::move(finalCodeBlock);
	blockInfo.entryPointName = entryPointName;
	return true;
}

//----------------------------------------------------------------------------------------
SuccessToken Direct3DShaderProgram::CompileProgram()
{
	// Ensure that all required shader modules have been provided
	if (_shaderBlocks.empty())
	{
		_log->Error("Failed to compile shader as no shader blocks have been specified.");
		return false;
	}
	if (_shaderBlocks.find(ShaderStage::Compute) == _shaderBlocks.end())
	{
		ShaderStage requiredBlocks[] = {ShaderStage::Fragment, ShaderStage::Vertex};
		for (auto& requiredBlock : requiredBlocks)
		{
			if (_shaderBlocks.find(requiredBlock) == _shaderBlocks.end())
			{
				_log->Error("Failed to find required shader block of type {0} when compiling shader.", requiredBlock);
				return false;
			}
		}
	}
	else if (_shaderBlocks.size() > 1)
	{
		_log->Error("Failed to compile shader as an invalid combination of shader modules was provided.");
		return false;
	}

	// Compile the shader modules
	for (auto& shaderBlockEntry : _shaderBlocks)
	{
		ShaderBlockInfo& shaderBlock = shaderBlockEntry.second;
		int shaderID;
		if (!CompileShaderBlock(shaderBlock, shaderID))
		{
			_log->Error("Shader compilation failed for stage {0}.", shaderBlock.stage);
			return false;
		}
	}

	// Record if this is a compute shader
	_isComputeShader = (_shaderBlocks.size() == 1) && (_shaderBlocks.begin()->first == IShaderProgram::ShaderStage::Compute);
	return true;
}

//----------------------------------------------------------------------------------------
bool Direct3DShaderProgram::CompileShaderBlock(ShaderBlockInfo& blockInfo, int& shaderID)
{
	// Attempt to compile the shader program, and obtain a reflection interface for it.
	ComPtr<ID3DBlob> codeBlob;
	ComPtr<ID3D11ShaderReflection> shaderReflection;
	if (blockInfo.format == CodeFormat::DXBC)
	{
		// Directly copy already compiled code
		D3DCreateBlob(blockInfo.code.size(), &codeBlob);
		std::memcpy(codeBlob->GetBufferPointer(), blockInfo.code.data(), blockInfo.code.size());

		// Attempt to retrieve reflection info for the shader
		HRESULT d3dReflectReturn = D3DReflect(codeBlob->GetBufferPointer(), codeBlob->GetBufferSize(), IID_PPV_ARGS(&shaderReflection));
		if (FAILED(d3dReflectReturn))
		{
			_log->Error("D3DReflect failed with error code {0}", d3dReflectReturn);
			ReleaseMemory();
			return false;
		}
	}
	else
	{
		// Build the HLSL shader compiler target string
		std::string targetString = GetShaderModelVersionString(blockInfo.stage, _shaderTargetInfo.targetShaderModelMajor, _shaderTargetInfo.targetShaderModelMinor);

		// Attempt to compile the shader program
		ComPtr<ID3DBlob> errorBlob;
		UINT compileFlags = ((_shaderTargetInfo.flags & ShaderTargetInfoDirect3D::Flags::SkipOptimization) != ShaderTargetInfoDirect3D::Flags::None) ? D3DCOMPILE_SKIP_OPTIMIZATION : D3DCOMPILE_OPTIMIZATION_LEVEL3;
		if ((_shaderTargetInfo.flags & ShaderTargetInfoDirect3D::Flags::EnableDebugInfo) != ShaderTargetInfoDirect3D::Flags::None)
		{
			compileFlags |= D3DCOMPILE_DEBUG;
		}
		if (FAILED(D3DCompile(reinterpret_cast<const char*>(blockInfo.code.data()), blockInfo.code.size(), nullptr, nullptr, nullptr, blockInfo.entryPointName.c_str(), targetString.c_str(), compileFlags, NULL, &codeBlob, &errorBlob)))
		{
			_log->Info("Compilation failed - The following shader code was provided:");

			auto lineIt = blockInfo.code.begin();
			auto lineCount = 1;
			while (lineIt != blockInfo.code.end())
			{
				auto endOfLine = std::find(lineIt, blockInfo.code.end(), '\n');

				auto lineNo = std::to_string(lineCount);
				while (lineNo.size() < 3)
				{
					std::string temp = "0";
					temp += lineNo;
					lineNo.swap(temp);
				}

				_log->Info(lineNo + ": " + std::string(lineIt, endOfLine));

				if (endOfLine == blockInfo.code.end())
				{
					break;
				}

				lineIt = endOfLine + 1;
				lineCount++;
			}
			_log->Info("(End of shader code in which compilation failed)");
			if (errorBlob != nullptr)
			{
				_log->Warning("Shader compilation failed for stage {0}. {1}", blockInfo.stage, reinterpret_cast<const char*>(errorBlob->GetBufferPointer()));
			}
			else
			{
				_log->Warning("Shader compilation failed for stage {0}.", blockInfo.stage);
			}
			ReleaseMemory();
			return false;
		}
		errorBlob.Reset();

		// Attempt to retrieve reflection info for the shader
		HRESULT d3dReflectReturn = D3DReflect(codeBlob->GetBufferPointer(), codeBlob->GetBufferSize(), IID_PPV_ARGS(&shaderReflection));
		if (FAILED(d3dReflectReturn))
		{
			_log->Error("D3DReflect failed with error code {0}", d3dReflectReturn);
			ReleaseMemory();
			return false;
		}
	}

	// Get information about the shader program
	D3D11_SHADER_DESC shaderDescription;
	HRESULT getShaderDescriptionReturn = shaderReflection->GetDesc(&shaderDescription);
	if (FAILED(getShaderDescriptionReturn))
	{
		_log->Error("GetDesc failed with error code {0}", getShaderDescriptionReturn);
		ReleaseMemory();
		return false;
	}

	// Retrieve the names of all the input parameters for the vertex shader
	if (blockInfo.stage == IShaderProgram::ShaderStage::Vertex)
	{
		for (uint32_t i = 0; i < shaderDescription.InputParameters; ++i)
		{
			// Retrieve info on this input parameter
			D3D11_SIGNATURE_PARAMETER_DESC paramDesc;
			HRESULT getInputParameterDescReturn = shaderReflection->GetInputParameterDesc(i, &paramDesc);
			if (FAILED(getInputParameterDescReturn))
			{
				_log->Error("GetInputParameterDesc failed with error code {0}", getInputParameterDescReturn);
				ReleaseMemory();
				return false;
			}

			// If this is a system value input (IE, SV_VertexID), it is not backed by an input buffer, and therefore
			// does not need to be bound, so we skip it here.
			if (paramDesc.SystemValueType != D3D_NAME_UNDEFINED)
			{
				continue;
			}

			// Store details on this input parameter
			auto vertexAttributeId = (VertexAttributeId)_attributeNameList.size();
			ShaderInputParameterInfo parameterInfo;
			parameterInfo.name = paramDesc.SemanticName;
			parameterInfo.index = paramDesc.SemanticIndex;
			parameterInfo.format = GetVertexShaderInputNativeDataFormat(paramDesc);
			_attributeNameList.push_back(parameterInfo);
			_attributeNameToID.insert(std::make_pair(paramDesc.SemanticName, vertexAttributeId));
			_attributeIDToName.emplace_back(paramDesc.SemanticName);
		}
	}

	// Backwards compatabile suffix
	std::string combinedSamplerSuffix1 = IShaderCode::CombinedImageSamplerPostfix;

	// As used by glslLang -> Spirv shaders. This is hardcoded into SpirV cross and could conceivably
	// be spat out by a SpirV toolchain going through HLSL.
	std::string combinedSamplerSuffix2 = "_sampler";

	// Retrieve the names of all resource array, texture, sampler, and state buffer resources.
	std::unordered_map<std::string, ShaderSamplerInfo> combinedSamplers;
	for (uint32_t i = 0; i < shaderDescription.BoundResources; ++i)
	{
		// Retrieve the binding description for this shader resource
		D3D11_SHADER_INPUT_BIND_DESC bindingDescription;
		HRESULT getResourceBindingDescReturn = shaderReflection->GetResourceBindingDesc(i, &bindingDescription);
		if (FAILED(getResourceBindingDescReturn))
		{
			_log->Error("GetResourceBindingDesc failed with error code {0}", getResourceBindingDescReturn);
			ReleaseMemory();
			return false;
		}

		// Determine how to handle this shader resource based on its type
		if (((bindingDescription.Type == D3D_SIT_TEXTURE) && (bindingDescription.Dimension == D3D_SRV_DIMENSION_BUFFER)) || (bindingDescription.Type == D3D_SIT_UAV_RWTYPED) || (bindingDescription.Type == D3D_SIT_STRUCTURED) || (bindingDescription.Type == D3D_SIT_UAV_RWSTRUCTURED) || (bindingDescription.Type == D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER) || (bindingDescription.Type == D3D_SIT_UAV_APPEND_STRUCTURED) || (bindingDescription.Type == D3D_SIT_UAV_CONSUME_STRUCTURED) || (bindingDescription.Type == D3D_SIT_BYTEADDRESS) || (bindingDescription.Type == D3D_SIT_UAV_RWBYTEADDRESS))
		{
			// Retrieve or create an entry for this resource array
			size_t resourceBufferIndex;
			std::string resourceBufferName = bindingDescription.Name;
			auto resourceBufferNameToIDIterator = _resourceBufferNameToID.find(resourceBufferName);
			if (resourceBufferNameToIDIterator != _resourceBufferNameToID.end())
			{
				resourceBufferIndex = (size_t)resourceBufferNameToIDIterator->second;
			}
			else
			{
				resourceBufferIndex = _resourceBufferResources.size();
				ResourceBufferInfo resourceBufferInfo;
				resourceBufferInfo.name = resourceBufferName;
				resourceBufferInfo.id = (ResourceArrayId)resourceBufferIndex;
				_resourceBufferResources.push_back(resourceBufferInfo);
				_resourceBufferNameToID.insert(std::make_pair(bindingDescription.Name, resourceBufferInfo.id));
			}
			ResourceBufferInfo& resourceBufferInfo = _resourceBufferResources[resourceBufferIndex];

			// Add this binding point for the resource array
			ResourceArrayBindPointEntry bindPoint;
			bindPoint.stage = blockInfo.stage;
			bindPoint.type = bindingDescription.Type;
			bool isReadOnlyBinding = ((bindingDescription.Type == D3D_SIT_STRUCTURED) || (bindingDescription.Type == D3D_SIT_TEXTURE) || (bindingDescription.Type == D3D_SIT_BYTEADDRESS));
			if (isReadOnlyBinding)
			{
				bindPoint.readOnlyBindPoint = bindingDescription.BindPoint;
				bindPoint.hasReadOnlyBinding = true;
			}
			else
			{
				bindPoint.readWriteBindPoint = bindingDescription.BindPoint;
				bindPoint.hasReadWriteBinding = true;
			}
			resourceBufferInfo.bindPoints.push_back(bindPoint);

			// Update the clear arrays to handle this resource array ID
			if (isReadOnlyBinding)
			{
				auto& clearArray = _resourceClearArray[ShaderStageToIndex(blockInfo.stage)];
				if (bindingDescription.BindPoint >= (uint32_t)clearArray.size())
				{
					clearArray.resize(bindingDescription.BindPoint + 1, nullptr);
				}
			}
			else
			{
				if (bindingDescription.BindPoint >= (uint32_t)_uavClearArray.size())
				{
					_uavClearArray.resize(bindingDescription.BindPoint + 1, nullptr);
					_uavCounterClearArray.resize(bindingDescription.BindPoint + 1, 0);
				}
			}
		}
		else if (bindingDescription.Type == D3D_SIT_TEXTURE)
		{
			// Retrieve or create an entry for this texture
			size_t textureIndex;
			std::string textureName = bindingDescription.Name;
			auto textureNameToIDIterator = _textureNameToID.find(textureName);
			if (textureNameToIDIterator != _textureNameToID.end())
			{
				textureIndex = (size_t)textureNameToIDIterator->second;
			}
			else
			{
				textureIndex = _textureResources.size();
				ShaderTextureInfo textureInfo;
				textureInfo.name = bindingDescription.Name;
				textureInfo.id = (TextureId)_textureResources.size();
				_textureResources.push_back(textureInfo);
				_textureNameToID.insert(std::make_pair(bindingDescription.Name, textureInfo.id));
			}
			ShaderTextureInfo& textureInfo = _textureResources[textureIndex];

			// Add this binding point for the texture
			TextureBindPointEntry bindPoint;
			bindPoint.stage = blockInfo.stage;
			bindPoint.bindPoint = bindingDescription.BindPoint;
			textureInfo.bindPoints.push_back(bindPoint);

			// Update the texture clear array to handle this texture ID
			auto& clearArray = _resourceClearArray[ShaderStageToIndex(blockInfo.stage)];
			if (bindingDescription.BindPoint >= (uint32_t)clearArray.size())
			{
				clearArray.resize(bindingDescription.BindPoint + 1, nullptr);
			}
		}
		else if (bindingDescription.Type == D3D_SIT_SAMPLER)
		{
			// Determine if this is sampler resource is a combined sampler
			auto stringStartsWith = [](const std::string& str, const std::string& prefix) { return (str.size() >= prefix.size()) && (str.compare(0, prefix.size(), prefix) == 0); };
			auto stringEndsWith = [](const std::string& str, const std::string& suffix) { return (str.size() >= suffix.size()) && (str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0); };
			std::string combinedSamplerName;
			if (stringEndsWith(bindingDescription.Name, combinedSamplerSuffix1))
			{
				combinedSamplerName = bindingDescription.Name;
				combinedSamplerName.resize(combinedSamplerName.size() - combinedSamplerSuffix1.size());
			}
			else if (stringStartsWith(bindingDescription.Name, "_") && stringEndsWith(bindingDescription.Name, combinedSamplerSuffix2))
			{
				combinedSamplerName = bindingDescription.Name;
				combinedSamplerName = combinedSamplerName.substr(1, (combinedSamplerName.size() - 1) - combinedSamplerSuffix2.size());
			}

			if (!combinedSamplerName.empty())
			{
				// Retrieve or create an entry for this combined sampler
				auto& samplerInfo = combinedSamplers[combinedSamplerName];
				samplerInfo.name = combinedSamplerName;
				samplerInfo.combinedSamplerFullName = bindingDescription.Name;

				// Add this binding point for the sampler
				SamplerBindPointEntry bindPoint;
				bindPoint.stage = blockInfo.stage;
				bindPoint.bindPoint = bindingDescription.BindPoint;
				samplerInfo.bindPoints.push_back(bindPoint);
			}
			else
			{
				// Retrieve or create an entry for this sampler
				size_t samplerIndex;
				std::string samplerName = bindingDescription.Name;
				auto samplerNameToIDIterator = _samplerNameToID.find(samplerName);
				if (samplerNameToIDIterator != _samplerNameToID.end())
				{
					samplerIndex = (size_t)samplerNameToIDIterator->second;
				}
				else
				{
					samplerIndex = _samplerResources.size();
					ShaderSamplerInfo samplerInfo;
					samplerInfo.name = bindingDescription.Name;
					samplerInfo.id = (SamplerId)_samplerResources.size();
					_samplerResources.push_back(samplerInfo);
					_samplerNameToID.insert(std::make_pair(bindingDescription.Name, samplerInfo.id));
				}
				ShaderSamplerInfo& samplerInfo = _samplerResources[samplerIndex];

				// Add this binding point for the sampler
				SamplerBindPointEntry bindPoint;
				bindPoint.stage = blockInfo.stage;
				bindPoint.bindPoint = bindingDescription.BindPoint;
				samplerInfo.bindPoints.push_back(bindPoint);

				// Update the texture clear array to handle this texture ID
				auto& clearArray = _samplerClearArray[ShaderStageToIndex(blockInfo.stage)];
				if (bindingDescription.BindPoint >= (uint32_t)clearArray.size())
				{
					clearArray.resize(bindingDescription.BindPoint + 1, nullptr);
				}
			}
		}
		else
		{
			continue;
		}
	}

	// Process each located combined sampler object
	for (auto& entry : combinedSamplers)
	{
		// Locate the corresponding texture resource for this combined sampler object
		auto textureNameToIDIterator = _textureNameToID.find(entry.first);
		if (textureNameToIDIterator == _textureNameToID.end())
		{
			_log->Warning("A combined texture sampler with the name {0} was found, but no corresponding texture resource exists.", entry.second.combinedSamplerFullName);
			continue;
		}

		// Store the association between the texture resource and the corresponding combined sampler
		ShaderTextureInfo& textureInfo = _textureResources[(size_t)textureNameToIDIterator->second];
		for (auto& samplerBindPoint : entry.second.bindPoints)
		{
			for (auto& textureBindPoint : textureInfo.bindPoints)
			{
				if (textureBindPoint.stage == samplerBindPoint.stage)
				{
					textureBindPoint.combinedSamplerBindPoint = samplerBindPoint.bindPoint;
					break;
				}
			}
		}
	}

	// Load information on all constant buffers in this shader stage
	if (!LoadConstantBufferInfoForShaderStage(blockInfo.stage, shaderDescription, shaderReflection.Get()))
	{
		_log->Error("Failed to load constant buffer info for shader stage {0}", blockInfo.stage);
		ReleaseMemory();
		return false;
	}

	// Attempt to create the shader object for the shader program
	ID3D11Device* device = _renderer->GetDevice();
	if (blockInfo.stage == IShaderProgram::ShaderStage::Vertex)
	{
		// Store the code blob for the vertex shader, so that we can call CreateInputLayout at a later time.
		_vertexShaderCodeBlob = codeBlob;

		// Create the vertex shader
		HRESULT createVertexShaderReturn = device->CreateVertexShader(codeBlob->GetBufferPointer(), codeBlob->GetBufferSize(), nullptr, &_vertexShader);
		if (FAILED(createVertexShaderReturn))
		{
			_log->Error("CreateVertexShader failed with error code {0}", createVertexShaderReturn);
			ReleaseMemory();
			return false;
		}
	}
	else if (blockInfo.stage == IShaderProgram::ShaderStage::Fragment)
	{
		// Create the pixel shader
		HRESULT createPixelShaderReturn = device->CreatePixelShader(codeBlob->GetBufferPointer(), codeBlob->GetBufferSize(), nullptr, &_pixelShader);
		if (FAILED(createPixelShaderReturn))
		{
			_log->Error("CreatePixelShader failed with error code {0}", createPixelShaderReturn);
			ReleaseMemory();
			return false;
		}
	}
	else if (blockInfo.stage == IShaderProgram::ShaderStage::Geometry)
	{
		// Create the geometry shader
		HRESULT createGeometryShaderReturn = device->CreateGeometryShader(codeBlob->GetBufferPointer(), codeBlob->GetBufferSize(), nullptr, &_geometryShader);
		if (FAILED(createGeometryShaderReturn))
		{
			_log->Error("CreateGeometryShader failed with error code {0}", createGeometryShaderReturn);
			ReleaseMemory();
			return false;
		}
	}
	else if (blockInfo.stage == IShaderProgram::ShaderStage::Compute)
	{
		// Create the compute shader
		HRESULT createComputeShaderReturn = device->CreateComputeShader(codeBlob->GetBufferPointer(), codeBlob->GetBufferSize(), nullptr, &_computeShader);
		if (FAILED(createComputeShaderReturn))
		{
			_log->Error("CreateComputeShader failed with error code {0}", createComputeShaderReturn);
			ReleaseMemory();
			return false;
		}
	}
	return true;
}

//----------------------------------------------------------------------------------------
std::string Direct3DShaderProgram::GetShaderModelVersionString(ShaderStage stage, unsigned int shaderModelVersionMajor, unsigned int shaderModelVersionMinor) const
{
	std::string targetString;
	switch (stage)
	{
	case IShaderProgram::ShaderStage::Vertex:
		targetString = "vs_" + std::to_string(shaderModelVersionMajor) + "_" + std::to_string(shaderModelVersionMinor);
		break;
	case IShaderProgram::ShaderStage::Fragment:
		targetString = "ps_" + std::to_string(shaderModelVersionMajor) + "_" + std::to_string(shaderModelVersionMinor);
		break;
	case IShaderProgram::ShaderStage::Geometry:
		targetString = "gs_" + std::to_string(shaderModelVersionMajor) + "_" + std::to_string(shaderModelVersionMinor);
		break;
	case IShaderProgram::ShaderStage::Compute:
		targetString = "cs_" + std::to_string(shaderModelVersionMajor) + "_" + std::to_string(shaderModelVersionMinor);
		break;
	}
	return targetString;
}

//----------------------------------------------------------------------------------------
constexpr uint32_t Direct3DShaderProgram::ShaderStageToIndex(ShaderStage stage)
{
	switch (stage)
	{
	case IShaderProgram::ShaderStage::Vertex:
		return 0;
	case IShaderProgram::ShaderStage::Fragment:
		return 1;
	case IShaderProgram::ShaderStage::Geometry:
		return 2;
	case IShaderProgram::ShaderStage::Compute:
		return 3;
	}
	UNREACHABLE();
	return {};
}

//----------------------------------------------------------------------------------------
bool Direct3DShaderProgram::LoadConstantBufferInfoForShaderStage(ShaderStage stage, const D3D11_SHADER_DESC& shaderDescription, ID3D11ShaderReflection* shaderReflection)
{
	// Load information on each constant buffer defined in the target shader stage
	auto shaderStageIndex = ShaderStageToIndex(stage);
	for (uint32_t i = 0; i < shaderDescription.ConstantBuffers; ++i)
	{
		// Attempt to retrieve information on the target constant buffer
		ID3D11ShaderReflectionConstantBuffer* bufferReflection = shaderReflection->GetConstantBufferByIndex(i);
		D3D11_SHADER_BUFFER_DESC bufferDescription;
		HRESULT getBufferDescriptionReturn = bufferReflection->GetDesc(&bufferDescription);
		if (FAILED(getBufferDescriptionReturn))
		{
			_log->Error("Failed to load constant buffer info for buffer with index {0}", i);
			return false;
		}

		// If this buffer is the global constant buffer for this shader stage, handle it separately, and advance to the
		// next constant buffer.
		std::string bufferName = bufferDescription.Name;
		if (bufferName == GlobalConstantBufferName)
		{
			if (!LoadGlobalConstantBufferInfoForShaderStage(stage, shaderDescription, shaderReflection, bufferDescription, bufferReflection))
			{
				_log->Error("Failed to load global constant buffer for shader stage {0}", stage);
				return false;
			}
			continue;
		}

		// Load the binding description for the target constant buffer
		D3D11_SHADER_INPUT_BIND_DESC bindingDescription;
		if (!GetResourceBindingDescription(bufferName, bindingDescription, shaderDescription, shaderReflection))
		{
			_log->Warning("Failed to locate binding point for constant buffer \"{0}\"", bufferName);
			return false;
		}

		// If this resource isn't of the target type, skip it.
		if (bindingDescription.Type != D3D_SIT_CBUFFER)
		{
			continue;
		}

		// Build the binding point info for the target constant buffer
		BindingPointInfo bindingPoint = {};
		bindingPoint.registerNo = bindingDescription.BindPoint;
		bindingPoint.registerCount = bindingDescription.BindCount;

		// If this constant buffer has already been added, set our binding point for the buffer, then advance to the
		// next constant buffer in the target shader stage.
		const auto& constantBufferNameToIDIterator = _constantBufferNameToID.find(bufferName);
		if (constantBufferNameToIDIterator != _constantBufferNameToID.end())
		{
			ConstantBufferInfo& bufferInfo = _constantBuffers[(size_t)constantBufferNameToIDIterator->second];
			bufferInfo.bindingPoints[shaderStageIndex] = bindingPoint;
			bufferInfo.bindingPointPresent[shaderStageIndex] = true;
			continue;
		}

		// Retrieve information on the target constant buffer
		ConstantBufferInfo bufferInfo;
		bufferInfo.bindingPoints[shaderStageIndex] = bindingPoint;
		bufferInfo.bindingPointPresent[shaderStageIndex] = true;
		std::unordered_map<std::string, StateValueId> attributeNamesToIDs;
		if (!LoadConstantBufferInfo(bufferInfo, attributeNamesToIDs, shaderDescription, shaderReflection, bufferDescription, bufferReflection))
		{
			_log->Error("Failed to load constant buffer info for buffer with name {0}", bufferName);
			return false;
		}

		// Add this constant buffer to the set of constant buffers for this shader program
		auto constantBufferID = (StateBufferId)_constantBuffers.size();
		_constantBufferNameToID.insert(std::make_pair(bufferName, constantBufferID));
		_constantBuffers.push_back(std::move(bufferInfo));
	}
	return true;
}

//----------------------------------------------------------------------------------------
bool Direct3DShaderProgram::LoadGlobalConstantBufferInfoForShaderStage(ShaderStage stage, const D3D11_SHADER_DESC& shaderDescription, ID3D11ShaderReflection* shaderReflection, const D3D11_SHADER_BUFFER_DESC& bufferDescription, ID3D11ShaderReflectionConstantBuffer* bufferReflection)
{
	// Load information on the global constant buffer for the shader
	auto shaderStageIndex = ShaderStageToIndex(stage);
	GlobalConstantBufferInfo& constantBufferInfo = _globalConstantBuffers[shaderStageIndex];
	if (!LoadConstantBufferInfo(constantBufferInfo.bufferInfo, _globalUniformNameToID, shaderDescription, shaderReflection, bufferDescription, bufferReflection))
	{
		_log->Error("Failed to load global constant buffer info for shader stage {0}", stage);
		return false;
	}

	// Load the binding description for the target constant buffer
	D3D11_SHADER_INPUT_BIND_DESC bindingDescription;
	if (!GetResourceBindingDescription(constantBufferInfo.bufferInfo.name, bindingDescription, shaderDescription, shaderReflection) || (bindingDescription.Type != D3D_SIT_CBUFFER))
	{
		_log->Warning("Failed to locate binding point for constant buffer \"{0}\"", constantBufferInfo.bufferInfo.name);
		return false;
	}

	// Build the binding point info for the target constant buffer
	BindingPointInfo bindingPoint = {};
	bindingPoint.registerNo = bindingDescription.BindPoint;
	bindingPoint.registerCount = bindingDescription.BindCount;
	constantBufferInfo.bufferInfo.bindingPoints[shaderStageIndex] = bindingPoint;
	constantBufferInfo.bufferInfo.bindingPointPresent[shaderStageIndex] = true;

	// Ensure each shader stage "view" of the global uniform buffer has the same set of uniform buffer entries, even if
	// they're empty for some stages. We do this to improve efficiency of writes later on.
	ConstantBufferEntryInfo emptyBufferEntry;
	emptyBufferEntry.defined = false;
	for (auto& bufferInfo : _globalConstantBuffers)
	{
		bufferInfo.bufferInfo.uniformBufferEntries.resize(_globalUniformNameToID.size(), emptyBufferEntry);
	}

	// Create the physical constant buffer
	D3D11_BUFFER_DESC physicalBufferDescription = {};
	physicalBufferDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	physicalBufferDescription.Usage = D3D11_USAGE_DEFAULT;
	physicalBufferDescription.CPUAccessFlags = 0;
	physicalBufferDescription.MiscFlags = 0;
	physicalBufferDescription.ByteWidth = constantBufferInfo.bufferInfo.totalBufferSize;
	HRESULT createGlobalConstantBufferReturn = _renderer->GetDevice()->CreateBuffer(&physicalBufferDescription, nullptr, &constantBufferInfo.nativeBuffer);
	if (FAILED(createGlobalConstantBufferReturn))
	{
		_log->Error("CreateBuffer failed in LoadGlobalConstantBufferInfoForShaderStage with error code {0}", createGlobalConstantBufferReturn);
		return false;
	}

	// Resize our local mirror of the constant buffer to match the constant buffer size
	constantBufferInfo.bufferContents.resize(constantBufferInfo.bufferInfo.totalBufferSize, 0);

	// Load the default values into our constant buffer
	for (uint32_t i = 0; i < constantBufferInfo.bufferInfo.uniformBufferEntries.size(); ++i)
	{
		auto& entryInfo = constantBufferInfo.bufferInfo.uniformBufferEntries[i];
		if (entryInfo.defined && entryInfo.hasDefaultValue)
		{
			for (size_t j = 0; j < entryInfo.defaultValues.size(); ++j)
			{
				std::memcpy(constantBufferInfo.bufferContents.data() + entryInfo.defaultValueOffsets[j], entryInfo.defaultValues[j].data(), entryInfo.dataSizeInBytes);
			}
		}
	}

	// Push the initial buffer state to the stack
	constantBufferInfo.bufferStack.push_back(constantBufferInfo.bufferContents);
	constantBufferInfo.bufferStackEntries = 1;

	// Allocate a new state buffer to handle efficient updates to global constant state
	uint32_t allocatedPageCount = 1;
	std::unique_ptr<Direct3DStateBuffer> stateBuffer = std::make_unique<Direct3DStateBuffer>(_log, _renderer, true);
	stateBuffer->SetManualPageSize(constantBufferInfo.bufferInfo.totalBufferSize);
	stateBuffer->SetPageSettings(allocatedPageCount, true);
	if (!stateBuffer->AllocateMemory())
	{
		_log->Error("Failed to allocate global constant buffer for shader stage");
		return false;
	}
	constantBufferInfo.allocatedPageCount = 0;
	constantBufferInfo.stateBuffer = std::move(stateBuffer);

	// Flag that the buffer exists, and return true to the caller.
	constantBufferInfo.bufferExists = true;
	return true;
}

//----------------------------------------------------------------------------------------
bool Direct3DShaderProgram::GetResourceBindingDescription(const std::string& resourceName, D3D11_SHADER_INPUT_BIND_DESC& bindingDescription, const D3D11_SHADER_DESC& shaderDescription, ID3D11ShaderReflection* shaderReflection) const
{
	// Attempt to locate the binding description for the target resource
	for (uint32_t i = 0; i < shaderDescription.BoundResources; ++i)
	{
		// Retrieve the binding description for this resource
		D3D11_SHADER_INPUT_BIND_DESC resourceBindingDesc;
		HRESULT getResourceBindingDescReturn = shaderReflection->GetResourceBindingDesc(i, &resourceBindingDesc);
		if (FAILED(getResourceBindingDescReturn))
		{
			_log->Warning("GetResourceBindingDesc failed in GetConstantBufferBindingPoint with error code {0}", getResourceBindingDescReturn);
			continue;
		}

		// If this is the target resource, return it to the caller.
		if (resourceBindingDesc.Name == resourceName)
		{
			bindingDescription = resourceBindingDesc;
			return true;
		}
	}
	_log->Warning("Failed to locate binding description for resource \"{0}\"", resourceName);
	return false;
}

//----------------------------------------------------------------------------------------
bool Direct3DShaderProgram::LoadConstantBufferInfo(ConstantBufferInfo& bufferInfo, std::unordered_map<std::string, StateValueId>& attributeNamesToIDs, const D3D11_SHADER_DESC& shaderDescription, ID3D11ShaderReflection* shaderReflection, const D3D11_SHADER_BUFFER_DESC& bufferDescription, ID3D11ShaderReflectionConstantBuffer* bufferReflection) const
{
	// Retrieve basic information on the constant buffer
	std::string constantBufferName = bufferDescription.Name;
	bufferInfo.name = constantBufferName;
	bufferInfo.totalBufferSize = bufferDescription.Size;

	// Load information on each variable in the constant buffer
	ConstantBufferEntryInfo emptyBufferEntry;
	emptyBufferEntry.defined = false;
	for (uint32_t i = 0; i < bufferDescription.Variables; ++i)
	{
		// Retrieve information on the target constant buffer variable
		ID3D11ShaderReflectionVariable* variableReflection = bufferReflection->GetVariableByIndex(i);
		D3D11_SHADER_VARIABLE_DESC variableDesc;
		HRESULT getVariableDescriptionReturn = variableReflection->GetDesc(&variableDesc);
		if (FAILED(getVariableDescriptionReturn))
		{
			_log->Error("GetDesc on constant variable failed with error code {0}", getVariableDescriptionReturn);
			return false;
		}

		// Obtain reflection information on the variable type
		ID3D11ShaderReflectionType* typeReflection = variableReflection->GetType();
		D3D11_SHADER_TYPE_DESC typeDescription;
		HRESULT getTypeDescriptionReturn = typeReflection->GetDesc(&typeDescription);
		if (FAILED(getTypeDescriptionReturn))
		{
			_log->Error("GetDesc on constant variable type failed with error code {0}", getTypeDescriptionReturn);
			return false;
		}

		// Add this member to the list of members in the buffer, recursing into nested structures.
		std::vector<size_t> arraySizes;
		std::vector<size_t> arrayStrides;
		if (typeDescription.Class == D3D_SVC_STRUCT)
		{
			if (!LoadConstantBufferStructEntry(bufferInfo, attributeNamesToIDs, variableDesc.Name, arraySizes, arrayStrides, (size_t)variableDesc.StartOffset, typeDescription, typeReflection, reinterpret_cast<const unsigned char*>(variableDesc.DefaultValue)))
			{
				_log->Error("LoadConstantBufferStructEntry failed");
				return false;
			}
		}
		else
		{
			std::string variableName = std::string(variableDesc.Name) + (typeDescription.Elements > 0 ? "[]" : "");
			if (!LoadConstantBufferVariableEntry(bufferInfo, attributeNamesToIDs, variableName, arraySizes, arrayStrides, typeDescription, (size_t)variableDesc.StartOffset, reinterpret_cast<const unsigned char*>(variableDesc.DefaultValue)))
			{
				_log->Error("LoadConstantBufferVariableEntry failed");
				return false;
			}
		}
	}
	return true;
}

//----------------------------------------------------------------------------------------
bool Direct3DShaderProgram::LoadConstantBufferStructEntry(ConstantBufferInfo& bufferInfo, std::unordered_map<std::string, StateValueId>& attributeNamesToIDs, const std::string& leadingVariableName, const std::vector<size_t>& leadingArraySizes, const std::vector<size_t>& leadingArrayStrides, size_t leadingBufferOffsetInBytes, const D3D11_SHADER_TYPE_DESC& typeDescription, ID3D11ShaderReflectionType* typeReflection, const unsigned char* defaultValue) const
{
	// Build array size and stride arrays to pass to our members
	auto arraySizes = leadingArraySizes;
	auto arrayStrides = leadingArrayStrides;
	bool isArray = (typeDescription.Elements > 0);
	if (isArray)
	{
		// Do a pre-pass of the members of this structure to calculate the total structure size in bytes. We need to do
		// this so we can calculate array strides.
		size_t structureSizeInBytes = 0;
		if (!CalculateConstantBufferStructEntrySizeInBytes(structureSizeInBytes, typeDescription, typeReflection))
		{
			_log->Error("CalculateConstantBufferStructEntrySizeInBytes failed");
			return false;
		}

		// Pad out the size of the structure to the end of the row to calculate the array stride
		auto int32SizeInBytes = Direct3DStateBufferLayout::GetDataTypeByteSize(IStateBufferLayout::DataType::Int32);
		structureSizeInBytes = ((structureSizeInBytes + (int32SizeInBytes * 3)) / (int32SizeInBytes * 4) * (int32SizeInBytes * 4));

		// Add the array size and stride to the dimensions passed to our members
		arraySizes.push_back(typeDescription.Elements);
		arrayStrides.push_back(structureSizeInBytes);
	}

	// Add each member of the structure to the list of members in the containing constant buffer
	for (unsigned int memberNo = 0; memberNo < typeDescription.Members; ++memberNo)
	{
		// Retrieve information on the struct member variable within the constant buffer
		ID3D11ShaderReflectionType* memberTypeReflection = typeReflection->GetMemberTypeByIndex(memberNo);

		// Obtain reflection information on struct member variable type within the constant buffer
		D3D11_SHADER_TYPE_DESC memberTypeDescription;
		HRESULT getMemberTypeDescriptionReturn = memberTypeReflection->GetDesc(&memberTypeDescription);
		if (FAILED(getMemberTypeDescriptionReturn))
		{
			_log->Error("GetDesc on constant variable member type failed with error code {0}", getMemberTypeDescriptionReturn);
			return false;
		}

		// Calculate the byte offset of this structure within the parent buffer. If the structure is an array, this will
		// be the offset to the first member of the array.
		size_t bufferOffsetInBytes = leadingBufferOffsetInBytes + (size_t)memberTypeDescription.Offset;

		// Generate the name of this member
		std::string variableName = leadingVariableName + (isArray ? ("[].") : ".") + typeReflection->GetMemberTypeName(memberNo);

		// Calculate the byte location of the default value for this member within the default value buffer
		const unsigned char* defaultValueForMember = nullptr;
		if (defaultValue != nullptr)
		{
			defaultValueForMember = defaultValue + (size_t)memberTypeDescription.Offset;
		}

		// Add the members of this field to the list of members in the constant buffer, recursing into structures as
		// required.
		if (memberTypeDescription.Class == D3D_SVC_STRUCT)
		{
			// Recurse into the nested structure
			if (!LoadConstantBufferStructEntry(bufferInfo, attributeNamesToIDs, variableName, arraySizes, arrayStrides, bufferOffsetInBytes, memberTypeDescription, memberTypeReflection, defaultValueForMember))
			{
				_log->Error("LoadConstantBufferStructEntry failed");
				return false;
			}
		}
		else
		{
			// Record information on this constant buffer field
			variableName += (memberTypeDescription.Elements > 0 ? "[]" : "");
			if (!LoadConstantBufferVariableEntry(bufferInfo, attributeNamesToIDs, variableName, arraySizes, arrayStrides, memberTypeDescription, bufferOffsetInBytes, defaultValueForMember))
			{
				_log->Error("LoadConstantBufferVariableEntry failed");
				return false;
			}
		}
	}
	return true;
}

//----------------------------------------------------------------------------------------
bool Direct3DShaderProgram::LoadConstantBufferVariableEntry(ConstantBufferInfo& bufferInfo, std::unordered_map<std::string, StateValueId>& attributeNamesToIDs, const std::string& variableName, const std::vector<size_t>& leadingArraySizes, const std::vector<size_t>& leadingArrayStrides, const D3D11_SHADER_TYPE_DESC& typeDescription, size_t startOffset, const unsigned char* defaultValue) const
{
	// Decode the type information for the field
	IStateBufferLayout::DataType type;
	size_t width;
	size_t height;
	if (!StateBufferEntryNativeTypeToBufferLayoutType(typeDescription, type, width, height))
	{
		_log->Error("State buffer contains an unsupported state value of class {0} and type {1} when loading state buffer layout", typeDescription.Class, typeDescription.Type);
		return false;
	}

	// Calculate the size of this field in bytes
	bool memberIsArray = (typeDescription.Elements > 0);
	size_t memberElementSizeInBytes = Direct3DStateBufferLayout::GetDataTypeByteSize(type);
	size_t memberSizeInBytes = 0;
	if (memberIsArray)
	{
		memberSizeInBytes = ((4 * memberElementSizeInBytes) * (height <= 0 ? 1 : height)) * typeDescription.Elements;
	}
	else
	{
		memberSizeInBytes = ((width <= 0 ? 1 : width) * memberElementSizeInBytes) * (height <= 0 ? 1 : height);
	}

	// Allocate or retrieve the uniform ID for the variable
	int uniformID;
	auto attributeNameIterator = attributeNamesToIDs.find(variableName);
	if (attributeNameIterator != attributeNamesToIDs.end())
	{
		uniformID = (int)attributeNameIterator->second;
	}
	else
	{
		uniformID = (int)attributeNamesToIDs.size();
		attributeNamesToIDs.insert(std::make_pair(std::string(variableName), StateValueId(uniformID)));
	}

	// Create a new uniform value entry for this variable
	if (uniformID >= (int)bufferInfo.uniformBufferEntries.size())
	{
		ConstantBufferEntryInfo emptyBufferEntry;
		emptyBufferEntry.defined = false;
		bufferInfo.uniformBufferEntries.resize(uniformID + 1, emptyBufferEntry);
	}
	ConstantBufferEntryInfo& entryInfo = bufferInfo.uniformBufferEntries[uniformID];
	entryInfo.defined = true;
	entryInfo.attributeID = (StateValueId)uniformID;
	entryInfo.attributeName = variableName;
	entryInfo.dataSizeInBytes = memberSizeInBytes;
	entryInfo.bufferOffset = startOffset;
	entryInfo.type = type;
	entryInfo.width = width;
	entryInfo.height = height;
	//##FIX## This is actually incorrect due to padding, but the rules are non-trivial for combining vector/matrix types
	//and array notation, and we haven't done that here. We use the actual byte size reported by reflection, and allow
	//writes to extend into padding, rather than restricting them. We should tighten this up here, and restrict the
	//entry size to be the region which is allowed to hold valid data, stripping off trailing padding. Note also that
	//there may not be any trailing padding on the last element in an array, such as when there is no following field in
	//the buffer. To handle this case and calculate the entry size correctly, we therefore need to round the reported
	//data size up to the next row boundary before using it to calculate the array entry size.
	size_t rowSizeInBytes = Direct3DStateBufferLayout::GetDataTypeByteSize(IStateBufferLayout::DataType::Float32) * 4;
	entryInfo.entrySizeInBytes = (typeDescription.Elements > 0) ? ((((entryInfo.dataSizeInBytes + (rowSizeInBytes - 1)) / rowSizeInBytes) * rowSizeInBytes) / typeDescription.Elements) : entryInfo.dataSizeInBytes;

	// Calculate the array size and stride. The alignment rules here follow the Direct3D packing rules as defined here:
	// https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/dx-graphics-hlsl-packing-rules
	if (typeDescription.Elements > 0)
	{
		//##FIX## Note that array stride may differ from true entry size due to padding. This only works currently
		//because we have not stripped the padding out of the element size calculation above. When we do strip padding,
		//this code needs to be changed.
		entryInfo.arrayStrideInBytes = entryInfo.entrySizeInBytes;
		entryInfo.arraySize = typeDescription.Elements;
	}
	else
	{
		entryInfo.arrayStrideInBytes = 0;
		entryInfo.arraySize = 1;
	}

	// Populate the array sizes and strides for this entry
	entryInfo.leadingArraySizes = leadingArraySizes;
	entryInfo.leadingArrayStrides = leadingArrayStrides;
	entryInfo.arraySizes = leadingArraySizes;
	entryInfo.arrayStrides = leadingArrayStrides;
	if (typeDescription.Elements > 0)
	{
		entryInfo.arraySizes.push_back(entryInfo.arraySize);
		entryInfo.arrayStrides.push_back(entryInfo.arrayStrideInBytes);
	}

	// Sanity check for the location and size of the uniform value in the buffer
	//##FIX## This doesn't take the array dimensions into account
	assert((startOffset + memberSizeInBytes) <= bufferInfo.totalBufferSize);

	// Load the default value for the uniform into the uniform entry information
	entryInfo.hasDefaultValue = (defaultValue != nullptr);
	if (entryInfo.hasDefaultValue)
	{
		// Store the default uniform value in its entry structure
		if (entryInfo.arraySizes.empty())
		{
			auto& defaultValueByteArray = entryInfo.defaultValues.emplace_back();
			defaultValueByteArray.resize(entryInfo.dataSizeInBytes);
			std::memcpy(defaultValueByteArray.data(), defaultValue, entryInfo.dataSizeInBytes);
			entryInfo.defaultValueOffsets.push_back(entryInfo.bufferOffset);
		}
		else
		{
			size_t arrayDimensions = entryInfo.arraySizes.size();
			std::vector<size_t> arrayIndices(arrayDimensions, 0);
			bool done = false;
			do
			{
				// Calculate the next array offset
				size_t arrayOffset = 0;
				for (size_t i = 0; i < arrayDimensions; ++i)
				{
					arrayOffset += arrayIndices[i] * entryInfo.arrayStrides[i];
				}

				// Add a default value entry for this array index
				auto& defaultValueByteArray = entryInfo.defaultValues.emplace_back();
				defaultValueByteArray.resize(entryInfo.dataSizeInBytes);
				std::memcpy(defaultValueByteArray.data(), defaultValue + arrayOffset, entryInfo.dataSizeInBytes);
				entryInfo.defaultValueOffsets.push_back(entryInfo.bufferOffset + arrayOffset);

				// Advance to the next array index
				size_t arrayDimensionIndex = arrayDimensions - 1;
				++arrayIndices[arrayDimensionIndex];
				while (arrayIndices[arrayDimensionIndex] >= entryInfo.arraySizes[arrayDimensionIndex])
				{
					arrayIndices[arrayDimensionIndex] = 0;
					if (arrayDimensionIndex-- == 0)
					{
						done = true;
						break;
					}
					++arrayIndices[arrayDimensionIndex];
				}
			} while (!done);
		}
	}
	return true;
}

//----------------------------------------------------------------------------------------
bool Direct3DShaderProgram::CalculateConstantBufferStructEntrySizeInBytes(size_t& structureSizeInBytes, const D3D11_SHADER_TYPE_DESC& typeDescription, ID3D11ShaderReflectionType* typeReflection) const
{
	// Initialize the size of the structure to 0
	structureSizeInBytes = 0;

	// Calculate the combined size of this structure based on its members
	for (unsigned int memberNo = 0; memberNo < typeDescription.Members; ++memberNo)
	{
		// Retrieve information on the struct member variable within the constant buffer
		ID3D11ShaderReflectionType* memberTypeReflection = typeReflection->GetMemberTypeByIndex(memberNo);

		// Obtain reflection information on struct member variable type within the constant buffer
		D3D11_SHADER_TYPE_DESC memberTypeDescription;
		HRESULT getMemberTypeDescriptionReturn = memberTypeReflection->GetDesc(&memberTypeDescription);
		if (FAILED(getMemberTypeDescriptionReturn))
		{
			_log->Error("GetDesc on constant variable member type failed with error code {0}", getMemberTypeDescriptionReturn);
			return false;
		}

		// Add the size of this member to the total size of the structure
		size_t memberSizeInBytes = 0;
		if (memberTypeDescription.Class == D3D_SVC_STRUCT)
		{
			// Recurse into the nested structure
			if (!CalculateConstantBufferStructEntrySizeInBytes(memberSizeInBytes, memberTypeDescription, memberTypeReflection))
			{
				_log->Error("CalculateConstantBufferStructEntrySizeInBytes failed");
				return false;
			}

			// If the nested structure is an array, calculate its size in bytes, without including trailing padding.
			if (memberTypeDescription.Elements > 0)
			{
				auto int32SizeInBytes = Direct3DStateBufferLayout::GetDataTypeByteSize(IStateBufferLayout::DataType::Int32);
				size_t paddedMemberSizeInBytes = ((memberSizeInBytes + (int32SizeInBytes * 3)) / (int32SizeInBytes * 4) * (int32SizeInBytes * 4));
				memberSizeInBytes = (paddedMemberSizeInBytes * (memberTypeDescription.Elements - 1)) + memberSizeInBytes;
			}
		}
		else
		{
			// Decode the type information for the field
			IStateBufferLayout::DataType type;
			size_t width;
			size_t height;
			if (!StateBufferEntryNativeTypeToBufferLayoutType(memberTypeDescription, type, width, height))
			{
				_log->Error("State buffer contains an unsupported state value of class {0} and type {1} when loading state buffer layout", typeDescription.Class, typeDescription.Type);
				return false;
			}

			// Calculate the size of this field in bytes
			bool memberIsArray = (memberTypeDescription.Elements > 0);
			size_t memberElementSizeInBytes = Direct3DStateBufferLayout::GetDataTypeByteSize(type);
			if (memberIsArray)
			{
				memberSizeInBytes = ((4 * memberElementSizeInBytes) * (height <= 0 ? 1 : height)) * memberTypeDescription.Elements;
			}
			else
			{
				memberSizeInBytes = ((width <= 0 ? 1 : width) * memberElementSizeInBytes) * (height <= 0 ? 1 : height);
			}
		}

		// Add the size of this member to the size of the total structure
		structureSizeInBytes = std::max(structureSizeInBytes, memberTypeDescription.Offset + memberSizeInBytes);
	}
	return true;
}

//----------------------------------------------------------------------------------------
bool Direct3DShaderProgram::StateBufferEntryNativeTypeToBufferLayoutType(const D3D11_SHADER_TYPE_DESC& typeDescription, IStateBufferLayout::DataType& type, size_t& elementWidth, size_t& elementHeight) const
{
	// Determine the class of the data type
	switch (typeDescription.Class)
	{
	case D3D_SVC_SCALAR:
		elementWidth = 0;
		elementHeight = 0;
		break;
	case D3D_SVC_VECTOR:
		elementWidth = typeDescription.Columns;
		elementHeight = 0;
		break;
	case D3D_SVC_MATRIX_COLUMNS:
	case D3D_SVC_MATRIX_ROWS:
		elementWidth = typeDescription.Rows;
		elementHeight = typeDescription.Columns;
		break;
	default:
		return false;
	}

	// Determine the underlying element data type
	switch (typeDescription.Type)
	{
	case D3D_SVT_BOOL:
		type = IStateBufferLayout::DataType::Boolean;
		break;
	case D3D_SVT_INT:
		type = IStateBufferLayout::DataType::Int32;
		break;
	case D3D_SVT_UINT:
		type = IStateBufferLayout::DataType::UInt32;
		break;
	case D3D_SVT_FLOAT:
		type = IStateBufferLayout::DataType::Float32;
		break;
	case D3D_SVT_DOUBLE:
		type = IStateBufferLayout::DataType::Float64;
		break;
	default:
		return false;
	}
	return true;
}

//----------------------------------------------------------------------------------------
void Direct3DShaderProgram::ReleaseMemory()
{
	_vertexShader.Reset();
	_pixelShader.Reset();
	_geometryShader.Reset();
	_computeShader.Reset();
	_vertexShaderCodeBlob.Reset();
	_globalUniformNameToID.clear();
	_attributeNameList.clear();
	_attributeNameToID.clear();
	_attributeIDToName.clear();
	_textureNameToID.clear();
	_samplerNameToID.clear();
	_globalUniformNameToID.clear();
	_constantBufferNameToID.clear();
	_resourceBufferNameToID.clear();
	_constantBuffers.clear();
	for (auto& bufferInfo : _globalConstantBuffers)
	{
		if (!bufferInfo.bufferExists)
		{
			continue;
		}
		bufferInfo.nativeBuffer.Reset();
		bufferInfo.stateBuffer.reset();
	}
}

//----------------------------------------------------------------------------------------
// Shader input methods
//----------------------------------------------------------------------------------------
std::string Direct3DShaderProgram::GetVertexAttributeName(VertexAttributeId id) const
{
	if ((size_t)id >= _attributeIDToName.size())
	{
		_log->Warning("Failed to locate vertex attribute with ID \"{0}\"", id);
		return "";
	}
	return _attributeIDToName[(size_t)id];
}

//----------------------------------------------------------------------------------------
bool Direct3DShaderProgram::VertexAttributeExists(const Marshal::In<std::string>& name) const
{
	return (_attributeNameToID.find(name) != _attributeNameToID.end());
}

//----------------------------------------------------------------------------------------
bool Direct3DShaderProgram::StateValueExists(const Marshal::In<std::string>& name) const
{
	return (_globalUniformNameToID.find(name) != _globalUniformNameToID.end());
}

//----------------------------------------------------------------------------------------
bool Direct3DShaderProgram::TextureExists(const Marshal::In<std::string>& name) const
{
	return (_textureNameToID.find(name) != _textureNameToID.end());
}

//----------------------------------------------------------------------------------------
bool Direct3DShaderProgram::SamplerExists(const Marshal::In<std::string>& name) const
{
	return (_samplerNameToID.find(name) != _samplerNameToID.end());
}

//----------------------------------------------------------------------------------------
bool Direct3DShaderProgram::StateBufferExists(const Marshal::In<std::string>& name) const
{
	return (_constantBufferNameToID.find(name) != _constantBufferNameToID.end());
}

//----------------------------------------------------------------------------------------
bool Direct3DShaderProgram::ResourceArrayExists(const Marshal::In<std::string>& name) const
{
	return (_resourceBufferNameToID.find(name) != _resourceBufferNameToID.end());
}

//----------------------------------------------------------------------------------------
VertexAttributeId Direct3DShaderProgram::GetVertexAttributeId(const Marshal::In<std::string>& name) const
{
	auto attributeNameToIDIterator = _attributeNameToID.find(name);
	if (attributeNameToIDIterator == _attributeNameToID.end())
	{
		_log->Warning("Failed to locate vertex attribute with name \"{0}\"", name.Get());
		return VertexAttributeId::Null;
	}
	return attributeNameToIDIterator->second;
}

//----------------------------------------------------------------------------------------
StateValueId Direct3DShaderProgram::GetStateValueId(const Marshal::In<std::string>& name) const
{
	auto uniformNameToIDIterator = _globalUniformNameToID.find(name);
	if (uniformNameToIDIterator == _globalUniformNameToID.end())
	{
		_log->Warning("Failed to locate shader uniform with name \"{0}\"", name.Get());
		return StateValueId::Null;
	}
	return uniformNameToIDIterator->second;
}

//----------------------------------------------------------------------------------------
TextureId Direct3DShaderProgram::GetTextureId(const Marshal::In<std::string>& name) const
{
	auto textureNameToIDIterator = _textureNameToID.find(name);
	if (textureNameToIDIterator == _textureNameToID.end())
	{
		_log->Warning("Failed to locate shader texture with name \"{0}\"", name.Get());
		return TextureId::Null;
	}
	return textureNameToIDIterator->second;
}

//----------------------------------------------------------------------------------------
SamplerId Direct3DShaderProgram::GetSamplerId(const Marshal::In<std::string>& name) const
{
	auto samplerNameToIDIterator = _samplerNameToID.find(name);
	if (samplerNameToIDIterator == _samplerNameToID.end())
	{
		_log->Warning("Failed to locate shader sampler with name \"{0}\"", name.Get());
		return SamplerId::Null;
	}
	return samplerNameToIDIterator->second;
}

//----------------------------------------------------------------------------------------
StateBufferId Direct3DShaderProgram::GetStateBufferId(const Marshal::In<std::string>& name) const
{
	auto stateBufferNameToIDIterator = _constantBufferNameToID.find(name);
	if (stateBufferNameToIDIterator == _constantBufferNameToID.end())
	{
		_log->Warning("Failed to locate state buffer with name \"{0}\"", name.Get());
		return StateBufferId::Null;
	}
	return (StateBufferId)stateBufferNameToIDIterator->second;
}

//----------------------------------------------------------------------------------------
ResourceArrayId Direct3DShaderProgram::GetResourceArrayId(const Marshal::In<std::string>& name) const
{
	auto resourceBufferNameToIDIterator = _resourceBufferNameToID.find(name);
	if (resourceBufferNameToIDIterator == _resourceBufferNameToID.end())
	{
		_log->Warning("Failed to locate resource array with name \"{0}\"", name.Get());
		return ResourceArrayId::Null;
	}
	return (ResourceArrayId)resourceBufferNameToIDIterator->second;
}

//----------------------------------------------------------------------------------------
constexpr DXGI_FORMAT Direct3DShaderProgram::GetVertexShaderInputNativeDataFormat(const D3D11_SIGNATURE_PARAMETER_DESC& paramDesc)
{
	if (paramDesc.Mask == D3D_COMPONENT_MASK_X)
	{
		switch (paramDesc.ComponentType)
		{
		case D3D_REGISTER_COMPONENT_UINT32:
			return DXGI_FORMAT_R32_UINT;
		case D3D_REGISTER_COMPONENT_SINT32:
			return DXGI_FORMAT_R32_SINT;
		case D3D_REGISTER_COMPONENT_FLOAT32:
			return DXGI_FORMAT_R32_FLOAT;
		}
		UNREACHABLE_CONSTEXPR();
	}
	else if (paramDesc.Mask <= (D3D_COMPONENT_MASK_X | D3D_COMPONENT_MASK_Y))
	{
		switch (paramDesc.ComponentType)
		{
		case D3D_REGISTER_COMPONENT_UINT32:
			return DXGI_FORMAT_R32G32_UINT;
		case D3D_REGISTER_COMPONENT_SINT32:
			return DXGI_FORMAT_R32G32_SINT;
		case D3D_REGISTER_COMPONENT_FLOAT32:
			return DXGI_FORMAT_R32G32_FLOAT;
		}
		UNREACHABLE_CONSTEXPR();
	}
	else if (paramDesc.Mask <= (D3D_COMPONENT_MASK_X | D3D_COMPONENT_MASK_Y | D3D_COMPONENT_MASK_Z))
	{
		switch (paramDesc.ComponentType)
		{
		case D3D_REGISTER_COMPONENT_UINT32:
			return DXGI_FORMAT_R32G32B32_UINT;
		case D3D_REGISTER_COMPONENT_SINT32:
			return DXGI_FORMAT_R32G32B32_SINT;
		case D3D_REGISTER_COMPONENT_FLOAT32:
			return DXGI_FORMAT_R32G32B32_FLOAT;
		}
		UNREACHABLE_CONSTEXPR();
	}
	else if (paramDesc.Mask <= (D3D_COMPONENT_MASK_X | D3D_COMPONENT_MASK_Y | D3D_COMPONENT_MASK_Z | D3D_COMPONENT_MASK_W))
	{
		switch (paramDesc.ComponentType)
		{
		case D3D_REGISTER_COMPONENT_UINT32:
			return DXGI_FORMAT_R32G32B32A32_UINT;
		case D3D_REGISTER_COMPONENT_SINT32:
			return DXGI_FORMAT_R32G32B32A32_SINT;
		case D3D_REGISTER_COMPONENT_FLOAT32:
			return DXGI_FORMAT_R32G32B32A32_FLOAT;
		}
		UNREACHABLE_CONSTEXPR();
	}
	return DXGI_FORMAT_UNKNOWN;
}

//----------------------------------------------------------------------------------------
// State buffer methods
//----------------------------------------------------------------------------------------
SuccessToken Direct3DShaderProgram::LoadStateBufferLayoutFromShader(StateBufferId stateBufferId, IStateBufferLayout* stateBufferLayout) const
{
	// Attempt to retrieve the target constant buffer
	if ((stateBufferId == graphics::StateBufferId::Null) || ((size_t)stateBufferId >= _constantBuffers.size()))
	{
		_log->Warning("Attempted to load state buffer layout from invalid state buffer ID {0}", stateBufferId);
		return false;
	}
	const ConstantBufferInfo& bufferInfo = _constantBuffers[(size_t)stateBufferId];

	// Initiate the state buffer layout build process
	auto* stateBufferLayoutResolved = KnownDynamicCast<Direct3DStateBufferLayout*>(stateBufferLayout);
	if (!stateBufferLayoutResolved->BeginManualLayoutDefinition())
	{
		_log->Error("Failed to start layout definition build process when loading state buffer layout");
		return false;
	}

	// Add each uniform to the state buffer layout
	for (const ConstantBufferEntryInfo& entryInfo : bufferInfo.uniformBufferEntries)
	{
		if (entryInfo.width == 0)
		{
			stateBufferLayoutResolved->AddManualField(entryInfo.attributeName, entryInfo.bufferOffset, entryInfo.type, entryInfo.arraySize, entryInfo.arrayStrideInBytes, entryInfo.leadingArraySizes, entryInfo.leadingArrayStrides);
		}
		else if (entryInfo.height == 0)
		{
			stateBufferLayoutResolved->AddManualVector(entryInfo.attributeName, entryInfo.bufferOffset, entryInfo.type, entryInfo.width, entryInfo.arraySize, entryInfo.arrayStrideInBytes, entryInfo.leadingArraySizes, entryInfo.leadingArrayStrides);
		}
		else
		{
			stateBufferLayoutResolved->AddManualMatrix(entryInfo.attributeName, entryInfo.bufferOffset, entryInfo.type, entryInfo.width, entryInfo.height, entryInfo.arraySize, entryInfo.arrayStrideInBytes, entryInfo.leadingArraySizes, entryInfo.leadingArrayStrides);
		}
	}

	// Attempt to construct the state buffer layout, and return the result to the caller.
	if (!stateBufferLayout->ConstructStateLayout())
	{
		_log->Error("Failed to construct state buffer layout for state buffer with ID {0}", stateBufferId);
		return false;
	}
	return true;
}

//----------------------------------------------------------------------------------------
// Shader binding methods
//----------------------------------------------------------------------------------------
bool Direct3DShaderProgram::IsComputeShader() const
{
	return _isComputeShader;
}

//----------------------------------------------------------------------------------------
bool Direct3DShaderProgram::CreateInputLayout(const std::vector<D3D11_INPUT_ELEMENT_DESC>& inputDescription, ID3D11InputLayout** inputLayout)
{
	// Direct3D requires that all vertex input parameters for our vertex shader are satisfied, even if we don't end up
	// using them, and no data is provided for them. To handle this, we identify any defined input parameters which
	// haven't been bound, and create dummy bindings here. The bound data won't be meaningful, but we assume here that a
	// shader uniform or the like is defined in this case to indicate that unbound vertex input data has not been
	// provided.
	std::vector<D3D11_INPUT_ELEMENT_DESC> inputDescriptionCopy = inputDescription;
	for (const auto& shaderInputParameter : _attributeNameList)
	{
		bool hasEntry = false;
		for (const auto& entry : inputDescription)
		{
			if (entry.SemanticName == shaderInputParameter.name)
			{
				hasEntry = true;
				break;
			}
		}
		if (hasEntry)
		{
			continue;
		}

		D3D11_INPUT_ELEMENT_DESC descriptionEntry = {};
		descriptionEntry.SemanticName = shaderInputParameter.name.c_str();
		descriptionEntry.SemanticIndex = (UINT)shaderInputParameter.index;
		descriptionEntry.InputSlot = 0;
		descriptionEntry.AlignedByteOffset = 0;
		descriptionEntry.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		descriptionEntry.InstanceDataStepRate = 0;
		descriptionEntry.Format = shaderInputParameter.format;
		inputDescriptionCopy.push_back(descriptionEntry);
	}

	// Try to create the input layout
	HRESULT createInputLayoutReturn = _renderer->GetDevice()->CreateInputLayout(inputDescriptionCopy.data(), (UINT)inputDescriptionCopy.size(), _vertexShaderCodeBlob->GetBufferPointer(), _vertexShaderCodeBlob->GetBufferSize(), inputLayout);
	if (FAILED(createInputLayoutReturn))
	{
		_log->Error("CreateInputLayout failed with error code {0}", createInputLayoutReturn);
		return false;
	}
	return true;
}

//----------------------------------------------------------------------------------------
void Direct3DShaderProgram::BindShaderProgram(ID3D11DeviceContext1* context, bool bindLegacyConstantBuffers)
{
	// Bind the shader programs
	context->VSSetShader(_vertexShader.Get(), nullptr, 0);
	context->PSSetShader(_pixelShader.Get(), nullptr, 0);
	context->GSSetShader(_geometryShader.Get(), nullptr, 0);
	context->CSSetShader(_computeShader.Get(), nullptr, 0);

	// Bind the constant buffers for the shaders
	if (bindLegacyConstantBuffers)
	{
		const GlobalConstantBufferInfo& constantBufferVertex = _globalConstantBuffers[ShaderStageToIndex(IShaderProgram::ShaderStage::Vertex)];
		const GlobalConstantBufferInfo& constantBufferFragment = _globalConstantBuffers[ShaderStageToIndex(IShaderProgram::ShaderStage::Fragment)];
		const GlobalConstantBufferInfo& constantBufferGeometry = _globalConstantBuffers[ShaderStageToIndex(IShaderProgram::ShaderStage::Geometry)];
		const GlobalConstantBufferInfo& constantBufferCompute = _globalConstantBuffers[ShaderStageToIndex(IShaderProgram::ShaderStage::Compute)];
		if (constantBufferVertex.bufferExists)
		{
			context->VSSetConstantBuffers(constantBufferVertex.bufferInfo.bindingPoints[ShaderStageToIndex(IShaderProgram::ShaderStage::Vertex)].registerNo, 1, constantBufferVertex.nativeBuffer.GetAddressOf());
		}
		if (constantBufferFragment.bufferExists)
		{
			context->PSSetConstantBuffers(constantBufferFragment.bufferInfo.bindingPoints[ShaderStageToIndex(IShaderProgram::ShaderStage::Fragment)].registerNo, 1, constantBufferFragment.nativeBuffer.GetAddressOf());
		}
		if (constantBufferGeometry.bufferExists)
		{
			context->GSSetConstantBuffers(constantBufferGeometry.bufferInfo.bindingPoints[ShaderStageToIndex(IShaderProgram::ShaderStage::Geometry)].registerNo, 1, constantBufferGeometry.nativeBuffer.GetAddressOf());
		}
		if (constantBufferCompute.bufferExists)
		{
			context->CSSetConstantBuffers(constantBufferCompute.bufferInfo.bindingPoints[ShaderStageToIndex(IShaderProgram::ShaderStage::Compute)].registerNo, 1, constantBufferCompute.nativeBuffer.GetAddressOf());
		}
	}
}

//----------------------------------------------------------------------------------------
void Direct3DShaderProgram::UnbindAllShaderResources(ID3D11DeviceContext1* context) const
{
	if (!IsComputeShader())
	{
		// Clear shader resources. We generally don't want to unbind buffers, as it's just unnecessary overhead. If a
		// resource is used, it either needs to be bound, or its state is undefined. If we have a texture resource
		// however which is bound to a framebuffer, then bound as a shader resource or vice versa, we'll get debug
		// warnings from Direct3D if the resource isn't unbound first. We also get similar issues when switching between
		// compute and graphics work that shares resources. We clear all resources here when transitioning between
		// shader programs to prevent this occurring, without introducing significant overhead above what we'd normally
		// incur from changing the program. We also clear discrete sampler objects here for consistency.
		context->VSSetShaderResources(0, (uint32_t)_resourceClearArray[ShaderStageToIndex(ShaderStage::Vertex)].size(), _resourceClearArray[ShaderStageToIndex(ShaderStage::Vertex)].data());
		context->PSSetShaderResources(0, (uint32_t)_resourceClearArray[ShaderStageToIndex(ShaderStage::Fragment)].size(), _resourceClearArray[ShaderStageToIndex(ShaderStage::Fragment)].data());
		context->GSSetShaderResources(0, (uint32_t)_resourceClearArray[ShaderStageToIndex(ShaderStage::Geometry)].size(), _resourceClearArray[ShaderStageToIndex(ShaderStage::Geometry)].data());
		context->VSSetSamplers(0, (uint32_t)_samplerClearArray[ShaderStageToIndex(ShaderStage::Vertex)].size(), _samplerClearArray[ShaderStageToIndex(ShaderStage::Vertex)].data());
		context->PSSetSamplers(0, (uint32_t)_samplerClearArray[ShaderStageToIndex(ShaderStage::Fragment)].size(), _samplerClearArray[ShaderStageToIndex(ShaderStage::Fragment)].data());
		context->GSSetSamplers(0, (uint32_t)_samplerClearArray[ShaderStageToIndex(ShaderStage::Geometry)].size(), _samplerClearArray[ShaderStageToIndex(ShaderStage::Geometry)].data());

		// Clear all UAV targets
		context->OMSetRenderTargetsAndUnorderedAccessViews(D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, nullptr, nullptr, 0, (UINT)_uavClearArray.size(), _uavClearArray.data(), _uavCounterClearArray.data());
	}
	else
	{
		// Clear shader resources
		context->CSSetShaderResources(0, (uint32_t)_resourceClearArray[ShaderStageToIndex(ShaderStage::Compute)].size(), _resourceClearArray[ShaderStageToIndex(ShaderStage::Compute)].data());
		context->CSSetSamplers(0, (uint32_t)_samplerClearArray[ShaderStageToIndex(ShaderStage::Compute)].size(), _samplerClearArray[ShaderStageToIndex(ShaderStage::Compute)].data());

		// Clear all UAV targets
		context->CSSetUnorderedAccessViews(0, (uint32_t)_uavClearArray.size(), _uavClearArray.data(), _uavCounterClearArray.data());
	}
}

//----------------------------------------------------------------------------------------
// Constant buffer binding methods
//----------------------------------------------------------------------------------------
void Direct3DShaderProgram::BindConstantBuffer(IStateBuffer* stateBuffer, StateBufferId stateBufferId, uint32_t stateBufferPageNo, ID3D11DeviceContext1* context) const
{
	// Validate the state buffer ID
	if ((stateBufferId == graphics::StateBufferId::Null) || ((size_t)stateBufferId > _constantBuffers.size()))
	{
		_log->Warning("Attempted to bind constant buffer with buffer ID {0}", stateBufferId);
		return;
	}

	// Retrieve the virtual address for the target state buffer page
	auto* stateBufferResolved = KnownDynamicCast<Direct3DStateBuffer*>(stateBuffer);
	ID3D11Buffer* nativeBuffer;
	UINT pageBlockOffsetInUnits;
	UINT pageSizeInUnits;
	if (!stateBufferResolved->GetStateBufferPageGpuAddress(stateBufferPageNo, nativeBuffer, pageBlockOffsetInUnits, pageSizeInUnits))
	{
		_log->Error("Failed to retrieve virtual address in constant buffer with buffer ID {0} for page number {1}", stateBufferId, stateBufferPageNo);
		return;
	}

	// Bind the state buffer page
	const ConstantBufferInfo& constantBuffer = _constantBuffers[(size_t)stateBufferId];
	constexpr uint32_t vertexIndex = ShaderStageToIndex(IShaderProgram::ShaderStage::Vertex);
	constexpr uint32_t fragmentIndex = ShaderStageToIndex(IShaderProgram::ShaderStage::Fragment);
	constexpr uint32_t geometryIndex = ShaderStageToIndex(IShaderProgram::ShaderStage::Geometry);
	constexpr uint32_t computeIndex = ShaderStageToIndex(IShaderProgram::ShaderStage::Compute);
	if (constantBuffer.bindingPointPresent[vertexIndex])
	{
		context->VSSetConstantBuffers1(constantBuffer.bindingPoints[vertexIndex].registerNo, 1, &nativeBuffer, &pageBlockOffsetInUnits, &pageSizeInUnits);
	}
	if (constantBuffer.bindingPointPresent[fragmentIndex])
	{
		context->PSSetConstantBuffers1(constantBuffer.bindingPoints[fragmentIndex].registerNo, 1, &nativeBuffer, &pageBlockOffsetInUnits, &pageSizeInUnits);
	}
	if (constantBuffer.bindingPointPresent[geometryIndex])
	{
		context->GSSetConstantBuffers1(constantBuffer.bindingPoints[geometryIndex].registerNo, 1, &nativeBuffer, &pageBlockOffsetInUnits, &pageSizeInUnits);
	}
	if (constantBuffer.bindingPointPresent[computeIndex])
	{
		context->CSSetConstantBuffers1(constantBuffer.bindingPoints[computeIndex].registerNo, 1, &nativeBuffer, &pageBlockOffsetInUnits, &pageSizeInUnits);
	}
}

//----------------------------------------------------------------------------------------
void Direct3DShaderProgram::UnbindConstantBuffer(IStateBuffer* stateBuffer, StateBufferId stateBufferId, uint32_t stateBufferPageNo, ID3D11DeviceContext1* context) const
{
	// Validate the state buffer ID
	if ((stateBufferId == graphics::StateBufferId::Null) || ((size_t)stateBufferId >= _constantBuffers.size()))
	{
		_log->Warning("Attempted to bind constant buffer with buffer ID {0}", stateBufferId);
		return;
	}

	// Unbind the state buffer page
	ID3D11Buffer* nullBuffer = nullptr;
	UINT zeroData = 0;
	const ConstantBufferInfo& constantBuffer = _constantBuffers[(size_t)stateBufferId];
	constexpr uint32_t vertexIndex = ShaderStageToIndex(IShaderProgram::ShaderStage::Vertex);
	constexpr uint32_t fragmentIndex = ShaderStageToIndex(IShaderProgram::ShaderStage::Fragment);
	constexpr uint32_t geometryIndex = ShaderStageToIndex(IShaderProgram::ShaderStage::Geometry);
	constexpr uint32_t computeIndex = ShaderStageToIndex(IShaderProgram::ShaderStage::Compute);
	if (constantBuffer.bindingPointPresent[vertexIndex])
	{
		context->VSSetConstantBuffers1(constantBuffer.bindingPoints[vertexIndex].registerNo, 1, &nullBuffer, &zeroData, &zeroData);
	}
	if (constantBuffer.bindingPointPresent[fragmentIndex])
	{
		context->PSSetConstantBuffers1(constantBuffer.bindingPoints[fragmentIndex].registerNo, 1, &nullBuffer, &zeroData, &zeroData);
	}
	if (constantBuffer.bindingPointPresent[geometryIndex])
	{
		context->GSSetConstantBuffers1(constantBuffer.bindingPoints[geometryIndex].registerNo, 1, &nullBuffer, &zeroData, &zeroData);
	}
	if (constantBuffer.bindingPointPresent[computeIndex])
	{
		context->CSSetConstantBuffers1(constantBuffer.bindingPoints[computeIndex].registerNo, 1, &nullBuffer, &zeroData, &zeroData);
	}
}

//----------------------------------------------------------------------------------------
void Direct3DShaderProgram::GetBindPointsForTexture(TextureId textureId, const TextureBindPointEntry*& bindPoints, size_t& bindPointCount) const
{
	if ((textureId == graphics::TextureId::Null) || ((size_t)textureId >= _textureResources.size()))
	{
		_log->Warning("Attempted to retrieve bind point for texture with ID \"{0}\"", textureId);
		bindPoints = nullptr;
		bindPointCount = 0;
		return;
	}

	const ShaderTextureInfo& textureResource = _textureResources[(size_t)textureId];
	bindPoints = textureResource.bindPoints.data();
	bindPointCount = textureResource.bindPoints.size();
}

//----------------------------------------------------------------------------------------
void Direct3DShaderProgram::GetBindPointsForSampler(SamplerId samplerId, const SamplerBindPointEntry*& bindPoints, size_t& bindPointCount) const
{
	if ((samplerId == graphics::SamplerId::Null) || ((size_t)samplerId >= _samplerResources.size()))
	{
		_log->Warning("Attempted to retrieve bind point for sampler with ID \"{0}\"", samplerId);
		bindPoints = nullptr;
		bindPointCount = 0;
		return;
	}

	const ShaderSamplerInfo& samplerResource = _samplerResources[(size_t)samplerId];
	bindPoints = samplerResource.bindPoints.data();
	bindPointCount = samplerResource.bindPoints.size();
}

//----------------------------------------------------------------------------------------
void Direct3DShaderProgram::GetBindPointsForResourceArray(ResourceArrayId resourceArrayId, const ResourceArrayBindPointEntry*& bindPoints, size_t& bindPointCount) const
{
	if ((resourceArrayId == graphics::ResourceArrayId::Null) || ((size_t)resourceArrayId >= _resourceBufferResources.size()))
	{
		_log->Warning("Attempted to retrieve bind point for resource array with ID \"{0}\"", resourceArrayId);
		bindPoints = nullptr;
		bindPointCount = 0;
		return;
	}

	const ResourceBufferInfo& resourceBufferInfo = _resourceBufferResources[(size_t)resourceArrayId];
	bindPoints = resourceBufferInfo.bindPoints.data();
	bindPointCount = resourceBufferInfo.bindPoints.size();
}

//----------------------------------------------------------------------------------------
// State value methods
//----------------------------------------------------------------------------------------
void Direct3DShaderProgram::UpdateStateValue(StateValueId stateId, const uint8_t* data, size_t dataSizeInBytes, const size_t* arrayIndices, size_t arrayIndexCount)
{
	// Ensure the supplied uniform ID is valid
	if ((stateId == graphics::StateValueId::Null) || ((size_t)stateId >= _globalUniformNameToID.size()))
	{
		_log->Error("Attempted to modify state value with an invalid ID of {0}", stateId);
		return;
	}

	// Update the value of the target uniform in each shader constant buffer
	for (auto& constantBuffer : _globalConstantBuffers)
	{
		// Attempt to retrieve a uniform buffer entry for the target uniform value in the constant buffer
		const ConstantBufferEntryInfo& bufferEntry = constantBuffer.bufferInfo.uniformBufferEntries[(size_t)stateId];
		if (!bufferEntry.defined)
		{
			continue;
		}

		// Validate the array indices, and calculate the array buffer offset.
		size_t arrayOffset = 0;
		size_t bufferArrayDimensions = bufferEntry.arraySizes.size();
		if (arrayIndexCount != bufferArrayDimensions)
		{
			_log->Error("Attempted to modify state value {0} with {1} array index values, when {2} indices are required.", bufferEntry.attributeName, arrayIndexCount, bufferArrayDimensions);
			return;
		}
		for (size_t i = 0; i < bufferArrayDimensions; ++i)
		{
			size_t arrayIndex = arrayIndices[i];
			if (arrayIndex >= bufferEntry.arraySizes[i])
			{
				_log->Error("Attempted to modify state value {0} at index {1}, when only {2} entries are present in the array.", bufferEntry.attributeName, arrayIndex, bufferEntry.arraySizes[i]);
				return;
			}
			arrayOffset += arrayIndex * bufferEntry.arrayStrides[i];
		}

		// Update the entry in the buffer
		uint8_t* uniformLocation = constantBuffer.bufferContents.data() + bufferEntry.bufferOffset + arrayOffset;
		auto sizeToCopy = std::min(dataSizeInBytes, bufferEntry.entrySizeInBytes);
		std::memcpy(uniformLocation, data, sizeToCopy);

		// Flag that the buffer is now dirty
		constantBuffer.isDirty = true;
	}
}

//----------------------------------------------------------------------------------------
void Direct3DShaderProgram::SetConstantValue(StateValueId stateId, const uint8_t* data, size_t dataSizeInBytes, const size_t* arrayIndices, size_t arrayIndexCount)
{
	// Direct3D doesn't support shader constants, so we redirect to uniform values here.
	UpdateStateValue(stateId, data, dataSizeInBytes, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void Direct3DShaderProgram::FlushDirtyGlobalConstantBuffers(ID3D11DeviceContext* context)
{
	// Flush the contents of any dirty constant buffers
	for (auto& constantBuffer : _globalConstantBuffers)
	{
		// If this shader stage doesn't have a constant buffer, or the buffer isn't marked as dirty, skip it.
		if (!constantBuffer.bufferExists || !constantBuffer.isDirty)
		{
			continue;
		}

		// Update the contents of the constant buffer for this shader stage
		context->UpdateSubresource(constantBuffer.nativeBuffer.Get(), 0, nullptr, constantBuffer.bufferContents.data(), 0, 0);

		// Remove the dirty flag for the target constant buffer
		constantBuffer.isDirty = false;
	}
}

//----------------------------------------------------------------------------------------
void Direct3DShaderProgram::PushGlobalConstantBufferState()
{
	for (auto& bufferInfo : _globalConstantBuffers)
	{
		if (bufferInfo.bufferExists)
		{
			bufferInfo.bufferStack.resize(++bufferInfo.bufferStackEntries);
			bufferInfo.bufferStack[bufferInfo.bufferStackEntries - 1] = bufferInfo.bufferContents;
			bufferInfo.isDirty = true;
		}
	}
}

//----------------------------------------------------------------------------------------
void Direct3DShaderProgram::PopGlobalConstantBufferState()
{
	for (auto& bufferInfo : _globalConstantBuffers)
	{
		if (bufferInfo.bufferExists)
		{
			assert(bufferInfo.bufferStackEntries > 1);
			bufferInfo.bufferContents = bufferInfo.bufferStack[--bufferInfo.bufferStackEntries];
			bufferInfo.isDirty = true;
		}
	}
}

//----------------------------------------------------------------------------------------
void Direct3DShaderProgram::RestoreGlobalConstantBufferBaseline()
{
	for (auto& bufferInfo : _globalConstantBuffers)
	{
		if (bufferInfo.bufferExists)
		{
			bufferInfo.bufferContents = bufferInfo.bufferStack[bufferInfo.bufferStackEntries - 1];
			bufferInfo.isDirty = true;
		}
	}
}

//----------------------------------------------------------------------------------------
// Global constant buffer session methods
//----------------------------------------------------------------------------------------
void Direct3DShaderProgram::ResetGlobalConstantBufferState()
{
	for (uint32_t i = 0; i < Direct3DShaderProgram::ShaderStageCount; ++i)
	{
		_globalConstantBufferBuildingSession.nextPageNumbers[i] = 0;
		_globalConstantBufferBuildingSession.lastWrittenPageBlockIndex[i] = 0;
		_globalConstantBufferBuildingSession.buffersExist[i] = _globalConstantBuffers[i].bufferExists;
		_globalConstantBufferBuildingSession.stateBuffers[i] = _globalConstantBuffers[i].stateBuffer.get();
	}
}

//----------------------------------------------------------------------------------------
void Direct3DShaderProgram::BeginGlobalConstantBufferBuildingSession(GlobalConstantBufferBuildingSession& stateInfo, ID3D11DeviceContext1* context)
{
	stateInfo = _globalConstantBufferBuildingSession;
}

//----------------------------------------------------------------------------------------
void Direct3DShaderProgram::CompleteGlobalConstantBufferBuildingSession(GlobalConstantBufferBuildingSession& stateInfo, ID3D11DeviceContext1* context)
{
	_globalConstantBufferBuildingSession = stateInfo;
	for (uint32_t i = 0; i < Direct3DShaderProgram::ShaderStageCount; ++i)
	{
		GlobalConstantBufferInfo& bufferInfo = _globalConstantBuffers[i];
		if (bufferInfo.bufferExists)
		{
			bufferInfo.stateBuffer->CompletePendingDataWritesForPageBlock(_globalConstantBufferBuildingSession.lastWrittenPageBlockIndex[i], context);
		}
	}
}

//----------------------------------------------------------------------------------------
void Direct3DShaderProgram::GenerateGlobalConstantBufferBindings(GlobalConstantBufferBuildingSession& stateInfo, GlobalConstantBufferBindingInfo& bindingInfo, ID3D11DeviceContext1* context)
{
	// Flag that no bindings have been generated so far based on the current buffer state
	bindingInfo.hasBindings = false;

	// Flush the contents of any dirty constant buffers
	for (uint32_t shaderStageID = 0; shaderStageID < ShaderStageCount; ++shaderStageID)
	{
		// If this shader stage doesn't have a constant buffer, or the buffer isn't marked as dirty, skip it.
		GlobalConstantBufferInfo& constantBuffer = _globalConstantBuffers[shaderStageID];
		if (!constantBuffer.bufferExists || !constantBuffer.isDirty)
		{
			bindingInfo.bindingSet[shaderStageID] = false;
			continue;
		}

		// Store information on this constant buffer binding in the binding info
		auto nextPageNo = stateInfo.nextPageNumbers[shaderStageID];
		bindingInfo.hasBindings = true;
		bindingInfo.pageNumbers[shaderStageID] = nextPageNo;
		bindingInfo.bindingSet[shaderStageID] = true;

		// Update the contents of the constant buffer for this shader stage
		if (nextPageNo >= constantBuffer.allocatedPageCount)
		{
			constantBuffer.allocatedPageCount += constantBuffer.stateBuffer->GetPagesPerPageBlock();
			constantBuffer.stateBuffer->ResizePageCount(constantBuffer.allocatedPageCount).IgnoreResult();
		}
		uint32_t pageIndexInBlock;
		uint32_t pageBlockIndex;
		constantBuffer.stateBuffer->SetRawPageDataWithReturnedPageIndex(nextPageNo, constantBuffer.bufferContents.data(), pageIndexInBlock, pageBlockIndex);
		stateInfo.nextPageNumbers[shaderStageID] = ++nextPageNo;
		stateInfo.lastWrittenPageBlockIndex[shaderStageID] = pageBlockIndex;

		// If we've just started a new page block in the constant buffer, commit changes to the previous page block.
		if ((pageIndexInBlock == 0) && (pageBlockIndex > 0))
		{
			constantBuffer.stateBuffer->CompletePendingDataWritesForPageBlock(pageBlockIndex - 1, context);
		}

		// Remove the dirty flag for the target constant buffer
		constantBuffer.isDirty = false;
	}
}

//----------------------------------------------------------------------------------------
void Direct3DShaderProgram::ApplyGlobalConstantBufferBindings(const GlobalConstantBufferBindingInfo& bindingInfo, ID3D11DeviceContext1* context)
{
	// If no bindings are set, abort any further processing.
	if (!bindingInfo.hasBindings)
	{
		return;
	}

	// Bind any required constant buffer pages we need
	constexpr uint32_t vertexIndex = ShaderStageToIndex(IShaderProgram::ShaderStage::Vertex);
	constexpr uint32_t fragmentIndex = ShaderStageToIndex(IShaderProgram::ShaderStage::Fragment);
	constexpr uint32_t geometryIndex = ShaderStageToIndex(IShaderProgram::ShaderStage::Geometry);
	constexpr uint32_t computeIndex = ShaderStageToIndex(IShaderProgram::ShaderStage::Compute);
	ID3D11Buffer* nativeBuffer;
	UINT pageBlockOffsetInUnits;
	UINT pageSizeInUnits;
	if (_globalConstantBuffers[vertexIndex].bufferExists && bindingInfo.bindingSet[vertexIndex])
	{
		if (_globalConstantBuffers[vertexIndex].stateBuffer->GetStateBufferPageGpuAddress(bindingInfo.pageNumbers[vertexIndex], nativeBuffer, pageBlockOffsetInUnits, pageSizeInUnits))
		{
			context->VSSetConstantBuffers1(_globalConstantBuffers[vertexIndex].bufferInfo.bindingPoints[vertexIndex].registerNo, 1, &nativeBuffer, &pageBlockOffsetInUnits, &pageSizeInUnits);
		}
	}
	if (_globalConstantBuffers[fragmentIndex].bufferExists && bindingInfo.bindingSet[fragmentIndex])
	{
		if (_globalConstantBuffers[fragmentIndex].stateBuffer->GetStateBufferPageGpuAddress(bindingInfo.pageNumbers[fragmentIndex], nativeBuffer, pageBlockOffsetInUnits, pageSizeInUnits))
		{
			context->PSSetConstantBuffers1(_globalConstantBuffers[fragmentIndex].bufferInfo.bindingPoints[fragmentIndex].registerNo, 1, &nativeBuffer, &pageBlockOffsetInUnits, &pageSizeInUnits);
		}
	}
	if (_globalConstantBuffers[geometryIndex].bufferExists && bindingInfo.bindingSet[geometryIndex])
	{
		if (_globalConstantBuffers[geometryIndex].stateBuffer->GetStateBufferPageGpuAddress(bindingInfo.pageNumbers[geometryIndex], nativeBuffer, pageBlockOffsetInUnits, pageSizeInUnits))
		{
			context->GSSetConstantBuffers1(_globalConstantBuffers[geometryIndex].bufferInfo.bindingPoints[geometryIndex].registerNo, 1, &nativeBuffer, &pageBlockOffsetInUnits, &pageSizeInUnits);
		}
	}
	if (_globalConstantBuffers[computeIndex].bufferExists && bindingInfo.bindingSet[computeIndex])
	{
		if (_globalConstantBuffers[computeIndex].stateBuffer->GetStateBufferPageGpuAddress(bindingInfo.pageNumbers[computeIndex], nativeBuffer, pageBlockOffsetInUnits, pageSizeInUnits))
		{
			context->CSSetConstantBuffers1(_globalConstantBuffers[computeIndex].bufferInfo.bindingPoints[computeIndex].registerNo, 1, &nativeBuffer, &pageBlockOffsetInUnits, &pageSizeInUnits);
		}
	}
}

} // namespace cobalt::graphics
