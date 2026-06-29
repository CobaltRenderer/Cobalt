// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../UnitTestBase.h"
namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

struct UniformBuffer
{
	std::array<V4Float32, 3> myCoords;
};

// Define our shader programs
const std::string VertexShader = R"(
struct UniformBuffer
{
  float4 coords[3];
};

cbuffer UniformCBuffer
{
  UniformBuffer myCB;
};

// Shader model 5.1+ version. Requires Direct3D 12 renderer. Recommend to use the cbuffer version for compatibility.
//ConstantBuffer<UniformBuffer> myCB;

float4 main(uint pointIndex : pointIndex) : SV_POSITION
{
  return myCB.coords[pointIndex];
}
)";
const std::string FragmentShader = R"(
float4 main(float4 position : SV_POSITION) : SV_TARGET0
{
  return float4(1.0, 0.0, 0.0, 1.0);
}
)";

DEFINE_UNIT_TEST_WITH_BASE("Resources/StateBuffer/StateBufferWithNoLayout", UnitTestBase)
{
	// This test works by putting a uniform buffer ( / Constant buffer / State
	// buffer / CBV / UBO / etc) on the GPU defining an array of 4D points,
	// and sending per-vertex data of uint8_t lookup values. We test flushing
	// the state buffer and testing to see if the array lookup moves the
	// points.

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
	auto uniformBufferId = shaderProgram->GetStateBufferId("UniformCBuffer");
	// Shader model 5.1+
	//auto uniformBufferId = shaderProgram->GetStateBufferId("myCB");

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

	// Create a state buffer to hold the uniform buffer
	auto stateBuffer = renderer.CreateStateBuffer();
	UniformBuffer ub = {};
	ub.myCoords = {V4Float32{-0.5f, 0.5f, 0.5f, 1.0f}, {0.0f, -0.5f, 0.5f, 1.0f}, {0.5f, 0.5f, 0.5f, 1.0f}};
	stateBuffer->SetManualPageSize(sizeof(UniformBuffer));
	stateBuffer->SetPageSettings(1);
	stateBuffer->SetPerformanceHints(IStateBuffer::PerformanceHint::WriteOften | IStateBuffer::PerformanceHint::ReadNever, IStateBuffer::PerformanceHint::WriteNever | IStateBuffer::PerformanceHint::ReadOften);
	REQUIRE(stateBuffer->AllocateMemory());
	stateBuffer->SetRawPageData(0, reinterpret_cast<const uint8_t*>(&ub), sizeof(UniformBuffer));

	// Bind the state buffer to our renderable
	renderableNode->BindStateBuffer(uniformBufferId, stateBuffer.get(), 0);

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

	// Change one of the uniform buffer elements so the triangle changes.
	ub.myCoords[1].Y() = 0.0f;
	stateBuffer->SetRawPageData(0, reinterpret_cast<const uint8_t*>(&ub), sizeof(UniformBuffer));
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("TriangleSquished", "A red triangle, squished a bit due to a full page update of data", std::move(frameBufferCapture));

	// Now change only a tiny chunk of the buffer - 4 bytes.
	float newValue = -1.0f;
	stateBuffer->SetRawPageData(0, reinterpret_cast<const uint8_t*>(&newValue), sizeof(float), 5 * sizeof(float));
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("TriangleStretched", "A red triangle, stretched a bit due to a single-float update within a page", std::move(frameBufferCapture));

	// Remove all our defined render passes
	renderer.RemoveAllRenderPasses();
	return true;
}

} // namespace cobalt::graphics::testing
