// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../UnitTestBase.h"
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
uniform uint2 screenSizeInPixels;
uniform bool flipVerticalAxis;

Buffer<float4> bufferData;

float4 main(float4 position : SV_POSITION) : SV_TARGET0
{
	uint indexValue = uint(position.x) + ((flipVerticalAxis ? (screenSizeInPixels.y - 1) - uint(position.y) : uint(position.y)) * screenSizeInPixels.x);
	float4 myColor = bufferData[indexValue];
	return myColor;
}
)";
} // namespace

DEFINE_UNIT_TEST_WITH_BASE("Resources/TexelArray/Buffer", UnitTestBase)
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
	auto& testWindowPlatformInfo = *session.TestWindowPlatformInfo();

	// Define the framebuffer
	auto windowSizeInPixels = session.TestWindowSize();
	auto testWindowFrameBuffer = renderer.CreateFrameBuffer();
	testWindowFrameBuffer->DefineViewportRegion(V2UInt32(0, 0), windowSizeInPixels);
	REQUIRE(uiThread.InvokeSync([&] { return testWindowFrameBuffer->BindWindow(testWindowPlatformInfo, IFrameBuffer::WindowDepthStencilMode::DepthUNorm24); }));

	// Create and compile our shader program
	auto shaderProgram = renderer.CreateShaderProgram();
	REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(VertexShader)));
	REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(FragmentShader)));
	REQUIRE(shaderProgram->CompileProgram());

	// Create the initial data for our data array
	std::vector<V4Float32> inputData;
	inputData.resize((size_t)windowSizeInPixels.X() * (size_t)windowSizeInPixels.Y());
	for (unsigned int posY = 0; posY < windowSizeInPixels.Y(); ++posY)
	{
		for (unsigned int posX = 0; posX < windowSizeInPixels.X(); ++posX)
		{
			inputData[posX + (posY * windowSizeInPixels.X())] = V4Float32((float)posX / (float)windowSizeInPixels.X(), (float)posY / (float)windowSizeInPixels.Y(), 1.0f, 1.0f);
		}
	}

	// Create our data array
	auto dataArray = renderer.CreateTexelArray();
	dataArray->SetBufferLayout(ITexelArray::ImageFormat::RGBA, ITexelArray::DataFormat::Float32, inputData.size());
	REQUIRE(dataArray->SetInitialData(inputData));
	REQUIRE(dataArray->AllocateMemory());

	// Retrieve our shader attribute IDs
	auto positionAttributeId = shaderProgram->GetVertexAttributeId("position");
	auto screenSizeInPixelsStateId = shaderProgram->GetStateValueId("screenSizeInPixels");
	auto flipVerticalAxisStateId = shaderProgram->GetStateValueId("flipVerticalAxis");
	auto dataArrayId = shaderProgram->GetResourceArrayId("bufferData");

	// Create our state group node
	auto groupNode = renderer.CreateStateGroupNode();
	groupNode->SetStateValue(screenSizeInPixelsStateId, windowSizeInPixels);
	groupNode->SetStateValue(flipVerticalAxisStateId, session.ApiFamily() == IRendererPlugin::ApiFamily::OpenGL);
	groupNode->BindResourceArray(dataArrayId, dataArray.get());

	// Disable backface culling since our strip data reverses winding order each primitive
	groupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);

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
	renderer.SetRenderPasses(&renderPassNode, 1);

	// Retrieve the primitive data
	std::vector<V3Float32> positionVertexData;
	std::vector<V3Float32> vertexNormals;
	std::vector<V2Float32> textureCoords;
	std::vector<V1UInt32> indexData;
	Geometry().CreatePrimitiveCircleAsTriangles(40, positionVertexData, vertexNormals, textureCoords, indexData);

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
	REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));

	// Add our renderable node to the scene
	groupNode->AddChildNode(renderableNode.get());
	IFrameBufferOutput::unique_ptr frameBufferCapture;

	// Ensure the complete circle is drawn by default
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("CircleColorGradient", "A circle with a color gradient", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Remove the renderable node
	groupNode->RemoveChildNode(renderableNode.get());

	// Remove all our defined render passes
	renderer.RemoveAllRenderPasses();
	return true;
}

} // namespace cobalt::graphics::testing
