// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../UnitTestBase.h"
namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

namespace {
// Define our shader programs
const std::string VertexShader = R"(
struct VSInput {
    uint vertexId : SV_VertexID;
    float4 position : position;
};

struct VSOutput {
    float4 position : SV_POSITION;
    float3 color : COLOR;
};

StructuredBuffer<float4> colorData;

VSOutput main(VSInput IN)
{
    VSOutput OUT;

    float4 color = colorData[IN.vertexId];

    OUT.color = color.rgb;
    OUT.position = IN.position;

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

DEFINE_UNIT_TEST_WITH_BASE("Resources/DataArray/GPUTransfer", UnitTestBase)
{
	// Ensure data arrays are supported by the current renderer
	if (!session.Device().IsFeatureSupported(IGraphicsDevice::Feature::ResourceArrays))
	{
		session.AddTestSkipped("Skipped test", "This test was skipped, as the current renderer doesn't support texel arrays on this device.");
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

	// Create the initial data for our resource arrays
	std::vector<V4Float32> colorData = {{1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}};
	std::vector<V4Float32> altColorData = {{0.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 0.0f, 1.0f}};

	// Create our resource arrays
	auto colorArray = renderer.CreateDataArray();
	colorArray->SetBufferLayout(sizeof(V4Float32), colorData.size());
	colorArray->SetUsageFlags(IDataArray::UsageFlags::ShaderInput | IDataArray::UsageFlags::TransferDestination);
	REQUIRE(colorArray->SetInitialData(colorData.data(), colorData.size() * sizeof(V4Float32)));
	REQUIRE(colorArray->AllocateMemory());
	auto altColorArray = renderer.CreateDataArray();
	altColorArray->SetBufferLayout(sizeof(V4Float32), altColorData.size());
	altColorArray->SetUsageFlags(IDataArray::UsageFlags::TransferSource);
	REQUIRE(altColorArray->SetInitialData(altColorData.data(), altColorData.size() * sizeof(V4Float32)));
	REQUIRE(altColorArray->AllocateMemory());

	// Retrieve our shader attribute IDs
	auto positionAttributeId = shaderProgram->GetVertexAttributeId("position");
	auto colorDataId = shaderProgram->GetResourceArrayId("colorData");

	// Create our vertex buffer and populate it with data
	std::vector<V4Float32> positionVertexData = {{-0.5f, 0.5f, 0.5f, 1.0f}, {0.0f, -0.5f, 0.5f, 1.0f}, {0.5f, 0.5f, 0.5f, 1.0f}};
	VertexAttribute<V4Float32> vertexAttributePosition(positionVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	auto vertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttributePosition));
	REQUIRE(vertexAttributePosition.SetInitialData(positionVertexData));
	REQUIRE(vertexBuffer->AllocateMemory());

	// Create our renderable node
	auto renderableNode = renderer.CreateRenderableNode();
	REQUIRE(renderableNode->BindVertexAttribute(vertexAttributePosition, positionAttributeId));
	REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));

	// Create our state group node
	auto groupNode = renderer.CreateStateGroupNode();
	groupNode->BindResourceArray(colorDataId, colorArray.get());
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
	{
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("RGBTriangleDown", "A red, green and blue triangle, pointing down", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);
	}

	// Queue a transfer overwriting an entry in the color array with an entry from our alternate color array
	REQUIRE(altColorArray->QueueDataTransfer(colorArray.get(), 1 * sizeof(V4Float32), 0 * sizeof(V4Float32), 2 * sizeof(V4Float32)));

	// Capture an image of the scene
	{
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("CyanSwap", "A red, green and cyan triangle, pointing down", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);
	}

	// Queue a transfer overwriting an entry in the color array with an entry from our alternate color array
	REQUIRE(altColorArray->QueueDataTransfer(colorArray.get(), 1 * sizeof(V4Float32), 1 * sizeof(V4Float32), 1 * sizeof(V4Float32)));

	// Capture an image of the scene
	{
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("MagentaSwap", "A red, magenta and cyan triangle, pointing down", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);
	}

	// Queue a transfer overwriting an entry in the color array with an entry from our alternate color array
	REQUIRE(altColorArray->QueueDataTransfer(colorArray.get(), 1 * sizeof(V4Float32), 2 * sizeof(V4Float32), 0 * sizeof(V4Float32)));

	// Capture an image of the scene
	{
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("YellowSwap", "A yellow, magenta and cyan triangle, pointing down", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);
	}

	// Remove all our defined render passes
	renderer.RemoveAllRenderPasses();
	return true;
}

} // namespace cobalt::graphics::testing
