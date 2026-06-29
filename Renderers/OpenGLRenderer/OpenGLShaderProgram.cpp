// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "OpenGLShaderProgram.h"
#include "OpenGLDebug.h"
#include "OpenGLRenderer.h"
#include "OpenGLStateBufferLayout.h"
#include <Internal/ShaderSupport/ShaderSupport.pkg>
#include <Internal/RendererSupport/KnownDynamicCast.h>
#include <algorithm>
#include <regex>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
OpenGLShaderProgram::OpenGLShaderProgram(cobalt::logging::ILogger* log, OpenGLRenderer* renderer)
: _log(log), _renderer(renderer), _shaderCompiled(false)
{
	// Set our default shader target info
	ShaderTargetInfoOpenGL::Flags shaderTargetFlags = ShaderTargetInfoOpenGL::Flags::None;
	_shaderTargetInfo = ShaderTargetInfoOpenGL(shaderTargetFlags);
}

//----------------------------------------------------------------------------------------
OpenGLShaderProgram::~OpenGLShaderProgram()
{
	ReleaseMemory();
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void OpenGLShaderProgram::Delete()
{
	_renderer->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
// Code format methods
//----------------------------------------------------------------------------------------
bool OpenGLShaderProgram::IsCodeFormatSupported(CodeFormat format) const
{
	return (format == CodeFormat::HLSL) || (format == CodeFormat::SPIRVAssembly) || (format == CodeFormat::SPIRV) || (format == CodeFormat::GLSL);
}

//----------------------------------------------------------------------------------------
OpenGLShaderProgram::CodeFormat OpenGLShaderProgram::PreferredCodeFormat() const
{
	return (_renderer->SpirvShadersSupported() ? CodeFormat::SPIRV : CodeFormat::GLSL);
}

//----------------------------------------------------------------------------------------
// Compilation methods
//----------------------------------------------------------------------------------------
SuccessToken OpenGLShaderProgram::ConfigureShaderTarget(const ShaderTargetInfoBase& shaderTargetInfo)
{
	// Resolve the structure down to its native type
	auto shaderTargetType = shaderTargetInfo.shaderTarget;
	if (shaderTargetType != ShaderTargetInfoBase::ShaderTarget::OpenGL)
	{
		_log->Error("Attempted to configure shader target using incompatible target type {0}", shaderTargetType);
		return false;
	}
	auto shaderTargetInfoResolved = reinterpret_cast<const ShaderTargetInfoOpenGL*>(&shaderTargetInfo);

	// Load the new shader target settings
	_shaderTargetInfo = *shaderTargetInfoResolved;
	return true;
}

//----------------------------------------------------------------------------------------
SuccessToken OpenGLShaderProgram::LoadShaderStage(ShaderStage stage, const ShaderSourceInfoBase& shaderSourceInfo)
{
	// Ensure something hasn't already been added for the target shader stage
	if (_shaderBlocks.find(stage) != _shaderBlocks.end())
	{
		_log->Error("Attempted to bind shader code for stage {0} when code has already been bound.", stage);
		return false;
	}

	// Ensure a valid shader stage is being targeted
	switch (stage)
	{
	case ShaderStage::Vertex:
	case ShaderStage::Geometry:
	case ShaderStage::Fragment:
		break;
#ifdef GL_VERSION_4_3
	case ShaderStage::Compute:
		break;
#endif
	default:
		_log->Error("Attempted to bind shader for unsupported stage {0}.", stage);
		return false;
	}

	// Retrieve the supplied shader info
	CodeFormat codeFormat = {};
	std::string codeAsGLSL;
	std::string codeAsHLSL;
	const uint8_t* code = nullptr;
	size_t codeSizeInBytes = 0;
	std::string entryPointName = IShaderCode::StandardShaderEntryPointName;
	auto shaderSourceType = shaderSourceInfo.shaderType;
	if (shaderSourceType == ShaderSourceInfoBase::ShaderType::HLSL)
	{
		auto shaderSourceInfoResolved = reinterpret_cast<const ShaderSourceInfoBaseHLSL*>(&shaderSourceInfo);
		codeFormat = CodeFormat::HLSL;
		code = reinterpret_cast<const uint8_t*>(shaderSourceInfoResolved->code);
		codeSizeInBytes = shaderSourceInfoResolved->codeSizeInBytes;
		if (shaderSourceInfoResolved->entryPointFunctionName != nullptr)
		{
			entryPointName = std::string(shaderSourceInfoResolved->entryPointFunctionName, shaderSourceInfoResolved->entryPointFunctionNameSizeInBytes);
		}
		codeAsHLSL.assign(shaderSourceInfoResolved->code, shaderSourceInfoResolved->code + shaderSourceInfoResolved->codeSizeInBytes);
	}
	else if (shaderSourceType == ShaderSourceInfoBase::ShaderType::SPIRVAssembly)
	{
		auto shaderSourceInfoResolved = reinterpret_cast<const ShaderSourceInfoBaseSPIRVAssembly*>(&shaderSourceInfo);
		codeFormat = CodeFormat::SPIRVAssembly;
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
		codeFormat = CodeFormat::SPIRV;
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
		codeFormat = CodeFormat::GLSL;
		code = reinterpret_cast<const uint8_t*>(shaderSourceInfoResolved->code);
		codeSizeInBytes = shaderSourceInfoResolved->codeSizeInBytes;
		codeAsGLSL.assign(shaderSourceInfoResolved->code, shaderSourceInfoResolved->code + shaderSourceInfoResolved->codeSizeInBytes);
	}
	else if (shaderSourceType == ShaderSourceInfoBase::ShaderType::MSL)
	{
		auto shaderSourceInfoResolved = reinterpret_cast<const ShaderSourceInfoBaseMSL*>(&shaderSourceInfo);
		codeFormat = CodeFormat::MSL;
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

	// Store information on the provided shader code block
	ShaderBlockInfo& blockInfo = _shaderBlocks[stage];
	blockInfo.stage = stage;
	blockInfo.format = codeFormat;
	blockInfo.code.assign(code, code + codeSizeInBytes);
	blockInfo.codeAsGLSLCached = codeAsGLSL;
	blockInfo.codeAsHLSLCached = codeAsHLSL;
	blockInfo.entryPointName = entryPointName;
	return true;
}

//----------------------------------------------------------------------------------------
GLenum OpenGLShaderProgram::ShaderStageToGLEnum(ShaderStage stage) const
{
	switch (stage)
	{
	case ShaderStage::Vertex:
		return GL_VERTEX_SHADER;
	case ShaderStage::Fragment:
		return GL_FRAGMENT_SHADER;
	case ShaderStage::Geometry:
		return GL_GEOMETRY_SHADER;
#ifdef GL_VERSION_4_3
	case ShaderStage::Compute:
		return GL_COMPUTE_SHADER;
#endif
	}
	UNREACHABLE();
	return {};
}

//----------------------------------------------------------------------------------------
bool OpenGLShaderProgram::CompileShaderBlock(ShaderBlockInfo& blockInfo, bool hasGeometryStage, GLuint& shaderID)
{
	// Map the shader stage value
	IShaderCode::Stage shaderStage = IShaderCode::Stage::Vertex;
	if (blockInfo.stage == ShaderStage::Vertex)
	{
		shaderStage = IShaderCode::Stage::Vertex;
	}
	else if (blockInfo.stage == ShaderStage::Geometry)
	{
		shaderStage = IShaderCode::Stage::Geometry;
	}
	else if (blockInfo.stage == ShaderStage::Fragment)
	{
		shaderStage = IShaderCode::Stage::Fragment;
	}
#ifdef GL_VERSION_4_3
	else if (blockInfo.stage == ShaderStage::Compute)
	{
		shaderStage = IShaderCode::Stage::Compute;
	}
#endif
	else
	{
		_log->Error("Attempted to compile shader for unsupported stage {0}.", blockInfo.stage);
		return false;
	}

	// Determine the shader language and environment settings to use
	IShaderCode::Language language = IShaderCode::Language::HLSL;
	IShaderCode::Environment sourceEnvironment = IShaderCode::Environment::General;
	switch (blockInfo.format)
	{
	case CodeFormat::HLSL:
		language = IShaderCode::Language::HLSL;
		break;
	case CodeFormat::SPIRVAssembly:
		language = IShaderCode::Language::SPIRVAssembly;
		sourceEnvironment = IShaderCode::Environment::Vulkan_11;
		break;
	case CodeFormat::SPIRV:
		language = IShaderCode::Language::SPIRV;
		sourceEnvironment = IShaderCode::Environment::Vulkan_11;
		break;
	case CodeFormat::GLSL:
		language = IShaderCode::Language::GLSL;
		sourceEnvironment = IShaderCode::Environment::OpenGL_43;
		break;
	case CodeFormat::MSL:
		language = IShaderCode::Language::MSL;
		break;
	default:
		_log->Error("Invalid code present for shader stage {0}", blockInfo.stage);
		return false;
	}

	// Determine the shader language to target. Note that we don't target SPIRV by default even if available, as the
	// necessity of performing extra conversion steps to ensure global uniforms are mapped correctly adds complexity and
	// overhead, creating worse performance and more possible points of failure.
	bool forceGLSL = ((_shaderTargetInfo.flags & ShaderTargetInfoOpenGL::Flags::ForceGLSL) != ShaderTargetInfoOpenGL::Flags::None);
	bool forceSPIRVIfAvailable = ((_shaderTargetInfo.flags & ShaderTargetInfoOpenGL::Flags::ForceSPIRVIfAvailable) != ShaderTargetInfoOpenGL::Flags::None);
	bool spirvAvailable = _renderer->SpirvShadersSupported();
	//##FIX## We disable SPIRV support entirely here right now. The problem is reflection. So names are not guaranteed
	//to be preserved through the linking process when they come from SPIRV in OpenGL. On Windows, it appears they are
	//preserved on all tested hardware, however this is not guaranteed by GL_ARB_gl_spirv. On Linux under Mesa, they are
	//not preserved. Reflection APIs give type and binding information, but return empty names. We could use the
	//extracted SPIRV reflection information to match and restore them, but this seems more trouble than it's worth at
	//this time. We instead stick to exporting to GLSL and using that instead.
	spirvAvailable = false;
	bool exportToSPIRV = spirvAvailable && forceSPIRVIfAvailable;
	//##DEBUG##
	//bool exportToSPIRV = !forceGLSL && spirvAvailable;

	// Set the target environment
	auto targetEnvironment = IShaderCode::Environment::OpenGL_33;
#ifdef GL_VERSION_4_3
	targetEnvironment = IShaderCode::Environment::OpenGL_43;
#endif

	// Determine the final code format and code data to use for the shader, performing any necessary conversions.
	CodeFormat finalCodeFormat;
	std::vector<uint8_t> finalCodeBlock;
	if ((forceGLSL || !(forceSPIRVIfAvailable && spirvAvailable)) && (language == IShaderCode::Language::GLSL))
	{
		finalCodeFormat = CodeFormat::GLSL;
		finalCodeBlock = blockInfo.code;
	}
	else if (spirvAvailable && !forceGLSL && (language == IShaderCode::Language::SPIRV))
	{
		// Load the native SPIR-V. Note that we still need to load native SPIR-V in this manner in order to generate
		// binding locations. Also note that the input SPIR-V here needs to be OpenGL compatible SPIR-V, with global
		// uniforms represented as StorageClassUniformConstant types instead of in a $Global UBO.
		auto shaderCode = IShaderCode::Create(_log->GetLoggerChildScope("ShaderSupport"));
		if (!shaderCode->LoadCode(language, shaderStage, sourceEnvironment, targetEnvironment, blockInfo.code.data(), blockInfo.code.size(), blockInfo.entryPointName, _baseBindingNoConversionTemp, _shaderResourceInfoConversionTemp))
		{
			_log->Error("Failed to import shader code for stage {0}", shaderStage);
			return false;
		}

		// Export the SPIRV back out with binding locations unified across the stages
		std::vector<uint32_t> convertedCode;
		if (!shaderCode->ExportCodeAsSPIRV(convertedCode))
		{
			_log->Error("Failed to convert shader code for stage {0}", shaderStage);
			return false;
		}
		finalCodeFormat = CodeFormat::SPIRV;
		finalCodeBlock.assign(reinterpret_cast<const uint8_t*>(convertedCode.data()), reinterpret_cast<const uint8_t*>(convertedCode.data()) + (convertedCode.size() * sizeof(*convertedCode.data())));
	}
	else
	{
		// Convert the code to GLSL if required
		if (blockInfo.codeAsGLSLCached.empty())
		{
			// Load the input code
			auto shaderCode = IShaderCode::Create(_log->GetLoggerChildScope("ShaderSupport"));
			if (!shaderCode->LoadCode(language, shaderStage, sourceEnvironment, targetEnvironment, blockInfo.code.data(), blockInfo.code.size(), blockInfo.entryPointName, (language == IShaderCode::Language::HLSL ? _baseBindingNoForGlobalBufferReflection : _baseBindingNoConversionTemp), (language == IShaderCode::Language::HLSL ? _shaderResourceInfoForGlobalBufferReflection : _shaderResourceInfoConversionTemp)))
			{
				_log->Error("Failed to import shader code for stage {0}", shaderStage);
				return false;
			}

			// Export the code to GLSL
			if (!shaderCode->ExportCodeAsGLSL(blockInfo.codeAsGLSLCached, hasGeometryStage))
			{
				_log->Error("Failed to convert shader code to GLSL for stage {0}", shaderStage);
				return false;
			}
		}

		// If we're targeting SPIRV as the final output format, convert the GLSL to SPIRV, otherwise output the GLSL
		// directly.
		if (!exportToSPIRV)
		{
			finalCodeFormat = CodeFormat::GLSL;
			finalCodeBlock.assign(reinterpret_cast<const uint8_t*>(blockInfo.codeAsGLSLCached.data()), reinterpret_cast<const uint8_t*>(blockInfo.codeAsGLSLCached.data()) + blockInfo.codeAsGLSLCached.size());
		}
		else
		{
			// Convert the GLSL to SPIRV. Since we've run through the GLSL conversion first, this will ensure that
			// global uniforms are represented as StorageClassUniformConstant types instead of in a $Global UBO, which
			// is required for OpenGL compatible SPIRV. OpenGL doesn't unroll $Global UBO buffers to actual uniforms,
			// and OpenGL also can't link SPIRV shaders with different binding locations and structure defintions
			// between stages. Our current toolchain needs our SPIRV to be generated from GLSL to preserve these
			// requirements, so we pass through GLSL first to get OpenGL compatible SPIRV out of it.
			auto shaderCode = IShaderCode::Create(_log->GetLoggerChildScope("ShaderSupport"));
			if (!shaderCode->LoadCode(IShaderCode::Language::GLSL, shaderStage, targetEnvironment, targetEnvironment, reinterpret_cast<const uint8_t*>(blockInfo.codeAsGLSLCached.data()), blockInfo.codeAsGLSLCached.size(), IShaderCode::StandardShaderEntryPointName, _baseBindingNoConversionTemp2, _shaderResourceInfoConversionTemp2))
			{
				_log->Error("Failed to import shader code for stage {0}", shaderStage);
				return false;
			}
			std::vector<uint32_t> codeAsSPIRV;
			if (!shaderCode->ExportCodeAsSPIRV(codeAsSPIRV))
			{
				_log->Error("Failed to convert shader code to SPIRV for stage {0}", shaderStage);
				return false;
			}
			finalCodeFormat = CodeFormat::SPIRV;
			finalCodeBlock.assign(reinterpret_cast<const uint8_t*>(codeAsSPIRV.data()), reinterpret_cast<const uint8_t*>(codeAsSPIRV.data()) + (codeAsSPIRV.size() * sizeof(*codeAsSPIRV.data())));
		}
	}

	// Create the shader object
	shaderID = glCreateShader(ShaderStageToGLEnum(blockInfo.stage));
	if (shaderID == 0)
	{
		_log->Error("Failed to create shader for stage {0}", blockInfo.stage);
		return false;
	}

	// Compile the shader
	const char* shaderCode = reinterpret_cast<const char*>(finalCodeBlock.data());
	auto shaderCodeSize = (GLint)finalCodeBlock.size();
	if (finalCodeFormat == CodeFormat::SPIRV)
	{
		glShaderBinary(1, &shaderID, GL_SHADER_BINARY_FORMAT_SPIR_V_ARB, shaderCode, shaderCodeSize);
		glSpecializeShaderARB(shaderID, blockInfo.entryPointName.c_str(), 0, nullptr, nullptr);
	}
	else
	{
		glShaderSource(shaderID, 1, &shaderCode, &shaderCodeSize);
		glCompileShader(shaderID);
	}

	// Check the result of the compilation process
	int shaderCompileResult;
	glGetShaderiv(shaderID, GL_COMPILE_STATUS, &shaderCompileResult);
	if (shaderCompileResult == 0)
	{
		GLint logLength;
		glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &logLength);
		std::string infoLog;
		infoLog.resize(logLength);
		glGetShaderInfoLog(shaderID, logLength, &logLength, infoLog.data());
		infoLog.push_back(0);
		_log->Info("Compilation failed - The following shader code was provided:");

		auto lineIt = finalCodeBlock.begin();
		auto lineCount = 1;
		while (lineIt != finalCodeBlock.end())
		{
			auto endOfLine = std::find(lineIt, finalCodeBlock.end(), '\n');

			auto lineNo = std::to_string(lineCount);
			while (lineNo.size() < 3)
			{
				std::string temp = "0";
				temp += lineNo;
				lineNo.swap(temp);
			}

			_log->Info(lineNo + ": " + std::string(lineIt, endOfLine));

			if (endOfLine == finalCodeBlock.end())
			{
				break;
			}

			lineIt = endOfLine + 1;
			lineCount++;
		}
		_log->Info("(End of shader code in which compilation failed)");
		if (!infoLog.empty())
		{
			_log->Warning("Shader compilation failed for stage {0}. {1}", blockInfo.stage, infoLog);
		}
		else
		{
			_log->Warning("Shader compilation failed for stage {0}.", blockInfo.stage);
		}
		return false;
	}
	return true;
}

//----------------------------------------------------------------------------------------
SuccessToken OpenGLShaderProgram::CompileProgram()
{
	// We need to invoke this on the render thread, as the application needs to be able to retrieve IDs after
	// compilation, but we can't get IDs until we are able to complete the compilation process. Unfortunately as we
	// can't use a rendering context across threads in OpenGL, we have to perform this compilation process on the render
	// thread. Other APIs such as Direct3D and Vulkan don't have this limitation here.
	return _renderer->RenderThreadInvokeSync([&] { return CompileProgramInternal(); });
}

//----------------------------------------------------------------------------------------
bool OpenGLShaderProgram::CompileProgramInternal()
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
	const bool geometryShaderPresent = (_shaderBlocks.find(ShaderStage::Geometry) != _shaderBlocks.end());
	std::vector<int> shaderIDs;
	for (auto& shaderBlockEntry : _shaderBlocks)
	{
		ShaderBlockInfo& shaderBlock = shaderBlockEntry.second;
		GLuint shaderID;
		if (!CompileShaderBlock(shaderBlock, geometryShaderPresent, shaderID))
		{
			_log->Error("Shader compilation failed for stage {0}.", shaderBlock.stage);
			for (int shader : shaderIDs)
			{
				glDeleteShader(shader);
			}
			CheckGLError(_log);
			return false;
		}
		shaderIDs.push_back(shaderID);
	}

	// Link the shader program
	GLuint shaderProgramID = glCreateProgram();
	if (shaderProgramID == 0)
	{
		_log->Error("Failed to generate new shader program.");
		CheckGLError(_log);
		return false;
	}
	for (int shaderID : shaderIDs)
	{
		glAttachShader(shaderProgramID, shaderID);
	}
	glLinkProgram(shaderProgramID);
	int shaderProgramLinkResult;
	glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &shaderProgramLinkResult);
	if (shaderProgramLinkResult == 0)
	{
		GLint logLength = 0;
		glGetProgramiv(shaderProgramID, GL_INFO_LOG_LENGTH, &logLength);
		std::string infoLog;
		infoLog.resize(logLength);
		glGetProgramInfoLog(shaderProgramID, logLength, &logLength, infoLog.data());
		infoLog.push_back(0);
		_log->Error("Shader linking failed. {0}", infoLog);
		CheckGLError(_log);
		glDeleteProgram(shaderProgramID);
		return false;
	}
	_programID = shaderProgramID;
	_shaderCompiled = true;

	// Obtain the maximum length of a vertex attribute name
	GLint maxVertexAttributeNameLength = 0;
	glGetProgramiv(_programID, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &maxVertexAttributeNameLength);

	// Retrieve information on all vertex attributes in the program
	GLint attributeCount = 0;
	glGetProgramiv(_programID, GL_ACTIVE_ATTRIBUTES, &attributeCount);
	std::vector<GLchar> vertexAttributeNameBuffer(maxVertexAttributeNameLength);
	for (size_t i = 0; i < (size_t)attributeCount; ++i)
	{
		// Retrieve information on the target vertex attribute
		GLsizei nameLength = 0;
		GLint dataSizeInBytes = 0;
		GLenum nativeDataType = 0;
		glGetActiveAttrib(_programID, (GLuint)i, (GLsizei)vertexAttributeNameBuffer.size(), &nameLength, &dataSizeInBytes, &nativeDataType, vertexAttributeNameBuffer.data());
		std::string attributeName(vertexAttributeNameBuffer.data(), nameLength);

		// Retrieve the location of the vertex attribute
		GLint location = glGetAttribLocation(_programID, attributeName.c_str());
		if (location < 0)
		{
			_log->Warning("Failed to locate vertex attribute with name \"{0}\"", attributeName);
			continue;
		}

		// Add this vertex attribute to the set of known vertex attributes
		ShaderInputParameterInfo parameterInfo;
		parameterInfo.name = attributeName;
		parameterInfo.location = location;
		parameterInfo.nativeDataType = nativeDataType;
		parameterInfo.dataSizeInBytes = dataSizeInBytes;
		_attributeNameList.push_back(parameterInfo);
		_attributeNameToID.insert(std::make_pair(attributeName, location));
	}

#ifdef GL_VERSION_4_3
	// Retrieve information on all variables inside shader storage blocks (SSBO's) in the program
	std::vector<ResourceBufferInfo> resourceBuffers;
	GLint resourceBufferCount;
	glGetProgramInterfaceiv(_programID, GL_SHADER_STORAGE_BLOCK, GL_ACTIVE_RESOURCES, &resourceBufferCount);
	for (GLint resourceBufferNo = 0; resourceBufferNo < resourceBufferCount; ++resourceBufferNo)
	{
		// Get the name of this data array
		GLint maxNameLength;
		glGetProgramInterfaceiv(_programID, GL_BUFFER_VARIABLE, GL_MAX_NAME_LENGTH, &maxNameLength);
		std::vector<GLchar> nameBufferRaw;
		nameBufferRaw.resize(maxNameLength);
		glGetProgramResourceName(_programID, GL_SHADER_STORAGE_BLOCK, (GLuint)resourceBufferNo, (GLsizei)nameBufferRaw.size(), nullptr, nameBufferRaw.data());
		std::string bufferName = nameBufferRaw.data();

		// If the buffer name has a period separator, take the name as the leading portion, unless it ends with an
		// underscore followed by a number, in which case we take the trailing portion. This fixes buffer names when
		// loading from SPIRV where there's both an instance name and a buffer name. In the case of an autogenerated
		// instance name from cross compilation, we use the original type name, while if there's apparently a manual
		// instance name, we use that instead.
		auto nameSeparatorPos = bufferName.find('.');
		if (nameSeparatorPos != std::string::npos)
		{
			bufferName = (std::regex_search(bufferName, std::regex("_\\d+$")) ? bufferName.substr(0, nameSeparatorPos) : bufferName.substr(nameSeparatorPos + 1));
		}

		// Retrieve basic information on this data array
		std::vector<GLenum> bufferPropertiesToRetrieve;
		bufferPropertiesToRetrieve.push_back(GL_BUFFER_BINDING);
		bufferPropertiesToRetrieve.push_back(GL_BUFFER_DATA_SIZE);
		bufferPropertiesToRetrieve.push_back(GL_NUM_ACTIVE_VARIABLES);
		std::vector<GLint> bufferProperties;
		bufferProperties.resize(bufferPropertiesToRetrieve.size());
		glGetProgramResourceiv(_programID, GL_SHADER_STORAGE_BLOCK, (GLuint)resourceBufferNo, (GLsizei)bufferPropertiesToRetrieve.size(), bufferPropertiesToRetrieve.data(), (GLsizei)bufferProperties.size(), nullptr, bufferProperties.data());

		// Build a buffer info object to hold information on this data array
		ResourceBufferInfo bufferInfo;
		bufferInfo.name = bufferName;
		bufferInfo.bindingPoint = (GLuint)bufferProperties[0];
		bufferInfo.minimumBufferSize = bufferProperties[1];

		// Retrieve the index values for each variable within this data array
		GLenum activeBufferVariablesEnum = GL_ACTIVE_VARIABLES;
		std::vector<GLint> bufferVariableIndices;
		bufferVariableIndices.resize(bufferProperties[2]);
		glGetProgramResourceiv(_programID, GL_SHADER_STORAGE_BLOCK, (GLuint)resourceBufferNo, 1, &activeBufferVariablesEnum, (GLsizei)bufferVariableIndices.size(), nullptr, bufferVariableIndices.data());

		// Retrieve information on each variable within this data array
		for (auto variableIndex : bufferVariableIndices)
		{
			// Retrieve basic information on this variable
			std::vector<GLenum> variablePropertiesToRetrieve;
			variablePropertiesToRetrieve.push_back(GL_TYPE);
			variablePropertiesToRetrieve.push_back(GL_NAME_LENGTH);
			variablePropertiesToRetrieve.push_back(GL_BLOCK_INDEX);
			variablePropertiesToRetrieve.push_back(GL_OFFSET);
			variablePropertiesToRetrieve.push_back(GL_ARRAY_SIZE);
			variablePropertiesToRetrieve.push_back(GL_ARRAY_STRIDE);
			variablePropertiesToRetrieve.push_back(GL_MATRIX_STRIDE);
			variablePropertiesToRetrieve.push_back(GL_IS_ROW_MAJOR);
			std::vector<GLint> variableProperties;
			variableProperties.resize(variablePropertiesToRetrieve.size());
			glGetProgramResourceiv(_programID, GL_BUFFER_VARIABLE, (GLuint)variableIndex, (GLsizei)variablePropertiesToRetrieve.size(), variablePropertiesToRetrieve.data(), (GLsizei)variableProperties.size(), nullptr, variableProperties.data());

			// Retrieve the name for this variable
			std::string variableName;
			variableName.resize(variableProperties[1] - 1);
			glGetProgramResourceName(_programID, GL_BUFFER_VARIABLE, (GLuint)variableIndex, (GLsizei)(variableName.size() + 1), nullptr, variableName.data());

			// Add information on this variable to the owning buffer object
			ResourceBufferVariableInfo variableInfo;
			variableInfo.name = variableName;
			variableInfo.type = (GLenum)variableProperties[0];
			variableInfo.blockIndex = variableProperties[2];
			variableInfo.bufferOffsetInBytes = variableProperties[3];
			variableInfo.arraySize = variableProperties[4];
			variableInfo.arrayStride = variableProperties[5];
			variableInfo.matrixStride = variableProperties[6];
			variableInfo.isRowMajor = (variableProperties[7] != 0);
			bufferInfo.variables.push_back(variableInfo);
		}

		// Add this data array to the list of identified data arrays
		bufferInfo.id = (ResourceArrayId)resourceBuffers.size();
		resourceBuffers.push_back(bufferInfo);
	}

	// Identify and link any associated counter buffers
	auto stringEndsWith = [](const std::string& str, const std::string& suffix) { return (str.size() >= suffix.size()) && (str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0); };
	//##TODO## The "_count" suffix is just what we get out of SPIRV-Cross by default. Consider making this longer and
	//more descriptive, like our "_CombinedSampler" suffix.
	std::string counterBufferSuffix = "_count";
	std::string counterBufferSuffixAlt = "_counter";
	for (auto& entry : resourceBuffers)
	{
		// Check if this appears to be a counter buffer
		if (!stringEndsWith(entry.name, counterBufferSuffix) && !stringEndsWith(entry.name, counterBufferSuffixAlt))
		{
			continue;
		}

		// Determine the expected name of the owning buffer
		std::string owningBufferName = entry.name;
		if (stringEndsWith(owningBufferName, counterBufferSuffix))
		{
			owningBufferName.resize(owningBufferName.size() - counterBufferSuffix.size());
		}
		else if (stringEndsWith(owningBufferName, counterBufferSuffixAlt))
		{
			owningBufferName.resize(owningBufferName.size() - counterBufferSuffixAlt.size());
		}

		// Attempt to locate the owning buffer
		bool foundOwningResourceBuffer = false;
		auto resourceBufferIterator = resourceBuffers.begin();
		while (!foundOwningResourceBuffer && (resourceBufferIterator != resourceBuffers.end()))
		{
			if (resourceBufferIterator->name == owningBufferName)
			{
				foundOwningResourceBuffer = true;
				break;
			}
			++resourceBufferIterator;
		}
		if (!foundOwningResourceBuffer)
		{
			continue;
		}
		auto& owningResourceBuffer = *resourceBufferIterator;

		entry.isCounterBuffer = true;
		entry.owningBufferName = owningBufferName;
		owningResourceBuffer.hasCounterBuffer = true;
		owningResourceBuffer.counterBufferId = entry.id;
	}

	// Store the identified set of data arrays
	_resourceBufferList = std::move(resourceBuffers);
	for (const auto& entry : _resourceBufferList)
	{
		_resourceBufferNameToID[entry.name] = (GLint)entry.id;
	}
#endif

	// Retrieve information on all uniform buffers in the program
	GLuint nextStateBufferBindPoint = 0;
	std::unordered_map<std::string, GLuint> stateBufferNameToBindPoint;
	GLint uniformBufferCount = 0;
	glGetProgramiv(_programID, GL_ACTIVE_UNIFORM_BLOCKS, &uniformBufferCount);
	_stateBufferBindPoints.resize(uniformBufferCount);
	for (size_t i = 0; i < (size_t)uniformBufferCount; ++i)
	{
		// Retrieve the length of the name of the target uniform buffer
		GLint uniformBufferNameLength;
		glGetActiveUniformBlockiv(_programID, (GLuint)i, GL_UNIFORM_BLOCK_NAME_LENGTH, &uniformBufferNameLength);

		// Retrieve the name of the target uniform buffer
		GLsizei nameLength = 0;
		std::vector<GLchar> uniformBufferNameBuffer(uniformBufferNameLength);
		glGetActiveUniformBlockName(_programID, (GLuint)i, (GLsizei)uniformBufferNameBuffer.size(), &nameLength, uniformBufferNameBuffer.data());
		std::string uniformBufferName(uniformBufferNameBuffer.data(), nameLength);

		// Retrieve or create a bind point for this uniform buffer
		GLuint bindPoint;
		auto stateBufferNameToBindPointIterator = stateBufferNameToBindPoint.find(uniformBufferName);
		if (stateBufferNameToBindPointIterator != stateBufferNameToBindPoint.end())
		{
			bindPoint = stateBufferNameToBindPointIterator->second;
		}
		else
		{
			bindPoint = nextStateBufferBindPoint++;
			stateBufferNameToBindPoint[uniformBufferName] = bindPoint;
		}

		// Add this uniform buffer to the set of known uniform buffers
		auto stateBufferId = (StateBufferId)i;
		_stateBufferBindPoints[i] = bindPoint;
		_stateBufferNameToID.insert(std::make_pair(uniformBufferName, (int)i));
		_stateBufferIDToName.insert(std::make_pair(stateBufferId, uniformBufferName));

		// Associate this uniform buffer to the selected binding point for this shader program
		glUniformBlockBinding(_programID, (GLuint)i, bindPoint);

		// Record information on the layout of this uniform buffer
		StateBufferLayoutInfo layoutInfo;
		ReadStateBufferLayoutInfoFromShader(stateBufferId, layoutInfo);
		_stateBufferLayoutInfo.insert(std::make_pair(stateBufferId, layoutInfo));
	}

	// Obtain the maximum length of a uniform name. Note that we have some paranoid checking code here that forces a
	// minimum name length of 256. We do this because it has been reported that some drivers report incorrect values for
	// GL_ACTIVE_UNIFORM_MAX_LENGTH, and we want to cap it to a safe minimum to work around this issue if it is present.
	// We could just leave it at 256 and be done with it, but we want to allow it to grow larger if required. See the
	// following page for more info:
	// https://stackoverflow.com/questions/12555165/incorrect-value-from-glgetprogramivprogram-gl-active-uniform-max-length-outpa
	GLint maxUniformNameLengthUncorrected = 0;
	glGetProgramiv(_programID, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxUniformNameLengthUncorrected);
	auto maxUniformNameLength = std::max(maxUniformNameLengthUncorrected, 256);

	// Retrieve information on all global uniforms in the program
	GLint uniformCount = 0;
	glGetProgramiv(_programID, GL_ACTIVE_UNIFORMS, &uniformCount);
	std::vector<GLchar> uniformNameBuffer(maxUniformNameLength);
	for (size_t i = 0; i < (size_t)uniformCount; ++i)
	{
		// Retrieve information on the target uniform variable
		GLsizei nameLength = 0;
		GLint dataSizeInBytes = 0;
		GLenum nativeDataType = 0;
		glGetActiveUniform(_programID, (GLuint)i, (GLsizei)uniformNameBuffer.size(), &nameLength, &dataSizeInBytes, &nativeDataType, uniformNameBuffer.data());
		std::string uniformName(uniformNameBuffer.data(), nameLength);

		// Check if the uniform value is an array
		size_t arraySize = 0;
		bool uniformIsArray = (nameLength > 3) && (uniformName[nameLength - 3] == '[') && (uniformName[nameLength - 2] == '0') && (uniformName[nameLength - 1] == ']');
		if (uniformIsArray)
		{
			arraySize = (uint32_t)dataSizeInBytes;
		}

		// Retrieve the location of the uniform
		GLint location = glGetUniformLocation(_programID, uniformNameBuffer.data());
		if (location < 0)
		{
			// If we can't locate this uniform variable, skip it. This will occur when we encounter uniforms which are
			// part of a uniform buffer. These entries are returned by glGetActiveUniform, but do not have a location as
			// they are not individually addressable as global uniforms. As we expect this case, we silently skip these
			// uniforms here.
			continue;
		}

		// If this uniform is an image without a combined sampler, add it to the list of known textures, and advance to
		// the next uniform variable. Note that GL_IMAGE_2D_RECT / GL_INT_IMAGE_2D_RECT / GL_UNSIGNED_INT_IMAGE_2D_RECT
		// are not supported, as "rect" images/textures are OpenGL specific, with no equivalent concept on Direct3D,
		// Vulkan or Metal. They're also of questionable use in modern OpenGL, as texelFetch can be used on a sampler
		// type to get integer addressing. They're mostly a legacy feature.
#ifdef GL_VERSION_4_2
		if ((nativeDataType == GL_IMAGE_1D) || (nativeDataType == GL_INT_IMAGE_1D) || (nativeDataType == GL_UNSIGNED_INT_IMAGE_1D) || (nativeDataType == GL_IMAGE_1D_ARRAY) || (nativeDataType == GL_INT_IMAGE_1D_ARRAY) || (nativeDataType == GL_UNSIGNED_INT_IMAGE_1D_ARRAY) || (nativeDataType == GL_IMAGE_2D) || (nativeDataType == GL_INT_IMAGE_2D) || (nativeDataType == GL_UNSIGNED_INT_IMAGE_2D) || (nativeDataType == GL_IMAGE_2D_ARRAY) || (nativeDataType == GL_INT_IMAGE_2D_ARRAY) || (nativeDataType == GL_UNSIGNED_INT_IMAGE_2D_ARRAY) || (nativeDataType == GL_IMAGE_2D_MULTISAMPLE) || (nativeDataType == GL_INT_IMAGE_2D_MULTISAMPLE) || (nativeDataType == GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE) || (nativeDataType == GL_IMAGE_2D_MULTISAMPLE_ARRAY) || (nativeDataType == GL_INT_IMAGE_2D_MULTISAMPLE_ARRAY) || (nativeDataType == GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY) || (nativeDataType == GL_IMAGE_3D) || (nativeDataType == GL_INT_IMAGE_3D) || (nativeDataType == GL_UNSIGNED_INT_IMAGE_3D) || (nativeDataType == GL_IMAGE_CUBE) || (nativeDataType == GL_INT_IMAGE_CUBE) || (nativeDataType == GL_UNSIGNED_INT_IMAGE_CUBE) || (nativeDataType == GL_IMAGE_CUBE_MAP_ARRAY) || (nativeDataType == GL_INT_IMAGE_CUBE_MAP_ARRAY) || (nativeDataType == GL_UNSIGNED_INT_IMAGE_CUBE_MAP_ARRAY))
		{
			// We've only got the location number for our buffer so far. We now need to retrieve the binding number in
			// order to determine the correct binding point to use for it. The (extremely poorly documented!!!) way to
			// do that is by calling glGetUniformiv on the uniform location.
			GLint binding;
			glGetUniformiv(_programID, location, &binding);

			// Add this image to the list of known textures
			_textureNameToID.insert(std::make_pair(uniformName, binding));
			continue;
		}
#endif

		// If this uniform is a texture sampler, add it to the list of known textures, and advance to the next uniform
		// variable. Note that GL_SAMPLER_2D_RECT / GL_INT_SAMPLER_2D_RECT / GL_UNSIGNED_INT_SAMPLER_2D_RECT are not
		// supported, as "rect" images/textures are OpenGL specific, with no equivalent concept on Direct3D, Vulkan or
		// Metal. They're also of questionable use in modern OpenGL, as texelFetch can be used on a sampler type to get
		// integer addressing. They're mostly a legacy feature.
		if ((nativeDataType == GL_SAMPLER_1D) || (nativeDataType == GL_INT_SAMPLER_1D) || (nativeDataType == GL_UNSIGNED_INT_SAMPLER_1D) || (nativeDataType == GL_SAMPLER_1D_ARRAY) || (nativeDataType == GL_INT_SAMPLER_1D_ARRAY) || (nativeDataType == GL_UNSIGNED_INT_SAMPLER_1D_ARRAY) || (nativeDataType == GL_SAMPLER_2D) || (nativeDataType == GL_INT_SAMPLER_2D) || (nativeDataType == GL_UNSIGNED_INT_SAMPLER_2D) || (nativeDataType == GL_SAMPLER_2D_ARRAY) || (nativeDataType == GL_INT_SAMPLER_2D_ARRAY) || (nativeDataType == GL_UNSIGNED_INT_SAMPLER_2D_ARRAY) || (nativeDataType == GL_SAMPLER_3D) || (nativeDataType == GL_INT_SAMPLER_3D) || (nativeDataType == GL_UNSIGNED_INT_SAMPLER_3D) || (nativeDataType == GL_SAMPLER_CUBE) || (nativeDataType == GL_INT_SAMPLER_CUBE) || (nativeDataType == GL_UNSIGNED_INT_SAMPLER_CUBE)
#ifdef GL_VERSION_4_0
		    || (nativeDataType == GL_SAMPLER_CUBE_MAP_ARRAY) || (nativeDataType == GL_INT_SAMPLER_CUBE_MAP_ARRAY) || (nativeDataType == GL_UNSIGNED_INT_SAMPLER_CUBE_MAP_ARRAY)
#endif
		)
		{
			// We've only got the location number for our sampler so far. We now need to retrieve the binding number in
			// order to determine the correct binding point to use for it. The (extremely poorly documented!!!) way to
			// do that is by calling glGetUniformiv on the uniform location in OpenGL 4.2 and above, to retrieve the
			// initial binding number from the layout qualifier in the shader. On earlier OpenGL versions, there are no
			// default bindings, so we pick the texture unit numbers ourselves.
			GLint binding;
#ifdef GL_VERSION_4_2
			glGetUniformiv(_programID, location, &binding);
#else
			binding = (GLint)_textureNameToID.size();
#endif

			// Add this texture sampler to the list of known texture samplers
			_textureSamplerAssociations.emplace_back(location, binding);
			_textureNameToID.insert(std::make_pair(uniformName, binding));
			continue;
		}

		// If this uniform is a texel array, add it to the list of known resource buffers, and advance to the next
		// uniform variable.
		if ((nativeDataType == GL_SAMPLER_BUFFER) || (nativeDataType == GL_INT_SAMPLER_BUFFER) || (nativeDataType == GL_UNSIGNED_INT_SAMPLER_BUFFER) || (nativeDataType == GL_IMAGE_BUFFER) || (nativeDataType == GL_INT_IMAGE_BUFFER) || (nativeDataType == GL_UNSIGNED_INT_IMAGE_BUFFER))
		{
			// We've only got the location number for our buffer so far. We now need to retrieve the binding number in
			// order to determine the correct binding point to use for it. The (extremely poorly documented!!!) way to
			// do that is by calling glGetUniformiv on the uniform location.
			GLint binding;
			glGetUniformiv(_programID, location, &binding);

			// Add this texel array to the list of known resource buffers
			ResourceBufferInfo bufferInfo{};
			bufferInfo.id = (ResourceArrayId)_resourceBufferList.size();
			bufferInfo.bindingPoint = binding;
			bufferInfo.name = uniformName;
			bufferInfo.writeableResource = (nativeDataType == GL_IMAGE_BUFFER) || (nativeDataType == GL_INT_IMAGE_BUFFER) || (nativeDataType == GL_UNSIGNED_INT_IMAGE_BUFFER);
			bufferInfo.texelArrayTextureUnitNo = binding;
			_resourceBufferList.push_back(bufferInfo);
			_resourceBufferNameToID[bufferInfo.name] = (GLint)bufferInfo.id;
			continue;
		}

		// Decode the data type of the uniform
		IStateBufferLayout::DataType dataType;
		uint32_t elementWidth;
		uint32_t elementHeight;
		if (!StateBufferEntryNativeTypeToBufferLayoutType(nativeDataType, dataType, elementWidth, elementHeight))
		{
			_log->Debug("Skipping global uniform {0} with name \"{1}\" as the data type {2} isn't supported", i, uniformName, nativeDataType);
			continue;
		}

		// Add this uniform to the list of known global uniforms
		GlobalUniformInfo& uniformInfo = _globalUniforms.emplace_back();
		uniformInfo.name = uniformName;
		uniformInfo.location = location;
		uniformInfo.nativeDataType = nativeDataType;
		uniformInfo.dataType = dataType;
		uniformInfo.elementWidth = elementWidth;
		uniformInfo.elementHeight = elementHeight;
		uniformInfo.arraySize = arraySize;
	}

	// Merge our individual global uniform values together to reconstruct parent arrays
	for (size_t uniformIndex = 0; uniformIndex < _globalUniforms.size(); ++uniformIndex)
	{
		// If this uniform isn't an array uniform, pass it through as-is, and advance to the next entry.
		auto& uniformInfo = _globalUniforms[uniformIndex];
		auto openBracePos = uniformInfo.name.find('[', 0);
		if (openBracePos == std::string::npos)
		{
			_uniformNameToMergedIndex.insert(std::make_pair(uniformInfo.name, _mergedGlobalUniforms.size()));
			MergedGlobalUniformInfo& mergedUniformInfo = _mergedGlobalUniforms.emplace_back();
			mergedUniformInfo.name = uniformInfo.name;
			mergedUniformInfo.underlyingUniformIndices.push_back(uniformIndex);
			mergedUniformInfo.underlyingUniformLocationsAsFlattenedArray.push_back(uniformInfo.location);
			continue;
		}

		// Build the merged name and array indices for this array uniform
		std::string mergedUniformName;
		std::vector<size_t> arraySizes;
		bool failed = false;
		size_t currentArrayLevel = 0;
		std::string::size_type lastNameStringCopyPos = 0;
		while (!failed && (openBracePos != std::string::npos))
		{
			// Find the closing brace
			auto closeBracePos = uniformInfo.name.find(']', openBracePos + 1);
			if (closeBracePos == std::string::npos)
			{
				_log->Warning("Skipping global uniform {0} with name \"{1}\" as we encountered mismatched brackets", uniformIndex, mergedUniformName);
				failed = true;
				continue;
			}

			// Append the next set of string data into the merged uniform name
			mergedUniformName += uniformInfo.name.substr(lastNameStringCopyPos, (openBracePos + 1) - lastNameStringCopyPos);
			lastNameStringCopyPos = closeBracePos;

			// Convert the base 10 array index string to an integer
			auto arrayIndex = (size_t)std::strtol(uniformInfo.name.c_str() + openBracePos + 1, nullptr, 10);

			// Increase the array size of this dimension to take into account the supplied array index
			uniformInfo.arrayIndices.resize(currentArrayLevel + 1);
			uniformInfo.arrayIndices[currentArrayLevel] = arrayIndex;
			arraySizes.resize(currentArrayLevel + 1);
			arraySizes[currentArrayLevel] = arrayIndex + 1;

			// Advance to the next array level
			++currentArrayLevel;
			openBracePos = uniformInfo.name.find('[', closeBracePos + 1);
		}
		if (failed)
		{
			continue;
		}
		mergedUniformName += uniformInfo.name.substr(lastNameStringCopyPos);

		// If this uniform was an array itself, we will get one entry for it of the form "someUniform[0]", with a full array
		// size already decoded. In this case, we need to override the last array size with the defined array size retrieved
		// earlier.
		if (uniformInfo.arraySize != 0)
		{
			arraySizes.back() = uniformInfo.arraySize;
		}

		// Combine this array info with the existing merged uniform entry, or create a new one if none exists.
		auto uniformNameToMergedIndexIterator = _uniformNameToMergedIndex.find(mergedUniformName);
		if (uniformNameToMergedIndexIterator == _uniformNameToMergedIndex.end())
		{
			_uniformNameToMergedIndex.insert(std::make_pair(mergedUniformName, _mergedGlobalUniforms.size()));
			MergedGlobalUniformInfo& mergedUniformInfo = _mergedGlobalUniforms.emplace_back();
			mergedUniformInfo.name = std::move(mergedUniformName);
			mergedUniformInfo.arraySizes = std::move(arraySizes);
			mergedUniformInfo.underlyingUniformIndices.push_back(uniformIndex);
		}
		else
		{
			MergedGlobalUniformInfo& mergedUniformInfo = _mergedGlobalUniforms[uniformNameToMergedIndexIterator->second];
			for (size_t i = 0; i < arraySizes.size(); ++i)
			{
				mergedUniformInfo.arraySizes[i] = std::max(arraySizes[i], mergedUniformInfo.arraySizes[i]);
			}
			mergedUniformInfo.underlyingUniformIndices.push_back(uniformIndex);
		}
	}

	// Due to elimination of unused uniform array entries, our array sizes may not be correct. In this case, we need to
	// pull reflection information from SPIR-V to figure out the array dimensions. We need the true array dimensions so
	// we can perform flattening below, otherwise flattened array locations may be incorrect. Note that if only raw GLSL
	// was supplied, and it was not converted to SPIR-V during loading, we will be required to convert the GLSL into
	// SPIR-V at this time in order to get the reflection information, even though we won't use the SPIR-V result here.
	bool foundGlobalBufferReflectionInfo = false;
	IShaderCode::Resource* globalBufferReflectionInfo = nullptr;
	for (auto& mergedUniformInfo : _mergedGlobalUniforms)
	{
		// If this merged uniform doesn't have multidimensional arrays, skip it.
		if (mergedUniformInfo.name.find("[][]") == std::string::npos)
		{
			continue;
		}

		// Parse GLSL shader code into SPIR-V here if required
		if (!foundGlobalBufferReflectionInfo && _shaderResourceInfoForGlobalBufferReflection.empty())
		{
			int baseBindingNoTemp = 0;
			int baseBindingNoTemp2 = 0;
			std::vector<IShaderCode::Resource> shaderResourceInfoTemp;
			std::vector<IShaderCode::Resource> shaderResourceInfoTemp2;
			for (auto& shaderBlockEntry : _shaderBlocks)
			{
				// Map the shader stage value
				ShaderBlockInfo& shaderBlock = shaderBlockEntry.second;
				IShaderCode::Stage shaderStage = IShaderCode::Stage::Vertex;
				if (shaderBlock.stage == ShaderStage::Vertex)
				{
					shaderStage = IShaderCode::Stage::Vertex;
				}
				else if (shaderBlock.stage == ShaderStage::Geometry)
				{
					shaderStage = IShaderCode::Stage::Geometry;
				}
				else if (shaderBlock.stage == ShaderStage::Fragment)
				{
					shaderStage = IShaderCode::Stage::Fragment;
				}
#ifdef GL_VERSION_4_3
				else if (shaderBlock.stage == ShaderStage::Compute)
				{
					shaderStage = IShaderCode::Stage::Compute;
				}
#endif

				// Set the target environment
				auto targetEnvironment = IShaderCode::Environment::OpenGL_33;
#ifdef GL_VERSION_4_3
				targetEnvironment = IShaderCode::Environment::OpenGL_43;
#endif

				// If we don't have a cached GLSL shader, we must have been initialized with a SPIRV shader directly,
				// otherwise we would have run through GLSL conversion as part of the loading step. In this case, we
				// need to convert the SPIRV to HLSL.
				if (shaderBlock.codeAsGLSLCached.empty())
				{
					auto shaderCode = IShaderCode::Create(_log->GetLoggerChildScope("ShaderSupport"));
					if (!shaderCode->LoadCode(IShaderCode::Language::SPIRV, shaderStage, targetEnvironment, targetEnvironment, shaderBlock.code.data(), shaderBlock.code.size(), IShaderCode::StandardShaderEntryPointName, baseBindingNoTemp, shaderResourceInfoTemp))
					{
						_log->Warning("Failed to parse shader code for stage {0} for reflection", shaderStage);
						break;
					}
					if (!shaderCode->ExportCodeAsHLSL(shaderBlock.codeAsHLSLCached, 5, 1))
					{
						_log->Warning("Failed to parse shader code for stage {0} for reflection", shaderStage);
						break;
					}
				}

				// If we don't have a cached HLSL shader, we must have a cached GLSL shader, as SPIRV has been dealt
				// with above, and GLSL would have been generated as part of conversion or must have been provided
				// directly. In this case, we need to convert the GLSL to HLSL, so we can reflect on it.
				if (shaderBlock.codeAsHLSLCached.empty())
				{
					auto shaderCode = IShaderCode::Create(_log->GetLoggerChildScope("ShaderSupport"));
					if (!shaderCode->LoadCode(IShaderCode::Language::GLSL, shaderStage, targetEnvironment, targetEnvironment, reinterpret_cast<const uint8_t*>(shaderBlock.codeAsGLSLCached.data()), shaderBlock.codeAsGLSLCached.size(), IShaderCode::StandardShaderEntryPointName, baseBindingNoTemp2, shaderResourceInfoTemp2))
					{
						_log->Warning("Failed to parse shader code for stage {0} for reflection", shaderStage);
						break;
					}
					if (!shaderCode->ExportCodeAsHLSL(shaderBlock.codeAsHLSLCached, 5, 1))
					{
						_log->Warning("Failed to parse shader code for stage {0} for reflection", shaderStage);
						break;
					}
				}

				// Since we must now have HLSL code, reflect on it now to build compatible shader resource info we can
				// inspect.
				auto shaderCode = IShaderCode::Create(_log->GetLoggerChildScope("ShaderSupport"));
				if (!shaderCode->LoadCode(IShaderCode::Language::HLSL, shaderStage, IShaderCode::Environment::General, targetEnvironment, reinterpret_cast<const uint8_t*>(shaderBlock.codeAsHLSLCached.data()), shaderBlock.codeAsHLSLCached.size(), IShaderCode::StandardShaderEntryPointName, _baseBindingNoForGlobalBufferReflection, _shaderResourceInfoForGlobalBufferReflection))
				{
					_log->Warning("Failed to parse shader code for stage {0} for reflection", shaderStage);
					break;
				}
			}
		}

		// Find the target resource for the global uniform buffer
		if (!foundGlobalBufferReflectionInfo)
		{
			size_t targetResourceIndex = 0;
			bool foundResource = false;
			for (size_t resourceIndex = 0; resourceIndex < _shaderResourceInfoForGlobalBufferReflection.size(); ++resourceIndex)
			{
				if (_shaderResourceInfoForGlobalBufferReflection[resourceIndex].name != IShaderCode::GlobalConstantBufferName)
				{
					continue;
				}
				targetResourceIndex = resourceIndex;
				foundResource = true;
				break;
			}
			if (foundResource)
			{
				globalBufferReflectionInfo = &_shaderResourceInfoForGlobalBufferReflection[targetResourceIndex];
				foundGlobalBufferReflectionInfo = true;
			}
		}
		if (!foundGlobalBufferReflectionInfo)
		{
			_log->Warning("Unable to verify array sizes for global uniform \"{0}\" as no resource info was available for reflection.", mergedUniformInfo.name);
			break;
		}

		// Break the uniform name into its path components and array dimension counts
		std::vector<std::string> variableNames;
		std::vector<std::size_t> arrayDimensionCounts;
		std::string nextVariableName;
		std::size_t nextArrayDimensionCount = 0;
		for (std::size_t i = 0; i < mergedUniformInfo.name.size(); ++i)
		{
			char nextChar = mergedUniformInfo.name[i];
			if (nextChar == '.')
			{
				variableNames.push_back(nextVariableName);
				arrayDimensionCounts.push_back(nextArrayDimensionCount);
				nextVariableName.clear();
				nextArrayDimensionCount = 0;
			}
			else if (nextChar == '[')
			{
				++nextArrayDimensionCount;
				++i;
			}
			else
			{
				nextVariableName += nextChar;
			}
		}
		variableNames.push_back(nextVariableName);
		arrayDimensionCounts.push_back(nextArrayDimensionCount);

		// Walk the resource hierarchy and find the true array dimensions
		size_t currentOverallArrayIndex = 0;
		IShaderCode::Resource* targetResource = globalBufferReflectionInfo;
		for (size_t pathElementIndex = 0; pathElementIndex < variableNames.size(); ++pathElementIndex)
		{
			// Find the target resource for the next variable name
			size_t targetFieldIndex = 0;
			bool foundField = false;
			for (size_t fieldIndex = 0; fieldIndex < targetResource->fields.size(); ++fieldIndex)
			{
				if (targetResource->fields[fieldIndex].name != variableNames[pathElementIndex])
				{
					continue;
				}
				targetFieldIndex = fieldIndex;
				foundField = true;
				break;
			}
			if (!foundField)
			{
				_log->Warning("Unable to verify array sizes for global uniform \"{0}\" as the element \"{1}\" (index {2}) couldn't be found during shader reflection", mergedUniformInfo.name, variableNames[pathElementIndex], pathElementIndex);
				continue;
			}

			// Validate the correct number of array dimensions are reported
			targetResource = &targetResource->fields[targetFieldIndex];
			if (targetResource->arraySizes.size() != arrayDimensionCounts[pathElementIndex])
			{
				_log->Warning("Unable to verify array sizes for global uniform \"{0}\" as the element \"{1}\" (index {2}) reported {3} number of array dimensions, while reflection reported {4}.", mergedUniformInfo.name, variableNames[pathElementIndex], pathElementIndex, arrayDimensionCounts[pathElementIndex], targetResource->arraySizes.size());
				continue;
			}

			// Update our array sizes to match. The SPIR-V reflection array size should be greater than or equal to the reported
			// array size through the OpenGL reflection API.
			for (size_t i = 0; i < targetResource->arraySizes.size(); ++i)
			{
				mergedUniformInfo.arraySizes[currentOverallArrayIndex + i] = (size_t)targetResource->arraySizes[i];
			}
			currentOverallArrayIndex += targetResource->arraySizes.size();
		}
	}

	// Now that array uniforms have been merged, generate the flattened location indices for each merged entry.
	for (auto& mergedUniformInfo : _mergedGlobalUniforms)
	{
		// Resize the flattened array, and initialize all members to an invalid location. Note that this is essential,
		// as under OpenGL, non-leaf array entries are "sparse", meaning not all indices in parent arrays may be
		// present.
		size_t flattenedArraySize = 1;
		for (size_t arraySize : mergedUniformInfo.arraySizes)
		{
			flattenedArraySize *= arraySize;
		}
		mergedUniformInfo.underlyingUniformLocationsAsFlattenedArray.resize(flattenedArraySize, (GLint)-1);

		// Populate the flattened array with each valid state value location
		for (auto uniformIndex : mergedUniformInfo.underlyingUniformIndices)
		{
			const auto& uniformInfo = _globalUniforms[uniformIndex];
			size_t flattenedIndex = 0;
			size_t flattenedArrayOffsetMultiplier = 1;
			size_t currentArrayIndex = uniformInfo.arrayIndices.size();
			while (currentArrayIndex-- > 0)
			{
				flattenedIndex += uniformInfo.arrayIndices[currentArrayIndex] * flattenedArrayOffsetMultiplier;
				flattenedArrayOffsetMultiplier *= mergedUniformInfo.arraySizes[currentArrayIndex];
			}
			size_t arraySize = (uniformInfo.arraySize != 0 ? uniformInfo.arraySize : 1);
			for (size_t i = 0; i < arraySize; ++i)
			{
				mergedUniformInfo.underlyingUniformLocationsAsFlattenedArray[flattenedIndex + i] = uniformInfo.location + (GLint)i;
			}
		}
	}

	// We now have all our global uniforms merged together, with flattened uniform locations generated, making the
	// actual uniform string name values no longer relevant. We now flatten multi-dimensional arrays to
	// single-dimensional arrays. While OpenGL fully supports multi-dimensional arrays in uniform buffers, Direct3D does
	// not, and it flattens multi-dimensional arrays to single-dimensional arrays in the reflection API. For consistency
	// across renderers, we opt to do the same thing here for OpenGL. To perform the flattening step, we identify
	// multi-dimensional arrays, multiply the array sizes together, and flatten the array notation in the variable name.
	// Actual writes to array uniforms will use the flattened uniform location array, which doesn't need to change in
	// response to this step.
	for (size_t mergedUniformIndex = 0; mergedUniformIndex < _mergedGlobalUniforms.size(); ++mergedUniformIndex)
	{
		// If this merged uniform doesn't have multidimensional arrays, skip it. Since we expect this to be a rare case,
		// this is a performance optimization to save additional work.
		MergedGlobalUniformInfo& mergedUniformInfo = _mergedGlobalUniforms[mergedUniformIndex];
		if (mergedUniformInfo.name.find("[][]") == std::string::npos)
		{
			continue;
		}

		// Find the first array notation in the uniform name
		std::string arrayNotationString = "[]";
		size_t previousArrayStringSearchPos = mergedUniformInfo.name.find(arrayNotationString);

		// Build a new name and set of array sizes for the uniform, with multidimensional arrays flattened.
		std::unordered_set<size_t> mergedArrayIndices;
		std::string flattenedName = mergedUniformInfo.name.substr(0, previousArrayStringSearchPos + arrayNotationString.size());
		std::vector<size_t> flattenedArraySizes;
		size_t currentArrayIndex = 0;
		flattenedArraySizes.push_back(mergedUniformInfo.arraySizes[currentArrayIndex++]);
		size_t arrayStringSearchPos = mergedUniformInfo.name.find(arrayNotationString, previousArrayStringSearchPos + arrayNotationString.size());
		while (arrayStringSearchPos != std::string::npos)
		{
			if (arrayStringSearchPos != (previousArrayStringSearchPos + arrayNotationString.size()))
			{
				// If the next array notation doesn't immediately follow the last one, pass it through without changes.
				flattenedName += mergedUniformInfo.name.substr(previousArrayStringSearchPos + arrayNotationString.size(), arrayStringSearchPos - (previousArrayStringSearchPos + arrayNotationString.size()));
				flattenedArraySizes.push_back(mergedUniformInfo.arraySizes[currentArrayIndex]);
			}
			else
			{
				// Since thie array notation immediately follows the last one, merge the array sizes together.
				flattenedArraySizes.back() *= mergedUniformInfo.arraySizes[currentArrayIndex];
				mergedArrayIndices.insert(currentArrayIndex);
			}
			++currentArrayIndex;
			previousArrayStringSearchPos = arrayStringSearchPos;
			arrayStringSearchPos = mergedUniformInfo.name.find(arrayNotationString, previousArrayStringSearchPos + arrayNotationString.size());
		}

		// Build a flattened set of array indices for each underlying global uniform in the merged uniform entry. We
		// need these to set default values below.
		for (size_t underlyingUniformIndex : mergedUniformInfo.underlyingUniformIndices)
		{
			GlobalUniformInfo& underlyingUniform = _globalUniforms[underlyingUniformIndex];
			size_t flattenedIndex = 0;
			size_t flattenedArrayOffsetMultiplier = 1;
			currentArrayIndex = underlyingUniform.arrayIndices.size();
			while (currentArrayIndex-- > 0)
			{
				if (mergedArrayIndices.find(currentArrayIndex) != mergedArrayIndices.end())
				{
					flattenedIndex += underlyingUniform.arrayIndices[currentArrayIndex] * flattenedArrayOffsetMultiplier;
					flattenedArrayOffsetMultiplier *= mergedUniformInfo.arraySizes[currentArrayIndex];
				}
				else
				{
					flattenedIndex += underlyingUniform.arrayIndices[currentArrayIndex];
					underlyingUniform.flattenedArrayIndices.push_back(flattenedIndex);
					flattenedIndex = 0;
					flattenedArrayOffsetMultiplier = 1;
				}
			}
		}

		// Update the merged uniform name with the flattened name in the global uniform index
		_uniformNameToMergedIndex.erase(mergedUniformInfo.name);
		_uniformNameToMergedIndex.insert(std::make_pair(flattenedName, mergedUniformIndex));

		// Assign the flattened uniform name and array sizes back to the merged uniform. Note that we need to do this after we
		// generate flattened indices for the underlying uniform entries above.
		mergedUniformInfo.name = std::move(flattenedName);
		mergedUniformInfo.arraySizes = std::move(flattenedArraySizes);
	}

	// Retrieve default values for all our global uniforms
	for (size_t mergedUniformIndex = 0; mergedUniformIndex < _mergedGlobalUniforms.size(); ++mergedUniformIndex)
	{
		const MergedGlobalUniformInfo& mergedUniformInfo = _mergedGlobalUniforms[mergedUniformIndex];
		for (size_t globalUniformIndex : mergedUniformInfo.underlyingUniformIndices)
		{
			// Add a set of initial values to the uniform info
			GlobalUniformInfo& uniformInfo = _globalUniforms[globalUniformIndex];
			auto arrayIndices = (!uniformInfo.flattenedArrayIndices.empty() ? uniformInfo.flattenedArrayIndices : uniformInfo.arrayIndices);
			size_t arraySize = (uniformInfo.arraySize != 0 ? uniformInfo.arraySize : 1);
			for (size_t arrayEntryNo = 0; arrayEntryNo < arraySize; ++arrayEntryNo)
			{
				// Retrieve the default value for the uniform
				if (uniformInfo.arraySize != 0)
				{
					arrayIndices.back() = arrayEntryNo;
				}
				auto defaultValue = GetGlobalUniformCurrentValue(uniformInfo.nativeDataType, uniformInfo.location + (GLint)arrayEntryNo, (StateValueId)mergedUniformIndex, arrayIndices.data(), arrayIndices.size());
				if (defaultValue == nullptr)
				{
					_log->Warning("Skipping global uniform {0} with name \"{1}\" as the data type {2} isn't supported", globalUniformIndex, uniformInfo.name, uniformInfo.nativeDataType);
					break;
				}

				// Add this default value to the list of defined default values
				uniformInfo.defaultValues.push_back(std::move(defaultValue));
			}

			// Add the default values for this uniform to the default list of state values for this program
			for (const std::unique_ptr<IStateValueInfo>& defaultValueEntry : uniformInfo.defaultValues)
			{
				_uniformDefaultStateList.push_back(defaultValueEntry.get());
			}
		}
	}

	// Push the initial buffer state to the stack
	_stateBufferStack.push_back(&_uniformDefaultStateList);
	_stateBufferStackEntries = 1;

	// Retrieve the maximum number of supported user clip planes
	glGetIntegerv(GL_MAX_CLIP_DISTANCES, &_maxUserClipPlaneCount);

	// Determine the number of user clip planes referenced in the shader. We manually parse the GLSL here with a regex,
	// which is obviously not ideal. Unfortunately the OpenGL shader reflection API doesn't seem to provide what we need to
	// interrogate the presence and size of the gl_ClipDistance array. We could use the SPIRV Cross reflection capabilities
	// within our ShaderSupport library to determine this information in a more robust way, however that hardly seems worth
	// the effort here, especially when currently raw GLSL may be provided with currently no need to run the code through
	// ShaderSupport at all. If issues arise in the future from this code here, we should resort to fully parsing the code
	// and extracting this information via the ShaderSupport library.
	std::regex clipDistanceSearch(R"(out\s+.*?gl_ClipDistance\s*\[\s*([0-9]+)\s*\])");
	_userClipPlaneCount = 0;
	auto vertexShaderIterator = _shaderBlocks.find(ShaderStage::Vertex);
	if (vertexShaderIterator != _shaderBlocks.end())
	{
		std::string shaderCode(vertexShaderIterator->second.code.begin(), vertexShaderIterator->second.code.end());
		std::replace(shaderCode.begin(), shaderCode.end(), '\n', ' ');
		std::replace(shaderCode.begin(), shaderCode.end(), '\r', ' ');
		std::smatch regexMatch;
		if (std::regex_search(shaderCode, regexMatch, clipDistanceSearch))
		{
			_userClipPlaneCount = std::max(_userClipPlaneCount, std::stoi(regexMatch.begin()[1]));
		}
	}
	auto geometryShaderIterator = _shaderBlocks.find(ShaderStage::Geometry);
	if (geometryShaderIterator != _shaderBlocks.end())
	{
		std::string shaderCode(geometryShaderIterator->second.code.begin(), geometryShaderIterator->second.code.end());
		std::replace(shaderCode.begin(), shaderCode.end(), '\n', ' ');
		std::replace(shaderCode.begin(), shaderCode.end(), '\r', ' ');
		std::smatch regexMatch;
		if (std::regex_search(shaderCode, regexMatch, clipDistanceSearch))
		{
			_userClipPlaneCount = std::max(_userClipPlaneCount, std::stoi(regexMatch.begin()[1]));
		}
	}

	// Clean up the compiled shader modules
	for (int shaderID : shaderIDs)
	{
		glDeleteShader(shaderID);
	}
	CheckGLError(_log);
	return true;
}

