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
    return float4(position, 1.0f);
}
)";
const std::string FragmentShader = R"(
float4 main() : SV_TARGET0
{
    return float4(0.0f, 1.0f, 0.0f, 1.0f);
}
)";

struct PaddedPositionUpdate
{
	V3Float32 position;
	float padding;
};
struct PaddedIndexUpdate
{
	V1UInt32 index;
	V1UInt32 padding;
};

} // namespace

DEFINE_UNIT_TEST_WITH_BASE("Resources/VertexBuffer/DynamicGeometryUpdates", UnitTestBase)
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
	auto positionAttributeId = shaderProgram->GetVertexAttributeId("position");

	// Define a visible quad, a moved quad, and an index pattern that degenerates the quad to nothing.
	std::vector<V3Float32> leftQuadPositions = {
	  {-0.95f, 0.75f, 0.0f},
	  {-0.05f, 0.75f, 0.0f},
	  {-0.95f, -0.75f, 0.0f},
	  {-0.05f, -0.75f, 0.0f},
	};
	std::vector<PaddedPositionUpdate> rightQuadPositionUpdates = {
	  {{0.05f, 0.75f, 0.0f}, 100.0f},
	  {{0.95f, 0.75f, 0.0f}, 101.0f},
	  {{0.05f, -0.75f, 0.0f}, 102.0f},
	  {{0.95f, -0.75f, 0.0f}, 103.0f},
	};
	std::vector<V1UInt32> visibleIndices = {{0}, {1}, {2}, {2}, {1}, {3}};
	std::vector<PaddedIndexUpdate> degenerateIndexUpdates = {
	  {{0}, {100}},
	  {{0}, {101}},
	  {{0}, {102}},
	  {{0}, {103}},
	  {{0}, {104}},
	  {{0}, {105}},
	};

	// Create dynamic geometry buffers where the renderer is allowed to discard the old contents on write.
	VertexAttribute<V3Float32> dynamicPositionAttribute(leftQuadPositions.size(), IVertexAttribute::PerformanceHint::WriteOften | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften, IVertexAttribute::DataPersistenceFlags::InvalidateExistingDataOnWrite);
	auto dynamicVertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE(dynamicVertexBuffer->BindVertexAttribute(dynamicPositionAttribute));
	REQUIRE(dynamicPositionAttribute.SetInitialData(leftQuadPositions));
	REQUIRE(dynamicVertexBuffer->AllocateMemory());

	IndexAttribute<V1UInt32> dynamicIndexAttribute(visibleIndices.size(), IIndexAttribute::PerformanceHint::WriteOften | IIndexAttribute::PerformanceHint::ReadNever, IIndexAttribute::PerformanceHint::WriteNever | IIndexAttribute::PerformanceHint::ReadOften, IIndexAttribute::DataPersistenceFlags::InvalidateExistingDataOnWrite);
	auto dynamicIndexBuffer = renderer.CreateIndexBuffer();
	REQUIRE(dynamicIndexBuffer->BindIndexAttribute(dynamicIndexAttribute));
	REQUIRE(dynamicIndexAttribute.SetInitialData(visibleIndices));
	REQUIRE(dynamicIndexBuffer->AllocateMemory());

	// Create our renderable node
	auto renderableNode = renderer.CreateRenderableNode();
	REQUIRE(renderableNode->BindVertexAttribute(dynamicPositionAttribute, positionAttributeId));
	REQUIRE(renderableNode->BindIndexAttribute(dynamicIndexAttribute));
	REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));

	// Create our render tree
	auto groupNode = renderer.CreateStateGroupNode();
	groupNode->AddChildNode(renderableNode.get());
	auto programNode = renderer.CreateProgramNode();
	REQUIRE(programNode->BindShaderProgram(shaderProgram.get()));
	programNode->AddChildNode(groupNode.get());
	auto renderPassNode = renderer.CreateRenderPassNode();
	renderPassNode->BindFrameBuffer(mainWindowFrameBuffer.get());
	renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
	renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Depth, 0, V4Float32(1.0f, 0.0f, 0.0f, 0.0f));
	renderPassNode->AddChildNode(programNode.get());
	renderer.SetRenderPasses(&renderPassNode, 1);

	// Draw the initial dynamic geometry before performing any updates.
	auto frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("DynamicInitialLeftQuad", "A green quad on the left before any dynamic geometry updates have been submitted.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Update the dynamic vertex buffer from padded source data, proving non-tightly-packed update sources are handled.
	REQUIRE(dynamicPositionAttribute.QueueDataUpdate(&rightQuadPositionUpdates[0].position, rightQuadPositionUpdates.size(), 0, sizeof(PaddedPositionUpdate)));
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("DynamicStridedVertexUpdate", "A green quad on the right after a strided dynamic vertex-buffer update.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Update the dynamic index buffer from padded source data, proving strided index updates are handled as well.
	REQUIRE(dynamicIndexAttribute.QueueDataUpdate(&degenerateIndexUpdates[0].index, degenerateIndexUpdates.size(), 0, sizeof(PaddedIndexUpdate)));
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("DynamicStridedIndexUpdate", "A black frame after a strided dynamic index-buffer update degenerates all triangles.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);
	session.AddTestSuccess("DynamicGeometryStridedUpdates", "Dynamic vertex and index buffers accepted strided update source data and flushed those pending writes during rendering.");

	// Replace the renderable with persistent geometry buffers, so the renderer also exercises updates that must
	// preserve existing buffer contents.
	groupNode->RemoveChildNode(renderableNode.get());
	VertexAttribute<V3Float32> persistentPositionAttribute(leftQuadPositions.size(), IVertexAttribute::PerformanceHint::WriteOften | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	auto persistentVertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE(persistentVertexBuffer->BindVertexAttribute(persistentPositionAttribute));
	REQUIRE(persistentPositionAttribute.SetInitialData(leftQuadPositions));
	REQUIRE(persistentVertexBuffer->AllocateMemory());

	IndexAttribute<V1UInt32> persistentIndexAttribute(visibleIndices.size(), IIndexAttribute::PerformanceHint::WriteOften | IIndexAttribute::PerformanceHint::ReadNever, IIndexAttribute::PerformanceHint::WriteNever | IIndexAttribute::PerformanceHint::ReadOften);
	auto persistentIndexBuffer = renderer.CreateIndexBuffer();
	REQUIRE(persistentIndexBuffer->BindIndexAttribute(persistentIndexAttribute));
	REQUIRE(persistentIndexAttribute.SetInitialData(visibleIndices));
	REQUIRE(persistentIndexBuffer->AllocateMemory());

	renderableNode = renderer.CreateRenderableNode();
	REQUIRE(renderableNode->BindVertexAttribute(persistentPositionAttribute, positionAttributeId));
	REQUIRE(renderableNode->BindIndexAttribute(persistentIndexAttribute));
	REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
	groupNode->AddChildNode(renderableNode.get());
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("PersistentInitialLeftQuad", "A green quad on the left before any persistent geometry updates have been submitted.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Queue strided updates on the persistent buffers and draw frames to flush those pending writes.
	REQUIRE(persistentPositionAttribute.QueueDataUpdate(&rightQuadPositionUpdates[0].position, rightQuadPositionUpdates.size(), 0, sizeof(PaddedPositionUpdate)));
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("PersistentStridedVertexUpdate", "A green quad on the right after a strided persistent vertex-buffer update.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);
	REQUIRE(persistentIndexAttribute.QueueDataUpdate(&degenerateIndexUpdates[0].index, degenerateIndexUpdates.size(), 0, sizeof(PaddedIndexUpdate)));
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("PersistentStridedIndexUpdate", "A black frame after a strided persistent index-buffer update degenerates all triangles.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);
	session.AddTestSuccess("PersistentGeometryStridedUpdates", "Persistent vertex and index buffers accepted strided update source data and flushed those pending writes during rendering.");

	// Remove all our defined render passes
	groupNode->RemoveAllChildNodes();
	renderer.RemoveAllRenderPasses();
	return true;
}

} // namespace cobalt::graphics::testing
