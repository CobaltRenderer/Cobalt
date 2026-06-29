// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../UnitTestBase.h"
#include <array>
namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

// Define our shader programs
const std::string VertexShader = R"(
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
const std::string FragmentShader = R"(
uniform float4 drawColor;

float4 main() : SV_TARGET0
{
    return drawColor;
}
)";

DEFINE_UNIT_TEST_WITH_BASE("StateGroup/BlendState", UnitTestBase)
{
	// Retrieve our important objects
	auto& renderer = session.Renderer();
	auto& uiThread = session.UIThread();
	auto& testWindowInfo = *session.TestWindowPlatformInfo();

	// Define the framebuffer
	auto frameBuffer = renderer.CreateFrameBuffer();
	frameBuffer->DefineViewportRegion(V2UInt32(0, 0), session.TestWindowSize());
	REQUIRE(uiThread.InvokeSync([&] { return frameBuffer->BindWindow(testWindowInfo, IFrameBuffer::WindowDepthStencilMode::None); }));

	// Create and compile our shader program
	auto shaderProgram = renderer.CreateShaderProgram();
	REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(VertexShader)));
	REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(FragmentShader)));
	REQUIRE(shaderProgram->CompileProgram());

	// Retrieve our shader attribute and state IDs
	auto positionAttributeId = shaderProgram->GetVertexAttributeId("position");
	auto drawColorStateId = shaderProgram->GetStateValueId("drawColor");

	// Generate data for the quads we want to render
	std::vector<V4Float32> backgroundVertexData;
	std::vector<V4Float32> overlayVertexData;
	Geometry().CreateFullscreenQuad(0.5f, backgroundVertexData);
	Geometry().CreateCenteredQuad(0.5f, 0.65f, overlayVertexData);

	// Create our vertex buffers and populate them with data
	VertexAttribute<V4Float32> backgroundVertexAttribute(backgroundVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	VertexAttribute<V4Float32> overlayVertexAttribute(overlayVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);

	auto backgroundVertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE(backgroundVertexBuffer->BindVertexAttribute(backgroundVertexAttribute));
	REQUIRE(backgroundVertexAttribute.SetInitialData(backgroundVertexData));
	REQUIRE(backgroundVertexBuffer->AllocateMemory());

	auto overlayVertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE(overlayVertexBuffer->BindVertexAttribute(overlayVertexAttribute));
	REQUIRE(overlayVertexAttribute.SetInitialData(overlayVertexData));
	REQUIRE(overlayVertexBuffer->AllocateMemory());

	// Create the background renderable node
	auto backgroundRenderableNode = renderer.CreateRenderableNode();
	REQUIRE(backgroundRenderableNode->BindVertexAttribute(backgroundVertexAttribute, positionAttributeId));
	REQUIRE(backgroundRenderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));

	// Create the main overlay renderable node
	auto overlayRenderableNode = renderer.CreateRenderableNode();
	REQUIRE(overlayRenderableNode->BindVertexAttribute(overlayVertexAttribute, positionAttributeId));
	REQUIRE(overlayRenderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));

	// Create the alpha seeding renderable node
	auto alphaSeedRenderableNode = renderer.CreateRenderableNode();
	REQUIRE(alphaSeedRenderableNode->BindVertexAttribute(overlayVertexAttribute, positionAttributeId));
	REQUIRE(alphaSeedRenderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));

	// Create the alpha reveal renderable node
	auto alphaRevealRenderableNode = renderer.CreateRenderableNode();
	REQUIRE(alphaRevealRenderableNode->BindVertexAttribute(overlayVertexAttribute, positionAttributeId));
	REQUIRE(alphaRevealRenderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));

	// Create the background state group node
	auto backgroundGroupNode = renderer.CreateStateGroupNode();
	backgroundGroupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);
	backgroundGroupNode->AddChildNode(backgroundRenderableNode.get());

	// Create the main overlay state group node
	auto overlayGroupNode = renderer.CreateStateGroupNode();
	overlayGroupNode->SetBlendEnabled(true);
	overlayGroupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);
	overlayGroupNode->AddChildNode(overlayRenderableNode.get());

	// Create the alpha seeding state group node
	auto alphaSeedGroupNode = renderer.CreateStateGroupNode();
	alphaSeedGroupNode->SetBlendEnabled(true);
	alphaSeedGroupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);
	alphaSeedGroupNode->AddChildNode(alphaSeedRenderableNode.get());

	// Create the alpha reveal state group node
	auto alphaRevealGroupNode = renderer.CreateStateGroupNode();
	alphaRevealGroupNode->SetBlendEnabled(true);
	alphaRevealGroupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);
	alphaRevealGroupNode->AddChildNode(alphaRevealRenderableNode.get());

	// Create the background program node
	auto backgroundProgramNode = renderer.CreateProgramNode();
	REQUIRE(backgroundProgramNode->BindShaderProgram(shaderProgram.get()));
	backgroundProgramNode->AddChildNode(backgroundGroupNode.get());

	// Create the main overlay program node
	auto overlayProgramNode = renderer.CreateProgramNode();
	REQUIRE(overlayProgramNode->BindShaderProgram(shaderProgram.get()));
	overlayProgramNode->AddChildNode(overlayGroupNode.get());

	// Create the alpha seeding program node
	auto alphaSeedProgramNode = renderer.CreateProgramNode();
	REQUIRE(alphaSeedProgramNode->BindShaderProgram(shaderProgram.get()));
	alphaSeedProgramNode->AddChildNode(alphaSeedGroupNode.get());

	// Create the alpha reveal program node
	auto alphaRevealProgramNode = renderer.CreateProgramNode();
	REQUIRE(alphaRevealProgramNode->BindShaderProgram(shaderProgram.get()));
	alphaRevealProgramNode->AddChildNode(alphaRevealGroupNode.get());

	// Create our render pass node
	auto renderPassNode = renderer.CreateRenderPassNode();
	renderPassNode->BindFrameBuffer(frameBuffer.get());
	renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));

	// Bind our render tree to the renderer
	renderer.SetRenderPasses(&renderPassNode, 1);

	// Define colors that exercise overlapping channel contributions instead of simple source or
	// destination replacement
	const V4Float32 factorBackgroundColor(0.30f, 0.65f, 0.20f, 0.25f);
	const V4Float32 factorOverlayColor(0.80f, 0.40f, 0.70f, 0.75f);
	const V4Float32 operationBackgroundColor(0.35f, 0.55f, 0.25f, 0.40f);
	const V4Float32 operationOverlayColor(0.45f, 0.20f, 0.60f, 0.70f);
	const V4Float32 alphaRevealBackgroundColor(0.15f, 0.25f, 0.55f, 0.10f);
	const V4Float32 alphaRevealOverlayColor(0.95f, 0.35f, 0.10f, 0.60f);

	// Capture a baseline with blending disabled
	renderPassNode->RemoveAllChildNodes();
	renderPassNode->AddChildNode(backgroundProgramNode.get());
	renderPassNode->AddChildNode(overlayProgramNode.get());
	backgroundRenderableNode->SetStateValue(drawColorStateId, factorBackgroundColor);
	overlayRenderableNode->SetStateValue(drawColorStateId, factorOverlayColor);
	overlayGroupNode->SetBlendEnabled(false);
	auto frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("BlendDisabled", "A green-blue background with a centered pink-purple quad rendered with blending disabled, so the centered quad appears as a direct overwrite.", std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.95);
	overlayGroupNode->SetBlendEnabled(true);

	// Capture cases that cover the available blend factors with overlapping source and destination
	// channels
	struct BlendFactorCase
	{
		const char* name;
		const char* description;
		V4Float32 backgroundColor;
		V4Float32 overlayColor;
		IStateGroupNode::BlendFactor sourceFactor;
		IStateGroupNode::BlendFactor destinationFactor;
	};
	const std::array<BlendFactorCase, 8> blendFactorCases = {{
	  {"FactorsZeroOne", "A green-blue background with no visible color change inside the centered quad, proving that Zero and One preserve the destination color.", factorBackgroundColor, factorOverlayColor, IStateGroupNode::BlendFactor::Zero, IStateGroupNode::BlendFactor::One},
	  {"FactorsOneZero", "A green-blue background with the centered quad replaced by the overlay color, proving that One and Zero use only the source color.", factorBackgroundColor, factorOverlayColor, IStateGroupNode::BlendFactor::One, IStateGroupNode::BlendFactor::Zero},
	  {"FactorsSourceColorOneMinusSourceColor", "A green-blue background with a centered quad whose red, green and blue channels are each mixed by the overlay color values.", factorBackgroundColor, factorOverlayColor, IStateGroupNode::BlendFactor::SourceColor, IStateGroupNode::BlendFactor::OneMinusSourceColor},
	  {"FactorsDestinationColorOneMinusDestinationColor", "A green-blue background with a centered quad whose channels are mixed using the destination color values already in the framebuffer.", factorBackgroundColor, factorOverlayColor, IStateGroupNode::BlendFactor::DestinationColor, IStateGroupNode::BlendFactor::OneMinusDestinationColor},
	  {"FactorsSourceAlphaOneMinusSourceAlpha", "A green-blue background with a centered quad blended by the overlay alpha, showing a strong but not complete source contribution.", factorBackgroundColor, factorOverlayColor, IStateGroupNode::BlendFactor::SourceAlpha, IStateGroupNode::BlendFactor::OneMinusSourceAlpha},
	  {"FactorsSourceAlphaTransparent", "A green-blue background with no visible change inside the centered quad, proving that SourceAlpha and OneMinusSourceAlpha respect a zero-alpha source.", factorBackgroundColor, V4Float32(factorOverlayColor.X(), factorOverlayColor.Y(), factorOverlayColor.Z(), 0.0f), IStateGroupNode::BlendFactor::SourceAlpha, IStateGroupNode::BlendFactor::OneMinusSourceAlpha},
	  {"FactorsSourceAlphaOpaque", "A green-blue background with the centered quad fully taking the overlay color, proving that SourceAlpha and OneMinusSourceAlpha respect a fully opaque source.", factorBackgroundColor, V4Float32(factorOverlayColor.X(), factorOverlayColor.Y(), factorOverlayColor.Z(), 1.0f), IStateGroupNode::BlendFactor::SourceAlpha, IStateGroupNode::BlendFactor::OneMinusSourceAlpha},
	  {"FactorsDestinationAlphaOneMinusDestinationAlpha", "A green-blue background with a centered quad only weakly influenced by the overlay, proving that DestinationAlpha and OneMinusDestinationAlpha use the destination alpha already stored in the framebuffer.", factorBackgroundColor, factorOverlayColor, IStateGroupNode::BlendFactor::DestinationAlpha, IStateGroupNode::BlendFactor::OneMinusDestinationAlpha},
	}};
	for (const auto& blendFactorCase : blendFactorCases)
	{
		backgroundRenderableNode->SetStateValue(drawColorStateId, blendFactorCase.backgroundColor);
		overlayRenderableNode->SetStateValue(drawColorStateId, blendFactorCase.overlayColor);
		overlayGroupNode->SetBlendMode(IStateGroupNode::BlendOperation::Add, blendFactorCase.sourceFactor, blendFactorCase.destinationFactor, IStateGroupNode::BlendOperation::Add, blendFactorCase.sourceFactor, blendFactorCase.destinationFactor);
		frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult(blendFactorCase.name, blendFactorCase.description, std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.95);
	}

	// Capture cases that cover the available RGB blend operations
	struct BlendOperationCase
	{
		const char* name;
		const char* description;
		IStateGroupNode::BlendOperation operation;
		IStateGroupNode::BlendFactor sourceFactor;
		IStateGroupNode::BlendFactor destinationFactor;
	};
	const std::array<BlendOperationCase, 5> blendOperationCases = {{
	  {"OperationAdd", "A green-blue background with a centered quad whose channels become brighter because source and destination are added together.", IStateGroupNode::BlendOperation::Add, IStateGroupNode::BlendFactor::One, IStateGroupNode::BlendFactor::One},
	  {"OperationSubtract", "A green-blue background with a centered quad showing only the channels where the overlay is greater than the background, because the operation subtracts destination from source.", IStateGroupNode::BlendOperation::Subtract, IStateGroupNode::BlendFactor::One, IStateGroupNode::BlendFactor::One},
	  {"OperationReverseSubtract", "A green-blue background with a centered quad showing only the channels where the background is greater than the overlay, because the operation subtracts source from destination.", IStateGroupNode::BlendOperation::ReverseSubtract, IStateGroupNode::BlendFactor::One, IStateGroupNode::BlendFactor::One},
	  {"OperationMin", "A green-blue background with a centered quad taking the darker value from each source channel, proving that the Min operation ignores the chosen blend factors.", IStateGroupNode::BlendOperation::Min, IStateGroupNode::BlendFactor::Zero, IStateGroupNode::BlendFactor::OneMinusSourceAlpha},
	  {"OperationMax", "A green-blue background with a centered quad taking the brighter value from each source channel, proving that the Max operation ignores the chosen blend factors.", IStateGroupNode::BlendOperation::Max, IStateGroupNode::BlendFactor::Zero, IStateGroupNode::BlendFactor::OneMinusSourceAlpha},
	}};
	backgroundRenderableNode->SetStateValue(drawColorStateId, operationBackgroundColor);
	overlayRenderableNode->SetStateValue(drawColorStateId, operationOverlayColor);
	for (const auto& blendOperationCase : blendOperationCases)
	{
		overlayGroupNode->SetBlendMode(blendOperationCase.operation, blendOperationCase.sourceFactor, blendOperationCase.destinationFactor, blendOperationCase.operation, blendOperationCase.sourceFactor, blendOperationCase.destinationFactor);
		frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult(blendOperationCase.name, blendOperationCase.description, std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.95);
	}

	// Capture cases that make the separate alpha blend equation visible through a destination alpha
	// controlled reveal pass
	struct AlphaBlendCase
	{
		const char* name;
		const char* description;
		float backgroundAlpha;
		float seedAlpha;
		IStateGroupNode::BlendOperation alphaOperation;
		IStateGroupNode::BlendFactor sourceAlphaFactor;
		IStateGroupNode::BlendFactor destinationAlphaFactor;
	};
	const std::array<AlphaBlendCase, 5> alphaBlendCases = {{
	  {"AlphaOperationAdd", "A blue background with a centered orange quad that appears strongly mixed because the alpha seed pass adds source alpha to destination alpha before the reveal pass reads it back.", 0.25f, 0.35f, IStateGroupNode::BlendOperation::Add, IStateGroupNode::BlendFactor::One, IStateGroupNode::BlendFactor::One},
	  {"AlphaOperationSubtract", "A blue background with a centered orange quad that appears moderately mixed because the alpha seed pass subtracts destination alpha from source alpha before the reveal pass reads it back.", 0.20f, 0.75f, IStateGroupNode::BlendOperation::Subtract, IStateGroupNode::BlendFactor::One, IStateGroupNode::BlendFactor::One},
	  {"AlphaOperationReverseSubtract", "A blue background with a centered orange quad that appears moderately mixed because the alpha seed pass subtracts source alpha from destination alpha before the reveal pass reads it back.", 0.80f, 0.30f, IStateGroupNode::BlendOperation::ReverseSubtract, IStateGroupNode::BlendFactor::One, IStateGroupNode::BlendFactor::One},
	  {"AlphaOperationMin", "A blue background with a dimmer centered orange quad because the alpha seed pass keeps the smaller of the background and source alpha values before the reveal pass reads it back.", 0.30f, 0.70f, IStateGroupNode::BlendOperation::Min, IStateGroupNode::BlendFactor::Zero, IStateGroupNode::BlendFactor::OneMinusSourceAlpha},
	  {"AlphaOperationMax", "A blue background with a brighter centered orange quad because the alpha seed pass keeps the larger of the background and source alpha values before the reveal pass reads it back.", 0.30f, 0.70f, IStateGroupNode::BlendOperation::Max, IStateGroupNode::BlendFactor::Zero, IStateGroupNode::BlendFactor::OneMinusSourceAlpha},
	}};
	for (const auto& alphaBlendCase : alphaBlendCases)
	{
		// Draw the background, seed only the render target alpha in the centred region, then reveal
		// that stored alpha through DestinationAlpha blending
		renderPassNode->RemoveAllChildNodes();
		renderPassNode->AddChildNode(backgroundProgramNode.get());
		renderPassNode->AddChildNode(alphaSeedProgramNode.get());
		renderPassNode->AddChildNode(alphaRevealProgramNode.get());

		backgroundRenderableNode->SetStateValue(drawColorStateId, V4Float32(alphaRevealBackgroundColor.X(), alphaRevealBackgroundColor.Y(), alphaRevealBackgroundColor.Z(), alphaBlendCase.backgroundAlpha));
		alphaSeedRenderableNode->SetStateValue(drawColorStateId, V4Float32(0.0f, 0.0f, 0.0f, alphaBlendCase.seedAlpha));
		alphaRevealRenderableNode->SetStateValue(drawColorStateId, alphaRevealOverlayColor);

		alphaSeedGroupNode->SetBlendMode(IStateGroupNode::BlendOperation::Add, IStateGroupNode::BlendFactor::Zero, IStateGroupNode::BlendFactor::One, alphaBlendCase.alphaOperation, alphaBlendCase.sourceAlphaFactor, alphaBlendCase.destinationAlphaFactor);
		alphaRevealGroupNode->SetBlendMode(IStateGroupNode::BlendOperation::Add, IStateGroupNode::BlendFactor::DestinationAlpha, IStateGroupNode::BlendFactor::OneMinusDestinationAlpha, IStateGroupNode::BlendOperation::Add, IStateGroupNode::BlendFactor::Zero, IStateGroupNode::BlendFactor::One);

		frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult(alphaBlendCase.name, alphaBlendCase.description, std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.95);
	}

	// Remove all our defined render passes
	renderer.RemoveAllRenderPasses();
	return true;
}

} // namespace cobalt::graphics::testing
