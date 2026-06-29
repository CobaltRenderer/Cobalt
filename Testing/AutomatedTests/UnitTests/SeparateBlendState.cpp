// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../UnitTestBase.h"

namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

// Define our blend test shader programs
const std::string BlendVertexShader = R"(
struct VSInput {
    float4 position : position;
};

struct VSOutput {
    float4 position : SV_POSITION;
};

VSOutput main(VSInput IN)
{
    VSOutput OUT;

    OUT.position = IN.position;

    return OUT;
}
)";

const std::string BlendFragmentShader = R"(
struct VSOutput {
    float4 position : SV_POSITION;
};

struct PSOutput {
    float4 color0 : SV_TARGET0;
    float4 color1 : SV_TARGET1;
};

uniform float4 drawColor;

PSOutput main(VSOutput IN)
{
    PSOutput OUT;

    OUT.color0 = drawColor;
    OUT.color1 = drawColor;

    return OUT;
}
)";

// Define our display shader programs
const std::string DisplayVertexShader = R"(
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

const std::string DisplayFragmentShader = R"(
struct VSOutput {
    float4 position : SV_POSITION;
    float2 texCoord : texCoord;
};

uniform Texture2D colorTexture;
uniform SamplerState colorTexture_CombinedSampler;

float4 main(VSOutput IN) : SV_TARGET0
{
    return colorTexture.Sample(colorTexture_CombinedSampler, IN.texCoord.xy);
}
)";

