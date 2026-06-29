// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../UnitTestBase.h"
#include <array>
#include <vector>
namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

namespace {

// Define our shader programs
const std::string Texture1DFragmentShader = R"(
uniform Texture1D colorTexture;
uniform SamplerState colorTexture_CombinedSampler;

struct VSOutput {
    float4 position : SV_POSITION;
    float2 texCoord : texCoord;
};

float4 main(VSOutput IN) : SV_TARGET0
{
    return colorTexture.Sample(colorTexture_CombinedSampler, IN.texCoord.x);
}
)";
const std::string Texture1DArrayFragmentShader = R"(
uniform Texture1DArray colorTexture;
uniform SamplerState colorTexture_CombinedSampler;
uniform float sampleLayer;

struct VSOutput {
    float4 position : SV_POSITION;
    float2 texCoord : texCoord;
};

float4 main(VSOutput IN) : SV_TARGET0
{
    return colorTexture.Sample(colorTexture_CombinedSampler, float2(IN.texCoord.x, sampleLayer));
}
)";
const std::string Texture2DArrayFragmentShader = R"(
uniform Texture2DArray colorTexture;
uniform SamplerState colorTexture_CombinedSampler;
uniform float sampleLayer;

struct VSOutput {
    float4 position : SV_POSITION;
    float2 texCoord : texCoord;
};

float4 main(VSOutput IN) : SV_TARGET0
{
    return colorTexture.Sample(colorTexture_CombinedSampler, float3(IN.texCoord.xy, sampleLayer));
}
)";
const std::string Texture3DFragmentShader = R"(
uniform Texture3D colorTexture;
uniform SamplerState colorTexture_CombinedSampler;
uniform float sampleDepth;

struct VSOutput {
    float4 position : SV_POSITION;
    float2 texCoord : texCoord;
};

float4 main(VSOutput IN) : SV_TARGET0
{
    return colorTexture.Sample(colorTexture_CombinedSampler, float3(IN.texCoord.xy, sampleDepth));
}
)";
const std::string TextureCubeFragmentShader = R"(
uniform TextureCube colorTexture;
uniform SamplerState colorTexture_CombinedSampler;
uniform uint faceIndex;

struct VSOutput {
    float4 position : SV_POSITION;
    float2 texCoord : texCoord;
};

float3 GetCubeDirection(float2 texCoord, uint selectedFace)
{
    float2 cubeCoord = (texCoord * 2.0f) - 1.0f;
    cubeCoord.y = -cubeCoord.y;

    if (selectedFace == 0)
    {
        return normalize(float3(1.0f, cubeCoord.y, -cubeCoord.x));
    }
    if (selectedFace == 1)
    {
        return normalize(float3(-1.0f, cubeCoord.y, cubeCoord.x));
    }
    if (selectedFace == 2)
    {
        return normalize(float3(cubeCoord.x, 1.0f, -cubeCoord.y));
    }
    if (selectedFace == 3)
    {
        return normalize(float3(cubeCoord.x, -1.0f, cubeCoord.y));
    }
    if (selectedFace == 4)
    {
        return normalize(float3(cubeCoord.x, cubeCoord.y, 1.0f));
    }
    return normalize(float3(-cubeCoord.x, cubeCoord.y, -1.0f));
}

float4 main(VSOutput IN) : SV_TARGET0
{
    return colorTexture.Sample(colorTexture_CombinedSampler, GetCubeDirection(IN.texCoord.xy, faceIndex));
}
)";
const std::string TextureCubeArrayFragmentShader = R"(
uniform TextureCubeArray colorTexture;
uniform SamplerState colorTexture_CombinedSampler;
uniform uint faceIndex;
uniform float sampleLayer;

struct VSOutput {
    float4 position : SV_POSITION;
    float2 texCoord : texCoord;
};

