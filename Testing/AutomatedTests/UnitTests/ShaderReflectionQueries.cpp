// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../UnitTestBase.h"
namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

namespace {

// Define our shader programs
const std::string ReflectionVertexShader = R"(
struct VSInput {
    float3 position : position;
    float2 texCoord : texCoord;
};

struct ReflectionData {
    float4 colorScale;
};

cbuffer ReflectionBuffer
{
    ReflectionData reflectionData;
};

struct VSOutput {
    float4 position : SV_POSITION;
    float2 texCoord : texCoord;
    float4 colorScale : COLOR;
};

VSOutput main(VSInput IN)
{
    VSOutput OUT;
    OUT.position = float4(IN.position, 1.0f);
    OUT.texCoord = IN.texCoord;
    OUT.colorScale = reflectionData.colorScale;
    return OUT;
}
)";
const std::string ReflectionFragmentShader = R"(
uniform Texture2D reflectionTexture;
uniform SamplerState reflectionTexture_CombinedSampler;
uniform float4 looseColor;

struct VSOutput {
    float4 position : SV_POSITION;
    float2 texCoord : texCoord;
    float4 colorScale : COLOR;
};

float4 main(VSOutput IN) : SV_TARGET0
{
    return (reflectionTexture.Sample(reflectionTexture_CombinedSampler, IN.texCoord) * IN.colorScale) + looseColor;
}
)";

const std::string ResourceArrayVertexShader = R"(
float4 main(float3 position : position) : SV_POSITION
{
    return float4(position, 1.0f);
}
)";
const std::string MinimalFragmentShader = R"(
float4 main() : SV_TARGET0
{
    return float4(1.0f, 0.0f, 0.0f, 1.0f);
}
)";
const std::string MinimalComputeShader = R"(
[numthreads(1, 1, 1)]
void main(uint3 dispatchId : SV_DispatchThreadID)
{}
)";
const std::string InvalidHLSLShader = R"(
this is not valid hlsl
)";
const std::string ResourceArrayFragmentShader = R"(
StructuredBuffer<float4> reflectionResourceArray;

float4 main() : SV_TARGET0
{
    return reflectionResourceArray[0];
}
)";
const std::string ComplexStateBufferVertexShader = R"(
struct VSInput {
    float3 position : position;
};

struct NestedReflectionData {
    float4 nestedColor;
    uint nestedFlag;
};

cbuffer ComplexReflectionBuffer
{
    bool enabled;
    int modeValue;
    uint flagValue;
    float scalarValues[6];
    float4 vectorValues[2];
    row_major float4x4 matrixValues[2];
    NestedReflectionData nestedValues[2];
};

float4 main(VSInput IN) : SV_POSITION
{
    float reflectedValue = scalarValues[5] + vectorValues[1].x + matrixValues[1][0][0] + nestedValues[1].nestedColor.x + float(nestedValues[1].nestedFlag);
    reflectedValue += enabled ? 0.125f : 0.25f;
    reflectedValue += (modeValue == 0) ? 0.375f : 0.5f;
    reflectedValue += (flagValue == 0) ? 0.625f : 0.75f;
    return float4(IN.position.xy, reflectedValue, 1.0f);
}
)";
const std::string WideStateBufferVertexShader = R"(
struct VSInput {
    float3 position : position;
};

cbuffer WideReflectionBuffer
{
    bool2 boolPair;
    bool3 boolTriple;
    bool4 boolQuad;
    int2 intPair;
    int3 intTriple;
    int4 intQuad;
    uint2 uintPair;
    uint3 uintTriple;
    uint4 uintQuad;
    float2 floatPair;
    float3 floatTriple;
    float2x2 matrix2x2;
    float3x3 matrix3x3;
    float2x3 matrix2x3;
    float2x4 matrix2x4;
    float3x2 matrix3x2;
    float3x4 matrix3x4;
    float4x2 matrix4x2;
    float4x3 matrix4x3;
    row_major float4x2 rowMatrix4x2;
    column_major float2x4 columnMatrix2x4;
};

