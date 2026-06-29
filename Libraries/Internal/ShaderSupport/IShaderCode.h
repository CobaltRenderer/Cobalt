// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <memory>
#include <string>
#include <vector>
namespace cobalt::graphics {

class IShaderCode
{
public:
	// Constants
	static constexpr const char* GlobalConstantBufferName = "$Global";
	static constexpr const char* StandardShaderEntryPointName = "main";
	static constexpr const char* CombinedImageSamplerPostfix = "_CombinedSampler";

	// Enumerations
	enum class Language
	{
		HLSL,
		SPIRVAssembly,
		SPIRV,
		GLSL,
		MSL,
	};
	enum class Environment
	{
		General,
		OpenGL_33,
		OpenGL_43,
		Vulkan_11,
		Vulkan_12,
	};
	enum class Stage
	{
		Vertex = 0x01,
		Fragment = 0x02,
		Geometry = 0x04,
		Compute = 0x08,
	};
	enum class ResourceType
	{
		None,
		Input,
		Output,
		Sampler,
		Texture,
		TextureWithCombinedSampler,
		Uniform,
		Field,
		StructField,
		DataArray,
		TexelArray,
	};
	enum class DataType
	{
		Null = 0,
		Boolean,
		Int32,
		UInt32,
		Float32,
		Float64,
	};

	// Structures
	//##TODO## Clean this up
	struct Resource
	{
		bool valid = false;
		int id = -1;
		std::string name;
		ResourceType type = ResourceType::None;
		int location{};
		int binding{};

		size_t width{};
		size_t elementSize{};
		size_t elementCount{};
		size_t elementStride{};
		size_t size{}; // elementCount * elementStride
		size_t vecSize{};
		size_t columns{};
		std::vector<uint32_t> arraySizes;
		Stage usedStages = {};
		bool writeableResource = false;

		size_t offset{};
		uint32_t nativeVulkanFormat = 0;
		uint32_t nativeVulkanImageViewType = 0;
		DataType layoutType = DataType::Null;
		size_t order{};
		std::vector<Resource> fields;
	};

	// Nested types
	class Deleter
	{
	public:
		inline void operator()(IShaderCode* target)
		{
			target->Delete();
		}
	};

	// Typedefs
	typedef std::unique_ptr<IShaderCode, Deleter> unique_ptr;

public:
	// Initialization methods
	static inline unique_ptr Create(cobalt::logging::ILogger::unique_ptr log);
	virtual void Delete() = 0;

	// Code methods
	virtual bool LoadCode(Language language, Stage stage, Environment sourceEnvironment, Environment targetEnvironment, const uint8_t* shaderCode, size_t shaderCodeSizeInBytes, const std::string& entryPointName, int& nextBindingNo, std::vector<Resource>& resourceSet) = 0;
	virtual bool ExportCodeAsHLSL(std::string& outputCode, unsigned int shaderModelVersionMajor, unsigned int shaderModelVersionMinor) const = 0;
	virtual bool ExportCodeAsSPIRVAssembly(std::string& outputCode) const = 0;
	virtual bool ExportCodeAsSPIRV(std::vector<uint32_t>& outputCode) const = 0;
	virtual bool ExportCodeAsGLSL(std::string& outputCode, bool hasGeometryStage) const = 0;
	virtual bool ExportCodeAsMSL(std::string& outputCode) const = 0;
	virtual bool HasLoadedCode() const = 0;
	virtual bool ValidateCode() const = 0;
};

} // namespace cobalt::graphics
#include "IShaderCode.inl"
