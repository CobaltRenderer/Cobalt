// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../UnitTestBase.h"
namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

// Define our shader programs
const std::string VertexShader = R"(
float4 main(float3 position : position) : SV_POSITION
{
    return float4(position, 1.0);
}
)";
const std::string FragmentShader = R"(
float4 main(float4 position : SV_POSITION) : SV_TARGET0
{
  return float4(1.0, 1.0, 0.0, 1.0);
}
)";

DEFINE_UNIT_TEST_WITH_BASE("Renderable/BufferOffsets", UnitTestBase)
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
	auto positionAttributeId = shaderProgram->GetVertexAttributeId("position");

	// Create our state group node
	auto groupNode = renderer.CreateStateGroupNode();

	// Disable backface culling since our strip data reverses winding order each primitive
	groupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);

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

	// Retrieve the primitive data
	std::vector<V3Float32> positionVertexData;
	std::vector<V1UInt32> indexData;
	Geometry().CreatePrimitiveCircleAsLineStrip(40, positionVertexData, indexData);

	// Create our vertex buffer and populate it with data
	VertexAttribute<V3Float32> vertexAttributePosition(positionVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	auto vertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttributePosition));
	REQUIRE(vertexAttributePosition.SetInitialData(positionVertexData));
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
	REQUIRE(renderableNode->BindIndexAttribute(indexAttribute));
	REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::LineStrip));

	// Add our renderable node to the scene
	groupNode->AddChildNode(renderableNode.get());
	IFrameBufferOutput::unique_ptr frameBufferCapture;

	// Ensure the complete circle is drawn by default
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("YellowCircleComplete", "A complete yellow circle", std::move(frameBufferCapture), cobalt::graphics::IImageDiff::Algorithm::BinaryCountExact | cobalt::graphics::IImageDiff::Algorithm::RegionRanges);

	// Ensure a manual vertex count works
	REQUIRE(renderableNode->SetVertexCount(indexAttribute.GetIndexCount(), 0, 0, 0));
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("YellowCircleCompleteManualVertexCount", "Testing that manually setting the vertex count works", std::move(frameBufferCapture), cobalt::graphics::IImageDiff::Algorithm::BinaryCountExact | cobalt::graphics::IImageDiff::Algorithm::RegionRanges);

	// Ensure that a reduced vertex count takes effect
	REQUIRE(renderableNode->SetVertexCount(indexAttribute.GetIndexCount() - 10, 0, 0, 0));
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("YellowCircleReducedVertexCount", "Testing that a reduced vertex count works", std::move(frameBufferCapture), cobalt::graphics::IImageDiff::Algorithm::BinaryCountExact | cobalt::graphics::IImageDiff::Algorithm::RegionRanges);

	// Ensure that an index buffer offset takes effect
	REQUIRE(renderableNode->SetVertexCount(indexAttribute.GetIndexCount() - 10, 0, 6, 0));
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("YellowCircleIndexBufferOffset", "Testing that an index buffer offset works", std::move(frameBufferCapture), cobalt::graphics::IImageDiff::Algorithm::BinaryCountExact | cobalt::graphics::IImageDiff::Algorithm::RegionRanges);

	// Ensure that a vertex buffer offset takes effect
	REQUIRE(renderableNode->SetVertexCount(indexAttribute.GetIndexCount() - 10, 4, 0, 0));
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("YellowCircleVertexBufferOffset", "Testing that a vertex buffer offset works", std::move(frameBufferCapture), cobalt::graphics::IImageDiff::Algorithm::BinaryCountExact | cobalt::graphics::IImageDiff::Algorithm::RegionRanges);

	// Ensure that a vertex buffer offset combined with an index buffer offset takes effect
	REQUIRE(renderableNode->SetVertexCount(indexAttribute.GetIndexCount() - 15, 4, 6, 0));
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("YellowCircleVertexAndIndexBufferOffset", "Testing that a vertex buffer offset with an index buffer offset works", std::move(frameBufferCapture), cobalt::graphics::IImageDiff::Algorithm::BinaryCountExact | cobalt::graphics::IImageDiff::Algorithm::RegionRanges);

	// Ensure that an index value offset takes effect
	REQUIRE(renderableNode->SetVertexCount(indexAttribute.GetIndexCount() - 20, 7, 10, -4));
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("YellowCircleIndexValueOffset", "Testing that an index value offset works", std::move(frameBufferCapture), cobalt::graphics::IImageDiff::Algorithm::BinaryCountExact | cobalt::graphics::IImageDiff::Algorithm::RegionRanges);

	// Remove the renderable node
	groupNode->RemoveChildNode(renderableNode.get());

	// Remove all our defined render passes
	renderer.RemoveAllRenderPasses();
	return true;
}

} // namespace cobalt::graphics::testing