//----------------------------------------------------------------------------------------
void OpenGLShaderProgram::ReleaseMemory()
{
	if (_shaderCompiled)
	{
		glDeleteProgram(_programID);
		_shaderCompiled = false;
	}
}

//----------------------------------------------------------------------------------------
// Shader input methods
//----------------------------------------------------------------------------------------
bool OpenGLShaderProgram::VertexAttributeExists(const Marshal::In<std::string>& name) const
{
	return (_attributeNameToID.find(name) != _attributeNameToID.end());
}

//----------------------------------------------------------------------------------------
bool OpenGLShaderProgram::StateValueExists(const Marshal::In<std::string>& name) const
{
	return (_uniformNameToMergedIndex.find(name) != _uniformNameToMergedIndex.end());
}

//----------------------------------------------------------------------------------------
bool OpenGLShaderProgram::TextureExists(const Marshal::In<std::string>& name) const
{
	return (_textureNameToID.find(name) != _textureNameToID.end());
}

//----------------------------------------------------------------------------------------
bool OpenGLShaderProgram::SamplerExists(const Marshal::In<std::string>& name) const
{
	return false;
}

//----------------------------------------------------------------------------------------
bool OpenGLShaderProgram::StateBufferExists(const Marshal::In<std::string>& name) const
{
	return (_stateBufferNameToID.find(name) != _stateBufferNameToID.end());
}

