// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../UnitTestBase.h"
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

DEFINE_UNIT_TEST_WITH_BASE("Framebuffer/FramebufferClearColor", UnitTestBase)
{
	// Retrieve our important objects
	auto& renderer = session.Renderer();
	auto& uiThread = session.UIThread();
	auto& testWindowInfo = *session.TestWindowPlatformInfo();

	// Create and compile our shader program
	auto shaderProgram = renderer.CreateShaderProgram();
	REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(VertexShader)));
	REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(FragmentShader)));
	REQUIRE(shaderProgram->CompileProgram());

	// Retrieve our shader attribute and state IDs
	auto positionAttributeId = shaderProgram->GetVertexAttributeId("position");
	auto drawColorStateId = shaderProgram->GetStateValueId("drawColor");

	// Create the triangle geometry used throughout the test
	std::vector<V4Float32> upperLeftVertexData;
	std::vector<V4Float32> upperRightVertexData;
	std::vector<V4Float32> lowerLeftVertexData;
	Geometry().CreateUpperLeftTriangle(0.5f, upperLeftVertexData);
	Geometry().CreateUpperRightTriangle(0.5f, upperRightVertexData);
	Geometry().CreateLowerLeftTriangle(0.5f, lowerLeftVertexData);

	VertexAttribute<V4Float32> upperLeftVertexAttribute(upperLeftVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	VertexAttribute<V4Float32> upperRightVertexAttribute(upperRightVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	VertexAttribute<V4Float32> lowerLeftVertexAttribute(lowerLeftVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);

	auto upperLeftVertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE(upperLeftVertexBuffer->BindVertexAttribute(upperLeftVertexAttribute));
	REQUIRE(upperLeftVertexAttribute.SetInitialData(upperLeftVertexData));
	REQUIRE(upperLeftVertexBuffer->AllocateMemory());

	auto upperRightVertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE(upperRightVertexBuffer->BindVertexAttribute(upperRightVertexAttribute));
	REQUIRE(upperRightVertexAttribute.SetInitialData(upperRightVertexData));
	REQUIRE(upperRightVertexBuffer->AllocateMemory());

	auto lowerLeftVertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE(lowerLeftVertexBuffer->BindVertexAttribute(lowerLeftVertexAttribute));
	REQUIRE(lowerLeftVertexAttribute.SetInitialData(lowerLeftVertexData));
	REQUIRE(lowerLeftVertexBuffer->AllocateMemory());

	// Create the program node that draws the white upper left triangle
	auto upperLeftRenderableNode = renderer.CreateRenderableNode();
	REQUIRE(upperLeftRenderableNode->BindVertexAttribute(upperLeftVertexAttribute, positionAttributeId));
	REQUIRE(upperLeftRenderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
	upperLeftRenderableNode->SetStateValue(drawColorStateId, V4Float32(1.0f, 1.0f, 1.0f, 1.0f));

	auto upperLeftGroupNode = renderer.CreateStateGroupNode();
	upperLeftGroupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);
	upperLeftGroupNode->AddChildNode(upperLeftRenderableNode.get());

	auto upperLeftProgramNode = renderer.CreateProgramNode();
	REQUIRE(upperLeftProgramNode->BindShaderProgram(shaderProgram.get()));
	upperLeftProgramNode->AddChildNode(upperLeftGroupNode.get());

	// Create the program node that draws the red upper right triangle
	auto upperRightRenderableNode = renderer.CreateRenderableNode();
	REQUIRE(upperRightRenderableNode->BindVertexAttribute(upperRightVertexAttribute, positionAttributeId));
	REQUIRE(upperRightRenderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
	upperRightRenderableNode->SetStateValue(drawColorStateId, V4Float32(1.0f, 0.0f, 0.0f, 1.0f));

	auto upperRightGroupNode = renderer.CreateStateGroupNode();
	upperRightGroupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);
	upperRightGroupNode->AddChildNode(upperRightRenderableNode.get());

	auto upperRightProgramNode = renderer.CreateProgramNode();
	REQUIRE(upperRightProgramNode->BindShaderProgram(shaderProgram.get()));
	upperRightProgramNode->AddChildNode(upperRightGroupNode.get());

	// Create the program node that draws the yellow lower left triangle
	auto lowerLeftRenderableNode = renderer.CreateRenderableNode();
	REQUIRE(lowerLeftRenderableNode->BindVertexAttribute(lowerLeftVertexAttribute, positionAttributeId));
	REQUIRE(lowerLeftRenderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
	lowerLeftRenderableNode->SetStateValue(drawColorStateId, V4Float32(1.0f, 1.0f, 0.0f, 1.0f));

	auto lowerLeftGroupNode = renderer.CreateStateGroupNode();
	lowerLeftGroupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);
	lowerLeftGroupNode->AddChildNode(lowerLeftRenderableNode.get());

	auto lowerLeftProgramNode = renderer.CreateProgramNode();
	REQUIRE(lowerLeftProgramNode->BindShaderProgram(shaderProgram.get()));
	lowerLeftProgramNode->AddChildNode(lowerLeftGroupNode.get());

	// Run the test sequence against a window bound framebuffer first
	auto mainWindowFrameBuffer = renderer.CreateFrameBuffer();
	mainWindowFrameBuffer->DefineViewportRegion(V2UInt32(0, 0), session.TestWindowSize());
	REQUIRE(uiThread.InvokeSync([&] { return mainWindowFrameBuffer->BindWindow(testWindowInfo, IFrameBuffer::WindowDepthStencilMode::None); }));
	auto renderPassNode = renderer.CreateRenderPassNode();
	renderPassNode->BindFrameBuffer(mainWindowFrameBuffer.get());
	renderer.SetRenderPasses(&renderPassNode, 1);

	// Clear the window bound color attachment with no renderable content
	renderPassNode->RemoveAllChildNodes();
	renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
	auto frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("WindowClearOnly", "A fully black image after clearing the window-bound color attachment with no renderable content.", std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.95);

	// Clear the window bound color attachment to a non-black color with distinct channel value
	renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.2f, 0.4f, 0.8f, 1.0f));
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("WindowClearOnlyNonBlack", "A fully blue-green image after clearing the window-bound color attachment to a non-black color with different red, green, and blue values.", std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.95);

	// Clear the window bound color attachment and draw into the upper left quadrant
	renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
	renderPassNode->AddChildNode(upperLeftProgramNode.get());
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("WindowClearAndUpperLeft", "A black image with a white triangle in the upper-left quadrant after clearing the window-bound color attachment and drawing one triangle.", std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.95);

	// Clear the window bound color attachment to a non-black color and draw into the upper left quadrant
	renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.7f, 0.15f, 0.35f, 1.0f));
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("WindowClearNonBlackAndUpperLeft", "A rose colored image with a white triangle in the upper-left quadrant after clearing the window-bound color attachment to a non-black color and drawing one triangle.", std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.95);

	// Clear the window bound color attachment again and draw into the lower left quadrant
	renderPassNode->RemoveAllChildNodes();
	renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
	renderPassNode->AddChildNode(lowerLeftProgramNode.get());
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("WindowClearAndLowerLeft", "A black image with a yellow triangle in the lower-left quadrant after clearing the window-bound color attachment again and drawing a new triangle.", std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.95);

	// Repeat the test sequence using a texture bound framebuffer
	V2UInt32 offscreenSize = session.TestWindowSize();
	auto colorTexture = renderer.CreateTextureBuffer2D();
	colorTexture->SetUsageFlags(ITextureBuffer::UsageFlags::FrameBufferOutput);
	colorTexture->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
	colorTexture->SetTextureDimensions(offscreenSize);
	REQUIRE(colorTexture->AllocateMemory());

	auto offscreenFrameBuffer = renderer.CreateFrameBuffer();
	offscreenFrameBuffer->DefineViewportRegion(V2UInt32(0, 0), offscreenSize);
	REQUIRE(offscreenFrameBuffer->BindTexture(colorTexture.get(), IFrameBuffer::AttachmentType::Color));

	renderPassNode = renderer.CreateRenderPassNode();
	renderPassNode->BindFrameBuffer(offscreenFrameBuffer.get());
	renderer.SetRenderPasses(&renderPassNode, 1);

	// Clear the offscreen color attachment with no renderable content
	renderPassNode->RemoveAllChildNodes();
	renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	offscreenFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("OffscreenClearOnly", "A fully black image after clearing the offscreen color attachment with no renderable content.", std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.95);

	// Clear the offscreen color attachment to a non-black color with distinct channel values
	renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.2f, 0.6f, 0.3f, 1.0f));
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	offscreenFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("OffscreenClearOnlyNonBlack", "A fully green image after clearing the offscreen color attachment to a non-black color with different red, green, and blue values.", std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.95);

	// Load the offscreen color attachment without drawing anything new
	renderPassNode->SetAttachmentLoadStoreBehavior(IFrameBuffer::AttachmentType::Color, 0, IRenderPassNode::AttachmentLoadBehavior::LoadExistingData, IRenderPassNode::AttachmentStoreBehavior::StoreFinalData);
	renderPassNode->RemoveAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	offscreenFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("OffscreenLoadOnly", "A fully green image preserved from the previous frame when the offscreen color attachment is loaded without any renderable content.", std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.95);

	// Clear the offscreen color attachment and draw into the upper left quadrant
	renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
	renderPassNode->AddChildNode(upperLeftProgramNode.get());
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	offscreenFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("OffscreenClearAndUpperLeft", "A black image with a white triangle in the upper-left quadrant after clearing the offscreen color attachment and drawing one triangle.", std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.95);

	// Clear the offscreen color attachment to a non-black color and draw into the upper left quadrant
	renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.15f, 0.35f, 0.7f, 1.0f));
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	offscreenFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("OffscreenClearNonBlackAndUpperLeft", "A blue image with a white triangle in the upper-left quadrant after clearing the offscreen color attachment to a non-black color and drawing one triangle.", std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.95);

	// Load the offscreen color attachment again with no new renderable content
	renderPassNode->RemoveAllChildNodes();
	renderPassNode->SetAttachmentLoadStoreBehavior(IFrameBuffer::AttachmentType::Color, 0, IRenderPassNode::AttachmentLoadBehavior::LoadExistingData, IRenderPassNode::AttachmentStoreBehavior::StoreFinalData);
	renderPassNode->RemoveAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	offscreenFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("OffscreenLoadAfterUpperLeft", "A blue image with the same white triangle still visible in the upper-left quadrant after loading the offscreen color attachment with no new renderable content.", std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.95);

	// Load the offscreen color attachment and add a second triangle in the upper right quadrant
	renderPassNode->AddChildNode(upperRightProgramNode.get());
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	offscreenFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("OffscreenLoadAndUpperRight", "A blue image with the original white triangle in the upper-left quadrant and a red triangle added in the upper-right quadrant after loading the offscreen color attachment.", std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.95);

	// Clear the offscreen color attachment again and draw into the lower left quadrant
	renderPassNode->RemoveAllChildNodes();
	renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
	renderPassNode->AddChildNode(lowerLeftProgramNode.get());
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	offscreenFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("OffscreenClearAndLowerLeft", "A black image with a yellow triangle in the lower-left quadrant after clearing the offscreen color attachment again and drawing a new triangle.", std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.95);

	// Remove all our defined render passes
	renderer.RemoveAllRenderPasses();
	return true;
}

} // namespace cobalt::graphics::testing
