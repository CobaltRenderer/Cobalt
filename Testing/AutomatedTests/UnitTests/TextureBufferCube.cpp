// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../UnitTestBase.h"
namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

namespace {
// Define our shader programs
const std::string FragmentShader = R"(
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
uniform TextureCube colorTexture;
uniform SamplerState colorTexture_CombinedSampler;

struct VSOutput {
    float4 position : SV_POSITION;
    float3 sampleDirection : sampleDirection;
};

float4 main(VSOutput IN) : SV_TARGET0
{
    return colorTexture.Sample(colorTexture_CombinedSampler, normalize(IN.sampleDirection));
}
)";

std::vector<V4UInt8> CreateFaceData(uint32_t faceLength, const V4UInt8& primaryColor, const V4UInt8& secondaryColor)
{
	std::vector<V4UInt8> imageData;
	imageData.resize((size_t)faceLength * (size_t)faceLength);
	for (uint32_t posY = 0; posY < faceLength; ++posY)
	{
		for (uint32_t posX = 0; posX < faceLength; ++posX)
		{
			imageData[posX + (posY * faceLength)] = (posX < (faceLength / 2)) ? primaryColor : secondaryColor;
		}
	}
	return imageData;
}

std::vector<V4UInt8> CreatePerspectiveFaceData(uint32_t faceLength, const V4UInt8& primaryColor, const V4UInt8& secondaryColor, const V4UInt8& accentColor)
{
	std::vector<V4UInt8> imageData;
	imageData.resize((size_t)faceLength * (size_t)faceLength);
	const uint32_t checkerSize = std::max(1U, faceLength / 8U);
	const uint32_t lineHalfWidth = std::max(1U, faceLength / 24U);
	const uint32_t center = faceLength / 2U;
	const int faceEnd = (int)faceLength - 1;
	for (uint32_t posY = 0; posY < faceLength; ++posY)
	{
		for (uint32_t posX = 0; posX < faceLength; ++posX)
		{
			bool usePrimary = ((((posX / checkerSize) + (posY / checkerSize)) & 1U) == 0U);
			V4UInt8 pixelColor = usePrimary ? primaryColor : secondaryColor;
			if ((std::abs((int)posX - (int)center) <= (int)lineHalfWidth) ||
			    (std::abs((int)posY - (int)center) <= (int)lineHalfWidth) ||
			    (std::abs((int)posX - (int)posY) <= (int)lineHalfWidth) ||
			    (std::abs((faceEnd - (int)posX) - (int)posY) <= (int)lineHalfWidth))
			{
				pixelColor = accentColor;
			}
			imageData[posX + (posY * faceLength)] = pixelColor;
		}
	}
	return imageData;
}
} // namespace