float4 main(VSInput IN) : SV_POSITION
{
    float reflectedValue = floatPair.x + floatTriple.y;
    reflectedValue += matrix2x2[0][0] + matrix3x3[0][0];
    reflectedValue += matrix2x3[0][0] + matrix2x4[0][0] + matrix3x2[0][0] + matrix3x4[0][0] + matrix4x2[0][0] + matrix4x3[0][0];
    reflectedValue += rowMatrix4x2[0][0] + columnMatrix2x4[0][0];
    reflectedValue += boolPair.x ? 0.03125f : 0.0625f;
    reflectedValue += boolTriple.y ? 0.09375f : 0.125f;
    reflectedValue += boolQuad.z ? 0.15625f : 0.1875f;
    reflectedValue += float(intPair.x + intTriple.y + intQuad.z);
    reflectedValue += float(uintPair.x + uintTriple.y + uintQuad.z);
    return float4(IN.position.xy, reflectedValue * 0.0001f, 1.0f);
}
)";
const std::string ArraysOfArraysStateBufferVertexShader = R"(
struct VSInput {
    float3 position : position;
};

cbuffer ArraysOfArraysReflectionBuffer
{
    float scalarValues[2][3];
};

float4 main(VSInput IN) : SV_POSITION
{
    return float4(IN.position.xy, scalarValues[1][2], 1.0f);
}
)";

} // namespace

