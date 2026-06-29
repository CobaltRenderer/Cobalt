// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../UnitTestBase.h"
#include <array>
namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

namespace {

const char* SampleCountName(ITextureBuffer::SampleCount sampleCount)
{
	switch (sampleCount)
	{
	case ITextureBuffer::SampleCount::SampleCount1:
		return "1x";
	case ITextureBuffer::SampleCount::SampleCount2:
		return "2x";
	case ITextureBuffer::SampleCount::SampleCount4:
		return "4x";
	case ITextureBuffer::SampleCount::SampleCount8:
		return "8x";
	case ITextureBuffer::SampleCount::SampleCount16:
		return "16x";
	case ITextureBuffer::SampleCount::SampleCount32:
		return "32x";
	default:
		return "Unknown";
	}
}

} // namespace

// Define our line rendering shader programs
const std::string LineVertexShader = R"(
struct VSInput {
    float4 position : position;
    float3 color : color;
};

struct VSOutput {
    float4 position : SV_POSITION;
    float3 color : COLOR;
};

VSOutput main(VSInput IN)
{
    VSOutput OUT;
    OUT.position = IN.position;
    OUT.color = IN.color;
    return OUT;
}
)";
const std::string LineFragmentShader = R"(
struct VSOutput {
    float4 position : SV_POSITION;
    float3 color : COLOR;
};

float4 main(VSOutput IN) : SV_TARGET0
{
    return float4(IN.color, 1.0f);
}
)";

// Define our fullscreen display shader programs
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