DEFINE_UNIT_TEST_WITH_BASE("StateGroup/SeparateBlendState", UnitTestBase)
{
	// Skip test unless the current device supports separate blend modes per render target
	if (!session.Device().IsFeatureSupported(IGraphicsDevice::Feature::SeparateBlendModePerTarget))
	{
		session.AddTestSkipped("Skipped test", "This test was skipped, as the current renderer doesn't support separate blend modes per render target on this device.");
		return true;
	}

	if (session.Device().GetFrameBufferLimits().maxFrameBufferColorAttachments < 2)
	{
		session.AddTestSkipped("Skipped test", "This test was skipped, as the current renderer doesn't support two framebuffer color attachments on this device.");
		return true;
	}

	// Retrieve our important objects
	auto& renderer = session.Renderer();
	auto& uiThread = session.UIThread();
	auto& testWindowInfo = *session.TestWindowPlatformInfo();

	// Define the colors that will make the two attachment results visibly different
	const V4Float32 backgroundColor(0.15f, 0.55f, 0.85f, 1.0f);
	const V4Float32 overlayColor(0.90f, 0.25f, 0.05f, 0.60f);

	// Generate data for the centred overlay quad we want to render
	std::vector<V4Float32> overlayVertexData;
	Geometry().CreateCenteredQuad(0.5f, 0.65f, overlayVertexData);

	// Generate data for the left and right display quads
	std::vector<V3Float32> leftDisplayPositionVertexData({
	  {-1.0f, -1.0f, 0.0f},
	  {-1.0f, 1.0f, 0.0f},
	  {0.0f, 1.0f, 0.0f},
	  {-1.0f, -1.0f, 0.0f},
	  {0.0f, 1.0f, 0.0f},
	  {0.0f, -1.0f, 0.0f},
	});
	std::vector<V3Float32> rightDisplayPositionVertexData({
	  {0.0f, -1.0f, 0.0f},
	  {0.0f, 1.0f, 0.0f},
	  {1.0f, 1.0f, 0.0f},
	  {0.0f, -1.0f, 0.0f},
	  {1.0f, 1.0f, 0.0f},
	  {1.0f, -1.0f, 0.0f},
	});
	std::vector<V2Float32> displayTexCoordVertexData;
	if (session.ApiFamily() == IRendererPlugin::ApiFamily::OpenGL)
	{
		displayTexCoordVertexData = {
		  {0.0f, 0.0f},
		  {0.0f, 1.0f},
		  {1.0f, 1.0f},
		  {0.0f, 0.0f},
		  {1.0f, 1.0f},
		  {1.0f, 0.0f},
		};
	}
	else
	{
		displayTexCoordVertexData = {
		  {0.0f, 1.0f},
		  {0.0f, 0.0f},
		  {1.0f, 0.0f},
		  {0.0f, 1.0f},
		  {1.0f, 0.0f},
		  {1.0f, 1.0f},
		};
	}

	// Create our two color attachment textures
	auto colorTexture0 = renderer.CreateTextureBuffer2D();
	colorTexture0->SetUsageFlags(ITextureBuffer::UsageFlags::FrameBufferOutput | ITextureBuffer::UsageFlags::ShaderInput);
	colorTexture0->SetTextureFormat(ITextureBuffer2D::ImageFormat::RGBA, ITextureBuffer2D::DataFormat::UNorm8);
	colorTexture0->SetTextureDimensions(V2UInt32(session.TestWindowSize()));
	REQUIRE(colorTexture0->AllocateMemory());

	auto colorTexture1 = renderer.CreateTextureBuffer2D();
	colorTexture1->SetUsageFlags(ITextureBuffer::UsageFlags::FrameBufferOutput | ITextureBuffer::UsageFlags::ShaderInput);
	colorTexture1->SetTextureFormat(ITextureBuffer2D::ImageFormat::RGBA, ITextureBuffer2D::DataFormat::UNorm8);
	colorTexture1->SetTextureDimensions(V2UInt32(session.TestWindowSize()));
	REQUIRE(colorTexture1->AllocateMemory());

	// Define the offscreen MRT framebuffer
	auto offscreenFrameBuffer = renderer.CreateFrameBuffer();
	offscreenFrameBuffer->DefineViewportRegion(V2UInt32(0, 0), session.TestWindowSize());
	REQUIRE(offscreenFrameBuffer->BindTexture(colorTexture0.get(), IFrameBuffer::AttachmentType::Color, 0));
	REQUIRE(offscreenFrameBuffer->BindTexture(colorTexture1.get(), IFrameBuffer::AttachmentType::Color, 1));

	// Define the window framebuffer used to display the attachment results on screen
	auto windowFrameBuffer = renderer.CreateFrameBuffer();
	windowFrameBuffer->DefineViewportRegion(V2UInt32(0, 0), session.TestWindowSize());
	REQUIRE(uiThread.InvokeSync([&] { return windowFrameBuffer->BindWindow(testWindowInfo, IFrameBuffer::WindowDepthStencilMode::None); }));

	// Create and compile our MRT blend shader program
	auto blendShaderProgram = renderer.CreateShaderProgram();
	REQUIRE(blendShaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(BlendVertexShader)));
	REQUIRE(blendShaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(BlendFragmentShader)));
	REQUIRE(blendShaderProgram->CompileProgram());

	// Create and compile our display shader program
	auto displayShaderProgram = renderer.CreateShaderProgram();
	REQUIRE(displayShaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(DisplayVertexShader)));
	REQUIRE(displayShaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(DisplayFragmentShader)));
	REQUIRE(displayShaderProgram->CompileProgram());

	// Retrieve our shader attribute and state IDs
	auto blendPositionAttributeId = blendShaderProgram->GetVertexAttributeId("position");
	auto drawColorStateId = blendShaderProgram->GetStateValueId("drawColor");
	auto displayPositionAttributeId = displayShaderProgram->GetVertexAttributeId("position");
	auto displayTexCoordAttributeId = displayShaderProgram->GetVertexAttributeId("texCoord");
	auto displayTextureId = displayShaderProgram->GetTextureId("colorTexture");

	// Create our vertex buffers and populate them with data
	VertexAttribute<V4Float32> overlayVertexAttribute(overlayVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	VertexAttribute<V3Float32> leftDisplayPositionVertexAttribute(leftDisplayPositionVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	VertexAttribute<V3Float32> rightDisplayPositionVertexAttribute(rightDisplayPositionVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	VertexAttribute<V2Float32> displayTexCoordVertexAttribute(displayTexCoordVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);

	auto blendVertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE(blendVertexBuffer->BindVertexAttribute(overlayVertexAttribute));
	REQUIRE(overlayVertexAttribute.SetInitialData(overlayVertexData));
	REQUIRE(blendVertexBuffer->AllocateMemory());

	auto displayVertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE(displayVertexBuffer->BindVertexAttribute(leftDisplayPositionVertexAttribute));
	REQUIRE(leftDisplayPositionVertexAttribute.SetInitialData(leftDisplayPositionVertexData));
	REQUIRE(displayVertexBuffer->BindVertexAttribute(rightDisplayPositionVertexAttribute));
	REQUIRE(rightDisplayPositionVertexAttribute.SetInitialData(rightDisplayPositionVertexData));
	REQUIRE(displayVertexBuffer->BindVertexAttribute(displayTexCoordVertexAttribute));
	REQUIRE(displayTexCoordVertexAttribute.SetInitialData(displayTexCoordVertexData));
	REQUIRE(displayVertexBuffer->AllocateMemory());

	// Create the overlay renderable node
	auto overlayRenderableNode = renderer.CreateRenderableNode();
	REQUIRE(overlayRenderableNode->BindVertexAttribute(overlayVertexAttribute, blendPositionAttributeId));
	REQUIRE(overlayRenderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
	overlayRenderableNode->SetStateValue(drawColorStateId, overlayColor);

	// Create the left display renderable node
	auto leftDisplayRenderableNode = renderer.CreateRenderableNode();
	REQUIRE(leftDisplayRenderableNode->BindVertexAttribute(leftDisplayPositionVertexAttribute, displayPositionAttributeId));
	REQUIRE(leftDisplayRenderableNode->BindVertexAttribute(displayTexCoordVertexAttribute, displayTexCoordAttributeId));
	REQUIRE(leftDisplayRenderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));

	// Create the right display renderable node
	auto rightDisplayRenderableNode = renderer.CreateRenderableNode();
	REQUIRE(rightDisplayRenderableNode->BindVertexAttribute(rightDisplayPositionVertexAttribute, displayPositionAttributeId));
	REQUIRE(rightDisplayRenderableNode->BindVertexAttribute(displayTexCoordVertexAttribute, displayTexCoordAttributeId));
	REQUIRE(rightDisplayRenderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));

	// Create the overlay state group node
	auto overlayGroupNode = renderer.CreateStateGroupNode();
	overlayGroupNode->SetBlendEnabled(true);
	overlayGroupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);
	overlayGroupNode->SetBlendMode(IFrameBuffer::AttachmentType::Color, 0, IStateGroupNode::BlendOperation::Add, IStateGroupNode::BlendFactor::One, IStateGroupNode::BlendFactor::Zero, IStateGroupNode::BlendOperation::Add, IStateGroupNode::BlendFactor::One, IStateGroupNode::BlendFactor::Zero);
	overlayGroupNode->SetBlendMode(IFrameBuffer::AttachmentType::Color, 1, IStateGroupNode::BlendOperation::Add, IStateGroupNode::BlendFactor::SourceAlpha, IStateGroupNode::BlendFactor::OneMinusSourceAlpha, IStateGroupNode::BlendOperation::Add, IStateGroupNode::BlendFactor::SourceAlpha, IStateGroupNode::BlendFactor::OneMinusSourceAlpha);
	overlayGroupNode->AddChildNode(overlayRenderableNode.get());

	// Create the display sampler and state group nodes
	auto sampler = renderer.CreateTextureSampler2D();
	sampler->SetTextureFilterMode(ITextureSampler::FilterMode::Nearest, ITextureSampler::FilterMode::Nearest);
	sampler->SetTextureWrapMode(ITextureSampler::WrapMode::ClampToEdge, ITextureSampler::WrapMode::ClampToEdge);

	auto leftDisplayGroupNode = renderer.CreateStateGroupNode();
	leftDisplayGroupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);
	leftDisplayGroupNode->BindTextureWithCombinedSampler(displayTextureId, colorTexture0.get(), sampler.get());
	leftDisplayGroupNode->AddChildNode(leftDisplayRenderableNode.get());

	auto rightDisplayGroupNode = renderer.CreateStateGroupNode();
	rightDisplayGroupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);
	rightDisplayGroupNode->BindTextureWithCombinedSampler(displayTextureId, colorTexture1.get(), sampler.get());
	rightDisplayGroupNode->AddChildNode(rightDisplayRenderableNode.get());

	// Create the program nodes
	auto overlayProgramNode = renderer.CreateProgramNode();
	REQUIRE(overlayProgramNode->BindShaderProgram(blendShaderProgram.get()));
	overlayProgramNode->AddChildNode(overlayGroupNode.get());

	auto leftDisplayProgramNode = renderer.CreateProgramNode();
	REQUIRE(leftDisplayProgramNode->BindShaderProgram(displayShaderProgram.get()));
	leftDisplayProgramNode->AddChildNode(leftDisplayGroupNode.get());

	auto rightDisplayProgramNode = renderer.CreateProgramNode();
	REQUIRE(rightDisplayProgramNode->BindShaderProgram(displayShaderProgram.get()));
	rightDisplayProgramNode->AddChildNode(rightDisplayGroupNode.get());

	// Create our render pass nodes
	auto offscreenRenderPassNode = renderer.CreateRenderPassNode();
	offscreenRenderPassNode->BindFrameBuffer(offscreenFrameBuffer.get());
	offscreenRenderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, backgroundColor);
	offscreenRenderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 1, backgroundColor);
	offscreenRenderPassNode->AddChildNode(overlayProgramNode.get());

	auto windowRenderPassNode = renderer.CreateRenderPassNode();
	windowRenderPassNode->BindFrameBuffer(windowFrameBuffer.get());
	windowRenderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
	windowRenderPassNode->AddChildNode(leftDisplayProgramNode.get());
	windowRenderPassNode->AddChildNode(rightDisplayProgramNode.get());

	// Bind our render tree to the renderer
	IRenderPassNode* passes[2] = {offscreenRenderPassNode.get(), windowRenderPassNode.get()};
	renderer.SetRenderPasses(&passes[0], 2);

	// Attach framebuffer output capture targets for both color attachments and the window result
	auto frameBufferCapture0 = renderer.CreateFrameBufferOutput();
	frameBufferCapture0->SetDetachAfterCapture(true);
	offscreenFrameBuffer->AddOutputCaptureTarget(frameBufferCapture0.get(), IFrameBuffer::AttachmentType::Color, 0);

	auto frameBufferCapture1 = renderer.CreateFrameBufferOutput();
	frameBufferCapture1->SetDetachAfterCapture(true);
	offscreenFrameBuffer->AddOutputCaptureTarget(frameBufferCapture1.get(), IFrameBuffer::AttachmentType::Color, 1);

	auto windowFrameBufferCapture = renderer.CreateFrameBufferOutput();
	windowFrameBufferCapture->SetDetachAfterCapture(true);
	windowFrameBuffer->AddOutputCaptureTarget(windowFrameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);

	// Render the frame
	DrawOneFrame();

	// Store our expected results
	session.AddTestImageResult("Attachment0Overwrite", "A blue background with a centered orange-red quad that appears as a direct overwrite in the first color attachment, proving that this target used One and Zero blending.", std::move(frameBufferCapture0), IImageDiff::Algorithm::NaiveDiff, 0.95);
	session.AddTestImageResult("Attachment1AlphaBlend", "A blue background with a centered orange-red quad that appears partially blended in the second color attachment, proving that this target used SourceAlpha and OneMinusSourceAlpha blending.", std::move(frameBufferCapture1), IImageDiff::Algorithm::NaiveDiff, 0.95);
	session.AddTestImageResult("WindowComparison", "A black window image showing the overwrite result on the left half and the alpha blended result on the right half after sampling both offscreen color attachments back in the same frame.", std::move(windowFrameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.95);

	// Remove all our defined render passes
	renderer.RemoveAllRenderPasses();
	return true;
}

} // namespace cobalt::graphics::testing
