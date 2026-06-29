// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../UnitTestBase.h"

namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

// Define our shader programs
/*
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
*/
const std::string VertexShader = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Vertex %main "main" %position %color %_entryPointOutput_position %color_Location0

OpName %main "main"
OpName %position "position"
OpName %color "color"
OpName %_entryPointOutput_position "@entryPointOutput.position"
OpName %color_Location0 "color_Location0"

OpDecorate %position Location 0
OpDecorate %color Location 1
OpDecorate %_entryPointOutput_position BuiltIn Position
OpDecorate %color_Location0 Location 0

%void = OpTypeVoid
%4 = OpTypeFunction %void
%float = OpTypeFloat 32
%v4float = OpTypeVector %float 4
%v3float = OpTypeVector %float 3

%_ptr_Input_v4float = OpTypePointer Input %v4float
%position = OpVariable %_ptr_Input_v4float Input

%_ptr_Input_v3float = OpTypePointer Input %v3float
%color = OpVariable %_ptr_Input_v3float Input

%_ptr_Output_v4float = OpTypePointer Output %v4float
%_entryPointOutput_position = OpVariable %_ptr_Output_v4float Output

%_ptr_Output_v3float = OpTypePointer Output %v3float
%color_Location0 = OpVariable %_ptr_Output_v3float Output

%main = OpFunction %void None %4
%6 = OpLabel
%36 = OpLoad %v4float %position
%40 = OpLoad %v3float %color
OpStore %_entryPointOutput_position %36
OpStore %color_Location0 %40
OpReturn
OpFunctionEnd
)";
/*
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
*/
const std::string FragmentShader = R"(
OpCapability Shader
OpMemoryModel Logical GLSL450
OpEntryPoint Fragment %main "main" %IN_position %IN_color_Location0 %_entryPointOutput
OpExecutionMode %main OriginUpperLeft

OpName %main "main"
OpName %IN_position "IN.position"
OpName %IN_color_Location0 "IN.color_Location0"
OpName %_entryPointOutput "@entryPointOutput"

OpDecorate %IN_position BuiltIn FragCoord
OpDecorate %IN_color_Location0 Location 0
OpDecorate %_entryPointOutput Location 0

%void = OpTypeVoid
%4 = OpTypeFunction %void
%float = OpTypeFloat 32
%v4float = OpTypeVector %float 4
%v3float = OpTypeVector %float 3
%float_1 = OpConstant %float 1

%_ptr_Input_v4float = OpTypePointer Input %v4float
%IN_position = OpVariable %_ptr_Input_v4float Input

%_ptr_Input_v3float = OpTypePointer Input %v3float
%IN_color_Location0 = OpVariable %_ptr_Input_v3float Input

%_ptr_Output_v4float = OpTypePointer Output %v4float
%_entryPointOutput = OpVariable %_ptr_Output_v4float Output

%main = OpFunction %void None %4
%6 = OpLabel
%37 = OpLoad %v3float %IN_color_Location0
%48 = OpCompositeExtract %float %37 0
%49 = OpCompositeExtract %float %37 1
%50 = OpCompositeExtract %float %37 2
%51 = OpCompositeConstruct %v4float %48 %49 %50 %float_1
OpStore %_entryPointOutput %51
OpReturn
OpFunctionEnd
)";

DEFINE_UNIT_TEST_WITH_BASE("Shader/TrivialSpirvAssembly", UnitTestBase)
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
	REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoSPIRVAssembly(VertexShader)));
	REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoSPIRVAssembly(FragmentShader)));
	REQUIRE(shaderProgram->CompileProgram());

	// Retrieve our shader attribute IDs
	auto positionAttributeId = shaderProgram->GetVertexAttributeId("position");
	auto colorAttributeId = shaderProgram->GetVertexAttributeId("color");

	// Create our vertex buffer and populate it with data
	std::vector<V4Float32> positionVertexData = {{-0.5f, 0.5f, 0.5f, 1.0f}, {0.0f, -0.5f, 0.5f, 1.0f}, {0.5f, 0.5f, 0.5f, 1.0f}};
	std::vector<V3Float32> colorVertexData = {{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}};
	VertexAttribute<V4Float32> vertexAttributePosition(positionVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	VertexAttribute<V3Float32> vertexAttributeColor(colorVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
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

	// Remove all our defined render passes
	groupNode->RemoveAllChildNodes();
	renderer.RemoveAllRenderPasses();
	return true;
}

} // namespace cobalt::graphics::testing
