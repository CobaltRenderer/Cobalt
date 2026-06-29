// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../UnitTestBase.h"
namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

namespace {
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
} // namespace

DEFINE_UNIT_TEST_WITH_BASE("Renderable/IndirectDraw", UnitTestBase)
{
	// Ensure data arrays and indirect drawing are supported by the current renderer
	if (!session.Device().IsFeatureSupported(IGraphicsDevice::Feature::IndirectDraw))
	{
		session.AddTestSkipped("Skipped test", "This test was skipped, as the current renderer doesn't support indirect drawing.");
		return true;
	}
	if (!session.Device().IsFeatureSupported(IGraphicsDevice::Feature::ResourceArrays))
	{
		session.AddTestSkipped("Skipped test", "This test was skipped, as the current renderer doesn't support data arrays.");
		return true;
	}

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

	// Create our data array
	auto dataArray = renderer.CreateDataArray();
	dataArray->SetUsageFlags(IDataArray::UsageFlags::IndirectDrawSource);
	dataArray->SetBufferLayout(sizeof(IRenderableNode::IndexedIndirectDrawParams), 1);
	REQUIRE(dataArray->AllocateMemory());

	// Retrieve our shader attribute IDs
	auto positionAttributeId = shaderProgram->GetVertexAttributeId("position");

	// Create our state group node
	auto groupNode = renderer.CreateStateGroupNode();

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

	// Enable indirect drawing for this renderable
	REQUIRE(renderableNode->SetIndirectDraw(1, dataArray.get()));

	// Add our renderable node to the scene
	groupNode->AddChildNode(renderableNode.get());
	IFrameBufferOutput::unique_ptr frameBufferCapture;

	// Initialize our indirect draw params
	IRenderableNode::IndexedIndirectDrawParams indexedIndirectDrawParams{};
	indexedIndirectDrawParams.indexCount = 0;
	indexedIndirectDrawParams.instanceCount = 1;
	indexedIndirectDrawParams.firstIndex = 0;
	indexedIndirectDrawParams.vertexOffset = 0;
	indexedIndirectDrawParams.firstInstance = 0;

	// Ensure the complete circle is drawn by default
	// Equivalent to renderableNode->SetVertexCount(indexAttribute.GetIndexCount(), 0, 0, 0);
	indexedIndirectDrawParams.indexCount = (uint32_t)indexAttribute.GetIndexCount();
	indexedIndirectDrawParams.firstIndex = 0;
	indexedIndirectDrawParams.vertexOffset = 0;
	REQUIRE(dataArray->QueueDataUpdate(&indexedIndirectDrawParams, sizeof(IRenderableNode::IndexedIndirectDrawParams)));
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("YellowCircleComplete", "A complete yellow circle", std::move(frameBufferCapture), cobalt::graphics::IImageDiff::Algorithm::BinaryCountExact | cobalt::graphics::IImageDiff::Algorithm::RegionRanges);

	// Ensure that a reduced vertex count takes effect
	// Equivalent to renderableNode->SetVertexCount(indexAttribute.GetIndexCount() - 10, 0, 0, 0);
	indexedIndirectDrawParams.indexCount = (uint32_t)indexAttribute.GetIndexCount() - 10;
	indexedIndirectDrawParams.firstIndex = 0;
	indexedIndirectDrawParams.vertexOffset = 0;
	REQUIRE(dataArray->QueueDataUpdate(&indexedIndirectDrawParams, sizeof(IRenderableNode::IndexedIndirectDrawParams)));
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("YellowCircleReducedVertexCount", "Testing that a reduced vertex count works", std::move(frameBufferCapture), cobalt::graphics::IImageDiff::Algorithm::BinaryCountExact | cobalt::graphics::IImageDiff::Algorithm::RegionRanges);

	// Ensure that an index buffer offset takes effect
	// Equivalent to renderableNode->SetVertexCount(indexAttribute.GetIndexCount() - 10, 0, 6, 0);
	indexedIndirectDrawParams.indexCount = (uint32_t)indexAttribute.GetIndexCount() - 10;
	indexedIndirectDrawParams.firstIndex = 6;
	indexedIndirectDrawParams.vertexOffset = 0;
	REQUIRE(dataArray->QueueDataUpdate(&indexedIndirectDrawParams, sizeof(IRenderableNode::IndexedIndirectDrawParams)));
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("YellowCircleIndexBufferOffset", "Testing that an index buffer offset works", std::move(frameBufferCapture), cobalt::graphics::IImageDiff::Algorithm::BinaryCountExact | cobalt::graphics::IImageDiff::Algorithm::RegionRanges);

	// Ensure that a vertex buffer offset takes effect
	// Equivalent to renderableNode->SetVertexCount(indexAttribute.GetIndexCount() - 10, 4, 0, 0);
	indexedIndirectDrawParams.indexCount = (uint32_t)indexAttribute.GetIndexCount() - 10;
	indexedIndirectDrawParams.firstIndex = 0;
	indexedIndirectDrawParams.vertexOffset = 4;
	REQUIRE(dataArray->QueueDataUpdate(&indexedIndirectDrawParams, sizeof(IRenderableNode::IndexedIndirectDrawParams)));
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("YellowCircleVertexBufferOffset", "Testing that a vertex buffer offset works", std::move(frameBufferCapture), cobalt::graphics::IImageDiff::Algorithm::BinaryCountExact | cobalt::graphics::IImageDiff::Algorithm::RegionRanges);

	// Ensure that a vertex buffer offset combined with an index buffer offset takes effect
	// Equivalent to renderableNode->SetVertexCount(indexAttribute.GetIndexCount() - 15, 4, 6, 0);
	indexedIndirectDrawParams.indexCount = (uint32_t)indexAttribute.GetIndexCount() - 15;
	indexedIndirectDrawParams.firstIndex = 6;
	indexedIndirectDrawParams.vertexOffset = 4;
	REQUIRE(dataArray->QueueDataUpdate(&indexedIndirectDrawParams, sizeof(IRenderableNode::IndexedIndirectDrawParams)));
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("YellowCircleVertexAndIndexBufferOffset", "Testing that a vertex buffer offset with an index buffer offset works", std::move(frameBufferCapture), cobalt::graphics::IImageDiff::Algorithm::BinaryCountExact | cobalt::graphics::IImageDiff::Algorithm::RegionRanges);

	// Ensure that an index value offset takes effect
	// Equivalent to renderableNode->SetVertexCount(indexAttribute.GetIndexCount() - 20, 7, 10, -4);
	indexedIndirectDrawParams.indexCount = (uint32_t)indexAttribute.GetIndexCount() - 20;
	indexedIndirectDrawParams.firstIndex = 10;
	indexedIndirectDrawParams.vertexOffset = 7 - 4;
	REQUIRE(dataArray->QueueDataUpdate(&indexedIndirectDrawParams, sizeof(IRenderableNode::IndexedIndirectDrawParams)));
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("YellowCircleIndexValueOffset", "Testing that an index value offset works", std::move(frameBufferCapture), cobalt::graphics::IImageDiff::Algorithm::BinaryCountExact | cobalt::graphics::IImageDiff::Algorithm::RegionRanges);

	// Remove the renderable node
	groupNode->RemoveChildNode(renderableNode.get());

	// Convert the vertex data to a non-indexed form
	positionVertexData.push_back(*positionVertexData.begin());

	// Create our vertex buffer and populate it with data
	vertexAttributePosition = VertexAttribute<V3Float32>(positionVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	vertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttributePosition));
	REQUIRE(vertexAttributePosition.SetInitialData(positionVertexData));
	REQUIRE(vertexBuffer->AllocateMemory());

	// Create our renderable node
	renderableNode = renderer.CreateRenderableNode();
	REQUIRE(renderableNode->BindVertexAttribute(vertexAttributePosition, positionAttributeId));
	REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::LineStrip));

	// Enable indirect drawing for this renderable
	REQUIRE(renderableNode->SetIndirectDraw(1, dataArray.get()));

	// Add our renderable node to the scene
	groupNode->AddChildNode(renderableNode.get());

	// Initialize our indirect draw params
	IRenderableNode::IndirectDrawParams indirectDrawParams{};
	indirectDrawParams.vertexCount = 0;
	indirectDrawParams.instanceCount = 1;
	indirectDrawParams.firstVertex = 0;
	indirectDrawParams.firstInstance = 0;

	// Ensure the complete circle is drawn by default
	// Equivalent to renderableNode->SetVertexCount(vertexAttributePosition.GetVertexCount(), 0, 0, 0);
	indirectDrawParams.vertexCount = (uint32_t)vertexAttributePosition.GetVertexCount();
	indirectDrawParams.firstVertex = 0;
	REQUIRE(dataArray->QueueDataUpdate(&indirectDrawParams, sizeof(IRenderableNode::IndirectDrawParams)));
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("VertexYellowCircleComplete", "A complete yellow circle", std::move(frameBufferCapture), cobalt::graphics::IImageDiff::Algorithm::BinaryCountExact | cobalt::graphics::IImageDiff::Algorithm::RegionRanges);

	// Ensure that a reduced vertex count takes effect
	// Equivalent to renderableNode->SetVertexCount(vertexAttributePosition.GetVertexCount() - 10, 0, 0, 0);
	indirectDrawParams.vertexCount = (uint32_t)vertexAttributePosition.GetVertexCount() - 10;
	indirectDrawParams.firstVertex = 0;
	REQUIRE(dataArray->QueueDataUpdate(&indirectDrawParams, sizeof(IRenderableNode::IndirectDrawParams)));
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("VertexYellowCircleReducedVertexCount", "Testing that a reduced vertex count works", std::move(frameBufferCapture), cobalt::graphics::IImageDiff::Algorithm::BinaryCountExact | cobalt::graphics::IImageDiff::Algorithm::RegionRanges);

	// Ensure that an index buffer offset takes effect
	// Equivalent to renderableNode->SetVertexCount(vertexAttributePosition.GetVertexCount() - 10, 0, 6, 0);
	indirectDrawParams.vertexCount = (uint32_t)vertexAttributePosition.GetVertexCount() - 10;
	indirectDrawParams.firstVertex = 6;
	REQUIRE(dataArray->QueueDataUpdate(&indirectDrawParams, sizeof(IRenderableNode::IndirectDrawParams)));
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("VertexYellowCircleIndexBufferOffset", "Testing that a vertex buffer offset works", std::move(frameBufferCapture), cobalt::graphics::IImageDiff::Algorithm::BinaryCountExact | cobalt::graphics::IImageDiff::Algorithm::RegionRanges);

	// ----------------------------

	// Now try getting the count from the same buffer as the values, all sent in one packed buffer onto
	// the GPU (as per MDF-style batching).
	struct IndirectDrawTempBuffer
	{
		uint32_t myActiveCount = 10;
		std::array<IRenderableNode::IndirectDrawParams, 10> myElements = {};
	};

	IndirectDrawTempBuffer complexDrawBuffer;

	for (auto i = 0; i < 10; ++i)
	{
		auto& e = complexDrawBuffer.myElements[i];
		e.firstInstance = 0;
		e.firstVertex = i * 4;
		e.vertexCount = 2;
		e.instanceCount = 1;
	}

	dataArray = renderer.CreateDataArray();
	dataArray->SetUsageFlags(IDataArray::UsageFlags::IndirectDrawSource | IDataArray::UsageFlags::IndirectDrawCountSource);
	dataArray->SetBufferLayout(sizeof(IndirectDrawTempBuffer), 1);
	REQUIRE(dataArray->AllocateMemory());
	REQUIRE(dataArray->QueueDataUpdate(&complexDrawBuffer, sizeof(IndirectDrawTempBuffer)));
	REQUIRE(renderableNode->SetIndirectDraw(10, dataArray.get(), 0, dataArray.get(), 4));

	auto screenshot = [&](const char* Name, const char* Desc) {
		frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult(Name, Desc, std::move(frameBufferCapture), cobalt::graphics::IImageDiff::Algorithm::BinaryCountExact | cobalt::graphics::IImageDiff::Algorithm::RegionRanges);
	};

	screenshot("All10", "10 line segments");

	complexDrawBuffer.myActiveCount = 9;
	REQUIRE(dataArray->QueueDataUpdate(&complexDrawBuffer, sizeof(IndirectDrawTempBuffer)));
	screenshot("Partial9-1", "9 line segments, one missing (we decremented the counter)");

	std::swap(complexDrawBuffer.myElements[4], complexDrawBuffer.myElements.back());
	REQUIRE(dataArray->QueueDataUpdate(&complexDrawBuffer, sizeof(IndirectDrawTempBuffer)));
	screenshot("Partial9-2", "A different one (bottom right) is missing - (we swapped 1 entry with end)");

	complexDrawBuffer.myActiveCount = 0;
	REQUIRE(dataArray->QueueDataUpdate(&complexDrawBuffer, sizeof(IndirectDrawTempBuffer)));
	screenshot("None", "We set the counter to 0 active line segments - without removing the renderable node");

	// Remove the renderable node
	groupNode->RemoveChildNode(renderableNode.get());

	// Remove all our defined render passes
	renderer.RemoveAllRenderPasses();
	return true;
}

} // namespace cobalt::graphics::testing
