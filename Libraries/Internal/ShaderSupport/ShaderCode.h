// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "IShaderCode.h"
#include <Cobalt/Debug/Debug.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
WARNINGS_PUSH_OFF
#include "spirv-tools/libspirv.hpp"
#include "spirv-tools/optimizer.hpp"
#include "spirv_cross.hpp"
#include "spirv_glsl.hpp"
#include "vulkan/vulkan.h"
WARNINGS_POP
#include <vector>
namespace cobalt::graphics {

class ShaderCode : public IShaderCode
{
public:
	// Constructors
	explicit ShaderCode(cobalt::logging::ILogger::unique_ptr&& log);
	~ShaderCode();

	// Initialization methods
	void Delete() override;

	// Code methods
	bool LoadCode(Language language, Stage stage, Environment sourceEnvironment, Environment targetEnvironment, const uint8_t* shaderCode, size_t shaderCodeSizeInBytes, const std::string& entryPointName, int& nextBindingNo, std::vector<Resource>& resourceSet) override;
	bool ExportCodeAsHLSL(std::string& outputCode, unsigned int shaderModelVersionMajor, unsigned int shaderModelVersionMinor) const override;
	bool ExportCodeAsSPIRVAssembly(std::string& outputCode) const override;
	bool ExportCodeAsSPIRV(std::vector<uint32_t>& outputCode) const override;
	bool ExportCodeAsGLSL(std::string& outputCode, bool hasGeometryStage) const override;
	bool ExportCodeAsMSL(std::string& outputCode) const override;
	bool HasLoadedCode() const override;
	bool ValidateCode() const override;

private:
	// Nested types
	class OpenGLLayoutCompiler : public spirv_cross::CompilerGLSL
	{
	public:
		// Constructors
		using spirv_cross::CompilerGLSL::CompilerGLSL;

		// Helper methods
		void MakeBufferLayoutsOpenGLCompatible();

	private:
		// Helper methods
		static uint32_t AlignTo(uint32_t value, uint32_t alignment);
		void RepackBuffer(const spirv_cross::Resource& resource, spirv_cross::BufferPackingStandard packing);
		void RepackStruct(const spirv_cross::SPIRType& type, spirv_cross::BufferPackingStandard packing);
		void RepackNestedType(const spirv_cross::SPIRType& type, const spirv_cross::Bitset& flags, spirv_cross::BufferPackingStandard packing);
	};

private:
	// Reflection methods
	void GetUniformFields(std::vector<Resource>* resourceVec, spirv_cross::SPIRType type) const;
	void GetNativeVulkanFormatAndSize(Resource* resource, spirv_cross::SPIRType type) const;
	static VkFormat GetNativeVulkanFormatFromImageFormat(spv::ImageFormat imageFormat);

	// Code modification
	void ChangeName(const std::string& name, uint32_t id);
	void ChangeBinding(uint32_t binding, uint32_t id);
	void ChangeDescriptorSet(uint32_t descriptor, uint32_t id);

	// Code methods
	static bool LocateResourceInSet(const std::vector<Resource>& resourceSet, const spirv_cross::Resource& resource, ResourceType type, size_t& resourceIndex);
	void ForwardLogMessage(const char* sourceSystem, spv_message_level_t level, const char* source, const spv_position_t& position, const char* message) const;

	// Code patch methods
	static bool StripDescriptorIndexingDecls(std::vector<uint32_t>& spirv);
	static bool FixVertexPointSizeAlwaysOne(std::vector<uint32_t>& spirv);
	static bool FixHlslConstantBufferNames(const std::string& hlsl, std::vector<uint32_t>& spv);
	static bool FixVulkanGlPerVertexClipDistance(std::vector<uint32_t>& spv);
	static bool FixVulkanBuiltinIds(std::vector<uint32_t>& spv);
	static bool FoldNamedStandaloneSamplersIntoImages(std::vector<uint32_t>& spv);
	static bool FixStructTypeNameCollisionsOnMoltenVK(std::vector<uint32_t>& spirv);
	static bool FixMissingLocationAttributes(std::vector<uint32_t>& spirv);

	// String manipulation methods
	static void StringReplaceAll(std::string& source, const std::string& from, const std::string& to);
	static void StringTrim(std::string& s);

private:
	cobalt::logging::ILogger::unique_ptr _log;
	std::vector<uint32_t> _spirvCode;
	std::unique_ptr<spirv_cross::Compiler> _compiler;
	Stage _stage = {};
	Environment _sourceEnvironment = {};
	Environment _targetEnvironment = {};
	spv_target_env _targetEnvironmentNative = {};
	bool _codeLoaded = false;
};

} // namespace cobalt::graphics
