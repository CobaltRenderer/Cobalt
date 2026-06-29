// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../UnitTestBase.h"
namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

// Define our shader programs
const std::string VertexShader = R"(
struct UniformBuffer
{
  float3 coords[3];
  float3 color[3];
};

cbuffer UniformCBuffer
{
  UniformBuffer myCB;
};

struct VSOutput {
    float4 position : SV_POSITION;
    float3 color : COLOR;
};

VSOutput main(uint pointIndex : pointIndex)
{
    VSOutput OUT;
    OUT.color = myCB.color[pointIndex];
    OUT.position = float4(myCB.coords[pointIndex], 1.0f);
    return OUT;
}
)";
const std::string FragmentShader = R"(
struct VSOutput {
    float4 position : SV_POSITION;
    float3 color : COLOR;
};

float4 main(VSOutput IN) : SV_TARGET0
{
    return float4(IN.color, 1.0f);
}
)";

DEFINE_UNIT_TEST_WITH_BASE("Resources/StateBuffer/StateBufferPaging", UnitTestBase)
{
	// Retrieve our important objects
	auto& renderer = session.Renderer();
	auto& uiThread = session.UIThread();
	auto& testWindowPlatformInfo = *session.TestWindowPlatformInfo();

	// Define the framebuffer
	auto testWindowFrameBuffer = renderer.CreateFrameBuffer();
	testWindowFrameBuffer->DefineViewportRegion(V2UInt32(0, 0), session.TestWindowSize());
	REQUIRE(uiThread.InvokeSync([&] { return testWindowFrameBuffer->BindWindow(testWindowPlatformInfo, IFrameBuffer::WindowDepthStencilMode::DepthUNorm24); }));

	// Create and compile our shader program
	auto shaderProgram = renderer.CreateShaderProgram();
	REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(VertexShader)));
	REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(FragmentShader)));
	REQUIRE(shaderProgram->CompileProgram());

	// Retrieve our shader attribute IDs
	auto positionAttributeId = shaderProgram->GetVertexAttributeId("pointIndex");
	ASSERT((int)positionAttributeId == 0);

	// Create our vertex buffer and populate it with data
	std::vector<V1UInt8> positionVertexData = {{0}, {1}, {2}};
	VertexAttribute<V1UInt8> vertexAttributePosition(positionVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
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
	groupNode->AddChildNode(renderableNode.get());

	// Create our state buffer
	auto stateBufferId = shaderProgram->GetStateBufferId("UniformCBuffer");
	auto stateBuffer = renderer.CreateStateBuffer();
	auto stateBufferLayout = renderer.CreateStateBufferLayout();
	REQUIRE(shaderProgram->LoadStateBufferLayoutFromShader(stateBufferId, stateBufferLayout.get()));
	REQUIRE(stateBuffer->BindBufferLayout(stateBufferLayout.get()));
	stateBuffer->SetPerformanceHints(IStateBuffer::PerformanceHint::WriteOften | IStateBuffer::PerformanceHint::ReadNever, IStateBuffer::PerformanceHint::WriteNever | IStateBuffer::PerformanceHint::ReadOften);
	stateBuffer->SetPageSettings(2, true);
	REQUIRE(stateBuffer->AllocateMemory());

	// Set our initial state buffer values. We start with two different pages populated with two distinct versions of a
	// triangle, the first solid red pointing down, and the second a squished RGB triangle.
	auto stateValudIdCoords = stateBuffer->GetStateValueId("myCB.coords[]");
	auto stateValudIdColor = stateBuffer->GetStateValueId("myCB.color[]");
	stateBuffer->SetStateValueForPage(0, stateValudIdCoords, V3Float32(-0.5f, 0.5f, 0.5f), 0);
	stateBuffer->SetStateValueForPage(0, stateValudIdCoords, V3Float32(0.0f, -0.5f, 0.5f), 1);
	stateBuffer->SetStateValueForPage(0, stateValudIdCoords, V3Float32(0.5f, 0.5f, 0.5f), 2);
	stateBuffer->SetStateValueForPage(0, stateValudIdColor, V3Float32(1.0f, 0.0f, 0.0f), 0);
	stateBuffer->SetStateValueForPage(0, stateValudIdColor, V3Float32(1.0f, 0.0f, 0.0f), 1);
	stateBuffer->SetStateValueForPage(0, stateValudIdColor, V3Float32(1.0f, 0.0f, 0.0f), 2);
	stateBuffer->SetStateValueForPage(1, stateValudIdCoords, V3Float32(-0.5f, 0.5f, 0.5f), 0);
	stateBuffer->SetStateValueForPage(1, stateValudIdCoords, V3Float32(0.0f, 0.0f, 0.5f), 1);
	stateBuffer->SetStateValueForPage(1, stateValudIdCoords, V3Float32(0.5f, 0.5f, 0.5f), 2);
	stateBuffer->SetStateValueForPage(1, stateValudIdColor, V3Float32(1.0f, 0.0f, 0.0f), 0);
	stateBuffer->SetStateValueForPage(1, stateValudIdColor, V3Float32(0.0f, 1.0f, 0.0f), 1);
	stateBuffer->SetStateValueForPage(1, stateValudIdColor, V3Float32(0.0f, 0.0f, 1.0f), 2);

	// Bind the state buffer to our renderable
	renderableNode->BindStateBuffer(stateBufferId, stateBuffer.get(), 0);

	// Create our program node
	auto programNode = renderer.CreateProgramNode();
	REQUIRE(programNode->BindShaderProgram(shaderProgram.get()));
	programNode->AddChildNode(groupNode.get());

	// Create our render pass node
	auto renderPassNode = renderer.CreateRenderPassNode();
	renderPassNode->BindFrameBuffer(testWindowFrameBuffer.get());
	renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
	renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Depth, 0, V4Float32(1.0f, 0.0f, 0.0f, 0.0f));
	renderPassNode->AddChildNode(programNode.get());

	// Bind our render tree to the renderer
	renderer.SetRenderPasses(&renderPassNode, 1);

	// Capture an image of the scene
	auto frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("TriangleDown", "A red triangle, pointing down", std::move(frameBufferCapture));

	// Switch to the second triangle
	renderableNode->BindStateBuffer(stateBufferId, stateBuffer.get(), 1);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("TriangleSquished", "An RGB triangle, squished a bit", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Add a new page to the buffer dynamically, and add an elongated green triangle.
	REQUIRE(stateBuffer->ResizePageCount(3));
	stateBuffer->SetStateValueForPage(2, stateValudIdCoords, V3Float32(-0.5f, 0.5f, 0.5f), 0);
	stateBuffer->SetStateValueForPage(2, stateValudIdCoords, V3Float32(0.0f, -1.0f, 0.5f), 1);
	stateBuffer->SetStateValueForPage(2, stateValudIdCoords, V3Float32(0.5f, 0.5f, 0.5f), 2);
	stateBuffer->SetStateValueForPage(2, stateValudIdColor, V3Float32(0.0f, 1.0f, 0.0f), 0);
	stateBuffer->SetStateValueForPage(2, stateValudIdColor, V3Float32(0.0f, 1.0f, 0.0f), 1);
	stateBuffer->SetStateValueForPage(2, stateValudIdColor, V3Float32(0.0f, 1.0f, 0.0f), 2);

	// Switch to the third triangle
	renderableNode->BindStateBuffer(stateBufferId, stateBuffer.get(), 2);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("TriangleStretched", "A green triangle, stretched a bit", std::move(frameBufferCapture));

	// Remove all our defined render passes
	renderer.RemoveAllRenderPasses();
	return true;
}

} // namespace cobalt::graphics::testing
