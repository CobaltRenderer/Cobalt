// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../UnitTestBase.h"
namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

// Define our shader programs
const std::string VertexShader = R"(
struct VSInput
  {
	float3 position : position;
    uint color : color;
};

struct VSOutput
  {
    float4 position : SV_POSITION;
    float3 color : COLOR;
};

VSOutput main(VSInput IN)
{
    VSOutput OUT;

    OUT.position = float4(IN.position, 1.0f);
    OUT.color = float3(float((IN.color >> 4) & 0x03) / 3.0f, float((IN.color >> 2) & 0x03) / 3.0f, float(IN.color & 0x03) / 3.0f);

    return OUT;
}
)";
const std::string FragmentShader = R"(
struct VSOutput
  {
    float4 position : SV_POSITION;
    float3 color : COLOR;
};

float4 main(VSOutput IN) : SV_TARGET0
{
    return float4(IN.color, 1.0f);
}
)";

struct VertexEntry
{
	V3Norm8 position;
	V1UInt8 color;
};
struct IndexEntry
{
	V1UInt16 index;
	V1UInt16 padding;
};

DEFINE_UNIT_TEST_WITH_BASE("Resources/VertexBuffer/ManualVertexBufferLayout", UnitTestBase)
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
	REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(VertexShader)));
	REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(FragmentShader)));
	REQUIRE(shaderProgram->CompileProgram());

	// Retrieve our shader attribute IDs
	auto positionAttributeId = shaderProgram->GetVertexAttributeId("position");
	auto colorAttributeId = shaderProgram->GetVertexAttributeId("color");

	// Create our vertex buffer and populate it with data
	//##TODO## This fails on older Intel hardware for Apple systems under MoltenVK (as of 2026-01-14), even though it
	//should theoretically work. The offset for the color is apparently being ignored, and treated as though it's 0.
	//Although MoltenVK requires a minimum stride of 4 bytes between vertex elements, a limit it inherits from Metal, it
	//is NOT limited to 4-byte aligned offsets, they just have to be a multiple of the component size, so for a 1-byte
	//component, an offset of 3 bytes here for color should be perfectly legal. We should report this to the MoltenVK
	//maintainers, or submit a patch ourselves.
	std::vector<VertexEntry> vertexData;
	VertexEntry vertexEntry{};
	vertexEntry.position = V3Norm8(BasicNorm8{0xC1}, BasicNorm8{0x3F}, BasicNorm8{0x3F});
	vertexEntry.color = V1UInt8(0x30);
	vertexData.push_back(vertexEntry);
	vertexEntry.position = V3Norm8(BasicNorm8{0x00}, BasicNorm8{0xC1}, BasicNorm8{0x3F});
	vertexEntry.color = V1UInt8(0x0C);
	vertexData.push_back(vertexEntry);
	vertexEntry.position = V3Norm8(BasicNorm8{0x3F}, BasicNorm8{0x3F}, BasicNorm8{0x3F});
	vertexEntry.color = V1UInt8(0x03);
	vertexData.push_back(vertexEntry);

	RawVertexAttribute vertexAttributePosition(IVertexAttribute::DataType::Norm8, 3, vertexData.size(), IVertexAttribute::PerformanceHint::WriteRarely | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	RawVertexAttribute vertexAttributeColor(IVertexAttribute::DataType::UInt8, 1, vertexData.size(), IVertexAttribute::PerformanceHint::WriteRarely | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	auto vertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE(vertexBuffer->BindVertexAttributeManualLayout(vertexAttributePosition, 0, 4));
	REQUIRE(vertexAttributePosition.SetInitialData(reinterpret_cast<const uint8_t*>(vertexData.data()), vertexData.size(), sizeof(VertexEntry)));
	REQUIRE(vertexBuffer->BindVertexAttributeManualLayout(vertexAttributeColor, 3, 4));
	REQUIRE(vertexAttributeColor.SetInitialData(reinterpret_cast<const uint8_t*>(vertexData.data()) + sizeof(V3Norm8), vertexData.size(), sizeof(VertexEntry)));
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

	// Now set the middle colour to white
	V1UInt8 white(0x3F);
	REQUIRE(vertexBuffer->QueueRawDataUpdate(reinterpret_cast<const uint8_t*>(&white), sizeof(V1UInt8), 7));

	// And capture the scene with the white point
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("RWBTriangleDown", "Replaced green with white", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Create an alternate vertex buffer that we populate using a raw initial data write to the buffer
	RawVertexAttribute vertexAttributePosition2(IVertexAttribute::DataType::Norm8, 3, vertexData.size(), IVertexAttribute::PerformanceHint::WriteRarely | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	RawVertexAttribute vertexAttributeColor2(IVertexAttribute::DataType::UInt8, 1, vertexData.size(), IVertexAttribute::PerformanceHint::WriteRarely | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	auto vertexBuffer2 = renderer.CreateVertexBuffer();
	REQUIRE(vertexBuffer2->BindVertexAttributeManualLayout(vertexAttributePosition2, 0, 4));
	REQUIRE(vertexBuffer2->BindVertexAttributeManualLayout(vertexAttributeColor2, 3, 4));
	REQUIRE(vertexBuffer2->SetRawInitialData(reinterpret_cast<const uint8_t*>(vertexData.data()), vertexData.size() * sizeof(VertexEntry)));
	REQUIRE(vertexBuffer2->AllocateMemory());

	// Re-create our renderable node
	groupNode->RemoveChildNode(renderableNode.get());
	renderableNode = renderer.CreateRenderableNode();
	REQUIRE(renderableNode->BindVertexAttribute(vertexAttributePosition2, positionAttributeId));
	REQUIRE(renderableNode->BindVertexAttribute(vertexAttributeColor2, colorAttributeId));
	REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
	groupNode->AddChildNode(renderableNode.get());

	// Capture an image of the scene
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("RGBTriangleDown2", "A red, green and blue triangle, pointing down", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Now set the middle colour to white
	REQUIRE(vertexBuffer2->QueueRawDataUpdate(reinterpret_cast<const uint8_t*>(&white), sizeof(V1UInt8), 7));

	// And capture the scene with the white point
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("RWBTriangleDown2", "Replaced green with white", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Index buffers only support tightly packed layouts, so padded manual index layouts must be rejected.
	RawIndexAttribute paddedIndexAttribute(IIndexAttribute::DataType::UInt16, 3, IIndexAttribute::PerformanceHint::WriteRarely | IIndexAttribute::PerformanceHint::ReadNever, IIndexAttribute::PerformanceHint::WriteNever | IIndexAttribute::PerformanceHint::ReadOften);
	auto paddedIndexBuffer = renderer.CreateIndexBuffer();
	REQUIRE(!paddedIndexBuffer->BindIndexAttributeManualLayout(paddedIndexAttribute, 0, sizeof(IndexEntry)));
	session.AddTestSuccess("PaddedManualIndexLayoutRejected", "Index buffers rejected a padded manual layout, as only tightly packed index layouts are supported.");

	// Create an index buffer that uses a raw initial data write with a tightly packed manual layout.
	std::vector<V1UInt16> rawIndexData = {
	  V1UInt16(0),
	  V1UInt16(1),
	  V1UInt16(2),
	};
	RawIndexAttribute rawIndexAttribute(IIndexAttribute::DataType::UInt16, rawIndexData.size(), IIndexAttribute::PerformanceHint::WriteRarely | IIndexAttribute::PerformanceHint::ReadNever, IIndexAttribute::PerformanceHint::WriteNever | IIndexAttribute::PerformanceHint::ReadOften);
	auto rawIndexBuffer = renderer.CreateIndexBuffer();
	REQUIRE(rawIndexBuffer->BindIndexAttributeManualLayout(rawIndexAttribute, 0, sizeof(V1UInt16)));
	REQUIRE(rawIndexBuffer->SetRawInitialData(reinterpret_cast<const uint8_t*>(rawIndexData.data()), rawIndexData.size() * sizeof(V1UInt16)));
	REQUIRE(rawIndexBuffer->AllocateMemory());

	// Re-create the renderable using the raw index buffer so the index data path is used for drawing.
	groupNode->RemoveChildNode(renderableNode.get());
	renderableNode = renderer.CreateRenderableNode();
	REQUIRE(renderableNode->BindVertexAttribute(vertexAttributePosition2, positionAttributeId));
	REQUIRE(renderableNode->BindVertexAttribute(vertexAttributeColor2, colorAttributeId));
	REQUIRE(renderableNode->BindIndexAttribute(rawIndexAttribute));
	REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
	groupNode->AddChildNode(renderableNode.get());

	// Capture an image proving that the raw index buffer initial data produces the expected indexed triangle.
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("RawIndexBufferInitialData", "A red, white and blue indexed triangle, using raw index buffer initial data", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Update the raw index buffer to a degenerate triangle and verify the raw update path takes effect.
	std::vector<V1UInt16> degenerateRawIndexData = {
	  V1UInt16(0),
	  V1UInt16(0),
	  V1UInt16(0),
	};
	REQUIRE(rawIndexBuffer->QueueRawDataUpdate(reinterpret_cast<const uint8_t*>(degenerateRawIndexData.data()), degenerateRawIndexData.size() * sizeof(V1UInt16), 0));
	frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	mainWindowFrameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("RawIndexBufferDegenerateUpdate", "A black frame after updating the raw index buffer to describe a degenerate triangle", std::move(frameBufferCapture), IImageDiff::Algorithm::AllOfThem, 0.95);

	// Remove all our defined render passes
	groupNode->RemoveAllChildNodes();
	renderer.RemoveAllRenderPasses();
	return true;
}

} // namespace cobalt::graphics::testing
