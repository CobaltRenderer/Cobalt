// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../GeometryHelper.h"
#include "../UnitTestBase.h"
namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

// Define our shader programs
const std::string VertexShader = R"(
struct VSInput {
    float3 position : position;
    float3 normal : normal;
};

struct VSOutput {
    float4 position : SV_POSITION;
    float3 normal : normal;
};

uniform row_major float4x4 viewProj;

VSOutput main(VSInput IN)
{
    VSOutput OUT;

    OUT.position = mul(viewProj, float4(IN.position, 1.0f));
    OUT.normal = IN.normal;

    return OUT;
}
)";
const std::string FragmentShader = R"(
struct VSOutput {
    float4 position : SV_POSITION;
    float3 normal : normal;
};

float4 main(VSOutput IN) : SV_TARGET0
{
    return float4((IN.normal + 1) / 2, 1.0f);
}
)";

DEFINE_UNIT_TEST_WITH_BASE("Scene/BunnyNormals", UnitTestBase)
{
	// Retrieve our important objects
	auto& renderer = session.Renderer();
	auto& uiThread = session.UIThread();
	auto& testWindowInfo = *session.TestWindowPlatformInfo();

	// Define the framebuffer
	auto mainWindowFrameBuffer = renderer.CreateFrameBuffer();
	mainWindowFrameBuffer->DefineViewportRegion(V2UInt32(0, 0), session.TestWindowSize());
	REQUIRE(uiThread.InvokeSync([&] { return mainWindowFrameBuffer->BindWindow(testWindowInfo, IFrameBuffer::WindowDepthStencilMode::DepthUNorm24); }));

	// Create and compile our shader program
	auto shaderProgram = renderer.CreateShaderProgram();
	REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(VertexShader)));
	REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(FragmentShader)));
	REQUIRE(shaderProgram->CompileProgram());

	// Retrieve our shader attribute IDs
	auto positionAttributeId = shaderProgram->GetVertexAttributeId("position");
	auto normalAttributeId = shaderProgram->GetVertexAttributeId("normal");

	// Generate data for the object we want to render
	std::vector<V3Float32> positionVertexData;
	std::vector<V3Float32> normalVertexData;
	std::vector<V1UInt32> indexData;
	Geometry().CreateModelStanfordBunny(positionVertexData, normalVertexData, indexData);

	// Create our vertex buffer and populate it with data
	VertexAttribute<V3Float32> vertexAttributePosition(positionVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	VertexAttribute<V3Float32> vertexAttributeNormal(normalVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	auto vertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttributePosition));
	REQUIRE(vertexAttributePosition.SetInitialData(positionVertexData));
	REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttributeNormal));
	REQUIRE(vertexAttributeNormal.SetInitialData(normalVertexData));
	REQUIRE(vertexBuffer->AllocateMemory());

	// Create our index buffer and populate it with data
	IndexAttribute<V1UInt32> indexAttribute(indexData.size(), IIndexAttribute::PerformanceHint::WriteNever | IIndexAttribute::PerformanceHint::ReadNever, IIndexAttribute::PerformanceHint::WriteNever | IIndexAttribute::PerformanceHint::ReadOften);
	auto indexBuffer = renderer.CreateIndexBuffer();
	REQUIRE(indexBuffer->BindIndexAttribute(indexAttribute));
	REQUIRE(indexAttribute.SetInitialData(indexData));
	REQUIRE(indexBuffer->AllocateMemory());

	// Create our renderable node
	auto renderableNode = renderer.CreateRenderableNode();
	REQUIRE(renderableNode->BindVertexAttribute(vertexAttributePosition, positionAttributeId));
	REQUIRE(renderableNode->BindVertexAttribute(vertexAttributeNormal, normalAttributeId));
	REQUIRE(renderableNode->BindIndexAttribute(indexAttribute));
	REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));

	// Create our state group node
	auto groupNode = renderer.CreateStateGroupNode();
	groupNode->AddChildNode(renderableNode.get());

	// Create our program node
	auto programNode = renderer.CreateProgramNode();
	REQUIRE(programNode->BindShaderProgram(shaderProgram.get()));
	programNode->AddChildNode(groupNode.get());

	// Create our render pass node
	auto renderPassNode = renderer.CreateRenderPassNode();
	renderPassNode->BindFrameBuffer(mainWindowFrameBuffer.get());
	renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
	renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Depth, 0, V4Float32(1.0f, 0.0f, 0.0f, 0.0f));
	renderPassNode->AddChildNode(programNode.get());

	// Bind our render tree to the renderer
	renderer.SetRenderPasses(&renderPassNode, 1);

	// Capture an image of the scene
	auto viewProj = Transform().LookAtCenterPerspective(session.TestWindowSizeAsFloat(), V3Float32(0.0f, -2.0f, 0.0f));
	renderableNode->SetStateValue(shaderProgram->GetStateValueId("viewProj"), viewProj);
	auto frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("BunnyNormals1", "Stanford bunny front on (tail on left)", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Capture another image of the scene with the camera repositioned
	viewProj = Transform().LookAtCenterPerspective(session.TestWindowSizeAsFloat(), V3Float32(-2.0f, 0.0f, 2.0f));
	renderableNode->SetStateValue(shaderProgram->GetStateValueId("viewProj"), viewProj);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("BunnyNormals2", "Stanford bunny facing camera", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Capture another image of the scene with the camera repositioned
	viewProj = Transform().LookAtCenterPerspective(session.TestWindowSizeAsFloat(), V3Float32(2.0f, 2.0f, -2.5f));
	renderableNode->SetStateValue(shaderProgram->GetStateValueId("viewProj"), viewProj);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("BunnyNormals3", "Stanford bunny bottom", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Remove all our defined render passes
	renderer.RemoveAllRenderPasses();
	return true;
}

} // namespace cobalt::graphics::testing