//----------------------------------------------------------------------------------------
bool OpenGLShaderProgram::ResourceArrayExists(const Marshal::In<std::string>& name) const
{
	return (_resourceBufferNameToID.find(name) != _resourceBufferNameToID.end());
}

//----------------------------------------------------------------------------------------
VertexAttributeId OpenGLShaderProgram::GetVertexAttributeId(const Marshal::In<std::string>& name) const
{
	std::string nameResolved = name.Get();
	auto attributeNameToIDIterator = _attributeNameToID.find(nameResolved);
	if (attributeNameToIDIterator == _attributeNameToID.end())
	{
		_log->Warning("Failed to locate vertex attribute with name \"{0}\"", nameResolved);
		return VertexAttributeId::Null;
	}
	return (VertexAttributeId)attributeNameToIDIterator->second;
}

//----------------------------------------------------------------------------------------
StateValueId OpenGLShaderProgram::GetStateValueId(const Marshal::In<std::string>& name) const
{
	std::string nameResolved = name.Get();
	auto uniformNameToIDIterator = _uniformNameToMergedIndex.find(nameResolved);
	if (uniformNameToIDIterator == _uniformNameToMergedIndex.end())
	{
		_log->Warning("Failed to locate shader uniform with name \"{0}\"", name.Get());
		return StateValueId::Null;
	}
	return (StateValueId)uniformNameToIDIterator->second;
}

