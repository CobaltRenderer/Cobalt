// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../UnitTestBase.h"
namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

namespace {
// Define our shader programs
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

std::vector<V4UInt8> CreateLayerData(uint32_t imageWidth, uint32_t imageHeight, const V4UInt8& topColor, const V4UInt8& bottomColor)
{
	std::vector<V4UInt8> imageData;
	imageData.resize((size_t)imageWidth * (size_t)imageHeight);
	for (uint32_t posY = 0; posY < imageHeight; ++posY)
	{
		for (uint32_t posX = 0; posX < imageWidth; ++posX)
		{
			imageData[posX + (posY * imageWidth)] = (posY < (imageHeight / 2)) ? topColor : bottomColor;
		}
	}
	return imageData;
}
} // namespace

DEFINE_UNIT_TEST_WITH_BASE("Resources/Images/TextureBuffer2DArray", UnitTestBase)
{
	// Retrieve our important objects
	auto& renderer = session.Renderer();

	// Define the framebuffer and render scene
	TexturedQuadSceneHelper::Scene scene;
	REQUIRE(TexturedQuad().InitializeScene(this, session, Geometry(), FragmentShader, scene));

	// Create our texture buffer and load our image data into it
	const V2UInt32 imageDimensions(128, 128);
	std::vector<std::vector<V4UInt8>> textureData;
	textureData.push_back(CreateLayerData(imageDimensions.X(), imageDimensions.Y(), V4UInt8(255, 96, 96, 255), V4UInt8(255, 224, 96, 255)));
	textureData.push_back(CreateLayerData(imageDimensions.X(), imageDimensions.Y(), V4UInt8(96, 160, 255, 255), V4UInt8(224, 96, 255, 255)));
	auto texture = renderer.CreateTextureBuffer2DArray();
	texture->SetTextureDimensions(imageDimensions, 2);
	texture->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
	texture->SetUsageFlags(ITextureBuffer::UsageFlags::ShaderInput);
	texture->SetPerformanceHints(ITextureBuffer::PerformanceHint::WriteOften | ITextureBuffer::PerformanceHint::ReadNever, ITextureBuffer::PerformanceHint::WriteNever | ITextureBuffer::PerformanceHint::ReadOften);
	REQUIRE(texture->SetInitialData(textureData[0], 0));
	REQUIRE(texture->SetInitialData(textureData[1], 1));
	REQUIRE(texture->AllocateMemory());

	// Create a texture sampler to read the texture data from the shader
	auto sampler = renderer.CreateTextureSampler2DArray();
	sampler->SetTextureFilterMode(ITextureSampler::FilterMode::Nearest, ITextureSampler::FilterMode::Nearest);
	sampler->SetTextureWrapMode(ITextureSampler::WrapMode::ClampToEdge, ITextureSampler::WrapMode::ClampToEdge);

	// Bind our texture to the renderable
	scene.renderableNode->BindTextureWithCombinedSampler(scene.shaderProgram->GetTextureId("colorTexture"), texture.get(), sampler.get());

	// Capture an image of layer 0
	scene.renderableNode->SetStateValue(scene.shaderProgram->GetStateValueId("sampleLayer"), V1Float32(0.0f));
	auto frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("WarmHorizontalBands", "Two warm-coloured horizontal bands sourced from layer 0 of a 2D texture array.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Capture an image of layer 1
	scene.renderableNode->SetStateValue(scene.shaderProgram->GetStateValueId("sampleLayer"), V1Float32(1.0f));
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("CoolHorizontalBands", "Two cool-coloured horizontal bands sourced from layer 1 of a 2D texture array.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Replace the middle rows of layer 1 with white data
	std::vector<V4UInt8> whiteRows;
	whiteRows.resize((size_t)imageDimensions.X() * ((size_t)imageDimensions.Y() / 3), V4UInt8(255, 255, 255, 255));
	REQUIRE(texture->QueueDataUpdate(whiteRows, 1, 0, V2UInt32(0, imageDimensions.Y() / 3), V2UInt32(imageDimensions.X(), imageDimensions.Y() / 3)));

	// Capture an image of layer 1
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("CoolHorizontalBandsWhiteMiddle", "Two cool-coloured horizontal bands sourced from layer 1 of a 2D texture array, with a white strip across the middle.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Capture images with each texture wrap mode
	REQUIRE(TexturedQuad().UpdateTexCoords(scene, V2Float32(2.5f, 2.5f), V2Float32(-0.75f, -0.75f)));
	sampler->SetTextureWrapMode(ITextureSampler::WrapMode::ClampToEdge, ITextureSampler::WrapMode::ClampToEdge);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("WrapClampToEdge", "A 2D texture array sampled with clamp-to-edge wrap mode.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	sampler->SetTextureWrapMode(ITextureSampler::WrapMode::Repeat, ITextureSampler::WrapMode::Repeat);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("WrapRepeat", "A 2D texture array sampled with repeat wrap mode.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	sampler->SetTextureWrapMode(ITextureSampler::WrapMode::RepeatMirrored, ITextureSampler::WrapMode::RepeatMirrored);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("WrapRepeatMirrored", "A 2D texture array sampled with mirrored-repeat wrap mode.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Capture images with each texture filter mode
	auto randomFilterPatch = TexturedQuad().CreateRandomPatchData(TexturedQuad().PatchSize(), TexturedQuad().PatchSize());
	auto filterPatchOffset = V2UInt32(TexturedQuad().GetPatchStart(imageDimensions.X()), TexturedQuad().GetPatchStart(imageDimensions.Y()));
	REQUIRE(texture->QueueDataUpdate(randomFilterPatch, 1, 0, filterPatchOffset, V2UInt32(TexturedQuad().PatchSize(), TexturedQuad().PatchSize())));
	REQUIRE(TexturedQuad().UpdateTexCoords(scene, TexturedQuad().GetTexCoordScale(imageDimensions), TexturedQuad().GetTexCoordOffset(imageDimensions)));

	sampler->SetTextureWrapMode(ITextureSampler::WrapMode::ClampToEdge, ITextureSampler::WrapMode::ClampToEdge);
	sampler->SetTextureFilterMode(ITextureSampler::FilterMode::Nearest, ITextureSampler::FilterMode::Nearest);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("FilterNearest", "A magnified 2D texture array sampled with nearest filtering.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	sampler->SetTextureFilterMode(ITextureSampler::FilterMode::Linear, ITextureSampler::FilterMode::Linear);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("FilterLinear", "A magnified 2D texture array sampled with linear filtering.", std::move(frameBufferCapture), IImageDiff::Algorithm::RegionRanges | IImageDiff::Algorithm::NaiveDiff, 0.95);

	// Remove all our defined render passes
	renderer.RemoveAllRenderPasses();
	return true;
}

} // namespace cobalt::graphics::testing
