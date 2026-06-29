// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../UnitTestBase.h"
namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

// Define our shader programs
const std::string VertexShader = R"(
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

    OUT.color = IN.color;
    OUT.position = IN.position;

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

DEFINE_UNIT_TEST_WITH_BASE("Resources/VertexBuffer/UpdateTest", UnitTestBase)
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
	auto colorAttributeId = shaderProgram->GetVertexAttributeId("color");

	// Create our vertex buffer and populate it with data
	std::vector<V4Float32> positionVertexData = {{-0.5f, 0.5f, 0.5f, 1.0f}, {0.0f, -0.5f, 0.5f, 1.0f}, {0.5f, 0.5f, 0.5f, 1.0f}};
	std::vector<V3Float32> colorVertexData = {{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}};
	VertexAttribute<V4Float32> vertexAttributePosition(positionVertexData.size(), IVertexAttribute::PerformanceHint::WriteRarely | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	VertexAttribute<V3Float32> vertexAttributeColor(colorVertexData.size(), IVertexAttribute::PerformanceHint::WriteRarely | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	auto vertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttributePosition));
	REQUIRE(vertexAttributePosition.SetInitialData(positionVertexData));
	REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttributeColor));
	REQUIRE(vertexAttributeColor.SetInitialData(colorVertexData));
	REQUIRE(vertexBuffer->AllocateMemory());

	// Create our renderable node
	auto renderableNode = renderer.CreateRenderableNode();
	REQUIRE(renderableNode->BindVertexAttribute(vertexAttributePosition, positionAttributeId));
	REQUIRE(renderableNode->BindVertexAttribute(vertexAttributeColor, colorAttributeId));
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
	auto frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("RGBTriangleDown", "A red, green and blue triangle, pointing down", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Test updating the entire attribute.
	for (auto i = 0; i < 10; i += 1)
	{
		positionVertexData[1].Y() += 0.1f;

		REQUIRE(vertexAttributePosition.QueueDataUpdate(positionVertexData.data(), positionVertexData.size()));

		frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("RGBTriangleFullAttributeUpdateFrame" + std::to_string(i), "Moving the point upwards by updating entire buffer", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.90);
	}

	// Test updating one attribute at a time.
	for (auto i = 0; i < 10; i += 1)
	{
		positionVertexData[1].Y() -= 0.1f;

		REQUIRE(vertexAttributePosition.QueueDataUpdate(positionVertexData.data() + 1, 1, 1));

		frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("RGBTrianglePartUpdateFrame" + std::to_string(i), "Moving the point downwards by updating one value at a time", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.90);
	}

	// Test updating a vertex in sync.
	for (auto i = 0; i < 10; i += 1)
	{
		positionVertexData[1].Y() += 0.1f;
		colorVertexData[1].X() += 0.1f;
		colorVertexData[1].Z() += 0.1f;

		REQUIRE(vertexAttributePosition.QueueDataUpdate(positionVertexData.data() + 1, 1, 1));
		REQUIRE(vertexAttributeColor.QueueDataUpdate(colorVertexData.data() + 1, 1, 1));

		frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("RGBTriangleVertexUpdateFrame" + std::to_string(i), "Moving the point upwards and whitewards one vertex (2 attributes) at a time", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.85);
	}

	// Remove all our defined render passes
	renderer.RemoveAllRenderPasses();
	return true;
}

} // namespace cobalt::graphics::testing