//----------------------------------------------------------------------------------------
TextureId OpenGLShaderProgram::GetTextureId(const Marshal::In<std::string>& name) const
{
	std::string nameResolved = name.Get();
	auto textureNameToIDIterator = _textureNameToID.find(nameResolved);
	if (textureNameToIDIterator == _textureNameToID.end())
	{
		_log->Warning("Failed to locate shader texture with name \"{0}\"", nameResolved);
		return TextureId::Null;
	}
	return (TextureId)textureNameToIDIterator->second;
}

//----------------------------------------------------------------------------------------
SamplerId OpenGLShaderProgram::GetSamplerId(const Marshal::In<std::string>& name) const
{
	std::string nameResolved = name.Get();
	_log->Warning("Attempted to locate sampler with name \"{0}\", but OpenGL only supports combined image samplers.", nameResolved);
	return SamplerId::Null;
}

//----------------------------------------------------------------------------------------
StateBufferId OpenGLShaderProgram::GetStateBufferId(const Marshal::In<std::string>& name) const
{
	std::string nameResolved = name.Get();
	auto stateBufferNameToIDIterator = _stateBufferNameToID.find(nameResolved);
	if (stateBufferNameToIDIterator == _stateBufferNameToID.end())
	{
		_log->Warning("Failed to locate state buffer with name \"{0}\"", nameResolved);
		return StateBufferId::Null;
	}
	return (StateBufferId)stateBufferNameToIDIterator->second;
}

