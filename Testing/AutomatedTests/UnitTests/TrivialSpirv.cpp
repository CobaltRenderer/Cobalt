// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../UnitTestBase.h"

namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

// clang-format off
// Define our shader programs. Line breaks deliniate indivudual instructions.
const uint32_t VertexShader[] =
{
	0x07230203, 0x00010300, 0x0008000a, 0x00000034, 0x00000000,

	0x00020011, 0x00000001,
	0x0006000b, 0x00000002, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 
	0x0003000e, 0x00000000, 0x00000001,

	/*
The below OpEntryPoint (0x....000f) defines the following:

	struct VSInput {
			float4 position : position;
			float3 color : color;
	};

	struct VSOutput {
			float4 position : SV_POSITION;
			float3 color : COLOR;
	};  */

	0x0009000f, 0x00000000, 0x00000005, 0x6e69616d, 0x00000000,	0x00000023, 0x00000027, 0x0000002f, 0x00000033, 

	0x00030007, 0x00000001, 0x00000000,
	0x00040003, 0x00000005, 0x000001f4, 0x00000001,

	0x00040005, 0x00000005, 0x6e69616d, 0x00000000,
	0x00050005, 0x00000023, 0x69736f70, 0x6e6f6974, 0x00000000, 
	0x00040005, 0x00000027, 0x6f6c6f63, 0x00000072, 
	0x00090005, 0x0000002f, 0x746e6540, 0x6f507972, 0x4f746e69, 0x75707475, 0x6f702e74, 0x69746973, 0x00006e6f,
	0x00060005, 0x00000033, 0x6f6c6f63, 0x6f4c5f72, 0x69746163, 0x00306e6f,

	0x0006014a, 0x72746e65, 0x6f702d79, 0x20746e69, 0x6e69616d, 0x00000000,
	0x0006014a, 0x6f747561, 0x70616d2d, 0x636f6c2d, 0x6f697461, 0x0000736e,
	0x0006014a, 0x67726174, 0x652d7465, 0x7320766e, 0x76726970, 0x00332e31,
	0x0005014a, 0x6c736c68, 0x66666f2d, 0x73746573, 0x00000000,

	0x00040047, 0x00000023, 0x0000001e, 0x00000000,
	0x00040047, 0x00000027, 0x0000001e, 0x00000001,
	0x00040047, 0x0000002f, 0x0000000b, 0x00000000,
	0x00040047, 0x00000033, 0x0000001e, 0x00000000,
	0x00020013, 0x00000003,
	0x00030021, 0x00000004, 0x00000003,
	0x00030016, 0x00000007, 0x00000020,
	0x00040017, 0x00000008, 0x00000007, 0x00000004,
	0x00040017, 0x00000009, 0x00000007, 0x00000003,
	0x00040020, 0x00000022, 0x00000001, 0x00000008,
	0x0004003b, 0x00000022, 0x00000023, 0x00000001,
	0x00040020, 0x00000026, 0x00000001, 0x00000009,
	0x0004003b, 0x00000026, 0x00000027, 0x00000001,
	0x00040020, 0x0000002e, 0x00000003, 0x00000008,
	0x0004003b, 0x0000002e, 0x0000002f, 0x00000003,
	0x00040020, 0x00000032, 0x00000003, 0x00000009,
	0x0004003b, 0x00000032, 0x00000033, 0x00000003,


	/*
	The code following OpFunction (0x....0036) to OpFunctionEnd(0x....0038)
	performs the following trivial shader.

	VSOutput main(VSInput IN)
	{
			VSOutput OUT;

			OUT.color = IN.color;
			OUT.position = IN.position;

			return OUT;
	}	*/

	0x00050036, 0x00000003, 0x00000005, 0x00000000, 0x00000004,
	0x000200f8, 0x00000006, 0x00040008, 0x00000001, 0x0000000d, 0x00000000,
	0x0004003d, 0x00000008, 0x00000024, 0x00000023,
	0x0004003d, 0x00000009, 0x00000028, 0x00000027,
	0x0003003e, 0x0000002f, 0x00000024,
	0x0003003e, 0x00000033, 0x00000028,
	0x000100fd,
	0x00010038,
};
constexpr size_t VertexShaderSizeInUnits = sizeof(VertexShader) / sizeof(VertexShader[0]);