float3 GetCubeDirection(float2 texCoord, uint selectedFace)
{
    float2 cubeCoord = (texCoord * 2.0f) - 1.0f;
    cubeCoord.y = -cubeCoord.y;

    if (selectedFace == 0)
    {
        return normalize(float3(1.0f, cubeCoord.y, -cubeCoord.x));
    }
    if (selectedFace == 1)
    {
        return normalize(float3(-1.0f, cubeCoord.y, cubeCoord.x));
    }
    if (selectedFace == 2)
    {
        return normalize(float3(cubeCoord.x, 1.0f, -cubeCoord.y));
    }
    if (selectedFace == 3)
    {
        return normalize(float3(cubeCoord.x, -1.0f, cubeCoord.y));
    }
    if (selectedFace == 4)
    {
        return normalize(float3(cubeCoord.x, cubeCoord.y, 1.0f));
    }
    return normalize(float3(-cubeCoord.x, cubeCoord.y, -1.0f));
}

float4 main(VSOutput IN) : SV_TARGET0
{
    return colorTexture.Sample(colorTexture_CombinedSampler, float4(GetCubeDirection(IN.texCoord.xy, faceIndex), sampleLayer));
}
)";

const std::array<ITextureBuffer::CubeMapFace, 6> CubeFaces = {
  ITextureBuffer::CubeMapFace::PositiveX,
  ITextureBuffer::CubeMapFace::NegativeX,
  ITextureBuffer::CubeMapFace::PositiveY,
  ITextureBuffer::CubeMapFace::NegativeY,
  ITextureBuffer::CubeMapFace::PositiveZ,
  ITextureBuffer::CubeMapFace::NegativeZ};

} // namespace

