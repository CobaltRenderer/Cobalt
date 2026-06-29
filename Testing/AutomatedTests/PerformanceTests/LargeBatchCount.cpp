// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../GeometryHelper.h"
#include "../PerformanceTestBase.h"
#include "../PseudoRandomGenerator.h"
namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

// Define our shader programs
const std::string VertexShader = R"(
uniform row_major float4x4 model;

struct VSInput {
    float3 position : position;
    float3 normal : normal;
};

struct VSOutput {
    float4 position : SV_POSITION;
    float3 normal : normal;
};

uniform row_major float4x4 viewProj;

VSOutput main(VSInput IN)
{
    VSOutput OUT;

    float4 modelPosition = mul(model, float4(IN.position, 1.0));
    OUT.position = mul(viewProj, modelPosition);
    OUT.normal = IN.normal;

    return OUT;
}
)";
const std::string FragmentShader = R"(
uniform float3 objectColor;

struct VSOutput {
    float4 position : SV_POSITION;
    float3 normal : normal;
};

float4 main(VSOutput IN) : SV_TARGET0
{
    float4 color = float4(objectColor, 1.0f);
    color.rgb *= abs(dot(IN.normal, normalize(float3(3.0f,-4.0f, 5.0f))));
    return color;
}
)";

DEFINE_UNIT_TEST_WITH_BASE("Performance/LargeBatchCount", PerformanceTestBase)
{
	// Retrieve our important objects
	auto& renderer = session.Renderer();
	auto& uiThread = session.UIThread();
	auto& testWindowInfo = *session.TestWindowPlatformInfo();

	// Define the framebuffer
	auto mainWindowFrameBuffer = renderer.CreateFrameBuffer();
	mainWindowFrameBuffer->DefineViewportRegion(V2UInt32(0, 0), session.TestWindowSize());
	REQUIRE(uiThread.InvokeSync([&] { return mainWindowFrameBuffer->BindWindow(testWindowInfo, IFrameBuffer::WindowDepthStencilMode::DepthUNorm24, IFrameBuffer::WindowColorSpaceMode::Default, IFrameBuffer::WindowBindingFlags::AllowTearing); }));

	// Create and compile our shader program
	auto shaderProgram = renderer.CreateShaderProgram();
	REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(VertexShader)));
	REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(FragmentShader)));
	REQUIRE(shaderProgram->CompileProgram());

	// Retrieve our shader attribute IDs
	auto positionAttributeId = shaderProgram->GetVertexAttributeId("position");
	auto normalAttributeId = shaderProgram->GetVertexAttributeId("normal");
	auto modelStateId = shaderProgram->GetStateValueId("model");
	auto objectColorStateId = shaderProgram->GetStateValueId("objectColor");

	// Create our state group node
	auto groupNode = renderer.CreateStateGroupNode();

	auto viewProj = Transform().LookAtCenterPerspective(session.TestWindowSizeAsFloat(), V3Float32(1.5f, 2.0f, 2.5f));
	groupNode->SetStateValue(shaderProgram->GetStateValueId("viewProj"), viewProj);

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

	// Create 100000 sphere objects with 98 triangles per sphere, with separate buffers and renderable objects, and unique
	// per-object state. This will stress test the ability of the main render loop to process and dispatch a large number
	// of primitives per frame, and handle a large volume of state changes mid frame. This is ideal for helping optimise
	// the innermost level of the main render loop, and the draw function of the renderable node.
	size_t primitiveCount = 100000;
	uint32_t vertexCountWidth = 7;
	uint32_t vertexCountHeight = 7;

	// Generate data for the object we want to render
	std::vector<V3Float32> positionVertexData;
	std::vector<V3Float32> normalVertexData;
	std::vector<V2Float32> texCoordData;
	std::vector<V1UInt32> indexData;
	Geometry().CreatePrimitiveSphereAsTriangles(vertexCountWidth, vertexCountHeight, positionVertexData, normalVertexData, texCoordData, indexData);

	// Create our vertex buffer and populate it with data
	VertexAttribute<V3Float32> vertexAttributePosition(positionVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	VertexAttribute<V3Float32> vertexAttributeNormal(normalVertexData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	auto vertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttributePosition));
	REQUIRE(vertexAttributePosition.SetInitialData(positionVertexData));
	REQUIRE(vertexBuffer->BindVertexAttribute(vertexAttributeNormal));
	REQUIRE(vertexAttributeNormal.SetInitialData(normalVertexData));
	REQUIRE(vertexBuffer->AllocateMemory());

	// Create our index buffer and populate it with data
	IndexAttribute<V1UInt32> indexAttribute(indexData.size(), IIndexAttribute::PerformanceHint::WriteNever | IIndexAttribute::PerformanceHint::ReadNever, IIndexAttribute::PerformanceHint::WriteNever | IIndexAttribute::PerformanceHint::ReadOften);
	auto indexBuffer = renderer.CreateIndexBuffer();
	REQUIRE(indexBuffer->BindIndexAttribute(indexAttribute));
	REQUIRE(indexAttribute.SetInitialData(indexData));
	REQUIRE(indexBuffer->AllocateMemory());

	//std::vector<IVertexBuffer::unique_ptr> vertexBuffers(primitiveCount);
	//std::vector<IIndexBuffer::unique_ptr> indexBuffers(primitiveCount);
	std::vector<IRenderableNode::unique_ptr> renderables(primitiveCount);
	PseudoRandomGenerator randomGenerator;
	for (size_t i = 0; i < primitiveCount; ++i)
	{
		// Create our renderable node
		auto renderableNode = renderer.CreateRenderableNode();
		REQUIRE(renderableNode->BindVertexAttribute(vertexAttributePosition, positionAttributeId));
		REQUIRE(renderableNode->BindVertexAttribute(vertexAttributeNormal, normalAttributeId));
		REQUIRE(renderableNode->BindIndexAttribute(indexAttribute));
		REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
		groupNode->AddChildNode(renderableNode.get());

		auto modelMatrix = Transform().GetRandomModelTransform(randomGenerator);
		renderableNode->SetStateValue(modelStateId, modelMatrix);
		const auto red = randomGenerator.GetNextNormalized();
		const auto green = randomGenerator.GetNextNormalized();
		const auto blue = randomGenerator.GetNextNormalized();
		V3Float32 objectColor(red, green, blue);
		renderableNode->SetStateValue(objectColorStateId, objectColor);

		//vertexBuffers[i]= std::move(vertexBuffer);
		//indexBuffers[i]= std::move(indexBuffer);
		renderables[i] = std::move(renderableNode);
	}

	// Profile our rendering performance
	auto profileResults = DrawFramesAndProfile(std::chrono::seconds(10));
	session.AddTestProfileResult("LargeBatchCount", "100000 spheres with 98 triangles each", profileResults);

	// Remove all our defined render passes
	renderer.RemoveAllRenderPasses();
	return true;
}

} // namespace cobalt::graphics::testing