const uint32_t FragmentShader[] =
{
	0x07230203, 0x00010300, 0x0008000a, 0x00000034, 0x00000000,

	0x00020011, 0x00000001,
	0x0006000b, 0x00000002, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000,
	0x0003000e, 0x00000000, 0x00000001,

	/* The following OpEntryPoint defines the following

	struct VSOutput {
		float4 position : SV_POSITION;
		float3 color : COLOR;
	};

	And defines the fragment shader output as a single float4 colour.
	*/

	0x0008000f, 0x00000004, 0x00000005, 0x6e69616d, 0x00000000, 0x0000001f, 0x00000024, 0x00000028,

	0x00030010, 0x00000005, 0x00000007,
	0x00030007, 0x00000001, 0x00000000, 0x00040003, 0x00000005, 0x000001f4, 0x00000001,

	0x00040005, 0x00000005, 0x6e69616d, 0x00000000, 0x00050005, 0x0000001f, 0x702e4e49, 0x7469736f, 0x006e6f69,
	0x00070005, 0x00000024, 0x632e4e49, 0x726f6c6f, 0x636f4c5f, 0x6f697461, 0x0000306e,
	0x00070005, 0x00000028, 0x746e6540, 0x6f507972, 0x4f746e69, 0x75707475, 0x00000074,

	0x0006014a, 0x72746e65, 0x6f702d79, 0x20746e69, 0x6e69616d, 0x00000000,
	0x0006014a, 0x6f747561, 0x70616d2d, 0x636f6c2d, 0x6f697461, 0x0000736e,
	0x0006014a, 0x67726174, 0x652d7465, 0x7320766e, 0x76726970, 0x00332e31,
	0x0005014a, 0x6c736c68, 0x66666f2d, 0x73746573, 0x00000000,

	0x00040047, 0x0000001f, 0x0000000b, 0x0000000f,
	0x00040047, 0x00000024, 0x0000001e, 0x00000000,
	0x00040047, 0x00000028, 0x0000001e, 0x00000000,
	0x00020013, 0x00000003,
	0x00030021, 0x00000004, 0x00000003,
	0x00030016, 0x00000007, 0x00000020,
	0x00040017, 0x00000008, 0x00000007, 0x00000004,
	0x00040017, 0x00000009, 0x00000007, 0x00000003,
	0x0004002b, 0x00000007, 0x00000015, 0x3f800000,
	0x00040020, 0x0000001e, 0x00000001, 0x00000008,
	0x0004003b, 0x0000001e, 0x0000001f, 0x00000001,
	0x00040020, 0x00000023, 0x00000001, 0x00000009,
	0x0004003b, 0x00000023, 0x00000024, 0x00000001,
	0x00040020, 0x00000027, 0x00000003, 0x00000008,
	0x0004003b, 0x00000027, 0x00000028, 0x00000003,

	/*
	The following OpFunction to OpFunctionEnd span performs the following
	trivial shader.

	float4 main(VSOutput IN) : SV_TARGET0
	{
			return float4(IN.color, 1.0f);
	}
	*/

	0x00050036, 0x00000003, 0x00000005, 0x00000000, 0x00000004,
	0x000200f8, 0x00000006, 0x0004003d, 0x00000009, 0x00000025, 0x00000024,
	0x00050051, 0x00000007, 0x00000030, 0x00000025, 0x00000000,
	0x00050051, 0x00000007, 0x00000031, 0x00000025, 0x00000001,
	0x00050051, 0x00000007, 0x00000032, 0x00000025, 0x00000002,
	0x00070050, 0x00000008, 0x00000033, 0x00000030, 0x00000031, 0x00000032, 0x00000015,
	0x0003003e, 0x00000028, 0x00000033,
	0x000100fd,
	0x00010038, 
};
constexpr size_t FragmentShaderSizeInUnits = sizeof(FragmentShader) / sizeof(FragmentShader[0]);
// clang-format on

DEFINE_UNIT_TEST_WITH_BASE("Shader/TrivialSpirv", UnitTestBase)
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
	REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoSPIRV(&VertexShader[0], VertexShaderSizeInUnits)));
	REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoSPIRV(&FragmentShader[0], FragmentShaderSizeInUnits)));
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
