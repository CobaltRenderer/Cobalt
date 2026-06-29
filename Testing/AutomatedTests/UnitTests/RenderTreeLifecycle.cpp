// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../UnitTestBase.h"
#include <array>
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
uniform float4 drawColor;

float4 main() : SV_TARGET0
{
    return drawColor;
}
)";

std::vector<V4Float32> CreateQuad(float minX, float maxX)
{
	return {
	  V4Float32(minX, 0.75f, 0.5f, 1.0f),
	  V4Float32(minX, -0.75f, 0.5f, 1.0f),
	  V4Float32(maxX, -0.75f, 0.5f, 1.0f),
	  V4Float32(minX, 0.75f, 0.5f, 1.0f),
	  V4Float32(maxX, -0.75f, 0.5f, 1.0f),
	  V4Float32(maxX, 0.75f, 0.5f, 1.0f),
	};
}

} // namespace

DEFINE_UNIT_TEST_WITH_BASE("Scene/RenderTreeLifecycle", UnitTestBase)
{
	// Retrieve our important objects
	auto& renderer = session.Renderer();
	auto& uiThread = session.UIThread();
	auto& testWindowInfo = *session.TestWindowPlatformInfo();

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

	// Create our vertex buffer and populate it with three independent quads
	auto leftQuadData = CreateQuad(-0.95f, -0.35f);
	auto centerQuadData = CreateQuad(-0.30f, 0.30f);
	auto rightQuadData = CreateQuad(0.35f, 0.95f);
	VertexAttribute<V4Float32> leftQuadAttribute(leftQuadData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	VertexAttribute<V4Float32> centerQuadAttribute(centerQuadData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	VertexAttribute<V4Float32> rightQuadAttribute(rightQuadData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	auto vertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE(vertexBuffer->BindVertexAttribute(leftQuadAttribute));
	REQUIRE(leftQuadAttribute.SetInitialData(leftQuadData));
	REQUIRE(vertexBuffer->BindVertexAttribute(centerQuadAttribute));
	REQUIRE(centerQuadAttribute.SetInitialData(centerQuadData));
	REQUIRE(vertexBuffer->BindVertexAttribute(rightQuadAttribute));
	REQUIRE(rightQuadAttribute.SetInitialData(rightQuadData));
	REQUIRE(vertexBuffer->AllocateMemory());

	// Create our renderable nodes
	auto leftRenderableNode = renderer.CreateRenderableNode();
	REQUIRE(leftRenderableNode->BindVertexAttribute(leftQuadAttribute, positionAttributeId));
	REQUIRE(leftRenderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
	leftRenderableNode->SetStateValue(drawColorStateId, V4Float32(1.0f, 0.0f, 0.0f, 1.0f));

	auto centerRenderableNode = renderer.CreateRenderableNode();
	REQUIRE(centerRenderableNode->BindVertexAttribute(centerQuadAttribute, positionAttributeId));
	REQUIRE(centerRenderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
	centerRenderableNode->SetStateValue(drawColorStateId, V4Float32(0.0f, 0.0f, 1.0f, 1.0f));

	auto alternateCenterRenderableNode = renderer.CreateRenderableNode();
	REQUIRE(alternateCenterRenderableNode->BindVertexAttribute(centerQuadAttribute, positionAttributeId));
	REQUIRE(alternateCenterRenderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
	alternateCenterRenderableNode->SetStateValue(drawColorStateId, V4Float32(1.0f, 1.0f, 0.0f, 1.0f));

	auto rightRenderableNode = renderer.CreateRenderableNode();
	REQUIRE(rightRenderableNode->BindVertexAttribute(rightQuadAttribute, positionAttributeId));
	REQUIRE(rightRenderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
	rightRenderableNode->SetStateValue(drawColorStateId, V4Float32(0.0f, 1.0f, 0.0f, 1.0f));

	// Exercise unique_ptr array overloads for each render-tree child list type.
	{
		std::array<IRenderableNode::unique_ptr, 2> uniqueRenderableNodes = {renderer.CreateRenderableNode(), renderer.CreateRenderableNode()};
		REQUIRE(uniqueRenderableNodes[0]->BindVertexAttribute(leftQuadAttribute, positionAttributeId));
		REQUIRE(uniqueRenderableNodes[0]->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
		REQUIRE(uniqueRenderableNodes[1]->BindVertexAttribute(rightQuadAttribute, positionAttributeId));
		REQUIRE(uniqueRenderableNodes[1]->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));

		auto uniqueStateGroupNode = renderer.CreateStateGroupNode();
		uniqueStateGroupNode->AddChildNodes(uniqueRenderableNodes.data(), uniqueRenderableNodes.size());
		uniqueStateGroupNode->RemoveChildNodes(uniqueRenderableNodes.data(), 1);
		uniqueStateGroupNode->SetChildNodes(uniqueRenderableNodes.data() + 1, 1);
		uniqueStateGroupNode->RemoveAllChildNodes();
		uniqueStateGroupNode->AddChildNode(uniqueRenderableNodes[0].get());
		uniqueStateGroupNode->RemoveChildNode(uniqueRenderableNodes[0].get());

		std::array<IStateGroupNode::unique_ptr, 2> uniqueStateGroupNodes = {renderer.CreateStateGroupNode(), renderer.CreateStateGroupNode()};
		auto uniqueProgramNode = renderer.CreateProgramNode();
		REQUIRE(uniqueProgramNode->BindShaderProgram(shaderProgram.get()));
		uniqueProgramNode->AddChildNodes(uniqueStateGroupNodes.data(), uniqueStateGroupNodes.size());
		uniqueProgramNode->RemoveChildNodes(uniqueStateGroupNodes.data(), 1);
		uniqueProgramNode->SetChildNodes(uniqueStateGroupNodes.data() + 1, 1);
		uniqueProgramNode->RemoveAllChildNodes();
		uniqueProgramNode->AddChildNode(uniqueStateGroupNodes[0].get());
		uniqueProgramNode->RemoveChildNode(uniqueStateGroupNodes[0].get());

		std::array<IProgramNode::unique_ptr, 2> uniqueProgramNodes = {renderer.CreateProgramNode(), renderer.CreateProgramNode()};
		std::array<IDefaultState::unique_ptr, 2> uniqueDefaultStates = {renderer.CreateDefaultState(), renderer.CreateDefaultState()};
		REQUIRE(uniqueProgramNodes[0]->BindShaderProgram(shaderProgram.get()));
		REQUIRE(uniqueProgramNodes[1]->BindShaderProgram(shaderProgram.get()));
		auto uniqueRenderPassNode = renderer.CreateRenderPassNode();
		uniqueRenderPassNode->BindFrameBuffer(frameBuffer.get());
		uniqueRenderPassNode->AddChildNodes(uniqueProgramNodes.data(), uniqueProgramNodes.size(), uniqueDefaultStates.data());
		uniqueRenderPassNode->RemoveChildNodes(uniqueProgramNodes.data(), 1);
		uniqueRenderPassNode->SetChildNodes(uniqueProgramNodes.data() + 1, 1, uniqueDefaultStates.data() + 1);
		uniqueRenderPassNode->RemoveAllChildNodes();
		uniqueRenderPassNode->AddChildNode(uniqueProgramNodes[0].get(), uniqueDefaultStates[0].get());
		uniqueRenderPassNode->RemoveChildNode(uniqueProgramNodes[0].get());
		session.AddTestSuccess("UniquePtrChildOverloads", "Render-tree child APIs accepted unique_ptr array overloads and single-child removal for renderable, state-group, and program nodes.");
	}

	// Create our state group nodes
	auto bulkGroupNode = renderer.CreateStateGroupNode();
	bulkGroupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);
	IRenderableNode* initialRenderableNodes[] = {leftRenderableNode.get(), rightRenderableNode.get()};
	bulkGroupNode->AddChildNodes(&initialRenderableNodes[0], 2);

	auto centerGroupNode = renderer.CreateStateGroupNode();
	centerGroupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);
	centerGroupNode->AddChildNode(alternateCenterRenderableNode.get());

	// Create our program node
	auto programNode = renderer.CreateProgramNode();
	REQUIRE(programNode->BindShaderProgram(shaderProgram.get()));
	IStateGroupNode* initialGroupNodes[] = {bulkGroupNode.get()};
	programNode->AddChildNodes(&initialGroupNodes[0], 1);

	// Create our render pass node
	auto renderPassNode = renderer.CreateRenderPassNode();
	renderPassNode->BindFrameBuffer(frameBuffer.get());
	renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
	IProgramNode* initialProgramNodes[] = {programNode.get()};
	renderPassNode->SetChildNodes(&initialProgramNodes[0], 1);

	// Bind our render tree to the renderer
	renderer.SetRenderPasses(&renderPassNode, 1);

	// The initial bulk-added renderables should draw on the left and right.
	auto frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("BulkRenderableChildren", "A red quad on the left and a green quad on the right after adding renderable children in bulk.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Removing one child from the state group should remove only that renderable.
	IRenderableNode* removedRenderableNodes[] = {rightRenderableNode.get()};
	bulkGroupNode->RemoveChildNodes(&removedRenderableNodes[0], 1);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("RemovedRenderableChild", "A red quad on the left with the right side clear after removing one renderable child.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Replacing state group children should leave only the new center renderable.
	IRenderableNode* replacementRenderableNodes[] = {centerRenderableNode.get()};
	bulkGroupNode->SetChildNodes(&replacementRenderableNodes[0], 1);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("ReplacedRenderableChildren", "A blue quad in the center after replacing the state-group renderable children.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Replacing program-node children should switch to the alternate state group and a distinct renderable.
	IStateGroupNode* replacementGroupNodes[] = {centerGroupNode.get()};
	programNode->SetChildNodes(&replacementGroupNodes[0], 1);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("ReplacedProgramChildren", "A yellow quad in the center after replacing the program-node state-group children.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Removing the program from the render pass should leave only the clear color.
	IProgramNode* removedProgramNodes[] = {programNode.get()};
	renderPassNode->RemoveChildNodes(&removedProgramNodes[0], 1);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("RemovedRenderPassProgram", "A fully black image after removing the program node from the render pass.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Adding the program back in bulk should restore drawing.
	renderPassNode->AddChildNodes(&initialProgramNodes[0], 1);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("AddedProgramChildNodes", "A yellow quad in the center after adding the program node back to the render pass in bulk.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// A disabled render pass should not execute or capture output.
	renderPassNode->SetIsEnabled(false);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	REQUIRE(!frameBufferCapture->HasCapturedOutput());
	auto disabledFrameBufferCapture = std::move(frameBufferCapture);
	frameBuffer->RemoveOutputCaptureTarget(disabledFrameBufferCapture.get());

	// Re-enabling the render pass should restore output capture.
	renderPassNode->SetIsEnabled(true);
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	REQUIRE(frameBufferCapture->HasCapturedOutput());
	disabledFrameBufferCapture.reset();
	session.AddTestImageResult("ReEnabledRenderPass", "A yellow quad in the center after re-enabling the render pass.", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Remove all our defined render passes
	renderer.RemoveAllRenderPasses();
	return true;
}

} // namespace cobalt::graphics::testing
