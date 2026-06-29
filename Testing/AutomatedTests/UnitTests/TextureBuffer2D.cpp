// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../GeometryHelper.h"
#include "../UnitTestBase.h"
namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

// Define our shader programs
const std::string VertexShader = R"(
struct VSInput {
    float3 position : position;
    float2 texCoord : texCoord;
};

struct VSOutput {
    float4 position : SV_POSITION;
    float2 texCoord : texCoord;
};

VSOutput main(VSInput IN)
{
    VSOutput OUT;

    OUT.position = float4(IN.position, 1.0f);
    OUT.texCoord = IN.texCoord;

    return OUT;
}
)";
const std::string FragmentShader = R"(
uniform Texture2D colorTexture;
uniform SamplerState colorTexture_CombinedSampler;

struct VSOutput {
    float4 position : SV_POSITION;
    float2 texCoord : texCoord;
};

float4 main(VSOutput IN) : SV_TARGET0
{
    float4 color = colorTexture.Sample(colorTexture_CombinedSampler, IN.texCoord.xy);
    return color;
}
)";

DEFINE_UNIT_TEST_WITH_BASE("Resources/Images/TextureBuffer2D", UnitTestBase)
{
	// Retrieve our important objects
	auto& renderer = session.Renderer();
	auto& uiThread = session.UIThread();
	auto& testWindowInfo = *session.TestWindowPlatformInfo();

	// Define the framebuffer
	auto mainWindowFrameBuffer = renderer.CreateFrameBuffer();
	mainWindowFrameBuffer->DefineViewportRegion(V2UInt32(0, 0), session.TestWindowSize());
	REQUIRE(uiThread.InvokeSync([&] { return mainWindowFrameBuffer->BindWindow(testWindowInfo, IFrameBuffer::WindowDepthStencilMode::DepthUNorm24); }));

	// Create and compile our shader program
	auto shaderProgram = renderer.CreateShaderProgram();
	REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(VertexShader)));
	REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(FragmentShader)));
	REQUIRE(shaderProgram->CompileProgram());

	// Retrieve our shader attribute IDs
	auto positionAttributeId = shaderProgram->GetVertexAttributeId("position");
	auto texCoordAttributeId = shaderProgram->GetVertexAttributeId("texCoord");

	// Generate data for the object we want to render
	std::vector<V3Float32> positionVertexData;
	std::vector<V3Float32> normalVertexData;
	std::vector<V2Float32> texCoordData;
	std::vector<V1UInt32> indexData;
	Geometry().CreatePrimitiveSquareAsTriangles(10, positionVertexData, normalVertexData, texCoordData, indexData);

	// Create our vertex buffer and populate it with data
	VertexAttribute<V3Float32> vertexAttributePosition(positionVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	VertexAttribute<V2Float32> vertexAttributeTexCoord(texCoordData.size(), IVertexAttribute::PerformanceHint::WriteRarely | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	auto vertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttributePosition));
	REQUIRE(vertexAttributePosition.SetInitialData(positionVertexData));
	REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttributeTexCoord));
	REQUIRE(vertexAttributeTexCoord.SetInitialData(texCoordData));
	REQUIRE(vertexBuffer->AllocateMemory());

	// Create our index buffer and populate it with data
	IndexAttribute<V1UInt32> indexAttribute(indexData.size(), IIndexAttribute::PerformanceHint::WriteNever | IIndexAttribute::PerformanceHint::ReadNever, IIndexAttribute::PerformanceHint::WriteNever | IIndexAttribute::PerformanceHint::ReadOften);
	auto indexBuffer = renderer.CreateIndexBuffer();
	REQUIRE(indexBuffer->BindIndexAttribute(indexAttribute));
	REQUIRE(indexAttribute.SetInitialData(indexData));
	REQUIRE(indexBuffer->AllocateMemory());

	// Create our texture buffer and load our image data into it
	auto colorLogoImage = Texture().LoadImageFromPngFile("CobaltLogoColorWithBlackBackground.png");
	REQUIRE(colorLogoImage != nullptr);
	auto texture = renderer.CreateTextureBuffer2D();
	texture->SetTextureDimensions(colorLogoImage->size);
	texture->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
	texture->SetUsageFlags(ITextureBuffer::UsageFlags::ShaderInput);
	texture->SetPerformanceHints(ITextureBuffer::PerformanceHint::WriteOften | ITextureBuffer::PerformanceHint::ReadNever, ITextureBuffer::PerformanceHint::WriteNever | ITextureBuffer::PerformanceHint::ReadOften);
	REQUIRE(texture->SetInitialData(colorLogoImage->mipmapTextureData[0].data(), colorLogoImage->mipmapTextureData[0].size()));
	REQUIRE(texture->AllocateMemory());

	// Create a texture sampler to read the texture data from the shader
	auto sampler = renderer.CreateTextureSampler2D();
	sampler->SetTextureFilterMode(ITextureSampler::FilterMode::Nearest, ITextureSampler::FilterMode::Nearest);
	sampler->SetTextureWrapMode(ITextureSampler::WrapMode::ClampToEdge, ITextureSampler::WrapMode::ClampToEdge);

	// Create our renderable node
	auto renderableNode = renderer.CreateRenderableNode();
	REQUIRE(renderableNode->BindVertexAttribute(vertexAttributePosition, positionAttributeId));
	REQUIRE(renderableNode->BindVertexAttribute(vertexAttributeTexCoord, texCoordAttributeId));
	REQUIRE(renderableNode->BindIndexAttribute(indexAttribute));
	REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));

	// Bind our texture to the renderable
	renderableNode->BindTextureWithCombinedSampler(shaderProgram->GetTextureId("colorTexture"), texture.get(), sampler.get());

	// Create our state group node
	auto groupNode = renderer.CreateStateGroupNode();
	groupNode->AddChildNode(renderableNode.get());

	// Create our program node
	auto programNode = renderer.CreateProgramNode();
	REQUIRE(programNode->BindShaderProgram(shaderProgram.get()));
	programNode->AddChildNode(groupNode.get());

	// Create our render pass node
	auto renderPassNode = renderer.CreateRenderPassNode();
	renderPassNode->BindFrameBuffer(mainWindowFrameBuffer.get());
	renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
	renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Depth, 0, V4Float32(1.0f, 0.0f, 0.0f, 0.0f));
	renderPassNode->AddChildNode(programNode.get());

	// Bind our render tree to the renderer
	renderer.SetRenderPasses(&renderPassNode, 1);

	// Capture an image of the scene
	auto frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("CobaltLogoColor", "Cobalt color logo on a black square", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Replace the horizontal strip in the middle 1/3 of the image with a white logo version
	auto whiteLogoImage = Texture().LoadImageFromPngFile("CobaltLogoWhiteWithBlackBackground.png");
	REQUIRE(whiteLogoImage != nullptr);
	V2UInt32 whiteLogoImageSegmentOffset(0, colorLogoImage->size.Y() / 3);
	V2UInt32 whiteLogoImageSegmentSize(colorLogoImage->size.X(), colorLogoImage->size.Y() / 3);
	std::vector<V4UInt8> whiteLogoImageSegment;
	whiteLogoImageSegment.resize((size_t)whiteLogoImageSegmentSize.X() * (size_t)whiteLogoImageSegmentSize.Y());
	for (unsigned int posY = 0; posY < whiteLogoImageSegmentSize.Y(); ++posY)
	{
		for (unsigned int posX = 0; posX < whiteLogoImageSegmentSize.X(); ++posX)
		{
			whiteLogoImageSegment[(posY * whiteLogoImageSegmentSize.X()) + posX] = whiteLogoImage->mipmapTextureData[0][((posY + whiteLogoImageSegmentOffset.Y()) * whiteLogoImage->size.X()) + (whiteLogoImageSegmentOffset.X() + posX)];
		}
	}
	REQUIRE(texture->QueueDataUpdate(whiteLogoImageSegment, 0, whiteLogoImageSegmentOffset, whiteLogoImageSegmentSize));

	// Capture an image of the scene
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("CobaltLogoWhiteStrip", "Cobalt color logo on a black square, with a white logo strip across the middle.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Replace the vertical strip on the right 1/2 of the image with a white background version. Deliberately testing
	// without an explicit size here to ensure it fills the remainder of the image from the offset position correctly.
	auto blackLogoImage = Texture().LoadImageFromPngFile("CobaltLogoBlackWithWhiteBackground.png");
	REQUIRE(blackLogoImage != nullptr);
	V2UInt32 blackLogoImageSegmentOffset(colorLogoImage->size.X() / 2, 0);
	V2UInt32 blackLogoImageSegmentSize(colorLogoImage->size.X() / 2, colorLogoImage->size.Y());
	std::vector<V4UInt8> blackLogoImageSegment;
	blackLogoImageSegment.resize((size_t)blackLogoImageSegmentSize.X() * (size_t)blackLogoImageSegmentSize.Y());
	for (unsigned int posY = 0; posY < blackLogoImageSegmentSize.Y(); ++posY)
	{
		for (unsigned int posX = 0; posX < blackLogoImageSegmentSize.X(); ++posX)
		{
			blackLogoImageSegment[(posY * blackLogoImageSegmentSize.X()) + posX] = blackLogoImage->mipmapTextureData[0][((posY + blackLogoImageSegmentOffset.Y()) * blackLogoImage->size.X()) + (blackLogoImageSegmentOffset.X() + posX)];
		}
	}
	REQUIRE(texture->QueueDataUpdate(blackLogoImageSegment, 0, blackLogoImageSegmentOffset));

	// Capture an image of the scene
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("CobaltLogoBlackStrip", "Cobalt color logo on a black square, with a white logo strip across the middle, and black logo on a white background strip on right side.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Adjust the texture coordinates to sample outside the image bounds so that wrap modes are visible.
	auto wrapTexCoordData = TexturedQuad().CreateScaledOffsetTexCoords(texCoordData, V2Float32(2.5f, 2.5f), V2Float32(-0.75f, -0.75f));
	REQUIRE(vertexAttributeTexCoord.QueueDataUpdate(wrapTexCoordData.data(), wrapTexCoordData.size()));

	sampler->SetTextureWrapMode(ITextureSampler::WrapMode::ClampToEdge, ITextureSampler::WrapMode::ClampToEdge);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("WrapClampToEdge", "Cobalt logo texture sampled with clamp-to-edge wrap mode.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	sampler->SetTextureWrapMode(ITextureSampler::WrapMode::Repeat, ITextureSampler::WrapMode::Repeat);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("WrapRepeat", "Cobalt logo texture sampled with repeat wrap mode.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	sampler->SetTextureWrapMode(ITextureSampler::WrapMode::RepeatMirrored, ITextureSampler::WrapMode::RepeatMirrored);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("WrapRepeatMirrored", "Cobalt logo texture sampled with mirrored-repeat wrap mode.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Zoom into a small region of the texture so that filter mode differences are visible.
	auto randomFilterPatch = TexturedQuad().CreateRandomPatchData(TexturedQuad().PatchSize(), TexturedQuad().PatchSize());
	auto filterPatchOffset = V2UInt32(TexturedQuad().GetPatchStart(colorLogoImage->size.X()), TexturedQuad().GetPatchStart(colorLogoImage->size.Y()));
	REQUIRE(texture->QueueDataUpdate(randomFilterPatch, 0, filterPatchOffset, V2UInt32(TexturedQuad().PatchSize(), TexturedQuad().PatchSize())));
	auto filterTexCoordData = TexturedQuad().CreateScaledOffsetTexCoords(texCoordData, TexturedQuad().GetTexCoordScale(colorLogoImage->size), TexturedQuad().GetTexCoordOffset(colorLogoImage->size));
	REQUIRE(vertexAttributeTexCoord.QueueDataUpdate(filterTexCoordData.data(), filterTexCoordData.size()));

	sampler->SetTextureWrapMode(ITextureSampler::WrapMode::ClampToEdge, ITextureSampler::WrapMode::ClampToEdge);
	sampler->SetTextureFilterMode(ITextureSampler::FilterMode::Nearest, ITextureSampler::FilterMode::Nearest);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("FilterNearest", "A magnified Cobalt logo texture sampled with nearest filtering.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	sampler->SetTextureFilterMode(ITextureSampler::FilterMode::Linear, ITextureSampler::FilterMode::Linear);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("FilterLinear", "A magnified Cobalt logo texture sampled with linear filtering.", std::move(frameBufferCapture), IImageDiff::Algorithm::RegionRanges | IImageDiff::Algorithm::NaiveDiff, 0.95);

	// Remove all our defined render passes
	renderer.RemoveAllRenderPasses();
	return true;
}

} // namespace cobalt::graphics::testing
