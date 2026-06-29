// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../UnitTestBase.h"
namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

namespace {
constexpr int TestMipmapLevelCount = 4;
constexpr float TestMaxMipmapLevel = (float)(TestMipmapLevelCount - 1);

const std::string FragmentShader = R"(
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
} // namespace

DEFINE_UNIT_TEST_WITH_BASE("Resources/Mipmapping/Texture2DArray", UnitTestBase)
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

	// Create a texture array and populate one layer with deliberately high-contrast mipmap data.
	const V2UInt32 imageDimensions(256, 256);
	auto texture = renderer.CreateTextureBuffer2DArray();
	texture->SetTextureDimensions(imageDimensions, 2, TestMipmapLevelCount);
	texture->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
	texture->SetUsageFlags(ITextureBuffer::UsageFlags::ShaderInput);
	texture->SetPerformanceHints(ITextureBuffer::PerformanceHint::WriteRarely | ITextureBuffer::PerformanceHint::ReadNever, ITextureBuffer::PerformanceHint::WriteNever | ITextureBuffer::PerformanceHint::ReadOften);
	REQUIRE(texture->AllocateMemory());
	std::vector<std::vector<V4UInt8>> mipLevelData;
	mipLevelData.resize(TestMipmapLevelCount);
	for (int mipmapLevel = 0; mipmapLevel < TestMipmapLevelCount; ++mipmapLevel)
	{
		mipLevelData[(size_t)mipmapLevel] = Mipmapping().CreateMipLevelBandData2D(Mipmapping().MipDimensions(imageDimensions, mipmapLevel), mipmapLevel);
		REQUIRE(texture->QueueDataUpdate(mipLevelData[(size_t)mipmapLevel], 1, mipmapLevel));
	}

	// Create a sampler to read the texture array from the shader.
	auto sampler = renderer.CreateTextureSampler2DArray();
	sampler->SetTextureWrapMode(ITextureSampler::WrapMode::ClampToEdge, ITextureSampler::WrapMode::ClampToEdge);
	sampler->SetTextureFilterMode(ITextureSampler::FilterMode::Nearest, ITextureSampler::FilterMode::Nearest);
	scene.renderableNode->BindTextureWithCombinedSampler(scene.shaderProgram->GetTextureId("colorTexture"), texture.get(), sampler.get());
	scene.renderableNode->SetStateValue(scene.shaderProgram->GetStateValueId("sampleLayer"), V1Float32(1.0f));

	// Capture images showing the effect of each mipmap mode on a flat screen-aligned quad.
	REQUIRE(TexturedQuad().UpdatePositionScale(scene, Mipmapping().ComputeQuadScaleForMipLevel(session.TestWindowSize(), imageDimensions, 2.0f)));
	sampler->SetTextureMipmapMode(ITextureSampler::MipmapMode::None);
	sampler->SetMipmapLevelMapping(0.0f, TestMaxMipmapLevel, 0.0f);
	auto frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("MipmapNone", "A 2D texture array sampled with mipmapping disabled.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	REQUIRE(TexturedQuad().UpdatePositionScale(scene, Mipmapping().ComputeQuadScaleForMipLevel(session.TestWindowSize(), imageDimensions, 1.25f)));
	sampler->SetTextureMipmapMode(ITextureSampler::MipmapMode::Nearest);
	sampler->SetMipmapLevelMapping(0.0f, TestMaxMipmapLevel, 0.0f);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("MipmapNearest", "A 2D texture array sampled with nearest mipmap selection.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	sampler->SetTextureMipmapMode(ITextureSampler::MipmapMode::Linear);
	sampler->SetMipmapLevelMapping(0.0f, TestMaxMipmapLevel, 0.0f);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("MipmapLinear", "A 2D texture array sampled with linear mipmap blending.", std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.95);

	REQUIRE(TexturedQuad().UpdatePositionScale(scene, Mipmapping().ComputeQuadScaleForMipLevel(session.TestWindowSize(), imageDimensions, 0.0f)));
	sampler->SetTextureMipmapMode(ITextureSampler::MipmapMode::Nearest);
	sampler->SetMipmapLevelMapping(2.0f, TestMaxMipmapLevel, 0.0f);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("MipmapMinLevel", "A 2D texture array sampled with the minimum mipmap level clamped to level 2.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	REQUIRE(TexturedQuad().UpdatePositionScale(scene, Mipmapping().ComputeQuadScaleForMipLevel(session.TestWindowSize(), imageDimensions, 3.0f)));
	sampler->SetMipmapLevelMapping(0.0f, 1.0f, 0.0f);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("MipmapMaxLevel", "A 2D texture array sampled with the maximum mipmap level clamped to level 1.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	if (mipmapLevelBiasSupported)
	{
		REQUIRE(TexturedQuad().UpdatePositionScale(scene, Mipmapping().ComputeQuadScaleForMipLevel(session.TestWindowSize(), imageDimensions, 1.0f)));
		sampler->SetMipmapLevelMapping(0.0f, TestMaxMipmapLevel, 1.0f);
		frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("MipmapBiasPositive", "A 2D texture array sampled with a positive mipmap level bias.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

		REQUIRE(TexturedQuad().UpdatePositionScale(scene, Mipmapping().ComputeQuadScaleForMipLevel(session.TestWindowSize(), imageDimensions, 2.0f)));
		sampler->SetMipmapLevelMapping(0.0f, TestMaxMipmapLevel, -1.0f);
		frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("MipmapBiasNegative", "A 2D texture array sampled with a negative mipmap level bias.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);
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
	REQUIRE(TexturedQuad().InitializeScene(this, session, Geometry(), TexturedQuad().PerspectiveVertexShader(), FragmentShader, perspectiveScene));
	perspectiveScene.renderableNode->BindTextureWithCombinedSampler(perspectiveScene.shaderProgram->GetTextureId("colorTexture"), texture.get(), sampler.get());
	perspectiveScene.renderableNode->SetStateValue(perspectiveScene.shaderProgram->GetStateValueId("sampleLayer"), V1Float32(1.0f));

	// Configure the receding plane so the mipmap transitions stay on screen over a wide depth range.
	MipmappingHelper::PerspectivePlaneSettings perspectiveSettings;
	perspectiveSettings.texCoordScale = V2Float32(8.0f, 32.0f);
	REQUIRE(Mipmapping().ConfigurePerspectivePlaneScene(perspectiveScene, Transform(), session, perspectiveSettings, TexturedQuad()));

	// Capture images showing the same mipmap modes on the perspective plane.
	sampler->SetTextureWrapMode(ITextureSampler::WrapMode::Repeat, ITextureSampler::WrapMode::Repeat);
	sampler->SetTextureFilterMode(ITextureSampler::FilterMode::Nearest, ITextureSampler::FilterMode::Nearest);
	sampler->SetTextureMipmapMode(ITextureSampler::MipmapMode::None);
	sampler->SetMipmapLevelMapping(0.0f, TestMaxMipmapLevel, 0.0f);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	perspectiveScene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("PerspectiveMipmapNone", "A 2D texture array sampled on a perspective floor plane with mipmapping disabled.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	sampler->SetTextureMipmapMode(ITextureSampler::MipmapMode::Nearest);
	sampler->SetMipmapLevelMapping(0.0f, TestMaxMipmapLevel, 0.0f);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	perspectiveScene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("PerspectiveMipmapNearest", "A 2D texture array sampled on a perspective floor plane with nearest mipmap selection.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	sampler->SetTextureMipmapMode(ITextureSampler::MipmapMode::Linear);
	sampler->SetMipmapLevelMapping(0.0f, TestMaxMipmapLevel, 0.0f);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	perspectiveScene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("PerspectiveMipmapLinear", "A 2D texture array sampled on a perspective floor plane with linear mipmap blending.", std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.90);

	sampler->SetTextureMipmapMode(ITextureSampler::MipmapMode::Nearest);
	sampler->SetMipmapLevelMapping(2.0f, TestMaxMipmapLevel, 0.0f);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	perspectiveScene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("PerspectiveMipmapMinLevel", "A 2D texture array sampled on a perspective floor plane with the minimum mipmap level clamped to level 2.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	sampler->SetMipmapLevelMapping(0.0f, 1.0f, 0.0f);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	perspectiveScene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("PerspectiveMipmapMaxLevel", "A 2D texture array sampled on a perspective floor plane with the maximum mipmap level clamped to level 1.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	if (mipmapLevelBiasSupported)
	{
		sampler->SetMipmapLevelMapping(0.0f, TestMaxMipmapLevel, 1.0f);
		frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		perspectiveScene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("PerspectiveMipmapBiasPositive", "A 2D texture array sampled on a perspective floor plane with a positive mipmap level bias.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.90);

		sampler->SetMipmapLevelMapping(0.0f, TestMaxMipmapLevel, -1.0f);
		frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		perspectiveScene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("PerspectiveMipmapBiasNegative", "A 2D texture array sampled on a perspective floor plane with a negative mipmap level bias.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.90);
	}
	else
	{
		skipMipmapLevelBiasTest("PerspectiveMipmapBiasPositive");
		skipMipmapLevelBiasTest("PerspectiveMipmapBiasNegative");
	}

	// Load a tiled floor texture with a generated mip chain so each mip level stays pattern-matched.
	auto floorImage = Texture().LoadImageFromPngFile("FloorTiles.png", true);
	REQUIRE(floorImage != nullptr);
	REQUIRE(floorImage->mipmapLevelCount == 10);

	// Create a texture array from the loaded floor image and upload every generated mipmap level to one layer.
	auto floorTexture = renderer.CreateTextureBuffer2DArray();
	floorTexture->SetTextureDimensions(floorImage->size, 2, floorImage->mipmapLevelCount);
	floorTexture->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
	floorTexture->SetUsageFlags(ITextureBuffer::UsageFlags::ShaderInput);
	floorTexture->SetPerformanceHints(ITextureBuffer::PerformanceHint::WriteRarely | ITextureBuffer::PerformanceHint::ReadNever, ITextureBuffer::PerformanceHint::WriteNever | ITextureBuffer::PerformanceHint::ReadOften);
	REQUIRE(floorTexture->AllocateMemory());
	for (uint32_t mipmapLevel = 0; mipmapLevel < floorImage->mipmapLevelCount; ++mipmapLevel)
	{
		REQUIRE(floorTexture->QueueDataUpdate(floorImage->mipmapTextureData[(size_t)mipmapLevel], 1, (int)mipmapLevel));
	}

	// Clear the previous perspective scene before binding a denser floor scene to the same test window.
	renderer.RemoveAllRenderPasses();
	perspectiveScene = TexturedQuadSceneHelper::Scene();
	renderer.WaitForDeferredDeletionComplete();

	// Configure the plane to behave like a floor stretching toward the horizon
	MipmappingHelper::PerspectivePlaneSettings floorSettings = perspectiveSettings;
	const float floorWidth = 10.0f;
	const float floorNearDistance = 0.5f;
	const float floorFarDistance = 8192.0f;
	const float floorTextureRepeatsAcrossWidth = 10.0f;
	floorSettings.width = floorWidth;
	floorSettings.nearY = floorNearDistance;
	floorSettings.farY = floorFarDistance;
	floorSettings.cameraPosition = V3Float32(0.0f, -3.0f, 1.7f);
	floorSettings.fov = 60.0f;
	floorSettings.farClip = floorFarDistance * 2.0f;
	const float floorDepth = floorSettings.farY - floorSettings.nearY;
	const float floorTextureRepeatsPerWorldUnit = floorTextureRepeatsAcrossWidth / floorSettings.width;
	floorSettings.texCoordScale = V2Float32(floorTextureRepeatsAcrossWidth, floorTextureRepeatsPerWorldUnit * floorDepth);

	// Use a segmented floor mesh so repeated V coordinates are locally re-based for each segment.
	std::vector<V3Float32> floorPositionData;
	std::vector<V2Float32> floorTexCoordData;
	std::vector<V1UInt32> floorIndexData;
	Mipmapping().CreateSegmentedPerspectivePlaneMesh(floorSettings, 9, 256, floorPositionData, floorTexCoordData, floorIndexData);
	TexturedQuadSceneHelper::Scene floorScene;
	REQUIRE(TexturedQuad().InitializeScene(this, session, TexturedQuad().PerspectiveVertexShader(), FragmentShader, floorPositionData, floorTexCoordData, floorIndexData, floorScene));
	auto floorViewProj = Transform().LookAtCenterPerspective(session.TestWindowSizeAsFloat(), floorSettings.cameraPosition, floorSettings.fov, floorSettings.nearClip, floorSettings.farClip);
	floorScene.renderableNode->SetStateValue(floorScene.shaderProgram->GetStateValueId("viewProj"), floorViewProj);
	floorScene.renderableNode->BindTextureWithCombinedSampler(floorScene.shaderProgram->GetTextureId("colorTexture"), floorTexture.get(), sampler.get());
	floorScene.renderableNode->SetStateValue(floorScene.shaderProgram->GetStateValueId("sampleLayer"), V1Float32(1.0f));

	// Capture comparison images for nearest and linear filtering on the tiled floor
	sampler->SetTextureWrapMode(ITextureSampler::WrapMode::Repeat, ITextureSampler::WrapMode::Repeat);
	sampler->SetAnisotropicFilterMode(false);
	sampler->SetTextureFilterMode(ITextureSampler::FilterMode::Nearest, ITextureSampler::FilterMode::Nearest);
	sampler->SetTextureMipmapMode(ITextureSampler::MipmapMode::Nearest);
	sampler->SetMipmapLevelMapping(0.0f, (float)(floorImage->mipmapLevelCount - 1), 0.0f);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	floorScene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("PerspectiveFloorNearest", "A tiled floor texture array sampled with nearest filtering on a perspective plane.", std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.95);

	sampler->SetTextureFilterMode(ITextureSampler::FilterMode::Linear, ITextureSampler::FilterMode::Linear);
	sampler->SetTextureMipmapMode(ITextureSampler::MipmapMode::Linear);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	floorScene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("PerspectiveFloorLinear", "A tiled floor texture array sampled with linear filtering on a perspective plane.", std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.95);

	// Perform anisotropic filtering tests if this device supports it
	if (session.Device().IsFeatureSupported(IGraphicsDevice::Feature::AnisotropicFiltering))
	{
		// Capture images at each supported anisotropic filtering level
		auto maxAnisotropyLevel = std::max(2, session.Device().GetImageLimits().maxSamplerAnisotropicFilteringLevel);
		auto autoMaxTestName = "PerspectiveFloorAnisotropic" + std::to_string(maxAnisotropyLevel) + "x";
		bool maxLevelCapturedExplicitly = false;
		for (int anisotropyLevel = 2; anisotropyLevel <= maxAnisotropyLevel; anisotropyLevel *= 2)
		{
			sampler->SetAnisotropicFilterMode(true, anisotropyLevel);
			frameBufferCapture = renderer.CreateFrameBufferOutput();
			frameBufferCapture->SetDetachAfterCapture(true);
			floorScene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
			DrawOneFrame();
			session.AddTestImageResult("PerspectiveFloorAnisotropic" + std::to_string(anisotropyLevel) + "x", "A tiled floor texture array sampled with anisotropic filtering level " + std::to_string(anisotropyLevel) + " on a perspective plane.", std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.95);
			maxLevelCapturedExplicitly = (anisotropyLevel == maxAnisotropyLevel);
		}
		if (!maxLevelCapturedExplicitly)
		{
			sampler->SetAnisotropicFilterMode(true, maxAnisotropyLevel);
			frameBufferCapture = renderer.CreateFrameBufferOutput();
			frameBufferCapture->SetDetachAfterCapture(true);
			floorScene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
			DrawOneFrame();
			session.AddTestImageResult(autoMaxTestName, "A tiled floor texture array sampled with anisotropic filtering level " + std::to_string(maxAnisotropyLevel) + " on a perspective plane.", std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.95);
		}

		// Capture a final image at the automatic max anisotropic filtering level
		sampler->SetAnisotropicFilterMode(true);
		frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		floorScene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult(autoMaxTestName, "A tiled floor texture array sampled with auto-max anisotropic filtering, which should match anisotropic filtering level " + std::to_string(maxAnisotropyLevel) + " on a perspective plane.", std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.95);
	}
	else
	{
		session.AddTestSkipped("PerspectiveFloorAnisotropic2x", "Perspective anisotropic mipmapping tests were skipped, as the current renderer doesn't support anisotropic filtering.");
	}

	renderer.RemoveAllRenderPasses();
	return true;
}

} // namespace cobalt::graphics::testing
