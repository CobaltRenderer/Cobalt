// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../UnitTestBase.h"
#include <numeric>
namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

namespace {
// Define our shader programs
const std::string VertexShader = R"(
struct VSInput
{
    float3 position : position;
    // You'd expect to be able to use SV_InstanceID here, but this DOESN'T work. In HLSL, SV_InstanceID and SV_VertexID
    // both start from 0, regardless of the provided firstVertex and firstInstance values. They act as an offset into
    // bound buffers, but the base values are not exposed to the shaders like they are in other APIs. See the following
    // for some more info:
    // https://github.com/gpuweb/gpuweb/issues/901
    // We fake an instance ID here using an instance attribute looking up into a linearly increasing array of integers,
    // as described here:
    // https://www.g-truc.net/post-0518.html
    // This is imperfect, as it doesn't allow mixing instance drawing with indirect multidraw. GLSL 4.6 adds the new
    // gl_DrawID built in variable to solve this properly, but there is no HLSL equivalent. Things are messy on Vulkan
    // too, as there's two optional and not universally supported related hardware features, multiDrawIndirect and
    // drawIndirectFirstInstance. See the following for more discussion on this issue in Vulkan:
    // https://community.khronos.org/t/vulkan-vs-multidrawindirect/6897/41
    uint instanceId : instanceId;
};
struct VSOutput
{
    float4 position : SV_POSITION;
    float4 color : color;
};
struct InstanceData
{
    float4 colorData;
    float4 positionData;
    float4 scaleData;
};

uniform row_major float4x4 viewProj;
StructuredBuffer<InstanceData> instanceData;

VSOutput main(VSInput IN)
{
    uint counterValue = IN.instanceId;

    VSOutput OUT;
    float3 adjustedPos = (IN.position / instanceData[counterValue].scaleData.xyz) + instanceData[counterValue].positionData.xyz;
    OUT.position = mul(viewProj, float4(adjustedPos, 1.0f));
    OUT.position /= OUT.position.w;
    OUT.color = instanceData[counterValue].colorData;
    return OUT;
}
)";
const std::string FragmentShader = R"(
struct VSOutput
{
    float4 position : SV_POSITION;
    float4 color : color;
};

float4 main(VSOutput IN) : SV_TARGET0
{
    return IN.color;
}
)";
const std::string ComputeShader1 = R"(
struct InstanceData
{
    float4 colorData;
    float4 positionData;
    float4 scaleData;
};

uniform uint indexCount;
uniform uint3 dimensions;
uniform float3 drawRegionStart;
uniform float3 drawRegionEnd;
RWStructuredBuffer<InstanceData> instanceData;
RWStructuredBuffer<uint> drawCount;
RWByteAddressBuffer drawCountRawBuffer;
RWByteAddressBuffer indirectDrawData;

[numthreads(1, 1, 1)]
void main(uint3 threadId : SV_DispatchThreadID)
{
    float3 scale = float3(dimensions);
    float3 position = float3(threadId) / scale;

    if ((position.x < drawRegionStart.x) || (position.x >= drawRegionEnd.x) || (position.y < drawRegionStart.y) || (position.y >= drawRegionEnd.y) || (position.z < drawRegionStart.z) || (position.z >= drawRegionEnd.z))
    {
        return;
    }

    uint counterValue = drawCount.IncrementCounter();
    drawCount[0] = 0;
    uint oldRawDrawCount;
    drawCountRawBuffer.InterlockedAdd(0u, 1u, oldRawDrawCount);

    instanceData[counterValue].scaleData = float4(scale, 0.0);
    instanceData[counterValue].positionData = float4(position, 0.0);
    instanceData[counterValue].colorData = float4(position, 1.0);

    uint bufferBaseAddress = (counterValue * 20);
    indirectDrawData.Store(bufferBaseAddress + 0, indexCount);
    indirectDrawData.Store(bufferBaseAddress + 4, 1);
    indirectDrawData.Store(bufferBaseAddress + 8, 0);
    indirectDrawData.Store(bufferBaseAddress + 12, 0);
    indirectDrawData.Store(bufferBaseAddress + 16, counterValue);
}
)";
} // namespace

