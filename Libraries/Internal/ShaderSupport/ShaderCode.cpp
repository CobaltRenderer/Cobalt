// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "ShaderCode.h"
#include <Cobalt/Debug/Debug.pkg>
WARNINGS_PUSH_OFF
#include "SPIRV/GlslangToSpv.h"
#include "StandAlone/ResourceLimits.cpp" // NOLINT
#include "StandAlone/ResourceLimits.h"
#include "glslang/Include/Common.h"
#include "glslang/Include/ResourceLimits.h"
#include "glslang/Include/ShHandle.h"
#include "spirv-tools/libspirv.hpp"
#include "spirv-tools/optimizer.hpp"
#include "spirv.hpp"
#include "spirv_cross.hpp"
#include "spirv_glsl.hpp"
#include "spirv_hlsl.hpp"
#include "spirv_msl.hpp"
//#include <glslang/Public/ResourceLimits.h>
WARNINGS_POP
#include <cctype>
#include <cstring>
#include <iostream>
#include <map>
#include <regex>
#include <unordered_map>
#include <unordered_set>
#ifdef _WIN32
#define FUNCTION_EXPORT __declspec(dllexport)
#else
#define FUNCTION_EXPORT __attribute__((visibility("default")))
#endif
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
namespace internal {
extern "C" FUNCTION_EXPORT IShaderCode* CreateIShaderCode(cobalt::logging::ILogger* log)
{
	return new ShaderCode(cobalt::logging::ILogger::unique_ptr(log));
}
} // namespace internal

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
ShaderCode::ShaderCode(cobalt::logging::ILogger::unique_ptr&& log)
: _log(std::move(log))
{}