//----------------------------------------------------------------------------------------
ResourceArrayId OpenGLShaderProgram::GetResourceArrayId(const Marshal::In<std::string>& name) const
{
	std::string nameResolved = name.Get();
	auto resourceBufferNameToIDIterator = _resourceBufferNameToID.find(nameResolved);
	if (resourceBufferNameToIDIterator == _resourceBufferNameToID.end())
	{
		_log->Warning("Failed to locate resource array with name \"{0}\"", nameResolved);
		return ResourceArrayId::Null;
	}
	return (ResourceArrayId)resourceBufferNameToIDIterator->second;
}

//----------------------------------------------------------------------------------------
bool OpenGLShaderProgram::GetUniformLocation(StateValueId stateId, const size_t* arrayIndices, size_t arrayIndexCount, GLint& location) const
{
	// Initialize the output location value
	location = -1;

	// Retrieve the target merged uniform info structure
	if (stateId == graphics::StateValueId::Null)
	{
		// We don't log an error here, as OpenGL will optimize away uniform values which it detects aren't used, giving
		// a state ID of -1. Since we anticipate this will be a fairly common occurrence, we ignore attempts to write to
		// these variables. A message will be recorded when the ID is retrieved.
		return false;
	}
	auto stateIdAsIndex = (size_t)stateId;
	if (stateIdAsIndex >= _mergedGlobalUniforms.size())
	{
		_log->Error("Attempted to modify state value with an invalid ID of {0}", stateId);
		return false;
	}
	const auto& mergedUniformInfo = _mergedGlobalUniforms[stateIdAsIndex];

	// Resolve the underlying target uniform index taking parent arrays into account
	if (mergedUniformInfo.arraySizes.size() != arrayIndexCount)
	{
		_log->Warning("Attempted to set state value {0} with {1} indices, when {2} indices are required.", mergedUniformInfo.name, arrayIndexCount, mergedUniformInfo.arraySizes.size());
		return false;
	}
	size_t flattenedIndex = 0;
	size_t flattenedArrayOffsetMultiplier = 1;
	size_t currentArrayIndex = arrayIndexCount;
	while (currentArrayIndex-- > 0)
	{
		if (mergedUniformInfo.arraySizes[currentArrayIndex] <= arrayIndices[currentArrayIndex])
		{
			_log->Warning("Attempted to set state value {0} with index {1} at array position {2}, when only {3} entries are present.", mergedUniformInfo.name, arrayIndices[currentArrayIndex], currentArrayIndex, mergedUniformInfo.arraySizes[currentArrayIndex]);
			return false;
		}
		flattenedIndex += arrayIndices[currentArrayIndex] * flattenedArrayOffsetMultiplier;
		flattenedArrayOffsetMultiplier *= mergedUniformInfo.arraySizes[currentArrayIndex];
	}

	// Return the location of the target uniform
	location = mergedUniformInfo.underlyingUniformLocationsAsFlattenedArray[flattenedIndex];
	return true;
}

