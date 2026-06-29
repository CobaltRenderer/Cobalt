// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "VulkanShaderProgram.h"
#include "VulkanDataArray.h"
#include "VulkanHeaders.h"
#include "VulkanRenderer.h"
#include "VulkanStateBuffer.h"
#include "VulkanStateBufferLayout.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <Internal/ShaderSupport/ShaderSupport.pkg>
#include <Internal/RendererSupport/KnownDynamicCast.h>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_map>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
VulkanShaderProgram::VulkanShaderProgram(cobalt::logging::ILogger* log, VulkanRenderer* renderer)
{
	_log = log;
	_renderer = renderer;
}

//----------------------------------------------------------------------------------------
VulkanShaderProgram::~VulkanShaderProgram()
{
	ReleaseMemory();
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void VulkanShaderProgram::Delete()
{
	_renderer->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
void VulkanShaderProgram::ReleaseMemory()
{
	VkDevice device = _renderer->GetDevice();
	for (auto& entry : _bindingDescriptorPools)
	{
		vkDestroyDescriptorPool(device, entry, nullptr);
	}
	for (auto& entry : _globalStateBindingDescriptorPools)
	{
		vkDestroyDescriptorPool(device, entry, nullptr);
	}
	for (auto& entry : _descriptorSetLayouts)
	{
		vkDestroyDescriptorSetLayout(device, entry, nullptr);
	}
	for (auto shaderModule : _shaderModules)
	{
		vkDestroyShaderModule(device, shaderModule, nullptr);
	}
}

//----------------------------------------------------------------------------------------
// Code format methods
//----------------------------------------------------------------------------------------
bool VulkanShaderProgram::IsCodeFormatSupported(CodeFormat format) const
{
	return (format == CodeFormat::HLSL) || (format == CodeFormat::SPIRVAssembly) || (format == CodeFormat::SPIRV) || (format == CodeFormat::GLSL);
}

//----------------------------------------------------------------------------------------
VulkanShaderProgram::CodeFormat VulkanShaderProgram::PreferredCodeFormat() const
{
	return CodeFormat::SPIRV;
}

//----------------------------------------------------------------------------------------
// Compilation methods
//----------------------------------------------------------------------------------------
SuccessToken VulkanShaderProgram::ConfigureShaderTarget(const ShaderTargetInfoBase& shaderTargetInfo)
{
	// No configuration options currently available for this renderer
	auto shaderTargetType = shaderTargetInfo.shaderTarget;
	_log->Error("Attempted to configure shader target using incompatible target type {0}", shaderTargetType);
	return false;
}

//----------------------------------------------------------------------------------------
SuccessToken VulkanShaderProgram::LoadShaderStage(ShaderStage stage, const ShaderSourceInfoBase& shaderSourceInfo)
{
	// Ensure something hasn't already been added for the target shader stage
	auto shaderStageIndex = ShaderStageToIndex(stage);
	if (_shaderBlocks[shaderStageIndex].code != nullptr)
	{
		_log->Error("Attempted to bind shader code for stage {0} when code has already been bound.", stage);
		return false;
	}

	// Retrieve the supplied shader info
	IShaderCode::Language language;
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

	// Attempt to convert the code to SPIR-V. Note that we still need to load native SPIR-V in this manner too, in order
	// to support reflection and generate binding locations.
	IShaderCode::Environment targetEnvironment = IShaderCode::Environment::Vulkan_11;
	auto shaderCode = IShaderCode::Create(_log->GetLoggerChildScope("ShaderSupport"));
	if (!shaderCode->LoadCode(language, shaderStage, sourceEnvironment, targetEnvironment, code, codeSizeInBytes, entryPointName, _baseBindingNo, _shaderResourceInfo))
	{
		_log->Error("Failed to import shader code for stage {0}", shaderStage);
		return false;
	}

	// Store information on the provided shader code block
	ShaderBlockInfo& blockInfo = _shaderBlocks[shaderStageIndex];
	blockInfo.code = std::move(shaderCode);
	blockInfo.entryPointName = entryPointName;
	return true;
}

//----------------------------------------------------------------------------------------
bool VulkanShaderProgram::CreateShaderModule(IShaderCode* shaderCode, VkShaderModule& shaderModule, ShaderStage shaderStage)
{
	// Export a SPIR-V version of the shader code
	std::vector<uint32_t> convertedCode;
	if (!shaderCode->ExportCodeAsSPIRV(convertedCode))
	{
		_log->Error("Failed to convert shader code for stage {0}", shaderStage);
		return false;
	}

	// Create a shader module
	VkShaderModuleCreateInfo shaderInfo = {};
	shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderInfo.codeSize = convertedCode.size() * sizeof(*convertedCode.data());
	shaderInfo.pCode = convertedCode.data();
	shaderInfo.flags = 0;
	shaderInfo.pNext = nullptr;
	VkResult createShaderModuleResult = vkCreateShaderModule(_renderer->GetDevice(), &shaderInfo, nullptr, &shaderModule);
	if (createShaderModuleResult != VK_SUCCESS)
	{
		_log->Error("Could not create shader module during compilation with error code {0}", createShaderModuleResult);
		return false;
	}
	return true;
}

//----------------------------------------------------------------------------------------
SuccessToken VulkanShaderProgram::CompileProgram()
{
	// Build the set of defined shader stages
	std::unordered_set<ShaderStage> definedStages;
	for (size_t stageIndex = 0; stageIndex < ShaderStageCount; ++stageIndex)
	{
		if (_shaderBlocks[stageIndex].code == nullptr)
		{
			continue;
		}
		definedStages.insert(ShaderIndexToStage(stageIndex));
	}

	// Ensure that all required shader modules have been provided
	if (definedStages.empty())
	{
		_log->Error("Failed to compile shader as no shader blocks have been specified.");
		return false;
	}
	if (definedStages.find(ShaderStage::Compute) == definedStages.end())
	{
		ShaderStage requiredBlocks[] = {ShaderStage::Fragment, ShaderStage::Vertex};
		for (auto& requiredBlock : requiredBlocks)
		{
			if (definedStages.find(requiredBlock) == definedStages.end())
			{
				_log->Error("Failed to find required shader block of type {0} when compiling shader.", requiredBlock);
				return false;
			}
		}
	}
	else if (definedStages.size() > 1)
	{
		_log->Error("Failed to compile shader as an invalid combination of shader modules was provided.");
		return false;
	}

	// Compile each shader module
	for (size_t stageIndex = 0; stageIndex < ShaderStageCount; ++stageIndex)
	{
		if (_shaderBlocks[stageIndex].code == nullptr)
		{
			continue;
		}
		if (!CreateShaderModule(_shaderBlocks[stageIndex].code.get(), _shaderModules[stageIndex], ShaderIndexToStage(stageIndex)))
		{
			_log->Error("Failed to compile shader module for stage {0}", stageIndex);
			return false;
		}
	}

	// Retrieve the names of all the input parameters for the vertex shader
	for (size_t i = 0; i < _shaderResourceInfo.size(); ++i)
	{
		const auto& resourceInfo = _shaderResourceInfo[i];
		if ((resourceInfo.type != IShaderCode::ResourceType::Input) || ((resourceInfo.usedStages & IShaderCode::Stage::Vertex) != IShaderCode::Stage::Vertex))
		{
			continue;
		}
		ShaderInputParameterInfo parameterInfo;
		parameterInfo.name = resourceInfo.name;
		parameterInfo.shaderResourceIndex = i;
		_attributeNameToID.insert(std::make_pair(resourceInfo.name, VertexAttributeId(_attributeNameList.size())));
		_attributeNameList.push_back(parameterInfo);
	}

	// Create vertex input binding and attribute info
	std::vector<VkVertexInputBindingDescription> descriptions;
	for (size_t i = 0; i < _attributeNameList.size(); ++i)
	{
		const auto& inputAttributeInfo = _attributeNameList[i];
		const auto& resourceInfo = _shaderResourceInfo[inputAttributeInfo.shaderResourceIndex];

		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = (uint32_t)i;
		bindingDescription.stride = (uint32_t)resourceInfo.elementSize;
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		_vertexInputBindingDescriptions.push_back(bindingDescription);

		VkVertexInputAttributeDescription attributeDescription = {};
		attributeDescription.binding = (uint32_t)i;
		attributeDescription.location = resourceInfo.location;
		attributeDescription.format = (VkFormat)resourceInfo.nativeVulkanFormat;
		attributeDescription.offset = 0;
		_vertexInputAttributeDescriptions.push_back(attributeDescription);
	}

	// Backwards compatabile suffix
	std::string combinedSamplerSuffix1 = IShaderCode::CombinedImageSamplerPostfix;

	// As used by glslLang -> Spirv shaders. This is hardcoded into SpirV cross and could conceivably
	// be spat out by a SpirV toolchain going through HLSL.
	std::string combinedSamplerSuffix2 = "_sampler";

	// Retrieve the names of all resource array, texture, sampler, and state buffer resources.
	std::unordered_map<std::string, ShaderSamplerInfo> combinedSamplers;
	for (size_t shaderResourceIndex = 0; shaderResourceIndex < _shaderResourceInfo.size(); ++shaderResourceIndex)
	{
		const auto& resourceInfo = _shaderResourceInfo[shaderResourceIndex];
		if ((resourceInfo.type == IShaderCode::ResourceType::TexelArray) || (resourceInfo.type == IShaderCode::ResourceType::DataArray))
		{
			// Retrieve or create an entry for this resource array
			size_t resourceBufferIndex;
			std::string resourceBufferName = resourceInfo.name;
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
				resourceBufferInfo.shaderResourceIndex = shaderResourceIndex;
				resourceBufferInfo.isDataArray = (resourceInfo.type == IShaderCode::ResourceType::DataArray);
				_resourceBufferResources.push_back(resourceBufferInfo);
				_resourceBufferNameToID.insert(std::make_pair(resourceInfo.name, resourceBufferInfo.id));
			}
			ResourceBufferInfo& resourceBufferInfo = _resourceBufferResources[resourceBufferIndex];

			// Add this binding point for the resource array
			ResourceArrayBindPointEntry bindPoint;
			bindPoint.writeable = resourceInfo.writeableResource;
			bindPoint.bindPoint = resourceInfo.binding;
			bindPoint.stageMask = resourceInfo.usedStages;
			resourceBufferInfo.bindPoints.push_back(bindPoint);
		}
		else if ((resourceInfo.type == IShaderCode::ResourceType::Texture) || (resourceInfo.type == IShaderCode::ResourceType::TextureWithCombinedSampler))
		{
			// Retrieve or create an entry for this texture
			size_t textureIndex;
			std::string textureName = resourceInfo.name;
			auto textureNameToIDIterator = _textureNameToID.find(textureName);
			if (textureNameToIDIterator != _textureNameToID.end())
			{
				textureIndex = (size_t)textureNameToIDIterator->second;
			}
			else
			{
				textureIndex = _textureResources.size();
				ShaderTextureInfo textureInfo;
				textureInfo.name = resourceInfo.name;
				textureInfo.id = (TextureId)_textureResources.size();
				textureInfo.shaderResourceIndex = shaderResourceIndex;
				textureInfo.imageViewType = (VkImageViewType)resourceInfo.nativeVulkanImageViewType;
				_textureResources.push_back(textureInfo);
				_textureNameToID.insert(std::make_pair(resourceInfo.name, textureInfo.id));
			}
			ShaderTextureInfo& textureInfo = _textureResources[textureIndex];

			// Add this binding point for the texture
			TextureBindPointEntry bindPoint;
			bindPoint.bindPoint = resourceInfo.binding;
			bindPoint.stageMask = resourceInfo.usedStages;
			bindPoint.descriptorType = (resourceInfo.type == IShaderCode::ResourceType::TextureWithCombinedSampler ? VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER : VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
			textureInfo.bindPoints.push_back(bindPoint);
		}
		else if (resourceInfo.type == IShaderCode::ResourceType::Sampler)
		{
			// Determine if this is sampler resource is a combined sampler
			auto stringStartsWith = [](const std::string& str, const std::string& prefix) { return (str.size() >= prefix.size()) && (str.compare(0, prefix.size(), prefix) == 0); };
			auto stringEndsWith = [](const std::string& str, const std::string& suffix) { return (str.size() >= suffix.size()) && (str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0); };
			std::string combinedSamplerName;
			if (stringEndsWith(resourceInfo.name, combinedSamplerSuffix1))
			{
				combinedSamplerName = resourceInfo.name;
				combinedSamplerName.resize(combinedSamplerName.size() - combinedSamplerSuffix1.size());
			}
			else if (stringStartsWith(resourceInfo.name, "_") && stringEndsWith(resourceInfo.name, combinedSamplerSuffix2))
			{
				combinedSamplerName = resourceInfo.name;
				combinedSamplerName = combinedSamplerName.substr(1, (combinedSamplerName.size() - 1) - combinedSamplerSuffix2.size());
			}

			if (!combinedSamplerName.empty())
			{
				// Retrieve or create an entry for this combined sampler
				auto& samplerInfo = combinedSamplers[combinedSamplerName];
				samplerInfo.name = combinedSamplerName;

				// Add this binding point for the sampler
				SamplerBindPointEntry bindPoint;
				bindPoint.bindPoint = resourceInfo.binding;
				bindPoint.stageMask = resourceInfo.usedStages;
				samplerInfo.bindPoints.push_back(bindPoint);
			}
			else
			{
				// Retrieve or create an entry for this sampler
				size_t samplerIndex;
				std::string samplerName = resourceInfo.name;
				auto samplerNameToIDIterator = _samplerNameToID.find(samplerName);
				if (samplerNameToIDIterator != _samplerNameToID.end())
				{
					samplerIndex = (size_t)samplerNameToIDIterator->second;
				}
				else
				{
					samplerIndex = _samplerResources.size();
					ShaderSamplerInfo samplerInfo;
					samplerInfo.name = resourceInfo.name;
					samplerInfo.id = (SamplerId)_samplerResources.size();
					samplerInfo.shaderResourceIndex = shaderResourceIndex;
					_samplerResources.push_back(samplerInfo);
					_samplerNameToID.insert(std::make_pair(resourceInfo.name, samplerInfo.id));
				}
				ShaderSamplerInfo& samplerInfo = _samplerResources[samplerIndex];

				// Add this binding point for the sampler
				SamplerBindPointEntry bindPoint;
				bindPoint.bindPoint = resourceInfo.binding;
				bindPoint.stageMask = resourceInfo.usedStages;
				samplerInfo.bindPoints.push_back(bindPoint);
			}
		}
		else if ((resourceInfo.type == IShaderCode::ResourceType::Uniform) && (resourceInfo.name != IShaderCode::GlobalConstantBufferName))
		{
			// Retrieve or create an entry for this state buffer
			size_t stateBufferIndex;
			std::string stateBufferName = resourceInfo.name;
			auto stateBufferNameToIDIterator = _stateBufferNameToID.find(stateBufferName);
			if (stateBufferNameToIDIterator != _stateBufferNameToID.end())
			{
				stateBufferIndex = (size_t)stateBufferNameToIDIterator->second;
			}
			else
			{
				stateBufferIndex = _stateBuffers.size();
				StateBufferInfo stateBufferInfo;
				stateBufferInfo.name = resourceInfo.name;
				stateBufferInfo.id = (StateBufferId)_stateBuffers.size();
				stateBufferInfo.shaderResourceIndex = shaderResourceIndex;
				std::unordered_map<std::string, StateValueId> attributeNamesToIDs;
				if (!LoadStateBufferInfo(0, stateBufferInfo, attributeNamesToIDs, resourceInfo))
				{
					_log->Error("Failed to load state buffer with name \"{0}\"", resourceInfo.name);
					return false;
				}
				_stateBuffers.push_back(stateBufferInfo);
				_stateBufferNameToID.insert(std::make_pair(resourceInfo.name, stateBufferInfo.id));
			}
			StateBufferInfo& constantBufferInfo = _stateBuffers[stateBufferIndex];

			// Add this binding point for the state buffer
			StateBufferBindPointEntry bindPoint;
			bindPoint.bindPoint = resourceInfo.binding;
			bindPoint.stageMask = resourceInfo.usedStages;
			constantBufferInfo.bindPoints.push_back(bindPoint);
		}
	}

	// Identify linked data array counters
	std::string counterBufferSuffix = "@count";
	std::string counterBufferSuffixAlt = "@counter";
	auto stringEndsWith = [](const std::string& str, const std::string& suffix) { return (str.size() >= suffix.size()) && (str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0); };
	std::unordered_map<std::string, size_t> counterBufferOwnerNameToCounterBufferIndex;
	for (size_t i = 0; i < _resourceBufferResources.size(); ++i)
	{
		const auto& resourceBufferInfo = _resourceBufferResources[i];
		if (!resourceBufferInfo.isDataArray)
		{
			continue;
		}
		if (stringEndsWith(resourceBufferInfo.name, counterBufferSuffix))
		{
			std::string owningBufferName = resourceBufferInfo.name;
			owningBufferName.resize(owningBufferName.size() - counterBufferSuffix.size());
			counterBufferOwnerNameToCounterBufferIndex[owningBufferName] = i;
		}
		else if (stringEndsWith(resourceBufferInfo.name, counterBufferSuffixAlt))
		{
			std::string owningBufferName = resourceBufferInfo.name;
			owningBufferName.resize(owningBufferName.size() - counterBufferSuffixAlt.size());
			counterBufferOwnerNameToCounterBufferIndex[owningBufferName] = i;
		}
	}
	for (size_t i = 0; i < _resourceBufferResources.size(); ++i)
	{
		auto& resourceBufferInfo = _resourceBufferResources[i];
		if (!resourceBufferInfo.isDataArray)
		{
			continue;
		}
		auto counterBufferOwnerNameToCounterBufferIndexIterator = counterBufferOwnerNameToCounterBufferIndex.find(resourceBufferInfo.name);
		if (counterBufferOwnerNameToCounterBufferIndexIterator != counterBufferOwnerNameToCounterBufferIndex.end())
		{
			size_t counterBufferIndex = counterBufferOwnerNameToCounterBufferIndexIterator->second;
			auto& counterBufferInfo = _resourceBufferResources[counterBufferIndex];
			resourceBufferInfo.hasAttachedCounterBuffer = true;
			resourceBufferInfo.counterBufferIndex = counterBufferIndex;
			counterBufferInfo.isAttachedDataArrayCounter = true;
			counterBufferInfo.counterBufferIndex = i;

			for (auto& bufferBindPoint : resourceBufferInfo.bindPoints)
			{
				for (auto& counterBindPoint : counterBufferInfo.bindPoints)
				{
					if (bufferBindPoint.stageMask == counterBindPoint.stageMask)
					{
						bufferBindPoint.counterBindPoint = counterBindPoint.bindPoint;
						break;
					}
				}
			}
		}
	}

	// Build information on the global state buffers for each shader stage. Note that we do expect to encounter separate
	// global uniform buffers here for each stage, with the same name for each one.
	for (const auto& resourceInfo : _shaderResourceInfo)
	{
		if ((resourceInfo.type != IShaderCode::ResourceType::Uniform) || (resourceInfo.name != IShaderCode::GlobalConstantBufferName))
		{
			continue;
		}
		std::vector<ShaderStage> usedStages;
		if ((resourceInfo.usedStages & IShaderCode::Stage::Vertex) == IShaderCode::Stage::Vertex)
		{
			usedStages.push_back(ShaderStage::Vertex);
		}
		if ((resourceInfo.usedStages & IShaderCode::Stage::Fragment) == IShaderCode::Stage::Fragment)
		{
			usedStages.push_back(ShaderStage::Fragment);
		}
		if ((resourceInfo.usedStages & IShaderCode::Stage::Geometry) == IShaderCode::Stage::Geometry)
		{
			usedStages.push_back(ShaderStage::Geometry);
		}
		if ((resourceInfo.usedStages & IShaderCode::Stage::Compute) == IShaderCode::Stage::Compute)
		{
			usedStages.push_back(ShaderStage::Compute);
		}
		for (ShaderStage shaderStage : usedStages)
		{
			size_t stageIndex = ShaderStageToIndex(shaderStage);
			auto& bufferInfo = _globalStateBuffers[stageIndex].bufferInfo;
			bufferInfo.resourceInfo[stageIndex] = resourceInfo;
			if (!LoadStateBufferInfo(stageIndex, bufferInfo, _stateNameToId, resourceInfo))
			{
				_log->Error("Failed to load global constant buffer for shader stage {0}", stageIndex);
				return false;
			}
		}
	}

	// Generate binding descriptor set information
	std::unordered_map<VkDescriptorType, uint32_t> descriptorBindingCounts;
	std::vector<VkDescriptorSetLayoutBinding> descriptors;
	descriptors.reserve(_shaderResourceInfo.size());
	_baseDescriptorBindingNo = std::numeric_limits<int>::max();
	for (const auto& resourceInfo : _shaderResourceInfo)
	{
		// Determine the descriptor type to use
		VkDescriptorType descriptorType;
		switch (resourceInfo.type)
		{
		case IShaderCode::ResourceType::Uniform:
			descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			break;
		case IShaderCode::ResourceType::Texture:
			descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			break;
		case IShaderCode::ResourceType::TextureWithCombinedSampler:
			descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			break;
		case IShaderCode::ResourceType::Sampler:
			descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
			break;
		case IShaderCode::ResourceType::DataArray:
			descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			break;
		case IShaderCode::ResourceType::TexelArray:
			descriptorType = (resourceInfo.writeableResource ? VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER : VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER);
			break;
		case IShaderCode::ResourceType::Input:
		case IShaderCode::ResourceType::Output:
			continue;
		default:
			_log->Warning("Ignoring unsupported shader resource with name \"{0}\" of type {1}", resourceInfo.name, resourceInfo.type);
			continue;
		}

		// If this is a global state buffer, skip it. These resources will be handled separately.
		if ((resourceInfo.type == IShaderCode::ResourceType::Uniform) && (resourceInfo.name == IShaderCode::GlobalConstantBufferName))
		{
			continue;
		}

		// Determine the stage flags to use
		VkShaderStageFlags stageFlags = 0;
		if ((resourceInfo.usedStages & IShaderCode::Stage::Vertex) == IShaderCode::Stage::Vertex)
		{
			stageFlags |= VK_SHADER_STAGE_VERTEX_BIT;
		}
		if ((resourceInfo.usedStages & IShaderCode::Stage::Geometry) == IShaderCode::Stage::Geometry)
		{
			stageFlags |= VK_SHADER_STAGE_GEOMETRY_BIT;
		}
		if ((resourceInfo.usedStages & IShaderCode::Stage::Fragment) == IShaderCode::Stage::Fragment)
		{
			stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
		}
		if ((resourceInfo.usedStages & IShaderCode::Stage::Compute) == IShaderCode::Stage::Compute)
		{
			stageFlags |= VK_SHADER_STAGE_COMPUTE_BIT;
		}

		// Build this descriptor set layout binding
		VkDescriptorSetLayoutBinding descriptor = {};
		descriptor.descriptorCount = 1;
		descriptor.pImmutableSamplers = nullptr;
		descriptor.binding = resourceInfo.binding;
		descriptor.stageFlags = stageFlags;
		descriptor.descriptorType = descriptorType;
		descriptors.push_back(descriptor);

		// Increment the binding counts for this type of descriptor
		++descriptorBindingCounts[descriptorType];

		// Calculate the starting binding number for our descriptor bindings
		_baseDescriptorBindingNo = std::min(_baseDescriptorBindingNo, resourceInfo.binding);
	}

	// Generate global descriptor set information
	std::vector<VkDescriptorSetLayoutBinding> globalDescriptors;
	globalDescriptors.reserve(ShaderStageCount);
	for (size_t stageIndex = 0; stageIndex < ShaderStageCount; ++stageIndex)
	{
		const auto& resourceInfo = _globalStateBuffers[stageIndex].bufferInfo.resourceInfo[stageIndex];
		if (!resourceInfo.valid)
		{
			continue;
		}

		// Determine the stage flags to use
		VkShaderStageFlags stageFlags = 0;
		switch (ShaderIndexToStage(stageIndex))
		{
		case ShaderStage::Vertex:
			stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			break;
		case ShaderStage::Geometry:
			stageFlags = VK_SHADER_STAGE_GEOMETRY_BIT;
			break;
		case ShaderStage::Fragment:
			stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			break;
		case ShaderStage::Compute:
			stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			break;
		default:
			_log->Warning("Encountered global shader resource with name \"{0}\" used in unsupported shader stage {1}", resourceInfo.name, stageIndex);
			continue;
		}

		// Build this descriptor set layout binding
		VkDescriptorSetLayoutBinding descriptor = {};
		descriptor.descriptorCount = 1;
		descriptor.pImmutableSamplers = nullptr;
		descriptor.binding = resourceInfo.binding;
		descriptor.stageFlags = stageFlags;
		descriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		globalDescriptors.push_back(descriptor);
	}

	// Create the binding descriptor set layout
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = (uint32_t)descriptors.size();
	layoutInfo.pBindings = descriptors.data();
	VkResult createDescriptorSetLayoutResult = vkCreateDescriptorSetLayout(_renderer->GetDevice(), &layoutInfo, nullptr, &_descriptorSetLayout);
	if (createDescriptorSetLayoutResult != VK_SUCCESS)
	{
		_log->Error("Could not create descriptor set layout with error code {0}", createDescriptorSetLayoutResult);
		return false;
	}
	_descriptorSetLayouts.push_back(_descriptorSetLayout);

	// Create the global state descriptor set layout
	layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = (uint32_t)globalDescriptors.size();
	layoutInfo.pBindings = globalDescriptors.data();
	VkResult createGlobalDescriptorSetLayoutResult = vkCreateDescriptorSetLayout(_renderer->GetDevice(), &layoutInfo, nullptr, &_globalDescriptorSetLayout);
	if (createGlobalDescriptorSetLayoutResult != VK_SUCCESS)
	{
		_log->Error("Could not create global descriptor set layout with error code {0}", createGlobalDescriptorSetLayoutResult);
		return false;
	}
	_descriptorSetLayouts.push_back(_globalDescriptorSetLayout);

	// If there's at least one resource binding for this shader, allocate a descriptor set layout and pool for resource
	// bindings.
	if (!descriptors.empty())
	{
		_descriptorSetLayoutArray.resize(DescriptorPoolPageSize, _descriptorSetLayout);

		// Calculate the size of each descriptor pool to create
		_bindingDescriptorPoolSizes.clear();
		_bindingDescriptorPoolSizes.reserve(descriptorBindingCounts.size());
		for (const auto& entry : descriptorBindingCounts)
		{
			VkDescriptorPoolSize descriptorPoolSize = {};
			descriptorPoolSize.type = entry.first;
			descriptorPoolSize.descriptorCount = entry.second * DescriptorPoolPageSize;
			_bindingDescriptorPoolSizes.push_back(descriptorPoolSize);
		}
		_bindingDescriptorCountPerSet = (uint32_t)descriptors.size();

		// Pre-allocate the first page of descriptor set layouts for this shader
		AllocateBindingDescriptorSet();
	}

	// If there's at least one global state buffer binding for this shader, allocate a descriptor set layout and pool
	// for global state buffer bindings.
	if (!globalDescriptors.empty())
	{
		_globalDescriptorSetLayoutArray.resize(DescriptorPoolPageSize, _globalDescriptorSetLayout);

		// Calculate the size of each descriptor pool to create
		_globalStateBindingDescriptorPoolSize = {};
		_globalStateBindingDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		_globalStateBindingDescriptorPoolSize.descriptorCount = (uint32_t)(globalDescriptors.size() * DescriptorPoolPageSize);

		// Pre-allocate the first page of descriptor set layouts for this shader
		AllocateGlobalStateDescriptorSet();
	}

	// Create constant buffer for each stage
	if (!CreateGlobalStateBuffer(ShaderStage::Vertex))
	{
		_log->Error("CreateConstantBuffer failed for vertex shader");
		return false;
	}
	if (!CreateGlobalStateBuffer(ShaderStage::Geometry))
	{
		_log->Error("CreateConstantBuffer failed for geometry shader");
		return false;
	}
	if (!CreateGlobalStateBuffer(ShaderStage::Fragment))
	{
		_log->Error("CreateConstantBuffer failed for fragment shader");
		return false;
	}
	if (!CreateGlobalStateBuffer(ShaderStage::Compute))
	{
		_log->Error("CreateConstantBuffer failed for compute shader");
		return false;
	}

	// Record if this is a compute shader
	_isComputeShader = (_shaderBlocks[ShaderStageToIndex(IShaderProgram::ShaderStage::Compute)].code != nullptr);
	return true;
}

//----------------------------------------------------------------------------------------
VkShaderModule VulkanShaderProgram::GetShaderModule(ShaderStage stage) const
{
	return _shaderModules[ShaderStageToIndex(stage)];
}

//----------------------------------------------------------------------------------------
std::string VulkanShaderProgram::GetShaderEntryPointName(ShaderStage stage) const
{
	return _shaderBlocks[ShaderStageToIndex(stage)].entryPointName;
}

//----------------------------------------------------------------------------------------
// Shader input methods
//----------------------------------------------------------------------------------------
bool VulkanShaderProgram::VertexAttributeExists(const Marshal::In<std::string>& name) const
{
	return (_attributeNameToID.find(name) != _attributeNameToID.end());
}

//----------------------------------------------------------------------------------------
bool VulkanShaderProgram::StateValueExists(const Marshal::In<std::string>& name) const
{
	return (_stateNameToId.find(name) != _stateNameToId.end());
}

//----------------------------------------------------------------------------------------
bool VulkanShaderProgram::TextureExists(const Marshal::In<std::string>& name) const
{
	return (_textureNameToID.find(name) != _textureNameToID.end());
}

//----------------------------------------------------------------------------------------
bool VulkanShaderProgram::SamplerExists(const Marshal::In<std::string>& name) const
{
	return (_samplerNameToID.find(name) != _samplerNameToID.end());
}

//----------------------------------------------------------------------------------------
bool VulkanShaderProgram::StateBufferExists(const Marshal::In<std::string>& name) const
{
	return (_stateBufferNameToID.find(name) != _stateBufferNameToID.end());
}

//----------------------------------------------------------------------------------------
bool VulkanShaderProgram::ResourceArrayExists(const Marshal::In<std::string>& name) const
{
	return (_resourceBufferNameToID.find(name) != _resourceBufferNameToID.end());
}

//----------------------------------------------------------------------------------------
VertexAttributeId VulkanShaderProgram::GetVertexAttributeId(const Marshal::In<std::string>& name) const
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
StateValueId VulkanShaderProgram::GetStateValueId(const Marshal::In<std::string>& name) const
{
	auto stateNameToIdIterator = _stateNameToId.find(name);
	if (stateNameToIdIterator != _stateNameToId.end())
	{
		return StateValueId(stateNameToIdIterator->second);
	}
	return StateValueId::Null;
}

//----------------------------------------------------------------------------------------
TextureId VulkanShaderProgram::GetTextureId(const Marshal::In<std::string>& name) const
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
SamplerId VulkanShaderProgram::GetSamplerId(const Marshal::In<std::string>& name) const
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
StateBufferId VulkanShaderProgram::GetStateBufferId(const Marshal::In<std::string>& name) const
{
	auto stateBufferNameToIDIterator = _stateBufferNameToID.find(name);
	if (stateBufferNameToIDIterator == _stateBufferNameToID.end())
	{
		_log->Warning("Failed to locate state buffer with name \"{0}\"", name.Get());
		return StateBufferId::Null;
	}
	return (StateBufferId)stateBufferNameToIDIterator->second;
}

//----------------------------------------------------------------------------------------
ResourceArrayId VulkanShaderProgram::GetResourceArrayId(const Marshal::In<std::string>& name) const
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
size_t VulkanShaderProgram::GetVertexAttributeLocation(VertexAttributeId id)
{
	const auto& attributeInfo = _attributeNameList[(uint32_t)id];
	return _shaderResourceInfo[attributeInfo.shaderResourceIndex].order;
}

//----------------------------------------------------------------------------------------
// State buffer methods
//----------------------------------------------------------------------------------------
SuccessToken VulkanShaderProgram::LoadStateBufferLayoutFromShader(StateBufferId stateBufferID, IStateBufferLayout* stateBufferLayout) const
{
	// Check ID is valid
	if ((size_t)stateBufferID >= _stateBuffers.size())
	{
		_log->Warning("Attempted to load state buffer layout from invalid state buffer ID {0}", stateBufferID);
		return false;
	}

	// Initiate the state buffer layout build process
	auto* stateBufferLayoutResolved = KnownDynamicCast<VulkanStateBufferLayout*>(stateBufferLayout);
	if (!stateBufferLayoutResolved->BeginManualLayoutDefinition())
	{
		_log->Error("Failed to start layout definition build process when loading state buffer layout");
		return false;
	}

	// Add all fields in the state buffer to the state buffer layout
	const auto& stateBufferInfo = _stateBuffers[(size_t)stateBufferID];
	for (const auto& entry : stateBufferInfo.uniformBufferEntries)
	{
		const auto& resource = entry.resourceEntries[0];
		if (resource.vecSize == 1)
		{
			stateBufferLayoutResolved->AddManualField(entry.attributeName, entry.bufferOffset[0], ShaderSupportDataTypeToGraphicsDataType(resource.layoutType), entry.arraySize, entry.arrayStrideInBytes[0], entry.leadingArraySizes, entry.leadingArrayStrides[0]);
		}
		else if (resource.columns == 1)
		{
			stateBufferLayoutResolved->AddManualVector(entry.attributeName, entry.bufferOffset[0], ShaderSupportDataTypeToGraphicsDataType(resource.layoutType), resource.vecSize, entry.arraySize, entry.arrayStrideInBytes[0], entry.leadingArraySizes, entry.leadingArrayStrides[0]);
		}
		else
		{
			stateBufferLayoutResolved->AddManualMatrix(entry.attributeName, entry.bufferOffset[0], ShaderSupportDataTypeToGraphicsDataType(resource.layoutType), resource.vecSize, resource.columns, entry.arraySize, entry.arrayStrideInBytes[0], entry.leadingArraySizes, entry.leadingArrayStrides[0]);
		}
	}

	// Attempt to construct the state buffer layout, and return the result to the caller.
	return stateBufferLayoutResolved->ConstructStateLayout();
}

//----------------------------------------------------------------------------------------
constexpr size_t VulkanShaderProgram::ShaderStageToIndex(ShaderStage stage)
{
	switch (stage)
	{
	case ShaderStage::Vertex:
		return 0;
	case ShaderStage::Geometry:
		return 1;
	case ShaderStage::Fragment:
		return 2;
	case ShaderStage::Compute:
		return 3;
	}
	UNREACHABLE();
	return {};
}

//----------------------------------------------------------------------------------------
constexpr VulkanShaderProgram::ShaderStage VulkanShaderProgram::ShaderIndexToStage(size_t index)
{
	switch (index)
	{
	case 0:
		return ShaderStage::Vertex;
	case 1:
		return ShaderStage::Geometry;
	case 2:
		return ShaderStage::Fragment;
	case 3:
		return ShaderStage::Compute;
	}
	UNREACHABLE();
	return {};
}

//----------------------------------------------------------------------------------------
constexpr IStateBufferLayout::DataType VulkanShaderProgram::ShaderSupportDataTypeToGraphicsDataType(IShaderCode::DataType dataType)
{
	switch (dataType)
	{
	case IShaderCode::DataType::Null:
		return IStateBufferLayout::DataType::Null;
	case IShaderCode::DataType::Boolean:
		return IStateBufferLayout::DataType::Boolean;
	case IShaderCode::DataType::Int32:
		return IStateBufferLayout::DataType::Int32;
	case IShaderCode::DataType::UInt32:
		return IStateBufferLayout::DataType::UInt32;
	case IShaderCode::DataType::Float32:
		return IStateBufferLayout::DataType::Float32;
	case IShaderCode::DataType::Float64:
		return IStateBufferLayout::DataType::Float64;
	}
	return IStateBufferLayout::DataType::Null;
}

//----------------------------------------------------------------------------------------
bool VulkanShaderProgram::LoadStateBufferInfo(size_t shaderStageIndex, StateBufferInfo& bufferInfo, std::unordered_map<std::string, StateValueId>& attributeNamesToIDs, const IShaderCode::Resource& shaderResource) const
{
	// Retrieve basic information on the constant buffer
	std::string constantBufferName = shaderResource.name;
	bufferInfo.name = constantBufferName;
	bufferInfo.totalBufferSize = shaderResource.size;
	bufferInfo.resourceInfo[shaderStageIndex] = shaderResource;

	// Load information on each variable in the state buffer
	for (const auto& member : shaderResource.fields)
	{
		// Add this member to the list of members in the buffer, recursing into nested structures.
		std::vector<size_t> arraySizes;
		std::vector<size_t> arrayStrides;
		if (member.type == IShaderCode::ResourceType::StructField)
		{
			if (!LoadStateBufferStructEntry(shaderStageIndex, bufferInfo, attributeNamesToIDs, member.name, arraySizes, arrayStrides, member.offset, member))
			{
				_log->Error("LoadStateBufferStructEntry failed");
				return false;
			}
		}
		else
		{
			std::string variableName = std::string(member.name) + (!member.arraySizes.empty() ? "[]" : "");
			if (!LoadStateBufferVariableEntry(shaderStageIndex, bufferInfo, attributeNamesToIDs, variableName, arraySizes, arrayStrides, member, member.offset))
			{
				_log->Error("LoadStateBufferVariableEntry failed");
				return false;
			}
		}
	}
	return true;
}

//----------------------------------------------------------------------------------------
bool VulkanShaderProgram::LoadStateBufferStructEntry(size_t shaderStageIndex, StateBufferInfo& bufferInfo, std::unordered_map<std::string, StateValueId>& attributeNamesToIDs, const std::string& leadingVariableName, const std::vector<size_t>& leadingArraySizes, const std::vector<size_t>& leadingArrayStrides, size_t leadingBufferOffsetInBytes, const IShaderCode::Resource& shaderResource) const
{
	// Build array size and stride arrays to pass to our members
	auto arraySizes = leadingArraySizes;
	auto arrayStrides = leadingArrayStrides;
	bool isArray = !shaderResource.arraySizes.empty();
	if (isArray)
	{
		// Add the array size and stride to the dimensions passed to our members
		size_t arraySize = 1;
		for (uint32_t nextArraySize : shaderResource.arraySizes)
		{
			arraySize *= nextArraySize;
		}
		arraySizes.push_back(arraySize);
		arrayStrides.push_back(shaderResource.elementStride);
	}

	// Add each member of the structure to the list of members in the containing state buffer
	for (const auto& member : shaderResource.fields)
	{
		// Calculate the byte offset of this structure within the parent buffer. If the structure is an array, this will
		// be the offset to the first member of the array.
		size_t bufferOffsetInBytes = leadingBufferOffsetInBytes + member.offset;

		// Generate the name of this member
		std::string variableName = leadingVariableName + (isArray ? ("[].") : ".") + member.name;

		// Add the members of this field to the list of members in the state buffer, recursing into structures as
		// required.
		if (member.type == IShaderCode::ResourceType::StructField)
		{
			// Recurse into the nested structure
			if (!LoadStateBufferStructEntry(shaderStageIndex, bufferInfo, attributeNamesToIDs, variableName, arraySizes, arrayStrides, bufferOffsetInBytes, member))
			{
				_log->Error("LoadStateBufferStructEntry failed");
				return false;
			}
		}
		else
		{
			// Record information on this state buffer field
			variableName += (!member.arraySizes.empty() ? "[]" : "");
			if (!LoadStateBufferVariableEntry(shaderStageIndex, bufferInfo, attributeNamesToIDs, variableName, arraySizes, arrayStrides, member, bufferOffsetInBytes))
			{
				_log->Error("LoadStateBufferVariableEntry failed");
				return false;
			}
		}
	}
	return true;
}

//----------------------------------------------------------------------------------------
bool VulkanShaderProgram::LoadStateBufferVariableEntry(size_t shaderStageIndex, StateBufferInfo& bufferInfo, std::unordered_map<std::string, StateValueId>& attributeNamesToIDs, const std::string& variableName, const std::vector<size_t>& leadingArraySizes, const std::vector<size_t>& leadingArrayStrides, const IShaderCode::Resource& shaderResource, size_t startOffset) const
{
	// Decode the type information for the field
	IStateBufferLayout::DataType type;
	if (!StateBufferEntryNativeTypeToBufferLayoutType(shaderResource, type))
	{
		_log->Error("State buffer contains an unsupported state value of class {0} and type {1} when loading state buffer layout", shaderResource.type, shaderResource.layoutType);
		return false;
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
		StateBufferEntryInfo emptyBufferEntry;
		for (bool& nextBufferDefined : emptyBufferEntry.defined)
		{
			nextBufferDefined = false;
		}
		bufferInfo.uniformBufferEntries.resize(uniformID + 1, emptyBufferEntry);
	}
	StateBufferEntryInfo& entryInfo = bufferInfo.uniformBufferEntries[uniformID];
	entryInfo.defined[shaderStageIndex] = true;
	entryInfo.attributeID = (StateValueId)uniformID;
	entryInfo.attributeName = variableName;
	entryInfo.entrySizeInBytes = shaderResource.size;
	entryInfo.bufferOffset[shaderStageIndex] = startOffset;
	entryInfo.resourceEntries[shaderStageIndex] = shaderResource;
	entryInfo.type = type;

	// Calculate the array size and stride
	entryInfo.arrayStrideInBytes[shaderStageIndex] = shaderResource.elementStride;
	size_t arraySize = 1;
	for (uint32_t nextArraySize : shaderResource.arraySizes)
	{
		arraySize *= nextArraySize;
	}
	entryInfo.arraySize = arraySize;
	entryInfo.dataSizeInBytes = shaderResource.size;

	// Populate the array sizes and strides for this entry
	entryInfo.leadingArraySizes = leadingArraySizes;
	entryInfo.leadingArrayStrides[shaderStageIndex] = leadingArrayStrides;
	entryInfo.arraySizes = leadingArraySizes;
	entryInfo.arrayStrides[shaderStageIndex] = leadingArrayStrides;
	if (!shaderResource.arraySizes.empty())
	{
		entryInfo.arraySizes.push_back(entryInfo.arraySize);
		entryInfo.arrayStrides[shaderStageIndex].push_back(entryInfo.arrayStrideInBytes[shaderStageIndex]);
	}

	// Sanity check for the location and size of the uniform value in the buffer
	//##FIX## This doesn't take the array dimensions into account
	assert((startOffset + entryInfo.dataSizeInBytes) <= bufferInfo.totalBufferSize);
	return true;
}

//----------------------------------------------------------------------------------------
bool VulkanShaderProgram::StateBufferEntryNativeTypeToBufferLayoutType(const IShaderCode::Resource& resourceInfo, IStateBufferLayout::DataType& type) const
{
	// Determine the underlying element data type
	switch (resourceInfo.layoutType)
	{
	case IShaderCode::DataType::Boolean:
		type = IStateBufferLayout::DataType::Boolean;
		break;
	case IShaderCode::DataType::Int32:
		type = IStateBufferLayout::DataType::Int32;
		break;
	case IShaderCode::DataType::UInt32:
		type = IStateBufferLayout::DataType::UInt32;
		break;
	case IShaderCode::DataType::Float32:
		type = IStateBufferLayout::DataType::Float32;
		break;
	case IShaderCode::DataType::Float64:
		type = IStateBufferLayout::DataType::Float64;
		break;
	default:
		return false;
	}
	return true;
}

//----------------------------------------------------------------------------------------
// State value methods
//----------------------------------------------------------------------------------------
void VulkanShaderProgram::UpdateStateValue(StateValueId stateId, const uint8_t* data, size_t dataSizeInBytes, const size_t* arrayIndices, size_t arrayIndexCount)
{
	// Ensure the supplied uniform ID is valid
	if ((size_t)stateId >= _stateNameToId.size())
	{
		_log->Error("Attempted to modify state value with an invalid ID of {0}", stateId);
		return;
	}

	// Update the value of the target uniform in each shader constant buffer
	for (unsigned int shaderStageID = 0; shaderStageID < ShaderStageCount; ++shaderStageID)
	{
		// Attempt to retrieve a uniform buffer entry for the target uniform value in the constant buffer
		GlobalStateBufferInfo& constantBuffer = _globalStateBuffers[shaderStageID];
		if (!constantBuffer.bufferExists || (constantBuffer.bufferInfo.uniformBufferEntries.size() <= (size_t)stateId))
		{
			continue;
		}
		const auto& bufferEntry = constantBuffer.bufferInfo.uniformBufferEntries[(size_t)stateId];
		if (!bufferEntry.defined[shaderStageID])
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
			arrayOffset += arrayIndex * bufferEntry.arrayStrides[shaderStageID][i];
		}

		// Update the entry in the buffer
		unsigned char* uniformLocation = constantBuffer.bufferContents.data() + bufferEntry.bufferOffset[shaderStageID] + arrayOffset;
		size_t sizeToCopy = std::min(dataSizeInBytes, bufferEntry.entrySizeInBytes);
		std::memcpy(uniformLocation, data, sizeToCopy);

		// Flag that the buffer is now dirty
		constantBuffer.isDirty = true;
	}
}

//----------------------------------------------------------------------------------------
void VulkanShaderProgram::SetConstantValue(StateValueId stateId, const uint8_t* data, size_t dataSizeInBytes, const size_t* arrayIndices, size_t arrayIndexCount)
{
	//##TODO## Vulkan does support shader constants but currently the Direct3D approach is used
	UpdateStateValue(stateId, data, dataSizeInBytes, arrayIndices, arrayIndexCount);
}

//----------------------------------------------------------------------------------------
void VulkanShaderProgram::PushGlobalStateBufferState()
{
	for (auto& bufferInfo : _globalStateBuffers)
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
void VulkanShaderProgram::PopGlobalStateBufferState()
{
	for (auto& bufferInfo : _globalStateBuffers)
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
void VulkanShaderProgram::RestoreGlobalStateBufferBaseline()
{
	for (auto& bufferInfo : _globalStateBuffers)
	{
		if (bufferInfo.bufferExists)
		{
			bufferInfo.bufferContents = bufferInfo.bufferStack[bufferInfo.bufferStackEntries - 1];
			bufferInfo.isDirty = true;
		}
	}
}

//----------------------------------------------------------------------------------------
bool VulkanShaderProgram::CreateGlobalStateBuffer(ShaderStage stage)
{
	//##TODO## Review and clean this up
	size_t shaderStageIndex = ShaderStageToIndex(stage);
	IShaderCode::Resource& resource = _globalStateBuffers[shaderStageIndex].bufferInfo.resourceInfo[shaderStageIndex];
	if (!resource.valid)
	{
		return true;
	}

	GlobalStateBufferInfo& constantBufferInfo = _globalStateBuffers[shaderStageIndex];
	StateBufferBindPointEntry bindPoint;
	bindPoint.bindPoint = resource.binding;
	constantBufferInfo.bufferInfo.bindPoints.push_back(bindPoint);

	// Resize our local mirror of the constant buffer to match the constant buffer size
	constantBufferInfo.bufferContents.resize(constantBufferInfo.bufferInfo.totalBufferSize, 0);

	// Push the initial buffer state to the stack
	constantBufferInfo.bufferStack.push_back(constantBufferInfo.bufferContents);
	constantBufferInfo.bufferStackEntries = 1;

	// Allocate a new state buffer to handle efficient updates to global constant state
	size_t allocatedPageCount = 1;
	std::unique_ptr<VulkanStateBuffer> stateBuffer = std::make_unique<VulkanStateBuffer>(_log, _renderer, true);
	stateBuffer->SetManualPageSize(constantBufferInfo.bufferInfo.totalBufferSize);
	stateBuffer->SetPageSettings((uint32_t)allocatedPageCount, true);
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
// Global constant buffer session methods
//----------------------------------------------------------------------------------------
void VulkanShaderProgram::ResetGlobalConstantBufferState()
{
	_nextGlobalStateBindingDescriptorSetIndex = 0;
	for (uint32_t i = 0; i < ShaderStageCount; ++i)
	{
		_globalStateBufferBuildingSession.nextPageNumbers[i] = 0;
		_globalStateBufferBuildingSession.lastWrittenPageBlockIndex[i] = 0;
		_globalStateBufferBuildingSession.buffersExist[i] = _globalStateBuffers[i].bufferExists;
		_globalStateBufferBuildingSession.stateBuffers[i] = _globalStateBuffers[i].stateBuffer.get();
	}
}

//----------------------------------------------------------------------------------------
void VulkanShaderProgram::BeginGlobalConstantBufferBuildingSession(VkCommandBuffer commandBuffer, GlobalStateBufferBuildingSession& stateInfo)
{
	stateInfo = _globalStateBufferBuildingSession;
}

//----------------------------------------------------------------------------------------
void VulkanShaderProgram::CompleteGlobalConstantBufferBuildingSession(VkCommandBuffer commandBuffer, GlobalStateBufferBuildingSession& stateInfo)
{
	_globalStateBufferBuildingSession = stateInfo;
	for (uint32_t i = 0; i < ShaderStageCount; ++i)
	{
		GlobalStateBufferInfo& bufferInfo = _globalStateBuffers[i];
		if (bufferInfo.bufferExists)
		{
			bufferInfo.stateBuffer->CompletePendingDataWritesForPageBlock(commandBuffer, _globalStateBufferBuildingSession.lastWrittenPageBlockIndex[i]);
		}
	}

	//##TODO## You would think this would be a performance win, batching all our global state descriptor set writes into
	//a single vkUpdateDescriptorSets call. Stress tests with 100000 separate renderables with unique state per
	//renderable showed a measurable performance degradation however across both Intel and AMD hardware, with NVidia
	//hardware untested. It's possible that there's no real advantage to batching at all, and the earlier dispatch of
	//the write requests to the hardware queues the operation, which completes before the next write is requested,
	//making the updates virtually free when we do them immediately, but causing a stall before drawing can proceed when
	//we defer them to the end. The code has been kept for reference, in case the situation changes with future driver
	//updates, however at this point (2021-05-09) it actually performs better to make vkUpdateDescriptorSets calls one
	//at a time within the GenerateGlobalConstantBufferBindings method.

	//// If we need to extend our array of VkWriteDescriptorSet objects, do it now.
	//size_t newGlobalStateEntryIndex = _globalStateDesriptorSetWrites.size();
	//if (_pendingDescriptorWrites.size() > _globalStateDesriptorSetWrites.size())
	//{
	//	// Create a default VkWriteDescriptorSet object with our common fields already set, to provide during the resize
	//	// operation.
	//	VkWriteDescriptorSet defaultDescriptorWrite = {};
	//	defaultDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	//	defaultDescriptorWrite.dstArrayElement = 0;
	//	defaultDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	//	defaultDescriptorWrite.descriptorCount = 1;
	//	defaultDescriptorWrite.pImageInfo = nullptr;
	//	defaultDescriptorWrite.pTexelBufferView = nullptr;

	//	// Resize our array of VkWriteDescriptorSet objects
	//	_globalStateDescriptorBufferInfo.resize(_pendingDescriptorWrites.size());
	//	_globalStateDesriptorSetWrites.resize(_pendingDescriptorWrites.size(), defaultDescriptorWrite);
	//	for (size_t i = newGlobalStateEntryIndex; i < _pendingDescriptorWrites.size(); ++i)
	//	{
	//		_globalStateDesriptorSetWrites[i].pBufferInfo = &_globalStateDescriptorBufferInfo[i];
	//	}
	//}

	//// Load information on each pending descriptor write into our data arrays
	//for (size_t i = 0; i < _pendingDescriptorWrites.size(); ++i)
	//{
	//	const PendingDescriptorWriteInfo& descriptorWriteInfo = _pendingDescriptorWrites[i];
	//	VkDescriptorBufferInfo&  bufferInfo = _globalStateDescriptorBufferInfo[i];
	//	VkWriteDescriptorSet& descriptorWrite = _globalStateDesriptorSetWrites[i];
	//	bufferInfo.buffer = descriptorWriteInfo.stateBuffer->GetNativeBuffer(descriptorWriteInfo.pageNumber);
	//	bufferInfo.offset = descriptorWriteInfo.stateBuffer->GetPageOffset(descriptorWriteInfo.pageNumber);
	//	bufferInfo.range = descriptorWriteInfo.stateBuffer->GetPageSize();
	//	descriptorWrite.dstSet = _globalStateDescriptorSets[descriptorWriteInfo.descriptorIndex];
	//	descriptorWrite.dstBinding = descriptorWriteInfo.binding;
	//}

	//// Perform all our descriptor set writes
	//vkUpdateDescriptorSets(_renderer->GetDevice(), _pendingDescriptorWrites.size(), _globalStateDesriptorSetWrites.data(), 0, nullptr);

	//// Clear the set of pending descriptor writes
	//_pendingDescriptorWrites.clear();
}

//----------------------------------------------------------------------------------------
void VulkanShaderProgram::GenerateGlobalConstantBufferBindings(VkCommandBuffer commandBuffer, GlobalStateBufferBuildingSession& stateInfo, GlobalConstantBufferBindingInfo& bindingInfo)
{
	// Flag that no bindings have been generated so far based on the current buffer state
	bindingInfo.hasBindings = false;

	// Flush the contents of any dirty constant buffers
	for (uint32_t shaderStageID = 0; shaderStageID < ShaderStageCount; ++shaderStageID)
	{
		// If this shader stage doesn't have a constant buffer, or the buffer isn't marked as dirty, skip it.
		GlobalStateBufferInfo& constantBuffer = _globalStateBuffers[shaderStageID];
		if (!constantBuffer.bufferExists || !constantBuffer.isDirty)
		{
			bindingInfo.bindingSet[shaderStageID] = false;
			continue;
		}

		// Allocate a new descriptor set index to store the unique state values in this buffer
		if (!bindingInfo.hasBindings)
		{
			bindingInfo.descriptorIndex = AllocateGlobalStateDescriptorSet();
		}

		// Store information on this constant buffer binding in the binding info
		uint32_t nextPageNo = stateInfo.nextPageNumbers[shaderStageID];
		bindingInfo.hasBindings = true;
		bindingInfo.pageNumbers[shaderStageID] = nextPageNo;
		bindingInfo.bindingSet[shaderStageID] = true;

		// Update the contents of the constant buffer for this shader stage
		if (nextPageNo >= constantBuffer.allocatedPageCount)
		{
			constantBuffer.allocatedPageCount += constantBuffer.stateBuffer->GetPagesPerPageBlock();
			constantBuffer.stateBuffer->ResizePageCount((uint32_t)constantBuffer.allocatedPageCount).IgnoreResult();
		}
		uint32_t pageIndexInBlock;
		uint32_t pageBlockIndex;
		constantBuffer.stateBuffer->SetRawPageDataWithReturnedPageIndex(nextPageNo, constantBuffer.bufferContents.data(), pageIndexInBlock, pageBlockIndex);
		stateInfo.nextPageNumbers[shaderStageID] = ++nextPageNo;
		stateInfo.lastWrittenPageBlockIndex[shaderStageID] = pageBlockIndex;

		// If we've just started a new page block in the constant buffer, commit changes to the previous page block.
		if ((pageIndexInBlock == 0) && (pageBlockIndex > 0))
		{
			constantBuffer.stateBuffer->CompletePendingDataWritesForPageBlock(commandBuffer, pageBlockIndex - 1);
		}

		// Write binding to descriptor set
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = constantBuffer.stateBuffer->GetNativeBuffer(bindingInfo.pageNumbers[shaderStageID]);
		bufferInfo.offset = constantBuffer.stateBuffer->GetPageOffset(bindingInfo.pageNumbers[shaderStageID]);
		bufferInfo.range = constantBuffer.stateBuffer->GetPageSize();
		VkWriteDescriptorSet descriptorWrite = {};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = _globalStateDescriptorSets[bindingInfo.descriptorIndex];
		descriptorWrite.dstBinding = _globalStateBuffers[shaderStageID].bufferInfo.resourceInfo[shaderStageID].binding;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &bufferInfo;
		descriptorWrite.pImageInfo = nullptr;
		descriptorWrite.pTexelBufferView = nullptr;
		vkUpdateDescriptorSets(_renderer->GetDevice(), 1, &descriptorWrite, 0, nullptr);

		//##TODO## See the CompleteGlobalConstantBufferBuildingSession method for notes on why this alternate approach
		//to updating descriptor sets is currently disabled
		//// Record information on the pending descriptor write for this global state buffer page
		//PendingDescriptorWriteInfo& descriptorWriteInfo = _pendingDescriptorWrites.emplace_back();
		//descriptorWriteInfo.stateBuffer = constantBuffer.stateBuffer.get();
		//descriptorWriteInfo.binding = _globalStateBufferInfo.resourceInfo[shaderStageID].binding;
		//descriptorWriteInfo.descriptorIndex = bindingInfo.descriptorIndex;
		//descriptorWriteInfo.pageNumber = bindingInfo.pageNumbers[shaderStageID];

		// Remove the dirty flag for the target constant buffer
		constantBuffer.isDirty = false;
	}
}

//----------------------------------------------------------------------------------------
void VulkanShaderProgram::ApplyGlobalConstantBufferBindings(VkCommandBuffer commandBuffer, const GlobalConstantBufferBindingInfo& bindingInfo, VkPipelineLayout pipelineLayout)
{
	// If no bindings are set, abort any further processing.
	if (!bindingInfo.hasBindings)
	{
		return;
	}

	vkCmdBindDescriptorSets(commandBuffer, (_isComputeShader ? VK_PIPELINE_BIND_POINT_COMPUTE : VK_PIPELINE_BIND_POINT_GRAPHICS), pipelineLayout, 1, 1, &_globalStateDescriptorSets[bindingInfo.descriptorIndex], 0, nullptr);
}

//----------------------------------------------------------------------------------------
// Shader binding methods
//----------------------------------------------------------------------------------------
size_t VulkanShaderProgram::GetVertexAttributeCount() const
{
	return _vertexInputBindingDescriptions.size();
}

//----------------------------------------------------------------------------------------
const std::vector<VkVertexInputBindingDescription>& VulkanShaderProgram::GetVertexBindingDescriptions() const
{
	return _vertexInputBindingDescriptions;
}

//----------------------------------------------------------------------------------------
const std::vector<VkVertexInputAttributeDescription>& VulkanShaderProgram::GetVertexAttributeDescriptions() const
{
	return _vertexInputAttributeDescriptions;
}

//----------------------------------------------------------------------------------------
void VulkanShaderProgram::GetBindPointsForStateBuffer(StateBufferId stateBufferId, const StateBufferBindPointEntry*& bindPoints, size_t& bindPointCount) const
{
	if ((stateBufferId == graphics::StateBufferId::Null) || ((uint32_t)stateBufferId >= _stateBuffers.size()))
	{
		_log->Warning("Attempted to retrieve bind point for state buffer with ID \"{0}\"", stateBufferId);
		bindPoints = nullptr;
		bindPointCount = 0;
		return;
	}

	const StateBufferInfo& stateBufferResource = _stateBuffers[(size_t)stateBufferId];
	bindPoints = stateBufferResource.bindPoints.data();
	bindPointCount = stateBufferResource.bindPoints.size();
}

//----------------------------------------------------------------------------------------
void VulkanShaderProgram::GetBindPointsForTexture(TextureId textureId, const TextureBindPointEntry*& bindPoints, size_t& bindPointCount) const
{
	if ((textureId == graphics::TextureId::Null) || ((uint32_t)textureId >= _textureResources.size()))
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
void VulkanShaderProgram::GetBindPointsForSampler(SamplerId samplerId, const SamplerBindPointEntry*& bindPoints, size_t& bindPointCount) const
{
	if ((samplerId == graphics::SamplerId::Null) || ((uint32_t)samplerId >= _samplerResources.size()))
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
void VulkanShaderProgram::GetBindPointsForResourceArray(ResourceArrayId resourceArrayId, const ResourceArrayBindPointEntry*& bindPoints, size_t& bindPointCount) const
{
	if ((resourceArrayId == graphics::ResourceArrayId::Null) || ((uint32_t)resourceArrayId >= _resourceBufferResources.size()))
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
void VulkanShaderProgram::BuildNullDescriptorFallbacks() const
{
	// Calculate the required size of our binding arrays
	size_t bufferInfoCount = 0;
	size_t imageInfoCount = 0;
	size_t texelViewCount = 0;
	size_t descriptorWriteCount = 0;
	for (const auto& stateBufferInfo : _stateBuffers)
	{
		bufferInfoCount += stateBufferInfo.bindPoints.size();
		descriptorWriteCount += stateBufferInfo.bindPoints.size();
	}
	for (const auto& textureInfo : _textureResources)
	{
		imageInfoCount += textureInfo.bindPoints.size();
		descriptorWriteCount += textureInfo.bindPoints.size();
	}
	for (const auto& samplerInfo : _samplerResources)
	{
		imageInfoCount += samplerInfo.bindPoints.size();
		descriptorWriteCount += samplerInfo.bindPoints.size();
	}
	for (const auto& resourceBufferInfo : _resourceBufferResources)
	{
		if (resourceBufferInfo.isAttachedDataArrayCounter)
		{
			continue;
		}
		for (const auto& bindPoint : resourceBufferInfo.bindPoints)
		{
			++descriptorWriteCount;
			if (resourceBufferInfo.isDataArray)
			{
				++bufferInfoCount;
				if (resourceBufferInfo.hasAttachedCounterBuffer && (bindPoint.counterBindPoint != 0xFFFFFFFF))
				{
					++bufferInfoCount;
					++descriptorWriteCount;
				}
			}
			else
			{
				++texelViewCount;
			}
		}
	}

	// Resize our set of data arrays to use in our fallback buffer bindings
	_nullDescriptorFallbackBufferInfo.reserve(bufferInfoCount);
	_nullDescriptorFallbackImageInfo.reserve(imageInfoCount);
	_nullDescriptorFallbackBufferViews.reserve(texelViewCount);
	_nullDescriptorFallbackDescriptorWrites.reserve(descriptorWriteCount);

	// Add fallback bindings for each state buffer
	for (const auto& stateBufferInfo : _stateBuffers)
	{
		for (const auto& bindPoint : stateBufferInfo.bindPoints)
		{
			auto& bufferInfo = _nullDescriptorFallbackBufferInfo.emplace_back();
			bufferInfo.buffer = _renderer->GetNullDescriptorFallbackUniformBuffer();
			bufferInfo.offset = 0;
			bufferInfo.range = VK_WHOLE_SIZE;

			auto& descriptorWrite = _nullDescriptorFallbackDescriptorWrites.emplace_back();
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = VK_NULL_HANDLE;
			descriptorWrite.dstBinding = bindPoint.bindPoint;
			descriptorWrite.descriptorCount = 1;
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrite.pBufferInfo = &bufferInfo;
		}
	}

	// Add fallback bindings for each texture resource
	for (const auto& textureInfo : _textureResources)
	{
		VkImageViewType imageViewType = (textureInfo.imageViewType != VK_IMAGE_VIEW_TYPE_MAX_ENUM ? textureInfo.imageViewType : VK_IMAGE_VIEW_TYPE_2D);
		for (const auto& bindPoint : textureInfo.bindPoints)
		{
			auto& imageInfo = _nullDescriptorFallbackImageInfo.emplace_back();
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = _renderer->GetNullDescriptorFallbackTextureView(imageViewType);
			if (bindPoint.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
			{
				imageInfo.sampler = _renderer->GetNullDescriptorFallbackSampler();
			}

			auto& descriptorWrite = _nullDescriptorFallbackDescriptorWrites.emplace_back();
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = VK_NULL_HANDLE;
			descriptorWrite.dstBinding = bindPoint.bindPoint;
			descriptorWrite.descriptorCount = 1;
			descriptorWrite.descriptorType = bindPoint.descriptorType;
			descriptorWrite.pImageInfo = &imageInfo;
		}
	}

	// Add fallback bindings for each sampler
	for (const auto& samplerInfo : _samplerResources)
	{
		for (const auto& bindPoint : samplerInfo.bindPoints)
		{
			auto& imageInfo = _nullDescriptorFallbackImageInfo.emplace_back();
			imageInfo.sampler = _renderer->GetNullDescriptorFallbackSampler();

			auto& descriptorWrite = _nullDescriptorFallbackDescriptorWrites.emplace_back();
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = VK_NULL_HANDLE;
			descriptorWrite.dstBinding = bindPoint.bindPoint;
			descriptorWrite.descriptorCount = 1;
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
			descriptorWrite.pImageInfo = &imageInfo;
		}
	}

	// Add fallback bindings for each resource array
	for (const auto& resourceBufferInfo : _resourceBufferResources)
	{
		if (resourceBufferInfo.isAttachedDataArrayCounter)
		{
			continue;
		}

		for (const auto& bindPoint : resourceBufferInfo.bindPoints)
		{
			auto& descriptorWrite = _nullDescriptorFallbackDescriptorWrites.emplace_back();
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = VK_NULL_HANDLE;
			descriptorWrite.dstBinding = bindPoint.bindPoint;
			descriptorWrite.descriptorCount = 1;
			if (resourceBufferInfo.isDataArray)
			{
				auto& bufferInfo = _nullDescriptorFallbackBufferInfo.emplace_back();
				bufferInfo.buffer = _renderer->GetNullDescriptorFallbackStorageBuffer();
				bufferInfo.offset = 0;
				bufferInfo.range = VK_WHOLE_SIZE;
				descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				descriptorWrite.pBufferInfo = &bufferInfo;

				if (resourceBufferInfo.hasAttachedCounterBuffer && (bindPoint.counterBindPoint != 0xFFFFFFFF))
				{
					auto& counterBufferInfo = _nullDescriptorFallbackBufferInfo.emplace_back();
					counterBufferInfo.buffer = _renderer->GetNullDescriptorFallbackStorageBuffer();
					counterBufferInfo.offset = 0;
					counterBufferInfo.range = VK_WHOLE_SIZE;

					auto& counterDescriptorWrite = _nullDescriptorFallbackDescriptorWrites.emplace_back();
					counterDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					counterDescriptorWrite.dstSet = VK_NULL_HANDLE;
					counterDescriptorWrite.dstBinding = bindPoint.counterBindPoint;
					counterDescriptorWrite.descriptorCount = 1;
					counterDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					counterDescriptorWrite.pBufferInfo = &counterBufferInfo;
				}
			}
			else
			{
				auto& bufferView = _nullDescriptorFallbackBufferViews.emplace_back(_renderer->GetNullDescriptorFallbackTexelBufferView((VkFormat)_shaderResourceInfo[resourceBufferInfo.shaderResourceIndex].nativeVulkanFormat, bindPoint.writeable));
				descriptorWrite.descriptorType = (bindPoint.writeable ? VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER : VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER);
				descriptorWrite.pTexelBufferView = &bufferView;
			}
		}
	}

	// Flag that our set of fallback bindings has now been defined
	_builtNullDescriptorFallback = true;
}

//----------------------------------------------------------------------------------------
void VulkanShaderProgram::BindNullDescriptorFallbacks(size_t setIndex) const
{
	// Build our set of fallback bindings if required
	if (!_builtNullDescriptorFallback)
	{
		BuildNullDescriptorFallbacks();
	}

	// Bind our set of fallback bindings where no null descriptor feature is available
	VkDescriptorSet descriptorSet = GetDescriptorSet(setIndex);
	for (auto& entry : _nullDescriptorFallbackDescriptorWrites)
	{
		entry.dstSet = descriptorSet;
	}
	vkUpdateDescriptorSets(_renderer->GetDevice(), (uint32_t)_nullDescriptorFallbackDescriptorWrites.size(), _nullDescriptorFallbackDescriptorWrites.data(), 0, nullptr);
}

//----------------------------------------------------------------------------------------
// Descriptor methods
//----------------------------------------------------------------------------------------
void VulkanShaderProgram::ClearDescriptorSets()
{
	_nextBindingDescriptorSetIndex = 0;
}

//----------------------------------------------------------------------------------------
size_t VulkanShaderProgram::AllocateBindingDescriptorSet()
{
	// If no descriptor set is required, abort any further processing.
	if (_descriptorSetLayoutArray.empty())
	{
		return std::numeric_limits<size_t>::max();
	}

	// If we've run out of available descriptor sets, allocate a new page of descriptor sets.
	if (_nextBindingDescriptorSetIndex >= _bindingDescriptorSets.size())
	{
		// Allocate a new descriptor pool
		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = (uint32_t)_bindingDescriptorPoolSizes.size();
		poolInfo.pPoolSizes = _bindingDescriptorPoolSizes.data();
		poolInfo.maxSets = DescriptorPoolPageSize;
		poolInfo.flags = 0;
		VkDescriptorPool bindingDescriptorPool;
		VkResult createDescriptorPoolResult = vkCreateDescriptorPool(_renderer->GetDevice(), &poolInfo, nullptr, &bindingDescriptorPool);
		if (createDescriptorPoolResult != VK_SUCCESS)
		{
			_log->Error("vkCreateDescriptorPool failed with error code {0}", createDescriptorPoolResult);
			return std::numeric_limits<size_t>::max();
		}
		_bindingDescriptorPools.push_back(bindingDescriptorPool);

		// Fill the new descriptor pool with allocated descriptor sets based on the binding descriptor set layout for
		// this shader
		VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
		descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorSetAllocateInfo.descriptorPool = bindingDescriptorPool;
		descriptorSetAllocateInfo.descriptorSetCount = DescriptorPoolPageSize;
		descriptorSetAllocateInfo.pSetLayouts = _descriptorSetLayoutArray.data();
		size_t descriptorSetBaseIndex = _bindingDescriptorSets.size();
		_bindingDescriptorSets.resize(descriptorSetBaseIndex + DescriptorPoolPageSize);
		VkResult allocateDescriptorSetsResult = vkAllocateDescriptorSets(_renderer->GetDevice(), &descriptorSetAllocateInfo, &_bindingDescriptorSets[descriptorSetBaseIndex]);
		if (allocateDescriptorSetsResult != VK_SUCCESS)
		{
			_log->Error("vkAllocateDescriptorSets failed with error code {0}", allocateDescriptorSetsResult);
			return std::numeric_limits<size_t>::max();
		}
	}

	// Return the index of the next available descriptor set
	return _nextBindingDescriptorSetIndex++;
}

//----------------------------------------------------------------------------------------
size_t VulkanShaderProgram::AllocateGlobalStateDescriptorSet()
{
	// If no descriptor set is required, abort any further processing.
	if (_globalDescriptorSetLayoutArray.empty())
	{
		return std::numeric_limits<size_t>::max();
	}

	// If we've run out of available descriptor sets, allocate a new page of descriptor sets.
	if (_nextGlobalStateBindingDescriptorSetIndex >= _globalStateDescriptorSets.size())
	{
		// Allocate a new descriptor pool
		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 1;
		poolInfo.pPoolSizes = &_globalStateBindingDescriptorPoolSize;
		poolInfo.maxSets = DescriptorPoolPageSize;
		poolInfo.flags = 0;
		VkDescriptorPool bindingDescriptorPool;
		VkResult createDescriptorPoolResult = vkCreateDescriptorPool(_renderer->GetDevice(), &poolInfo, nullptr, &bindingDescriptorPool);
		if (createDescriptorPoolResult != VK_SUCCESS)
		{
			_log->Error("vkCreateDescriptorPool failed with error code {0}", createDescriptorPoolResult);
			return std::numeric_limits<size_t>::max();
		}
		_globalStateBindingDescriptorPools.push_back(bindingDescriptorPool);

		// Fill the new descriptor pool with allocated descriptor sets based on the binding descriptor set layout for
		// this shader
		VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
		descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorSetAllocateInfo.descriptorPool = bindingDescriptorPool;
		descriptorSetAllocateInfo.descriptorSetCount = DescriptorPoolPageSize;
		descriptorSetAllocateInfo.pSetLayouts = _globalDescriptorSetLayoutArray.data();
		size_t descriptorSetBaseIndex = _globalStateDescriptorSets.size();
		_globalStateDescriptorSets.resize(descriptorSetBaseIndex + DescriptorPoolPageSize);
		VkResult allocateDescriptorSetsResult = vkAllocateDescriptorSets(_renderer->GetDevice(), &descriptorSetAllocateInfo, &_globalStateDescriptorSets[descriptorSetBaseIndex]);
		if (allocateDescriptorSetsResult != VK_SUCCESS)
		{
			_log->Error("vkAllocateDescriptorSets failed with error code {0}", allocateDescriptorSetsResult);
			return std::numeric_limits<size_t>::max();
		}
	}

	// Return the index of the next available descriptor set
	return _nextGlobalStateBindingDescriptorSetIndex++;
}

//----------------------------------------------------------------------------------------
void VulkanShaderProgram::CopyDescriptorSet(size_t sourceDescriptorSetIndex, size_t targetDescriptorSetIndex)
{
	VkCopyDescriptorSet descriptorCopy = {};
	descriptorCopy.sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET;
	descriptorCopy.dstSet = _bindingDescriptorSets[targetDescriptorSetIndex];
	descriptorCopy.dstBinding = _baseDescriptorBindingNo;
	descriptorCopy.dstArrayElement = 0;
	descriptorCopy.srcSet = _bindingDescriptorSets[sourceDescriptorSetIndex];
	descriptorCopy.srcBinding = _baseDescriptorBindingNo;
	descriptorCopy.srcArrayElement = 0;
	descriptorCopy.descriptorCount = _bindingDescriptorCountPerSet;
	vkUpdateDescriptorSets(_renderer->GetDevice(), 0, nullptr, 1, &descriptorCopy);
}

//----------------------------------------------------------------------------------------
void VulkanShaderProgram::BindDescriptorSet(VkCommandBuffer commandBuffer, size_t index, VkPipelineLayout pipelineLayout)
{
	vkCmdBindDescriptorSets(commandBuffer, (_isComputeShader ? VK_PIPELINE_BIND_POINT_COMPUTE : VK_PIPELINE_BIND_POINT_GRAPHICS), pipelineLayout, 0, 1, &_bindingDescriptorSets[index], 0, nullptr);
}

//----------------------------------------------------------------------------------------
VkDescriptorSet VulkanShaderProgram::GetDescriptorSet(size_t setIndex) const
{
	return _bindingDescriptorSets[setIndex];
}

//----------------------------------------------------------------------------------------
const std::vector<VkDescriptorSetLayout>& VulkanShaderProgram::GetDescriptorSetLayouts() const
{
	return _descriptorSetLayouts;
}

} // namespace cobalt::graphics
