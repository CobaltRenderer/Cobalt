// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../UnitTestBase.h"
#include <unordered_set>
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
const std::string ComputeShader1 = R"(
uniform uint indexCount;
RWByteAddressBuffer bufferDataOut;

[numthreads(1, 1, 1)]
void main(uint3 threadId : SV_DispatchThreadID)
{
	bufferDataOut.Store(0, indexCount);
	bufferDataOut.Store(4, 1);
	bufferDataOut.Store(8, 0);
	bufferDataOut.Store(12, 0);
	bufferDataOut.Store(16, 0);
}
)";
} // namespace

DEFINE_UNIT_TEST_WITH_BASE("Renderable/IndirectDrawFromCompute", UnitTestBase)
{
	// Ensure compute shaders are supported by the current renderer
	if (!session.Device().IsFeatureSupported(IGraphicsDevice::Feature::ComputeShaders))
	{
		session.AddTestSkipped("Skipped test", "This test was skipped, as the current renderer doesn't support compute shaders on this device.");
		return true;
	}
	if (!session.Device().IsFeatureSupported(IGraphicsDevice::Feature::ResourceArrays))
	{
		session.AddTestSkipped("Skipped test", "This test was skipped, as the current renderer doesn't support data arrays.");
		return true;
	}
	if (!session.Device().IsFeatureSupported(IGraphicsDevice::Feature::IndirectDraw))
	{
		session.AddTestSkipped("Skipped test", "This test was skipped, as the current renderer doesn't support indirect drawing.");
		return true;
	}

	// Retrieve our important objects
	auto& renderer = session.Renderer();
	auto& uiThread = session.UIThread();
	auto& testWindowPlatformInfo = *session.TestWindowPlatformInfo();

	// Set our thread counts for the compute operation
	uint32_t threadCount = 1;

	// Create and compile our compute shader program
	auto computeShaderProgram = renderer.CreateShaderProgram();
	REQUIRE(computeShaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Compute, ShaderSourceInfoHLSL(ComputeShader1)));
	REQUIRE(computeShaderProgram->CompileProgram());

	// Create our output data array
	auto dataArray = renderer.CreateDataArray();
	dataArray->SetUsageFlags(IDataArray::UsageFlags::IndirectDrawSource);
	size_t structureSizeOut = sizeof(IRenderableNode::IndexedIndirectDrawParams);
	dataArray->SetBufferLayout(structureSizeOut, 1);
	REQUIRE(dataArray->AllocateMemory());

	// Retrieve our shader attribute IDs
	auto dataArrayOutId = computeShaderProgram->GetResourceArrayId("bufferDataOut");

	// Create our state group node
	auto computeGroupNode = renderer.CreateStateGroupNode();
	computeGroupNode->BindResourceArray(dataArrayOutId, dataArray.get());

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
	auto testWindowFrameBuffer = renderer.CreateFrameBuffer();
	testWindowFrameBuffer->DefineViewportRegion(V2UInt32(0, 0), session.TestWindowSize());
	REQUIRE(uiThread.InvokeSync([&] { return testWindowFrameBuffer->BindWindow(testWindowPlatformInfo, IFrameBuffer::WindowDepthStencilMode::DepthUNorm24); }));

	// Create and compile our shader program
	auto shaderProgram = renderer.CreateShaderProgram();
	REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(VertexShader)));
	REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(FragmentShader)));
	REQUIRE(shaderProgram->CompileProgram());

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
	IRenderPassNode* passes[] = {computeRenderPassNode.get(), renderPassNode.get()};
	renderer.SetRenderPasses(&passes[0], 2);

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

	// Pass the index count to our compute shader
	computeGroupNode->SetStateValue(computeShaderProgram->GetStateValueId("indexCount"), V1UInt32((unsigned int)indexAttribute.GetIndexCount()));

	// Ensure the complete circle is drawn
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("YellowCircleComplete", "A complete yellow circle", std::move(frameBufferCapture), cobalt::graphics::IImageDiff::Algorithm::BinaryCountExact | cobalt::graphics::IImageDiff::Algorithm::RegionRanges);

	// Remove all our defined render passes
	renderer.RemoveAllRenderPasses();
	return true;
}

} // namespace cobalt::graphics::testing
