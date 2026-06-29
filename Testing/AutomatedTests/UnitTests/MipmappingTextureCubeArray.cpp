// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../UnitTestBase.h"
namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

namespace {
constexpr int TestMipmapLevelCount = 4;
constexpr float TestMaxMipmapLevel = (float)(TestMipmapLevelCount - 1);

const std::string FragmentShader = R"(
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

const std::string PerspectiveVertexShader = R"(
struct VSInput {
    float3 position : position;
};

struct VSOutput {
    float4 position : SV_POSITION;
    float3 sampleDirection : sampleDirection;
};

uniform row_major float4x4 viewProj;

VSOutput main(VSInput IN)
{
    VSOutput OUT;

    OUT.position = mul(viewProj, float4(IN.position, 1.0f));
    OUT.sampleDirection = IN.position;

    return OUT;
}
)";

const std::string PerspectiveFragmentShader = R"(
uniform TextureCubeArray colorTexture;
uniform SamplerState colorTexture_CombinedSampler;
uniform float sampleLayer;

struct VSOutput {
    float4 position : SV_POSITION;
    float3 sampleDirection : sampleDirection;
};

float4 main(VSOutput IN) : SV_TARGET0
{
    return colorTexture.Sample(colorTexture_CombinedSampler, float4(normalize(IN.sampleDirection), sampleLayer));
}
)";
} // namespace