DEFINE_UNIT_TEST_WITH_BASE("Shader/ReflectionQueries", UnitTestBase)
{
	// Retrieve our important objects
	auto& renderer = session.Renderer();

	// Verify shader loading and compile-time contract failures before testing successful reflection.
	{
		auto emptyProgram = renderer.CreateShaderProgram();
		REQUIRE(!emptyProgram->CompileProgram());

		auto incompatibleTargetProgram = renderer.CreateShaderProgram();
		if (session.ApiFamily() == IRendererPlugin::ApiFamily::Direct3D)
		{
			REQUIRE(!incompatibleTargetProgram->ConfigureShaderTarget(ShaderTargetInfoOpenGL()));
		}
		else
		{
			REQUIRE(!incompatibleTargetProgram->ConfigureShaderTarget(ShaderTargetInfoDirect3D()));
		}

		auto duplicateStageProgram = renderer.CreateShaderProgram();
		REQUIRE(duplicateStageProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(ResourceArrayVertexShader)));
		REQUIRE(!duplicateStageProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(ResourceArrayVertexShader)));

		auto emptyShaderProgram = renderer.CreateShaderProgram();
		REQUIRE(!emptyShaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL("", 0)));

		auto vertexOnlyProgram = renderer.CreateShaderProgram();
		REQUIRE(vertexOnlyProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(ResourceArrayVertexShader)));
		REQUIRE(!vertexOnlyProgram->CompileProgram());

		auto fragmentOnlyProgram = renderer.CreateShaderProgram();
		REQUIRE(fragmentOnlyProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(MinimalFragmentShader)));
		REQUIRE(!fragmentOnlyProgram->CompileProgram());

		auto invalidSyntaxProgram = renderer.CreateShaderProgram();
		if (invalidSyntaxProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(InvalidHLSLShader)))
		{
			REQUIRE(invalidSyntaxProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(MinimalFragmentShader)));
			REQUIRE(!invalidSyntaxProgram->CompileProgram());
		}

		if (session.Device().IsFeatureSupported(IGraphicsDevice::Feature::ComputeShaders))
		{
			auto mixedComputeProgram = renderer.CreateShaderProgram();
			REQUIRE(mixedComputeProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(ResourceArrayVertexShader)));
			REQUIRE(mixedComputeProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(MinimalFragmentShader)));
			REQUIRE(mixedComputeProgram->LoadShaderStage(IShaderProgram::ShaderStage::Compute, ShaderSourceInfoHLSL(MinimalComputeShader)));
			REQUIRE(!mixedComputeProgram->CompileProgram());
		}
		else
		{
			session.AddTestSkipped("MixedGraphicsComputeShaderProgram", "Mixed graphics and compute shader compile validation was skipped, as the current renderer doesn't support compute shaders on this device.");
		}
		session.AddTestSuccess("ShaderCompilationContracts", "Shader programs rejected missing stages, duplicate stages, empty shader code, incompatible target configuration, invalid shader code, and invalid graphics/compute stage combinations.");
	}

	// Create and compile a shader program with each common binding category.
	auto shaderProgram = renderer.CreateShaderProgram();
	REQUIRE(shaderProgram->IsCodeFormatSupported(IShaderProgram::CodeFormat::HLSL));
	REQUIRE(shaderProgram->IsCodeFormatSupported(shaderProgram->PreferredCodeFormat()));
	if (session.ApiFamily() == IRendererPlugin::ApiFamily::Direct3D)
	{
		REQUIRE(shaderProgram->ConfigureShaderTarget(ShaderTargetInfoDirect3D()));
	}
	else if (session.ApiFamily() == IRendererPlugin::ApiFamily::OpenGL)
	{
		REQUIRE(shaderProgram->ConfigureShaderTarget(ShaderTargetInfoOpenGL()));
	}
	REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(ReflectionVertexShader)));
	REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(ReflectionFragmentShader)));
	REQUIRE(shaderProgram->CompileProgram());

	// Validate shader reflection query methods for positive and negative cases.
	REQUIRE(shaderProgram->VertexAttributeExists("position"));
	REQUIRE(shaderProgram->VertexAttributeExists("texCoord"));
	REQUIRE(!shaderProgram->VertexAttributeExists("missingPosition"));
	REQUIRE(shaderProgram->StateValueExists("looseColor"));
	REQUIRE(!shaderProgram->StateValueExists("missingLooseColor"));
	REQUIRE(shaderProgram->TextureExists("reflectionTexture"));
	REQUIRE(!shaderProgram->TextureExists("missingTexture"));
	REQUIRE(!shaderProgram->SamplerExists("reflectionTexture_CombinedSampler"));
	REQUIRE(!shaderProgram->SamplerExists("missingSampler"));
	REQUIRE(shaderProgram->StateBufferExists("ReflectionBuffer"));
	REQUIRE(!shaderProgram->StateBufferExists("MissingReflectionBuffer"));
	REQUIRE(!shaderProgram->VertexAttributeExists(""));
	REQUIRE(!shaderProgram->VertexAttributeExists("Position"));
	REQUIRE(!shaderProgram->StateValueExists(""));
	REQUIRE(!shaderProgram->StateValueExists("reflectionData.colorScale[999]"));
	REQUIRE(!shaderProgram->TextureExists(""));
	REQUIRE(!shaderProgram->TextureExists("ReflectionTexture"));
	REQUIRE(!shaderProgram->SamplerExists(""));
	REQUIRE(!shaderProgram->StateBufferExists(""));
	REQUIRE(!shaderProgram->StateBufferExists("ReflectionBuffer."));

	auto positionAttributeId = shaderProgram->GetVertexAttributeId("position");
	auto texCoordAttributeId = shaderProgram->GetVertexAttributeId("texCoord");
	auto looseColorStateId = shaderProgram->GetStateValueId("looseColor");
	auto reflectionTextureId = shaderProgram->GetTextureId("reflectionTexture");
	auto reflectionSamplerId = shaderProgram->GetSamplerId("reflectionTexture_CombinedSampler");
	auto reflectionBufferId = shaderProgram->GetStateBufferId("ReflectionBuffer");
	REQUIRE(positionAttributeId != texCoordAttributeId);
	REQUIRE(looseColorStateId != StateValueId::Null);
	REQUIRE(reflectionTextureId != TextureId::Null);
	REQUIRE(reflectionSamplerId == SamplerId::Null);
	REQUIRE(reflectionBufferId != StateBufferId::Null);
	REQUIRE(shaderProgram->GetVertexAttributeId("") == VertexAttributeId::Null);
	REQUIRE(shaderProgram->GetVertexAttributeId("Position") == VertexAttributeId::Null);
	REQUIRE(shaderProgram->GetStateValueId("") == StateValueId::Null);
	REQUIRE(shaderProgram->GetStateValueId("reflectionData.colorScale[999]") == StateValueId::Null);
	REQUIRE(shaderProgram->GetTextureId("") == TextureId::Null);
	REQUIRE(shaderProgram->GetTextureId("ReflectionTexture") == TextureId::Null);
	REQUIRE(shaderProgram->GetSamplerId("") == SamplerId::Null);
	REQUIRE(shaderProgram->GetStateBufferId("") == StateBufferId::Null);
	REQUIRE(shaderProgram->GetStateBufferId("ReflectionBuffer.") == StateBufferId::Null);

	auto stateBufferLayout = renderer.CreateStateBufferLayout();
	REQUIRE(shaderProgram->LoadStateBufferLayoutFromShader(reflectionBufferId, stateBufferLayout.get()));
	auto missingStateBufferLayout = renderer.CreateStateBufferLayout();
	REQUIRE(!shaderProgram->LoadStateBufferLayoutFromShader(StateBufferId::Null, missingStateBufferLayout.get()));
	session.AddTestSuccess("ShaderBindingQueries", "Shader reflection reports vertex attributes, state values, textures, samplers, and state buffers correctly.");

	// Compile a shader with scalar integer values, single-dimensional arrays, vector arrays, matrix arrays, and nested
	// structure arrays in a state buffer. This verifies the renderer reflection path used to reconstruct a reusable
	// state buffer layout from backend shader metadata without requiring shader arrays-of-arrays support.
	{
		auto complexStateBufferProgram = renderer.CreateShaderProgram();
		REQUIRE(complexStateBufferProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(ComplexStateBufferVertexShader)));
		REQUIRE(complexStateBufferProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(MinimalFragmentShader)));
		REQUIRE(complexStateBufferProgram->CompileProgram());
		REQUIRE(complexStateBufferProgram->StateBufferExists("ComplexReflectionBuffer"));
		auto complexStateBufferId = complexStateBufferProgram->GetStateBufferId("ComplexReflectionBuffer");
		REQUIRE(complexStateBufferId != StateBufferId::Null);
		auto complexStateBufferLayout = renderer.CreateStateBufferLayout();
		REQUIRE(complexStateBufferProgram->LoadStateBufferLayoutFromShader(complexStateBufferId, complexStateBufferLayout.get()));
		session.AddTestSuccess("ComplexStateBufferLayoutReflection", "Shader reflection reconstructed a state buffer layout containing scalar integer values, single-dimensional arrays, vector arrays, matrix arrays, and nested structure arrays.");
	}

	// Arrays of arrays are optional on OpenGL 3.3, where they require GL_ARB_arrays_of_arrays. Keep this reflection
	// check separate so devices without the feature still run the broader state-buffer reflection coverage above.
	if (session.Device().IsFeatureSupported(IGraphicsDevice::Feature::ShaderArraysOfArrays))
	{
		auto arraysOfArraysStateBufferProgram = renderer.CreateShaderProgram();
		REQUIRE(arraysOfArraysStateBufferProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(ArraysOfArraysStateBufferVertexShader)));
		REQUIRE(arraysOfArraysStateBufferProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(MinimalFragmentShader)));
		REQUIRE(arraysOfArraysStateBufferProgram->CompileProgram());
		REQUIRE(arraysOfArraysStateBufferProgram->StateBufferExists("ArraysOfArraysReflectionBuffer"));
		auto arraysOfArraysStateBufferId = arraysOfArraysStateBufferProgram->GetStateBufferId("ArraysOfArraysReflectionBuffer");
		REQUIRE(arraysOfArraysStateBufferId != StateBufferId::Null);
		auto arraysOfArraysStateBufferLayout = renderer.CreateStateBufferLayout();
		REQUIRE(arraysOfArraysStateBufferProgram->LoadStateBufferLayoutFromShader(arraysOfArraysStateBufferId, arraysOfArraysStateBufferLayout.get()));
		session.AddTestSuccess("ArraysOfArraysStateBufferLayoutReflection", "Shader reflection reconstructed a state buffer layout containing an array of arrays.");
	}
	else
	{
		session.AddTestSkipped("ArraysOfArraysStateBufferLayoutReflection", "This part of the test was skipped, as the current renderer doesn't support shader arrays of arrays on this device.");
	}

	// Compile a second state-buffer shader with each common vector width and matrix shape. The shader references every
	// value so the compiler has to preserve them, giving the backend reflection code direct coverage of the full set of
	// type conversions used when rebuilding state-buffer layouts.
	{
		auto wideStateBufferProgram = renderer.CreateShaderProgram();
		REQUIRE(wideStateBufferProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(WideStateBufferVertexShader)));
		REQUIRE(wideStateBufferProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(MinimalFragmentShader)));
		REQUIRE(wideStateBufferProgram->CompileProgram());
		REQUIRE(wideStateBufferProgram->StateBufferExists("WideReflectionBuffer"));
		auto wideStateBufferId = wideStateBufferProgram->GetStateBufferId("WideReflectionBuffer");
		REQUIRE(wideStateBufferId != StateBufferId::Null);
		auto wideStateBufferLayout = renderer.CreateStateBufferLayout();
		REQUIRE(wideStateBufferProgram->LoadStateBufferLayoutFromShader(wideStateBufferId, wideStateBufferLayout.get()));
		session.AddTestSuccess("WideStateBufferLayoutReflection", "Shader reflection reconstructed a state buffer layout containing boolean, integer, unsigned integer, and floating point vectors along with square and rectangular matrix values.");
	}

	// Direct3D exposes compiler and shader model target controls. Compile a small graphics program through the
	// legacy FXC path, and a DXC path when that renderer supports it, so the target settings are covered directly.
	if (session.ApiFamily() == IRendererPlugin::ApiFamily::Direct3D)
	{
		auto direct3DTargetFlags = ShaderTargetInfoDirect3D::Flags::ForceFXC | ShaderTargetInfoDirect3D::Flags::SkipOptimization | ShaderTargetInfoDirect3D::Flags::EnableDebugInfo;
		auto fxcTargetProgram = renderer.CreateShaderProgram();
		REQUIRE(fxcTargetProgram->ConfigureShaderTarget(ShaderTargetInfoDirect3D(5, 0, direct3DTargetFlags)));
		REQUIRE(fxcTargetProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(ResourceArrayVertexShader)));
		REQUIRE(fxcTargetProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(MinimalFragmentShader)));
		REQUIRE(fxcTargetProgram->CompileProgram());

		if (session.Device().IsFeatureSupported(IGraphicsDevice::Feature::ComputeShaders))
		{
			auto fxcComputeProgram = renderer.CreateShaderProgram();
			REQUIRE(fxcComputeProgram->ConfigureShaderTarget(ShaderTargetInfoDirect3D(5, 0, direct3DTargetFlags)));
			REQUIRE(fxcComputeProgram->LoadShaderStage(IShaderProgram::ShaderStage::Compute, ShaderSourceInfoHLSL(MinimalComputeShader)));
			REQUIRE(fxcComputeProgram->CompileProgram());
		}

		if (shaderProgram->IsCodeFormatSupported(IShaderProgram::CodeFormat::DXIL))
		{
			auto dxcTargetFlags = ShaderTargetInfoDirect3D::Flags::SkipOptimization | ShaderTargetInfoDirect3D::Flags::EnableDebugInfo;
			auto dxcTargetProgram = renderer.CreateShaderProgram();
			REQUIRE(dxcTargetProgram->ConfigureShaderTarget(ShaderTargetInfoDirect3D(dxcTargetFlags)));
			REQUIRE(dxcTargetProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(ResourceArrayVertexShader)));
			REQUIRE(dxcTargetProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(MinimalFragmentShader)));
			REQUIRE(dxcTargetProgram->CompileProgram());

			if (session.Device().IsFeatureSupported(IGraphicsDevice::Feature::ComputeShaders))
			{
				auto dxcComputeProgram = renderer.CreateShaderProgram();
				REQUIRE(dxcComputeProgram->ConfigureShaderTarget(ShaderTargetInfoDirect3D(dxcTargetFlags)));
				REQUIRE(dxcComputeProgram->LoadShaderStage(IShaderProgram::ShaderStage::Compute, ShaderSourceInfoHLSL(MinimalComputeShader)));
				REQUIRE(dxcComputeProgram->CompileProgram());
			}
		}
		session.AddTestSuccess("Direct3DShaderTargetVariants", "Direct3D shader target settings compiled graphics and compute shaders through FXC and DXC target variants where supported.");
	}
	else
	{
		session.AddTestSkipped("Direct3DShaderTargetVariants", "Direct3D shader target variant coverage was skipped, as the current renderer doesn't use Direct3D shader target settings.");
	}

	// Resource arrays are feature dependent, so keep that part of the reflection coverage conditional.
	if (!session.Device().IsFeatureSupported(IGraphicsDevice::Feature::ResourceArrays))
	{
		session.AddTestSkipped("ResourceArrayQueries", "This part of the test was skipped, as the current renderer doesn't support resource arrays on this device.");
		return true;
	}

	auto resourceArrayShaderProgram = renderer.CreateShaderProgram();
	REQUIRE(resourceArrayShaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(ResourceArrayVertexShader)));
	REQUIRE(resourceArrayShaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(ResourceArrayFragmentShader)));
	REQUIRE(resourceArrayShaderProgram->CompileProgram());
	REQUIRE(resourceArrayShaderProgram->ResourceArrayExists("reflectionResourceArray"));
	REQUIRE(!resourceArrayShaderProgram->ResourceArrayExists("missingResourceArray"));
	auto resourceArrayId = resourceArrayShaderProgram->GetResourceArrayId("reflectionResourceArray");
	REQUIRE(resourceArrayId != ResourceArrayId::Null);
	session.AddTestSuccess("ResourceArrayQueries", "Shader reflection reports resource arrays correctly.");
	return true;
}

} // namespace cobalt::graphics::testing
