// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../UnitTestBase.h"
namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

namespace {
// Define our shader programs
const std::string FragmentShader = R"(
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

std::vector<V4UInt8> CreateStripData(uint32_t imageWidth)
{
	std::vector<V4UInt8> imageData;
	imageData.resize(imageWidth);
	for (uint32_t posX = 0; posX < imageWidth; ++posX)
	{
		if (posX < (imageWidth / 3))
		{
			imageData[posX] = V4UInt8(255, 32, 32, 255);
		}
		else if (posX < ((imageWidth * 2) / 3))
		{
			imageData[posX] = V4UInt8(32, 255, 32, 255);
		}
		else
		{
			imageData[posX] = V4UInt8(32, 32, 255, 255);
		}
	}
	return imageData;
}
} // namespace

DEFINE_UNIT_TEST_WITH_BASE("Resources/Images/TextureBuffer1D", UnitTestBase)
{
	// Retrieve our important objects
	auto& renderer = session.Renderer();

	// Define the framebuffer and render scene
	TexturedQuadSceneHelper::Scene scene;
	REQUIRE(TexturedQuad().InitializeScene(this, session, Geometry(), FragmentShader, scene));

	// Create our texture buffer and load our image data into it
	const uint32_t imageWidth = 256;
	auto textureData = CreateStripData(imageWidth);
	auto texture = renderer.CreateTextureBuffer1D();
	texture->SetTextureDimensions(V1UInt32(imageWidth));
	texture->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
	texture->SetUsageFlags(ITextureBuffer::UsageFlags::ShaderInput);
	texture->SetPerformanceHints(ITextureBuffer::PerformanceHint::WriteOften | ITextureBuffer::PerformanceHint::ReadNever, ITextureBuffer::PerformanceHint::WriteNever | ITextureBuffer::PerformanceHint::ReadOften);
	REQUIRE(texture->SetInitialData(textureData));
	REQUIRE(texture->AllocateMemory());

	// Create a texture sampler to read the texture data from the shader
	auto sampler = renderer.CreateTextureSampler1D();
	sampler->SetTextureFilterMode(ITextureSampler::FilterMode::Nearest, ITextureSampler::FilterMode::Nearest);
	sampler->SetTextureWrapMode(ITextureSampler::WrapMode::ClampToEdge);

	// Bind our texture to the renderable
	scene.renderableNode->BindTextureWithCombinedSampler(scene.shaderProgram->GetTextureId("colorTexture"), texture.get(), sampler.get());

	// Capture an image of the scene
	auto frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("PrimaryColorBands", "Three coloured vertical bands sourced from a 1D texture.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Replace the middle segment with white data
	std::vector<V4UInt8> whiteStrip;
	whiteStrip.resize(imageWidth / 3, V4UInt8(255, 255, 255, 255));
	REQUIRE(texture->QueueDataUpdate(whiteStrip, 0, V1UInt32(imageWidth / 3), V1UInt32((uint32_t)whiteStrip.size())));

	// Capture an image of the scene
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("WhiteMiddleBand", "Three coloured vertical bands sourced from a 1D texture, with the middle band replaced with white.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Capture images with each texture wrap mode
	REQUIRE(TexturedQuad().UpdateTexCoords(scene, V2Float32(2.5f, 1.0f), V2Float32(-0.75f, 0.0f)));
	sampler->SetTextureWrapMode(ITextureSampler::WrapMode::ClampToEdge);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("WrapClampToEdge", "A 1D texture sampled with clamp-to-edge wrap mode.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	sampler->SetTextureWrapMode(ITextureSampler::WrapMode::Repeat);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("WrapRepeat", "A 1D texture sampled with repeat wrap mode.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	sampler->SetTextureWrapMode(ITextureSampler::WrapMode::RepeatMirrored);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("WrapRepeatMirrored", "A 1D texture sampled with mirrored-repeat wrap mode.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Capture images with each texture filter mode
	auto randomFilterPatch = TexturedQuad().CreateRandomPatchData(TexturedQuad().PatchSize());
	REQUIRE(texture->QueueDataUpdate(randomFilterPatch, 0, V1UInt32(TexturedQuad().GetPatchStart(imageWidth)), V1UInt32(TexturedQuad().PatchSize())));
	REQUIRE(TexturedQuad().UpdateTexCoords(scene, TexturedQuad().GetTexCoordScale(imageWidth), TexturedQuad().GetTexCoordOffset(imageWidth)));

	sampler->SetTextureWrapMode(ITextureSampler::WrapMode::ClampToEdge);
	sampler->SetTextureFilterMode(ITextureSampler::FilterMode::Nearest, ITextureSampler::FilterMode::Nearest);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("FilterNearest", "A magnified 1D texture sampled with nearest filtering.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	sampler->SetTextureFilterMode(ITextureSampler::FilterMode::Linear, ITextureSampler::FilterMode::Linear);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("FilterLinear", "A magnified 1D texture sampled with linear filtering.", std::move(frameBufferCapture), IImageDiff::Algorithm::RegionRanges | IImageDiff::Algorithm::NaiveDiff, 0.95);

	// Remove all our defined render passes
	renderer.RemoveAllRenderPasses();
	return true;
}

} // namespace cobalt::graphics::testing
