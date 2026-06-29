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
    float2 instanceOffset : instanceOffset;
    float3 instanceColor : instanceColor;
};

struct VSOutput {
    float4 position : SV_POSITION;
    float3 color : COLOR;
};

VSOutput main(VSInput IN)
{
    VSOutput OUT;
    OUT.position = float4(IN.position.xy + IN.instanceOffset, IN.position.z, IN.position.w);
    OUT.color = IN.instanceColor;
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

} // namespace

DEFINE_UNIT_TEST_WITH_BASE("Renderable/InstanceMode", UnitTestBase)
{
	// Retrieve our important objects
	auto& renderer = session.Renderer();
	auto& uiThread = session.UIThread();
	auto& testWindowInfo = *session.TestWindowPlatformInfo();
	bool instanceOffsetSupported = session.Device().IsFeatureSupported(IGraphicsDevice::Feature::InstanceOffset);

	auto frameBuffer = renderer.CreateFrameBuffer();
	frameBuffer->DefineViewportRegion(V2UInt32(0, 0), session.TestWindowSize());
	REQUIRE(uiThread.InvokeSync([&] { return frameBuffer->BindWindow(testWindowInfo, IFrameBuffer::WindowDepthStencilMode::None); }));

	// Create and compile our shader program
	auto shaderProgram = renderer.CreateShaderProgram();
	REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(VertexShader)));
	REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(FragmentShader)));
	REQUIRE(shaderProgram->CompileProgram());

	// Retrieve our shader attribute IDs
	auto positionAttributeId = shaderProgram->GetVertexAttributeId("position");
	auto instanceOffsetAttributeId = shaderProgram->GetVertexAttributeId("instanceOffset");
	auto instanceColorAttributeId = shaderProgram->GetVertexAttributeId("instanceColor");

	// Create our vertex and instance data
	std::vector<V4Float32> positionVertexData = {
	  V4Float32(-0.20f, 0.50f, 0.5f, 1.0f),
	  V4Float32(-0.20f, -0.50f, 0.5f, 1.0f),
	  V4Float32(0.20f, -0.50f, 0.5f, 1.0f),
	  V4Float32(-0.20f, 0.50f, 0.5f, 1.0f),
	  V4Float32(0.20f, -0.50f, 0.5f, 1.0f),
	  V4Float32(0.20f, 0.50f, 0.5f, 1.0f),
	};
	std::vector<V2Float32> instanceOffsetData = {V2Float32(-0.50f, 0.0f), V2Float32(0.50f, 0.0f)};
	std::vector<V3Float32> instanceColorData = {V3Float32(1.0f, 0.0f, 0.0f), V3Float32(0.0f, 1.0f, 0.0f)};
	std::vector<V1UInt32> indexData = {
	  V1UInt32(0),
	  V1UInt32(1),
	  V1UInt32(2),
	  V1UInt32(3),
	  V1UInt32(4),
	  V1UInt32(5),
	};

	VertexAttribute<V4Float32> vertexAttributePosition(positionVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	VertexAttribute<V2Float32> vertexAttributeInstanceOffset(instanceOffsetData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	VertexAttribute<V3Float32> vertexAttributeInstanceColor(instanceColorData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	auto vertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttributePosition));
	REQUIRE(vertexAttributePosition.SetInitialData(positionVertexData));
	REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttributeInstanceOffset));
	REQUIRE(vertexAttributeInstanceOffset.SetInitialData(instanceOffsetData));
	REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttributeInstanceColor));
	REQUIRE(vertexAttributeInstanceColor.SetInitialData(instanceColorData));
	REQUIRE(vertexBuffer->AllocateMemory());

	IndexAttribute<V1UInt32> indexAttribute(indexData.size(), IIndexAttribute::PerformanceHint::WriteNever | IIndexAttribute::PerformanceHint::ReadNever, IIndexAttribute::PerformanceHint::WriteNever | IIndexAttribute::PerformanceHint::ReadOften);
	auto indexBuffer = renderer.CreateIndexBuffer();
	REQUIRE(indexBuffer->BindIndexAttribute(indexAttribute));
	REQUIRE(indexAttribute.SetInitialData(indexData));
	REQUIRE(indexBuffer->AllocateMemory());

	// Create our renderable node
	auto renderableNode = renderer.CreateRenderableNode();
	REQUIRE(renderableNode->BindVertexAttribute(vertexAttributePosition, positionAttributeId));
	REQUIRE(renderableNode->BindVertexInstanceAttribute(vertexAttributeInstanceOffset, instanceOffsetAttributeId));
	REQUIRE(renderableNode->BindVertexInstanceAttribute(vertexAttributeInstanceColor, instanceColorAttributeId));
	REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
	REQUIRE(renderableNode->SetInstanceMode(2));

	auto indexedRenderableNode = renderer.CreateRenderableNode();
	REQUIRE(indexedRenderableNode->BindVertexAttribute(vertexAttributePosition, positionAttributeId));
	REQUIRE(indexedRenderableNode->BindVertexInstanceAttribute(vertexAttributeInstanceOffset, instanceOffsetAttributeId));
	REQUIRE(indexedRenderableNode->BindVertexInstanceAttribute(vertexAttributeInstanceColor, instanceColorAttributeId));
	REQUIRE(indexedRenderableNode->BindIndexAttribute(indexAttribute));
	REQUIRE(indexedRenderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
	REQUIRE(indexedRenderableNode->SetInstanceMode(2));

	// Create our state group node
	auto groupNode = renderer.CreateStateGroupNode();
	groupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);
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

	// Both instances should draw by default.
	auto frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("TwoInstances", "A red instance on the left and a green instance on the right.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Drawing one instance from offset one should draw only the second instance.
	if (instanceOffsetSupported)
	{
		REQUIRE(renderableNode->SetInstanceMode(1, 1));
		frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("SecondInstanceOnly", "Only the green right-hand instance is rendered after setting an instance count of one with an offset of one.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);
	}
	else
	{
		session.AddTestSkipped("SecondInstanceOnly", "This part of the test was skipped, as the current renderer doesn't support instance offsets on this device.");
	}

	// Repeat the same instance tests with an index buffer bound.
	IRenderableNode* indexedRenderableNodes[] = {indexedRenderableNode.get()};
	groupNode->SetChildNodes(&indexedRenderableNodes[0], 1);
	REQUIRE(indexedRenderableNode->SetInstanceMode(2));
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("TwoInstancesIndexed", "A red instance on the left and a green instance on the right, rendered with an index buffer bound.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	if (instanceOffsetSupported)
	{
		REQUIRE(indexedRenderableNode->SetInstanceMode(1, 1));
		frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("SecondInstanceOnlyIndexed", "Only the green right-hand instance is rendered with an index buffer bound after setting an instance count of one with an offset of one.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);
	}
	else
	{
		session.AddTestSkipped("SecondInstanceOnlyIndexed", "This part of the test was skipped, as the current renderer doesn't support instance offsets on this device.");
	}

	// Remove all our defined render passes
	renderer.RemoveAllRenderPasses();
	return true;
}

} // namespace cobalt::graphics::testing
