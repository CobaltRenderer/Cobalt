// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../UnitTestBase.h"
#include <array>
#include <vector>
namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

namespace {

// Define our shader programs
const std::string Shader = R"(
uniform bool rBound;
uniform bool gBound;
uniform bool bBound;

struct VSInput {
    float r: r;
    float g: g;
    float b: b;
    uint vertexId: SV_VertexID;
    uint instanceId: SV_InstanceID;
};

struct VSOutput {
    float4 position: SV_POSITION;
    float3 color: color;
};

static const float3 ANCHORS[3] = {
    float3(-0.5, -0.5, 0.5),
    float3(-0.5, 0.5, 0.5),
    float3(0.5, 0.0, 0.5),
};

static const float3 POSITIONS[3] = {
    float3(-0.3, -0.3, 0.0),
    float3(0.3, -0.3, 0.0),
    float3(0, 0.3, 0.0)
};

VSOutput vertexMain(VSInput IN)
{
    VSOutput OUT;

    OUT.position = float4(ANCHORS[IN.instanceId] + POSITIONS[IN.vertexId], 1.0f);
    OUT.color = float3(0.0f, 0.0f, 0.0f);
    if (rBound)
    {
        OUT.color.r += IN.r;
    }
    if (gBound)
    {
        OUT.color.g += IN.g;
    }
    if (bBound)
    {
        OUT.color.b += IN.b;
    }
    return OUT;
}

float4 main(VSOutput IN) : SV_TARGET0
{
    return float4(IN.color, 1.0f);
}
)";

} // namespace

DEFINE_UNIT_TEST_WITH_BASE("Renderable/RenderableUnboundAttributes", UnitTestBase)
{
	// Retrieve our important objects and define the framebuffer.
	auto& renderer = session.Renderer();
	auto& uiThread = session.UIThread();
	auto& testWindowInfo = *session.TestWindowPlatformInfo();
	auto frameBuffer = renderer.CreateFrameBuffer();
	frameBuffer->DefineViewportRegion(V2UInt32(0, 0), session.TestWindowSize());
	REQUIRE(uiThread.InvokeSync([&] { return frameBuffer->BindWindow(testWindowInfo, IFrameBuffer::WindowDepthStencilMode::None); }));

	// Keep all three shader inputs live; uniforms disable reads from unbound attributes at draw time.
	auto shaderProgram = renderer.CreateShaderProgram();
	REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(Shader, "vertexMain")));
	REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(Shader)));
	REQUIRE(shaderProgram->CompileProgram());
	const std::array<VertexAttributeId, 3> attributeIds = {
	  shaderProgram->GetVertexAttributeId("r"),
	  shaderProgram->GetVertexAttributeId("g"),
	  shaderProgram->GetVertexAttributeId("b"),
	};
	const std::array<StateValueId, 3> boundStateIds = {
	  shaderProgram->GetStateValueId("rBound"),
	  shaderProgram->GetStateValueId("gBound"),
	  shaderProgram->GetStateValueId("bBound"),
	};
	for (size_t i = 0; i < attributeIds.size(); ++i)
	{
		REQUIRE(attributeIds[i] != VertexAttributeId::Null);
		REQUIRE(boundStateIds[i] != StateValueId::Null);
	}

	// Vertex data produces a gradient within each triangle; instance data gives each triangle a different intensity.
	const std::vector<V1Float32> vertexData = {V1Float32(0.0f), V1Float32(0.5f), V1Float32(1.0f)};
	const std::vector<V1Float32> instanceData = {V1Float32(0.25f), V1Float32(0.625f), V1Float32(1.0f)};
	VertexAttribute<V1Float32> vertexAttribute(vertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	VertexAttribute<V1Float32> instanceAttribute(instanceData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	auto vertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttribute));
	REQUIRE(vertexAttribute.SetInitialData(vertexData));
	REQUIRE(vertexBuffer->AllocateMemory());
	auto instanceBuffer = renderer.CreateVertexBuffer();
	REQUIRE(instanceBuffer->BindVertexAttribute(instanceAttribute));
	REQUIRE(instanceAttribute.SetInitialData(instanceData));
	REQUIRE(instanceBuffer->AllocateMemory());

	auto groupNode = renderer.CreateStateGroupNode();
	groupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);
	auto programNode = renderer.CreateProgramNode();
	REQUIRE(programNode->BindShaderProgram(shaderProgram.get()));
	programNode->AddChildNode(groupNode.get());
	auto renderPassNode = renderer.CreateRenderPassNode();
	renderPassNode->BindFrameBuffer(frameBuffer.get());
	renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(1.0f, 1.0f, 1.0f, 1.0f));
	renderPassNode->AddChildNode(programNode.get());
	renderer.SetRenderPasses(&renderPassNode, 1);

	// A channel index of -1 leaves that binding absent. Use a fresh node so every case creates its own input layout.
	auto runCase = [&](const std::string& resultName, const std::string& resultDescription, int vertexChannel, int instanceChannel) {
		auto renderableNode = renderer.CreateRenderableNode();
		if (vertexChannel >= 0)
		{
			REQUIRE(renderableNode->BindVertexAttribute(vertexAttribute, attributeIds[vertexChannel]));
		}
		if (instanceChannel >= 0)
		{
			REQUIRE(renderableNode->BindVertexInstanceAttribute(instanceAttribute, attributeIds[instanceChannel]));
		}
		for (int i = 0; i < static_cast<int>(boundStateIds.size()); ++i)
		{
			renderableNode->SetStateValue(boundStateIds[i], i == vertexChannel || i == instanceChannel);
		}
		REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
		// Explicit counts are needed when there is no per-vertex attribute to infer a vertex count from.
		REQUIRE(renderableNode->SetVertexCount(vertexData.size()));
		REQUIRE(renderableNode->SetInstanceMode(static_cast<uint32_t>(instanceData.size())));
		groupNode->AddChildNode(renderableNode.get());

		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult(resultName, resultDescription, std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.99);
		groupNode->RemoveAllChildNodes();
	};

	runCase("NoAttributesBound", "Three black triangles on a grey background with all shader attributes unbound.", -1, -1);
	const std::array<std::string, 3> channelNames = {"R", "G", "B"};
	for (int channel = 0; channel < static_cast<int>(channelNames.size()); ++channel)
	{
		runCase("PerVertex" + channelNames[channel], "Three triangles with a per-vertex gradient in " + channelNames[channel] + " and the other two shader attributes unbound.", channel, -1);
		runCase("PerInstance" + channelNames[channel], "Three triangles with different per-instance intensities in " + channelNames[channel] + " and the other two shader attributes unbound.", -1, channel);
	}

	// Leave each shader input unbound in turn, assigning the other two inputs both ways and binding in both orders.
	for (int vertexChannel = 0; vertexChannel < static_cast<int>(channelNames.size()); ++vertexChannel)
	{
		for (int instanceChannel = 0; instanceChannel < static_cast<int>(channelNames.size()); ++instanceChannel)
		{
			if (vertexChannel == instanceChannel) continue;
			const std::string resultName = "PerVertex" + channelNames[vertexChannel] + "PerInstance" + channelNames[instanceChannel];
			const std::string resultDescription = "Three triangles with a per-vertex gradient in " + channelNames[vertexChannel] + " and per-instance intensities in " + channelNames[instanceChannel] + ", leaving the third shader attribute unbound. ";
			runCase(resultName, resultDescription, vertexChannel, instanceChannel);
		}
	}

	renderer.RemoveAllRenderPasses();
	return true;
}

} // namespace cobalt::graphics::testing
