// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../UnitTestBase.h"
namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

// Define our shader programs
const std::string CombinedShader = R"(
struct VSInput {
    float4 position : position;
};

struct VSOutput {
    float4 position : SV_POSITION;
    float3 color : COLOR;
};

VSOutput VertexShaderRedTriangle(VSInput IN)
{
    VSOutput OUT;
    OUT.color = float3(1.0f, 0.0f, 0.0f);
    OUT.position = IN.position;
    return OUT;
}

VSOutput VertexShaderGreenTriangle(VSInput IN)
{
    VSOutput OUT;
    OUT.color = float3(0.0f, 1.0f, 0.0f);
    OUT.position = IN.position;
    return OUT;
}

VSOutput VertexShaderBlueTriangle(VSInput IN)
{
    VSOutput OUT;
    OUT.color = float3(0.0f, 0.0f, 1.0f);
    OUT.position = IN.position;
    return OUT;
}

float4 FragmentShader(VSOutput IN) : SV_TARGET0
{
    return float4(IN.color, 1.0f);
}
)";

DEFINE_UNIT_TEST_WITH_BASE("Shader/ShaderEntryPointSelection", UnitTestBase)
{
	// Retrieve our important objects
	auto& renderer = session.Renderer();
	auto& uiThread = session.UIThread();
	auto& testWindowInfo = *session.TestWindowPlatformInfo();

	// Define the framebuffer
	auto mainWindowFrameBuffer = renderer.CreateFrameBuffer();
	mainWindowFrameBuffer->DefineViewportRegion(V2UInt32(0, 0), session.TestWindowSize());
	REQUIRE(uiThread.InvokeSync([&] { return mainWindowFrameBuffer->BindWindow(testWindowInfo, IFrameBuffer::WindowDepthStencilMode::DepthUNorm24); }));

	// Create and compile our shader programs
	auto shaderProgramRedTriangle = renderer.CreateShaderProgram();
	REQUIRE(shaderProgramRedTriangle->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(CombinedShader, "VertexShaderRedTriangle")));
	REQUIRE(shaderProgramRedTriangle->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(CombinedShader, "FragmentShader")));
	REQUIRE(shaderProgramRedTriangle->CompileProgram());
	auto shaderProgramGreenTriangle = renderer.CreateShaderProgram();
	REQUIRE(shaderProgramGreenTriangle->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(CombinedShader, "VertexShaderGreenTriangle")));
	REQUIRE(shaderProgramGreenTriangle->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(CombinedShader, "FragmentShader")));
	REQUIRE(shaderProgramGreenTriangle->CompileProgram());
	auto shaderProgramBlueTriangle = renderer.CreateShaderProgram();
	REQUIRE(shaderProgramBlueTriangle->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(CombinedShader, "VertexShaderBlueTriangle")));
	REQUIRE(shaderProgramBlueTriangle->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(CombinedShader, "FragmentShader")));
	REQUIRE(shaderProgramBlueTriangle->CompileProgram());

	// Retrieve our shader attribute IDs
	auto positionAttributeId = shaderProgramRedTriangle->GetVertexAttributeId("position");

	// Create our vertex buffer and populate it with data
	std::vector<V4Float32> positionVertexData = {{-0.5f, 0.5f, 0.5f, 1.0f}, {0.0f, -0.5f, 0.5f, 1.0f}, {0.5f, 0.5f, 0.5f, 1.0f}};
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
	groupNode->AddChildNode(renderableNode.get());

	// Create our program nodes
	auto programNodeRedTringle = renderer.CreateProgramNode();
	REQUIRE(programNodeRedTringle->BindShaderProgram(shaderProgramRedTriangle.get()));
	auto programNodeGreenTringle = renderer.CreateProgramNode();
	REQUIRE(programNodeGreenTringle->BindShaderProgram(shaderProgramGreenTriangle.get()));
	auto programNodeBlueTringle = renderer.CreateProgramNode();
	REQUIRE(programNodeBlueTringle->BindShaderProgram(shaderProgramBlueTriangle.get()));

	// Create our render pass node
	auto renderPassNode = renderer.CreateRenderPassNode();
	renderPassNode->BindFrameBuffer(mainWindowFrameBuffer.get());
	renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
	renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Depth, 0, V4Float32(1.0f, 0.0f, 0.0f, 0.0f));
	renderPassNode->AddChildNode(programNodeRedTringle.get());
	renderPassNode->AddChildNode(programNodeGreenTringle.get());
	renderPassNode->AddChildNode(programNodeBlueTringle.get());

	// Bind our render tree to the renderer
	renderer.SetRenderPasses(&renderPassNode, 1);

	// Add our renderables to the red shader program node
	programNodeRedTringle->AddChildNode(groupNode.get());

	// Capture an image of the scene
	auto frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("RedTriangleDown", "A red triangle pointing down", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Move the triangle to the green shader program node
	programNodeRedTringle->RemoveChildNode(groupNode.get());
	programNodeGreenTringle->AddChildNode(groupNode.get());

	// Capture an image of the scene
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("GreenTriangleDown", "A green triangle pointing down", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Move the triangle to the blue shader program node
	programNodeGreenTringle->RemoveChildNode(groupNode.get());
	programNodeBlueTringle->AddChildNode(groupNode.get());

	// Capture an image of the scene
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("BlueTriangleDown", "A blue triangle pointing down", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Remove all our defined render passes
	groupNode->RemoveAllChildNodes();
	renderer.RemoveAllRenderPasses();
	return true;
}

} // namespace cobalt::graphics::testing
