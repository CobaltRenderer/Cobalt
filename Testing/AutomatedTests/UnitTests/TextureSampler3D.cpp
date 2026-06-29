// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../UnitTestBase.h"
namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

namespace {
// Define our shader programs
const std::string FragmentShader = R"(
uniform Texture3D colorTexture;
uniform SamplerState colorSampler;
uniform float sampleDepth;

struct VSOutput {
    float4 position : SV_POSITION;
    float2 texCoord : texCoord;
};

float4 main(VSOutput IN) : SV_TARGET0
{
    return colorTexture.Sample(colorSampler, float3(IN.texCoord.xy, sampleDepth));
}
)";

std::vector<V4UInt8> CreateSliceData(uint32_t imageWidth, uint32_t imageHeight, const V4UInt8& colorA, const V4UInt8& colorB)
{
	std::vector<V4UInt8> imageData;
	imageData.resize((size_t)imageWidth * (size_t)imageHeight);
	for (uint32_t posY = 0; posY < imageHeight; ++posY)
	{
		for (uint32_t posX = 0; posX < imageWidth; ++posX)
		{
			imageData[posX + (posY * imageWidth)] = (posY < (imageHeight / 2)) ? colorA : colorB;
		}
	}
	return imageData;
}

std::vector<V4UInt8> CreateVolumeData(const V3UInt32& imageDimensions)
{
	std::vector<V4UInt8> volumeData;
	volumeData.resize((size_t)imageDimensions.X() * (size_t)imageDimensions.Y() * (size_t)imageDimensions.Z());
	auto slice0 = CreateSliceData(imageDimensions.X(), imageDimensions.Y(), V4UInt8(255, 64, 64, 255), V4UInt8(255, 224, 64, 255));
	auto slice1 = CreateSliceData(imageDimensions.X(), imageDimensions.Y(), V4UInt8(64, 160, 255, 255), V4UInt8(192, 64, 255, 255));
	for (uint32_t posY = 0; posY < imageDimensions.Y(); ++posY)
	{
		for (uint32_t posX = 0; posX < imageDimensions.X(); ++posX)
		{
			volumeData[posX + (posY * imageDimensions.X())] = slice0[posX + (posY * imageDimensions.X())];
			volumeData[posX + (posY * imageDimensions.X()) + ((size_t)imageDimensions.X() * (size_t)imageDimensions.Y())] = slice1[posX + (posY * imageDimensions.X())];
		}
	}
	return volumeData;
}
} // namespace

