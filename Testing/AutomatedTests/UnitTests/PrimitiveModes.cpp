// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../UnitTestBase.h"
namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

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
  return float4(0.0, 1.0, 0.0, 1.0);
}
)";

DEFINE_UNIT_TEST_WITH_BASE("Renderable/PrimitiveModes", UnitTestBase)
{
	// Retrieve our important objects
	auto& renderer = session.Renderer();
	auto& uiThread = session.UIThread();
	auto& testWindowPlatformInfo = *session.TestWindowPlatformInfo();

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

	// Draw a blank screen
	{
		// Attach a framebuffer output capture target for our screenshot
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);

		// Capture a screenshot of the scene
		DrawOneFrame();
		session.AddTestImageResult("BlankScreen", "Totally black screen", std::move(frameBufferCapture));
	}

	// Draw a sphere made up of green points
	{
		// Retrieve the primitive data
		std::vector<V3Float32> positionVertexData;
		std::vector<V1UInt32> indexData;
		Geometry().CreatePrimitiveSphereAsPoints(100, 100, positionVertexData, indexData);

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
		REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Points));

		// Add our renderable node to the scene
		groupNode->AddChildNode(renderableNode.get());

		// Capture an image of the scene
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("GreenSphereAsPoints", "A sphere made up of green points", std::move(frameBufferCapture));

		// Remove the renderable node
		groupNode->RemoveChildNode(renderableNode.get());
	}

	// Draw a sphere made up of green lines
	{
		// Retrieve the primitive data
		std::vector<V3Float32> positionVertexData;
		std::vector<V1UInt32> indexData;
		Geometry().CreatePrimitiveSphereAsLines(100, 100, positionVertexData, indexData);

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
		REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Lines));

		// Add our renderable node to the scene
		groupNode->AddChildNode(renderableNode.get());

		// Capture an image of the scene
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("GreenSphereAsLines", "A sphere made up of green lines", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

		// Remove the renderable node
		groupNode->RemoveChildNode(renderableNode.get());
	}

	// Draw a circle made up of a green line strip
	{
		// Retrieve the primitive data
		std::vector<V3Float32> positionVertexData;
		std::vector<V1UInt32> indexData;
		Geometry().CreatePrimitiveCircleAsLineStrip(100, positionVertexData, indexData);

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

		// Add our renderable node to the scene
		groupNode->AddChildNode(renderableNode.get());

		// Capture an image of the scene
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("GreenCircleAsLineStrip", "A circle made up of a green line strip", std::move(frameBufferCapture));

		// Remove the renderable node
		groupNode->RemoveChildNode(renderableNode.get());
	}

	// Draw a sphere made up of green triangles
	{
		// Retrieve the primitive data
		std::vector<V3Float32> positionVertexData;
		std::vector<V3Float32> normalVertexData;
		std::vector<V2Float32> texCoordData;
		std::vector<V1UInt32> indexData;
		Geometry().CreatePrimitiveSphereAsTriangles(20, 20, positionVertexData, normalVertexData, texCoordData, indexData);

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

		// Capture an image of the scene
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("GreenSphereAsTriangles", "A sphere made up of green triangles", std::move(frameBufferCapture));

		// Remove the renderable node
		groupNode->RemoveChildNode(renderableNode.get());
	}

	// Draw a square made up of a green triangle strip
	{
		// Retrieve the primitive data
		std::vector<V3Float32> positionVertexData;
		std::vector<V3Float32> normalVertexData;
		std::vector<V2Float32> texCoordData;
		std::vector<V1UInt32> indexData;
		Geometry().CreatePrimitiveSquareAsTriangleStrip(10, positionVertexData, normalVertexData, texCoordData, indexData);

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
		REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::TriangleStrip));

		// Add our renderable node to the scene
		groupNode->AddChildNode(renderableNode.get());

		// Capture an image of the scene
		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		testWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult("GreenSquareAsTriangleStrip", "A square made up of a green triangle strip", std::move(frameBufferCapture));

		// Remove the renderable node
		groupNode->RemoveChildNode(renderableNode.get());
	}

	// Remove all our defined render passes
	renderer.RemoveAllRenderPasses();
	return true;
}

} // namespace cobalt::graphics::testing
