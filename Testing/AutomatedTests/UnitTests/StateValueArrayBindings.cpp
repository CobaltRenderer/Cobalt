// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../UnitTestBase.h"
namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

// Define our shader programs
const std::string VertexShader = R"(
struct VSInput {
    uint vertexId : SV_VertexID;
    float3 position : position;
};

struct VSOutput {
    float4 position : SV_POSITION;
    float3 color : COLOR;
};

// Test nested array syntax
struct Level3 {
    float mulLevel3Start;
    float value[3];
    float mulLevel3End;
};
struct Level2 {
    float mulLevel2Start;
    Level3 level3[3];
    float mulLevel2End;
};
struct Level1 {
    float mulLevel1Start;
    Level2 level2[8];
    float multiDimensionalArray[2][4][6];
    float mulLevel1End;
};
uniform Level1 level1;

// Test multidimensional array flattening
uniform float globalMultiDimensionalArray[2][4][6];

// Test default value initialization
uniform float entryWithDefaultValue[3] = {0.5f, 1.0f, 0.25f};
struct SomeOtherStruct {
    float member1;
};
struct SomeStruct {
    float member1;
    SomeOtherStruct member2[2];
};
uniform SomeStruct someStruct[3] = {{0.0f, {{0.0f}, {0.0f}}}, {0.0f, {{0.0f}, {1.0f}}}, {0.0f, {{0.0f}, {0.0f}}}};

VSOutput main(VSInput IN)
{
    VSOutput OUT;

//    OUT.color = float3(level1.level2[1].level3[0].value[IN.vertexId] * level1.mulLevel1End * level1.level2[0].mulLevel2End * entryWithDefaultValue[1] * someStruct[1].member2[1].member1 * globalMultiDimensionalArray[1][2][3],
//                       level1.level2[3].level3[1].value[IN.vertexId] * level1.mulLevel1End * level1.level2[2].mulLevel2End * entryWithDefaultValue[1] * someStruct[1].member2[1].member1 * globalMultiDimensionalArray[1][2][3],
//                       level1.level2[5].level3[2].value[IN.vertexId] * level1.mulLevel1End * level1.level2[1].mulLevel2End * entryWithDefaultValue[1] * someStruct[1].member2[1].member1 * globalMultiDimensionalArray[1][2][3]);
    OUT.color = float3(level1.level2[1].level3[0].value[IN.vertexId] * level1.mulLevel1End * level1.level2[0].mulLevel2End * globalMultiDimensionalArray[1][2][3] * level1.multiDimensionalArray[1][2][3],
                       level1.level2[3].level3[1].value[IN.vertexId] * level1.mulLevel1End * level1.level2[2].mulLevel2End * globalMultiDimensionalArray[1][2][3] * level1.multiDimensionalArray[1][2][3],
                       level1.level2[5].level3[2].value[IN.vertexId] * level1.mulLevel1End * level1.level2[1].mulLevel2End * globalMultiDimensionalArray[1][2][3] * level1.multiDimensionalArray[1][2][3]);
    OUT.position = float4(IN.position, 1.0f);

    return OUT;
}
)";
const std::string FragmentShader = R"(
uniform float someUniformInFragmentShader = 0.0f;

struct VSOutput {
    float4 position : SV_POSITION;
    float3 color : COLOR;
};

float4 main(VSOutput IN) : SV_TARGET0
{
    return float4(IN.color, 1.0f + someUniformInFragmentShader);
}
)";

DEFINE_UNIT_TEST_WITH_BASE("Resources/StateValue/StateValueArrayBindings", UnitTestBase)
{
	// Ensure arrays of arrays are supported by the current renderer. It's really only OpenGL 3.3 where this isn't
	// necessarily the case, although it's core in 4.3 so macOS is the only place this feature is likely to be missing.
	if (!session.Device().IsFeatureSupported(IGraphicsDevice::Feature::ShaderArraysOfArrays))
	{
		session.AddTestSkipped("Skipped test", "This test was skipped, as the current renderer doesn't support shader arrays of arrays on this device.");
		return true;
	}

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

	// Create our vertex buffer and populate it with data
	std::vector<V3Float32> positionVertexData = {{-0.5f, 0.5f, 0.5f}, {0.0f, -0.5f, 0.5f}, {0.5f, 0.5f, 0.5f}};
	VertexAttribute<V3Float32> vertexAttributePosition(positionVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	auto vertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttributePosition));
	REQUIRE(vertexAttributePosition.SetInitialData(positionVertexData));
	REQUIRE(vertexBuffer->AllocateMemory());

	// Create our renderable node
	auto renderableNode = renderer.CreateRenderableNode();
	REQUIRE(renderableNode->BindVertexAttribute(vertexAttributePosition, positionAttributeId));
	REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));

	// Set our state values
	auto stateValueIdGlobalMultidimensionalArrayFlattened = shaderProgram->GetStateValueId("globalMultiDimensionalArray[]");
	renderableNode->SetStateValue(stateValueIdGlobalMultidimensionalArrayFlattened, V1Float32(1.0f), (1 * (4 * 6)) + (2 * 6) + 3);
	auto stateValueIdMulLevel1End = shaderProgram->GetStateValueId("level1.mulLevel1End");
	renderableNode->SetStateValue(stateValueIdMulLevel1End, V1Float32(2.0f));
	auto stateValueIdMultidimensionalArrayFlattened = shaderProgram->GetStateValueId("level1.multiDimensionalArray[]");
	renderableNode->SetStateValue(stateValueIdMultidimensionalArrayFlattened, V1Float32(1.0f), (1 * (4 * 6)) + (2 * 6) + 3);
	auto stateValueIdMulLevel2End = shaderProgram->GetStateValueId("level1.level2[].mulLevel2End");
	renderableNode->SetStateValue(stateValueIdMulLevel2End, V1Float32(0.5f), 0);
	renderableNode->SetStateValue(stateValueIdMulLevel2End, V1Float32(0.25f), 2);
	renderableNode->SetStateValue(stateValueIdMulLevel2End, V1Float32(0.5f), 1);
	auto stateValueId = shaderProgram->GetStateValueId("level1.level2[].level3[].value[]");
	renderableNode->SetStateValue(stateValueId, V1Float32(1.0f), 1, 0, 0);
	renderableNode->SetStateValue(stateValueId, V1Float32(0.0f), 3, 1, 0);
	renderableNode->SetStateValue(stateValueId, V1Float32(0.0f), 5, 2, 0);
	renderableNode->SetStateValue(stateValueId, V1Float32(0.0f), 1, 0, 1);
	renderableNode->SetStateValue(stateValueId, V1Float32(2.0f), 3, 1, 1);
	renderableNode->SetStateValue(stateValueId, V1Float32(0.0f), 5, 2, 1);
	renderableNode->SetStateValue(stateValueId, V1Float32(0.0f), 1, 0, 2);
	renderableNode->SetStateValue(stateValueId, V1Float32(0.0f), 3, 1, 2);
	renderableNode->SetStateValue(stateValueId, V1Float32(1.0f), 5, 2, 2);

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

	// Remove all our defined render passes
	groupNode->RemoveAllChildNodes();
	renderer.RemoveAllRenderPasses();
	return true;
}

} // namespace cobalt::graphics::testing
