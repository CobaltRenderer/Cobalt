// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../UnitTestBase.h"
namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

namespace {

// Define our shader programs
const std::string VertexShader = R"(
struct VSInput {
    float4 position : position;
};

float4 main(VSInput IN) : SV_POSITION
{
    return IN.position;
}
)";
const std::string FragmentShader = R"(
uniform int4 signed8Value;
uniform int4 signed16Value;
uniform int4 signed32Value;
uniform uint4 unsigned8Value;
uniform uint4 unsigned16Value;
uniform uint4 unsigned32Value;
uniform float4 float32Value;
uniform float4 float64Value;

float4 main() : SV_TARGET0
{
    float signedContribution = float(signed8Value.x + signed16Value.x + signed32Value.x) * 0.01f;
    float unsignedContribution = float(unsigned8Value.x + unsigned16Value.x + unsigned32Value.x) * 0.01f;
    return saturate(float32Value + (float64Value * 0.01f) + float4(signedContribution + unsignedContribution, 0.0f, 0.0f, 0.0f));
}
)";

} // namespace

DEFINE_UNIT_TEST_WITH_BASE("Resources/StateValue/StateValueTypeApplication", UnitTestBase)
{
	// Retrieve our important objects
	auto& renderer = session.Renderer();
	auto& uiThread = session.UIThread();
	auto& testWindowInfo = *session.TestWindowPlatformInfo();

	// Create a window-bound framebuffer so the state value entries are applied during a normal draw.
	auto frameBuffer = renderer.CreateFrameBuffer();
	frameBuffer->DefineViewportRegion(V2UInt32(0, 0), session.TestWindowSize());
	REQUIRE(uiThread.InvokeSync([&] { return frameBuffer->BindWindow(testWindowInfo, IFrameBuffer::WindowDepthStencilMode::None); }));

	// Create and compile the shader program which consumes each value family.
	auto shaderProgram = renderer.CreateShaderProgram();
	REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(VertexShader)));
	REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(FragmentShader)));
	REQUIRE(shaderProgram->CompileProgram());
	auto positionAttributeId = shaderProgram->GetVertexAttributeId("position");
	auto signed8ValueId = shaderProgram->GetStateValueId("signed8Value");
	auto signed16ValueId = shaderProgram->GetStateValueId("signed16Value");
	auto signed32ValueId = shaderProgram->GetStateValueId("signed32Value");
	auto unsigned8ValueId = shaderProgram->GetStateValueId("unsigned8Value");
	auto unsigned16ValueId = shaderProgram->GetStateValueId("unsigned16Value");
	auto unsigned32ValueId = shaderProgram->GetStateValueId("unsigned32Value");
	auto float32ValueId = shaderProgram->GetStateValueId("float32Value");
	auto float64ValueId = shaderProgram->GetStateValueId("float64Value");

	// Create a fullscreen quad for the draw.
	std::vector<V4Float32> positionVertexData;
	Geometry().CreateFullscreenQuad(0.5f, positionVertexData);
	VertexAttribute<V4Float32> vertexAttributePosition(positionVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	auto vertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttributePosition));
	REQUIRE(vertexAttributePosition.SetInitialData(positionVertexData));
	REQUIRE(vertexBuffer->AllocateMemory());

	// Program-node constants and renderable state values use separate storage paths, so apply a mix of both.
	auto renderableNode = renderer.CreateRenderableNode();
	REQUIRE(renderableNode->BindVertexAttribute(vertexAttributePosition, positionAttributeId));
	REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
	renderableNode->SetStateValue(signed8ValueId, V4Int8(1, 0, 0, 0));
	renderableNode->SetStateValue(signed16ValueId, V4Int16(2, 0, 0, 0));
	renderableNode->SetStateValue(signed32ValueId, V4Int32(3, 0, 0, 0));
	renderableNode->SetStateValue(unsigned8ValueId, V4UInt8(4, 0, 0, 0));
	renderableNode->SetStateValue(unsigned16ValueId, V4UInt16(5, 0, 0, 0));
	renderableNode->SetStateValue(unsigned32ValueId, V4UInt32(6, 0, 0, 0));

	auto stateGroupNode = renderer.CreateStateGroupNode();
	stateGroupNode->SetStateValue(float32ValueId, V4Float32(0.25f, 0.35f, 0.45f, 1.0f));
	// Keep the double state entry at zero. This still exercises the double-value binding path, while avoiding a
	// non-zero double-to-float uniform conversion where backend APIs have different native representations.
	stateGroupNode->SetStateValue(float64ValueId, V4Float64(0.0, 0.0, 0.0, 0.0));
	stateGroupNode->AddChildNode(renderableNode.get());

	auto programNode = renderer.CreateProgramNode();
	REQUIRE(programNode->BindShaderProgram(shaderProgram.get()));
	programNode->SetConstantStateValue(float32ValueId, V4Float32(0.25f, 0.35f, 0.45f, 1.0f));
	programNode->AddChildNode(stateGroupNode.get());

	// Bind the render tree and draw once to force all value entries to be applied to the backend shader state.
	auto renderPassNode = renderer.CreateRenderPassNode();
	renderPassNode->BindFrameBuffer(frameBuffer.get());
	renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
	renderPassNode->AddChildNode(programNode.get());
	renderer.SetRenderPasses(&renderPassNode, 1);

	auto frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("MixedNumericStateValues", "A fullscreen blue-green color generated from signed, unsigned, and floating-point state values.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);
	renderer.RemoveAllRenderPasses();
	session.AddTestSuccess("StateValueTypeApplication", "Program constants and state containers applied signed, unsigned, and floating point state value entries during a live draw.");
	return true;
}

} // namespace cobalt::graphics::testing
