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

DEFINE_UNIT_TEST_WITH_BASE("Framebuffer/ViewportAndScissor", UnitTestBase)
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

	// Generate data for a fullscreen quad so that viewport and scissor alone define the visible
	// output region
	std::vector<V4Float32> vertexData;
	Geometry().CreateFullscreenQuad(0.5f, vertexData);

	// Create our vertex buffer and populate it with data
	VertexAttribute<V4Float32> vertexAttribute(vertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	auto vertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttribute));
	REQUIRE(vertexAttribute.SetInitialData(vertexData));
	REQUIRE(vertexBuffer->AllocateMemory());

	// Create our renderable node
	auto renderableNode = renderer.CreateRenderableNode();
	REQUIRE(renderableNode->BindVertexAttribute(vertexAttribute, positionAttributeId));
	REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));

	// Create our state group node
	auto groupNode = renderer.CreateStateGroupNode();
	groupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);
	groupNode->AddChildNode(renderableNode.get());

	// Create our program node
	auto programNode = renderer.CreateProgramNode();
	REQUIRE(programNode->BindShaderProgram(shaderProgram.get()));
	programNode->AddChildNode(groupNode.get());

	// Run the viewport and scissor sequence against a window bound framebuffer first
	auto windowFrameBuffer = renderer.CreateFrameBuffer();
	windowFrameBuffer->DefineViewportRegion(V2UInt32(0, 0), session.TestWindowSize());
	REQUIRE(uiThread.InvokeSync([&] { return windowFrameBuffer->BindWindow(testWindowInfo, IFrameBuffer::WindowDepthStencilMode::None); }));

	V2UInt32 windowFullSize = session.TestWindowSize();
	V2UInt32 windowViewportOnlyStart(windowFullSize.X() / 5, windowFullSize.Y() / 7);
	V2UInt32 windowViewportOnlySize((windowFullSize.X() * 11) / 20, (windowFullSize.Y() * 9) / 16);
	V2UInt32 windowScissorOnlyStart((windowFullSize.X() * 3) / 10, windowFullSize.Y() / 6);
	V2UInt32 windowScissorOnlySize((windowFullSize.X() * 9) / 20, (windowFullSize.Y() * 5) / 8);
	V2UInt32 windowOffsetViewportStart(windowFullSize.X() / 8, windowFullSize.Y() / 8);
	V2UInt32 windowOffsetViewportSize((windowFullSize.X() * 3) / 4, (windowFullSize.Y() * 3) / 4);
	V2UInt32 windowOffsetScissorStart((windowFullSize.X() * 3) / 8, windowFullSize.Y() / 8);
	V2UInt32 windowOffsetScissorSize((windowFullSize.X() * 3) / 8, (windowFullSize.Y() * 5) / 8);

	auto renderPassNode = renderer.CreateRenderPassNode();
	renderPassNode->BindFrameBuffer(windowFrameBuffer.get());
	renderPassNode->AddChildNode(programNode.get());
	renderer.SetRenderPasses(&renderPassNode, 1);

	// Capture a full frame draw with no scissor region
	windowFrameBuffer->DefineViewportRegion(V2UInt32(0, 0), windowFullSize);
	windowFrameBuffer->RemoveScissorRegion();
	renderableNode->SetStateValue(drawColorStateId, V4Float32(1.0f, 0.0f, 0.0f, 1.0f));
	renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
	auto frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	windowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("WindowFullViewport", "A full red window image on a black-cleared background, drawn through a full viewport with no scissor region.", std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.95);

	// Capture a centered viewport with no scissor region
	windowFrameBuffer->DefineViewportRegion(windowViewportOnlyStart, windowViewportOnlySize);
	windowFrameBuffer->RemoveScissorRegion();
	renderableNode->SetStateValue(drawColorStateId, V4Float32(0.0f, 1.0f, 0.0f, 1.0f));
	renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	windowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("WindowViewportOnly", "A green rectangle offset within the window on a black background, drawn through a reduced viewport with different X and Y position and size values.", std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.95);

	// Capture a full viewport constrained by an offset rectangular scissor region
	windowFrameBuffer->DefineViewportRegion(V2UInt32(0, 0), windowFullSize);
	windowFrameBuffer->DefineScissorRegion(windowScissorOnlyStart, windowScissorOnlySize);
	renderableNode->SetStateValue(drawColorStateId, V4Float32(0.0f, 0.0f, 1.0f, 1.0f));
	renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	windowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("WindowScissorOnly", "A blue rectangle offset within the window on a black background, drawn through a full viewport with a rectangular scissor region that uses different X and Y position and size values.", std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.95);

	// Capture the intersection of a reduced viewport and an offset scissor region
	windowFrameBuffer->DefineViewportRegion(windowOffsetViewportStart, windowOffsetViewportSize);
	windowFrameBuffer->DefineScissorRegion(windowOffsetScissorStart, windowOffsetScissorSize);
	renderableNode->SetStateValue(drawColorStateId, V4Float32(1.0f, 1.0f, 0.0f, 1.0f));
	renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.15f, 0.15f, 0.15f, 1.0f));
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	windowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("WindowViewportAndScissor", "A yellow rectangular region on a dark grey window background, showing the intersection of a reduced viewport and an offset scissor region.", std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.95);

	// Repeat the same logical sequence using a texture bound framebuffer
	V2UInt32 offscreenFullSize = session.TestWindowSize();
	auto colorTexture = renderer.CreateTextureBuffer2D();
	colorTexture->SetUsageFlags(ITextureBuffer::UsageFlags::FrameBufferOutput);
	colorTexture->SetTextureFormat(ITextureBuffer::ImageFormat::RGBA, ITextureBuffer::DataFormat::UNorm8);
	colorTexture->SetTextureDimensions(offscreenFullSize);
	REQUIRE(colorTexture->AllocateMemory());

	auto offscreenFrameBuffer = renderer.CreateFrameBuffer();
	offscreenFrameBuffer->DefineViewportRegion(V2UInt32(0, 0), offscreenFullSize);
	REQUIRE(offscreenFrameBuffer->BindTexture(colorTexture.get(), IFrameBuffer::AttachmentType::Color));

	V2UInt32 offscreenViewportOnlyStart(offscreenFullSize.X() / 5, offscreenFullSize.Y() / 7);
	V2UInt32 offscreenViewportOnlySize((offscreenFullSize.X() * 11) / 20, (offscreenFullSize.Y() * 9) / 16);
	V2UInt32 offscreenScissorOnlyStart((offscreenFullSize.X() * 3) / 10, offscreenFullSize.Y() / 6);
	V2UInt32 offscreenScissorOnlySize((offscreenFullSize.X() * 9) / 20, (offscreenFullSize.Y() * 5) / 8);
	V2UInt32 offscreenOffsetViewportStart(offscreenFullSize.X() / 8, offscreenFullSize.Y() / 8);
	V2UInt32 offscreenOffsetViewportSize((offscreenFullSize.X() * 3) / 4, (offscreenFullSize.Y() * 3) / 4);
	V2UInt32 offscreenOffsetScissorStart((offscreenFullSize.X() * 3) / 8, offscreenFullSize.Y() / 8);
	V2UInt32 offscreenOffsetScissorSize((offscreenFullSize.X() * 3) / 8, (offscreenFullSize.Y() * 5) / 8);

	renderPassNode = renderer.CreateRenderPassNode();
	renderPassNode->BindFrameBuffer(offscreenFrameBuffer.get());
	renderPassNode->AddChildNode(programNode.get());
	renderer.SetRenderPasses(&renderPassNode, 1);

	// Capture a full frame draw with no scissor region
	offscreenFrameBuffer->DefineViewportRegion(V2UInt32(0, 0), offscreenFullSize);
	offscreenFrameBuffer->RemoveScissorRegion();
	renderableNode->SetStateValue(drawColorStateId, V4Float32(1.0f, 0.0f, 0.0f, 1.0f));
	renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	offscreenFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("OffscreenFullViewport", "A full red square on a black-cleared background, drawn through a full viewport with no scissor region.", std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.95);

	// Capture a centered viewport with no scissor region
	offscreenFrameBuffer->DefineViewportRegion(offscreenViewportOnlyStart, offscreenViewportOnlySize);
	offscreenFrameBuffer->RemoveScissorRegion();
	renderableNode->SetStateValue(drawColorStateId, V4Float32(0.0f, 1.0f, 0.0f, 1.0f));
	renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	offscreenFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("OffscreenViewportOnly", "A green rectangle offset within the image on a black background, drawn through a reduced viewport with different X and Y position and size values.", std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.95);

	// Capture a full viewport constrained by an offset rectangular scissor region
	offscreenFrameBuffer->DefineViewportRegion(V2UInt32(0, 0), offscreenFullSize);
	offscreenFrameBuffer->DefineScissorRegion(offscreenScissorOnlyStart, offscreenScissorOnlySize);
	renderableNode->SetStateValue(drawColorStateId, V4Float32(0.0f, 0.0f, 1.0f, 1.0f));
	renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	offscreenFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("OffscreenScissorOnly", "A blue rectangle offset within the image on a black background, drawn through a full viewport with a rectangular scissor region that uses different X and Y position and size values.", std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.95);

	// Capture the intersection of a reduced viewport and an offset scissor region
	offscreenFrameBuffer->DefineViewportRegion(offscreenOffsetViewportStart, offscreenOffsetViewportSize);
	offscreenFrameBuffer->DefineScissorRegion(offscreenOffsetScissorStart, offscreenOffsetScissorSize);
	renderableNode->SetStateValue(drawColorStateId, V4Float32(1.0f, 1.0f, 0.0f, 1.0f));
	renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.15f, 0.15f, 0.15f, 1.0f));
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	offscreenFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("OffscreenViewportAndScissor", "A yellow rectangular region on a dark grey background, showing the intersection of a reduced viewport and an offset scissor region.", std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.95);

	// Remove all our defined render passes
	renderer.RemoveAllRenderPasses();
	return true;
}

} // namespace cobalt::graphics::testing
