// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../UnitTestBase.h"
#include <array>
namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

namespace {

const std::string VertexShader = R"(
struct VSInput {
    float4 position : position;
};

struct VSOutput {
    float4 position : SV_POSITION;
};

VSOutput main(VSInput IN)
{
    VSOutput OUT;
    OUT.position = IN.position;
    return OUT;
}
)";
const std::string FragmentShader = R"(
uniform float4 drawColor;

float4 main() : SV_TARGET0
{
    return drawColor;
}
)";

} // namespace

DEFINE_UNIT_TEST_WITH_BASE("Scene/RenderPassSortOrder", UnitTestBase)
{
	// Retrieve our important objects
	auto& renderer = session.Renderer();
	auto& uiThread = session.UIThread();
	auto& testWindowInfo = *session.TestWindowPlatformInfo();

	// Define the framebuffer
	auto frameBuffer = renderer.CreateFrameBuffer();
	frameBuffer->DefineViewportRegion(V2UInt32(0, 0), session.TestWindowSize());
	REQUIRE(uiThread.InvokeSync([&] { return frameBuffer->BindWindow(testWindowInfo, IFrameBuffer::WindowDepthStencilMode::None); }));

	// Create and compile our shader program
	auto shaderProgram = renderer.CreateShaderProgram();
	REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(VertexShader)));
	REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(FragmentShader)));
	REQUIRE(shaderProgram->CompileProgram());

	// Retrieve our shader attribute and state IDs
	auto positionAttributeId = shaderProgram->GetVertexAttributeId("position");
	auto drawColorStateId = shaderProgram->GetStateValueId("drawColor");

	// Create the geometry used by the base and overlay render passes.
	std::vector<V4Float32> fullscreenVertexData;
	std::vector<V4Float32> centeredVertexData;
	Geometry().CreateFullscreenQuad(0.0f, fullscreenVertexData);
	Geometry().CreateCenteredQuad(0.0f, 0.5f, centeredVertexData);

	VertexAttribute<V4Float32> fullscreenVertexAttribute(fullscreenVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	VertexAttribute<V4Float32> centeredVertexAttribute(centeredVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);

	auto fullscreenVertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE(fullscreenVertexBuffer->BindVertexAttribute(fullscreenVertexAttribute));
	REQUIRE(fullscreenVertexAttribute.SetInitialData(fullscreenVertexData));
	REQUIRE(fullscreenVertexBuffer->AllocateMemory());

	auto centeredVertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE(centeredVertexBuffer->BindVertexAttribute(centeredVertexAttribute));
	REQUIRE(centeredVertexAttribute.SetInitialData(centeredVertexData));
	REQUIRE(centeredVertexBuffer->AllocateMemory());

	// Verify that the raw pointer overload orders passes by the supplied sort array instead of the array order.
	{
		auto baseRenderableNode = renderer.CreateRenderableNode();
		REQUIRE(baseRenderableNode->BindVertexAttribute(fullscreenVertexAttribute, positionAttributeId));
		REQUIRE(baseRenderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
		baseRenderableNode->SetStateValue(drawColorStateId, V4Float32(1.0f, 0.0f, 0.0f, 1.0f));

		auto baseGroupNode = renderer.CreateStateGroupNode();
		baseGroupNode->SetDepthTestEnabled(false);
		baseGroupNode->SetDepthWriteEnabled(false);
		baseGroupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);
		baseGroupNode->AddChildNode(baseRenderableNode.get());

		auto baseProgramNode = renderer.CreateProgramNode();
		REQUIRE(baseProgramNode->BindShaderProgram(shaderProgram.get()));
		baseProgramNode->AddChildNode(baseGroupNode.get());

		auto overlayRenderableNode = renderer.CreateRenderableNode();
		REQUIRE(overlayRenderableNode->BindVertexAttribute(centeredVertexAttribute, positionAttributeId));
		REQUIRE(overlayRenderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
		overlayRenderableNode->SetStateValue(drawColorStateId, V4Float32(0.0f, 1.0f, 0.0f, 1.0f));

		auto overlayGroupNode = renderer.CreateStateGroupNode();
		overlayGroupNode->SetDepthTestEnabled(false);
		overlayGroupNode->SetDepthWriteEnabled(false);
		overlayGroupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);
		overlayGroupNode->AddChildNode(overlayRenderableNode.get());

		auto overlayProgramNode = renderer.CreateProgramNode();
		REQUIRE(overlayProgramNode->BindShaderProgram(shaderProgram.get()));
		overlayProgramNode->AddChildNode(overlayGroupNode.get());

		auto basePass = renderer.CreateRenderPassNode();
		basePass->BindFrameBuffer(frameBuffer.get());
		basePass->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
		basePass->AddChildNode(baseProgramNode.get());

		auto overlayPass = renderer.CreateRenderPassNode();
		overlayPass->BindFrameBuffer(frameBuffer.get());
		overlayPass->SetAttachmentLoadStoreBehavior(IFrameBuffer::AttachmentType::Color, 0, IRenderPassNode::AttachmentLoadBehavior::LoadExistingData, IRenderPassNode::AttachmentStoreBehavior::StoreFinalData);
		overlayPass->AddChildNode(overlayProgramNode.get());

		IRenderPassNode* passes[] = {overlayPass.get(), basePass.get()};
		const int32_t sortOrder[] = {1, 0};
		renderer.SetRenderPasses(&passes[0], 2, &sortOrder[0]);

		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("RawPointerSortOrder", "A green centered quad over a red background, proving the supplied raw-pointer sort order was used.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);
		renderer.RemoveAllRenderPasses();
	}

	// Verify that the unique pointer overload applies the same sorted ordering rules.
	{
		auto baseRenderableNode = renderer.CreateRenderableNode();
		REQUIRE(baseRenderableNode->BindVertexAttribute(fullscreenVertexAttribute, positionAttributeId));
		REQUIRE(baseRenderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
		baseRenderableNode->SetStateValue(drawColorStateId, V4Float32(1.0f, 0.0f, 0.0f, 1.0f));

		auto baseGroupNode = renderer.CreateStateGroupNode();
		baseGroupNode->SetDepthTestEnabled(false);
		baseGroupNode->SetDepthWriteEnabled(false);
		baseGroupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);
		baseGroupNode->AddChildNode(baseRenderableNode.get());

		auto baseProgramNode = renderer.CreateProgramNode();
		REQUIRE(baseProgramNode->BindShaderProgram(shaderProgram.get()));
		baseProgramNode->AddChildNode(baseGroupNode.get());

		auto overlayRenderableNode = renderer.CreateRenderableNode();
		REQUIRE(overlayRenderableNode->BindVertexAttribute(centeredVertexAttribute, positionAttributeId));
		REQUIRE(overlayRenderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
		overlayRenderableNode->SetStateValue(drawColorStateId, V4Float32(0.0f, 0.0f, 1.0f, 1.0f));

		auto overlayGroupNode = renderer.CreateStateGroupNode();
		overlayGroupNode->SetDepthTestEnabled(false);
		overlayGroupNode->SetDepthWriteEnabled(false);
		overlayGroupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);
		overlayGroupNode->AddChildNode(overlayRenderableNode.get());

		auto overlayProgramNode = renderer.CreateProgramNode();
		REQUIRE(overlayProgramNode->BindShaderProgram(shaderProgram.get()));
		overlayProgramNode->AddChildNode(overlayGroupNode.get());

		std::array<IRenderPassNode::unique_ptr, 2> passes = {renderer.CreateRenderPassNode(), renderer.CreateRenderPassNode()};
		passes[0]->BindFrameBuffer(frameBuffer.get());
		passes[0]->SetAttachmentLoadStoreBehavior(IFrameBuffer::AttachmentType::Color, 0, IRenderPassNode::AttachmentLoadBehavior::LoadExistingData, IRenderPassNode::AttachmentStoreBehavior::StoreFinalData);
		passes[0]->AddChildNode(overlayProgramNode.get());

		passes[1]->BindFrameBuffer(frameBuffer.get());
		passes[1]->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
		passes[1]->AddChildNode(baseProgramNode.get());

		const int32_t sortOrder[] = {1, 0};
		renderer.SetRenderPasses(passes.data(), passes.size(), &sortOrder[0]);

		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("UniquePointerSortOrder", "A blue centered quad over a red background, proving the supplied unique-pointer sort order was used.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);
		renderer.RemoveAllRenderPasses();
	}

	return true;
}

} // namespace cobalt::graphics::testing