DEFINE_UNIT_TEST_WITH_BASE("Renderable/IndirectMultiDrawFromCompute", UnitTestBase)
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
	uint32_t instanceCountX = 20;
	uint32_t instanceCountY = 20;
	uint32_t instanceCountZ = 20;
	uint32_t maxInstanceCount = instanceCountX * instanceCountY * instanceCountZ;

	// Create and compile our compute shader program
	auto computeShaderProgram = renderer.CreateShaderProgram();
	REQUIRE(computeShaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Compute, ShaderSourceInfoHLSL(ComputeShader1)));
	REQUIRE(computeShaderProgram->CompileProgram());

	// Create our draw count data array
	auto drawCountDataArray = renderer.CreateDataArray();
	drawCountDataArray->SetUsageFlags(IDataArray::UsageFlags::IndirectDrawCountSource);
	drawCountDataArray->SetBufferLayout(4, 1, true);
	REQUIRE(drawCountDataArray->AllocateMemory());

	// Create our indirect draw data data array
	auto indirectDrawDataArray = renderer.CreateDataArray();
	indirectDrawDataArray->SetUsageFlags(IDataArray::UsageFlags::IndirectDrawSource);
	size_t indirectDrawStructureSizeOut = sizeof(IRenderableNode::IndexedIndirectDrawParams);
	indirectDrawDataArray->SetBufferLayout(indirectDrawStructureSizeOut, maxInstanceCount);
	REQUIRE(indirectDrawDataArray->AllocateMemory());

	// Create our indirect draw counter data array
	auto indirectDrawRawCounterDataArray = renderer.CreateDataArray();
	indirectDrawRawCounterDataArray->SetUsageFlags(IDataArray::UsageFlags::IndirectDrawCountSource | IDataArray::UsageFlags::AtomicOperations);
	indirectDrawRawCounterDataArray->SetBufferLayout(4, 1);
	REQUIRE(indirectDrawRawCounterDataArray->AllocateMemory());

	// Create our instance data array
	auto instanceDataArray = renderer.CreateDataArray();
	instanceDataArray->SetUsageFlags(IDataArray::UsageFlags::ShaderInput | IDataArray::UsageFlags::ShaderOutput);
	size_t instanceDataStructureSizeOut = sizeof(V4Float32) * 3;
	instanceDataArray->SetBufferLayout(instanceDataStructureSizeOut, maxInstanceCount);
	REQUIRE(instanceDataArray->AllocateMemory());

	// Create our state group node
	auto computeGroupNode = renderer.CreateStateGroupNode();
	computeGroupNode->BindResourceArray(computeShaderProgram->GetResourceArrayId("drawCountRawBuffer"), indirectDrawRawCounterDataArray.get());
	computeGroupNode->BindResourceArray(computeShaderProgram->GetResourceArrayId("drawCount"), drawCountDataArray.get());
	computeGroupNode->BindResourceArray(computeShaderProgram->GetResourceArrayId("instanceData"), instanceDataArray.get());
	computeGroupNode->BindResourceArray(computeShaderProgram->GetResourceArrayId("indirectDrawData"), indirectDrawDataArray.get());

	// Define our compute task
	V3UInt32 threadGroupCounts(instanceCountX, instanceCountY, instanceCountZ);
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

	// Fill an instance attribute to fake a draw ID
	std::vector<V1UInt32> instaceValues(maxInstanceCount);
	std::iota(instaceValues.begin(), instaceValues.end(), 0);

	// Create our vertex buffer and populate it with data
	VertexAttribute<V3Float32> vertexAttributePosition(positionVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	VertexAttribute<V1UInt32> vertexInstanceAttribute(maxInstanceCount, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	auto vertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttributePosition));
	REQUIRE(vertexBuffer->BindVertexAttribute(vertexInstanceAttribute));
	REQUIRE(vertexAttributePosition.SetInitialData(positionVertexData));
	REQUIRE(vertexInstanceAttribute.SetInitialData(instaceValues));
	REQUIRE(vertexBuffer->AllocateMemory());

	// Create our index buffer and populate it with data
	IndexAttribute<V1UInt32> indexAttribute(indexData.size(), IIndexAttribute::PerformanceHint::WriteNever | IIndexAttribute::PerformanceHint::ReadNever, IIndexAttribute::PerformanceHint::WriteNever | IIndexAttribute::PerformanceHint::ReadOften);
	auto indexBuffer = renderer.CreateIndexBuffer();
	REQUIRE(indexBuffer->BindIndexAttribute(indexAttribute));
	REQUIRE(indexAttribute.SetInitialData(indexData));
	REQUIRE(indexBuffer->AllocateMemory());

	// Create our renderable node
	auto renderableNode = renderer.CreateRenderableNode();
	REQUIRE(renderableNode->BindVertexAttribute(vertexAttributePosition, shaderProgram->GetVertexAttributeId("position")));
	REQUIRE(renderableNode->BindVertexInstanceAttribute(vertexInstanceAttribute, shaderProgram->GetVertexAttributeId("instanceId")));
	REQUIRE(renderableNode->BindIndexAttribute(indexAttribute));
	REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::LineStrip));
	renderableNode->BindResourceArray(shaderProgram->GetResourceArrayId("instanceData"), instanceDataArray.get());

	// Pass the index count to our compute shader
	computeGroupNode->SetStateValue(computeShaderProgram->GetStateValueId("indexCount"), V1UInt32((unsigned int)indexAttribute.GetIndexCount()));
	computeGroupNode->SetStateValue(computeShaderProgram->GetStateValueId("dimensions"), threadGroupCounts);
	computeGroupNode->SetStateValue(computeShaderProgram->GetStateValueId("drawRegionStart"), V3Float32(0.1f, 0.1f, 0.1f));
	computeGroupNode->SetStateValue(computeShaderProgram->GetStateValueId("drawRegionEnd"), V3Float32(0.9f, 0.9f, 0.9f));

	// Set our camera position
	auto viewProj = Transform().LookAtCenterPerspective(session.TestWindowSizeAsFloat(), V3Float32(1.1f, 1.1f, 1.1f));
	groupNode->SetStateValue(shaderProgram->GetStateValueId("viewProj"), viewProj);

	// Add our renderable node to the scene
	groupNode->AddChildNode(renderableNode.get());
	IFrameBufferOutput::unique_ptr frameBufferCapture;
	uint32_t zeroRawCounterValue = 0;

	// Test indirect multi-draw with a known correct draw count statically provided
	REQUIRE(indirectDrawRawCounterDataArray->QueueDataUpdate(&zeroRawCounterValue, sizeof(zeroRawCounterValue)));
	REQUIRE(renderableNode->SetIndirectDraw((size_t)16 * 16 * 16, indirectDrawDataArray.get()));
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("StaticDrawCount", "3D field of coloured circles", std::move(frameBufferCapture), cobalt::graphics::IImageDiff::Algorithm::BinaryCountExact | cobalt::graphics::IImageDiff::Algorithm::NaiveDiff | cobalt::graphics::IImageDiff::Algorithm::RegionRanges, 0.95);

	// Test indirect multi-draw with the draw count retrieved from an attached counter
	REQUIRE(indirectDrawRawCounterDataArray->QueueDataUpdate(&zeroRawCounterValue, sizeof(zeroRawCounterValue)));
	REQUIRE(renderableNode->SetIndirectDraw(maxInstanceCount, drawCountDataArray.get(), indirectDrawDataArray.get()));
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("DrawCountFromCounter", "3D field of coloured circles", std::move(frameBufferCapture), cobalt::graphics::IImageDiff::Algorithm::BinaryCountExact | cobalt::graphics::IImageDiff::Algorithm::NaiveDiff | cobalt::graphics::IImageDiff::Algorithm::RegionRanges, 0.95);

	// Test indirect multi-draw with the draw count retrieved from the contents of a data array
	REQUIRE(indirectDrawRawCounterDataArray->QueueDataUpdate(&zeroRawCounterValue, sizeof(zeroRawCounterValue)));
	REQUIRE(renderableNode->SetIndirectDraw(maxInstanceCount, indirectDrawRawCounterDataArray.get(), 0, indirectDrawDataArray.get()));
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("DrawCountFromDataArray", "3D field of coloured circles", std::move(frameBufferCapture), cobalt::graphics::IImageDiff::Algorithm::BinaryCountExact | cobalt::graphics::IImageDiff::Algorithm::NaiveDiff | cobalt::graphics::IImageDiff::Algorithm::RegionRanges, 0.95);

	// Remove all our defined render passes
	renderer.RemoveAllRenderPasses();
	return true;
}

} // namespace cobalt::graphics::testing