DEFINE_UNIT_TEST_WITH_BASE("Resources/Images/TextureBufferCube", UnitTestBase)
{
	// Retrieve our important objects
	auto& renderer = session.Renderer();

	// Define the framebuffer and render scene
	TexturedQuadSceneHelper::Scene scene;
	REQUIRE(TexturedQuad().InitializeScene(this, session, Geometry(), FragmentShader, scene));

	// Create our texture buffer and load our image data into it
	const uint32_t faceLength = 128;
	std::vector<std::vector<V4UInt8>> textureData;
	textureData.push_back(CreateFaceData(faceLength, V4UInt8(255, 64, 64, 255), V4UInt8(255, 224, 64, 255)));
	textureData.push_back(TexturedQuad().CreateSolidColorImageData((size_t)faceLength * (size_t)faceLength, V4UInt8(32, 255, 96, 255)));
	textureData.push_back(TexturedQuad().CreateSolidColorImageData((size_t)faceLength * (size_t)faceLength, V4UInt8(96, 255, 255, 255)));
	textureData.push_back(TexturedQuad().CreateSolidColorImageData((size_t)faceLength * (size_t)faceLength, V4UInt8(255, 96, 224, 255)));
	textureData.push_back(TexturedQuad().CreateSolidColorImageData((size_t)faceLength * (size_t)faceLength, V4UInt8(224, 224, 224, 255)));
	textureData.push_back(CreateFaceData(faceLength, V4UInt8(64, 160, 255, 255), V4UInt8(224, 64, 255, 255)));
	auto texture = renderer.CreateTextureBufferCube();
	texture->SetTextureDimensions(faceLength);
	texture->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
	texture->SetUsageFlags(ITextureBuffer::UsageFlags::ShaderInput);
	texture->SetPerformanceHints(ITextureBuffer::PerformanceHint::WriteOften | ITextureBuffer::PerformanceHint::ReadNever, ITextureBuffer::PerformanceHint::WriteNever | ITextureBuffer::PerformanceHint::ReadOften);
	REQUIRE(texture->SetInitialData(textureData[0], ITextureBuffer::CubeMapFace::PositiveX));
	REQUIRE(texture->SetInitialData(textureData[1], ITextureBuffer::CubeMapFace::NegativeX));
	REQUIRE(texture->SetInitialData(textureData[2], ITextureBuffer::CubeMapFace::PositiveY));
	REQUIRE(texture->SetInitialData(textureData[3], ITextureBuffer::CubeMapFace::NegativeY));
	REQUIRE(texture->SetInitialData(textureData[4], ITextureBuffer::CubeMapFace::PositiveZ));
	REQUIRE(texture->SetInitialData(textureData[5], ITextureBuffer::CubeMapFace::NegativeZ));
	REQUIRE(texture->AllocateMemory());

	// Create a texture sampler to read the texture data from the shader
	auto sampler = renderer.CreateTextureSamplerCube();
	sampler->SetTextureFilterMode(ITextureSampler::FilterMode::Nearest, ITextureSampler::FilterMode::Nearest);
	sampler->SetTextureWrapMode(ITextureSampler::WrapMode::ClampToEdge, ITextureSampler::WrapMode::ClampToEdge);

	// Bind our texture to the renderable
	scene.renderableNode->BindTextureWithCombinedSampler(scene.shaderProgram->GetTextureId("colorTexture"), texture.get(), sampler.get());

	// Capture an image of the positive X face
	scene.renderableNode->SetStateValue(scene.shaderProgram->GetStateValueId("faceIndex"), V1UInt32(0));
	auto frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("PositiveXFace", "A cube texture positive X face with warm vertical bands.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Capture an image of the negative Z face
	scene.renderableNode->SetStateValue(scene.shaderProgram->GetStateValueId("faceIndex"), V1UInt32(5));
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("NegativeZFace", "A cube texture negative Z face with cool vertical bands.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Replace the middle columns of the negative Z face with white data
	std::vector<V4UInt8> whiteColumns;
	whiteColumns.resize((size_t)(faceLength / 3) * (size_t)faceLength, V4UInt8(255, 255, 255, 255));
	REQUIRE(texture->QueueDataUpdate(whiteColumns, ITextureBuffer::CubeMapFace::NegativeZ, 0, V2UInt32(faceLength / 3, 0), V2UInt32(faceLength / 3, faceLength)));

	// Capture an image of the negative Z face
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("NegativeZFaceWhiteMiddle", "A cube texture negative Z face with cool vertical bands, with a white strip through the middle.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Capture images with each texture wrap mode
	REQUIRE(TexturedQuad().UpdateTexCoords(scene, V2Float32(2.5f, 2.5f), V2Float32(-0.75f, -0.75f)));
	sampler->SetTextureWrapMode(ITextureSampler::WrapMode::ClampToEdge, ITextureSampler::WrapMode::ClampToEdge);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("WrapClampToEdge", "A cube texture sampled with clamp-to-edge wrap mode.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	sampler->SetTextureWrapMode(ITextureSampler::WrapMode::Repeat, ITextureSampler::WrapMode::Repeat);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("WrapRepeat", "A cube texture sampled with repeat wrap mode.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	sampler->SetTextureWrapMode(ITextureSampler::WrapMode::RepeatMirrored, ITextureSampler::WrapMode::RepeatMirrored);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("WrapRepeatMirrored", "A cube texture sampled with mirrored-repeat wrap mode.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Capture images with each texture filter mode
	auto randomFilterPatch = TexturedQuad().CreateRandomPatchData(TexturedQuad().PatchSize(), TexturedQuad().PatchSize());
	auto filterPatchOffset = V2UInt32(TexturedQuad().GetPatchStart(faceLength), TexturedQuad().GetPatchStart(faceLength));
	REQUIRE(texture->QueueDataUpdate(randomFilterPatch, ITextureBuffer::CubeMapFace::NegativeZ, 0, filterPatchOffset, V2UInt32(TexturedQuad().PatchSize(), TexturedQuad().PatchSize())));
	REQUIRE(TexturedQuad().UpdateTexCoords(scene, TexturedQuad().GetTexCoordScale(V2UInt32(faceLength, faceLength)), TexturedQuad().GetTexCoordOffset(V2UInt32(faceLength, faceLength))));

	sampler->SetTextureWrapMode(ITextureSampler::WrapMode::ClampToEdge, ITextureSampler::WrapMode::ClampToEdge);
	sampler->SetTextureFilterMode(ITextureSampler::FilterMode::Nearest, ITextureSampler::FilterMode::Nearest);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("FilterNearest", "A magnified cube texture sampled with nearest filtering.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	sampler->SetTextureFilterMode(ITextureSampler::FilterMode::Linear, ITextureSampler::FilterMode::Linear);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	scene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("FilterLinear", "A magnified cube texture sampled with linear filtering.", std::move(frameBufferCapture), IImageDiff::Algorithm::RegionRanges | IImageDiff::Algorithm::NaiveDiff, 0.95);

	// Clear the scene content, and wait for destruction to be complete. We need to do this so we can unbind and re-bind
	// to the same window here without rendering a frame.
	renderer.RemoveAllRenderPasses();
	scene = TexturedQuadSceneHelper::Scene();
	renderer.WaitForDeferredDeletionComplete();

	// Replace each face with dense patterned data so the perspective capture shows useful face transitions and
	// filtering behavior across the sphere rather than mostly flat face colors.
	const std::array<ITextureBuffer::CubeMapFace, 6> cubeFaces = {
	  ITextureBuffer::CubeMapFace::PositiveX,
	  ITextureBuffer::CubeMapFace::NegativeX,
	  ITextureBuffer::CubeMapFace::PositiveY,
	  ITextureBuffer::CubeMapFace::NegativeY,
	  ITextureBuffer::CubeMapFace::PositiveZ,
	  ITextureBuffer::CubeMapFace::NegativeZ};
	const std::array<std::vector<V4UInt8>, 6> perspectiveTextureData = {
	  CreatePerspectiveFaceData(faceLength, V4UInt8(255, 64, 64, 255), V4UInt8(255, 224, 64, 255), V4UInt8(255, 255, 255, 255)),
	  CreatePerspectiveFaceData(faceLength, V4UInt8(32, 255, 96, 255), V4UInt8(32, 96, 255, 255), V4UInt8(255, 255, 255, 255)),
	  CreatePerspectiveFaceData(faceLength, V4UInt8(96, 255, 255, 255), V4UInt8(64, 160, 255, 255), V4UInt8(255, 255, 255, 255)),
	  CreatePerspectiveFaceData(faceLength, V4UInt8(255, 96, 224, 255), V4UInt8(224, 64, 255, 255), V4UInt8(255, 255, 255, 255)),
	  CreatePerspectiveFaceData(faceLength, V4UInt8(224, 224, 224, 255), V4UInt8(128, 128, 128, 255), V4UInt8(255, 64, 64, 255)),
	  CreatePerspectiveFaceData(faceLength, V4UInt8(64, 160, 255, 255), V4UInt8(224, 64, 255, 255), V4UInt8(255, 255, 255, 255))};
	for (size_t faceIndex = 0; faceIndex < cubeFaces.size(); ++faceIndex)
	{
		REQUIRE(texture->QueueDataUpdate(perspectiveTextureData[faceIndex], cubeFaces[faceIndex], 0));
	}

	// Define a curved display scene so we can verify that the cubemap samples correctly across a useful spread of
	// directions rather than just on a single face-aligned quad.
	MipmappingHelper::CubeMapDisplayScene perspectiveScene;
	REQUIRE(Mipmapping().InitializeCubeMapDisplayScene(this, session, Geometry(), PerspectiveVertexShader, PerspectiveFragmentShader, perspectiveScene));
	perspectiveScene.renderableNode->BindTextureWithCombinedSampler(perspectiveScene.shaderProgram->GetTextureId("colorTexture"), texture.get(), sampler.get());
	auto viewProj = Transform().LookAtCenterPerspective(session.TestWindowSizeAsFloat(), V3Float32(6.0f, -6.0f, 6.0f), 60.0f);
	perspectiveScene.renderableNode->SetStateValue(perspectiveScene.shaderProgram->GetStateValueId("viewProj"), viewProj);

	sampler->SetTextureWrapMode(ITextureSampler::WrapMode::ClampToEdge, ITextureSampler::WrapMode::ClampToEdge);
	sampler->SetTextureFilterMode(ITextureSampler::FilterMode::Nearest, ITextureSampler::FilterMode::Nearest);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	perspectiveScene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("PerspectiveDiagonalNearest", "A cube texture sampled on a diagonally viewed sphere using nearest filtering.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	sampler->SetTextureFilterMode(ITextureSampler::FilterMode::Linear, ITextureSampler::FilterMode::Linear);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	perspectiveScene.frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("PerspectiveDiagonalLinear", "A cube texture sampled on a diagonally viewed sphere using linear filtering.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Remove all our defined render passes
	renderer.RemoveAllRenderPasses();
	return true;
}

} // namespace cobalt::graphics::testing