DEFINE_UNIT_TEST_WITH_BASE("Resources/Mipmapping/TextureCubeArray", UnitTestBase)
{
	if (!session.Device().IsFeatureSupported(IGraphicsDevice::Feature::TextureCubeArray))
	{
		session.AddTestSkipped("Skipped test", "This test was skipped, as the current renderer doesn't support cube texture arrays.");
		return true;
	}

	// Retrieve our important objects.
	auto& renderer = session.Renderer();
	bool mipmapLevelBiasSupported = session.Device().IsFeatureSupported(IGraphicsDevice::Feature::MipmapLevelBias);
	auto skipMipmapLevelBiasTest = [&](const std::string& testName) {
		session.AddTestSkipped(testName, "This part of the test was skipped, as the current renderer doesn't support mipmap level bias on this device.");
	};

	// Define a scene for rendering a screen-aligned quad.
	TexturedQuadSceneHelper::Scene scene;
	REQUIRE(TexturedQuad().InitializeScene(this, session, Geometry(), FragmentShader, scene));

	// Create our cube texture array and populate one layer with deliberately high-contrast face mipmap data.
	const uint32_t faceLength = 256;
	auto texture = renderer.CreateTextureBufferCubeArray();
	texture->SetTextureDimensions(faceLength, 2, TestMipmapLevelCount);
	texture->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
	texture->SetUsageFlags(ITextureBuffer::UsageFlags::ShaderInput);
	texture->SetPerformanceHints(ITextureBuffer::PerformanceHint::WriteRarely | ITextureBuffer::PerformanceHint::ReadNever, ITextureBuffer::PerformanceHint::WriteNever | ITextureBuffer::PerformanceHint::ReadOften);
	REQUIRE(texture->AllocateMemory());
	std::vector<std::array<std::vector<V4UInt8>, 6>> mipLevelData;
	mipLevelData.resize(TestMipmapLevelCount);
	for (int mipmapLevel = 0; mipmapLevel < TestMipmapLevelCount; ++mipmapLevel)
	{
		auto mipFaceDimensions = Mipmapping().MipDimensions(V2UInt32(faceLength, faceLength), mipmapLevel);
		mipLevelData[(size_t)mipmapLevel][0] = Mipmapping().CreateMipLevelBandData2D(mipFaceDimensions, mipmapLevel + 0);
		mipLevelData[(size_t)mipmapLevel][1] = Mipmapping().CreateMipLevelBandData2D(mipFaceDimensions, mipmapLevel + 1);
		mipLevelData[(size_t)mipmapLevel][2] = Mipmapping().CreateMipLevelBandData2D(mipFaceDimensions, mipmapLevel + 2);
		mipLevelData[(size_t)mipmapLevel][3] = Mipmapping().CreateMipLevelBandData2D(mipFaceDimensions, mipmapLevel + 3);
		mipLevelData[(size_t)mipmapLevel][4] = Mipmapping().CreateMipLevelBandData2D(mipFaceDimensions, mipmapLevel + 1);
		mipLevelData[(size_t)mipmapLevel][5] = Mipmapping().CreateMipLevelBandData2D(mipFaceDimensions, mipmapLevel + 2);
		REQUIRE(texture->QueueDataUpdate(mipLevelData[(size_t)mipmapLevel][0], ITextureBuffer::CubeMapFace::PositiveX, 1, mipmapLevel));
		REQUIRE(texture->QueueDataUpdate(mipLevelData[(size_t)mipmapLevel][1], ITextureBuffer::CubeMapFace::NegativeX, 1, mipmapLevel));
		REQUIRE(texture->QueueDataUpdate(mipLevelData[(size_t)mipmapLevel][2], ITextureBuffer::CubeMapFace::PositiveY, 1, mipmapLevel));
		REQUIRE(texture->QueueDataUpdate(mipLevelData[(size_t)mipmapLevel][3], ITextureBuffer::CubeMapFace::NegativeY, 1, mipmapLevel));
		REQUIRE(texture->QueueDataUpdate(mipLevelData[(size_t)mipmapLevel][4], ITextureBuffer::CubeMapFace::PositiveZ, 1, mipmapLevel));
		REQUIRE(texture->QueueDataUpdate(mipLevelData[(size_t)mipmapLevel][5], ITextureBuffer::CubeMapFace::NegativeZ, 1, mipmapLevel));
	}

	// Create a sampler to read the cube texture array from the shader.
	auto sampler = renderer.CreateTextureSamplerCubeArray();
	sampler->SetTextureWrapMode(ITextureSampler::WrapMode::ClampToEdge, ITextureSampler::WrapMode::ClampToEdge);
	sampler->SetTextureFilterMode(ITextureSampler::FilterMode::Nearest, ITextureSampler::FilterMode::Nearest);
	scene.renderableNode->BindTextureWithCombinedSampler(scene.shaderProgram->GetTextureId("colorTexture"), texture.get(), sampler.get());
	scene.renderableNode->SetStateValue(scene.shaderProgram->GetStateValueId("faceIndex"), V1UInt32(0));
	scene.renderableNode->SetStateValue(scene.shaderProgram->GetStateValueId("sampleLayer"), V1Float32(1.0f));

	// Helper to pick flat-quad scales that target specific mip levels while sampling a chosen cube face.
	auto scaleForLevel = [&](float targetMipLevel) {
		return Mipmapping().ComputeQuadScaleForMipLevel(session.TestWindowSize(), V2UInt32(faceLength, faceLength), targetMipLevel, 2.0f, 2.0f);
	};

	// Capture images showing the effect of each mipmap mode on a flat screen-aligned quad.
	REQUIRE(TexturedQuad().UpdatePositionScale(scene, scaleForLevel(2.0f)));
	sampler->SetTextureMipmapMode(ITextureSampler::MipmapMode::None);
	sampler->SetMipmapLevelMapping(0.0f, TestMaxMipmapLevel, 0.0f);
	auto frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("MipmapNone", "A cube texture array sampled with mipmapping disabled.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	REQUIRE(TexturedQuad().UpdatePositionScale(scene, scaleForLevel(1.25f)));
	sampler->SetTextureMipmapMode(ITextureSampler::MipmapMode::Nearest);
	sampler->SetMipmapLevelMapping(0.0f, TestMaxMipmapLevel, 0.0f);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("MipmapNearest", "A cube texture array sampled with nearest mipmap selection.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	sampler->SetTextureMipmapMode(ITextureSampler::MipmapMode::Linear);
	sampler->SetMipmapLevelMapping(0.0f, TestMaxMipmapLevel, 0.0f);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("MipmapLinear", "A cube texture array sampled with linear mipmap blending.", std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.95);

	REQUIRE(TexturedQuad().UpdatePositionScale(scene, scaleForLevel(0.0f)));
	sampler->SetTextureMipmapMode(ITextureSampler::MipmapMode::Nearest);
	sampler->SetMipmapLevelMapping(2.0f, TestMaxMipmapLevel, 0.0f);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("MipmapMinLevel", "A cube texture array sampled with the minimum mipmap level clamped to level 2.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	REQUIRE(TexturedQuad().UpdatePositionScale(scene, scaleForLevel(3.0f)));
	sampler->SetMipmapLevelMapping(0.0f, 1.0f, 0.0f);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("MipmapMaxLevel", "A cube texture array sampled with the maximum mipmap level clamped to level 1.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	if (mipmapLevelBiasSupported)
	{
		REQUIRE(TexturedQuad().UpdatePositionScale(scene, scaleForLevel(1.0f)));
		sampler->SetMipmapLevelMapping(0.0f, TestMaxMipmapLevel, 1.0f);
		frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("MipmapBiasPositive", "A cube texture array sampled with a positive mipmap level bias.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

		REQUIRE(TexturedQuad().UpdatePositionScale(scene, scaleForLevel(2.0f)));
		sampler->SetMipmapLevelMapping(0.0f, TestMaxMipmapLevel, -1.0f);
		frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("MipmapBiasNegative", "A cube texture array sampled with a negative mipmap level bias.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);
	}
	else
	{
		skipMipmapLevelBiasTest("MipmapBiasPositive");
		skipMipmapLevelBiasTest("MipmapBiasNegative");
	}

	// Clear the scene content, and wait for destruction to be complete. We need to do this so we can unbind and re-bind
	// to the same window here without rendering a frame.
	renderer.RemoveAllRenderPasses();
	scene = TexturedQuadSceneHelper::Scene();
	renderer.WaitForDeferredDeletionComplete();

	// Define a curved display scene
	MipmappingHelper::CubeMapDisplayScene perspectiveScene;
	REQUIRE(Mipmapping().InitializeCubeMapDisplayScene(this, session, Geometry(), PerspectiveVertexShader, PerspectiveFragmentShader, perspectiveScene));
	perspectiveScene.renderableNode->BindTextureWithCombinedSampler(perspectiveScene.shaderProgram->GetTextureId("colorTexture"), texture.get(), sampler.get());
	perspectiveScene.renderableNode->SetStateValue(perspectiveScene.shaderProgram->GetStateValueId("sampleLayer"), V1Float32(1.0f));
	auto viewProj = Transform().LookAtCenterPerspective(session.TestWindowSizeAsFloat(), V3Float32(6.0f, -6.0f, 6.0f), 60.0f);
	perspectiveScene.renderableNode->SetStateValue(perspectiveScene.shaderProgram->GetStateValueId("viewProj"), viewProj);

	// Capture images showing the same mipmap modes while viewing the cubemap on curved geometry.
	sampler->SetTextureFilterMode(ITextureSampler::FilterMode::Nearest, ITextureSampler::FilterMode::Nearest);
	sampler->SetTextureMipmapMode(ITextureSampler::MipmapMode::None);
	sampler->SetMipmapLevelMapping(0.0f, TestMaxMipmapLevel, 0.0f);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	perspectiveScene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("PerspectiveMipmapNone", "A cube texture array sampled on curved geometry with mipmapping disabled.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	sampler->SetTextureMipmapMode(ITextureSampler::MipmapMode::Nearest);
	sampler->SetMipmapLevelMapping(0.0f, TestMaxMipmapLevel, 0.0f);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	perspectiveScene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("PerspectiveMipmapNearest", "A cube texture array sampled on curved geometry with nearest mipmap selection.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	sampler->SetTextureMipmapMode(ITextureSampler::MipmapMode::Linear);
	sampler->SetMipmapLevelMapping(0.0f, TestMaxMipmapLevel, 0.0f);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	perspectiveScene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("PerspectiveMipmapLinear", "A cube texture array sampled on curved geometry with linear mipmap blending.", std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.95);

	sampler->SetTextureMipmapMode(ITextureSampler::MipmapMode::Nearest);
	sampler->SetMipmapLevelMapping(2.0f, TestMaxMipmapLevel, 0.0f);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	perspectiveScene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("PerspectiveMipmapMinLevel", "A cube texture array sampled on curved geometry with the minimum mipmap level clamped to level 2.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	sampler->SetMipmapLevelMapping(0.0f, 1.0f, 0.0f);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	perspectiveScene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("PerspectiveMipmapMaxLevel", "A cube texture array sampled on curved geometry with the maximum mipmap level clamped to level 1.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	if (mipmapLevelBiasSupported)
	{
		sampler->SetMipmapLevelMapping(0.0f, TestMaxMipmapLevel, 1.0f);
		frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		perspectiveScene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("PerspectiveMipmapBiasPositive", "A cube texture array sampled on curved geometry with a positive mipmap level bias.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.90);

		sampler->SetMipmapLevelMapping(0.0f, TestMaxMipmapLevel, -1.0f);
		frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		perspectiveScene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("PerspectiveMipmapBiasNegative", "A cube texture array sampled on curved geometry with a negative mipmap level bias.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.90);
	}
	else
	{
		skipMipmapLevelBiasTest("PerspectiveMipmapBiasPositive");
		skipMipmapLevelBiasTest("PerspectiveMipmapBiasNegative");
	}

	renderer.RemoveAllRenderPasses();
	return true;
}

} // namespace cobalt::graphics::testing
