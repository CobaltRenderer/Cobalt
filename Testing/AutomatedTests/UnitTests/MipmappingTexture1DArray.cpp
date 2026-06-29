// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../UnitTestBase.h"
namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

namespace {
constexpr int TestMipmapLevelCount = 4;
constexpr float TestMaxMipmapLevel = (float)(TestMipmapLevelCount - 1);

const std::string FragmentShader = R"(
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

const std::string PerspectiveFragmentShader = R"(
uniform Texture1DArray colorTexture;
uniform SamplerState colorTexture_CombinedSampler;
uniform float sampleLayer;

struct VSOutput {
    float4 position : SV_POSITION;
    float2 texCoord : texCoord;
};

float4 main(VSOutput IN) : SV_TARGET0
{
    return colorTexture.Sample(colorTexture_CombinedSampler, float2(IN.texCoord.y, sampleLayer));
}
)";
} // namespace

DEFINE_UNIT_TEST_WITH_BASE("Resources/Mipmapping/Texture1DArray", UnitTestBase)
{
	// Retrieve our important objects.
	auto& renderer = session.Renderer();
	bool mipmapLevelBiasSupported = session.Device().IsFeatureSupported(IGraphicsDevice::Feature::MipmapLevelBias);
	auto skipMipmapLevelBiasTest = [&](const std::string& testName) {
		session.AddTestSkipped(testName, "This part of the test was skipped, as the current renderer doesn't support mipmap level bias on this device.");
	};

	// Define a scene for rendering a screen-aligned quad.
	TexturedQuadSceneHelper::Scene scene;
	REQUIRE(TexturedQuad().InitializeScene(this, session, Geometry(), FragmentShader, scene));

	// Create a texture array and populate one layer with deliberately high-contrast banded mipmap data.
	const uint32_t imageWidth = 256;
	auto texture = renderer.CreateTextureBuffer1DArray();
	texture->SetTextureDimensions(V1UInt32(imageWidth), 2, TestMipmapLevelCount);
	texture->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
	texture->SetUsageFlags(ITextureBuffer::UsageFlags::ShaderInput);
	texture->SetPerformanceHints(ITextureBuffer::PerformanceHint::WriteRarely | ITextureBuffer::PerformanceHint::ReadNever, ITextureBuffer::PerformanceHint::WriteNever | ITextureBuffer::PerformanceHint::ReadOften);
	REQUIRE(texture->AllocateMemory());
	std::vector<std::vector<V4UInt8>> mipLevelData;
	mipLevelData.resize(TestMipmapLevelCount);
	for (int mipmapLevel = 0; mipmapLevel < TestMipmapLevelCount; ++mipmapLevel)
	{
		mipLevelData[(size_t)mipmapLevel] = Mipmapping().CreateMipLevelBandData1D(Mipmapping().MipDimension(imageWidth, mipmapLevel), mipmapLevel);
		REQUIRE(texture->QueueDataUpdate(mipLevelData[(size_t)mipmapLevel], 1, mipmapLevel));
	}

	// Create a sampler to read the texture array from the shader.
	auto sampler = renderer.CreateTextureSampler1DArray();
	sampler->SetTextureWrapMode(ITextureSampler::WrapMode::ClampToEdge);
	sampler->SetTextureFilterMode(ITextureSampler::FilterMode::Nearest, ITextureSampler::FilterMode::Nearest);
	scene.renderableNode->BindTextureWithCombinedSampler(scene.shaderProgram->GetTextureId("colorTexture"), texture.get(), sampler.get());
	scene.renderableNode->SetStateValue(scene.shaderProgram->GetStateValueId("sampleLayer"), V1Float32(1.0f));

	// Capture images showing the effect of each mipmap mode on a flat screen-aligned quad.
	REQUIRE(TexturedQuad().UpdatePositionScale(scene, V2Float32(Mipmapping().ComputeQuadScaleForMipLevel(session.TestWindowSize().X(), imageWidth, 2.0f), 0.5f)));
	sampler->SetTextureMipmapMode(ITextureSampler::MipmapMode::None);
	sampler->SetMipmapLevelMapping(0.0f, TestMaxMipmapLevel, 0.0f);
	auto frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("MipmapNone", "A 1D texture array sampled with mipmapping disabled.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	REQUIRE(TexturedQuad().UpdatePositionScale(scene, V2Float32(Mipmapping().ComputeQuadScaleForMipLevel(session.TestWindowSize().X(), imageWidth, 1.25f), 0.5f)));
	sampler->SetTextureMipmapMode(ITextureSampler::MipmapMode::Nearest);
	sampler->SetMipmapLevelMapping(0.0f, TestMaxMipmapLevel, 0.0f);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("MipmapNearest", "A 1D texture array sampled with nearest mipmap selection.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	sampler->SetTextureMipmapMode(ITextureSampler::MipmapMode::Linear);
	sampler->SetMipmapLevelMapping(0.0f, TestMaxMipmapLevel, 0.0f);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("MipmapLinear", "A 1D texture array sampled with linear mipmap blending.", std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.95);

	REQUIRE(TexturedQuad().UpdatePositionScale(scene, V2Float32(Mipmapping().ComputeQuadScaleForMipLevel(session.TestWindowSize().X(), imageWidth, 0.0f), 0.5f)));
	sampler->SetTextureMipmapMode(ITextureSampler::MipmapMode::Nearest);
	sampler->SetMipmapLevelMapping(2.0f, TestMaxMipmapLevel, 0.0f);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("MipmapMinLevel", "A 1D texture array sampled with the minimum mipmap level clamped to level 2.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	REQUIRE(TexturedQuad().UpdatePositionScale(scene, V2Float32(Mipmapping().ComputeQuadScaleForMipLevel(session.TestWindowSize().X(), imageWidth, 3.0f), 0.5f)));
	sampler->SetMipmapLevelMapping(0.0f, 1.0f, 0.0f);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("MipmapMaxLevel", "A 1D texture array sampled with the maximum mipmap level clamped to level 1.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	if (mipmapLevelBiasSupported)
	{
		REQUIRE(TexturedQuad().UpdatePositionScale(scene, V2Float32(Mipmapping().ComputeQuadScaleForMipLevel(session.TestWindowSize().X(), imageWidth, 1.0f), 0.5f)));
		sampler->SetMipmapLevelMapping(0.0f, TestMaxMipmapLevel, 1.0f);
		frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("MipmapBiasPositive", "A 1D texture array sampled with a positive mipmap level bias.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

		REQUIRE(TexturedQuad().UpdatePositionScale(scene, V2Float32(Mipmapping().ComputeQuadScaleForMipLevel(session.TestWindowSize().X(), imageWidth, 2.0f), 0.5f)));
		sampler->SetMipmapLevelMapping(0.0f, TestMaxMipmapLevel, -1.0f);
		frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("MipmapBiasNegative", "A 1D texture array sampled with a negative mipmap level bias.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);
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

	// Define a perspective scene so the mip chain is visible along a plane receding into the distance.
	TexturedQuadSceneHelper::Scene perspectiveScene;
	REQUIRE(TexturedQuad().InitializeScene(this, session, Geometry(), TexturedQuad().PerspectiveVertexShader(), PerspectiveFragmentShader, perspectiveScene));
	perspectiveScene.renderableNode->BindTextureWithCombinedSampler(perspectiveScene.shaderProgram->GetTextureId("colorTexture"), texture.get(), sampler.get());
	perspectiveScene.renderableNode->SetStateValue(perspectiveScene.shaderProgram->GetStateValueId("sampleLayer"), V1Float32(1.0f));

	// Configure the receding plane so the mipmap transitions stay on screen over a wide depth range.
	MipmappingHelper::PerspectivePlaneSettings perspectiveSettings;
	perspectiveSettings.texCoordScale = V2Float32(1.0f, 48.0f);
	REQUIRE(Mipmapping().ConfigurePerspectivePlaneScene(perspectiveScene, Transform(), session, perspectiveSettings, TexturedQuad()));

	// Capture images showing the same mipmap modes on the perspective plane.
	sampler->SetTextureWrapMode(ITextureSampler::WrapMode::Repeat);
	sampler->SetTextureFilterMode(ITextureSampler::FilterMode::Nearest, ITextureSampler::FilterMode::Nearest);
	sampler->SetTextureMipmapMode(ITextureSampler::MipmapMode::None);
	sampler->SetMipmapLevelMapping(0.0f, TestMaxMipmapLevel, 0.0f);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	perspectiveScene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("PerspectiveMipmapNone", "A 1D texture array sampled on a perspective floor plane with mipmapping disabled.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	sampler->SetTextureMipmapMode(ITextureSampler::MipmapMode::Nearest);
	sampler->SetMipmapLevelMapping(0.0f, TestMaxMipmapLevel, 0.0f);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	perspectiveScene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("PerspectiveMipmapNearest", "A 1D texture array sampled on a perspective floor plane with nearest mipmap selection.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	sampler->SetTextureMipmapMode(ITextureSampler::MipmapMode::Linear);
	sampler->SetMipmapLevelMapping(0.0f, TestMaxMipmapLevel, 0.0f);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	perspectiveScene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("PerspectiveMipmapLinear", "A 1D texture array sampled on a perspective floor plane with linear mipmap blending.", std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.95);

	sampler->SetTextureMipmapMode(ITextureSampler::MipmapMode::Nearest);
	sampler->SetMipmapLevelMapping(2.0f, TestMaxMipmapLevel, 0.0f);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	perspectiveScene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("PerspectiveMipmapMinLevel", "A 1D texture array sampled on a perspective floor plane with the minimum mipmap level clamped to level 2.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	sampler->SetMipmapLevelMapping(0.0f, 1.0f, 0.0f);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	perspectiveScene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("PerspectiveMipmapMaxLevel", "A 1D texture array sampled on a perspective floor plane with the maximum mipmap level clamped to level 1.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	if (mipmapLevelBiasSupported)
	{
		sampler->SetMipmapLevelMapping(0.0f, TestMaxMipmapLevel, 1.0f);
		frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		perspectiveScene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("PerspectiveMipmapBiasPositive", "A 1D texture array sampled on a perspective floor plane with a positive mipmap level bias.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.90);

		sampler->SetMipmapLevelMapping(0.0f, TestMaxMipmapLevel, -1.0f);
		frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		perspectiveScene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("PerspectiveMipmapBiasNegative", "A 1D texture array sampled on a perspective floor plane with a negative mipmap level bias.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.90);
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