//----------------------------------------------------------------------------------------
void OpenGLShaderProgram::GetBindPointForResourceArray(ResourceArrayId bufferId, GLuint& bufferBindingPoint, GLuint& counterBindingPoint, int& textureUnitNo, bool& writeable) const
{
	// Ensure a valid buffer ID has been provided
	if ((bufferId == ResourceArrayId::Null) || ((size_t)bufferId >= _resourceBufferList.size()))
	{
		_log->Error("GetBindingPointForResourceBuffer called with invalid ID {0}", bufferId);
		return;
	}

	// Retrieve the binding points for the buffer and associated counter buffer if present
	const auto& bufferInfo = _resourceBufferList[(size_t)bufferId];
	bufferBindingPoint = bufferInfo.bindingPoint;
	if (bufferInfo.hasCounterBuffer)
	{
		const auto& counterBufferInfo = _resourceBufferList[(size_t)bufferInfo.counterBufferId];
		counterBindingPoint = counterBufferInfo.bindingPoint;
	}
	else
	{
		counterBindingPoint = 0xFFFFFFFF;
	}
	textureUnitNo = bufferInfo.texelArrayTextureUnitNo;
	writeable = bufferInfo.writeableResource;
}

//----------------------------------------------------------------------------------------
void OpenGLShaderProgram::GetBindPointForStateBuffer(StateBufferId bufferId, GLuint& bufferBindingPoint) const
{
	// Ensure a valid buffer ID has been provided
	if ((bufferId == StateBufferId::Null) || ((size_t)bufferId >= _stateBufferBindPoints.size()))
	{
		_log->Error("GetBindPointForStateBuffer called with invalid ID {0}", bufferId);
		return;
	}

	// Retrieve the binding point for the buffer
	bufferBindingPoint = _stateBufferBindPoints[(size_t)bufferId];
}

