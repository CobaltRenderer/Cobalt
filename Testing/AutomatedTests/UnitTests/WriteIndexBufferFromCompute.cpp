// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../UnitTestBase.h"
#include <unordered_set>
namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

namespace {
// Define our shader programs
const std::string VertexShader = R"(
struct VSInput
{
    float4 position : position;
    float4 color : color;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

VSOutput main(VSInput IN)
{
    VSOutput OUT;

    OUT.color = IN.color;
    OUT.position = IN.position;

    return OUT;
}
)";
const std::string FragmentShader = R"(
struct VSOutput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

float4 main(VSOutput IN) : SV_TARGET0
{
    return IN.color;
}
)";
const std::string ComputeShader = R"(
RWBuffer<float4> vertexData;
RWBuffer<uint> indexData;

[numthreads(1, 1, 1)]
void main(uint3 threadId : SV_DispatchThreadID)
{
	// Position data
	vertexData[0] = float4(-0.5f, 0.5f, 0.5f, 1.0f);
	vertexData[1] = float4(0.0f, -0.5f, 0.5f, 1.0f);
	vertexData[2] = float4(0.5f, 0.5f, 0.5f, 1.0f);

	// Color data
	vertexData[3] = float4(1.0f, 0.0f, 0.0f, 1.0f);
	vertexData[4] = float4(0.0f, 1.0f, 0.0f, 1.0f);
	vertexData[5] = float4(0.0f, 0.0f, 1.0f, 1.0f);

	// Index data
	indexData[0] = 0;
	indexData[1] = 1;
	indexData[2] = 2;
}
)";
} // namespace

DEFINE_UNIT_TEST_WITH_BASE("Compute/WriteIndexBufferFromCompute", UnitTestBase)
{
	// Ensure compute shaders are supported by the current renderer
	if (!session.Device().IsFeatureSupported(IGraphicsDevice::Feature::ComputeShaders))
	{
		session.AddTestSkipped("Skipped test", "This test was skipped, as the current renderer doesn't support compute shaders on this device.");
		return true;
	}

	// Retrieve our important objects
	auto& renderer = session.Renderer();
	auto& uiThread = session.UIThread();
	auto& testWindowInfo = *session.TestWindowPlatformInfo();

	// Set our thread counts for the compute operation
	uint32_t threadCount = 1;

	// Create and compile our compute shader program
	auto computeShaderProgram = renderer.CreateShaderProgram();
	REQUIRE(computeShaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Compute, ShaderSourceInfoHLSL(ComputeShader)));
	REQUIRE(computeShaderProgram->CompileProgram());

	// Create our data arrays
	auto vertexData = renderer.CreateTexelArray();
	vertexData->SetBufferLayout(ITexelArray::ImageFormat::RGBA, ITexelArray::DataFormat::Float32, 6);
	auto indexData = renderer.CreateTexelArray();
	indexData->SetBufferLayout(ITexelArray::ImageFormat::R, ITexelArray::DataFormat::UInt32, 3);

	// Retrieve our shader attribute IDs
	auto vertexDataId = computeShaderProgram->GetResourceArrayId("vertexData");
	auto indexDataId = computeShaderProgram->GetResourceArrayId("indexData");

	// Create our state group node
	auto computeGroupNode = renderer.CreateStateGroupNode();
	computeGroupNode->BindResourceArray(vertexDataId, vertexData.get());
	computeGroupNode->BindResourceArray(indexDataId, indexData.get());

	// Define our compute task
	V3UInt32 threadGroupCounts(threadCount, 1, 1);
	computeGroupNode->SetComputeTask(threadGroupCounts);

	// Create our program node
	auto computeProgramNode = renderer.CreateProgramNode();
	REQUIRE(computeProgramNode->BindShaderProgram(computeShaderProgram.get()));
	computeProgramNode->AddChildNode(computeGroupNode.get());

	// Create our render pass node
	auto computeRenderPassNode = renderer.CreateRenderPassNode();
	computeRenderPassNode->AddChildNode(computeProgramNode.get());

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
	auto colorAttributeId = shaderProgram->GetVertexAttributeId("color");

	// Create our vertex buffer and alias it to the compute buffer
	size_t vertexCount = 3;
	VertexAttribute<V4Float32> vertexAttributePosition(vertexCount, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteOften | IVertexAttribute::PerformanceHint::ReadOften);
	VertexAttribute<V4Float32> vertexAttributeColor(vertexCount, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteOften | IVertexAttribute::PerformanceHint::ReadOften);
	auto vertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE(vertexBuffer->BindVertexAttributeManualLayout(vertexAttributePosition, 0, sizeof(V4Float32)));
	REQUIRE(vertexBuffer->BindVertexAttributeManualLayout(vertexAttributeColor, vertexCount * sizeof(V4Float32), sizeof(V4Float32)));
	REQUIRE(vertexBuffer->AllocateMemoryWithAlias(vertexData.get()));

	// Create our index buffer and alias it to the compute buffer
	size_t indexCount = 3;
	IndexAttribute<V1UInt32> indexAttribute(indexCount, IIndexAttribute::PerformanceHint::WriteNever | IIndexAttribute::PerformanceHint::ReadNever, IIndexAttribute::PerformanceHint::WriteOften | IIndexAttribute::PerformanceHint::ReadOften);
	auto indexBuffer = renderer.CreateIndexBuffer();
	REQUIRE(indexBuffer->BindIndexAttributeManualLayout(indexAttribute, 0, sizeof(V1UInt32)));
	REQUIRE(indexBuffer->AllocateMemoryWithAlias(indexData.get()));

	// Create our renderable node
	auto renderableNode = renderer.CreateRenderableNode();
	REQUIRE(renderableNode->BindVertexAttribute(vertexAttributePosition, positionAttributeId));
	REQUIRE(renderableNode->BindVertexAttribute(vertexAttributeColor, colorAttributeId));
	REQUIRE(renderableNode->BindIndexAttribute(indexAttribute));
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
	IRenderPassNode* passes[] = {computeRenderPassNode.get(), renderPassNode.get()};
	renderer.SetRenderPasses(&passes[0], 2);

	// Capture an image of the scene
	auto frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("RGBTriangleDown", "A red, green and blue triangle, pointing down", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Remove all our defined render passes
	renderer.RemoveAllRenderPasses();
	return true;
}

} // namespace cobalt::graphics::testing