DEFINE_UNIT_TEST_WITH_BASE("Resources/ImageSamplers/TextureSampler3D", UnitTestBase)
{
	// Ensure separate texture samplers are supported by the current renderer
	if (!session.Device().IsFeatureSupported(IGraphicsDevice::Feature::SeparateTextureSamplers))
	{
		session.AddTestSkipped("Skipped test", "This test was skipped, as the current renderer doesn't support separate texture samplers on this device.");
		return true;
	}

	// Retrieve our important objects
	auto& renderer = session.Renderer();

	// Define the framebuffer and render scene
	TexturedQuadSceneHelper::Scene scene;
	REQUIRE(TexturedQuad().InitializeScene(this, session, Geometry(), FragmentShader, scene));

	// Create our texture buffer and load our image data into it
	const V3UInt32 imageDimensions(128, 128, 2);
	auto textureData = CreateVolumeData(imageDimensions);
	auto texture = renderer.CreateTextureBuffer3D();
	texture->SetTextureDimensions(imageDimensions);
	texture->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
	texture->SetUsageFlags(ITextureBuffer::UsageFlags::ShaderInput);
	texture->SetPerformanceHints(ITextureBuffer::PerformanceHint::WriteOften | ITextureBuffer::PerformanceHint::ReadNever, ITextureBuffer::PerformanceHint::WriteNever | ITextureBuffer::PerformanceHint::ReadOften);
	REQUIRE(texture->SetInitialData(textureData, 0));
	REQUIRE(texture->AllocateMemory());

	// Create a texture sampler to read the texture data from the shader
	auto sampler = renderer.CreateTextureSampler3D();
	sampler->SetTextureFilterMode(ITextureSampler::FilterMode::Nearest, ITextureSampler::FilterMode::Nearest);
	sampler->SetTextureWrapMode(ITextureSampler::WrapMode::ClampToEdge, ITextureSampler::WrapMode::ClampToEdge, ITextureSampler::WrapMode::ClampToEdge);

	// Bind our texture and sampler to the renderable
	scene.renderableNode->BindTexture(scene.shaderProgram->GetTextureId("colorTexture"), texture.get());
	scene.renderableNode->BindSampler(scene.shaderProgram->GetSamplerId("colorSampler"), sampler.get());

	// Capture an image of slice 0
	scene.renderableNode->SetStateValue(scene.shaderProgram->GetStateValueId("sampleDepth"), V1Float32(0.0f));
	auto frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("WarmHorizontalBands", "Two warm-coloured horizontal bands sourced from slice 0 of a 3D texture using a separate sampler.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Capture an image of slice 1
	scene.renderableNode->SetStateValue(scene.shaderProgram->GetStateValueId("sampleDepth"), V1Float32(1.0f));
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("CoolHorizontalBands", "Two cool-coloured horizontal bands sourced from slice 1 of a 3D texture using a separate sampler.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Replace the middle rows of slice 1 with white data
	std::vector<V4UInt8> whiteRows;
	whiteRows.resize((size_t)imageDimensions.X() * ((size_t)imageDimensions.Y() / 3), V4UInt8(255, 255, 255, 255));
	REQUIRE(texture->QueueDataUpdate(whiteRows, 0, V3UInt32(0, imageDimensions.Y() / 3, 1), V3UInt32(imageDimensions.X(), imageDimensions.Y() / 3, 1)));

	// Capture an image of slice 1
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("CoolHorizontalBandsWhiteMiddle", "Two cool-coloured horizontal bands sourced from slice 1 of a 3D texture using a separate sampler, with a white strip across the middle.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Capture images with each texture wrap mode
	REQUIRE(TexturedQuad().UpdateTexCoords(scene, V2Float32(2.5f, 2.5f), V2Float32(-0.75f, -0.75f)));
	sampler->SetTextureWrapMode(ITextureSampler::WrapMode::ClampToEdge, ITextureSampler::WrapMode::ClampToEdge, ITextureSampler::WrapMode::ClampToEdge);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("WrapClampToEdge", "A 3D texture sampled with a separate sampler using clamp-to-edge wrap mode.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	sampler->SetTextureWrapMode(ITextureSampler::WrapMode::Repeat, ITextureSampler::WrapMode::Repeat, ITextureSampler::WrapMode::Repeat);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("WrapRepeat", "A 3D texture sampled with a separate sampler using repeat wrap mode.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	sampler->SetTextureWrapMode(ITextureSampler::WrapMode::RepeatMirrored, ITextureSampler::WrapMode::RepeatMirrored, ITextureSampler::WrapMode::RepeatMirrored);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("WrapRepeatMirrored", "A 3D texture sampled with a separate sampler using mirrored-repeat wrap mode.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Capture images with each texture filter mode
	auto randomFilterPatch = TexturedQuad().CreateRandomPatchData(TexturedQuad().PatchSize(), TexturedQuad().PatchSize());
	auto filterPatchOffset = V3UInt32(TexturedQuad().GetPatchStart(imageDimensions.X()), TexturedQuad().GetPatchStart(imageDimensions.Y()), 1);
	REQUIRE(texture->QueueDataUpdate(randomFilterPatch, 0, filterPatchOffset, V3UInt32(TexturedQuad().PatchSize(), TexturedQuad().PatchSize(), 1)));
	REQUIRE(TexturedQuad().UpdateTexCoords(scene, TexturedQuad().GetTexCoordScale(V2UInt32(imageDimensions.X(), imageDimensions.Y())), TexturedQuad().GetTexCoordOffset(V2UInt32(imageDimensions.X(), imageDimensions.Y()))));

	sampler->SetTextureWrapMode(ITextureSampler::WrapMode::ClampToEdge, ITextureSampler::WrapMode::ClampToEdge, ITextureSampler::WrapMode::ClampToEdge);
	sampler->SetTextureFilterMode(ITextureSampler::FilterMode::Nearest, ITextureSampler::FilterMode::Nearest);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("FilterNearest", "A magnified 3D texture sampled with a separate sampler using nearest filtering.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	sampler->SetTextureFilterMode(ITextureSampler::FilterMode::Linear, ITextureSampler::FilterMode::Linear);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("FilterLinear", "A magnified 3D texture sampled with a separate sampler using linear filtering.", std::move(frameBufferCapture), IImageDiff::Algorithm::RegionRanges | IImageDiff::Algorithm::NaiveDiff, 0.95);

	// Replace the same patch region in both slices with different data and sample between them so that trilinear
	// filtering becomes visible along the depth axis.
	auto betweenSliceFilterPatch0 = TexturedQuad().CreateRandomPatchData(TexturedQuad().PatchSize(), TexturedQuad().PatchSize());
	auto betweenSliceFilterPatch1 = betweenSliceFilterPatch0;
	for (auto& pixel : betweenSliceFilterPatch1)
	{
		pixel = V4UInt8((uint8_t)(255 - pixel.Z()), (uint8_t)(255 - pixel.X()), (uint8_t)(255 - pixel.Y()), 255);
	}
	REQUIRE(texture->QueueDataUpdate(betweenSliceFilterPatch0, 0, V3UInt32(filterPatchOffset.X(), filterPatchOffset.Y(), 0), V3UInt32(TexturedQuad().PatchSize(), TexturedQuad().PatchSize(), 1)));
	REQUIRE(texture->QueueDataUpdate(betweenSliceFilterPatch1, 0, V3UInt32(filterPatchOffset.X(), filterPatchOffset.Y(), 1), V3UInt32(TexturedQuad().PatchSize(), TexturedQuad().PatchSize(), 1)));
	scene.renderableNode->SetStateValue(scene.shaderProgram->GetStateValueId("sampleDepth"), V1Float32(0.45f));

	sampler->SetTextureFilterMode(ITextureSampler::FilterMode::Nearest, ITextureSampler::FilterMode::Nearest);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("FilterNearestBetweenSlices", "A magnified 3D texture sampled between slices with a separate sampler using nearest filtering.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	sampler->SetTextureFilterMode(ITextureSampler::FilterMode::Linear, ITextureSampler::FilterMode::Linear);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("FilterLinearBetweenSlices", "A magnified 3D texture sampled between slices with a separate sampler using linear filtering.", std::move(frameBufferCapture), IImageDiff::Algorithm::RegionRanges | IImageDiff::Algorithm::NaiveDiff, 0.95);

	// Remove all our defined render passes
	renderer.RemoveAllRenderPasses();
	return true;
}

} // namespace cobalt::graphics::testing
