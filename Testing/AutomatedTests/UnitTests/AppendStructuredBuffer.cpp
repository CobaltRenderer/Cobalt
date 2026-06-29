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

struct BufferEntryIn
{
	float4 otherData;
	float4 color;
	float4 someOtherData;
};
struct BufferEntryOut
{
	uint hasEntry;
	uint readIndexValue;
	uint dummyValue;
	uint dummyValue2;
	float4 color;
};

StructuredBuffer<BufferEntryIn> bufferDataIn;
AppendStructuredBuffer<BufferEntryOut> bufferDataOut;

float4 main(float4 position : SV_POSITION) : SV_TARGET0
{
	uint indexValue = uint(position.x) + ((flipVerticalAxis ? (screenSizeInPixels.y - 1) - uint(position.y) : uint(position.y)) * screenSizeInPixels.x);
	float4 myColor = bufferDataIn[indexValue].color;
	BufferEntryOut outEntry;
	outEntry.hasEntry = 1;
	outEntry.readIndexValue = indexValue;
	outEntry.dummyValue = 0xDEADBEEF;
	outEntry.dummyValue2 = 0xDEADBEEF;
	outEntry.color = myColor;
	bufferDataOut.Append(outEntry);
	return myColor;
}
)";

// Define our local structures
struct BufferEntryIn
{
	V4Float32 otherData;
	V4Float32 color;
	V4Float32 someOtherData;
};
struct BufferEntryOut
{
	unsigned int hasEntry;
	unsigned int readIndexValue;
	unsigned int dummyValue;
	unsigned int dummyValue2;
	V4Float32 color;
};
} // namespace

DEFINE_UNIT_TEST_WITH_BASE("Resources/DataArray/AppendStructuredBuffer", UnitTestBase)
{
	// Ensure data arrays are supported by the current renderer
	if (!session.Device().IsFeatureSupported(IGraphicsDevice::Feature::ResourceArrays))
	{
		session.AddTestSkipped("Skipped test", "This test was skipped, as the current renderer doesn't support data arrays on this device.");
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
	BufferEntryIn defaultVals{};
	defaultVals.otherData = V4Float32(1.0f, 0.0f, 0.0f, 1.0f);
	defaultVals.color = V4Float32(0.0f, 0.0f, 0.0f, 0.0f);
	defaultVals.someOtherData = V4Float32(0.0f, 1.0f, 0.0f, 1.0f);
	std::vector<BufferEntryIn> inputData;
	inputData.resize((size_t)windowSizeInPixels.X() * (size_t)windowSizeInPixels.Y(), defaultVals);
	for (unsigned int posY = 0; posY < windowSizeInPixels.Y(); ++posY)
	{
		for (unsigned int posX = 0; posX < windowSizeInPixels.X(); ++posX)
		{
			inputData[posX + (posY * windowSizeInPixels.X())].color = V4Float32((float)posX / (float)windowSizeInPixels.X(), (float)posY / (float)windowSizeInPixels.Y(), 1.0f, 1.0f);
		}
	}

	// Create our data array
	auto dataArrayIn = renderer.CreateDataArray();
	size_t structureSize = sizeof(BufferEntryIn);
	dataArrayIn->SetBufferLayout(structureSize, inputData.size());
	REQUIRE(dataArrayIn->SetInitialData(inputData.data(), inputData.size() * structureSize));
	REQUIRE(dataArrayIn->AllocateMemory());

	// Create our output data array
	auto dataArrayOut = renderer.CreateDataArray();
	size_t structureSizeOut = sizeof(BufferEntryOut);
	dataArrayOut->SetBufferLayout(structureSizeOut, inputData.size(), true);
	REQUIRE(dataArrayOut->AllocateMemory());

	// Create our output data array
	auto dataArrayCapturedOutput = renderer.CreateDataArrayOutput();
	dataArrayOut->AddOutputCaptureTarget(dataArrayCapturedOutput.get());

	// Retrieve our shader attribute IDs
	auto positionAttributeId = shaderProgram->GetVertexAttributeId("position");
	auto screenSizeInPixelsStateId = shaderProgram->GetStateValueId("screenSizeInPixels");
	auto flipVerticalAxisStateId = shaderProgram->GetStateValueId("flipVerticalAxis");
	auto dataArrayInId = shaderProgram->GetResourceArrayId("bufferDataIn");
	auto dataArrayOutId = shaderProgram->GetResourceArrayId("bufferDataOut");

	// Create our state group node
	auto groupNode = renderer.CreateStateGroupNode();
	groupNode->SetStateValue(screenSizeInPixelsStateId, windowSizeInPixels);
	groupNode->SetStateValue(flipVerticalAxisStateId, session.ApiFamily() == IRendererPlugin::ApiFamily::OpenGL);
	groupNode->BindResourceArray(dataArrayInId, dataArrayIn.get());
	groupNode->BindResourceArray(dataArrayOutId, dataArrayOut.get());

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
	Geometry().CreatePrimitiveSquareAsTriangles(40, positionVertexData, vertexNormals, textureCoords, indexData);

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

	// Ensure the correct image is drawn
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("SquareColorGradient", "A square with a color gradient", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Ensure the write buffer was modified and captured correctly
	REQUIRE(dataArrayCapturedOutput->HasCapturedOutput());
	REQUIRE(dataArrayCapturedOutput->HasCapturedCounterValue());
	uint32_t counterValue;
	REQUIRE(dataArrayCapturedOutput->ReadCounterValue(counterValue));
	REQUIRE(counterValue == (windowSizeInPixels.X() * windowSizeInPixels.Y()));
	std::vector<BufferEntryOut> outputData;
	outputData.resize(counterValue, BufferEntryOut{});
	REQUIRE(dataArrayCapturedOutput->ReadBufferData(outputData.data(), outputData.size() * sizeof(BufferEntryOut)));
	for (size_t i = 0; i < counterValue; ++i)
	{
		const auto& entry = outputData[i];
		REQUIRE(entry.hasEntry != 0);
		REQUIRE(entry.readIndexValue < inputData.size());
		REQUIRE(entry.color == inputData[entry.readIndexValue].color);
		REQUIRE(entry.dummyValue == 0xDEADBEEF);
	}
	session.AddTestSuccess("DataValueMatch", "Testing the output array to ensure it is valid, complete, and matches the input data.");

	// Remove the renderable node
	groupNode->RemoveChildNode(renderableNode.get());

	// Remove all our defined render passes
	renderer.RemoveAllRenderPasses();
	return true;
}

} // namespace cobalt::graphics::testing