//----------------------------------------------------------------------------------------
ShaderCode::~ShaderCode()
{
	_compiler.reset();
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void ShaderCode::Delete()
{
	delete this;
}

//----------------------------------------------------------------------------------------
// Code methods
//----------------------------------------------------------------------------------------
bool ShaderCode::LoadCode(Language language, Stage stage, Environment sourceEnvironment, Environment targetEnvironment, const uint8_t* shaderCode, size_t shaderCodeSizeInBytes, const std::string& entryPointName, int& nextBindingNo, std::vector<Resource>& resourceSet)
{
	// Ensure that the specified language is supported
	if (language == Language::MSL)
	{
		_log->Error("Uncompiled MSL cannot be loaded currently. MSL can only be emitted as an output, not as an input for shader conversion.");
		return false;
	}

	_stage = stage;
	_sourceEnvironment = sourceEnvironment;
	_targetEnvironment = targetEnvironment;

	spv_target_env targetEnvironmentNative = spv_target_env::SPV_ENV_UNIVERSAL_1_3;
	switch (targetEnvironment)
	{
	case Environment::OpenGL_33:
		targetEnvironmentNative = spv_target_env::SPV_ENV_UNIVERSAL_1_3;
		break;
	case Environment::OpenGL_43:
		targetEnvironmentNative = spv_target_env::SPV_ENV_OPENGL_4_3;
		break;
	case Environment::Vulkan_11:
		targetEnvironmentNative = spv_target_env::SPV_ENV_VULKAN_1_1;
		break;
	case Environment::Vulkan_12:
		targetEnvironmentNative = spv_target_env::SPV_ENV_VULKAN_1_2;
		break;
	case Environment::General:
		targetEnvironmentNative = spv_target_env::SPV_ENV_UNIVERSAL_1_3;
		break;
	}
	_targetEnvironmentNative = targetEnvironmentNative;

	if (language == Language::SPIRV)
	{
		// Use SPIRV code directly
		if ((shaderCodeSizeInBytes % 4) != 0)
		{
			_log->Error("Invalid SPIRV code with size {0} bytes, not divisible by 4.", shaderCodeSizeInBytes);
			return false;
		}
		_spirvCode.resize(shaderCodeSizeInBytes / 4);
		std::memcpy(_spirvCode.data(), shaderCode, shaderCodeSizeInBytes);

		if (targetEnvironment == Environment::OpenGL_43)
		{
			auto spirvVersionNoRaw = _spirvCode[1];
			if (spirvVersionNoRaw >= 0x10300) // Version 1.3.0
			{
				// We're not going to attempt to load the spirv into OpenGL 4.3 (that's not allowed to 4.6),
				// so use this environment to get around a warning about "spirv > 1.0 isn't supported" when
				// using more modern SPIR-V than the 1.0 release.
				targetEnvironmentNative = spv_target_env::SPV_ENV_UNIVERSAL_1_3;
			}
		}
	}
	else if (language == Language::SPIRVAssembly)
	{
		spvtools::SpirvTools context(_targetEnvironmentNative);
		context.SetMessageConsumer([this](spv_message_level_t level, const char* source, const spv_position_t& position, const char* message) { ForwardLogMessage("SpirvTools::Assemble", level, source, position, message); });

		uint32_t assemblyOptions = SPV_TEXT_TO_BINARY_OPTION_NONE;
		if (!context.Assemble(reinterpret_cast<const char*>(shaderCode), shaderCodeSizeInBytes, &_spirvCode, assemblyOptions))
		{
			_log->Error("Failed to import SPIR-V code as text");
			return false;
		}
	}
	else
	{
		// Compile HLSL or GLSL to SPIRV
		//##TODO## Investigate DXC for compiling HLSL to SPIRV instead of glslang
		glslang::InitializeProcess();

		// Set shader stage
		EShLanguage shaderStage = EShLanguage::EShLangVertex;
		if (stage == Stage::Vertex)
		{
			shaderStage = EShLanguage::EShLangVertex;
		}
		else if (stage == Stage::Geometry)
		{
			shaderStage = EShLanguage::EShLangGeometry;
		}
		else if (stage == Stage::Fragment)
		{
			shaderStage = EShLanguage::EShLangFragment;
		}
		else if (stage == Stage::Compute)
		{
			shaderStage = EShLanguage::EShLangCompute;
		}
		else
		{
			UNREACHABLE();
		}
		glslang::TShader shader(shaderStage);

		//##TODO## Clean this up
		glslang::EShSource sourceLanguageNative = glslang::EShSource::EShSourceNone;
		//		glslang::EShClient sourceClientNative = glslang::EShClient::EShClientNone;
		//		int sourceClientVersion = 0;
		switch (language)
		{
		case Language::GLSL:
			sourceLanguageNative = glslang::EShSource::EShSourceGlsl;
			//			sourceClientNative = glslang::EShClient::EShClientOpenGL;
			//			switch (sourceEnvironment)
			//			{
			//			default:
			//			case IShaderCode::Environment::OpenGL_33:
			//				sourceClientVersion = 330;
			//				break;
			//			case IShaderCode::Environment::OpenGL_43:
			//				sourceClientVersion = 430;
			//				break;
			//			}
			break;
		case Language::HLSL:
			sourceLanguageNative = glslang::EShSource::EShSourceHlsl;
			//			sourceClientNative = glslang::EShClient::EShClientNone;
			//			sourceClientVersion = 0;
			break;
		case Language::SPIRV:
			sourceLanguageNative = glslang::EShSource::EShSourceNone;
			//			sourceClientNative = glslang::EShClient::EShClientNone;
			//			sourceClientVersion = 0;
			break;
		}

		glslang::EShClient targetClientNative = glslang::EShClient::EShClientNone;
		glslang::EShTargetClientVersion targetClientVersion = glslang::EShTargetClientVersion::EShTargetVulkan_1_2;
		bool targetEnvironmentIsVulkan = false;
		bool targetEnvironmentIsOpenGL = false;
		switch (targetEnvironment)
		{
		case Environment::OpenGL_33:
		case Environment::OpenGL_43:
			targetClientNative = glslang::EShClient::EShClientOpenGL;
			targetClientVersion = glslang::EShTargetClientVersion::EShTargetOpenGL_450;
			targetEnvironmentIsOpenGL = true;
			break;
		case Environment::Vulkan_11:
			targetClientNative = glslang::EShClient::EShClientVulkan;
			targetClientVersion = glslang::EShTargetClientVersion::EShTargetVulkan_1_1;
			targetEnvironmentIsVulkan = true;
			break;
		case Environment::Vulkan_12:
			targetClientNative = glslang::EShClient::EShClientVulkan;
			targetClientVersion = glslang::EShTargetClientVersion::EShTargetVulkan_1_2;
			targetEnvironmentIsVulkan = true;
			break;
		case Environment::General:
			targetClientNative = glslang::EShClient::EShClientVulkan;
			targetClientVersion = glslang::EShTargetClientVersion::EShTargetVulkan_1_2;
			break;
		}

		// Note that the use of these arguments is confusing and poorly documented, but we've verified what we're doing
		// here is correct.
		shader.setEnvInput(sourceLanguageNative, shaderStage, targetClientNative, 100);
		shader.setEnvClient(targetClientNative, targetClientVersion);
		shader.setEnvTarget(glslang::EShTargetLanguage::EShTargetSpv, (targetClientVersion == glslang::EShTargetClientVersion::EShTargetOpenGL_450 ? glslang::EShTargetLanguageVersion::EShTargetSpv_1_0 : glslang::EShTargetLanguageVersion::EShTargetSpv_1_3));

		// Set shader code
		std::string shaderString(shaderCode, shaderCode + shaderCodeSizeInBytes);
		const auto* shaderCString = shaderString.c_str();
		shader.setStrings(&shaderCString, 1);
		shader.setEntryPoint(entryPointName.empty() ? StandardShaderEntryPointName : entryPointName.c_str());

		// Set options
		//shader.setAutoMapBindings(true);
		shader.setAutoMapLocations(true);
		shader.setInvertY(false);
		if (targetEnvironmentIsOpenGL)
		{
			// Only forcefully combine samplers with textures for OpenGL targets. For Vulkan, we may need to preserve or
			// combine them depending on naming conventions. We apply the transformation later in this case.
			shader.setTextureSamplerTransformMode(EShTexSampTransUpgradeTextureRemoveSampler);
		}

		// Set resources
		const TBuiltInResource* resources = &glslang::DefaultTBuiltInResource;
		//const TBuiltInResource* resources = GetDefaultResources();

		// Parse code to intermediate format, then to SPIRV
		if (!shader.parse(resources, 100, false, EShMessages::EShMsgDefault))
		{
			// Invalid code, print error log
			_codeLoaded = false;
			const char* shaderLog = shader.getInfoLog();
			if (std::strlen(shaderLog) > 0)
			{
				_log->Error(shaderLog);
			}

			glslang::FinalizeProcess();
			return false;
		}

		// Print out any warnings or log messages
		const char* shaderLog = shader.getInfoLog();
		if (std::strlen(shaderLog) > 0)
		{
			_log->Warning(shaderLog);
		}

		// Set options for translating to SPIRV
		glslang::SpvOptions spvOptions;
		spvOptions.generateDebugInfo = true;
		spvOptions.disableOptimizer = true;
		spvOptions.optimizeSize = false;
		spvOptions.disassemble = false;
		spvOptions.validate = true;

		// Translate to SPIRV
		glslang::TIntermediate& intermediate = *shader.getIntermediate();
		glslang::GlslangToSpv(intermediate, _spirvCode, &spvOptions);
		glslang::FinalizeProcess();

		// When turning GLSL into SPIR-V, glslang doesn't generate location attributes for us when they're absent, it's only
		// able to do this at the linking phase, but we don't use glslang to link a complete program here, just translate
		// individual stages in isolation. As a result, we need to generate location attributes ourselves for any inputs or
		// outputs that are missing them.
		FixMissingLocationAttributes(_spirvCode);

		// Fix OpExecutionMode "%main" Invocations, in geometry shaders. Instruction is incorrectly generated causing an
		// error.
		if (stage == Stage::Geometry)
		{
			for (size_t i = 0; i < _spirvCode.size() - 2; i++)
			{
				// if (Malformed instruction AND OpExecution instruction AND changing invocations
				if (((_spirvCode[i] >> 16) == 3) && ((_spirvCode[i] & 0xFFFF) == 16) && (_spirvCode[i + 2] == 0))
				{
					// Fix instruction by making it longer and including invocation number
					_spirvCode[i] = 0x00040010;
					_spirvCode.insert(_spirvCode.begin() + i + 3, 1, 1);
					break;
				}
			}
		}

		// When targeting the Vulkan environment, the "OriginLowerLeft" execution mode for the "OpExecutionMode"
		// instruction is not legal, and "OriginUpperLeft" must be used instead. Currently glslang incorrectly generates
		// OpExecutionMode instructions for fragment shaders with the illegal execution mode, which we identify and
		// correct here.
		if (targetEnvironmentIsVulkan && (stage == Stage::Fragment))
		{
			for (size_t i = 0; i < _spirvCode.size() - 2; i++)
			{
				if (((_spirvCode[i] >> 16) == 3) && ((_spirvCode[i] & 0xFFFF) == 16) && (_spirvCode[i + 2] == 8))
				{
					_spirvCode[i + 2] = 7;
					break;
				}
			}
		}

		// Apply targeted fixes to the generated SPIRV if we converted from GLSL to Vulkan. This is needed due to current
		// deficiencies in glslang (as of 2025-12-14), where it will produce output which is non-compliant with Vulkan,
		// despite being advised of the correct Vulkan version as the destination environment. Note that the
		// FixVertexPointSizeAlwaysOne fix is also required when coming from HLSL.
		if (targetEnvironmentIsVulkan && (stage == Stage::Vertex))
		{
			FixVertexPointSizeAlwaysOne(_spirvCode);
			if (language == Language::GLSL)
			{
				FixVulkanBuiltinIds(_spirvCode);
				FixVulkanGlPerVertexClipDistance(_spirvCode);
				StripDescriptorIndexingDecls(_spirvCode);
			}
		}

		// If "ConstantBuffer<>" constructs are provided in HLSL, glslang alters the names. In this case, we need to
		// restore the original names so that we can bind to the buffers by name.
		if (language == Language::HLSL)
		{
			FixHlslConstantBufferNames(shaderString, _spirvCode);
		}

		// Apply targeted fixes when compiling for Vulkan, for the benefit of MoltenVK running on macOS. As of
		// 2025-12-10, there's a known bug with how SPIR-V is translated to MSL, which causes compilation failures. See
		// the following:
		// https://github.com/KhronosGroup/MoltenVK/issues/2597
		// We work around the issue by modifying the SPIR-V to ensure variable names don't alias type names.
		if (targetEnvironmentIsVulkan)
		{
			FixStructTypeNameCollisionsOnMoltenVK(_spirvCode);
		}

		// If we're not targeting OpenGL, textures and samplers haven't already been combined. In this case, we need to
		// selectively combine them based on naming convention.
		if (!targetEnvironmentIsOpenGL)
		{
			FoldNamedStandaloneSamplersIntoImages(_spirvCode);
		}
	}

	if ((language != Language::SPIRV) && (language != Language::SPIRVAssembly))
	{
		// If we've just generated SPIRV - legalize and optimize it. If the user provided their own SPIRV
		// then we assume they've optimised it offline and can skip this step.
		spvtools::Optimizer optimizer(targetEnvironmentNative);
		optimizer.SetMessageConsumer([this](spv_message_level_t level, const char* source, const spv_position_t& position, const char* message) { ForwardLogMessage("spvtools::Optimizer", level, source, position, message); });
		std::vector<uint32_t> spirvCodeOpt;

		// Add special flag if SPIRV originated from HLSL
		if (language == Language::HLSL)
		{
			optimizer.RegisterLegalizationPasses();
		}

		optimizer.RegisterPerformancePasses();

		// Attempt to optimize the SPIR-V code
		if (!optimizer.Run(_spirvCode.data(), _spirvCode.size(), &spirvCodeOpt))
		{
			_log->Warning("Could not optimize shader, using an unoptimized shader instead.");
		}
		else
		{
			_spirvCode = std::move(spirvCodeOpt);
		}
	}

	// Shader reflection
	try
	{
		// Make compiler object to do reflection on code
		_compiler = std::make_unique<spirv_cross::Compiler>(_spirvCode);

		// Restore resource names
		if (language == Language::HLSL)
		{
			spirv_cross::ShaderResources resources = _compiler->get_shader_resources();
			for (const auto& resource : resources.stage_inputs)
			{
				auto index = resource.name.find_last_of('.');
				if (index != std::string::npos)
				{
					++index;
					std::string newName = resource.name.substr(index, resource.name.size() - index);
					_compiler->set_name(resource.id, newName);
					ChangeName(newName, resource.id);
				}
			}

			if (stage != IShaderCode::Stage::Vertex)
			{
				for (const auto& resource : resources.stage_inputs)
				{
					std::string newName = resource.name + "_Location" + std::to_string(_compiler->get_decoration(resource.id, spv::DecorationLocation));
					_compiler->set_name(resource.id, newName);
					ChangeName(newName, resource.id);
				}
			}

			if (stage != IShaderCode::Stage::Fragment)
			{
				for (const auto& resource : resources.stage_outputs)
				{
					auto index = resource.name.find_last_of('.');
					if (index != std::string::npos)
					{
						++index;
						std::string newName = resource.name.substr(index, resource.name.size() - index) + "_Location" + std::to_string(_compiler->get_decoration(resource.id, spv::DecorationLocation));
						_compiler->set_name(resource.id, newName);
						ChangeName(newName, resource.id);
					}
				}
			}

			for (const auto& resource : resources.storage_buffers)
			{
				// Rename buffers with attached counters of the form "<name>@count" to "<name>@counter", to prevent
				// compilation failures on macOS when running under MoltenVK due to its shader converter causing type name
				// clashes. Shaders of this form are the default when converting HLSL to SPIR-V. Without this fix, we'll get
				// errors like this:
				//   [mvk-error] VK_ERROR_INITIALIZATION_FAILED: Shader library compile failed (Error code 3):
				//   program_source:48:12: error: must use 'struct' tag to refer to type 'bufferDataIn_count' in this scope
				//       device bufferDataIn_count* bufferDataOut_count [[id(4)]];
				//              ^
				//              struct
				//   program_source:46:32: note: struct 'bufferDataIn_count' is hidden by a non-type declaration of 'bufferDataIn_count' here
				//       device bufferDataIn_count* bufferDataIn_count [[id(2)]];
				//                                  ^
				//[mvk-error] VK_ERROR_INITIALIZATION_FAILED: Fragment shader function could not be compiled into pipeline. See previous logged error.
				auto nameExtensionPos = resource.name.find_last_of('@');
				if (nameExtensionPos != std::string::npos)
				{
					std::string name = resource.name.substr(0, nameExtensionPos);
					std::string extension = resource.name.substr(nameExtensionPos, resource.name.size() - nameExtensionPos);
					if (extension == "@count")
					{
						extension = "@counter";
						std::string newName = name + extension;
						_compiler->set_name(resource.id, newName);
						ChangeName(newName, resource.id);
					}
				}
			}
		}

		spirv_cross::ShaderResources resources = _compiler->get_shader_resources();

		// Shader inputs
		int nextInputResourceOrder = 0;
		for (const auto& resource : resources.stage_inputs)
		{
			Resource shaderResource = {};
			shaderResource.id = resource.id;
			shaderResource.type = ResourceType::Input;
			shaderResource.valid = true;
			shaderResource.name = resource.name;
			shaderResource.location = _compiler->get_decoration(resource.id, spv::DecorationLocation);
			shaderResource.binding = _compiler->get_decoration(resource.id, spv::DecorationBinding);
			shaderResource.offset = 0;
			shaderResource.order = nextInputResourceOrder++;
			shaderResource.usedStages |= stage;

			GetNativeVulkanFormatAndSize(&shaderResource, _compiler->get_type(resource.type_id));
			resourceSet.push_back(shaderResource);
		}

		// Shader outputs
		int nextOutputResourceOrder = 0;
		for (const auto& resource : resources.stage_outputs)
		{
			Resource shaderResource = {};
			shaderResource.id = resource.id;
			shaderResource.type = ResourceType::Output;
			shaderResource.valid = true;
			shaderResource.name = resource.name;
			shaderResource.location = _compiler->get_decoration(resource.id, spv::DecorationLocation);
			shaderResource.binding = _compiler->get_decoration(resource.id, spv::DecorationBinding);
			shaderResource.offset = 0;
			shaderResource.order = nextOutputResourceOrder++;
			shaderResource.usedStages |= stage;

			GetNativeVulkanFormatAndSize(&shaderResource, _compiler->get_type(resource.type_id));
			resourceSet.push_back(shaderResource);
		}

		// Shader samplers and textures
		auto getNativeVulkanImageViewType = [](const spirv_cross::SPIRType& typeInfo) {
			switch (typeInfo.image.dim)
			{
			case spv::Dim1D:
				return (!typeInfo.image.arrayed ? VK_IMAGE_VIEW_TYPE_1D : VK_IMAGE_VIEW_TYPE_1D_ARRAY);
			case spv::Dim2D:
				return (!typeInfo.image.arrayed ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_2D_ARRAY);
			case spv::Dim3D:
				return VK_IMAGE_VIEW_TYPE_3D;
			case spv::DimCube:
				return (!typeInfo.image.arrayed ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_CUBE_ARRAY);
			default:
				return VK_IMAGE_VIEW_TYPE_MAX_ENUM;
			}
		};
		for (const auto& resource : resources.sampled_images)
		{
			// If this sampled image is actually a read-only texel array, skip it here.
			auto typeInfo = _compiler->get_type(resource.type_id);
			if (typeInfo.image.dim == spv::DimBuffer)
			{
				continue;
			}

			// Determine the binding number to use for this resource
			size_t resourceIndex;
			bool locatedExistingResource = LocateResourceInSet(resourceSet, resource, ResourceType::TextureWithCombinedSampler, resourceIndex);
			int bindingNo = 0;
			if (!locatedExistingResource)
			{
				bindingNo = nextBindingNo++;
			}
			else
			{
				bindingNo = resourceSet[resourceIndex].binding;
				resourceSet[resourceIndex].usedStages |= stage;
			}

			// Update the resource binding number
			ChangeDescriptorSet(0, resource.id);
			ChangeBinding(bindingNo, resource.id);

			// If we located an existing matching resource, advance to the next resource entry.
			if (locatedExistingResource)
			{
				continue;
			}

			// Add this resource to the resource set
			auto arraySizes = typeInfo.array;
			Resource shaderResource = {};
			shaderResource.id = resource.id;
			shaderResource.type = ResourceType::TextureWithCombinedSampler;
			shaderResource.valid = true;
			shaderResource.name = resource.name;
			shaderResource.location = 0;
			shaderResource.binding = bindingNo;
			shaderResource.offset = 0;
			shaderResource.order = 0;
			shaderResource.nativeVulkanFormat = VK_FORMAT_UNDEFINED;
			shaderResource.nativeVulkanImageViewType = getNativeVulkanImageViewType(typeInfo);
			shaderResource.size = 0;
			shaderResource.arraySizes = std::vector<uint32_t>(arraySizes.data(), arraySizes.data() + arraySizes.size());
			shaderResource.usedStages |= stage;
			resourceSet.push_back(shaderResource);
		}
		for (const auto& resource : resources.separate_images)
		{
			// If this separate image is actually a read-only texel array, skip it here.
			auto typeInfo = _compiler->get_type(resource.type_id);
			if (typeInfo.image.dim == spv::DimBuffer)
			{
				continue;
			}

			// Determine the binding number to use for this resource
			size_t resourceIndex;
			bool locatedExistingResource = LocateResourceInSet(resourceSet, resource, ResourceType::Texture, resourceIndex);
			int bindingNo = 0;
			if (!locatedExistingResource)
			{
				bindingNo = nextBindingNo++;
			}
			else
			{
				bindingNo = resourceSet[resourceIndex].binding;
				resourceSet[resourceIndex].usedStages |= stage;
			}

			// Update the resource binding number
			ChangeDescriptorSet(0, resource.id);
			ChangeBinding(bindingNo, resource.id);

			// If we located an existing matching resource, advance to the next resource entry.
			if (locatedExistingResource)
			{
				continue;
			}

			// Add this resource to the resource set
			auto arraySizes = typeInfo.array;
			Resource shaderResource = {};
			shaderResource.id = resource.id;
			shaderResource.type = ResourceType::Texture;
			shaderResource.valid = true;
			shaderResource.name = resource.name;
			shaderResource.location = 0;
			shaderResource.binding = bindingNo;
			shaderResource.offset = 0;
			shaderResource.order = 0;
			shaderResource.nativeVulkanFormat = VK_FORMAT_UNDEFINED;
			shaderResource.nativeVulkanImageViewType = getNativeVulkanImageViewType(typeInfo);
			shaderResource.size = 0;
			shaderResource.arraySizes = std::vector<uint32_t>(arraySizes.data(), arraySizes.data() + arraySizes.size());
			shaderResource.usedStages |= stage;
			resourceSet.push_back(shaderResource);
		}
		for (const auto& resource : resources.separate_samplers)
		{
			// Determine the binding number to use for this resource
			size_t resourceIndex;
			bool locatedExistingResource = LocateResourceInSet(resourceSet, resource, ResourceType::Sampler, resourceIndex);
			int bindingNo = 0;
			if (!locatedExistingResource)
			{
				bindingNo = nextBindingNo++;
			}
			else
			{
				bindingNo = resourceSet[resourceIndex].binding;
				resourceSet[resourceIndex].usedStages |= stage;
			}

			// Update the resource binding number
			ChangeDescriptorSet(0, resource.id);
			ChangeBinding(bindingNo, resource.id);

			// If we located an existing matching resource, advance to the next resource entry.
			if (locatedExistingResource)
			{
				continue;
			}

			// Add this resource to the resource set
			auto arraySizes = _compiler->get_type(resource.type_id).array;
			Resource shaderResource = {};
			shaderResource.id = resource.id;
			shaderResource.type = ResourceType::Sampler;
			shaderResource.valid = true;
			shaderResource.name = resource.name;
			shaderResource.location = 0;
			shaderResource.binding = bindingNo;
			shaderResource.offset = 0;
			shaderResource.order = 0;
			shaderResource.nativeVulkanFormat = VK_FORMAT_UNDEFINED;
			shaderResource.size = 0;
			shaderResource.arraySizes = std::vector<uint32_t>(arraySizes.data(), arraySizes.data() + arraySizes.size());
			shaderResource.usedStages |= stage;
			resourceSet.push_back(shaderResource);
		}

		// Shader data arrays
		for (const auto& resource : resources.storage_buffers)
		{
			// Determine the binding number to use for this resource
			size_t resourceIndex;
			bool locatedExistingResource = LocateResourceInSet(resourceSet, resource, ResourceType::DataArray, resourceIndex);
			int bindingNo = 0;
			if (!locatedExistingResource)
			{
				bindingNo = nextBindingNo++;
			}
			else
			{
				bindingNo = resourceSet[resourceIndex].binding;
				resourceSet[resourceIndex].usedStages |= stage;
			}

			// Update the resource binding number
			ChangeDescriptorSet(0, resource.id);
			ChangeBinding(bindingNo, resource.id);

			// If we located an existing matching resource, advance to the next resource entry.
			if (locatedExistingResource)
			{
				continue;
			}

			// Add this resource to the resource set
			auto arraySizes = _compiler->get_type(resource.type_id).array;
			Resource shaderResource = {};
			shaderResource.id = resource.id;
			shaderResource.type = ResourceType::DataArray;
			shaderResource.valid = true;
			shaderResource.name = resource.name;
			shaderResource.location = 0;
			shaderResource.binding = bindingNo;
			shaderResource.offset = 0;
			shaderResource.order = 0;
			shaderResource.nativeVulkanFormat = VK_FORMAT_UNDEFINED;
			shaderResource.size = 0;
			shaderResource.arraySizes = std::vector<uint32_t>(arraySizes.data(), arraySizes.data() + arraySizes.size());
			shaderResource.usedStages |= stage;
			resourceSet.push_back(shaderResource);
		}

		// Shader texel arrays
		std::vector<spirv_cross::Resource> texelArrayResources(resources.storage_images.data(), resources.storage_images.data() + resources.storage_images.size());
		for (const auto& resource : resources.sampled_images)
		{
			// If this sampled image isn't a read-only texel array, skip it here.
			auto typeInfo = _compiler->get_type(resource.type_id);
			if (typeInfo.image.dim != spv::DimBuffer)
			{
				continue;
			}

			// Determine the binding number to use for this resource
			size_t resourceIndex;
			bool locatedExistingResource = LocateResourceInSet(resourceSet, resource, ResourceType::TexelArray, resourceIndex);
			int bindingNo = 0;
			if (!locatedExistingResource)
			{
				bindingNo = nextBindingNo++;
			}
			else
			{
				bindingNo = resourceSet[resourceIndex].binding;
				resourceSet[resourceIndex].usedStages |= stage;
			}

			// Update the resource binding number
			ChangeDescriptorSet(0, resource.id);
			ChangeBinding(bindingNo, resource.id);

			// If we located an existing matching resource, advance to the next resource entry.
			if (locatedExistingResource)
			{
				continue;
			}

			// Add this resource to the resource set
			auto arraySizes = typeInfo.array;
			Resource shaderResource = {};
			shaderResource.id = resource.id;
			shaderResource.type = ResourceType::TexelArray;
			shaderResource.valid = true;
			shaderResource.name = resource.name;
			shaderResource.location = 0;
			shaderResource.binding = bindingNo;
			shaderResource.offset = 0;
			shaderResource.order = 0;
			shaderResource.nativeVulkanFormat = GetNativeVulkanFormatFromImageFormat(typeInfo.image.format);
			shaderResource.size = 0;
			shaderResource.arraySizes = std::vector<uint32_t>(arraySizes.data(), arraySizes.data() + arraySizes.size());
			shaderResource.usedStages |= stage;
			shaderResource.writeableResource = false;
			resourceSet.push_back(shaderResource);
		}
		for (const auto& resource : resources.separate_images)
		{
			// If this separate image isn't a read-only texel array, skip it here.
			auto typeInfo = _compiler->get_type(resource.type_id);
			if (typeInfo.image.dim != spv::DimBuffer)
			{
				continue;
			}

			// Determine the binding number to use for this resource
			size_t resourceIndex;
			bool locatedExistingResource = LocateResourceInSet(resourceSet, resource, ResourceType::TexelArray, resourceIndex);
			int bindingNo = 0;
			if (!locatedExistingResource)
			{
				bindingNo = nextBindingNo++;
			}
			else
			{
				bindingNo = resourceSet[resourceIndex].binding;
				resourceSet[resourceIndex].usedStages |= stage;
			}

			// Update the resource binding number
			ChangeDescriptorSet(0, resource.id);
			ChangeBinding(bindingNo, resource.id);

			// If we located an existing matching resource, advance to the next resource entry.
			if (locatedExistingResource)
			{
				continue;
			}

			// Add this resource to the resource set
			auto arraySizes = typeInfo.array;
			Resource shaderResource = {};
			shaderResource.id = resource.id;
			shaderResource.type = ResourceType::TexelArray;
			shaderResource.valid = true;
			shaderResource.name = resource.name;
			shaderResource.location = 0;
			shaderResource.binding = bindingNo;
			shaderResource.offset = 0;
			shaderResource.order = 0;
			shaderResource.nativeVulkanFormat = GetNativeVulkanFormatFromImageFormat(typeInfo.image.format);
			shaderResource.size = 0;
			shaderResource.arraySizes = std::vector<uint32_t>(arraySizes.data(), arraySizes.data() + arraySizes.size());
			shaderResource.usedStages |= stage;
			shaderResource.writeableResource = false;
			resourceSet.push_back(shaderResource);
		}
		for (const auto& resource : resources.storage_images)
		{
			// Determine the binding number to use for this resource
			size_t resourceIndex;
			bool locatedExistingResource = LocateResourceInSet(resourceSet, resource, ResourceType::TexelArray, resourceIndex);
			int bindingNo = 0;
			if (!locatedExistingResource)
			{
				bindingNo = nextBindingNo++;
			}
			else
			{
				bindingNo = resourceSet[resourceIndex].binding;
				resourceSet[resourceIndex].usedStages |= stage;
			}

			// Update the resource binding number
			ChangeDescriptorSet(0, resource.id);
			ChangeBinding(bindingNo, resource.id);

			// If we located an existing matching resource, advance to the next resource entry.
			if (locatedExistingResource)
			{
				continue;
			}

			// Add this resource to the resource set
			auto typeInfo = _compiler->get_type(resource.type_id);
			auto arraySizes = typeInfo.array;
			Resource shaderResource = {};
			shaderResource.id = resource.id;
			shaderResource.type = ResourceType::TexelArray;
			shaderResource.valid = true;
			shaderResource.name = resource.name;
			shaderResource.location = 0;
			shaderResource.binding = bindingNo;
			shaderResource.offset = 0;
			shaderResource.order = 0;
			shaderResource.nativeVulkanFormat = GetNativeVulkanFormatFromImageFormat(typeInfo.image.format);
			shaderResource.size = 0;
			shaderResource.arraySizes = std::vector<uint32_t>(arraySizes.data(), arraySizes.data() + arraySizes.size());
			shaderResource.usedStages |= stage;
			shaderResource.writeableResource = true;
			resourceSet.push_back(shaderResource);
		}

		// Shader uniform buffers
		for (const auto& resource : resources.uniform_buffers)
		{
			// Determine the binding number to use for this resource
			int bindingNo = 0;
			bool locatedExistingResource = false;
			bool isGlobalConstantBuffer = (resource.name == GlobalConstantBufferName);
			if (!isGlobalConstantBuffer)
			{
				size_t resourceIndex;
				locatedExistingResource = LocateResourceInSet(resourceSet, resource, ResourceType::Uniform, resourceIndex);
				if (locatedExistingResource)
				{
					bindingNo = resourceSet[resourceIndex].binding;
					resourceSet[resourceIndex].usedStages |= stage;
				}
			}
			if (!locatedExistingResource)
			{
				bindingNo = nextBindingNo++;
			}

			// Update the resource binding number
			ChangeDescriptorSet((isGlobalConstantBuffer ? 1 : 0), resource.id);
			ChangeBinding(bindingNo, resource.id);

			// If we located an existing matching resource, advance to the next resource entry.
			if (locatedExistingResource)
			{
				continue;
			}

			spirv_cross::SPIRType type = _compiler->get_type(resource.type_id);
			Resource shaderResource = {};
			shaderResource.id = resource.id;
			shaderResource.type = ResourceType::Uniform;
			shaderResource.valid = true;
			shaderResource.name = resource.name;
			shaderResource.location = 0;
			shaderResource.binding = bindingNo;
			shaderResource.size = _compiler->get_declared_struct_size(type);
			shaderResource.offset = 0;
			shaderResource.order = 0;
			shaderResource.nativeVulkanFormat = VK_FORMAT_UNDEFINED;
			shaderResource.usedStages |= stage;

			// Unroll state buffer for structures in field
			GetUniformFields(&shaderResource.fields, type);

			// Add this resource to the resource set
			resourceSet.push_back(shaderResource);
		}
	}
	catch (const spirv_cross::CompilerError& SpirvException)
	{
		// Don't leak exceptions out - they may not safely cross the marshalling barrier.

		_log->Error("Unable to parse Spirv. SpirvCross threw: {0}", SpirvException.what());
		return false;
	}

	_codeLoaded = true;
	return true;
}

//----------------------------------------------------------------------------------------
bool ShaderCode::LocateResourceInSet(const std::vector<Resource>& resourceSet, const spirv_cross::Resource& resource, ResourceType type, size_t& resourceIndex)
{
	for (size_t i = 0; i < resourceSet.size(); ++i)
	{
		const auto& entry = resourceSet[i];
		if (entry.type != type)
		{
			continue;
		}
		if (entry.name == resource.name)
		{
			resourceIndex = i;
			return true;
		}
	}
	return false;
}

//----------------------------------------------------------------------------------------
bool ShaderCode::ExportCodeAsHLSL(std::string& outputCode, unsigned int shaderModelVersionMajor, unsigned int shaderModelVersionMinor) const
{
	// Initialize the code output and validate we have valid code to export
	outputCode.clear();
	if (!_codeLoaded)
	{
		_log->Error("Cannot export code in target language, original code was not valid");
		return false;
	}

	// Set the output shader model version
	spirv_cross::CompilerHLSL compiler(_spirvCode);
	spirv_cross::CompilerHLSL::Options options;
	options.shader_model = std::max((shaderModelVersionMajor * 10) + shaderModelVersionMinor, 51u);
	compiler.set_hlsl_options(options);

	// Ensure vertex attribute names are preserved as semantic names in the exported HLSL, to allow name-based binding.
	if (compiler.get_execution_model() == spv::ExecutionModelVertex)
	{
		for (const auto& variable : compiler.get_active_interface_variables())
		{
			if (compiler.get_storage_class(variable) == spv::StorageClassInput)
			{
				auto name = compiler.get_name(variable);
				auto location = compiler.get_decoration(variable, spv::DecorationLocation);
				spirv_cross::HLSLVertexAttributeRemap remap;
				remap.semantic = name;
				remap.location = location;
				compiler.add_vertex_attribute_remap(remap);
			}
		}
	}

	// Export the code as HLSL
	try
	{
		outputCode = compiler.compile();
	}
	catch (const std::runtime_error& e)
	{
		_log->Error("std::runtime_error in ExportCodeAsHLSL: {0}", e.what());
		return false;
	}
	return true;
}

//----------------------------------------------------------------------------------------
bool ShaderCode::ExportCodeAsSPIRVAssembly(std::string& outputCode) const
{
	// Initialize the code output and validate we have valid code to export
	outputCode.clear();
	if (!_codeLoaded)
	{
		_log->Error("Cannot export code in target language, original code was not valid");
		return false;
	}

	// Disassemble the SPIRV to the output
	spvtools::SpirvTools context(_targetEnvironmentNative);
	context.SetMessageConsumer([this](spv_message_level_t level, const char* source, const spv_position_t& position, const char* message) { ForwardLogMessage("SpirvTools::Disassemble", level, source, position, message); });
	uint32_t disassemblyOptions = SPV_BINARY_TO_TEXT_OPTION_FRIENDLY_NAMES | SPV_BINARY_TO_TEXT_OPTION_SHOW_BYTE_OFFSET;
	if (!context.Disassemble(_spirvCode.data(), _spirvCode.size(), &outputCode, disassemblyOptions))
	{
		_log->Error("Failed to export SPIR-V code as text");
		return false;
	}
	return true;
}

//----------------------------------------------------------------------------------------
bool ShaderCode::ExportCodeAsSPIRV(std::vector<uint32_t>& outputCode) const
{
	// Initialize the code output and validate we have valid code to export
	outputCode.clear();
	if (!_codeLoaded)
	{
		_log->Error("Cannot export code in target language, original code was not valid");
		return false;
	}

	// Copy the SPIRV code directly to the output
	outputCode = _spirvCode;
	return true;
}

//----------------------------------------------------------------------------------------
bool ShaderCode::ExportCodeAsGLSL(std::string& outputCode, bool hasGeometryStage) const
{
	// Initialize the code output and validate we have valid code to export
	outputCode.clear();
	if (!_codeLoaded)
	{
		_log->Error("Cannot export code in target language, original code was not valid");
		return false;
	}

	// Set our GLSL compiler options
	spirv_cross::CompilerGLSL::Options opt;
	opt.version = 430;
	switch (_targetEnvironment)
	{
	case IShaderCode::Environment::OpenGL_33:
		opt.version = 330;
		break;
	case IShaderCode::Environment::OpenGL_43:
		opt.version = 430;
		break;
	}
	opt.enable_420pack_extension = false;

	// Compile the SPIRV code to GLSL
	spirv_cross::CompilerGLSL compiler(_spirvCode);
	compiler.set_common_options(opt);
	std::string glslCode;
	try
	{
		glslCode = compiler.compile();
	}
	catch (const std::runtime_error& e)
	{
		_log->Error("std::runtime_error in ExportCodeAsGLSL: {0}", e.what());
		return false;
	}

	// Under the MSVC STL, std::regex was multi-line by default, with no std::regex_constants::multiline option, right
	// up until the VS2026 release, when it became single-line by default and std::regex_constants::multiline was added.
	// To deal with compatibility issues, we use this helper to construct a multi-line regex which will work across
	// compiler versions.
	// https://developercommunity.visualstudio.com/t/multiline-c/268592
	// https://github.com/microsoft/STL/pull/5535
	auto constructMultiLineRegex = [](const std::string& regexString) {
#if defined(_MSC_VER) && _MSC_VER < 1950
		return std::regex(regexString);
#else
		return std::regex(regexString, std::regex_constants::multiline);
#endif
	};

	// Remove uniform block instance names
	std::regex uniformBlockNameSearch = constructMultiLineRegex("^\\} (_\\d+);$");
	std::sregex_iterator regexMatchIterator(glslCode.begin(), glslCode.end(), uniformBlockNameSearch);
	std::sregex_iterator regexMatchEnd;
	std::vector<std::string> blockNames;
	while (regexMatchIterator != regexMatchEnd)
	{
		std::string blockName = (*regexMatchIterator)[1];
		blockNames.push_back(blockName);
		++regexMatchIterator;
	}
	for (auto& blockName : blockNames)
	{
		StringReplaceAll(glslCode, "} " + blockName + ";", "};");
		StringReplaceAll(glslCode, "" + blockName + ".", "");
	}

	// Strip std140 layout qualifiers from uniform blocks
	StringReplaceAll(glslCode, "layout(std140) ", "");
	StringReplaceAll(glslCode, ", std140)", ")");

	//##TODO## Determine if we need this
	//StringReplaceAll(glslCode, "layout(row_major) ", "");

	// Strip "_entryPointOutput" annotations
	StringReplaceAll(glslCode, "_entryPointOutput_", "");
	glslCode = std::regex_replace(glslCode, constructMultiLineRegex("(\\W)_entryPointOutput(\\W)"), "$1entryPointOutput$2");

	// Determine the input/output prefixes to use for this shader stage
	std::string outputPrefix;
	std::string previousOutputPrefix;
	switch (_stage)
	{
	case IShaderCode::Stage::Vertex:
		outputPrefix = "VSOUT_";
		break;
	case IShaderCode::Stage::Geometry:
		outputPrefix = "GSOUT_";
		previousOutputPrefix = "VSOUT_";
		break;
	case IShaderCode::Stage::Fragment:
		outputPrefix = "FSOUT_";
		previousOutputPrefix = (hasGeometryStage ? "GSOUT_" : "VSOUT_");
		break;
	case IShaderCode::Stage::Compute:
		outputPrefix = "CSOUT_";
		break;
	}

	// Rename shader inputs/outputs to use the correct prefixes. This will ensure input attribute names are preserved,
	// while "private" inputs/outputs between shader stages have correct matching names between shader stages, while
	// still keeping the names unique. This ensures geometry shaders in particular can properly link under GLSL 3.30,
	// where we can't specify location attributes for inputs/outputs between shader stages, while keeping things
	// generally sane with matching names.
	auto addInterfacePrefix = [](const std::string& prefix, std::string variableName) {
		while (!variableName.empty() && variableName.front() == '_')
		{
			variableName.erase(variableName.begin());
		}
		return prefix + variableName;
	};
	std::regex interfaceVariableNameSearch = constructMultiLineRegex("^([ \\t]*(?:layout\\([^\\)]*\\)[ \\t]*)?(?:(?:flat|smooth|noperspective|centroid|sample|invariant|precise)[ \\t]+)*)(in|out)([ \\t]+(?:[a-zA-Z_][a-zA-Z0-9_]*[ \\t]+)+)([a-zA-Z_][a-zA-Z0-9_]*)(\\[[^\\]]+\\])?([ \\t]*;[ \\t]*)$");
	std::sregex_iterator regexMatchIterator1(glslCode.begin(), glslCode.end(), interfaceVariableNameSearch);
	std::sregex_iterator regexMatchEnd1;
	std::map<std::string, std::string> interfaceVariableRemap;
	while (regexMatchIterator1 != regexMatchEnd1)
	{
		std::string qualifier = (*regexMatchIterator1)[2];
		std::string variableName = (*regexMatchIterator1)[4];
		std::string newVariableName;
		if (qualifier == "in")
		{
			if (variableName.rfind("IN_", 0) == 0)
			{
				newVariableName = addInterfacePrefix(previousOutputPrefix, variableName.substr(3));
			}
		}
		else if ((qualifier == "out") && !outputPrefix.empty() && (variableName.rfind(outputPrefix, 0) != 0))
		{
			newVariableName = addInterfacePrefix(outputPrefix, variableName);
		}

		if (!newVariableName.empty() && (newVariableName != variableName))
		{
			interfaceVariableRemap.insert(std::make_pair(variableName, newVariableName));
		}
		++regexMatchIterator1;
	}
	for (const auto& entry : interfaceVariableRemap)
	{
		glslCode = std::regex_replace(glslCode, constructMultiLineRegex("(\\W)" + entry.first + "(\\W)"), "$1" + entry.second + "$2");
	}

	// Shift global uniforms out of the _Global uniform buffer into true global uniform variables
	glslCode = std::regex_replace(glslCode, constructMultiLineRegex("layout\\([a-zA-Z0-9 =,]+\\) uniform _Global"), "uniform _Global");
	std::string globalBufferSearchString = "uniform _Global";
	size_t globalBlockStartPos = glslCode.find(globalBufferSearchString);
	if (globalBlockStartPos != std::string::npos)
	{
		size_t globalVarListStartPos = glslCode.find_first_of('{', globalBlockStartPos);
		size_t globalVarListEndPos = glslCode.find_first_of('}', globalBlockStartPos);
		globalVarListEndPos = glslCode.find_first_of('\n', globalVarListEndPos);
		++globalVarListEndPos;

		std::vector<std::string> globalVars;
		size_t currentLinePos = glslCode.find_first_of('\n', globalVarListStartPos);
		while (currentLinePos != std::string::npos)
		{
			++currentLinePos;
			size_t nextLinePos = glslCode.find_first_of('\n', currentLinePos);
			if ((nextLinePos + 1) >= globalVarListEndPos)
			{
				nextLinePos = std::string::npos;
			}
			else if (nextLinePos != std::string::npos)
			{
				std::string globalVar = glslCode.substr(currentLinePos, nextLinePos - currentLinePos);
				StringTrim(globalVar);
				if (!globalVar.empty())
				{
					globalVars.push_back("uniform " + globalVar);
				}
			}
			currentLinePos = nextLinePos;
		}

		glslCode.replace(globalBlockStartPos, globalVarListEndPos - globalBlockStartPos, "");

		size_t globalVarInsertPos = globalBlockStartPos;
		for (auto& globalVar : globalVars)
		{
			glslCode.insert(globalVarInsertPos, globalVar + "\n");
			globalVarInsertPos += globalVar.size() + 1;
		}
	}

	// Return the modified code to the caller
	outputCode = std::move(glslCode);
	return true;
}

//----------------------------------------------------------------------------------------
bool ShaderCode::ExportCodeAsMSL(std::string& outputCode) const
{
	// Initialize the code output and validate we have valid code to export
	outputCode.clear();
	if (!_codeLoaded)
	{
		_log->Error("Cannot export code in target language, original code was not valid");
		return false;
	}

	// Export the code as MSL
	spirv_cross::CompilerMSL compiler(_spirvCode);
	try
	{
		outputCode = compiler.compile();
	}
	catch (const std::runtime_error& e)
	{
		_log->Error("std::runtime_error in ExportCodeAsMSL: {0}", e.what());
		return false;
	}
	return true;
}

//----------------------------------------------------------------------------------------
bool ShaderCode::HasLoadedCode() const
{
	return _codeLoaded;
}

//----------------------------------------------------------------------------------------
bool ShaderCode::ValidateCode() const
{
	spvtools::SpirvTools context(_targetEnvironmentNative);
	context.SetMessageConsumer([this](spv_message_level_t level, const char* source, const spv_position_t& position, const char* message) { ForwardLogMessage("SpirvTools::Validate", level, source, position, message); });
	if (!context.Validate(_spirvCode.data(), _spirvCode.size()))
	{
		_log->Error("Failed to validate code");
		return false;
	}

	return true;
}

//----------------------------------------------------------------------------------------
void ShaderCode::ForwardLogMessage(const char* sourceSystem, spv_message_level_t level, const char* source, const spv_position_t& position, const char* message) const
{
	// Convert the severity level to our logging system
	cobalt::logging::ILogger::Severity severity = logging::ILogger::Severity::Info;
	switch (level)
	{
	case SPV_MSG_FATAL:
	case SPV_MSG_INTERNAL_ERROR:
	case SPV_MSG_ERROR:
		severity = logging::ILogger::Severity::Error;
		break;
	case SPV_MSG_WARNING:
		severity = logging::ILogger::Severity::Warning;
		break;
	case SPV_MSG_INFO:
	case SPV_MSG_DEBUG:
		severity = logging::ILogger::Severity::Info;
		break;
	}

	// Log the message in our logging system
	_log->Log(severity, "{0} - Line {1}:{2} {3}", sourceSystem, position.line, position.column, message);
}

//----------------------------------------------------------------------------------------
// Reflection methods
//----------------------------------------------------------------------------------------
void ShaderCode::GetUniformFields(std::vector<ShaderCode::Resource>* resourceVec, spirv_cross::SPIRType type) const
{
	auto memberCount = (uint32_t)type.member_types.size();
	for (uint32_t i = 0; i < memberCount; ++i)
	{
		const auto& member = _compiler->get_type(type.member_types[i]);

		Resource shaderResource = {};
		shaderResource.id = type.self;
		shaderResource.name = _compiler->get_member_name(type.self, i);
		shaderResource.valid = true;
		shaderResource.location = 0;
		shaderResource.binding = 0;
		shaderResource.order = i;
		shaderResource.offset = _compiler->type_struct_member_offset(type, i);

		// Recursively get member resources (if available)
		if (!member.member_types.empty())
		{
			shaderResource.type = ResourceType::StructField;
			shaderResource.nativeVulkanFormat = VK_FORMAT_UNDEFINED;

			uint32_t fullArray = 1;
			for (uint32_t arraySize : member.array)
			{
				fullArray *= arraySize;
			}
			shaderResource.arraySizes = std::vector<uint32_t>(member.array.data(), member.array.data() + member.array.size());

			std::reverse(shaderResource.arraySizes.begin(), shaderResource.arraySizes.end());
			shaderResource.elementCount = fullArray;
			shaderResource.elementSize = _compiler->get_declared_struct_size(member);
			shaderResource.size = shaderResource.elementSize * shaderResource.elementCount;

			GetUniformFields(&shaderResource.fields, member);
		}
		else
		{
			shaderResource.type = ResourceType::Field;
			shaderResource.elementCount = 1;

			GetNativeVulkanFormatAndSize(&shaderResource, member);
			shaderResource.size = _compiler->get_declared_struct_member_size(type, i);
		}

		// Calculate the stride of the innermost element if this field is an array
		shaderResource.elementStride = shaderResource.size;
		if (!shaderResource.arraySizes.empty())
		{
			// Calculate the stride of the outermost dimension of a potentially multidimensional array
			shaderResource.elementStride = _compiler->type_struct_member_array_stride(type, i);

			// If this is a multidimensional array, derive the stride of the innermost element.
			if (shaderResource.arraySizes.size() > 1)
			{
				size_t divisor = 1;
				for (size_t j = 1; j < shaderResource.arraySizes.size(); ++j)
				{
					divisor *= shaderResource.arraySizes[j];
				}
				shaderResource.elementStride /= divisor;
			}
		}

		resourceVec->push_back(shaderResource);
	}
}

//----------------------------------------------------------------------------------------
VkFormat ShaderCode::GetNativeVulkanFormatFromImageFormat(spv::ImageFormat imageFormat)
{
	switch (imageFormat)
	{
	case spv::ImageFormatRgba32f:
		return VK_FORMAT_R32G32B32A32_SFLOAT;
	case spv::ImageFormatRgba16f:
		return VK_FORMAT_R16G16B16A16_SFLOAT;
	case spv::ImageFormatR32f:
		return VK_FORMAT_R32_SFLOAT;
	case spv::ImageFormatRgba8:
		return VK_FORMAT_R8G8B8A8_UNORM;
	case spv::ImageFormatRgba8Snorm:
		return VK_FORMAT_R8G8B8A8_SNORM;
	case spv::ImageFormatRg32f:
		return VK_FORMAT_R32G32_SFLOAT;
	case spv::ImageFormatRg16f:
		return VK_FORMAT_R16G16_SFLOAT;
	case spv::ImageFormatR11fG11fB10f:
		return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
	case spv::ImageFormatR16f:
		return VK_FORMAT_R16_SFLOAT;
	case spv::ImageFormatRgba16:
		return VK_FORMAT_R16G16B16A16_UNORM;
	case spv::ImageFormatRgb10A2:
		return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
	case spv::ImageFormatRg16:
		return VK_FORMAT_R16G16_UNORM;
	case spv::ImageFormatRg8:
		return VK_FORMAT_R8G8_UNORM;
	case spv::ImageFormatR16:
		return VK_FORMAT_R16_UNORM;
	case spv::ImageFormatR8:
		return VK_FORMAT_R8_UNORM;
	case spv::ImageFormatRgba16Snorm:
		return VK_FORMAT_R16G16B16A16_SNORM;
	case spv::ImageFormatRg16Snorm:
		return VK_FORMAT_R16G16_SNORM;
	case spv::ImageFormatRg8Snorm:
		return VK_FORMAT_R8G8_SNORM;
	case spv::ImageFormatR16Snorm:
		return VK_FORMAT_R16_SNORM;
	case spv::ImageFormatR8Snorm:
		return VK_FORMAT_R8_SNORM;
	case spv::ImageFormatRgba32i:
		return VK_FORMAT_R32G32B32A32_SINT;
	case spv::ImageFormatRgba16i:
		return VK_FORMAT_R16G16B16A16_SINT;
	case spv::ImageFormatRgba8i:
		return VK_FORMAT_R8G8B8A8_SINT;
	case spv::ImageFormatR32i:
		return VK_FORMAT_R32_SINT;
	case spv::ImageFormatRg32i:
		return VK_FORMAT_R32G32_SINT;
	case spv::ImageFormatRg16i:
		return VK_FORMAT_R16G16_SINT;
	case spv::ImageFormatRg8i:
		return VK_FORMAT_R8G8_SINT;
	case spv::ImageFormatR16i:
		return VK_FORMAT_R16_SINT;
	case spv::ImageFormatR8i:
		return VK_FORMAT_R8_SINT;
	case spv::ImageFormatRgba32ui:
		return VK_FORMAT_R32G32B32A32_UINT;
	case spv::ImageFormatRgba16ui:
		return VK_FORMAT_R16G16B16A16_UINT;
	case spv::ImageFormatRgba8ui:
		return VK_FORMAT_R8G8B8A8_UINT;
	case spv::ImageFormatR32ui:
		return VK_FORMAT_R32_UINT;
	case spv::ImageFormatRgb10a2ui:
		return VK_FORMAT_A2B10G10R10_UINT_PACK32;
	case spv::ImageFormatRg32ui:
		return VK_FORMAT_R32G32_UINT;
	case spv::ImageFormatRg16ui:
		return VK_FORMAT_R16G16_UINT;
	case spv::ImageFormatRg8ui:
		return VK_FORMAT_R8G8_UINT;
	case spv::ImageFormatR16ui:
		return VK_FORMAT_R16_UINT;
	case spv::ImageFormatR8ui:
		return VK_FORMAT_R8_UINT;
	case spv::ImageFormatR64ui:
		return VK_FORMAT_R64_UINT;
	case spv::ImageFormatR64i:
		return VK_FORMAT_R64_SINT;
	}
	return VK_FORMAT_UNDEFINED;
}

//----------------------------------------------------------------------------------------
void ShaderCode::GetNativeVulkanFormatAndSize(Resource* resource, spirv_cross::SPIRType type) const
{
	// Calculate type and size information for SPIRV resource

	// Calculate size and stride info for resource
	resource->width = type.width / 8;
	resource->vecSize = type.vecsize;
	resource->columns = type.columns;

	uint32_t fullArray = 1;
	for (uint32_t arraySize : type.array)
	{
		fullArray *= arraySize;
	}
	resource->arraySizes = std::vector<uint32_t>(type.array.data(), type.array.data() + type.array.size());

	// Arrays provided in opposite order to reference, reverse for convenience
	std::reverse(resource->arraySizes.begin(), resource->arraySizes.end());

	resource->elementCount = fullArray;
	resource->elementSize = resource->width * resource->vecSize * resource->columns;

	resource->nativeVulkanFormat = VK_FORMAT_UNDEFINED;
	if (type.basetype == spirv_cross::SPIRType::BaseType::Int)
	{
		resource->layoutType = DataType::Int32;
		resource->elementStride = 16;
		if (type.vecsize == 1)
		{
			resource->nativeVulkanFormat = VK_FORMAT_R32_SINT;
		}
		else if (type.vecsize == 2)
		{
			resource->nativeVulkanFormat = VK_FORMAT_R32G32_SINT;
		}
		else if (type.vecsize == 3)
		{
			resource->nativeVulkanFormat = VK_FORMAT_R32G32B32_SINT;
		}
		else if (type.vecsize == 4)
		{
			resource->nativeVulkanFormat = VK_FORMAT_R32G32B32A32_SINT;
		}
	}
	else if (type.basetype == spirv_cross::SPIRType::BaseType::UInt)
	{
		resource->layoutType = DataType::UInt32;
		resource->elementStride = 16;
		if (type.vecsize == 1)
		{
			resource->nativeVulkanFormat = VK_FORMAT_R32_UINT;
		}
		else if (type.vecsize == 2)
		{
			resource->nativeVulkanFormat = VK_FORMAT_R32G32_UINT;
		}
		else if (type.vecsize == 3)
		{
			resource->nativeVulkanFormat = VK_FORMAT_R32G32B32_UINT;
		}
		else if (type.vecsize == 4)
		{
			resource->nativeVulkanFormat = VK_FORMAT_R32G32B32A32_UINT;
		}
	}
	else if (type.basetype == spirv_cross::SPIRType::BaseType::Int64)
	{
		//##TODO## support or ignore 64 bit ints
		resource->layoutType = DataType::Int32;
		resource->elementStride = 32;
		if (type.vecsize == 1)
		{
			resource->nativeVulkanFormat = VK_FORMAT_R64_SINT;
		}
		else if (type.vecsize == 2)
		{
			resource->nativeVulkanFormat = VK_FORMAT_R64G64_SINT;
		}
		else if (type.vecsize == 3)
		{
			resource->nativeVulkanFormat = VK_FORMAT_R64G64B64_SINT;
		}
		else if (type.vecsize == 4)
		{
			resource->nativeVulkanFormat = VK_FORMAT_R64G64B64A64_SINT;
		}
	}
	else if (type.basetype == spirv_cross::SPIRType::BaseType::UInt64)
	{
		resource->layoutType = DataType::UInt32;
		resource->elementStride = 32;
		if (type.vecsize == 1)
		{
			resource->nativeVulkanFormat = VK_FORMAT_R64_UINT;
		}
		else if (type.vecsize == 2)
		{
			resource->nativeVulkanFormat = VK_FORMAT_R64G64_UINT;
		}
		else if (type.vecsize == 3)
		{
			resource->nativeVulkanFormat = VK_FORMAT_R64G64B64_UINT;
		}
		else if (type.vecsize == 4)
		{
			resource->nativeVulkanFormat = VK_FORMAT_R64G64B64A64_UINT;
		}
	}
	else if (type.basetype == spirv_cross::SPIRType::BaseType::Float)
	{
		resource->layoutType = DataType::Float32;
		resource->elementStride = 16;
		if (type.vecsize == 1)
		{
			resource->nativeVulkanFormat = VK_FORMAT_R32_SFLOAT;
		}
		else if (type.vecsize == 2)
		{
			resource->nativeVulkanFormat = VK_FORMAT_R32G32_SFLOAT;
		}
		else if (type.vecsize == 3)
		{
			resource->nativeVulkanFormat = VK_FORMAT_R32G32B32_SFLOAT;
		}
		else if (type.vecsize == 4)
		{
			resource->nativeVulkanFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
		}
	}
	else if (type.basetype == spirv_cross::SPIRType::BaseType::Double)
	{
		resource->layoutType = DataType::Float64;
		resource->elementStride = 32;
		if (type.vecsize == 1)
		{
			resource->nativeVulkanFormat = VK_FORMAT_R64_SFLOAT;
		}
		else if (type.vecsize == 2)
		{
			resource->nativeVulkanFormat = VK_FORMAT_R64G64_SFLOAT;
		}
		else if (type.vecsize == 3)
		{
			resource->nativeVulkanFormat = VK_FORMAT_R64G64B64_SFLOAT;
		}
		else if (type.vecsize == 4)
		{
			resource->nativeVulkanFormat = VK_FORMAT_R64G64B64A64_SFLOAT;
		}
	}
	else if (type.basetype == spirv_cross::SPIRType::BaseType::Char || type.basetype == spirv_cross::SPIRType::BaseType::Boolean)
	{
		resource->layoutType = DataType::Boolean;
		resource->elementStride = 16;
		if (type.vecsize == 1)
		{
			resource->nativeVulkanFormat = VK_FORMAT_R8_UINT;
		}
		else if (type.vecsize == 2)
		{
			resource->nativeVulkanFormat = VK_FORMAT_R8G8_UINT;
		}
		else if (type.vecsize == 3)
		{
			resource->nativeVulkanFormat = VK_FORMAT_R8G8B8_UINT;
		}
		else if (type.vecsize == 4)
		{
			resource->nativeVulkanFormat = VK_FORMAT_R8G8B8A8_UINT;
		}
	}
	resource->elementStride *= resource->columns;
	resource->size = resource->elementStride * resource->elementCount;
}

//----------------------------------------------------------------------------------------
// Code modification
//----------------------------------------------------------------------------------------
void ShaderCode::ChangeName(const std::string& name, uint32_t id)
{
	for (size_t i = 1; i < _spirvCode.size(); i++)
	{
		// if (OpName instruction AND effecting resource with ID)
		if (((_spirvCode[i - 1] & 0xFFFF) == 5) && (_spirvCode[i] == id))
		{
			// Get instruction length
			int length = _spirvCode[i - 1] >> 16;

			// Calculate new length
			int newLength = 3 + ((int)name.size() / 4);

			// Remove and make room for instruction
			_spirvCode.erase(_spirvCode.begin() + i - 1, _spirvCode.begin() + i + length - 1);
			_spirvCode.insert(_spirvCode.begin() + i - 1, newLength, 0);

			// OpName instruction
			_spirvCode[i - 1] = (newLength << 16) + 0x05;

			// Acting on resource with ID
			_spirvCode[i] = id;

			// Changing name
			std::memcpy(&_spirvCode[i + 1], name.c_str(), name.size());
			return;
		}
	}
}

//----------------------------------------------------------------------------------------
void ShaderCode::ChangeBinding(uint32_t binding, uint32_t id)
{
	for (size_t i = 1; i < _spirvCode.size() - 2; i++)
	{
		//##TODO## Is the get_binary_offset_for_decoration method what we should use here?
		// if (OpDecorate instruction AND effecting resource with ID AND changing Binding)
		if (((_spirvCode[i - 1] & 0xFFFF) == 71) && (_spirvCode[i] == id) && (_spirvCode[i + 1] == 33))
		{
			_spirvCode[i + 2] = binding;
			return;
		}
	}
}

//----------------------------------------------------------------------------------------
void ShaderCode::ChangeDescriptorSet(uint32_t descriptorSet, uint32_t id)
{
	for (size_t i = 1; i < _spirvCode.size() - 2; i++)
	{
		// if (OpDecorate instruction AND effecting resource with ID AND changing DescriptorSet)
		if (((_spirvCode[i - 1] & 0xFFFF) == 71) && (_spirvCode[i] == id) && (_spirvCode[i + 1] == 34))
		{
			_spirvCode[i + 2] = descriptorSet;
			return;
		}
	}
}

//----------------------------------------------------------------------------------------
// Code patch methods
//----------------------------------------------------------------------------------------
// NOLINTBEGIN
// Fixes bad GLSL to SPIR-V generated by glslang for Vulkan. Without this fix, we get errors like this:
//   Validator Error VUID-VkShaderModuleCreateInfo-pCode-01091: Validation Error:
//   [ VUID-VkShaderModuleCreateInfo-pCode-01091 ] Object 0: handle = 0x1dbf44e1780, type = VK_OBJECT_TYPE_DEVICE;
//   | MessageID = 0xa7bb8db6 | vkCreateShaderModule(): The SPIR-V Capability (RuntimeDescriptorArray) was declared,
//   but none of the requirements were met to use it. The Vulkan spec states: If pCode declares any of the capabilities
//   listed in the SPIR-V Environment appendix, one of the corresponding requirements must be satisfied
//   (https://www.khronos.org/registry/vulkan/specs/1.3-extensions/html/vkspec.html#VUID-VkShaderModuleCreateInfo-pCode-01091)
// This is caused because glslang is introducing dynamic array sizing for the ClipDistance array it adds in
// automatically, and that isn't supported in Vulkan. There's a separate fix to deal with the ClipDistance array itself,
// but this fix removes the request for the dynamic array sizing feature itself.
//##TODO## ChatGPT generated. Needs review.
bool ShaderCode::StripDescriptorIndexingDecls(std::vector<uint32_t>& spirv)
{
	auto op = [](uint32_t w) -> uint32_t { return (w & 0xFFFFu); };
	auto wc = [](uint32_t w) -> uint32_t { return (w >> 16); };

	auto spirvStrEq = [&](size_t firstWord, size_t wordCount, const char* s) -> bool {
		size_t si = 0;
		for (size_t w = 0; w < wordCount; ++w)
		{
			uint32_t v = spirv[firstWord + w];
			for (int b = 0; b < 4; ++b)
			{
				char c = char((v >> (8 * b)) & 0xFF);
				if (c == '\0') return s[si] == '\0';
				if (s[si] == '\0' || s[si] != c) return false;
				++si;
			}
		}
		return false; // no NUL terminator found within provided words
	};

	if (spirv.size() < 5 || spirv[0] != 0x07230203u)
		return false;

	std::vector<uint32_t> out;
	out.reserve(spirv.size());
	out.insert(out.end(), spirv.begin(), spirv.begin() + 5);

	bool changed = false;

	for (size_t i = 5; i < spirv.size();)
	{
		uint32_t w0 = spirv[i];
		uint32_t n = wc(w0);
		uint32_t o = op(w0);

		if (n == 0 || i + n > spirv.size())
		{
			out.insert(out.end(), spirv.begin() + i, spirv.end());
			break;
		}

		bool skip = false;

		// OpCapability RuntimeDescriptorArray (17, wc=2, cap=0x14B6)
		if (o == 17 && n == 2 && spirv[i + 1] == 0x000014B6u)
		{
			skip = true;
			changed = true;
		}

		// OpExtension "SPV_EXT_descriptor_indexing" (10, wc>=2, string at i+1)
		if (!skip && o == 10 && n >= 2 && spirvStrEq(i + 1, n - 1, "SPV_EXT_descriptor_indexing"))
		{
			skip = true;
			changed = true;
		}

		if (!skip)
			out.insert(out.end(), spirv.begin() + i, spirv.begin() + i + n);

		i += n;
	}

	if (changed) spirv.swap(out);
	return changed;
}

//----------------------------------------------------------------------------------------
// Fixes bad GLSL to SPIR-V generated by glslang for Vulkan. Without this fix, we get errors like this:
//   Validator Error UNASSIGNED-CoreValidation-Shader-PointSizeMissing: Validation Error:
//   [ UNASSIGNED-CoreValidation-Shader-PointSizeMissing ] Object 0: VK_NULL_HANDLE, type = VK_OBJECT_TYPE_PIPELINE;
//   | MessageID = 0xf3693078 | Pipeline topology is set to POINT_LIST, but PointSize is not written to in the shader
//   corresponding to VK_SHADER_STAGE_VERTEX_BIT.
// This is caused by the fact that writing PointSize is mandatory in Vulkan, but glslang never adds this in. We solve
// it here by injecting a write to the variable, with it fixed at 1. We don't need to support actually changing it,
// because point sizes other than 1 are non-portable (no Direct3D support), and not supported by the Cobalt renderer.
//
// Robustly forces PointSize = 1.0 for *all* Vertex entry points in a SPIR-V module.
// This is intended to satisfy Vulkan validation / spec requirements for POINT_LIST pipelines
// where a PointSize-decorated output must be written by the vertex shader.
//
// Design goals / robustness:
//  - Patch *any* vertex shader module, even if it originally contains no float types.
//  - Do not depend on gl_PerVertex struct layouts or member access chains.
//  - Do not assume the module has an int type (many don't).
//  - Do not assume the entry function has only OpReturn exits.
//  - Inject the write at function start in a SPIR-V-valid place:
//      after the entry OpLabel AND after any OpVariable(Function) local declarations.
//
// What we do, in order:
//  1) PASS 1: scan the module to gather metadata:
//       - All Vertex OpEntryPoint function ids.
//       - The earliest OpFunction (insertion point for types/consts/vars).
//       - The earliest type/const/var (insertion point for decorations).
//       - Existing OpTypeFloat 32 (if any).
//       - Existing OpConstant(float 1.0) (if any).
//       - Any ids decorated BuiltIn PointSize (OpDecorate).
//       - Map of pointer types (OpTypePointer) and variables (OpVariable) for ids < bound.
//  2) Decide what we need to create:
//       - Ensure float32 type exists (OpTypeFloat 32).
//       - Ensure ptr(Output,float32) exists (OpTypePointer Output float32).
//       - Ensure const float32 1.0 exists (OpConstant 1.0).
//       - Reuse an existing Output float32 variable decorated BuiltIn PointSize if possible.
//         Otherwise create a new Output float32 variable and decorate it BuiltIn PointSize.
//       - IMPORTANT: If the module already mentions BuiltIn PointSize but we can't identify
//         a usable Output float32 variable for it, we fail safely rather than risking a
//         duplicate BuiltIn PointSize output (which may cause new validation errors).
//  3) PASS 2: rewrite the module, applying changes:
//       - Insert new OpDecorate (if creating the PointSize var).
//       - Insert new OpTypeFloat / OpTypePointer / OpConstant / OpVariable as needed.
//       - For every Vertex OpEntryPoint, ensure the PointSize var is in the interface list.
//       - For every Vertex entry function, inject:
//            OpStore %PointSizeVar %Const1
//         at the earliest valid point in the entry block (after local OpVariable(Function)s).
//
// Returns true if modifications were made; false if not applicable or unsafe to patch.
//
// Notes:
//  - Patches *all* Vertex entry points in the module by injecting:
//      gl_PerVertex.PointSize = 1.0;
//    or (fallback) a standalone Output BuiltIn PointSize variable set to 1.0.
//  - Does NOT try to detect if shader already writes PointSize; redundant write is fine.
//  - Does NOT rely on gl_PerVertex member layouts (no OpAccessChain, no int constants needed).
//  - If the module already has a BuiltIn PointSize decoration but it's not an Output var of float32,
//    we return false rather than create a duplicate BuiltIn PointSize output (duplicates can be invalid).
//##TODO## ChatGPT generated. Needs review.
bool ShaderCode::FixVertexPointSizeAlwaysOne(std::vector<uint32_t>& spirv)
{
	auto op = [](uint32_t w) { return uint16_t(w & 0xFFFFu); };
	auto wc = [](uint32_t w) { return uint16_t(w >> 16); };
	auto f1u32 = []() { return 0x3f800000u; }; // 1.0f

	if (spirv.size() < 5 || spirv[0] != 0x07230203u) return false;

	const uint32_t originalBound = spirv[3];
	uint32_t nextId = originalBound;

	//------------------------------------------------------------------------------------
	// PASS 1: scan module
	//------------------------------------------------------------------------------------
	std::vector<uint32_t> vertexEntryIds;
	vertexEntryIds.reserve(4);

	size_t firstOpFunction = spirv.size();
	size_t firstTypeConstVar = spirv.size();

	uint32_t floatTy32 = 0;
	uint32_t intTy32 = 0;

	// Existing standalone PointSize var via OpDecorate BuiltIn PointSize
	std::vector<uint32_t> decoratedPointSizeIds;

	// Existing member PointSize via OpMemberDecorate BuiltIn PointSize
	uint32_t pvStructTy = 0;
	uint32_t pvMemberIdx = 0xFFFFFFFFu;

	struct PtrInfo
	{
		uint32_t storage = 0, pointee = 0;
		bool valid = false;
	};
	struct VarInfo
	{
		uint32_t ptrType = 0, storage = 0;
		bool valid = false;
	};
	std::vector<PtrInfo> ptrInfo(originalBound);
	std::vector<VarInfo> varInfo(originalBound);

	auto isTypeConstVarOp = [&](uint16_t opcode) {
		return (opcode == 19 || opcode == 20 || opcode == 21 || opcode == 22 || opcode == 23 || opcode == 24 ||
		        opcode == 25 || opcode == 26 || opcode == 27 || opcode == 28 || opcode == 29 || opcode == 30 ||
		        opcode == 31 || opcode == 32 ||
		        opcode == 41 || opcode == 42 || opcode == 43 || opcode == 59);
	};

	for (size_t i = 5; i < spirv.size();)
	{
		uint16_t W = wc(spirv[i]), O = op(spirv[i]);
		if (!W || i + W > spirv.size()) break;

		if (firstOpFunction == spirv.size() && O == 54 /*OpFunction*/)
			firstOpFunction = i;

		if (firstTypeConstVar == spirv.size() && isTypeConstVarOp(O))
			firstTypeConstVar = i;

		if (O == 15 /*OpEntryPoint*/ && W >= 4 && spirv[i + 1] == 0u /*Vertex*/)
			vertexEntryIds.push_back(spirv[i + 2]);

		if (O == 22 /*OpTypeFloat*/ && W == 3 && spirv[i + 2] == 32u)
			floatTy32 = spirv[i + 1];

		if (O == 21 /*OpTypeInt*/ && W == 4 && spirv[i + 2] == 32u)
			intTy32 = spirv[i + 1];

		if (O == 32 /*OpTypePointer*/ && W == 4)
		{
			uint32_t id = spirv[i + 1];
			if (id < originalBound)
			{
				ptrInfo[id].storage = spirv[i + 2];
				ptrInfo[id].pointee = spirv[i + 3];
				ptrInfo[id].valid = true;
			}
		}

		if (O == 59 /*OpVariable*/ && W >= 4)
		{
			uint32_t ptrType = spirv[i + 1];
			uint32_t id = spirv[i + 2];
			uint32_t storage = spirv[i + 3];
			if (id < originalBound)
			{
				varInfo[id].ptrType = ptrType;
				varInfo[id].storage = storage;
				varInfo[id].valid = true;
			}
		}

		// OpDecorate %id BuiltIn PointSize
		if (O == 71 /*OpDecorate*/ && W == 4 &&
		    spirv[i + 2] == 11u /*BuiltIn*/ && spirv[i + 3] == 1u /*PointSize*/)
		{
			decoratedPointSizeIds.push_back(spirv[i + 1]);
		}

		// OpMemberDecorate %type member BuiltIn PointSize
		if (O == 72 /*OpMemberDecorate*/ && W == 5 &&
		    spirv[i + 3] == 11u /*BuiltIn*/ && spirv[i + 4] == 1u /*PointSize*/)
		{
			pvStructTy = spirv[i + 1];
			pvMemberIdx = spirv[i + 2];
		}

		i += W;
	}

	if (vertexEntryIds.empty()) return false;
	if (firstOpFunction == spirv.size()) return false;
	if (firstTypeConstVar == spirv.size()) firstTypeConstVar = firstOpFunction;

	auto isVertexEntryId = [&](uint32_t id) {
		for (uint32_t v : vertexEntryIds)
			if (v == id) return true;
		return false;
	};

	//------------------------------------------------------------------------------------
	// Decide targets: standalone vs member
	//------------------------------------------------------------------------------------
	// We will always need float32 + const 1.0
	bool needFloatTy = (floatTy32 == 0);
	if (needFloatTy) floatTy32 = nextId++;

	// Find/ensure OpConstant float 1.0
	uint32_t constF1 = 0;
	for (size_t i = 5; i < spirv.size();)
	{
		uint16_t W = wc(spirv[i]), O = op(spirv[i]);
		if (!W || i + W > spirv.size()) break;
		if (O == 43 /*OpConstant*/ && W == 4 && spirv[i + 1] == floatTy32 && spirv[i + 3] == f1u32())
		{
			constF1 = spirv[i + 2];
			break;
		}
		i += W;
	}
	bool needConstF1 = (constF1 == 0);
	if (needConstF1) constF1 = nextId++;

	// Helper: verify standalone decorated id is an Output pointer-to-float32 variable
	auto isOutputFloat32Var = [&](uint32_t varId) -> bool {
		if (varId >= originalBound) return false;
		if (!varInfo[varId].valid) return false;
		if (varInfo[varId].storage != 3u /*Output*/) return false;
		uint32_t pty = varInfo[varId].ptrType;
		if (pty >= originalBound) return false;
		if (!ptrInfo[pty].valid) return false;
		if (ptrInfo[pty].storage != 3u /*Output*/) return false;
		return ptrInfo[pty].pointee == floatTy32;
	};

	// Prefer standalone PointSize if it exists (avoids access-chain/member index machinery)
	uint32_t pointSizeVar = 0;
	for (uint32_t id : decoratedPointSizeIds)
	{
		if (isOutputFloat32Var(id))
		{
			pointSizeVar = id;
			break;
		}
	}

	// If no standalone var, try member path via gl_PerVertex struct member
	bool useMemberPath = false;
	uint32_t pvPtrTy = 0, pvVarId = 0;
	uint32_t ptrOutFloat32 = 0;

	auto findPtrOutFloat32 = [&]() -> uint32_t {
		for (size_t i = 5; i < spirv.size();)
		{
			uint16_t W = wc(spirv[i]), O = op(spirv[i]);
			if (!W || i + W > spirv.size()) break;
			if (O == 32 /*OpTypePointer*/ && W == 4 &&
			    spirv[i + 2] == 3u /*Output*/ && spirv[i + 3] == floatTy32)
				return spirv[i + 1];
			i += W;
		}
		return 0;
	};
	ptrOutFloat32 = findPtrOutFloat32();
	bool needPtrOutFloat32 = (ptrOutFloat32 == 0);
	if (needPtrOutFloat32) ptrOutFloat32 = nextId++;

	uint32_t cIdx = 0;
	bool needIntTy32 = false;
	bool needCIdx = false;

	if (pointSizeVar == 0 && pvStructTy && pvMemberIdx != 0xFFFFFFFFu)
	{
		// Need an int type for the member index constant id
		needIntTy32 = (intTy32 == 0);
		if (needIntTy32) intTy32 = nextId++;

		// Find pointer-to(Output, pvStructTy) and the Output variable of that type
		for (size_t i = 5; i < spirv.size();)
		{
			uint16_t W = wc(spirv[i]), O = op(spirv[i]);
			if (!W || i + W > spirv.size()) break;

			if (O == 32 /*OpTypePointer*/ && W == 4 &&
			    spirv[i + 2] == 3u /*Output*/ && spirv[i + 3] == pvStructTy)
				pvPtrTy = spirv[i + 1];

			if (pvPtrTy && O == 59 /*OpVariable*/ && W >= 4 &&
			    spirv[i + 1] == pvPtrTy && spirv[i + 3] == 3u /*Output*/)
			{
				pvVarId = spirv[i + 2];
				break;
			}
			i += W;
		}

		if (pvVarId)
		{
			// Find existing const int pvMemberIdx, else create
			for (size_t i = 5; i < spirv.size();)
			{
				uint16_t W = wc(spirv[i]), O = op(spirv[i]);
				if (!W || i + W > spirv.size()) break;
				if (O == 43 /*OpConstant*/ && W == 4 &&
				    spirv[i + 1] == intTy32 && spirv[i + 3] == pvMemberIdx)
				{
					cIdx = spirv[i + 2];
					break;
				}
				i += W;
			}
			needCIdx = (cIdx == 0);
			if (needCIdx) cIdx = nextId++;
			useMemberPath = true;
		}
	}

	// If neither standalone nor member exists, create standalone (safe: no other PointSize present)
	bool needCreateStandalonePS = (!pointSizeVar && !useMemberPath);
	bool needDecorateStandalonePS = false;
	bool needDeclareStandalonePS = false;
	if (needCreateStandalonePS)
	{
		pointSizeVar = nextId++;
		needDecorateStandalonePS = true;
		needDeclareStandalonePS = true;
	}

	//------------------------------------------------------------------------------------
	// PASS 2: rewrite + inject stores
	//------------------------------------------------------------------------------------
	auto entryInterfaceStart = [&](size_t instStart, uint16_t W) -> size_t {
		size_t j = instStart + 3;
		for (; j < instStart + W; ++j)
		{
			uint32_t w = spirv[j];
			if ((w & 0x000000FFu) == 0u ||
			    (w & 0x0000FF00u) == 0u ||
			    (w & 0x00FF0000u) == 0u ||
			    (w & 0xFF000000u) == 0u)
				return j + 1;
		}
		return instStart + W;
	};
	auto hasIdIn = [&](size_t b, size_t e, uint32_t id) -> bool {
		for (size_t k = b; k < e; ++k)
			if (spirv[k] == id) return true;
		return false;
	};

	std::vector<uint32_t> out;
	out.reserve(spirv.size() + 256);
	out.insert(out.end(), spirv.begin(), spirv.begin() + 5);

	bool changed = false;

	bool inVertexEntryFunc = false;
	bool injectPending = false;
	bool afterEntryLabel = false;

	for (size_t i = 5; i < spirv.size();)
	{
		// Insert decorations (only if we created standalone PS var)
		if (i == firstTypeConstVar && needDecorateStandalonePS)
		{
			out.push_back((4u << 16) | 71u); // OpDecorate
			out.push_back(pointSizeVar);
			out.push_back(11u);
			out.push_back(1u);
			changed = true;
		}

		// Insert types/const/vars before first function
		if (i == firstOpFunction)
		{
			if (needFloatTy)
			{
				out.push_back((3u << 16) | 22u); // OpTypeFloat
				out.push_back(floatTy32);
				out.push_back(32u);
				changed = true;
			}
			if (needIntTy32)
			{
				out.push_back((4u << 16) | 21u); // OpTypeInt
				out.push_back(intTy32);
				out.push_back(32u);
				out.push_back(0u); // unsigned
				changed = true;
			}
			if (needPtrOutFloat32)
			{
				out.push_back((4u << 16) | 32u); // OpTypePointer
				out.push_back(ptrOutFloat32);
				out.push_back(3u);
				out.push_back(floatTy32);
				changed = true;
			}
			if (needConstF1)
			{
				out.push_back((4u << 16) | 43u); // OpConstant float 1.0
				out.push_back(floatTy32);
				out.push_back(constF1);
				out.push_back(f1u32());
				changed = true;
			}
			if (needCIdx)
			{
				out.push_back((4u << 16) | 43u); // OpConstant int memberIndex
				out.push_back(intTy32);
				out.push_back(cIdx);
				out.push_back(pvMemberIdx);
				changed = true;
			}
			if (needDeclareStandalonePS)
			{
				out.push_back((4u << 16) | 59u); // OpVariable Output float
				out.push_back(ptrOutFloat32);
				out.push_back(pointSizeVar);
				out.push_back(3u);
				changed = true;
			}
		}

		uint16_t W = wc(spirv[i]), O = op(spirv[i]);
		if (!W || i + W > spirv.size())
		{
			out.insert(out.end(), spirv.begin() + i, spirv.end());
			break;
		}

		// Patch Vertex OpEntryPoint interface list: include the written interface variable
		// - standalone path: pointSizeVar
		// - member path: pvVarId (gl_PerVertex output var)
		uint32_t ifaceId = useMemberPath ? pvVarId : pointSizeVar;
		if (O == 15 && W >= 4 && spirv[i + 1] == 0u /*Vertex*/ && ifaceId)
		{
			size_t ifaceStart = entryInterfaceStart(i, W);
			if (!hasIdIn(ifaceStart, i + W, ifaceId))
			{
				out.push_back(((uint32_t(W) + 1u) << 16) | 15u);
				out.insert(out.end(), spirv.begin() + i + 1, spirv.begin() + i + W);
				out.push_back(ifaceId);
				changed = true;
				i += W;
				continue;
			}
		}

		// Track function boundaries
		if (O == 54 /*OpFunction*/ && W == 5)
		{
			uint32_t funcId = spirv[i + 2];
			inVertexEntryFunc = isVertexEntryId(funcId);
			injectPending = inVertexEntryFunc;
			afterEntryLabel = false;
		}
		else if (O == 56 /*OpFunctionEnd*/)
		{
			inVertexEntryFunc = false;
			injectPending = false;
			afterEntryLabel = false;
		}

		// Special case: OpLabel must be emitted before any OpStore
		if (inVertexEntryFunc && O == 248 /*OpLabel*/ && W == 2)
		{
			out.insert(out.end(), spirv.begin() + i, spirv.begin() + i + W);
			afterEntryLabel = true;
			i += W;
			continue;
		}

		// Inject at earliest valid point: after entry label and after local OpVariable(Function)s
		if (inVertexEntryFunc && injectPending && afterEntryLabel)
		{
			bool isLocalVarDecl = (O == 59 && W >= 4 && spirv[i + 3] == 7u /*Function*/);
			if (!isLocalVarDecl)
			{
				if (!useMemberPath)
				{
					// OpStore %pointSizeVar %constF1
					out.push_back((3u << 16) | 62u);
					out.push_back(pointSizeVar);
					out.push_back(constF1);
				}
				else
				{
					// %tmp = OpAccessChain %ptrOutFloat32 %pvVarId %cIdx
					// OpStore %tmp %constF1
					uint32_t tmpId = nextId++;
					out.push_back((5u << 16) | 65u); // OpAccessChain
					out.push_back(ptrOutFloat32);
					out.push_back(tmpId);
					out.push_back(pvVarId);
					out.push_back(cIdx);

					out.push_back((3u << 16) | 62u); // OpStore
					out.push_back(tmpId);
					out.push_back(constF1);
				}

				injectPending = false;
				changed = true;
			}
		}

		out.insert(out.end(), spirv.begin() + i, spirv.begin() + i + W);
		i += W;
	}

	if (!changed) return false;

	out[3] = nextId;
	spirv.swap(out);
	return true;
}

//----------------------------------------------------------------------------------------
// Fixes duplicate HLSL ConstantBuffer<T> names in SPIR-V that cause spirv-cross to expose
// a generated "_t" name instead of the original HLSL resource name.
//
// Problem:
//   HLSL source may contain:
//
//       ConstantBuffer<UniformBuffer> myCB;
//
//   DXC can lower this to SPIR-V with two separate ids:
//
//       OpName %blockType  "myCB"
//       OpName %uniformVar "myCB"
//       OpDecorate %blockType Block
//       OpTypePointer %ptr Uniform %blockType
//       OpVariable %ptr %uniformVar Uniform
//
//   The SPIR-V is valid, but the duplicated debug/reflection name is ambiguous. Reflection
//   tools such as spirv-cross may disambiguate one side of the collision by inventing a
//   suffix, commonly exposing the constant buffer as:
//
//       "myCB_t"
//
//   instead of the original HLSL source name:
//
//       "myCB"
//
// Why this patch exists:
//   Our renderer deliberately binds resources by stable source-level names. The intended
//   contract is that:
//
//       ConstantBuffer<T> myCB;
//
//   reflects as:
//
//       "myCB"
//
//   Numeric descriptor bindings are not the naming contract here, and relying on the
//   compiler/reflection tool's collision-renaming policy is brittle.
//
// What we do, in order:
//   1) Parse the supplied HLSL source:
//        - Strip // and /* */ comments.
//        - Find ConstantBuffer<...> declarations.
//        - Record the declared source-level resource names.
//
//   2) Scan the SPIR-V module:
//        - Collect OpName strings by id.
//        - Collect Block-decorated ids.
//        - Collect OpTypePointer result ids and pointee type ids.
//        - Collect OpVariable result ids, result type ids, and storage classes.
//
//   3) For each HLSL ConstantBuffer<T> resource name:
//        - Find a Uniform OpVariable with that exact source name.
//        - Follow its result type through OpTypePointer.
//        - Confirm the pointee type is Block-decorated.
//        - Confirm the pointee Block type is also named the same thing.
//
//   4) If both the Uniform variable and its Block type have the same name:
//        - Keep the Block type named with the original HLSL resource name.
//        - Rename the Uniform variable instance aside to:
//              "<name>_instance"
//
//      This removes the duplicate OpName collision while preserving the desired reflected
//      constant-buffer name on the Block type.
//
// Returns true if modifications were made; false if no applicable duplicate
// ConstantBuffer<T> naming issue was found, or if the SPIR-V was malformed/unsafe to patch.
//
// Notes:
//   - This changes only OpName debug/reflection metadata.
//   - It does not alter descriptor sets, bindings, layouts, decorations, storage classes,
//     access chains, or executable shader semantics.
//   - The patch is deliberately source-guided: only names corresponding to actual
//     ConstantBuffer<T> declarations in the supplied HLSL are considered.
//   - The patch is structure-guided: it only operates on Uniform variables pointing to
//     Block-decorated types with the same duplicate name.
//----------------------------------------------------------------------------------------
//##TODO## ChatGPT generated. Needs review.
bool ShaderCode::FixHlslConstantBufferNames(const std::string& hlsl, std::vector<uint32_t>& spv)
{
	if (spv.size() < 5 || spv[0] != 0x07230203u) return false;

	auto WC = [](uint32_t w) { return w >> 16; };
	auto OP = [](uint32_t w) { return w & 0xFFFFu; };

	auto okInst = [&](size_t i) -> bool {
		if (i >= spv.size()) return false;
		uint32_t W = WC(spv[i]);
		return W && (i + W) <= spv.size();
	};

	auto isIdentStart = [](char c) -> bool {
		return c == '_' || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
	};

	auto isIdentChar = [&](char c) -> bool {
		return isIdentStart(c) || (c >= '0' && c <= '9');
	};

	auto skipWs = [&](const std::string& s, size_t& p) {
		while (p < s.size() && (s[p] == ' ' || s[p] == '\t' || s[p] == '\r' || s[p] == '\n'))
			++p;
	};

	auto decodeString = [&](const uint32_t* words, uint32_t wordCount) -> std::string {
		std::string out;

		for (uint32_t i = 0; i < wordCount; ++i)
		{
			uint32_t w = words[i];

			for (int b = 0; b < 4; ++b)
			{
				char c = char((w >> (8 * b)) & 0xFFu);
				if (!c) return out;
				out.push_back(c);
			}
		}

		return out;
	};

	auto encodeString = [&](const std::string& s) -> std::vector<uint32_t> {
		std::vector<uint32_t> words;
		uint32_t word = 0;
		uint32_t byte = 0;

		auto push = [&](uint8_t b) {
			word |= uint32_t(b) << (byte * 8);

			if (++byte == 4)
			{
				words.push_back(word);
				word = 0;
				byte = 0;
			}
		};

		for (char c : s) push(uint8_t(c));

		// SPIR-V strings are null-terminated and padded to 32-bit word alignment.
		push(0);

		if (byte) words.push_back(word);
		return words;
	};

	// Strip comments so disabled or explanatory ConstantBuffer<T> text does not produce
	// false positives.
	std::string src;
	src.reserve(hlsl.size());

	for (size_t i = 0; i < hlsl.size();)
	{
		if (i + 1 < hlsl.size() && hlsl[i] == '/' && hlsl[i + 1] == '/')
		{
			i += 2;
			while (i < hlsl.size() && hlsl[i] != '\n' && hlsl[i] != '\r') ++i;
		}
		else if (i + 1 < hlsl.size() && hlsl[i] == '/' && hlsl[i + 1] == '*')
		{
			i += 2;
			while (i + 1 < hlsl.size() && !(hlsl[i] == '*' && hlsl[i + 1] == '/')) ++i;
			if (i + 1 < hlsl.size()) i += 2;
		}
		else
		{
			src.push_back(hlsl[i++]);
		}
	}

	// Extract source-level ConstantBuffer<T> variable names.
	std::unordered_set<std::string> cbNames;

	for (size_t p = 0;;)
	{
		p = src.find("ConstantBuffer", p);
		if (p == std::string::npos) break;

		const size_t keywordLen = sizeof("ConstantBuffer") - 1;

		// Require token boundaries.
		if (p > 0 && isIdentChar(src[p - 1]))
		{
			++p;
			continue;
		}
		if (p + keywordLen < src.size() && isIdentChar(src[p + keywordLen]))
		{
			++p;
			continue;
		}

		size_t q = p + keywordLen;
		skipWs(src, q);

		if (q >= src.size() || src[q] != '<')
		{
			++p;
			continue;
		}

		// Skip the template argument.
		int depth = 0;
		while (q < src.size())
		{
			if (src[q] == '<')
				++depth;
			else if (src[q] == '>' && --depth == 0)
			{
				++q;
				break;
			}

			++q;
		}

		if (depth != 0) break;

		skipWs(src, q);

		if (q < src.size() && isIdentStart(src[q]))
		{
			size_t b = q++;
			while (q < src.size() && isIdentChar(src[q])) ++q;
			cbNames.insert(src.substr(b, q - b));
		}

		p = q;
	}

	if (cbNames.empty()) return false;

	constexpr uint32_t OpName = 5u;
	constexpr uint32_t OpDecorate = 71u;
	constexpr uint32_t OpTypePointer = 32u;
	constexpr uint32_t OpVariable = 59u;

	constexpr uint32_t DecorationBlock = 2u;
	constexpr uint32_t StorageUniform = 2u;

	struct PtrInfo
	{
		uint32_t storage = 0;
		uint32_t pointeeType = 0;
	};

	struct VarInfo
	{
		uint32_t resultType = 0;
		uint32_t storage = 0;
	};

	std::unordered_map<uint32_t, std::string> opNames;
	std::unordered_set<uint32_t> blockTypeIds;
	std::unordered_map<uint32_t, PtrInfo> ptrTypes;
	std::unordered_map<uint32_t, VarInfo> variables;

	// Gather names and the minimal type graph needed to connect:
	//   Uniform variable -> pointer type -> Block-decorated struct/type.
	for (size_t i = 5; i < spv.size();)
	{
		if (!okInst(i)) return false;

		uint32_t W = WC(spv[i]);
		uint32_t O = OP(spv[i]);

		if (O == OpName && W >= 3)
		{
			uint32_t id = spv[i + 1];
			opNames[id] = decodeString(&spv[i + 2], W - 2);
		}
		else if (O == OpDecorate && W >= 3)
		{
			if (spv[i + 2] == DecorationBlock)
				blockTypeIds.insert(spv[i + 1]);
		}
		else if (O == OpTypePointer && W >= 4)
		{
			uint32_t resultId = spv[i + 1];
			ptrTypes[resultId] = {spv[i + 2], spv[i + 3]};
		}
		else if (O == OpVariable && W >= 4)
		{
			uint32_t resultType = spv[i + 1];
			uint32_t resultId = spv[i + 2];
			uint32_t storage = spv[i + 3];

			variables[resultId] = {resultType, storage};
		}

		i += W;
	}

	std::unordered_map<uint32_t, std::string> renameById;

	// Detect only the confirmed failure mode:
	//   OpName %blockType  "myCB"
	//   OpName %uniformVar "myCB"
	//
	// The block type is the name we want reflected, so only the Uniform variable instance
	// is renamed aside.
	for (const std::string& cbName : cbNames)
	{
		for (const auto& it : variables)
		{
			uint32_t varId = it.first;
			const VarInfo& var = it.second;

			if (var.storage != StorageUniform)
				continue;

			auto varNameIt = opNames.find(varId);
			if (varNameIt == opNames.end() || varNameIt->second != cbName)
				continue;

			auto ptrIt = ptrTypes.find(var.resultType);
			if (ptrIt == ptrTypes.end())
				continue;

			uint32_t blockTypeId = ptrIt->second.pointeeType;
			if (!blockTypeIds.count(blockTypeId))
				continue;

			auto blockNameIt = opNames.find(blockTypeId);
			if (blockNameIt == opNames.end() || blockNameIt->second != cbName)
				continue;

			renameById[varId] = cbName + "_instance";
		}
	}

	if (renameById.empty()) return false;

	// Rebuild the module so variable-length OpName edits cannot invalidate later offsets.
	std::vector<uint32_t> out;
	out.reserve(spv.size());

	out.insert(out.end(), spv.begin(), spv.begin() + 5);

	for (size_t i = 5; i < spv.size();)
	{
		if (!okInst(i)) return false;

		uint32_t W = WC(spv[i]);
		uint32_t O = OP(spv[i]);

		if (O == OpName && W >= 3)
		{
			uint32_t targetId = spv[i + 1];
			auto r = renameById.find(targetId);

			if (r != renameById.end())
			{
				std::vector<uint32_t> encoded = encodeString(r->second);
				uint32_t newW = uint32_t(2 + encoded.size());

				out.push_back((newW << 16) | OpName);
				out.push_back(targetId);
				out.insert(out.end(), encoded.begin(), encoded.end());

				i += W;
				continue;
			}
		}

		out.insert(out.end(), spv.begin() + i, spv.begin() + i + W);
		i += W;
	}

	spv.swap(out);
	return true;
}

//----------------------------------------------------------------------------------------
// Fixes bad GLSL to SPIR-V generated by glslang for Vulkan. Without this fix, we get errors like this:
//   spvtools::Optimizer - Line 0:0 [VUID-StandaloneSpirv-OpTypeRuntimeArray-04680] For Vulkan, OpTypeStruct variables
//   containing OpTypeRuntimeArray must have storage class of StorageBuffer, PhysicalStorageBuffer, or Uniform.
//     %_ = OpVariable %_ptr_Output_gl_PerVertex Output
// This is caused by glslang inserting a ClipDistance[] array output, even when not originally present, and declaring
// it as a dynamically sized "OpTypeRuntimeArray", when ChatGPT says:
//   "In Vulkan, interface variables (Input/Output) must have a statically-sized layout. A RuntimeArray is only legal
//    inside structs used as StorageBuffer / PhysicalStorageBuffer / Uniform"
// This function will locate the ClipDistance array specifically, and convert it to a statically sized array. To
// determine the array size, it will check if it's currently used in the shader with only static index values. If it
// is, it will size the array to the maximum size used. If it's used with a dynamic index, it will size it to 8, which
// is the typical hardware clip limit. If it's unused, it will size it to 1, since that's much easier than removing it.
//##TODO## ChatGPT generated. Needs review.
bool ShaderCode::FixVulkanGlPerVertexClipDistance(std::vector<uint32_t>& spv)
{
	if (spv.size() < 5 || spv[0] != 0x07230203u) return false;

	auto WC = [](uint32_t w) { return w >> 16; };
	auto OP = [](uint32_t w) { return w & 0xFFFFu; };
	auto ok = [&](size_t i) {
		if (i >= spv.size()) return false;
		uint32_t w = WC(spv[i]);
		return w && (i + w) <= spv.size();
	};
	auto readStr = [&](size_t firstWord, size_t endWord) {
		std::string s;
		for (size_t i = firstWord; i < endWord; ++i)
		{
			uint32_t w = spv[i];
			for (int b = 0; b < 4; ++b)
			{
				char c = char((w >> (8 * b)) & 0xFF);
				if (!c) return s;
				s.push_back(c);
			}
		}
		return s;
	};
	auto emit = [&](std::vector<uint32_t>& out, uint16_t opcode, std::initializer_list<uint32_t> ops) {
		out.push_back((uint32_t(1 + ops.size()) << 16) | opcode);
		out.insert(out.end(), ops.begin(), ops.end());
	};

	// Opcodes (SPIR-V core)
	constexpr uint32_t OpName = 5u;
	constexpr uint32_t OpMemberName = 6u;
	constexpr uint32_t OpTypeFloat = 22u;
	constexpr uint32_t OpTypeRuntimeArray = 29u;
	constexpr uint32_t OpTypeArray = 28u;
	constexpr uint32_t OpTypeStruct = 30u;
	constexpr uint32_t OpTypePointer = 32u;
	constexpr uint32_t OpTypeInt = 21u;
	constexpr uint32_t OpConstant = 43u;
	constexpr uint32_t OpVariable = 59u;
	constexpr uint32_t OpDecorate = 71u;
	constexpr uint32_t OpAccessChain = 65u;
	constexpr uint32_t OpInBoundsAccessChain = 66u;
	constexpr uint32_t OpFunction = 54u;

	// Decoration enums
	constexpr uint32_t DecorationArrayStride = 6u;

	// Storage classes
	constexpr uint32_t StorageInput = 1u;
	constexpr uint32_t StorageOutput = 3u;

	auto isTypeConstGlobal = [&](uint32_t op) -> bool {
		return (op >= 19u && op <= 39u) || (op >= 41u && op <= 48u) || (op == OpVariable);
	};

	// Find boundaries
	size_t firstFunc = spv.size(), typesStart = spv.size();
	for (size_t i = 5; i < spv.size();)
	{
		if (!ok(i)) return false;
		uint32_t op = OP(spv[i]), w = WC(spv[i]);
		if (typesStart == spv.size() && isTypeConstGlobal(op)) typesStart = i;
		if (op == OpFunction)
		{
			firstFunc = i;
			break;
		}
		i += w;
	}
	if (typesStart == spv.size()) typesStart = 5;

	// Names
	std::unordered_map<uint32_t, std::string> nameOf;
	std::unordered_map<uint64_t, std::string> memberNameOf;
	for (size_t i = 5; i < firstFunc;)
	{
		if (!ok(i)) return false;
		uint32_t w = WC(spv[i]), op = OP(spv[i]);
		if (op == OpName && w >= 3)
		{
			nameOf[spv[i + 1]] = readStr(i + 2, i + w);
		}
		else if (op == OpMemberName && w >= 4)
		{
			memberNameOf[(uint64_t(spv[i + 1]) << 32) | spv[i + 2]] = readStr(i + 3, i + w);
		}
		i += w;
	}

	// Find gl_PerVertex struct id
	uint32_t glPerVertexStruct = 0;
	for (auto& kv : nameOf)
		if (kv.second == "gl_PerVertex")
		{
			glPerVertexStruct = kv.first;
			break;
		}
	if (!glPerVertexStruct) return false;

	// Parse type info + locate struct definition position
	std::unordered_map<uint32_t, std::vector<uint32_t>> structMems;
	std::unordered_map<uint32_t, uint32_t> runtimeElem;
	std::unordered_map<uint32_t, std::pair<uint32_t, uint32_t>> ptrInfo;
	std::unordered_set<uint32_t> float32Types;
	size_t glPerVertexStructPos = spv.size();

	for (size_t i = 5; i < firstFunc;)
	{
		if (!ok(i)) return false;
		uint32_t w = WC(spv[i]), op = OP(spv[i]);

		if (op == OpTypeFloat && w == 3 && spv[i + 2] == 32u)
		{
			float32Types.insert(spv[i + 1]);
		}
		else if (op == OpTypeRuntimeArray && w == 3)
		{
			runtimeElem[spv[i + 1]] = spv[i + 2];
		}
		else if (op == OpTypePointer && w == 4)
		{
			ptrInfo[spv[i + 1]] = {spv[i + 2], spv[i + 3]};
		}
		else if (op == OpTypeStruct && w >= 2)
		{
			uint32_t sid = spv[i + 1];
			std::vector<uint32_t> mem;
			mem.reserve(w - 2);
			for (uint32_t k = 2; k < w; ++k) mem.push_back(spv[i + k]);
			structMems[sid] = std::move(mem);
			if (sid == glPerVertexStruct) glPerVertexStructPos = i;
		}

		i += w;
	}
	if (float32Types.empty() || glPerVertexStructPos == spv.size()) return false;

	// Find member index named gl_ClipDistance
	auto smIt = structMems.find(glPerVertexStruct);
	if (smIt == structMems.end()) return false;

	uint32_t clipMemberIndex = ~0u;
	for (uint32_t mi = 0; mi < smIt->second.size(); ++mi)
	{
		auto it = memberNameOf.find((uint64_t(glPerVertexStruct) << 32) | mi);
		if (it != memberNameOf.end() && it->second == "gl_ClipDistance")
		{
			clipMemberIndex = mi;
			break;
		}
	}
	if (clipMemberIndex == ~0u) return false;

	// Verify it's runtime array of float
	uint32_t clipMemberType = smIt->second[clipMemberIndex];
	auto re = runtimeElem.find(clipMemberType);
	if (re == runtimeElem.end()) return false; // already fixed or not runtime array
	uint32_t clipElemType = re->second;
	if (!float32Types.count(clipElemType)) return false;

	// Find per-vertex interface variables (Input/Output pointers to gl_PerVertex)
	std::unordered_set<uint32_t> perVertexVars;
	for (size_t i = 5; i < firstFunc;)
	{
		if (!ok(i)) return false;
		uint32_t w = WC(spv[i]), op = OP(spv[i]);
		if (op == OpVariable && w >= 4)
		{
			uint32_t resultType = spv[i + 1];
			uint32_t varId = spv[i + 2];
			uint32_t storage = spv[i + 3];
			if (storage == StorageInput || storage == StorageOutput)
			{
				auto p = ptrInfo.find(resultType);
				if (p != ptrInfo.end() && p->second.second == glPerVertexStruct) perVertexVars.insert(varId);
			}
		}
		i += w;
	}
	if (perVertexVars.empty()) return false;

	// Infer needed N (max constant index + 1; dynamic => 8; unused => 1)
	std::unordered_map<uint32_t, uint32_t> constAnyI32; // constId->literal (for any int32 consts)
	// Gather ALL 32-bit int constants (signed or unsigned) just so we can detect const indices
	// by looking at literal operand (works because your indices are small and encoded as single word).
	for (size_t i = 5; i < firstFunc;)
	{
		if (!ok(i)) return false;
		uint32_t w = WC(spv[i]), op = OP(spv[i]);
		if (op == OpConstant && w >= 4)
		{
			constAnyI32[spv[i + 2]] = spv[i + 3];
		}
		i += w;
	}
	auto getConstLit = [&](uint32_t id, uint32_t& out) -> bool {
		auto it = constAnyI32.find(id);
		if (it == constAnyI32.end()) return false;
		out = it->second;
		return true;
	};

	uint32_t maxConstIdx = 0;
	bool sawAnyUse = false, sawConst = false, sawDynamic = false;

	for (size_t i = 5; i < spv.size();)
	{
		if (!ok(i)) return false;
		uint32_t w = WC(spv[i]), op = OP(spv[i]);

		if ((op == OpAccessChain || op == OpInBoundsAccessChain) && w >= 6)
		{
			uint32_t base = spv[i + 3];
			if (!perVertexVars.count(base))
			{
				i += w;
				continue;
			}

			uint32_t m = 0;
			if (!getConstLit(spv[i + 4], m) || m != clipMemberIndex)
			{
				i += w;
				continue;
			}

			sawAnyUse = true;
			uint32_t e = 0;
			if (getConstLit(spv[i + 5], e))
			{
				sawConst = true;
				maxConstIdx = std::max(maxConstIdx, e);
			}
			else
				sawDynamic = true;
		}

		i += w;
	}

	uint32_t N = 1;
	if (sawConst)
		N = maxConstIdx + 1;
	else if (sawDynamic)
		N = 8;
	else if (!sawAnyUse)
		N = 1;

	// Allocate new IDs (ALWAYS create our own uint + constant to guarantee def-before-use)
	uint32_t& bound = spv[3];
	auto newId = [&]() { return bound++; };

	uint32_t u32Type = newId();
	uint32_t lenConst = newId();
	uint32_t arrType = newId();

	std::vector<uint32_t> annInject;
	std::vector<uint32_t> typeInject;

	emit(annInject, OpDecorate, {arrType, DecorationArrayStride, 4u});
	emit(typeInject, OpTypeInt, {u32Type, 32u, 0u});      // uint
	emit(typeInject, OpConstant, {u32Type, lenConst, N}); // %len = N
	emit(typeInject, OpTypeArray, {arrType, clipElemType, lenConst});

	// Patch gl_PerVertex member type operand in-place
	{
		size_t i = glPerVertexStructPos;
		if (!ok(i) || OP(spv[i]) != OpTypeStruct) return false;
		uint32_t w = WC(spv[i]);
		size_t memberWord = i + 2 + clipMemberIndex;
		if (memberWord >= i + w) return false;
		if (spv[memberWord] != clipMemberType) return false;
		spv[memberWord] = arrType;
	}

	// Insert decoration into annotations section (before first type/const/global)
	spv.insert(spv.begin() + typesStart, annInject.begin(), annInject.end());
	glPerVertexStructPos += annInject.size(); // struct shifted forward

	// Insert new defs immediately before the struct definition (so struct can reference arrType)
	spv.insert(spv.begin() + glPerVertexStructPos, typeInject.begin(), typeInject.end());

	return true;
}

//----------------------------------------------------------------------------------------
// Fixes bad GLSL to SPIR-V generated by glslang for Vulkan. Without this fix, we get errors like this:
//   "spvtools::Optimizer - Line 0:0 Vulkan spec doesn't allow BuiltIn VertexId to be used."
// Analysis by ChatGPT:
//   Your module declares an input variable with BuiltIn VertexId (SPIR-V enum value 5):
//     OpVariable %_ptr_Input_int Input %gl_VertexID
//     decorated with OpDecorate %gl_VertexID BuiltIn VertexId
//   In Vulkan, the allowed vertex index builtin in the vertex stage is VertexIndex, not VertexId. (VertexId is the
//   OpenGL-style builtin; Vulkan uses the VertexIndex/InstanceIndex naming and semantics.)
//   Below is a single, self-contained function that fixes:
//     VertexId -> VertexIndex
//     InstanceId -> InstanceIndex
//   only when the variable is an OpVariable in Input storage class.
//##TODO## ChatGPT generated. Needs review.
bool ShaderCode::FixVulkanBuiltinIds(std::vector<uint32_t>& spv)
{
	if (spv.size() < 5 || spv[0] != 0x07230203u) return false;

	auto WC = [](uint32_t w) { return w >> 16; };
	auto OP = [](uint32_t w) { return w & 0xFFFFu; };
	auto okInst = [&](size_t i) -> bool {
		if (i >= spv.size()) return false;
		uint32_t W = WC(spv[i]);
		return W && (i + W) <= spv.size();
	};

	// SPIR-V BuiltIn enum values we care about:
	//   VertexId     = 5
	//   InstanceId   = 6
	//   VertexIndex  = 42
	//   InstanceIndex= 43
	// (These are stable across SPIR-V versions.)
	constexpr uint32_t BuiltIn = 11u; // Decoration::BuiltIn
	constexpr uint32_t VertexId = 5u;
	constexpr uint32_t InstanceId = 6u;
	constexpr uint32_t VertexIndex = 42u;
	constexpr uint32_t InstanceIndex = 43u;

	// Find all IDs that are OpVariable ... Input
	std::unordered_set<uint32_t> inputVars;
	for (size_t i = 5; i < spv.size();)
	{
		if (!okInst(i)) return false;
		uint32_t w0 = spv[i], W = WC(w0), O = OP(w0);
		if (O == 59u && W >= 4)
		{ // OpVariable
			uint32_t resultId = spv[i + 2];
			uint32_t storage = spv[i + 3];
			if (storage == 1u /*Input*/) inputVars.insert(resultId);
		}
		i += W;
	}
	if (inputVars.empty()) return false;

	// Patch OpDecorate %id BuiltIn VertexId/InstanceId  -> VertexIndex/InstanceIndex
	bool modified = false;
	for (size_t i = 5; i < spv.size();)
	{
		if (!okInst(i)) return false;
		uint32_t w0 = spv[i], W = WC(w0), O = OP(w0);

		if (O == 71u && W >= 4)
		{ // OpDecorate
			uint32_t targetId = spv[i + 1];
			uint32_t deco = spv[i + 2];
			if (deco == BuiltIn && inputVars.count(targetId))
			{
				uint32_t bi = spv[i + 3];
				if (bi == VertexId)
				{
					spv[i + 3] = VertexIndex;
					modified = true;
				}
				if (bi == InstanceId)
				{
					spv[i + 3] = InstanceIndex;
					modified = true;
				}
			}
		}

		i += W;
	}

	return modified;
}

//----------------------------------------------------------------------------------------
// This function converts resource pairs following this naming convention:
//
//     <imageName>                  : separate image
//     <imageName>_CombinedSampler  : separate sampler
//
// into a single combined image/sampler resource:
//
//     <imageName>                  : sampled image
//
// The combined resource is represented by the original sampler variable.
// That variable is retargeted from pointer-to-sampler to
// pointer-to-sampled-image, and its OpName is rewritten from
// "<imageName>_CombinedSampler" to "<imageName>".
//
// The function is intentionally conservative and only transforms a sampler when:
// 1. Every OpSampledImage use of that sampler matches the naming convention.
// 2. Every such use agrees on the same OpTypeSampledImage.
// 3. Every OpLoad from the sampler variable is part of a rewritten chain.
// 4. Every retyped load/copy value is consumed only by a rewritten copy chain
//    or by an OpSampledImage that this function rewrites.
// 5. The existing OpTypeSampledImage is reused, never duplicated.
//
// High-level sequence:
//
// 1. Parse relevant parts of the module.
// 2. Discover all OpSampledImage instructions.
// 3. Resolve image/sampler operands back to UniformConstant variables.
// 4. Identify foldable sampler/image pairs by naming convention.
// 5. Reject samplers with mixed, unmatched, or unsafe usage.
// 6. Collect exact backward alias chains for sampler values.
// 7. Verify all uses of rewritten values by scanning all instruction operands.
// 8. Move/reuse sampled-image types and create pointer types if needed.
// 9. Retype the sampler variable to pointer-to-sampled-image.
// 10. Retype all loads/copies in the alias chains.
// 11. Replace OpSampledImage with OpCopyObject.
// 12. Rename "<imageName>_CombinedSampler" to "<imageName>".
//
// The original separate image variable is left untouched. If it becomes unused,
// a later dead-code elimination pass can remove it.
//##TODO## ChatGPT generated. Needs review.
bool ShaderCode::FoldNamedStandaloneSamplersIntoImages(std::vector<uint32_t>& spv)
{
	if (spv.size() < 5 || spv[0] != 0x07230203u)
		return false;

	auto WC = [](uint32_t w) { return w >> 16; };
	auto OP = [](uint32_t w) { return w & 0xffffu; };
	auto MakeOp = [](uint32_t wc, uint32_t op) { return (wc << 16) | op; };

	constexpr uint32_t OpName = 5;
	constexpr uint32_t OpTypeImage = 25;
	constexpr uint32_t OpTypeSampler = 26;
	constexpr uint32_t OpTypeSampledImage = 27;
	constexpr uint32_t OpTypePointer = 32;
	constexpr uint32_t OpVariable = 59;
	constexpr uint32_t OpLoad = 61;
	constexpr uint32_t OpCopyObject = 83;
	constexpr uint32_t OpSampledImage = 86;
	constexpr uint32_t OpFunction = 54;
	constexpr uint32_t OpEntryPoint = 15;
	constexpr uint32_t OpDecorate = 71;

	constexpr uint32_t StorageUniformConstant = 0;

	auto validInst = [&](size_t i) -> bool {
		if (i >= spv.size())
			return false;

		uint32_t wc = WC(spv[i]);
		return wc != 0 && i + wc <= spv.size();
	};

	auto readString = [&](size_t first, size_t last) -> std::string {
		std::string s;

		for (size_t i = first; i < last; ++i)
		{
			uint32_t w = spv[i];

			for (int b = 0; b < 4; ++b)
			{
				char c = char((w >> (8 * b)) & 0xffu);
				if (!c)
					return s;

				s.push_back(c);
			}
		}

		return s;
	};

	auto encodeString = [&](const std::string& s) -> std::vector<uint32_t> {
		std::vector<uint32_t> words((s.size() + 4) / 4, 0);

		for (size_t i = 0; i < s.size(); ++i)
			words[i / 4] |= uint32_t(uint8_t(s[i])) << (8 * (i % 4));

		return words;
	};

	struct PtrType
	{
		uint32_t storage = 0;
		uint32_t pointee = 0;
	};

	struct Var
	{
		size_t inst = 0;
		uint32_t ptrType = 0;
		uint32_t pointee = 0;
	};

	struct Load
	{
		size_t inst = 0;
		uint32_t type = 0;
		uint32_t ptr = 0;
	};

	struct Copy
	{
		size_t inst = 0;
		uint32_t type = 0;
		uint32_t operand = 0;
	};

	struct SampleUse
	{
		size_t inst = 0;
		uint32_t sampledType = 0;
		uint32_t resultId = 0;
		uint32_t imageValue = 0;
		uint32_t samplerValue = 0;
		uint32_t imageVar = 0;
		uint32_t samplerVar = 0;
		bool foldable = false;
	};

	std::unordered_map<uint32_t, std::string> names;
	std::unordered_set<uint32_t> imageTypes;
	std::unordered_set<uint32_t> samplerTypes;
	std::unordered_map<uint32_t, uint32_t> sampledImageTypeToImageType;
	std::unordered_map<uint32_t, PtrType> ptrTypes;
	std::unordered_map<uint32_t, Var> vars;
	std::unordered_map<uint32_t, Load> loads;
	std::unordered_map<uint32_t, Copy> copies;
	std::unordered_map<uint32_t, std::vector<uint32_t>> loadsByPtr;
	std::vector<SampleUse> sampleUses;

	// Parse names, types, variables, loads, and copy aliases.
	for (size_t i = 5; i < spv.size();)
	{
		if (!validInst(i))
			return false;

		uint32_t wc = WC(spv[i]);
		uint32_t op = OP(spv[i]);

		if (op == OpName && wc >= 3)
			names[spv[i + 1]] = readString(i + 2, i + wc);
		else if (op == OpTypeImage && wc >= 2)
			imageTypes.insert(spv[i + 1]);
		else if (op == OpTypeSampler && wc >= 2)
			samplerTypes.insert(spv[i + 1]);
		else if (op == OpTypeSampledImage && wc >= 3)
			sampledImageTypeToImageType[spv[i + 1]] = spv[i + 2];
		else if (op == OpTypePointer && wc >= 4)
			ptrTypes[spv[i + 1]] = {spv[i + 2], spv[i + 3]};
		else if (op == OpVariable && wc >= 4)
		{
			auto pt = ptrTypes.find(spv[i + 1]);

			if (pt != ptrTypes.end() && spv[i + 3] == StorageUniformConstant)
				vars[spv[i + 2]] = {i, spv[i + 1], pt->second.pointee};
		}
		else if (op == OpLoad && wc >= 4)
		{
			loads[spv[i + 2]] = {i, spv[i + 1], spv[i + 3]};
			loadsByPtr[spv[i + 3]].push_back(spv[i + 2]);
		}
		else if (op == OpCopyObject && wc >= 4)
		{
			copies[spv[i + 2]] = {i, spv[i + 1], spv[i + 3]};
		}

		i += wc;
	}

	// Resolve a value backward through OpCopyObject aliases to its loaded
	// UniformConstant variable.
	auto resolveLoadedVar = [&](uint32_t value,
	                            const std::unordered_set<uint32_t>& allowedPointeeTypes) -> uint32_t {
		std::unordered_set<uint32_t> seen;

		for (;;)
		{
			if (!seen.insert(value).second)
				return 0;

			auto l = loads.find(value);
			if (l != loads.end())
			{
				auto v = vars.find(l->second.ptr);
				if (v == vars.end())
					return 0;

				if (!allowedPointeeTypes.count(v->second.pointee))
					return 0;

				return l->second.ptr;
			}

			auto c = copies.find(value);
			if (c == copies.end())
				return 0;

			value = c->second.operand;
		}
	};

	// Collect the full backward alias chain for a sampler operand, including the
	// final value, intermediate OpCopyObject results, and the root OpLoad result.
	auto collectBackwardAliasChain = [&](uint32_t value, std::vector<uint32_t>& chain) -> bool {
		std::unordered_set<uint32_t> seen;

		for (;;)
		{
			if (!seen.insert(value).second)
				return false;

			chain.push_back(value);

			if (loads.count(value))
				return true;

			auto c = copies.find(value);
			if (c == copies.end())
				return false;

			value = c->second.operand;
		}
	};

	// Collect every OpSampledImage, including non-foldable uses, so mixed sampler
	// usage can be rejected instead of partially rewritten.
	for (size_t i = 5; i < spv.size();)
	{
		if (!validInst(i))
			return false;

		uint32_t wc = WC(spv[i]);
		uint32_t op = OP(spv[i]);

		if (op == OpSampledImage && wc == 5)
		{
			SampleUse su;
			su.inst = i;
			su.sampledType = spv[i + 1];
			su.resultId = spv[i + 2];
			su.imageValue = spv[i + 3];
			su.samplerValue = spv[i + 4];

			auto sampledTypeIt = sampledImageTypeToImageType.find(su.sampledType);
			if (sampledTypeIt != sampledImageTypeToImageType.end())
			{
				su.imageVar = resolveLoadedVar(su.imageValue, imageTypes);
				su.samplerVar = resolveLoadedVar(su.samplerValue, samplerTypes);

				if (su.imageVar && su.samplerVar)
				{
					auto imageVarIt = vars.find(su.imageVar);
					auto imageName = names.find(su.imageVar);
					auto samplerName = names.find(su.samplerVar);

					su.foldable =
					  imageVarIt != vars.end() &&
					  imageVarIt->second.pointee == sampledTypeIt->second &&
					  imageName != names.end() &&
					  samplerName != names.end() &&
					  samplerName->second == imageName->second + CombinedImageSamplerPostfix;
				}
			}

			sampleUses.push_back(su);
		}

		i += wc;
	}

	if (sampleUses.empty())
		return false;

	std::unordered_map<uint32_t, std::vector<size_t>> allUsesBySamplerVar;

	for (size_t i = 0; i < sampleUses.size(); ++i)
	{
		if (sampleUses[i].samplerVar)
			allUsesBySamplerVar[sampleUses[i].samplerVar].push_back(i);
	}

	std::unordered_map<uint32_t, std::vector<size_t>> foldUsesBySamplerVar;

	// A sampler is foldable only if every OpSampledImage use of that sampler is
	// foldable and all uses agree on exactly one sampled-image type.
	for (const auto& group : allUsesBySamplerVar)
	{
		uint32_t samplerVar = group.first;
		const std::vector<size_t>& uses = group.second;

		bool reject = false;
		uint32_t sampledType = 0;

		for (size_t useIndex : uses)
		{
			const SampleUse& su = sampleUses[useIndex];

			if (!su.foldable)
			{
				reject = true;
				break;
			}

			if (!sampledType)
				sampledType = su.sampledType;
			else if (sampledType != su.sampledType)
			{
				reject = true;
				break;
			}
		}

		if (!reject && sampledType)
			foldUsesBySamplerVar[samplerVar] = uses;
	}

	if (foldUsesBySamplerVar.empty())
		return false;

	std::unordered_map<uint32_t, uint32_t> valueToSampledType;

	// Retype the exact backward chain, not just the final sampler operand.
	for (const auto& group : foldUsesBySamplerVar)
	{
		uint32_t sampledType = sampleUses[group.second.front()].sampledType;

		for (size_t useIndex : group.second)
		{
			std::vector<uint32_t> chain;

			if (!collectBackwardAliasChain(sampleUses[useIndex].samplerValue, chain))
				return false;

			for (uint32_t value : chain)
			{
				auto existing = valueToSampledType.find(value);
				if (existing != valueToSampledType.end() && existing->second != sampledType)
					return false;

				valueToSampledType[value] = sampledType;
			}
		}
	}

	// Once a sampler variable is retargeted, every load from it must be retargeted.
	for (const auto& group : foldUsesBySamplerVar)
	{
		uint32_t samplerVar = group.first;

		auto lbp = loadsByPtr.find(samplerVar);
		if (lbp == loadsByPtr.end())
			return false;

		for (uint32_t loadResult : lbp->second)
		{
			if (!valueToSampledType.count(loadResult))
				return false;
		}
	}

	std::unordered_set<size_t> rewrittenSampleInsts;
	std::unordered_set<uint32_t> rewrittenValues;

	for (const auto& group : foldUsesBySamplerVar)
	{
		for (size_t useIndex : group.second)
			rewrittenSampleInsts.insert(sampleUses[useIndex].inst);
	}

	for (const auto& p : valueToSampledType)
		rewrittenValues.insert(p.first);

	struct UseSite
	{
		size_t inst = 0;
		uint32_t op = 0;
		uint32_t word = 0;
	};

	std::unordered_map<uint32_t, std::vector<UseSite>> usesById;

	// Full conservative operand scan. Skip only the defining result-id words of
	// the rewritten OpLoad / OpCopyObject values themselves.
	for (size_t i = 5; i < spv.size();)
	{
		if (!validInst(i))
			return false;

		uint32_t wc = WC(spv[i]);
		uint32_t op = OP(spv[i]);

		for (uint32_t word = 1; word < wc; ++word)
		{
			if ((op == OpLoad || op == OpCopyObject) && word == 2)
				continue;

			uint32_t id = spv[i + word];

			if (rewrittenValues.count(id))
				usesById[id].push_back({i, op, word});
		}

		i += wc;
	}

	// Rewritten values may only feed rewritten copy aliases or the sampler
	// operand of an OpSampledImage that this pass will replace.
	for (uint32_t value : rewrittenValues)
	{
		auto useIt = usesById.find(value);
		if (useIt == usesById.end())
			continue;

		for (const UseSite& use : useIt->second)
		{
			if (use.op == OpCopyObject && use.word == 3)
			{
				uint32_t copyResult = spv[use.inst + 2];

				if (!rewrittenValues.count(copyResult))
					return false;

				continue;
			}

			if (use.op == OpSampledImage && use.word == 4)
			{
				if (!rewrittenSampleInsts.count(use.inst))
					return false;

				continue;
			}

			return false;
		}
	}

	auto reparseMutableInstructionOffsets = [&]() -> bool {
		vars.clear();
		loads.clear();
		copies.clear();
		loadsByPtr.clear();

		for (size_t i = 5; i < spv.size();)
		{
			if (!validInst(i))
				return false;

			uint32_t wc = WC(spv[i]);
			uint32_t op = OP(spv[i]);

			if (op == OpVariable && wc >= 4)
			{
				auto pt = ptrTypes.find(spv[i + 1]);

				if (pt != ptrTypes.end() && spv[i + 3] == StorageUniformConstant)
					vars[spv[i + 2]] = {i, spv[i + 1], pt->second.pointee};
			}
			else if (op == OpLoad && wc >= 4)
			{
				loads[spv[i + 2]] = {i, spv[i + 1], spv[i + 3]};
				loadsByPtr[spv[i + 3]].push_back(spv[i + 2]);
			}
			else if (op == OpCopyObject && wc >= 4)
			{
				copies[spv[i + 2]] = {i, spv[i + 1], spv[i + 3]};
			}

			i += wc;
		}

		for (SampleUse& su : sampleUses)
		{
			for (size_t i = 5; i < spv.size();)
			{
				if (!validInst(i))
					return false;

				uint32_t wc = WC(spv[i]);
				uint32_t op = OP(spv[i]);

				if (op == OpSampledImage && wc == 5 && spv[i + 2] == su.resultId)
				{
					su.inst = i;
					break;
				}

				i += wc;
			}
		}

		return true;
	};

	auto findFirstVariableOrFunction = [&]() -> size_t {
		for (size_t i = 5; i < spv.size();)
		{
			if (!validInst(i))
				return spv.size();

			uint32_t wc = WC(spv[i]);
			uint32_t op = OP(spv[i]);

			if (op == OpVariable || op == OpFunction)
				return i;

			i += wc;
		}

		return spv.size();
	};

	auto findDefInst = [&](uint32_t id) -> size_t {
		for (size_t i = 5; i < spv.size();)
		{
			if (!validInst(i))
				return spv.size();

			uint32_t wc = WC(spv[i]);
			uint32_t op = OP(spv[i]);

			if ((op == OpTypeImage ||
			     op == OpTypeSampler ||
			     op == OpTypeSampledImage ||
			     op == OpTypePointer ||
			     op == OpVariable) &&
			    wc >= 2 &&
			    spv[i + 1] == id)
			{
				return i;
			}

			i += wc;
		}

		return spv.size();
	};

	// Move an existing declaration and reparse positions afterward. This avoids
	// fragile manual offset remapping.
	auto moveInstructionBefore = [&](size_t from, size_t to) -> bool {
		if (from == spv.size() || to == spv.size())
			return false;

		if (from == to)
			return true;

		uint32_t wc = WC(spv[from]);
		std::vector<uint32_t> inst(spv.begin() + from, spv.begin() + from + wc);

		spv.erase(spv.begin() + from, spv.begin() + from + wc);

		if (from < to)
			to -= wc;

		spv.insert(spv.begin() + to, inst.begin(), inst.end());

		return reparseMutableInstructionOffsets();
	};

	auto findPointerType = [&](uint32_t sampledType) -> uint32_t {
		for (const auto& p : ptrTypes)
		{
			if (p.second.storage == StorageUniformConstant &&
			    p.second.pointee == sampledType)
				return p.first;
		}

		return 0;
	};

	auto prepareSampledImagePointerType = [&](uint32_t sampledType, uint32_t samplerVar) -> uint32_t {
		auto samplerVarIt = vars.find(samplerVar);
		if (samplerVarIt == vars.end())
			return 0;

		size_t samplerVarInst = samplerVarIt->second.inst;

		auto sampledTypeImageIt = sampledImageTypeToImageType.find(sampledType);
		if (sampledTypeImageIt == sampledImageTypeToImageType.end())
			return 0;

		uint32_t imageType = sampledTypeImageIt->second;

		size_t imageTypeInst = findDefInst(imageType);
		size_t sampledTypeInst = findDefInst(sampledType);

		if (imageTypeInst == spv.size() || sampledTypeInst == spv.size())
			return 0;

		// The sampled-image type depends on the image type. If the image type is
		// after the sampler variable, moving the sampled-image type before the
		// sampler variable would still be invalid unless the image type is moved too.
		if (imageTypeInst > samplerVarInst)
		{
			if (!moveInstructionBefore(imageTypeInst, samplerVarInst))
				return 0;

			samplerVarInst = vars.find(samplerVar)->second.inst;
			sampledTypeInst = findDefInst(sampledType);
		}

		// Move OpTypeSampledImage only as far as needed: before the sampler variable,
		// but never before its OpTypeImage dependency.
		if (sampledTypeInst > samplerVarInst)
		{
			if (!moveInstructionBefore(sampledTypeInst, samplerVarInst))
				return 0;

			samplerVarInst = vars.find(samplerVar)->second.inst;
		}

		uint32_t existingPtr = findPointerType(sampledType);
		if (existingPtr)
		{
			size_t ptrInst = findDefInst(existingPtr);
			samplerVarInst = vars.find(samplerVar)->second.inst;

			if (ptrInst != spv.size() && ptrInst > samplerVarInst)
			{
				if (!moveInstructionBefore(ptrInst, samplerVarInst))
					return 0;
			}

			return existingPtr;
		}

		samplerVarInst = vars.find(samplerVar)->second.inst;

		uint32_t newPointerType = spv[3]++;

		std::vector<uint32_t> inst = {
		  MakeOp(4, OpTypePointer),
		  newPointerType,
		  StorageUniformConstant,
		  sampledType};

		spv.insert(spv.begin() + samplerVarInst, inst.begin(), inst.end());
		ptrTypes[newPointerType] = {StorageUniformConstant, sampledType};

		if (!reparseMutableInstructionOffsets())
			return 0;

		return newPointerType;
	};

	std::unordered_map<uint32_t, uint32_t> samplerVarToPointerType;

	for (const auto& group : foldUsesBySamplerVar)
	{
		uint32_t samplerVar = group.first;
		uint32_t sampledType = sampleUses[group.second.front()].sampledType;
		uint32_t pointerType = prepareSampledImagePointerType(sampledType, samplerVar);

		if (!pointerType)
			return false;

		samplerVarToPointerType[samplerVar] = pointerType;
	}

	std::unordered_set<uint32_t> rewrittenSamplerVars;

	for (const auto& p : samplerVarToPointerType)
		rewrittenSamplerVars.insert(p.first);

	// Validate direct uses of the sampler variable ID itself. Once the variable is
	// retargeted from pointer-to-sampler to pointer-to-sampled-image, any old
	// pointer-typed use can become invalid. Be conservative: allow only debug/name,
	// decorations, entry-point interface references, and OpLoad pointer operands
	// whose result is also being retyped.
	for (size_t i = 5; i < spv.size();)
	{
		if (!validInst(i))
			return false;

		uint32_t wc = WC(spv[i]);
		uint32_t op = OP(spv[i]);

		for (uint32_t word = 1; word < wc; ++word)
		{
			uint32_t id = spv[i + word];

			if (!rewrittenSamplerVars.count(id))
				continue;

			if (op == OpVariable && word == 2)
				continue;

			if (op == OpName)
				continue;

			if (op == OpDecorate && word == 1)
				continue;

			if (op == OpEntryPoint && word >= 4)
				continue;

			if (op == OpLoad && word == 3)
			{
				uint32_t loadResult = spv[i + 2];

				if (!valueToSampledType.count(loadResult))
					return false;

				continue;
			}

			return false;
		}

		i += wc;
	}

	// Retype the X_CombinedSampler variables to sampled-image variables.
	for (const auto& p : samplerVarToPointerType)
	{
		auto v = vars.find(p.first);
		if (v == vars.end())
			return false;

		spv[v->second.inst + 1] = p.second;
	}

	// Retype every OpLoad / OpCopyObject in the exact backward alias chains.
	for (const auto& valueType : valueToSampledType)
	{
		uint32_t value = valueType.first;
		uint32_t sampledType = valueType.second;

		auto l = loads.find(value);
		if (l != loads.end())
		{
			spv[l->second.inst + 1] = sampledType;
			continue;
		}

		auto c = copies.find(value);
		if (c != copies.end())
		{
			spv[c->second.inst + 1] = sampledType;
			continue;
		}
	}

	// Replace OpSampledImage with OpCopyObject. Process from highest offset to
	// lowest so erasing one word does not invalidate pending earlier offsets.
	std::vector<size_t> rewriteOrder;

	for (const auto& group : foldUsesBySamplerVar)
	{
		for (size_t useIndex : group.second)
			rewriteOrder.push_back(useIndex);
	}

	std::sort(rewriteOrder.begin(), rewriteOrder.end(), [&](size_t a, size_t b) {
		return sampleUses[a].inst > sampleUses[b].inst;
	});

	for (size_t useIndex : rewriteOrder)
	{
		const SampleUse& su = sampleUses[useIndex];

		spv[su.inst + 0] = MakeOp(4, OpCopyObject);
		spv[su.inst + 1] = su.sampledType;
		spv[su.inst + 2] = su.resultId;
		spv[su.inst + 3] = su.samplerValue;

		spv.erase(spv.begin() + su.inst + 4);
	}

	// Rename X_CombinedSampler to X.
	for (const auto& group : foldUsesBySamplerVar)
	{
		uint32_t samplerVar = group.first;
		uint32_t imageVar = sampleUses[group.second.front()].imageVar;

		auto imageName = names.find(imageVar);
		if (imageName == names.end())
			continue;

		std::vector<uint32_t> encodedName = encodeString(imageName->second);

		for (size_t i = 5; i < spv.size();)
		{
			if (!validInst(i))
				return false;

			uint32_t wc = WC(spv[i]);
			uint32_t op = OP(spv[i]);

			if (op == OpName && wc >= 3 && spv[i + 1] == samplerVar)
			{
				std::vector<uint32_t> newInst;
				newInst.reserve(2 + encodedName.size());
				newInst.push_back(MakeOp(uint32_t(2 + encodedName.size()), OpName));
				newInst.push_back(samplerVar);
				newInst.insert(newInst.end(), encodedName.begin(), encodedName.end());

				spv.erase(spv.begin() + i, spv.begin() + i + wc);
				spv.insert(spv.begin() + i, newInst.begin(), newInst.end());

				names[samplerVar] = imageName->second;
				break;
			}

			i += wc;
		}
	}

	return true;
}

//----------------------------------------------------------------------------------------
// Workaround for a MoltenVK bug on macOS translating SPIR-V to MSL. Fixes errors like this:
//   Validator Info mvk-info: Compiling Metal shader with FastMath enabled and PreserveInvariance disabled.
//   [mvk-error] VK_ERROR_INITIALIZATION_FAILED: Shader library compile failed (Error code 3):
//   program_source:51:12: error: must use 'struct' tag to refer to type 'drawCountRawBuffer' in this scope
//     device drawCountRawBuffer* indirectDrawData [[id(5)]];
//            ^
//            struct
//   program_source:49:32: note: struct 'drawCountRawBuffer' is hidden by a non-type declaration of 'drawCountRawBuffer' here
//     device drawCountRawBuffer* drawCountRawBuffer [[id(3)]];
//                                ^
//##TODO## ChatGPT generated. Needs review.
bool ShaderCode::FixStructTypeNameCollisionsOnMoltenVK(std::vector<uint32_t>& spirv)
{
	// SPIR-V header is 5 words.
	if (spirv.size() < 5) return false;

	// Local opcode constants (SPIR-V 1.x).
	constexpr uint16_t OpName = 5;
	constexpr uint16_t OpTypeStruct = 30;
	constexpr uint16_t OpVariable = 59;

	// Decode a SPIR-V literal string from words (little-endian bytes, nul-terminated).
	auto decodeString = [](const uint32_t* words, size_t wordCount) -> std::string {
		std::string out;
		out.reserve(wordCount * 4);
		for (size_t i = 0; i < wordCount; ++i)
		{
			uint32_t w = words[i];
			for (int b = 0; b < 4; ++b)
			{
				char c = static_cast<char>((w >> (8 * b)) & 0xFF);
				if (c == '\0') return out;
				out.push_back(c);
			}
		}
		return out;
	};

	// Encode a C-string into SPIR-V words (little-endian), including terminating '\0'.
	auto encodeStringWords = [](const std::string& s) -> std::vector<uint32_t> {
		const size_t byteCount = s.size() + 1; // include '\0'
		const size_t wordCount = (byteCount + 3) / 4;
		std::vector<uint32_t> words(wordCount, 0u);
		for (size_t i = 0; i < s.size(); ++i)
		{
			size_t wi = i / 4;
			size_t bi = i % 4;
			words[wi] |= (static_cast<uint32_t>(static_cast<uint8_t>(s[i])) << (8 * bi));
		}
		return words; // null already present
	};

	// Choose a unique type name based on base, avoiding any used names.
	auto makeUnique = [](const std::string& base, const std::unordered_set<std::string>& used) -> std::string {
		std::string c = base + "_t";
		if (!used.count(c)) return c;
		for (uint32_t i = 2; i < 1000000u; ++i)
		{
			c = base + "_t" + std::to_string(i);
			if (!used.count(c)) return c;
		}
		return base + "_t_unique"; // extremely unlikely fallback
	};

	// ---- Pass 1: parse instructions and collect names/IDs ----
	std::unordered_set<uint32_t> structTypeIds;
	std::unordered_set<uint32_t> variableIds;
	std::unordered_map<uint32_t, std::string> idToName; // targetId -> name (OpName)
	idToName.reserve(512);

	size_t idx = 5;
	while (idx < spirv.size())
	{
		const uint32_t first = spirv[idx];
		const uint16_t wc = static_cast<uint16_t>(first >> 16);
		const uint16_t op = static_cast<uint16_t>(first & 0xFFFF);

		if (wc == 0 || idx + wc > spirv.size())
			return false; // malformed SPIR-V

		if (op == OpTypeStruct)
		{
			// OpTypeStruct %resultId %memberType0 ...
			if (wc >= 2) structTypeIds.insert(spirv[idx + 1]);
		}
		else if (op == OpVariable)
		{
			// OpVariable %resultType %resultId %storageClass [initializer]
			if (wc >= 4) variableIds.insert(spirv[idx + 2]);
		}
		else if (op == OpName)
		{
			// OpName %target "literal"
			if (wc >= 3)
			{
				const uint32_t targetId = spirv[idx + 1];
				const std::string nm = decodeString(&spirv[idx + 2], static_cast<size_t>(wc - 2));
				idToName[targetId] = nm; // last one wins if duplicated
			}
		}

		idx += wc;
	}

	if (idToName.empty() || structTypeIds.empty() || variableIds.empty())
		return false;

	// Build sets of used names + variable names.
	std::unordered_set<std::string> usedNames;
	usedNames.reserve(idToName.size() * 2);
	for (const auto& kv : idToName)
		if (!kv.second.empty()) usedNames.insert(kv.second);

	std::unordered_set<std::string> variableNames;
	variableNames.reserve(variableIds.size() * 2);
	for (uint32_t varId : variableIds)
	{
		auto it = idToName.find(varId);
		if (it != idToName.end() && !it->second.empty())
			variableNames.insert(it->second);
	}

	// Determine which struct type IDs should be renamed: structName == any variableName.
	std::unordered_map<uint32_t, std::string> renameStruct;
	renameStruct.reserve(structTypeIds.size());
	for (uint32_t structId : structTypeIds)
	{
		auto it = idToName.find(structId);
		if (it == idToName.end()) continue;
		const std::string& structName = it->second;
		if (structName.empty()) continue;

		if (variableNames.count(structName))
		{
			std::string newName = makeUnique(structName, usedNames);
			usedNames.insert(newName);
			renameStruct[structId] = std::move(newName);
		}
	}

	if (renameStruct.empty())
		return false;

	// ---- Pass 2: rebuild module, rewriting only OpName for affected struct IDs ----
	std::vector<uint32_t> out;
	out.reserve(spirv.size() + 64);
	out.insert(out.end(), spirv.begin(), spirv.begin() + 5); // header

	idx = 5;
	bool changed = false;

	while (idx < spirv.size())
	{
		const uint32_t first = spirv[idx];
		const uint16_t wc = static_cast<uint16_t>(first >> 16);
		const uint16_t op = static_cast<uint16_t>(first & 0xFFFF);

		if (wc == 0 || idx + wc > spirv.size())
			return false; // malformed SPIR-V

		if (op == OpName && wc >= 3)
		{
			const uint32_t targetId = spirv[idx + 1];
			auto rn = renameStruct.find(targetId);
			if (rn != renameStruct.end())
			{
				const auto encoded = encodeStringWords(rn->second);
				const uint16_t newWc = static_cast<uint16_t>(2 + encoded.size());
				const uint32_t newFirst = (static_cast<uint32_t>(newWc) << 16) | static_cast<uint32_t>(OpName);

				out.push_back(newFirst);
				out.push_back(targetId);
				out.insert(out.end(), encoded.begin(), encoded.end());

				changed = true;
				idx += wc;
				continue;
			}
		}

		// Copy instruction unchanged.
		out.insert(out.end(), spirv.begin() + idx, spirv.begin() + idx + wc);
		idx += wc;
	}

	if (changed) spirv.swap(out);
	return changed;
}

//----------------------------------------------------------------------------------------
bool ShaderCode::FixMissingLocationAttributes(std::vector<uint32_t>& spirv)
{
	// Patches SPIR-V binary by inserting:
	//   OpDecorate %id Location <unique>
	// for any Input/Output OpVariable missing Location,
	// while skipping BuiltIns AND "built-in interface blocks" like gl_PerVertex
	// whose struct members are BuiltIn-decorated.
	//
	// No third-party libs. Returns true if modified.

	if (spirv.size() < 5) return false; // SPIR-V header is 5 words

	// ---- Local SPIR-V constants (SPIR-V 1.x) ----
	constexpr uint16_t OpDecorate = 71;
	constexpr uint16_t OpMemberDecorate = 72;
	constexpr uint16_t OpVariable = 59;
	constexpr uint16_t OpTypePointer = 32;
	constexpr uint16_t OpTypeStruct = 30;

	// StorageClass enum values
	constexpr uint32_t StorageClassInput = 1;
	constexpr uint32_t StorageClassOutput = 3;

	// Decoration enum values
	constexpr uint32_t DecorationBuiltIn = 11;
	constexpr uint32_t DecorationLocation = 30;

	auto wcOf = [](uint32_t firstWord) -> uint16_t { return static_cast<uint16_t>(firstWord >> 16); };
	auto opOf = [](uint32_t firstWord) -> uint16_t { return static_cast<uint16_t>(firstWord & 0xFFFFu); };

	// Insert annotations before the first type/const instruction (canonical-ish ordering).
	auto isTypeOrConstOpcode = [](uint16_t op) -> bool {
		// Type declarations (19..39), constants/spec constants (41..51) in SPIR-V 1.x
		return (op >= 19 && op <= 39) || (op >= 41 && op <= 51);
	};

	// ---- Pass 1: parse module for:
	//  - IO variables and their types
	//  - existing Location usage
	//  - direct BuiltIn decorations
	//  - structs with BuiltIn-decorated members
	//  - pointer type -> pointee type map
	//  - insertion point
	std::vector<uint32_t> ioVars; // IDs of Input/Output variables
	ioVars.reserve(64);

	std::unordered_map<uint32_t, uint32_t> varIdToPtrType;   // varId -> pointerTypeId
	std::unordered_map<uint32_t, uint32_t> ptrTypeToPointee; // ptrTypeId -> pointeeTypeId
	std::unordered_set<uint32_t> structIds;                  // OpTypeStruct result IDs
	std::unordered_set<uint32_t> structHasBuiltInMember;     // structId where any member has BuiltIn
	std::unordered_set<uint32_t> varIsDirectBuiltIn;         // varId with OpDecorate BuiltIn
	std::unordered_set<uint32_t> varHasLocation;             // varId with OpDecorate Location
	std::unordered_set<uint32_t> usedLocations;              // all used location literals

	varIdToPtrType.reserve(128);
	ptrTypeToPointee.reserve(128);
	structIds.reserve(128);
	structHasBuiltInMember.reserve(128);
	varIsDirectBuiltIn.reserve(128);
	varHasLocation.reserve(128);
	usedLocations.reserve(128);

	size_t insertAt = 5;
	bool foundInsertAt = false;

	for (size_t idx = 5; idx < spirv.size();)
	{
		const uint32_t first = spirv[idx];
		const uint16_t wc = wcOf(first);
		const uint16_t op = opOf(first);

		if (wc == 0 || idx + wc > spirv.size())
			return false; // malformed

		if (!foundInsertAt && isTypeOrConstOpcode(op))
		{
			insertAt = idx;
			foundInsertAt = true;
		}

		if (op == OpTypeStruct)
		{
			// OpTypeStruct %resultId %memberType0 ...
			if (wc >= 2)
				structIds.insert(spirv[idx + 1]);
		}
		else if (op == OpTypePointer)
		{
			// OpTypePointer %resultId %storageClass %type
			if (wc >= 4)
			{
				const uint32_t ptrTypeId = spirv[idx + 1];
				const uint32_t pointeeId = spirv[idx + 3];
				ptrTypeToPointee[ptrTypeId] = pointeeId;
			}
		}
		else if (op == OpVariable)
		{
			// OpVariable %resultType %resultId %storageClass [initializer]
			if (wc >= 4)
			{
				const uint32_t resultType = spirv[idx + 1]; // pointer type
				const uint32_t resultId = spirv[idx + 2];
				const uint32_t storage = spirv[idx + 3];

				if (storage == StorageClassInput || storage == StorageClassOutput)
				{
					ioVars.push_back(resultId);
					varIdToPtrType[resultId] = resultType;
				}
			}
		}
		else if (op == OpDecorate && wc >= 4)
		{
			// OpDecorate %target Decoration [literals...]
			const uint32_t targetId = spirv[idx + 1];
			const uint32_t decoration = spirv[idx + 2];

			if (decoration == DecorationBuiltIn)
			{
				varIsDirectBuiltIn.insert(targetId);
			}
			else if (decoration == DecorationLocation)
			{
				varHasLocation.insert(targetId);
				usedLocations.insert(spirv[idx + 3]);
			}
		}
		else if (op == OpMemberDecorate && wc >= 5)
		{
			// OpMemberDecorate %struct member Decoration [literals...]
			const uint32_t structId = spirv[idx + 1];
			const uint32_t decoration = spirv[idx + 3];
			if (decoration == DecorationBuiltIn)
				structHasBuiltInMember.insert(structId);
		}

		idx += wc;
	}

	if (ioVars.empty())
		return false;

	// Helper: determine if an IO var should be treated as built-in.
	auto isBuiltInVar = [&](uint32_t varId) -> bool {
		if (varIsDirectBuiltIn.count(varId))
			return true;

		auto itPtr = varIdToPtrType.find(varId);
		if (itPtr == varIdToPtrType.end())
			return false;

		auto itPointee = ptrTypeToPointee.find(itPtr->second);
		if (itPointee == ptrTypeToPointee.end())
			return false;

		const uint32_t pointeeTypeId = itPointee->second;

		// If the pointee type is a struct and any member is BuiltIn => treat variable as built-in (e.g. gl_PerVertex).
		if (structIds.count(pointeeTypeId) && structHasBuiltInMember.count(pointeeTypeId))
			return true;

		return false;
	};

	// Decide which vars to decorate.
	std::vector<std::pair<uint32_t, uint32_t>> toDecorate; // (varId, location)
	toDecorate.reserve(ioVars.size());

	auto nextUnusedLocation = [&]() -> uint32_t {
		uint32_t loc = 0;
		while (usedLocations.count(loc)) ++loc;
		usedLocations.insert(loc);
		return loc;
	};

	for (uint32_t varId : ioVars)
	{
		if (isBuiltInVar(varId))
			continue; // IMPORTANT: skip built-ins, including gl_PerVertex-like interface blocks
		if (varHasLocation.count(varId))
			continue; // already has a location

		toDecorate.emplace_back(varId, nextUnusedLocation());
	}

	if (toDecorate.empty())
		return false;

	// Build OpDecorate %id Location <literal>
	auto makeDecorateLocationInst = [](uint32_t id, uint32_t loc) -> std::array<uint32_t, 4> {
		constexpr uint16_t OpDecorateLocal = 71;
		constexpr uint32_t DecorationLocationLocal = 30;
		const uint32_t first = (static_cast<uint32_t>(4) << 16) | static_cast<uint32_t>(OpDecorateLocal);
		return {first, id, DecorationLocationLocal, loc};
	};

	// ---- Pass 2: rebuild with inserted annotations ----
	std::vector<uint32_t> out;
	out.reserve(spirv.size() + toDecorate.size() * 4);

	// Copy header.
	out.insert(out.end(), spirv.begin(), spirv.begin() + 5);

	for (size_t idx = 5; idx < spirv.size();)
	{
		if (idx == insertAt)
		{
			for (const auto& [id, loc] : toDecorate)
			{
				const auto inst = makeDecorateLocationInst(id, loc);
				out.insert(out.end(), inst.begin(), inst.end());
			}
		}

		const uint32_t first = spirv[idx];
		const uint16_t wc = wcOf(first);

		if (wc == 0 || idx + wc > spirv.size())
			return false;

		out.insert(out.end(), spirv.begin() + idx, spirv.begin() + idx + wc);
		idx += wc;
	}

	// If we never found a types/consts boundary, insert at end (rare, but safe).
	if (!foundInsertAt)
	{
		for (const auto& [id, loc] : toDecorate)
		{
			const auto inst = makeDecorateLocationInst(id, loc);
			out.insert(out.end(), inst.begin(), inst.end());
		}
	}

	spirv.swap(out);
	return true;
}
// NOLINTEND

//----------------------------------------------------------------------------------------
// String manipulation methods
//----------------------------------------------------------------------------------------
void ShaderCode::StringReplaceAll(std::string& source, const std::string& from, const std::string& to)
{
	std::string newString;
	newString.reserve(source.length());

	std::string::size_type lastPos = 0;
	std::string::size_type findPos;
	while (std::string::npos != (findPos = source.find(from, lastPos)))
	{
		newString.append(source, lastPos, findPos - lastPos);
		newString += to;
		lastPos = findPos + from.length();
	}

	newString += source.substr(lastPos);

	source.swap(newString);
}

//----------------------------------------------------------------------------------------
void ShaderCode::StringTrim(std::string& s)
{
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) { return std::isspace(ch) == 0; }));
	s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) { return std::isspace(ch) == 0; }).base(), s.end());
}

} // namespace cobalt::graphics
