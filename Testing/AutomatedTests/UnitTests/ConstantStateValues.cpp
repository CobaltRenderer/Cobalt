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
uniform float4 drawColor;

float4 main() : SV_TARGET0
{
    return drawColor;
}
)";
const std::string StateBufferFragmentShader = R"(
cbuffer ColorCBuffer
{
    float4 drawColor;
};

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

DEFINE_UNIT_TEST_WITH_BASE("Resources/StateValue/ConstantStateValues", UnitTestBase)
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

	// Create a fullscreen quad used by the focused image checks for the broad storage contracts.
	std::vector<V4Float32> fullscreenQuadData;
	Geometry().CreateFullscreenQuad(0.5f, fullscreenQuadData);
	VertexAttribute<V4Float32> fullscreenQuadAttribute(fullscreenQuadData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	auto fullscreenVertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE(fullscreenVertexBuffer->BindVertexAttribute(fullscreenQuadAttribute));
	REQUIRE(fullscreenQuadAttribute.SetInitialData(fullscreenQuadData));
	REQUIRE(fullscreenVertexBuffer->AllocateMemory());

	// Exercise program-node constant state storage for every supported value type. The scratch program node is not
	// added to the render tree, so these deliberately varied value types test storage and replacement without asking
	// the shader to consume a mismatched value.
	auto constantStorageProgramNode = renderer.CreateProgramNode();
	REQUIRE(constantStorageProgramNode->BindShaderProgram(shaderProgram.get()));
	M2Float32 matrix2 = {};
	M3Float32 matrix3 = {};
	M4Float32 matrix4 = {};
	for (size_t i = 0; i < matrix2.size(); ++i)
	{
		matrix2.data()[i] = (i % 3) == 0 ? 1.0f : 0.0f;
	}
	for (size_t i = 0; i < matrix3.size(); ++i)
	{
		matrix3.data()[i] = (i % 4) == 0 ? 1.0f : 0.0f;
	}
	for (size_t i = 0; i < matrix4.size(); ++i)
	{
		matrix4.data()[i] = (i % 5) == 0 ? 1.0f : 0.0f;
	}
	constantStorageProgramNode->SetConstantStateValue(drawColorStateId, true);
	constantStorageProgramNode->SetConstantStateValue(drawColorStateId, V1Int8(1));
	constantStorageProgramNode->SetConstantStateValue(drawColorStateId, V1Int16(1));
	constantStorageProgramNode->SetConstantStateValue(drawColorStateId, V1Int32(1));
	constantStorageProgramNode->SetConstantStateValue(drawColorStateId, V1UInt8(1));
	constantStorageProgramNode->SetConstantStateValue(drawColorStateId, V1UInt16(1));
	constantStorageProgramNode->SetConstantStateValue(drawColorStateId, V1UInt32(1));
	constantStorageProgramNode->SetConstantStateValue(drawColorStateId, V1Float32(1.0f));
	constantStorageProgramNode->SetConstantStateValue(drawColorStateId, V1Float64(1.0));
	constantStorageProgramNode->SetConstantStateValue(drawColorStateId, V2Int8(1, 2));
	constantStorageProgramNode->SetConstantStateValue(drawColorStateId, V2Int16(1, 2));
	constantStorageProgramNode->SetConstantStateValue(drawColorStateId, V2Int32(1, 2));
	constantStorageProgramNode->SetConstantStateValue(drawColorStateId, V2UInt8(1, 2));
	constantStorageProgramNode->SetConstantStateValue(drawColorStateId, V2UInt16(1, 2));
	constantStorageProgramNode->SetConstantStateValue(drawColorStateId, V2UInt32(1, 2));
	constantStorageProgramNode->SetConstantStateValue(drawColorStateId, V2Float32(1.0f, 2.0f));
	constantStorageProgramNode->SetConstantStateValue(drawColorStateId, V2Float64(1.0, 2.0));
	constantStorageProgramNode->SetConstantStateValue(drawColorStateId, V3Int8(1, 2, 3));
	constantStorageProgramNode->SetConstantStateValue(drawColorStateId, V3Int16(1, 2, 3));
	constantStorageProgramNode->SetConstantStateValue(drawColorStateId, V3Int32(1, 2, 3));
	constantStorageProgramNode->SetConstantStateValue(drawColorStateId, V3UInt8(1, 2, 3));
	constantStorageProgramNode->SetConstantStateValue(drawColorStateId, V3UInt16(1, 2, 3));
	constantStorageProgramNode->SetConstantStateValue(drawColorStateId, V3UInt32(1, 2, 3));
	constantStorageProgramNode->SetConstantStateValue(drawColorStateId, V3Float32(1.0f, 2.0f, 3.0f));
	constantStorageProgramNode->SetConstantStateValue(drawColorStateId, V3Float64(1.0, 2.0, 3.0));
	constantStorageProgramNode->SetConstantStateValue(drawColorStateId, V4Int8(1, 2, 3, 4));
	constantStorageProgramNode->SetConstantStateValue(drawColorStateId, V4Int16(1, 2, 3, 4));
	constantStorageProgramNode->SetConstantStateValue(drawColorStateId, V4Int32(1, 2, 3, 4));
	constantStorageProgramNode->SetConstantStateValue(drawColorStateId, V4UInt8(1, 2, 3, 4));
	constantStorageProgramNode->SetConstantStateValue(drawColorStateId, V4UInt16(1, 2, 3, 4));
	constantStorageProgramNode->SetConstantStateValue(drawColorStateId, V4UInt32(1, 2, 3, 4));
	constantStorageProgramNode->SetConstantStateValue(drawColorStateId, V4Float32(1.0f, 2.0f, 3.0f, 4.0f));
	constantStorageProgramNode->SetConstantStateValue(drawColorStateId, V4Float64(1.0, 2.0, 3.0, 4.0));
	constantStorageProgramNode->SetConstantStateValue(drawColorStateId, matrix2);
	constantStorageProgramNode->SetConstantStateValue(drawColorStateId, matrix3);
	constantStorageProgramNode->SetConstantStateValue(drawColorStateId, matrix4);
	const size_t arrayIndices[] = {0, 1};
	constantStorageProgramNode->SetConstantStateValue(drawColorStateId, V4Float32(0.25f, 0.50f, 0.75f, 1.0f), &arrayIndices[0], 2);
	constantStorageProgramNode->ResetConstantStateValue(drawColorStateId, &arrayIndices[0], 2);
	constantStorageProgramNode->ResetConstantStateValue(drawColorStateId);

	// Render one concrete program-node constant value after the broad storage pass so the contract has a reference image.
	{
		auto renderableNode = renderer.CreateRenderableNode();
		REQUIRE(renderableNode->BindVertexAttribute(fullscreenQuadAttribute, positionAttributeId));
		REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));

		auto groupNode = renderer.CreateStateGroupNode();
		groupNode->AddChildNode(renderableNode.get());

		auto programNode = renderer.CreateProgramNode();
		REQUIRE(programNode->BindShaderProgram(shaderProgram.get()));
		programNode->SetConstantStateValue(drawColorStateId, V4Float32(0.75f, 0.25f, 0.50f, 1.0f));
		programNode->AddChildNode(groupNode.get());

		auto renderPassNode = renderer.CreateRenderPassNode();
		renderPassNode->BindFrameBuffer(frameBuffer.get());
		renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
		renderPassNode->AddChildNode(programNode.get());
		renderer.SetRenderPasses(&renderPassNode, 1);

		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("ProgramNodeConstantValueTypeStorage", "A fullscreen purple color supplied by a program-node constant after exercising all constant value storage variants.", std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.98);
		renderer.RemoveAllRenderPasses();
	}

	// Exercise the shared state-container value storage path with the same value set.
	auto stateContainerStorage = renderer.CreateDefaultState();
	stateContainerStorage->SetStateValue(drawColorStateId, true);
	stateContainerStorage->SetStateValue(drawColorStateId, V1Int8(1));
	stateContainerStorage->SetStateValue(drawColorStateId, V1Int16(1));
	stateContainerStorage->SetStateValue(drawColorStateId, V1Int32(1));
	stateContainerStorage->SetStateValue(drawColorStateId, V1UInt8(1));
	stateContainerStorage->SetStateValue(drawColorStateId, V1UInt16(1));
	stateContainerStorage->SetStateValue(drawColorStateId, V1UInt32(1));
	stateContainerStorage->SetStateValue(drawColorStateId, V1Float32(1.0f));
	stateContainerStorage->SetStateValue(drawColorStateId, V1Float64(1.0));
	stateContainerStorage->SetStateValue(drawColorStateId, V2Int8(1, 2));
	stateContainerStorage->SetStateValue(drawColorStateId, V2Int16(1, 2));
	stateContainerStorage->SetStateValue(drawColorStateId, V2Int32(1, 2));
	stateContainerStorage->SetStateValue(drawColorStateId, V2UInt8(1, 2));
	stateContainerStorage->SetStateValue(drawColorStateId, V2UInt16(1, 2));
	stateContainerStorage->SetStateValue(drawColorStateId, V2UInt32(1, 2));
	stateContainerStorage->SetStateValue(drawColorStateId, V2Float32(1.0f, 2.0f));
	stateContainerStorage->SetStateValue(drawColorStateId, V2Float64(1.0, 2.0));
	stateContainerStorage->SetStateValue(drawColorStateId, V3Int8(1, 2, 3));
	stateContainerStorage->SetStateValue(drawColorStateId, V3Int16(1, 2, 3));
	stateContainerStorage->SetStateValue(drawColorStateId, V3Int32(1, 2, 3));
	stateContainerStorage->SetStateValue(drawColorStateId, V3UInt8(1, 2, 3));
	stateContainerStorage->SetStateValue(drawColorStateId, V3UInt16(1, 2, 3));
	stateContainerStorage->SetStateValue(drawColorStateId, V3UInt32(1, 2, 3));
	stateContainerStorage->SetStateValue(drawColorStateId, V3Float32(1.0f, 2.0f, 3.0f));
	stateContainerStorage->SetStateValue(drawColorStateId, V3Float64(1.0, 2.0, 3.0));
	stateContainerStorage->SetStateValue(drawColorStateId, V4Int8(1, 2, 3, 4));
	stateContainerStorage->SetStateValue(drawColorStateId, V4Int16(1, 2, 3, 4));
	stateContainerStorage->SetStateValue(drawColorStateId, V4Int32(1, 2, 3, 4));
	stateContainerStorage->SetStateValue(drawColorStateId, V4UInt8(1, 2, 3, 4));
	stateContainerStorage->SetStateValue(drawColorStateId, V4UInt16(1, 2, 3, 4));
	stateContainerStorage->SetStateValue(drawColorStateId, V4UInt32(1, 2, 3, 4));
	stateContainerStorage->SetStateValue(drawColorStateId, V4Float32(1.0f, 2.0f, 3.0f, 4.0f));
	stateContainerStorage->SetStateValue(drawColorStateId, V4Float64(1.0, 2.0, 3.0, 4.0));
	stateContainerStorage->SetStateValue(drawColorStateId, matrix2);
	stateContainerStorage->SetStateValue(drawColorStateId, matrix3);
	stateContainerStorage->SetStateValue(drawColorStateId, matrix4);
	stateContainerStorage->SetStateValue(drawColorStateId, V4Float32(0.25f, 0.50f, 0.75f, 1.0f), &arrayIndices[0], 2);
	stateContainerStorage->ResetStateValue(drawColorStateId, &arrayIndices[0], 2);
	stateContainerStorage->ResetStateValue(drawColorStateId);

	// Render one concrete default-state value after the broad state-container storage pass.
	{
		auto renderableNode = renderer.CreateRenderableNode();
		REQUIRE(renderableNode->BindVertexAttribute(fullscreenQuadAttribute, positionAttributeId));
		REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));

		auto groupNode = renderer.CreateStateGroupNode();
		groupNode->AddChildNode(renderableNode.get());

		auto programNode = renderer.CreateProgramNode();
		REQUIRE(programNode->BindShaderProgram(shaderProgram.get()));
		programNode->AddChildNode(groupNode.get());

		auto visibleDefaultState = renderer.CreateDefaultState();
		visibleDefaultState->SetStateValue(drawColorStateId, V4Float32(0.0f, 1.0f, 1.0f, 1.0f));

		auto renderPassNode = renderer.CreateRenderPassNode();
		renderPassNode->BindFrameBuffer(frameBuffer.get());
		renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
		renderPassNode->AddChildNode(programNode.get(), visibleDefaultState.get());
		renderer.SetRenderPasses(&renderPassNode, 1);

		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("StateContainerValueTypeStorage", "A fullscreen cyan color supplied by a default-state container after exercising all state value storage variants.", std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.98);
		renderer.RemoveAllRenderPasses();
	}

	// Exercise the state-buffer value storage path directly, using a wide matrix field so every value type can be
	// written safely without depending on a shader draw to consume the data.
	auto missingLayoutStateBuffer = renderer.CreateStateBuffer();
	REQUIRE(missingLayoutStateBuffer->GetStateValueId("wideValue") == StateValueId::Null);
	REQUIRE(!missingLayoutStateBuffer->AllocateMemory());

	auto stateBufferLayoutStorage = renderer.CreateStateBufferLayout();
	REQUIRE(stateBufferLayoutStorage->BeginLayoutDefinition());
	stateBufferLayoutStorage->AppendMatrix("wideValue", IStateBufferLayout::DataType::Float32, 4, 4);
	stateBufferLayoutStorage->AppendVector("arrayValue", IStateBufferLayout::DataType::Float32, 4, 3);
	REQUIRE(stateBufferLayoutStorage->ConstructStateLayout());

	auto manualPageStateBuffer = renderer.CreateStateBuffer();
	manualPageStateBuffer->SetManualPageSize(64);
	REQUIRE(!manualPageStateBuffer->BindBufferLayout(stateBufferLayoutStorage.get()));
	manualPageStateBuffer->SetPageSettings(1);
	REQUIRE(manualPageStateBuffer->AllocateMemory());
	REQUIRE(!manualPageStateBuffer->ResizePageCount(2));

	auto stateBufferStorage = renderer.CreateStateBuffer();
	REQUIRE(stateBufferStorage->BindBufferLayout(stateBufferLayoutStorage.get()));
	REQUIRE(!stateBufferStorage->BindBufferLayout(stateBufferLayoutStorage.get()));
	stateBufferStorage->SetPerformanceHints(IStateBuffer::PerformanceHint::WriteOften | IStateBuffer::PerformanceHint::ReadNever, IStateBuffer::PerformanceHint::WriteNever | IStateBuffer::PerformanceHint::ReadOften);
	stateBufferStorage->SetPageSettings(2, true);
	REQUIRE(stateBufferStorage->AllocateMemory());
	REQUIRE(!stateBufferStorage->AllocateMemory());
	REQUIRE(stateBufferStorage->ResizePageCount(3));

	auto wideValueStateId = stateBufferStorage->GetStateValueId("wideValue");
	auto arrayValueStateId = stateBufferStorage->GetStateValueId("arrayValue");
	REQUIRE(wideValueStateId != StateValueId::Null);
	REQUIRE(arrayValueStateId != StateValueId::Null);
	REQUIRE(stateBufferStorage->GetStateValueId("missingValue") == StateValueId::Null);
	stateBufferStorage->SetStateValue(wideValueStateId, true);
	stateBufferStorage->SetStateValue(wideValueStateId, V1Int8(1));
	stateBufferStorage->SetStateValue(wideValueStateId, V1Int16(1));
	stateBufferStorage->SetStateValue(wideValueStateId, V1Int32(1));
	stateBufferStorage->SetStateValue(wideValueStateId, V1UInt8(1));
	stateBufferStorage->SetStateValue(wideValueStateId, V1UInt16(1));
	stateBufferStorage->SetStateValue(wideValueStateId, V1UInt32(1));
	stateBufferStorage->SetStateValue(wideValueStateId, V1Float32(1.0f));
	stateBufferStorage->SetStateValue(wideValueStateId, V1Float64(1.0));
	stateBufferStorage->SetStateValue(wideValueStateId, V2Int8(1, 2));
	stateBufferStorage->SetStateValue(wideValueStateId, V2Int16(1, 2));
	stateBufferStorage->SetStateValue(wideValueStateId, V2Int32(1, 2));
	stateBufferStorage->SetStateValue(wideValueStateId, V2UInt8(1, 2));
	stateBufferStorage->SetStateValue(wideValueStateId, V2UInt16(1, 2));
	stateBufferStorage->SetStateValue(wideValueStateId, V2UInt32(1, 2));
	stateBufferStorage->SetStateValue(wideValueStateId, V2Float32(1.0f, 2.0f));
	stateBufferStorage->SetStateValue(wideValueStateId, V2Float64(1.0, 2.0));
	stateBufferStorage->SetStateValue(wideValueStateId, V3Int8(1, 2, 3));
	stateBufferStorage->SetStateValue(wideValueStateId, V3Int16(1, 2, 3));
	stateBufferStorage->SetStateValue(wideValueStateId, V3Int32(1, 2, 3));
	stateBufferStorage->SetStateValue(wideValueStateId, V3UInt8(1, 2, 3));
	stateBufferStorage->SetStateValue(wideValueStateId, V3UInt16(1, 2, 3));
	stateBufferStorage->SetStateValue(wideValueStateId, V3UInt32(1, 2, 3));
	stateBufferStorage->SetStateValue(wideValueStateId, V3Float32(1.0f, 2.0f, 3.0f));
	stateBufferStorage->SetStateValue(wideValueStateId, V3Float64(1.0, 2.0, 3.0));
	stateBufferStorage->SetStateValue(wideValueStateId, V4Int8(1, 2, 3, 4));
	stateBufferStorage->SetStateValue(wideValueStateId, V4Int16(1, 2, 3, 4));
	stateBufferStorage->SetStateValue(wideValueStateId, V4Int32(1, 2, 3, 4));
	stateBufferStorage->SetStateValue(wideValueStateId, V4UInt8(1, 2, 3, 4));
	stateBufferStorage->SetStateValue(wideValueStateId, V4UInt16(1, 2, 3, 4));
	stateBufferStorage->SetStateValue(wideValueStateId, V4UInt32(1, 2, 3, 4));
	stateBufferStorage->SetStateValue(wideValueStateId, V4Float32(1.0f, 2.0f, 3.0f, 4.0f));
	stateBufferStorage->SetStateValue(wideValueStateId, V4Float64(1.0, 2.0, 3.0, 4.0));
	stateBufferStorage->SetStateValue(wideValueStateId, matrix2);
	stateBufferStorage->SetStateValue(wideValueStateId, matrix3);
	stateBufferStorage->SetStateValue(wideValueStateId, matrix4);
	stateBufferStorage->SetStateValueForPage(1, wideValueStateId, V4Float32(0.25f, 0.50f, 0.75f, 1.0f));
	stateBufferStorage->SetStateValueForPage(2, wideValueStateId, matrix4);
	stateBufferStorage->SetStateValue(arrayValueStateId, V4Float32(0.25f, 0.50f, 0.75f, 1.0f), 1);
	stateBufferStorage->SetStateValueForPage(2, arrayValueStateId, V4Float32(0.75f, 0.50f, 0.25f, 1.0f), 2);
	stateBufferStorage->SetStateValue(StateValueId::Null, V4Float32(1.0f, 0.0f, 0.0f, 1.0f));
	stateBufferStorage->SetStateValue(arrayValueStateId, V4Float32(1.0f, 0.0f, 0.0f, 1.0f));
	stateBufferStorage->SetStateValue(arrayValueStateId, V4Float32(1.0f, 0.0f, 0.0f, 1.0f), 99);
	stateBufferStorage->SetStateValueForPage(99, wideValueStateId, V4Float32(1.0f, 0.0f, 0.0f, 1.0f));
	uint8_t rawStateData[16] = {};
	stateBufferStorage->SetRawPageData(0, &rawStateData[0], sizeof(rawStateData));
	stateBufferStorage->SetRawPageData(99, &rawStateData[0], sizeof(rawStateData));
	stateBufferStorage->SetRawPageData(0, &rawStateData[0], sizeof(rawStateData), session.Device().GetDataBufferLimits().maxStateBufferPageSize);

	// Render one concrete state-buffer value after the broad state-buffer storage and validation pass.
	{
		auto stateBufferShaderProgram = renderer.CreateShaderProgram();
		REQUIRE(stateBufferShaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(VertexShader)));
		REQUIRE(stateBufferShaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(StateBufferFragmentShader)));
		REQUIRE(stateBufferShaderProgram->CompileProgram());
		auto stateBufferPositionAttributeId = stateBufferShaderProgram->GetVertexAttributeId("position");
		auto colorBufferId = stateBufferShaderProgram->GetStateBufferId("ColorCBuffer");

		auto visibleStateBufferLayout = renderer.CreateStateBufferLayout();
		REQUIRE(stateBufferShaderProgram->LoadStateBufferLayoutFromShader(colorBufferId, visibleStateBufferLayout.get()));
		auto visibleStateBuffer = renderer.CreateStateBuffer();
		REQUIRE(visibleStateBuffer->BindBufferLayout(visibleStateBufferLayout.get()));
		visibleStateBuffer->SetPageSettings(1);
		REQUIRE(visibleStateBuffer->AllocateMemory());
		visibleStateBuffer->SetStateValue(visibleStateBuffer->GetStateValueId("drawColor"), V4Float32(1.0f, 1.0f, 0.0f, 1.0f));

		auto renderableNode = renderer.CreateRenderableNode();
		REQUIRE(renderableNode->BindVertexAttribute(fullscreenQuadAttribute, stateBufferPositionAttributeId));
		REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
		renderableNode->BindStateBuffer(colorBufferId, visibleStateBuffer.get());

		auto groupNode = renderer.CreateStateGroupNode();
		groupNode->AddChildNode(renderableNode.get());

		auto programNode = renderer.CreateProgramNode();
		REQUIRE(programNode->BindShaderProgram(stateBufferShaderProgram.get()));
		programNode->AddChildNode(groupNode.get());

		auto renderPassNode = renderer.CreateRenderPassNode();
		renderPassNode->BindFrameBuffer(frameBuffer.get());
		renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
		renderPassNode->AddChildNode(programNode.get());
		renderer.SetRenderPasses(&renderPassNode, 1);

		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("StateBufferValueTypeStorage", "A fullscreen yellow color supplied by a state buffer after exercising state-buffer value storage and validation paths.", std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.98);
		renderer.RemoveAllRenderPasses();
	}

	// Create our vertex buffer and populate it with two independent quads
	auto leftQuadData = CreateQuad(-0.95f, -0.35f);
	auto rightQuadData = CreateQuad(0.35f, 0.95f);
	VertexAttribute<V4Float32> leftQuadAttribute(leftQuadData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	VertexAttribute<V4Float32> rightQuadAttribute(rightQuadData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	auto vertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE(vertexBuffer->BindVertexAttribute(leftQuadAttribute));
	REQUIRE(leftQuadAttribute.SetInitialData(leftQuadData));
	REQUIRE(vertexBuffer->BindVertexAttribute(rightQuadAttribute));
	REQUIRE(rightQuadAttribute.SetInitialData(rightQuadData));
	REQUIRE(vertexBuffer->AllocateMemory());

	// Create our renderable nodes. The drawColor value is deliberately not set on the default state, state groups,
	// or renderables because program constant state values are not part of the normal state override hierarchy.
	auto leftRenderableNode = renderer.CreateRenderableNode();
	REQUIRE(leftRenderableNode->BindVertexAttribute(leftQuadAttribute, positionAttributeId));
	REQUIRE(leftRenderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));

	auto rightRenderableNode = renderer.CreateRenderableNode();
	REQUIRE(rightRenderableNode->BindVertexAttribute(rightQuadAttribute, positionAttributeId));
	REQUIRE(rightRenderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));

	auto leftGroupNode = renderer.CreateStateGroupNode();
	leftGroupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);
	leftGroupNode->AddChildNode(leftRenderableNode.get());

	auto rightGroupNode = renderer.CreateStateGroupNode();
	rightGroupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);
	rightGroupNode->AddChildNode(rightRenderableNode.get());

	// Create two program nodes that use the same shader program but have different constant state values.
	auto leftProgramNode = renderer.CreateProgramNode();
	REQUIRE(leftProgramNode->BindShaderProgram(shaderProgram.get()));
	leftProgramNode->SetConstantStateValue(drawColorStateId, V4Float32(1.0f, 0.0f, 0.0f, 1.0f));
	leftProgramNode->AddChildNode(leftGroupNode.get());

	auto rightProgramNode = renderer.CreateProgramNode();
	REQUIRE(rightProgramNode->BindShaderProgram(shaderProgram.get()));
	rightProgramNode->SetConstantStateValue(drawColorStateId, V4Float32(0.0f, 1.0f, 0.0f, 1.0f));
	rightProgramNode->AddChildNode(rightGroupNode.get());

	// Create our render pass node
	auto renderPassNode = renderer.CreateRenderPassNode();
	renderPassNode->BindFrameBuffer(frameBuffer.get());
	renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
	IProgramNode* programNodes[] = {leftProgramNode.get(), rightProgramNode.get()};
	renderPassNode->AddChildNodes(&programNodes[0], 2);

	// Bind our render tree to the renderer
	renderer.SetRenderPasses(&renderPassNode, 1);

	// Constant state values are scoped to the program node that set them.
	auto frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("ProgramNodeConstants", "A red quad on the left and a green quad on the right, using different program-node constant state values.", std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.98);

	// Resetting and setting a new constant value should update that program node without relying on downstream state.
	rightProgramNode->ResetConstantStateValue(drawColorStateId);
	rightProgramNode->SetConstantStateValue(drawColorStateId, V4Float32(0.0f, 0.0f, 1.0f, 1.0f));
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("ProgramNodeConstantResetAndReplace", "The left quad remains red while the right quad becomes blue after resetting and replacing its program-node constant state value.", std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.98);

	// Remove all our defined render passes
	renderer.RemoveAllRenderPasses();
	return true;
}

} // namespace cobalt::graphics::testing