DEFINE_UNIT_TEST_WITH_BASE("Resources/Images/TextureUpdateCoverage", UnitTestBase)
{
	// Retrieve our important objects
	auto& renderer = session.Renderer();

	// Exercise direct partial updates on a 1D texture, including a visible base-level update and a non-zero mip level
	// update which is flushed through the same upload pathway.
	{
		TexturedQuadSceneHelper::Scene scene;
		REQUIRE(TexturedQuad().InitializeScene(this, session, Geometry(), Texture1DFragmentShader, scene));

		auto texture = renderer.CreateTextureBuffer1D();
		texture->SetUsageFlags(ITextureBuffer::UsageFlags::ShaderInput);
		texture->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
		texture->SetTextureDimensions(V1UInt32(4), 2);
		texture->SetPerformanceHints(ITextureBuffer::PerformanceHint::WriteOften | ITextureBuffer::PerformanceHint::ReadNever, ITextureBuffer::PerformanceHint::WriteNever | ITextureBuffer::PerformanceHint::ReadOften);
		std::vector<V4UInt8> level0Data(4, V4UInt8(0, 0, 255, 255));
		std::vector<V4UInt8> level1Data(2, V4UInt8(0, 0, 128, 255));
		REQUIRE(texture->SetInitialData(level0Data, 0));
		REQUIRE(texture->SetInitialData(level1Data, 1));
		REQUIRE(texture->AllocateMemory());

		auto sampler = renderer.CreateTextureSampler1D();
		sampler->SetTextureFilterMode(ITextureSampler::FilterMode::Nearest, ITextureSampler::FilterMode::Nearest);
		sampler->SetTextureWrapMode(ITextureSampler::WrapMode::ClampToEdge);
		scene.renderableNode->BindTextureWithCombinedSampler(scene.shaderProgram->GetTextureId("colorTexture"), texture.get(), sampler.get());

		std::vector<V4UInt8> partialUpdate = {V4UInt8(255, 0, 0, 255), V4UInt8(0, 255, 0, 255)};
		REQUIRE(texture->QueueDataUpdate(partialUpdate, 0, V1UInt32(1), V1UInt32(2)));
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("Texture1DBaseLevelPartialUpdate", "A 1D texture with blue outer texels and red and green texels from a partial base-level update.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

		REQUIRE(texture->QueueDataUpdate(partialUpdate, 1, V1UInt32(0), V1UInt32(0)));
		sampler->SetTextureMipmapMode(ITextureSampler::MipmapMode::Nearest);
		sampler->SetMipmapLevelMapping(1.0f, 1.0f, 0.0f);
		frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("Texture1DNonZeroMipPartialUpdate", "A 1D texture sampled from mip level 1 after a direct partial update to the non-zero mipmap level.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);
		renderer.RemoveAllRenderPasses();
		scene = TexturedQuadSceneHelper::Scene();
		renderer.WaitForDeferredDeletionComplete();
	}

	// Exercise direct partial updates on an array texture with a non-zero array layer.
	{
		TexturedQuadSceneHelper::Scene scene;
		REQUIRE(TexturedQuad().InitializeScene(this, session, Geometry(), Texture1DArrayFragmentShader, scene));

		auto texture = renderer.CreateTextureBuffer1DArray();
		texture->SetUsageFlags(ITextureBuffer::UsageFlags::ShaderInput);
		texture->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
		texture->SetTextureDimensions(V1UInt32(4), 2);
		texture->SetPerformanceHints(ITextureBuffer::PerformanceHint::WriteOften | ITextureBuffer::PerformanceHint::ReadNever, ITextureBuffer::PerformanceHint::WriteNever | ITextureBuffer::PerformanceHint::ReadOften);
		std::vector<V4UInt8> layerData(4, V4UInt8(0, 0, 255, 255));
		REQUIRE(texture->SetInitialData(layerData, 0));
		REQUIRE(texture->SetInitialData(layerData, 1));
		REQUIRE(texture->AllocateMemory());

		auto sampler = renderer.CreateTextureSampler1DArray();
		sampler->SetTextureFilterMode(ITextureSampler::FilterMode::Nearest, ITextureSampler::FilterMode::Nearest);
		sampler->SetTextureWrapMode(ITextureSampler::WrapMode::ClampToEdge);
		scene.renderableNode->BindTextureWithCombinedSampler(scene.shaderProgram->GetTextureId("colorTexture"), texture.get(), sampler.get());
		scene.renderableNode->SetStateValue(scene.shaderProgram->GetStateValueId("sampleLayer"), V1Float32(1.0f));

		std::vector<V4UInt8> partialUpdate = {V4UInt8(255, 255, 0, 255), V4UInt8(255, 0, 255, 255)};
		REQUIRE(texture->QueueDataUpdate(partialUpdate, (size_t)1, 0, V1UInt32(1), V1UInt32(2)));
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("Texture1DArrayLayerPartialUpdate", "A 1D-array layer with blue outer texels and yellow and magenta texels from a partial layer update.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);
		renderer.RemoveAllRenderPasses();
		scene = TexturedQuadSceneHelper::Scene();
		renderer.WaitForDeferredDeletionComplete();
	}

	// Exercise direct partial updates on a 2D array texture using an explicit source format conversion.
	{
		TexturedQuadSceneHelper::Scene scene;
		REQUIRE(TexturedQuad().InitializeScene(this, session, Geometry(), Texture2DArrayFragmentShader, scene));

		auto texture = renderer.CreateTextureBuffer2DArray();
		texture->SetUsageFlags(ITextureBuffer::UsageFlags::ShaderInput);
		texture->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
		texture->SetTextureDimensions(V2UInt32(2, 2), 2);
		texture->SetPerformanceHints(ITextureBuffer::PerformanceHint::WriteOften | ITextureBuffer::PerformanceHint::ReadNever, ITextureBuffer::PerformanceHint::WriteNever | ITextureBuffer::PerformanceHint::ReadOften);
		std::vector<V4UInt8> layerData(4, V4UInt8(0, 0, 255, 255));
		REQUIRE(texture->SetInitialData(layerData, 0));
		REQUIRE(texture->SetInitialData(layerData, 1));
		REQUIRE(texture->AllocateMemory());

		auto sampler = renderer.CreateTextureSampler2DArray();
		sampler->SetTextureFilterMode(ITextureSampler::FilterMode::Nearest, ITextureSampler::FilterMode::Nearest);
		sampler->SetTextureWrapMode(ITextureSampler::WrapMode::ClampToEdge, ITextureSampler::WrapMode::ClampToEdge);
		scene.renderableNode->BindTextureWithCombinedSampler(scene.shaderProgram->GetTextureId("colorTexture"), texture.get(), sampler.get());
		scene.renderableNode->SetStateValue(scene.shaderProgram->GetStateValueId("sampleLayer"), V1Float32(1.0f));

		std::vector<V4UInt8> bgraUpdate = {V4UInt8(0, 0, 255, 255), V4UInt8(0, 255, 0, 255)};
		REQUIRE(texture->QueueDataUpdate(bgraUpdate.data(), bgraUpdate.size() * sizeof(V4UInt8), ITextureBuffer::SourceImageFormat::BGRA, ITextureBuffer::SourceDataFormat::UNorm8, (size_t)1, 0, V2UInt32(0, 1), V2UInt32(2, 1)));
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("Texture2DArrayConvertedPartialUpdate", "A 2D-array layer with a red and green row produced by a BGRA-to-RGBA converted partial update.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);
		renderer.RemoveAllRenderPasses();
		scene = TexturedQuadSceneHelper::Scene();
		renderer.WaitForDeferredDeletionComplete();
	}

	// Exercise direct partial updates on a 3D texture.
	{
		TexturedQuadSceneHelper::Scene scene;
		REQUIRE(TexturedQuad().InitializeScene(this, session, Geometry(), Texture3DFragmentShader, scene));

		auto texture = renderer.CreateTextureBuffer3D();
		texture->SetUsageFlags(ITextureBuffer::UsageFlags::ShaderInput);
		texture->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
		texture->SetTextureDimensions(V3UInt32(2, 2, 2));
		texture->SetPerformanceHints(ITextureBuffer::PerformanceHint::WriteOften | ITextureBuffer::PerformanceHint::ReadNever, ITextureBuffer::PerformanceHint::WriteNever | ITextureBuffer::PerformanceHint::ReadOften);
		std::vector<V4UInt8> textureData(8, V4UInt8(0, 0, 255, 255));
		REQUIRE(texture->SetInitialData(textureData));
		REQUIRE(texture->AllocateMemory());

		auto sampler = renderer.CreateTextureSampler3D();
		sampler->SetTextureFilterMode(ITextureSampler::FilterMode::Nearest, ITextureSampler::FilterMode::Nearest);
		sampler->SetTextureWrapMode(ITextureSampler::WrapMode::ClampToEdge, ITextureSampler::WrapMode::ClampToEdge, ITextureSampler::WrapMode::ClampToEdge);
		scene.renderableNode->BindTextureWithCombinedSampler(scene.shaderProgram->GetTextureId("colorTexture"), texture.get(), sampler.get());
		scene.renderableNode->SetStateValue(scene.shaderProgram->GetStateValueId("sampleDepth"), V1Float32(1.0f));

		std::vector<V4UInt8> partialUpdate = {V4UInt8(0, 255, 255, 255), V4UInt8(255, 128, 0, 255)};
		REQUIRE(texture->QueueDataUpdate(partialUpdate, 0, V3UInt32(0, 1, 1), V3UInt32(2, 1, 1)));
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("Texture3DDepthSlicePartialUpdate", "A 3D texture slice with a cyan and orange row from a partial depth-slice update.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);
		renderer.RemoveAllRenderPasses();
		scene = TexturedQuadSceneHelper::Scene();
		renderer.WaitForDeferredDeletionComplete();
	}

	// Exercise direct partial updates on a cubemap face.
	{
		TexturedQuadSceneHelper::Scene scene;
		REQUIRE(TexturedQuad().InitializeScene(this, session, Geometry(), TextureCubeFragmentShader, scene));

		auto texture = renderer.CreateTextureBufferCube();
		texture->SetUsageFlags(ITextureBuffer::UsageFlags::ShaderInput);
		texture->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
		texture->SetTextureDimensions(2);
		texture->SetPerformanceHints(ITextureBuffer::PerformanceHint::WriteOften | ITextureBuffer::PerformanceHint::ReadNever, ITextureBuffer::PerformanceHint::WriteNever | ITextureBuffer::PerformanceHint::ReadOften);
		std::vector<V4UInt8> faceData(4, V4UInt8(0, 0, 255, 255));
		for (auto cubeFace : CubeFaces)
		{
			REQUIRE(texture->SetInitialData(faceData, cubeFace));
		}
		REQUIRE(texture->AllocateMemory());

		auto sampler = renderer.CreateTextureSamplerCube();
		sampler->SetTextureFilterMode(ITextureSampler::FilterMode::Nearest, ITextureSampler::FilterMode::Nearest);
		sampler->SetTextureWrapMode(ITextureSampler::WrapMode::ClampToEdge, ITextureSampler::WrapMode::ClampToEdge);
		scene.renderableNode->BindTextureWithCombinedSampler(scene.shaderProgram->GetTextureId("colorTexture"), texture.get(), sampler.get());
		scene.renderableNode->SetStateValue(scene.shaderProgram->GetStateValueId("faceIndex"), V1UInt32(5));

		std::vector<V4UInt8> partialUpdate = {V4UInt8(255, 255, 255, 255), V4UInt8(0, 0, 0, 255)};
		REQUIRE(texture->QueueDataUpdate(partialUpdate, ITextureBuffer::CubeMapFace::NegativeZ, 0, V2UInt32(0, 1), V2UInt32(2, 1)));
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("TextureCubeFacePartialUpdate", "A cubemap face with a white and black row from a partial face update.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);
		renderer.RemoveAllRenderPasses();
		scene = TexturedQuadSceneHelper::Scene();
		renderer.WaitForDeferredDeletionComplete();
	}

	// Exercise direct partial updates on a cube-array texture where that optional resource family is supported.
	if (!session.Device().IsFeatureSupported(IGraphicsDevice::Feature::TextureCubeArray))
	{
		session.AddTestSkipped("TextureCubeArrayDirectPartialUpdates", "Cube-array texture update coverage was skipped, as the current renderer doesn't support cube-array textures on this device.");
	}
	else
	{
		TexturedQuadSceneHelper::Scene scene;
		REQUIRE(TexturedQuad().InitializeScene(this, session, Geometry(), TextureCubeArrayFragmentShader, scene));

		auto texture = renderer.CreateTextureBufferCubeArray();
		texture->SetUsageFlags(ITextureBuffer::UsageFlags::ShaderInput);
		texture->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
		texture->SetTextureDimensions(2, 2);
		texture->SetPerformanceHints(ITextureBuffer::PerformanceHint::WriteOften | ITextureBuffer::PerformanceHint::ReadNever, ITextureBuffer::PerformanceHint::WriteNever | ITextureBuffer::PerformanceHint::ReadOften);
		std::vector<V4UInt8> faceData(4, V4UInt8(0, 0, 255, 255));
		for (size_t arrayIndex = 0; arrayIndex < 2; ++arrayIndex)
		{
			for (auto cubeFace : CubeFaces)
			{
				REQUIRE(texture->SetInitialData(faceData, cubeFace, arrayIndex));
			}
		}
		REQUIRE(texture->AllocateMemory());

		auto sampler = renderer.CreateTextureSamplerCubeArray();
		sampler->SetTextureFilterMode(ITextureSampler::FilterMode::Nearest, ITextureSampler::FilterMode::Nearest);
		sampler->SetTextureWrapMode(ITextureSampler::WrapMode::ClampToEdge, ITextureSampler::WrapMode::ClampToEdge);
		scene.renderableNode->BindTextureWithCombinedSampler(scene.shaderProgram->GetTextureId("colorTexture"), texture.get(), sampler.get());
		scene.renderableNode->SetStateValue(scene.shaderProgram->GetStateValueId("faceIndex"), V1UInt32(2));
		scene.renderableNode->SetStateValue(scene.shaderProgram->GetStateValueId("sampleLayer"), V1Float32(1.0f));

		std::vector<V4UInt8> partialUpdate = {V4UInt8(255, 0, 0, 255), V4UInt8(0, 255, 0, 255)};
		REQUIRE(texture->QueueDataUpdate(partialUpdate, ITextureBuffer::CubeMapFace::PositiveY, (size_t)1, 0, V2UInt32(0, 1), V2UInt32(2, 1)));
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("TextureCubeArrayLayerFacePartialUpdate", "A cube-array face with a red and green row from a partial update on layer 1.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);
		renderer.RemoveAllRenderPasses();
		scene = TexturedQuadSceneHelper::Scene();
		renderer.WaitForDeferredDeletionComplete();
	}

	renderer.WaitForDeferredDeletionComplete();
	return true;
}

} // namespace cobalt::graphics::testing