//----------------------------------------------------------------------------------------
// State buffer methods
//----------------------------------------------------------------------------------------
void OpenGLShaderProgram::ReadStateBufferLayoutInfoFromShader(StateBufferId stateBufferId, StateBufferLayoutInfo& layoutInfo) const
{
	// Retrieve the number of uniforms in the target state buffer
	GLint uniformCount;
	glGetActiveUniformBlockiv(_programID, (int)stateBufferId, GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS, &uniformCount);

	// Retrieve the indices of each uniform in the buffer
	std::vector<GLint> uniformIndices((size_t)uniformCount);
	glGetActiveUniformBlockiv(_programID, (int)stateBufferId, GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES, uniformIndices.data());

	// Retrieve information on each uniform in the state buffer
	std::vector<GLint> uniformTypes(uniformIndices.size());
	std::vector<GLint> uniformNameLengths(uniformIndices.size());
	std::vector<GLint> uniformBlockIndexes(uniformIndices.size());
	std::vector<GLint> uniformOffsets(uniformIndices.size());
	std::vector<GLint> uniformArraySizes(uniformIndices.size());
	std::vector<GLint> uniformArrayStrides(uniformIndices.size());
	std::vector<GLint> uniformMatrixStrides(uniformIndices.size());
	std::vector<GLint> uniformMatrixIsRowMajors(uniformIndices.size());
	std::vector<std::string> uniformNames(uniformIndices.size());
	glGetActiveUniformsiv(_programID, (GLsizei)uniformIndices.size(), reinterpret_cast<const GLuint*>(uniformIndices.data()), GL_UNIFORM_TYPE, reinterpret_cast<GLint*>(uniformTypes.data()));
	glGetActiveUniformsiv(_programID, (GLsizei)uniformIndices.size(), reinterpret_cast<const GLuint*>(uniformIndices.data()), GL_UNIFORM_NAME_LENGTH, reinterpret_cast<GLint*>(uniformNameLengths.data()));
	glGetActiveUniformsiv(_programID, (GLsizei)uniformIndices.size(), reinterpret_cast<const GLuint*>(uniformIndices.data()), GL_UNIFORM_BLOCK_INDEX, reinterpret_cast<GLint*>(uniformBlockIndexes.data()));
	glGetActiveUniformsiv(_programID, (GLsizei)uniformIndices.size(), reinterpret_cast<const GLuint*>(uniformIndices.data()), GL_UNIFORM_OFFSET, reinterpret_cast<GLint*>(uniformOffsets.data()));
	glGetActiveUniformsiv(_programID, (GLsizei)uniformIndices.size(), reinterpret_cast<const GLuint*>(uniformIndices.data()), GL_UNIFORM_SIZE, reinterpret_cast<GLint*>(uniformArraySizes.data()));
	glGetActiveUniformsiv(_programID, (GLsizei)uniformIndices.size(), reinterpret_cast<const GLuint*>(uniformIndices.data()), GL_UNIFORM_ARRAY_STRIDE, reinterpret_cast<GLint*>(uniformArrayStrides.data()));
	glGetActiveUniformsiv(_programID, (GLsizei)uniformIndices.size(), reinterpret_cast<const GLuint*>(uniformIndices.data()), GL_UNIFORM_MATRIX_STRIDE, reinterpret_cast<GLint*>(uniformMatrixStrides.data()));
	glGetActiveUniformsiv(_programID, (GLsizei)uniformIndices.size(), reinterpret_cast<const GLuint*>(uniformIndices.data()), GL_UNIFORM_IS_ROW_MAJOR, reinterpret_cast<GLint*>(uniformMatrixIsRowMajors.data()));
	for (uint32_t i = 0; i < uniformIndices.size(); ++i)
	{
		std::string& uniformName = uniformNames[i];
		auto uniformNameLength = uniformNameLengths[i];
		uniformName.resize(uniformNameLength);
		GLsizei writtenLength;
		glGetActiveUniformName(_programID, uniformIndices[i], uniformNameLength, &writtenLength, reinterpret_cast<GLchar*>(uniformName.data()));
		uniformName.resize(writtenLength);
	}

	// If the uniform name starts with the buffer name, strip it off. This can happen when loading shaders from SPIRV.
	auto stringStartsWith = [](const std::string& str, const std::string& prefix) { return (str.size() >= prefix.size()) && (str.compare(0, prefix.size(), prefix) == 0); };
	const auto& stateBufferName = _stateBufferIDToName.find(stateBufferId)->second;
	std::string stateBufferNamePrefix = stateBufferName + ".";
	for (auto& uniformName : uniformNames)
	{
		if (stringStartsWith(uniformName, stateBufferNamePrefix))
		{
			uniformName = uniformName.substr(stateBufferNamePrefix.size());
		}
	}

	// Build a set of structures containing information on each uniform
	std::vector<StateBufferUniformInfo> uniforms;
	uniforms.reserve(uniformIndices.size());
	for (size_t underlyingUniformIndex = 0; underlyingUniformIndex < uniformIndices.size(); ++underlyingUniformIndex)
	{
		// Convert the uniform type information
		IStateBufferLayout::DataType type;
		uint32_t elementWidth;
		uint32_t elementHeight;
		if (!StateBufferEntryNativeTypeToBufferLayoutType(uniformTypes[underlyingUniformIndex], type, elementWidth, elementHeight))
		{
			_log->Error("State buffer with ID {0} contains an unsupported state value of type {1} when loading state buffer layout from program {2}", stateBufferId, uniformTypes[underlyingUniformIndex], _programID);
			continue;
		}

		// Add the uniform to the list of uniforms
		auto& uniform = uniforms.emplace_back();
		uniform.name = uniformNames[underlyingUniformIndex];
		uniform.dataType = type;
		uniform.elementWidth = elementWidth;
		uniform.elementHeight = elementHeight;
		uniform.nativeDataType = (GLenum)uniformTypes[underlyingUniformIndex];
		uniform.bufferOffset = (size_t)uniformOffsets[underlyingUniformIndex];
		uniform.arraySize = (size_t)uniformArraySizes[underlyingUniformIndex];
		uniform.arrayStride = (size_t)uniformArrayStrides[underlyingUniformIndex];
	}

	// Merge our individual uniform values together to reconstruct parent arrays
	std::vector<MergedStateBufferUniformInfo> mergedUniforms;
	std::unordered_map<std::string, size_t> uniformNameToMergedIndex;
	for (size_t uniformIndex = 0; uniformIndex < uniforms.size(); ++uniformIndex)
	{
		// If this uniform isn't an array uniform, pass it through as-is, and advance to the next entry.
		auto& uniform = uniforms[uniformIndex];
		auto openBracePos = uniform.name.find('[', 0);
		if (openBracePos == std::string::npos)
		{
			uniformNameToMergedIndex.insert(std::make_pair(uniform.name, mergedUniforms.size()));
			MergedStateBufferUniformInfo& mergedUniformInfo = mergedUniforms.emplace_back();
			mergedUniformInfo.name = uniform.name;
			mergedUniformInfo.dataType = uniform.dataType;
			mergedUniformInfo.elementWidth = uniform.elementWidth;
			mergedUniformInfo.elementHeight = uniform.elementHeight;
			mergedUniformInfo.nativeDataType = uniform.nativeDataType;
			mergedUniformInfo.bufferOffset = uniform.bufferOffset;
			mergedUniformInfo.arraySize = uniform.arraySize;
			mergedUniformInfo.arrayStride = uniform.arrayStride;
			mergedUniformInfo.uniformIndices.push_back(uniformIndex);
			continue;
		}

		// Build the merged name and array indices for this array uniform
		std::string mergedUniformName;
		std::vector<size_t> arraySizes;
		bool failed = false;
		size_t currentArrayLevel = 0;
		std::string::size_type lastNameStringCopyPos = 0;
		while (!failed && (openBracePos != std::string::npos))
		{
			// Find the closing brace
			auto closeBracePos = uniform.name.find(']', openBracePos + 1);
			if (closeBracePos == std::string::npos)
			{
				_log->Warning("Skipping state buffer uniform {0} with name \"{1}\" as we encountered mismatched brackets", uniformIndex, mergedUniformName);
				failed = true;
				continue;
			}

			// Append the next set of string data into the merged uniform name
			mergedUniformName += uniform.name.substr(lastNameStringCopyPos, (openBracePos + 1) - lastNameStringCopyPos);
			lastNameStringCopyPos = closeBracePos;

			// Convert the base 10 array index string to an integer
			auto arrayIndex = (size_t)std::strtol(uniform.name.c_str() + openBracePos + 1, nullptr, 10);

			// Increase the array size of this dimension to take into account the supplied array index
			uniform.arrayIndices.resize(currentArrayLevel + 1);
			uniform.arrayIndices[currentArrayLevel] = arrayIndex;
			arraySizes.resize(currentArrayLevel + 1);
			arraySizes[currentArrayLevel] = arrayIndex + 1;

			// Advance to the next array level
			++currentArrayLevel;
			openBracePos = uniform.name.find('[', closeBracePos + 1);
		}
		if (failed)
		{
			continue;
		}
		mergedUniformName += uniform.name.substr(lastNameStringCopyPos);

		// If this uniform was an array itself, we will get one entry for it of the form "someUniform[0]", with a full
		// array size already decoded and passed in. In this case, we need to strip the last array size entry, since we
		// need to pass this along separately.
		if (uniform.arraySize > 1)
		{
			arraySizes.pop_back();
		}

		// Combine this array info with the existing merged uniform entry, or create a new one if none exists.
		auto uniformNameToMergedIndexIterator = uniformNameToMergedIndex.find(mergedUniformName);
		if (uniformNameToMergedIndexIterator == uniformNameToMergedIndex.end())
		{
			uniformNameToMergedIndex.insert(std::make_pair(mergedUniformName, mergedUniforms.size()));
			MergedStateBufferUniformInfo& mergedUniformInfo = mergedUniforms.emplace_back();
			mergedUniformInfo.name = std::move(mergedUniformName);
			mergedUniformInfo.dataType = uniform.dataType;
			mergedUniformInfo.elementWidth = uniform.elementWidth;
			mergedUniformInfo.elementHeight = uniform.elementHeight;
			mergedUniformInfo.nativeDataType = uniform.nativeDataType;
			mergedUniformInfo.bufferOffset = uniform.bufferOffset;
			mergedUniformInfo.arraySize = uniform.arraySize;
			mergedUniformInfo.arrayStride = uniform.arrayStride;
			mergedUniformInfo.leadingArraySizes = std::move(arraySizes);
			mergedUniformInfo.uniformIndices.push_back(uniformIndex);
		}
		else
		{
			MergedStateBufferUniformInfo& mergedUniformInfo = mergedUniforms[uniformNameToMergedIndexIterator->second];
			for (size_t i = 0; i < arraySizes.size(); ++i)
			{
				mergedUniformInfo.leadingArraySizes[i] = std::max(arraySizes[i], mergedUniformInfo.leadingArraySizes[i]);
			}
			mergedUniformInfo.uniformIndices.push_back(uniformIndex);
		}
	}

	// Calculate array stride values for our merged uniforms
	for (auto& mergedUniform : mergedUniforms)
	{
		mergedUniform.leadingArrayStrides.resize(mergedUniform.leadingArraySizes.size(), 0);
		for (size_t arraySizeIndex = 0; arraySizeIndex < mergedUniform.leadingArraySizes.size(); ++arraySizeIndex)
		{
			// Ensure that there are at least two entries in the target array. If there aren't, a default stride of 0 is
			// fine, since there's only one array entry.
			if (mergedUniform.leadingArraySizes[arraySizeIndex] <= 1)
			{
				continue;
			}

			// Calculate the array stride from the first two elements
			bool foundFirstOffset = false;
			bool foundSecondOffset = false;
			size_t firstOffset = 0;
			size_t secondOffset = 0;
			size_t uniformSearchIndex = 0;
			while ((!foundFirstOffset || !foundSecondOffset) && (uniformSearchIndex < mergedUniform.uniformIndices.size()))
			{
				const auto& targetUniform = uniforms[mergedUniform.uniformIndices[uniformSearchIndex]];
				bool isPossibleFirstOffset = true;
				bool isPossibleSecondOffset = true;
				size_t arraySearchIndex = 0;
				while ((isPossibleFirstOffset || isPossibleSecondOffset) && (arraySearchIndex < mergedUniform.leadingArraySizes.size()))
				{
					isPossibleFirstOffset &= (targetUniform.arrayIndices[arraySearchIndex] == 0);
					isPossibleSecondOffset &= ((targetUniform.arrayIndices[arraySearchIndex] == 0) && (arraySearchIndex != arraySizeIndex)) || ((targetUniform.arrayIndices[arraySearchIndex] == 1) && (arraySearchIndex == arraySizeIndex));
					++arraySearchIndex;
				}
				if (isPossibleFirstOffset)
				{
					firstOffset = targetUniform.bufferOffset;
					foundFirstOffset = true;
				}
				else if (isPossibleSecondOffset)
				{
					secondOffset = targetUniform.bufferOffset;
					foundSecondOffset = true;
				}
				++uniformSearchIndex;
			}
			size_t arrayStride = secondOffset - firstOffset;
			mergedUniform.leadingArrayStrides[arraySizeIndex] = arrayStride;
		}
	}

	// We now flatten multi-dimensional arrays to single-dimensional arrays. While OpenGL fully supports
	// multi-dimensional arrays in uniform buffers, Direct3D does not, and it flattens multi-dimensional arrays to
	// single-dimensional arrays in the reflection API. For consistency across renderers, we opt to do the same thing
	// here for OpenGL. To perform the flattening step, we identify multi-dimensional arrays, multiply the array sizes
	// together, and flatten the array notation in the variable name.
	for (auto& mergedUniformInfo : mergedUniforms)
	{
		// If this merged uniform doesn't have multidimensional arrays, skip it. Since we expect this to be a rare case,
		// this is a performance optimization to save additional work.
		if (mergedUniformInfo.name.find("[][]") == std::string::npos)
		{
			continue;
		}

		// Find the first array notation in the uniform name
		std::string arrayNotationString = "[]";
		size_t previousArrayStringSearchPos = mergedUniformInfo.name.find(arrayNotationString);

		// Build a new name and set of leading array sizes for the uniform, with multidimensional arrays flattened.
		std::string flattenedName = mergedUniformInfo.name.substr(0, previousArrayStringSearchPos + arrayNotationString.size());
		std::vector<size_t> flattenedLeadingArraySizes;
		std::vector<size_t> flattenedLeadingArrayStrides;
		size_t currentArrayIndex = 0;
		flattenedLeadingArraySizes.push_back(mergedUniformInfo.leadingArraySizes[currentArrayIndex]);
		flattenedLeadingArrayStrides.push_back(mergedUniformInfo.leadingArrayStrides[currentArrayIndex]);
		++currentArrayIndex;
		size_t arrayStringSearchPos = mergedUniformInfo.name.find(arrayNotationString, previousArrayStringSearchPos + arrayNotationString.size());
		while (arrayStringSearchPos != std::string::npos)
		{
			if ((mergedUniformInfo.arraySize > 1) && (currentArrayIndex == mergedUniformInfo.leadingArraySizes.size()))
			{
				// If this uniform is an array uniform, and we're up to the last array notation, merge the last leading
				// array notation into it if required, and abort any further processing.
				if (arrayStringSearchPos == (previousArrayStringSearchPos + arrayNotationString.size()))
				{
					mergedUniformInfo.arraySize *= flattenedLeadingArraySizes.back();
					flattenedLeadingArraySizes.pop_back();
					flattenedLeadingArrayStrides.pop_back();
				}
				break;
			}
			if (arrayStringSearchPos != (previousArrayStringSearchPos + arrayNotationString.size()))
			{
				// If the next array notation doesn't immediately follow the last one, pass it through without changes.
				flattenedName += mergedUniformInfo.name.substr(previousArrayStringSearchPos + arrayNotationString.size(), arrayStringSearchPos - (previousArrayStringSearchPos + arrayNotationString.size()));
				flattenedLeadingArraySizes.push_back(mergedUniformInfo.leadingArraySizes[currentArrayIndex]);
				flattenedLeadingArrayStrides.push_back(mergedUniformInfo.leadingArrayStrides[currentArrayIndex]);
			}
			else
			{
				// Since thie array notation immediately follows the last one, merge the array sizes together, and take
				// the stride of the lower dimension.
				flattenedLeadingArraySizes.back() *= mergedUniformInfo.leadingArraySizes[currentArrayIndex];
				flattenedLeadingArrayStrides.back() = mergedUniformInfo.leadingArrayStrides[currentArrayIndex];
			}
			++currentArrayIndex;
			previousArrayStringSearchPos = arrayStringSearchPos;
			arrayStringSearchPos = mergedUniformInfo.name.find(arrayNotationString, previousArrayStringSearchPos + arrayNotationString.size());
		}

		// Assign the flattened uniform name and array sizes back to the merged uniform. Note that we need to do this after we
		// generate flattened indices for the underlying uniform entries above.
		mergedUniformInfo.name = std::move(flattenedName);
		mergedUniformInfo.leadingArraySizes = std::move(flattenedLeadingArraySizes);
		mergedUniformInfo.leadingArrayStrides = std::move(flattenedLeadingArrayStrides);
	}

	// Return the layout info to the caller
	//##FIX## Don't store redundant data here we don't need
	layoutInfo.uniformCount = uniformCount;
	layoutInfo.uniformIndices = std::move(uniformIndices);
	layoutInfo.uniformTypes = std::move(uniformTypes);
	layoutInfo.uniformNameLengths = std::move(uniformNameLengths);
	layoutInfo.uniformBlockIndexes = std::move(uniformBlockIndexes);
	layoutInfo.uniformOffsets = std::move(uniformOffsets);
	layoutInfo.uniformArraySizes = std::move(uniformArraySizes);
	layoutInfo.uniformArrayStrides = std::move(uniformArrayStrides);
	layoutInfo.uniformMatrixStrides = std::move(uniformMatrixStrides);
	layoutInfo.uniformMatrixIsRowMajors = std::move(uniformMatrixIsRowMajors);
	layoutInfo.uniformNames = std::move(uniformNames);
	layoutInfo.uniforms = std::move(uniforms);
	layoutInfo.mergedUniforms = std::move(mergedUniforms);
	layoutInfo.uniformNameToMergedIndex = std::move(uniformNameToMergedIndex);
}

//----------------------------------------------------------------------------------------
SuccessToken OpenGLShaderProgram::LoadStateBufferLayoutFromShader(StateBufferId stateBufferId, IStateBufferLayout* stateBufferLayout) const
{
	// Validate the supplied attributes
	if (stateBufferId == StateBufferId::Null)
	{
		_log->Warning("Attempted to load state buffer layout from invalid state buffer ID {0}", stateBufferId);
		return false;
	}

	// Retrieve the layout info for the target state buffer
	auto stateBufferLayoutInfoIterator = _stateBufferLayoutInfo.find(stateBufferId);
	if (stateBufferLayoutInfoIterator == _stateBufferLayoutInfo.end())
	{
		_log->Error("Failed to locate state buffer layout info for state buffer with ID {0}", stateBufferId);
		return false;
	}
	const StateBufferLayoutInfo& layoutInfo = stateBufferLayoutInfoIterator->second;

	// Initiate the state buffer layout build process
	auto* stateBufferLayoutResolved = KnownDynamicCast<OpenGLStateBufferLayout*>(stateBufferLayout);
	if (!stateBufferLayoutResolved->BeginManualLayoutDefinition())
	{
		_log->Error("Failed to start layout definition build process when loading state buffer layout from program {0}", _programID);
		return false;
	}

	// Add each uniform to the state buffer layout
	//##TODO## Handle uniformMatrixStrides
	//##TODO## Handle uniformMatrixIsRowMajors
	for (const auto& mergedUniform : layoutInfo.mergedUniforms)
	{
		// Add this uniform to the state buffer layout
		if (mergedUniform.elementWidth == 0)
		{
			stateBufferLayoutResolved->AddManualField(mergedUniform.name, mergedUniform.bufferOffset, mergedUniform.dataType, mergedUniform.arraySize, mergedUniform.arrayStride, mergedUniform.leadingArraySizes, mergedUniform.leadingArrayStrides);
		}
		else if (mergedUniform.elementHeight == 0)
		{
			stateBufferLayoutResolved->AddManualVector(mergedUniform.name, mergedUniform.bufferOffset, mergedUniform.dataType, mergedUniform.elementWidth, mergedUniform.arraySize, mergedUniform.arrayStride, mergedUniform.leadingArraySizes, mergedUniform.leadingArrayStrides);
		}
		else
		{
			stateBufferLayoutResolved->AddManualMatrix(mergedUniform.name, mergedUniform.bufferOffset, mergedUniform.dataType, mergedUniform.elementWidth, mergedUniform.elementHeight, mergedUniform.arraySize, mergedUniform.arrayStride, mergedUniform.leadingArraySizes, mergedUniform.leadingArrayStrides);
		}
	}

	// Attempt to construct the state buffer layout, and return the result to the caller.
	return stateBufferLayout->ConstructStateLayout();
}

