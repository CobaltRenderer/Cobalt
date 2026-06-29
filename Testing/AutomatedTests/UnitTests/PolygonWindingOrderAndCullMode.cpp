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
uniform float4 frontFaceColor;
uniform float4 backFaceColor;

float4 main(bool isFrontFace : SV_IsFrontFace) : SV_TARGET0
{
    return isFrontFace ? frontFaceColor : backFaceColor;
}
)";

DEFINE_UNIT_TEST_WITH_BASE("StateGroup/PolygonWindingOrderAndCullMode", UnitTestBase)
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
	auto frontFaceColorStateId = shaderProgram->GetStateValueId("frontFaceColor");
	auto backFaceColorStateId = shaderProgram->GetStateValueId("backFaceColor");

	// Generate data for the triangle we want to render
	std::vector<V4Float32> positionVertexData;
	Geometry().CreateRGBTrianglePositions(0.5f, positionVertexData);

	// Create our vertex buffer and populate it with data
	VertexAttribute<V4Float32> vertexAttributePosition(positionVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	auto vertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttributePosition));
	REQUIRE(vertexAttributePosition.SetInitialData(positionVertexData));
	REQUIRE(vertexBuffer->AllocateMemory());

	// Create our renderable node
	auto renderableNode = renderer.CreateRenderableNode();
	REQUIRE(renderableNode->BindVertexAttribute(vertexAttributePosition, positionAttributeId));
	REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));

	// Create our state group node
	auto groupNode = renderer.CreateStateGroupNode();
	groupNode->SetStateValue(frontFaceColorStateId, V4Float32(1.0f, 0.2f, 0.2f, 1.0f));
	groupNode->SetStateValue(backFaceColorStateId, V4Float32(0.2f, 0.6f, 1.0f, 1.0f));
	groupNode->AddChildNode(renderableNode.get());

	// Create our program node
	auto programNode = renderer.CreateProgramNode();
	REQUIRE(programNode->BindShaderProgram(shaderProgram.get()));
	programNode->AddChildNode(groupNode.get());

	// Create our render pass node
	auto renderPassNode = renderer.CreateRenderPassNode();
	renderPassNode->BindFrameBuffer(frameBuffer.get());
	renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
	renderPassNode->AddChildNode(programNode.get());

	// Bind our render tree to the renderer
	renderer.SetRenderPasses(&renderPassNode, 1);

	// Capture the triangle with culling disabled and counter clockwise winding treated as the front
	// face
	groupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);
	groupNode->SetPolygonWindingOrder(IStateGroupNode::PolygonWindingOrder::CounterClockwise);
	auto frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("CullNoneCounterClockwiseFrontFace", "A red triangle on a black background, where culling is disabled and counter-clockwise winding is treated as the front face.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Capture the triangle with culling disabled and clockwise winding treated as the front face
	groupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);
	groupNode->SetPolygonWindingOrder(IStateGroupNode::PolygonWindingOrder::Clockwise);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("CullNoneClockwiseFrontFace", "A blue triangle on a black background, where culling is disabled and clockwise winding is treated as the front face.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Capture the triangle with back face culling and counter clockwise winding treated as the front
	// face
	groupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::Back);
	groupNode->SetPolygonWindingOrder(IStateGroupNode::PolygonWindingOrder::CounterClockwise);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("CullBackCounterClockwiseFrontFace", "A red triangle on a black background, where back-face culling removes the back-facing side and counter-clockwise winding is treated as the front face.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Capture the triangle with back face culling and clockwise winding treated as the front face
	groupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::Back);
	groupNode->SetPolygonWindingOrder(IStateGroupNode::PolygonWindingOrder::Clockwise);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("CullBackClockwiseFrontFace", "A black image, where back-face culling removes the triangle after clockwise winding is treated as the front face.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Capture the triangle with front face culling and counter clockwise winding treated as the
	// front face
	groupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::Front);
	groupNode->SetPolygonWindingOrder(IStateGroupNode::PolygonWindingOrder::CounterClockwise);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("CullFrontCounterClockwiseFrontFace", "A black image, where front-face culling removes the triangle when counter-clockwise winding is treated as the front face.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Capture the triangle with front face culling and clockwise winding treated as the front face
	groupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::Front);
	groupNode->SetPolygonWindingOrder(IStateGroupNode::PolygonWindingOrder::Clockwise);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("CullFrontClockwiseFrontFace", "A blue triangle on a black background, where front-face culling leaves the back-facing side visible after clockwise winding is treated as the front face.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Remove all our defined render passes
	renderer.RemoveAllRenderPasses();
	return true;
}

} // namespace cobalt::graphics::testing