DEFINE_UNIT_TEST_WITH_BASE("Framebuffer/MultiSampling", UnitTestBase)
{
	// Retrieve our important objects
	auto& renderer = session.Renderer();
	auto& uiThread = session.UIThread();
	auto& testWindowInfo = *session.TestWindowPlatformInfo();

	// Create and compile our line rendering shader program
	auto lineShaderProgram = renderer.CreateShaderProgram();
	REQUIRE(lineShaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(LineVertexShader)));
	REQUIRE(lineShaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(LineFragmentShader)));
	REQUIRE(lineShaderProgram->CompileProgram());

	// Create and compile our fullscreen display shader program
	auto displayShaderProgram = renderer.CreateShaderProgram();
	REQUIRE(displayShaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(DisplayVertexShader)));
	REQUIRE(displayShaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(DisplayFragmentShader)));
	REQUIRE(displayShaderProgram->CompileProgram());

	// Retrieve our shader attribute IDs
	auto linePositionAttributeId = lineShaderProgram->GetVertexAttributeId("position");
	auto lineColorAttributeId = lineShaderProgram->GetVertexAttributeId("color");
	auto displayPositionAttributeId = displayShaderProgram->GetVertexAttributeId("position");
	auto displayTexCoordAttributeId = displayShaderProgram->GetVertexAttributeId("texCoord");
	auto displayTextureId = displayShaderProgram->GetTextureId("colorTexture");

	// Generate a dense set of lines to show the effect of multi sampling on horizontal, vertical,
	// and angled edges.
	std::vector<V4Float32> linePositionVertexData = {
	  {-0.90f, -0.95f, 0.5f, 1.0f},
	  {-0.90f, 0.95f, 0.5f, 1.0f},
	  {-0.60f, -0.95f, 0.5f, 1.0f},
	  {-0.60f, 0.95f, 0.5f, 1.0f},
	  {-0.30f, -0.95f, 0.5f, 1.0f},
	  {-0.30f, 0.95f, 0.5f, 1.0f},
	  {0.00f, -0.95f, 0.5f, 1.0f},
	  {0.00f, 0.95f, 0.5f, 1.0f},
	  {0.30f, -0.95f, 0.5f, 1.0f},
	  {0.30f, 0.95f, 0.5f, 1.0f},
	  {0.60f, -0.95f, 0.5f, 1.0f},
	  {0.60f, 0.95f, 0.5f, 1.0f},
	  {-0.95f, -0.80f, 0.5f, 1.0f},
	  {0.95f, -0.80f, 0.5f, 1.0f},
	  {-0.95f, -0.45f, 0.5f, 1.0f},
	  {0.95f, -0.45f, 0.5f, 1.0f},
	  {-0.95f, -0.10f, 0.5f, 1.0f},
	  {0.95f, -0.10f, 0.5f, 1.0f},
	  {-0.95f, 0.25f, 0.5f, 1.0f},
	  {0.95f, 0.25f, 0.5f, 1.0f},
	  {-0.95f, 0.60f, 0.5f, 1.0f},
	  {0.95f, 0.60f, 0.5f, 1.0f},
	  {-0.95f, 0.95f, 0.5f, 1.0f},
	  {0.95f, -0.95f, 0.5f, 1.0f},
	  {-0.95f, 0.70f, 0.5f, 1.0f},
	  {0.70f, -0.95f, 0.5f, 1.0f},
	  {-0.95f, 0.35f, 0.5f, 1.0f},
	  {0.35f, -0.95f, 0.5f, 1.0f},
	  {-0.95f, 0.00f, 0.5f, 1.0f},
	  {0.00f, -0.95f, 0.5f, 1.0f},
	  {-0.95f, -0.35f, 0.5f, 1.0f},
	  {-0.35f, -0.95f, 0.5f, 1.0f},
	  {-0.95f, -0.70f, 0.5f, 1.0f},
	  {-0.70f, -0.95f, 0.5f, 1.0f},
	  {-0.95f, -0.95f, 0.5f, 1.0f},
	  {0.95f, 0.95f, 0.5f, 1.0f},
	  {-0.70f, -0.95f, 0.5f, 1.0f},
	  {0.95f, 0.70f, 0.5f, 1.0f},
	  {-0.35f, -0.95f, 0.5f, 1.0f},
	  {0.95f, 0.35f, 0.5f, 1.0f},
	  {0.00f, -0.95f, 0.5f, 1.0f},
	  {0.95f, 0.00f, 0.5f, 1.0f},
	  {0.35f, -0.95f, 0.5f, 1.0f},
	  {0.95f, -0.35f, 0.5f, 1.0f},
	  {0.70f, -0.95f, 0.5f, 1.0f},
	  {0.95f, -0.70f, 0.5f, 1.0f},
	  {-0.95f, -0.60f, 0.5f, 1.0f},
	  {0.80f, 0.95f, 0.5f, 1.0f},
	  {-0.80f, -0.95f, 0.5f, 1.0f},
	  {0.95f, 0.50f, 0.5f, 1.0f},
	  {-0.95f, 0.50f, 0.5f, 1.0f},
	  {0.80f, -0.95f, 0.5f, 1.0f},
	  {-0.80f, 0.95f, 0.5f, 1.0f},
	  {0.95f, -0.50f, 0.5f, 1.0f},
	  {-0.20f, -0.95f, 0.5f, 1.0f},
	  {0.95f, 0.85f, 0.5f, 1.0f},
	  {-0.95f, 0.85f, 0.5f, 1.0f},
	  {0.20f, -0.95f, 0.5f, 1.0f},
	  {-0.95f, -0.20f, 0.5f, 1.0f},
	  {0.50f, 0.95f, 0.5f, 1.0f},
	  {-0.50f, -0.95f, 0.5f, 1.0f},
	  {0.95f, 0.20f, 0.5f, 1.0f},
	  {-0.95f, 0.15f, 0.5f, 1.0f},
	  {0.60f, -0.95f, 0.5f, 1.0f},
	  {-0.60f, 0.95f, 0.5f, 1.0f},
	  {0.95f, -0.15f, 0.5f, 1.0f},
	};
	std::vector<V3Float32> lineColorVertexData;
	lineColorVertexData.reserve(linePositionVertexData.size());
	for (size_t lineNo = 0; lineNo < linePositionVertexData.size() / 2; ++lineNo)
	{
		V3Float32 lineColor;
		switch (lineNo % 4)
		{
		case 0:
			lineColor = V3Float32(1.0f, 1.0f, 1.0f);
			break;
		case 1:
			lineColor = V3Float32(1.0f, 0.0f, 0.0f);
			break;
		case 2:
			lineColor = V3Float32(0.0f, 1.0f, 0.0f);
			break;
		default:
			lineColor = V3Float32(0.0f, 0.0f, 1.0f);
			break;
		}
		lineColorVertexData.push_back(lineColor);
		lineColorVertexData.push_back(lineColor);
	}

	// Generate data for the fullscreen quad used to display resolve textures in the window. Note
	// that we need to flip the texture coordinates under OpenGL to account for the reversed
	// coordinate system.
	std::vector<V3Float32> quadPositionVertexData = {{-1.0f, -1.0f, 0.0f}, {-1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {-1.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {1.0f, -1.0f, 0.0f}};
	std::vector<V2Float32> quadTexCoordVertexData;
	if (session.ApiFamily() == IRendererPlugin::ApiFamily::OpenGL)
	{
		quadTexCoordVertexData = {{0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 1.0f}, {1.0f, 0.0f}};
	}
	else
	{
		quadTexCoordVertexData = {{0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}};
	}

	// Create our vertex buffers and populate them with data
	VertexAttribute<V4Float32> lineVertexAttributePosition(linePositionVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	VertexAttribute<V3Float32> lineVertexAttributeColor(lineColorVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	VertexAttribute<V3Float32> quadVertexAttributePosition(quadPositionVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	VertexAttribute<V2Float32> quadVertexAttributeTexCoord(quadTexCoordVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);

	auto vertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE(vertexBuffer->BindVertexAttribute(lineVertexAttributePosition));
	REQUIRE(lineVertexAttributePosition.SetInitialData(linePositionVertexData));
	REQUIRE(vertexBuffer->BindVertexAttribute(lineVertexAttributeColor));
	REQUIRE(lineVertexAttributeColor.SetInitialData(lineColorVertexData));
	REQUIRE(vertexBuffer->BindVertexAttribute(quadVertexAttributePosition));
	REQUIRE(quadVertexAttributePosition.SetInitialData(quadPositionVertexData));
	REQUIRE(vertexBuffer->BindVertexAttribute(quadVertexAttributeTexCoord));
	REQUIRE(quadVertexAttributeTexCoord.SetInitialData(quadTexCoordVertexData));
	REQUIRE(vertexBuffer->AllocateMemory());

	// Create the renderable node that draws the multi sample test pattern
	auto lineRenderableNode = renderer.CreateRenderableNode();
	REQUIRE(lineRenderableNode->BindVertexAttribute(lineVertexAttributePosition, linePositionAttributeId));
	REQUIRE(lineRenderableNode->BindVertexAttribute(lineVertexAttributeColor, lineColorAttributeId));
	REQUIRE(lineRenderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Lines));

	// Create the renderable node that displays resolve textures in the window
	auto displayRenderableNode = renderer.CreateRenderableNode();
	REQUIRE(displayRenderableNode->BindVertexAttribute(quadVertexAttributePosition, displayPositionAttributeId));
	REQUIRE(displayRenderableNode->BindVertexAttribute(quadVertexAttributeTexCoord, displayTexCoordAttributeId));
	REQUIRE(displayRenderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));

	// Create our state group nodes
	auto lineGroupNode = renderer.CreateStateGroupNode();
	lineGroupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);
	lineGroupNode->AddChildNode(lineRenderableNode.get());

	auto displayGroupNode = renderer.CreateStateGroupNode();
	displayGroupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);
	displayGroupNode->AddChildNode(displayRenderableNode.get());

	// Create our program nodes
	auto lineProgramNode = renderer.CreateProgramNode();
	REQUIRE(lineProgramNode->BindShaderProgram(lineShaderProgram.get()));
	lineProgramNode->AddChildNode(lineGroupNode.get());

	auto displayProgramNode = renderer.CreateProgramNode();
	REQUIRE(displayProgramNode->BindShaderProgram(displayShaderProgram.get()));
	displayProgramNode->AddChildNode(displayGroupNode.get());

	// Create the sampler used to read resolve textures back in the same frame
	auto sampler = renderer.CreateTextureSampler2D();
	sampler->SetTextureFilterMode(ITextureSampler::FilterMode::Nearest, ITextureSampler::FilterMode::Nearest);
	sampler->SetTextureWrapMode(ITextureSampler::WrapMode::ClampToEdge, ITextureSampler::WrapMode::ClampToEdge);

	// Capture the line pattern on a window bound framebuffer first
	auto windowFrameBuffer = renderer.CreateFrameBuffer();
	windowFrameBuffer->DefineViewportRegion(V2UInt32(0, 0), session.TestWindowSize());
	REQUIRE(uiThread.InvokeSync([&] { return windowFrameBuffer->BindWindow(testWindowInfo, IFrameBuffer::WindowDepthStencilMode::None); }));

	auto windowPass = renderer.CreateRenderPassNode();
	windowPass->BindFrameBuffer(windowFrameBuffer.get());
	windowPass->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
	windowPass->AddChildNode(lineProgramNode.get());
	renderer.SetRenderPasses(&windowPass, 1);

	auto frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	windowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("WindowRender", "White, red, green and blue lines at horizontal, vertical and many diagonal angles on a black window background.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Define the offscreen size to match the current test window
	V2UInt32 offscreenSize = session.TestWindowSize();

	// Run the offscreen multi sample render, resolve, and same frame window display for every
	// sample count.
	const std::array<ITextureBuffer::SampleCount, 6> sampleCounts = {{
	  ITextureBuffer::SampleCount::SampleCount1,
	  ITextureBuffer::SampleCount::SampleCount2,
	  ITextureBuffer::SampleCount::SampleCount4,
	  ITextureBuffer::SampleCount::SampleCount8,
	  ITextureBuffer::SampleCount::SampleCount16,
	  ITextureBuffer::SampleCount::SampleCount32,
	}};
	for (auto sampleCount : sampleCounts)
	{
		auto multiSampleColorTexture = renderer.CreateTextureBuffer2D();
		if (!multiSampleColorTexture->IsSampleCountSupported(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8, sampleCount))
		{
			session.AddTestSkipped(std::string("Skipped ") + SampleCountName(sampleCount) + " multi-sampling", "This test was skipped, as the current device does not support a " + std::string(SampleCountName(sampleCount)) + " multi-sampled color attachment.");
			continue;
		}
		multiSampleColorTexture->SetUsageFlags(ITextureBuffer::UsageFlags::FrameBufferOutput);
		multiSampleColorTexture->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
		multiSampleColorTexture->SetTextureDimensions(offscreenSize);
		multiSampleColorTexture->SetSampleCount(sampleCount);
		REQUIRE(multiSampleColorTexture->AllocateMemory());

		auto resolveTexture = renderer.CreateTextureBuffer2D();
		resolveTexture->SetUsageFlags(ITextureBuffer::UsageFlags::ShaderInput | ITextureBuffer::UsageFlags::FrameBufferOutput | ITextureBuffer::UsageFlags::MultiSampleResolve);
		resolveTexture->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
		resolveTexture->SetTextureDimensions(offscreenSize);
		REQUIRE(resolveTexture->AllocateMemory());

		auto multiSampleFrameBuffer = renderer.CreateFrameBuffer();
		multiSampleFrameBuffer->DefineViewportRegion(V2UInt32(0, 0), offscreenSize);
		REQUIRE(multiSampleFrameBuffer->BindTexture(multiSampleColorTexture.get(), IFrameBuffer::AttachmentType::Color));
		REQUIRE(multiSampleFrameBuffer->BindMultiSamplingResolveTexture(resolveTexture.get(), IFrameBuffer::AttachmentType::Color));

		auto resolvedFrameBuffer = renderer.CreateFrameBuffer();
		resolvedFrameBuffer->DefineViewportRegion(V2UInt32(0, 0), offscreenSize);
		REQUIRE(resolvedFrameBuffer->BindTexture(resolveTexture.get(), IFrameBuffer::AttachmentType::Color));

		// Create the render pass that draws into the multi sampled target and resolves it
		auto multiSamplePass = renderer.CreateRenderPassNode();
		multiSamplePass->BindFrameBuffer(multiSampleFrameBuffer.get());
		multiSamplePass->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
		multiSamplePass->EnableAttachmentMultiSamplingResolution(IFrameBuffer::AttachmentType::Color, 0);
		multiSamplePass->AddChildNode(lineProgramNode.get());

		// Create the render pass that exposes the resolved texture as an output image
		auto resolvedPass = renderer.CreateRenderPassNode();
		resolvedPass->BindFrameBuffer(resolvedFrameBuffer.get());
		resolvedPass->SetAttachmentLoadStoreBehavior(IFrameBuffer::AttachmentType::Color, 0, IRenderPassNode::AttachmentLoadBehavior::LoadExistingData, IRenderPassNode::AttachmentStoreBehavior::StoreFinalData);

		// Create the render pass that displays the resolved texture in the window in the same frame
		displayGroupNode->BindTextureWithCombinedSampler(displayTextureId, resolveTexture.get(), sampler.get());
		auto displayPass = renderer.CreateRenderPassNode();
		displayPass->BindFrameBuffer(windowFrameBuffer.get());
		displayPass->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
		displayPass->AddChildNode(displayProgramNode.get());

		// Bind the render passes for this sample count
		IRenderPassNode* renderPasses[3] = {multiSamplePass.get(), resolvedPass.get(), displayPass.get()};
		renderer.SetRenderPasses(&renderPasses[0], 3);

		// Capture the offscreen resolved image
		auto offscreenCapture = renderer.CreateFrameBufferOutput();
		offscreenCapture->SetDetachAfterCapture(true);
		resolvedFrameBuffer->AddOutputCaptureTarget(offscreenCapture.get(), IFrameBuffer::AttachmentType::Color);

		// Capture the same resolved image after it has been sampled onto a fullscreen quad in the
		// window
		auto windowCapture = renderer.CreateFrameBufferOutput();
		windowCapture->SetDetachAfterCapture(true);
		windowFrameBuffer->AddOutputCaptureTarget(windowCapture.get(), IFrameBuffer::AttachmentType::Color);

		DrawOneFrame();

		session.AddTestImageResult(std::string("OffscreenMultiSample") + SampleCountName(sampleCount), "White, red, green and blue lines at horizontal, vertical and many diagonal angles on a black background, resolved from an offscreen " + std::string(SampleCountName(sampleCount)) + " multi-sampled render target.", std::move(offscreenCapture), IImageDiff::Algorithm::NaiveDiff, 0.98);
		session.AddTestImageResult(std::string("WindowMultiSample") + SampleCountName(sampleCount), "The same resolved " + std::string(SampleCountName(sampleCount)) + " multi-sampled line image displayed on a fullscreen quad in the window during the same frame.", std::move(windowCapture), IImageDiff::Algorithm::NaiveDiff, 0.98);

		multiSamplePass->RemoveAllChildNodes();
	}

	// Remove all our defined render passes
	renderer.RemoveAllRenderPasses();
	return true;
}

} // namespace cobalt::graphics::testing