//----------------------------------------------------------------------------------------
constexpr bool OpenGLShaderProgram::StateBufferEntryNativeTypeToBufferLayoutType(GLint nativeType, IStateBufferLayout::DataType& type, uint32_t& elementWidth, uint32_t& elementHeight)
{
	elementWidth = 0;
	elementHeight = 0;
	switch (nativeType)
	{
	case GL_FLOAT:
		type = IStateBufferLayout::DataType::Float32;
		return true;
	case GL_FLOAT_VEC2:
		type = IStateBufferLayout::DataType::Float32;
		elementWidth = 2;
		return true;
	case GL_FLOAT_VEC3:
		type = IStateBufferLayout::DataType::Float32;
		elementWidth = 3;
		return true;
	case GL_FLOAT_VEC4:
		type = IStateBufferLayout::DataType::Float32;
		elementWidth = 4;
		return true;
#ifdef GL_VERSION_4_1
	case GL_DOUBLE:
		type = IStateBufferLayout::DataType::Float64;
		return true;
	case GL_DOUBLE_VEC2:
		type = IStateBufferLayout::DataType::Float64;
		elementWidth = 2;
		return true;
	case GL_DOUBLE_VEC3:
		type = IStateBufferLayout::DataType::Float64;
		elementWidth = 3;
		return true;
	case GL_DOUBLE_VEC4:
		type = IStateBufferLayout::DataType::Float64;
		elementWidth = 4;
		return true;
#endif
	case GL_INT:
		type = IStateBufferLayout::DataType::Int32;
		return true;
	case GL_INT_VEC2:
		type = IStateBufferLayout::DataType::Int32;
		elementWidth = 2;
		return true;
	case GL_INT_VEC3:
		type = IStateBufferLayout::DataType::Int32;
		elementWidth = 3;
		return true;
	case GL_INT_VEC4:
		type = IStateBufferLayout::DataType::Int32;
		elementWidth = 4;
		return true;
	case GL_UNSIGNED_INT:
		type = IStateBufferLayout::DataType::UInt32;
		return true;
	case GL_UNSIGNED_INT_VEC2:
		type = IStateBufferLayout::DataType::UInt32;
		elementWidth = 2;
		return true;
	case GL_UNSIGNED_INT_VEC3:
		type = IStateBufferLayout::DataType::UInt32;
		elementWidth = 3;
		return true;
	case GL_UNSIGNED_INT_VEC4:
		type = IStateBufferLayout::DataType::UInt32;
		elementWidth = 4;
		return true;
	case GL_BOOL:
		type = IStateBufferLayout::DataType::Boolean;
		return true;
	case GL_BOOL_VEC2:
		type = IStateBufferLayout::DataType::Boolean;
		elementWidth = 2;
		return true;
	case GL_BOOL_VEC3:
		type = IStateBufferLayout::DataType::Boolean;
		elementWidth = 3;
		return true;
	case GL_BOOL_VEC4:
		type = IStateBufferLayout::DataType::Boolean;
		elementWidth = 4;
		return true;
	case GL_FLOAT_MAT2:
		type = IStateBufferLayout::DataType::Float32;
		elementWidth = 2;
		elementHeight = 2;
		return true;
	case GL_FLOAT_MAT3:
		type = IStateBufferLayout::DataType::Float32;
		elementWidth = 3;
		elementHeight = 3;
		return true;
	case GL_FLOAT_MAT4:
		type = IStateBufferLayout::DataType::Float32;
		elementWidth = 4;
		elementHeight = 4;
		return true;
	case GL_FLOAT_MAT2x3:
		type = IStateBufferLayout::DataType::Float32;
		elementWidth = 2;
		elementHeight = 3;
		return true;
	case GL_FLOAT_MAT2x4:
		type = IStateBufferLayout::DataType::Float32;
		elementWidth = 2;
		elementHeight = 4;
		return true;
	case GL_FLOAT_MAT3x2:
		type = IStateBufferLayout::DataType::Float32;
		elementWidth = 3;
		elementHeight = 2;
		return true;
	case GL_FLOAT_MAT3x4:
		type = IStateBufferLayout::DataType::Float32;
		elementWidth = 3;
		elementHeight = 4;
		return true;
	case GL_FLOAT_MAT4x2:
		type = IStateBufferLayout::DataType::Float32;
		elementWidth = 4;
		elementHeight = 2;
		return true;
	case GL_FLOAT_MAT4x3:
		type = IStateBufferLayout::DataType::Float32;
		elementWidth = 4;
		elementHeight = 3;
		return true;
#ifdef GL_VERSION_4_1
	case GL_DOUBLE_MAT2:
		type = IStateBufferLayout::DataType::Float64;
		elementWidth = 2;
		elementHeight = 2;
		return true;
	case GL_DOUBLE_MAT3:
		type = IStateBufferLayout::DataType::Float64;
		elementWidth = 3;
		elementHeight = 3;
		return true;
	case GL_DOUBLE_MAT4:
		type = IStateBufferLayout::DataType::Float64;
		elementWidth = 4;
		elementHeight = 4;
		return true;
	case GL_DOUBLE_MAT2x3:
		type = IStateBufferLayout::DataType::Float64;
		elementWidth = 2;
		elementHeight = 3;
		return true;
	case GL_DOUBLE_MAT2x4:
		type = IStateBufferLayout::DataType::Float64;
		elementWidth = 2;
		elementHeight = 4;
		return true;
	case GL_DOUBLE_MAT3x2:
		type = IStateBufferLayout::DataType::Float64;
		elementWidth = 3;
		elementHeight = 2;
		return true;
	case GL_DOUBLE_MAT3x4:
		type = IStateBufferLayout::DataType::Float64;
		elementWidth = 3;
		elementHeight = 4;
		return true;
	case GL_DOUBLE_MAT4x2:
		type = IStateBufferLayout::DataType::Float64;
		elementWidth = 4;
		elementHeight = 2;
		return true;
	case GL_DOUBLE_MAT4x3:
		type = IStateBufferLayout::DataType::Float64;
		elementWidth = 4;
		elementHeight = 3;
		return true;
#endif
	}
	return false;
}

//----------------------------------------------------------------------------------------
std::unique_ptr<IStateValueInfo> OpenGLShaderProgram::GetGlobalUniformCurrentValue(GLenum nativeDataType, GLint location, StateValueId stateId, const size_t* arrayIndices, size_t arrayIndexCount) const
{
	switch (nativeDataType)
	{
	case GL_FLOAT:
	{
		V1Float32 value;
		glGetUniformfv(_programID, location, value.data());
		return std::unique_ptr<IStateValueInfo>(new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
	}
	case GL_FLOAT_VEC2:
	{
		V2Float32 value;
		glGetUniformfv(_programID, location, value.data());
		return std::unique_ptr<IStateValueInfo>(new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
	}
	case GL_FLOAT_VEC3:
	{
		V3Float32 value;
		glGetUniformfv(_programID, location, value.data());
		return std::unique_ptr<IStateValueInfo>(new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
	}
	case GL_FLOAT_VEC4:
	{
		V4Float32 value;
		glGetUniformfv(_programID, location, value.data());
		return std::unique_ptr<IStateValueInfo>(new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
	}
#ifdef GL_VERSION_4_1
	case GL_DOUBLE:
	{
		V1Float64 value;
		glGetUniformdv(_programID, location, value.data());
		return std::unique_ptr<IStateValueInfo>(new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
	}
	case GL_DOUBLE_VEC2:
	{
		V2Float64 value;
		glGetUniformdv(_programID, location, value.data());
		return std::unique_ptr<IStateValueInfo>(new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
	}
	case GL_DOUBLE_VEC3:
	{
		V3Float64 value;
		glGetUniformdv(_programID, location, value.data());
		return std::unique_ptr<IStateValueInfo>(new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
	}
	case GL_DOUBLE_VEC4:
	{
		V4Float64 value;
		glGetUniformdv(_programID, location, value.data());
		return std::unique_ptr<IStateValueInfo>(new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
	}
#endif
	case GL_INT:
	{
		V1Int32 value;
		glGetUniformiv(_programID, location, value.data());
		return std::unique_ptr<IStateValueInfo>(new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
	}
	case GL_INT_VEC2:
	{
		V2Int32 value;
		glGetUniformiv(_programID, location, value.data());
		return std::unique_ptr<IStateValueInfo>(new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
	}
	case GL_INT_VEC3:
	{
		V3Int32 value;
		glGetUniformiv(_programID, location, value.data());
		return std::unique_ptr<IStateValueInfo>(new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
	}
	case GL_INT_VEC4:
	{
		V4Int32 value;
		glGetUniformiv(_programID, location, value.data());
		return std::unique_ptr<IStateValueInfo>(new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
	}
	case GL_UNSIGNED_INT:
	{
		V1UInt32 value;
		glGetUniformuiv(_programID, location, value.data());
		return std::unique_ptr<IStateValueInfo>(new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
	}
	case GL_UNSIGNED_INT_VEC2:
	{
		V2UInt32 value;
		glGetUniformuiv(_programID, location, value.data());
		return std::unique_ptr<IStateValueInfo>(new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
	}
	case GL_UNSIGNED_INT_VEC3:
	{
		V3UInt32 value;
		glGetUniformuiv(_programID, location, value.data());
		return std::unique_ptr<IStateValueInfo>(new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
	}
	case GL_UNSIGNED_INT_VEC4:
	{
		V4UInt32 value;
		glGetUniformuiv(_programID, location, value.data());
		return std::unique_ptr<IStateValueInfo>(new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
	}
	case GL_BOOL:
	{
		int value;
		glGetUniformiv(_programID, location, &value);
		return std::unique_ptr<IStateValueInfo>(new StateValueInfo<bool>(stateId, (bool)value, arrayIndices, arrayIndexCount));
	}
	case GL_FLOAT_MAT2:
	{
		M2Float32 value;
		glGetUniformfv(_programID, location, value.data());
		return std::unique_ptr<IStateValueInfo>(new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
	}
	case GL_FLOAT_MAT3:
	{
		M3Float32 value;
		glGetUniformfv(_programID, location, value.data());
		return std::unique_ptr<IStateValueInfo>(new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
	}
	case GL_FLOAT_MAT4:
	{
		M4Float32 value;
		glGetUniformfv(_programID, location, value.data());
		return std::unique_ptr<IStateValueInfo>(new StateValueInfo<std::remove_const<std::remove_reference<decltype(value)>::type>::type>(stateId, value, arrayIndices, arrayIndexCount));
	}
	}
	return nullptr;
}

//----------------------------------------------------------------------------------------
// Shader binding methods
//----------------------------------------------------------------------------------------
void OpenGLShaderProgram::BindShaderProgram()
{
	// Bind this program
	glUseProgram(_programID);

	// Associate our texture samplers to their defined binding points
	for (const auto& entry : _textureSamplerAssociations)
	{
		glUniform1i(entry.first, entry.second);
	}

	// Set the clip distance count for this shader. Note that user clip planes are permanently enabled in Direct3D and
	// Vulkan, and cannot be disabled. We desire this behaviour for our OpenGL implementation too, rather than having to
	// manually define which planes are enabled. As this is a shader feature under the programmable pipeline, it seems
	// logical that planes won't exist where they aren't required, and where they are required, but optionally disabled, it
	// is far more preferable to force a positive clip distance to disable culling in the shader, than it is to require a
	// CPU state change to disable the clipping plane. To get this behaviour here, and make it consistent with Direct3D and
	// Vulkan, we have scanned the shader and determined which clip distances are defined, and force them to enabled, and
	// all others to disabled, to make this behaviour automatic for the application.
	int clipPlaneNo = 0;
	while (clipPlaneNo < _userClipPlaneCount)
	{
		glEnable(GL_CLIP_DISTANCE0 + clipPlaneNo);
		++clipPlaneNo;
	}
	while (clipPlaneNo < _maxUserClipPlaneCount)
	{
		glDisable(GL_CLIP_DISTANCE0 + clipPlaneNo);
		++clipPlaneNo;
	}
}

//----------------------------------------------------------------------------------------
// State value methods
//----------------------------------------------------------------------------------------
void OpenGLShaderProgram::PushGlobalConstantBufferState(const std::vector<IStateValueInfo*>* valueEntries)
{
	uint32_t targetStackIndex = _stateBufferStackEntries;
	++_stateBufferStackEntries;
	_stateBufferStack.resize(_stateBufferStackEntries);
	_stateBufferStack[targetStackIndex] = valueEntries;
}

//----------------------------------------------------------------------------------------
void OpenGLShaderProgram::PopGlobalConstantBufferState()
{
	assert(_stateBufferStackEntries > 1);
	--_stateBufferStackEntries;
	RestoreGlobalConstantBufferBaseline();
}

//----------------------------------------------------------------------------------------
void OpenGLShaderProgram::RestoreGlobalConstantBufferBaseline()
{
	for (uint32_t i = 0; i < _stateBufferStackEntries; ++i)
	{
		const std::vector<IStateValueInfo*>* stateValues = _stateBufferStack[i];
		for (IStateValueInfo* stateValue : *stateValues)
		{
			stateValue->ApplyValue(this);
		}
	}
}

} // namespace cobalt::graphics
