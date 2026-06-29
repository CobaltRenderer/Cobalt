// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../UnitTestBase.h"
namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

namespace {
// Define our shader programs
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

std::vector<V4UInt8> CreateLayerData(uint32_t imageWidth, const V4UInt8& colorA, const V4UInt8& colorB, const V4UInt8& colorC)
{
	std::vector<V4UInt8> imageData;
	imageData.resize(imageWidth);
	for (uint32_t posX = 0; posX < imageWidth; ++posX)
	{
		if (posX < (imageWidth / 3))
		{
			imageData[posX] = colorA;
		}
		else if (posX < ((imageWidth * 2) / 3))
		{
			imageData[posX] = colorB;
		}
		else
		{
			imageData[posX] = colorC;
		}
	}
	return imageData;
}
} // namespace

DEFINE_UNIT_TEST_WITH_BASE("Resources/Images/TextureBuffer1DArray", UnitTestBase)
{
	// Retrieve our important objects
	auto& renderer = session.Renderer();

	// Define the framebuffer and render scene
	TexturedQuadSceneHelper::Scene scene;
	REQUIRE(TexturedQuad().InitializeScene(this, session, Geometry(), FragmentShader, scene));

	// Create our texture buffer and load our image data into it
	const uint32_t imageWidth = 256;
	std::vector<std::vector<V4UInt8>> textureData;
	textureData.push_back(CreateLayerData(imageWidth, V4UInt8(255, 32, 32, 255), V4UInt8(255, 192, 32, 255), V4UInt8(255, 255, 32, 255)));
	textureData.push_back(CreateLayerData(imageWidth, V4UInt8(32, 128, 255, 255), V4UInt8(64, 255, 255, 255), V4UInt8(255, 64, 255, 255)));
	auto texture = renderer.CreateTextureBuffer1DArray();
	texture->SetTextureDimensions(V1UInt32(imageWidth), 2);
	texture->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
	texture->SetUsageFlags(ITextureBuffer::UsageFlags::ShaderInput);
	texture->SetPerformanceHints(ITextureBuffer::PerformanceHint::WriteOften | ITextureBuffer::PerformanceHint::ReadNever, ITextureBuffer::PerformanceHint::WriteNever | ITextureBuffer::PerformanceHint::ReadOften);
	REQUIRE(texture->SetInitialData(textureData[0], 0));
	REQUIRE(texture->SetInitialData(textureData[1], 1));
	REQUIRE(texture->AllocateMemory());

	// Create a texture sampler to read the texture data from the shader
	auto sampler = renderer.CreateTextureSampler1DArray();
	sampler->SetTextureFilterMode(ITextureSampler::FilterMode::Nearest, ITextureSampler::FilterMode::Nearest);
	sampler->SetTextureWrapMode(ITextureSampler::WrapMode::ClampToEdge);

	// Bind our texture to the renderable
	scene.renderableNode->BindTextureWithCombinedSampler(scene.shaderProgram->GetTextureId("colorTexture"), texture.get(), sampler.get());

	// Capture an image of layer 0
	scene.renderableNode->SetStateValue(scene.shaderProgram->GetStateValueId("sampleLayer"), V1Float32(0.0f));
	auto frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("WarmColorBands", "Three warm-coloured vertical bands sourced from layer 0 of a 1D texture array.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Capture an image of layer 1
	scene.renderableNode->SetStateValue(scene.shaderProgram->GetStateValueId("sampleLayer"), V1Float32(1.0f));
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("CoolColorBands", "Three cool-coloured vertical bands sourced from layer 1 of a 1D texture array.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Replace the middle segment of layer 1 with white data
	std::vector<V4UInt8> whiteStrip;
	whiteStrip.resize(imageWidth / 3, V4UInt8(255, 255, 255, 255));
	REQUIRE(texture->QueueDataUpdate(whiteStrip, 1, 0, V1UInt32(imageWidth / 3), V1UInt32((uint32_t)whiteStrip.size())));

	// Capture an image of layer 1
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("CoolColorBandsWhiteMiddle", "Three cool-coloured vertical bands sourced from layer 1 of a 1D texture array, with the middle band replaced with white.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Capture images with each texture wrap mode
	REQUIRE(TexturedQuad().UpdateTexCoords(scene, V2Float32(2.5f, 1.0f), V2Float32(-0.75f, 0.0f)));
	sampler->SetTextureWrapMode(ITextureSampler::WrapMode::ClampToEdge);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("WrapClampToEdge", "A 1D texture array sampled with clamp-to-edge wrap mode.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	sampler->SetTextureWrapMode(ITextureSampler::WrapMode::Repeat);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("WrapRepeat", "A 1D texture array sampled with repeat wrap mode.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	sampler->SetTextureWrapMode(ITextureSampler::WrapMode::RepeatMirrored);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("WrapRepeatMirrored", "A 1D texture array sampled with mirrored-repeat wrap mode.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Capture images with each texture filter mode
	auto randomFilterPatch = TexturedQuad().CreateRandomPatchData(TexturedQuad().PatchSize());
	REQUIRE(texture->QueueDataUpdate(randomFilterPatch, 1, 0, V1UInt32(TexturedQuad().GetPatchStart(imageWidth)), V1UInt32(TexturedQuad().PatchSize())));
	REQUIRE(TexturedQuad().UpdateTexCoords(scene, TexturedQuad().GetTexCoordScale(imageWidth), TexturedQuad().GetTexCoordOffset(imageWidth)));

	sampler->SetTextureWrapMode(ITextureSampler::WrapMode::ClampToEdge);
	sampler->SetTextureFilterMode(ITextureSampler::FilterMode::Nearest, ITextureSampler::FilterMode::Nearest);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("FilterNearest", "A magnified 1D texture array sampled with nearest filtering.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	sampler->SetTextureFilterMode(ITextureSampler::FilterMode::Linear, ITextureSampler::FilterMode::Linear);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("FilterLinear", "A magnified 1D texture array sampled with linear filtering.", std::move(frameBufferCapture), IImageDiff::Algorithm::RegionRanges | IImageDiff::Algorithm::NaiveDiff, 0.95);

	// Remove all our defined render passes
	renderer.RemoveAllRenderPasses();
	return true;
}

} // namespace cobalt::graphics::testing
