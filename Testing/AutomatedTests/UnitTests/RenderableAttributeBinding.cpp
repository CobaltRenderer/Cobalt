// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../UnitTestBase.h"
#include <array>
#include <memory>
#include <vector>
namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

namespace {

// Define our shader programs
const std::string VertexShader = R"(
struct VSInput {
    float4 position : position;
};

float4 main(VSInput IN) : SV_POSITION
{
    return IN.position;
}
)";
const std::string FragmentShader = R"(
uniform float4 drawColor;

float4 main() : SV_TARGET0
{
    return drawColor;
}
)";

struct AttributeFormatCase
{
	IVertexAttribute::DataType dataType;
	size_t elementCount;
};

} // namespace

DEFINE_UNIT_TEST_WITH_BASE("Renderable/AttributeBinding", UnitTestBase)
{
	// Retrieve our important objects
	auto& renderer = session.Renderer();
	auto& uiThread = session.UIThread();
	auto& testWindowInfo = *session.TestWindowPlatformInfo();
	const auto frameBufferSize = session.TestWindowSize();

	// Define the framebuffer
	auto frameBuffer = renderer.CreateFrameBuffer();
	frameBuffer->DefineViewportRegion(V2UInt32(0, 0), frameBufferSize);
	REQUIRE(uiThread.InvokeSync([&] { return frameBuffer->BindWindow(testWindowInfo, IFrameBuffer::WindowDepthStencilMode::None); }));

	// Create and compile our shader program
	auto shaderProgram = renderer.CreateShaderProgram();
	REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(VertexShader)));
	REQUIRE(shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(FragmentShader)));
	REQUIRE(shaderProgram->CompileProgram());

	// Retrieve our shader attribute and state IDs
	auto positionAttributeId = shaderProgram->GetVertexAttributeId("position");
	auto drawColorStateId = shaderProgram->GetStateValueId("drawColor");

	// Define our indexed quad data and persistence flags
	const auto vertexPersistence = IVertexAttribute::DataPersistenceFlags::InvalidateExistingDataOnWrite | IVertexAttribute::DataPersistenceFlags::InvalidateExistingDataAfterDrawComplete;
	const auto indexPersistence = IIndexAttribute::DataPersistenceFlags::InvalidateExistingDataOnWrite | IIndexAttribute::DataPersistenceFlags::InvalidateExistingDataAfterDrawComplete;
	std::vector<V4Float32> leftQuadData;
	std::vector<V4Float32> centerQuadData;
	std::vector<V4Float32> rightQuadData;
	std::vector<V4Float32> diagnosticQuadData;
	std::vector<V1UInt16> leftIndexData;
	std::vector<V1UInt32> centerIndexData;
	std::vector<V1UInt16> rightIndexData;
	Geometry().CreateIndexedQuad(0.5f, -0.95f, -0.40f, -0.70f, 0.70f, leftQuadData, leftIndexData);
	Geometry().CreateIndexedQuad(0.5f, -0.25f, 0.25f, -0.70f, 0.70f, centerQuadData, centerIndexData);
	Geometry().CreateIndexedQuad(0.5f, 0.40f, 0.95f, -0.70f, 0.70f, rightQuadData, rightIndexData);
	Geometry().CreateCenteredQuad(0.5f, 0.45f, diagnosticQuadData);

	// Create raw, normal, and read-only source attributes
	RawVertexAttribute leftPositionAttribute(IVertexAttribute::DataType::Float32, 4, leftQuadData.size(), IVertexAttribute::PerformanceHint::WriteRarely | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften, vertexPersistence);
	VertexAttribute<V4Float32> centerPositionAttribute(centerQuadData.size(), IVertexAttribute::PerformanceHint::WriteRarely | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften, vertexPersistence);
	RawVertexAttribute rightPositionAttribute(IVertexAttribute::DataType::Float32, 4, rightQuadData.size(), IVertexAttribute::PerformanceHint::WriteRarely | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften, vertexPersistence);
	RawIndexAttribute leftIndexAttribute(IIndexAttribute::DataType::UInt16, leftIndexData.size(), IIndexAttribute::PerformanceHint::WriteRarely | IIndexAttribute::PerformanceHint::ReadNever, IIndexAttribute::PerformanceHint::WriteNever | IIndexAttribute::PerformanceHint::ReadOften, indexPersistence);
	IndexAttribute<V1UInt32> centerIndexAttribute(centerIndexData.size(), IIndexAttribute::PerformanceHint::WriteRarely | IIndexAttribute::PerformanceHint::ReadNever, IIndexAttribute::PerformanceHint::WriteNever | IIndexAttribute::PerformanceHint::ReadOften, indexPersistence);
	RawIndexAttribute rightIndexAttribute(IIndexAttribute::DataType::UInt16, rightIndexData.size(), IIndexAttribute::PerformanceHint::WriteRarely | IIndexAttribute::PerformanceHint::ReadNever, IIndexAttribute::PerformanceHint::WriteNever | IIndexAttribute::PerformanceHint::ReadOften, indexPersistence);

	// Create our vertex buffer and populate it with data
	auto vertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE(vertexBuffer->BindVertexAttribute(leftPositionAttribute));
	REQUIRE(leftPositionAttribute.SetInitialData(reinterpret_cast<const uint8_t*>(leftQuadData.data()), leftQuadData.size(), sizeof(V4Float32)));
	REQUIRE(vertexBuffer->BindVertexAttribute(centerPositionAttribute));
	REQUIRE(centerPositionAttribute.SetInitialData(centerQuadData));
	REQUIRE(vertexBuffer->BindVertexAttribute(rightPositionAttribute));
	REQUIRE(rightPositionAttribute.SetInitialData(reinterpret_cast<const uint8_t*>(rightQuadData.data()), rightQuadData.size(), sizeof(V4Float32)));
	REQUIRE(vertexBuffer->AllocateMemory());

	// Create our index buffers and populate them with data
	auto leftIndexBuffer = renderer.CreateIndexBuffer();
	REQUIRE(leftIndexBuffer->BindIndexAttribute(leftIndexAttribute));
	REQUIRE(leftIndexAttribute.SetInitialData(reinterpret_cast<const uint8_t*>(leftIndexData.data()), leftIndexData.size(), sizeof(V1UInt16)));
	REQUIRE(leftIndexBuffer->AllocateMemory());
	auto centerIndexBuffer = renderer.CreateIndexBuffer();
	REQUIRE(centerIndexBuffer->BindIndexAttribute(centerIndexAttribute));
	REQUIRE(centerIndexAttribute.SetInitialData(centerIndexData));
	REQUIRE(centerIndexBuffer->AllocateMemory());
	auto rightIndexBuffer = renderer.CreateIndexBuffer();
	REQUIRE(rightIndexBuffer->BindIndexAttribute(rightIndexAttribute));
	REQUIRE(rightIndexAttribute.SetInitialData(reinterpret_cast<const uint8_t*>(rightIndexData.data()), rightIndexData.size(), sizeof(V1UInt16)));
	REQUIRE(rightIndexBuffer->AllocateMemory());

	// Diagnostic renders use a non-indexed six-vertex quad so they only validate the contract section which just ran.
	VertexAttribute<V4Float32> diagnosticPositionAttribute(diagnosticQuadData.size(), IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	auto diagnosticVertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE(diagnosticVertexBuffer->BindVertexAttribute(diagnosticPositionAttribute));
	REQUIRE(diagnosticPositionAttribute.SetInitialData(diagnosticQuadData));
	REQUIRE(diagnosticVertexBuffer->AllocateMemory());

	// Draw a small diagnostic image after each contract-only section so these cases have reference images too.
	auto addDiagnosticImage = [&](const std::string& resultName, const std::string& resultDescription, const V4Float32& resultColor) {
		auto renderableNode = renderer.CreateRenderableNode();
		REQUIRE(renderableNode->BindVertexAttribute(diagnosticPositionAttribute, positionAttributeId));
		REQUIRE(renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
		renderableNode->SetStateValue(drawColorStateId, resultColor);

		auto groupNode = renderer.CreateStateGroupNode();
		groupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);
		groupNode->AddChildNode(renderableNode.get());

		auto programNode = renderer.CreateProgramNode();
		REQUIRE(programNode->BindShaderProgram(shaderProgram.get()));
		programNode->AddChildNode(groupNode.get());

		auto renderPassNode = renderer.CreateRenderPassNode();
		renderPassNode->BindFrameBuffer(frameBuffer.get());
		renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
		renderPassNode->AddChildNode(programNode.get());
		renderer.SetRenderPasses(&renderPassNode, 1);

		auto frameBufferCapture = renderer.CreateFrameBufferOutput();
		frameBufferCapture->SetDetachAfterCapture(true);
		frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
		DrawOneFrame();
		session.AddTestImageResult(resultName, resultDescription, std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.98);
		renderer.RemoveAllRenderPasses();
	};

	// Verify negative shader and attribute binding contracts before the valid renderables are added to the tree.
	auto shaderProgramBindingContractNode = renderer.CreateProgramNode();
	REQUIRE(!shaderProgramBindingContractNode->BindShaderProgram(nullptr));
	REQUIRE(shaderProgramBindingContractNode->BindShaderProgram(shaderProgram.get()));
	REQUIRE(!shaderProgramBindingContractNode->BindShaderProgram(shaderProgram.get()));

	auto invalidShaderAttributeNode = renderer.CreateRenderableNode();
	REQUIRE(!invalidShaderAttributeNode->BindVertexAttribute(leftPositionAttribute, VertexAttributeId::Null));
	REQUIRE(!invalidShaderAttributeNode->BindVertexInstanceAttribute(leftPositionAttribute, VertexAttributeId::Null));

	auto duplicateShaderAttributeNode = renderer.CreateRenderableNode();
	REQUIRE(duplicateShaderAttributeNode->BindVertexAttribute(leftPositionAttribute, positionAttributeId));
	REQUIRE(!duplicateShaderAttributeNode->BindVertexAttribute(centerPositionAttribute, positionAttributeId));

	VertexAttribute<V4Float32> unboundPositionAttribute(centerQuadData.size(), IVertexAttribute::PerformanceHint::WriteRarely | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften, vertexPersistence);
	auto unboundAttributeNode = renderer.CreateRenderableNode();
	REQUIRE(!unboundAttributeNode->BindVertexAttribute(unboundPositionAttribute, positionAttributeId));

	VertexAttribute<V4Float32> unallocatedPositionAttribute(centerQuadData.size(), IVertexAttribute::PerformanceHint::WriteRarely | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften, vertexPersistence);
	auto unallocatedVertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE(unallocatedVertexBuffer->BindVertexAttribute(unallocatedPositionAttribute));
	REQUIRE(unallocatedPositionAttribute.SetInitialData(centerQuadData));
	auto unallocatedAttributeNode = renderer.CreateRenderableNode();
	REQUIRE(!unallocatedAttributeNode->BindVertexAttribute(unallocatedPositionAttribute, positionAttributeId));

	RawVertexAttribute incompatiblePositionAttribute(IVertexAttribute::DataType::Float32, 5, centerQuadData.size(), IVertexAttribute::PerformanceHint::WriteRarely | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften, vertexPersistence);
	auto incompatibleVertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE(incompatibleVertexBuffer->BindVertexAttribute(incompatiblePositionAttribute));
	std::array<float, 20> incompatiblePositionData = {};
	REQUIRE(incompatiblePositionAttribute.SetInitialData(reinterpret_cast<const uint8_t*>(incompatiblePositionData.data()), centerQuadData.size(), 5 * sizeof(float)));
	REQUIRE(incompatibleVertexBuffer->AllocateMemory());
	auto incompatibleAttributeNode = renderer.CreateRenderableNode();
	REQUIRE(!incompatibleAttributeNode->BindVertexAttribute(incompatiblePositionAttribute, positionAttributeId));

	IndexAttribute<V1UInt16> unboundIndexAttribute(leftIndexData.size(), IIndexAttribute::PerformanceHint::WriteRarely | IIndexAttribute::PerformanceHint::ReadNever, IIndexAttribute::PerformanceHint::WriteNever | IIndexAttribute::PerformanceHint::ReadOften, indexPersistence);
	auto unboundIndexNode = renderer.CreateRenderableNode();
	REQUIRE(!unboundIndexNode->BindIndexAttribute(unboundIndexAttribute));

	IndexAttribute<V1UInt16> unallocatedIndexAttribute(leftIndexData.size(), IIndexAttribute::PerformanceHint::WriteRarely | IIndexAttribute::PerformanceHint::ReadNever, IIndexAttribute::PerformanceHint::WriteNever | IIndexAttribute::PerformanceHint::ReadOften, indexPersistence);
	auto unallocatedIndexBuffer = renderer.CreateIndexBuffer();
	REQUIRE(unallocatedIndexBuffer->BindIndexAttribute(unallocatedIndexAttribute));
	REQUIRE(unallocatedIndexAttribute.SetInitialData(leftIndexData));
	auto unallocatedIndexNode = renderer.CreateRenderableNode();
	REQUIRE(!unallocatedIndexNode->BindIndexAttribute(unallocatedIndexAttribute));
	addDiagnosticImage("NegativeBindingContracts", "A centered purple quad rendered after negative shader and attribute binding contracts rejected invalid inputs.", V4Float32(0.75f, 0.25f, 0.75f, 1.0f));

	// Verify that index buffers reject attempts to bind more than one index attribute
	RawIndexAttribute firstSingleBufferIndexAttribute(IIndexAttribute::DataType::UInt16, leftIndexData.size(), IIndexAttribute::PerformanceHint::WriteRarely | IIndexAttribute::PerformanceHint::ReadNever, IIndexAttribute::PerformanceHint::WriteNever | IIndexAttribute::PerformanceHint::ReadOften, indexPersistence);
	RawIndexAttribute secondSingleBufferIndexAttribute(IIndexAttribute::DataType::UInt16, leftIndexData.size(), IIndexAttribute::PerformanceHint::WriteRarely | IIndexAttribute::PerformanceHint::ReadNever, IIndexAttribute::PerformanceHint::WriteNever | IIndexAttribute::PerformanceHint::ReadOften, indexPersistence);
	auto singleAttributeIndexBuffer = renderer.CreateIndexBuffer();
	REQUIRE(singleAttributeIndexBuffer->BindIndexAttribute(firstSingleBufferIndexAttribute));
	REQUIRE(!singleAttributeIndexBuffer->BindIndexAttribute(secondSingleBufferIndexAttribute));
	addDiagnosticImage("SingleIndexAttributePerBuffer", "A centered cyan quad rendered after an index buffer rejected a second index attribute binding.", V4Float32(0.0f, 1.0f, 1.0f, 1.0f));

	// Verify the native vertex attribute format mappings for each supported scalar type and vector width.
	auto verifyAttributeFormats = [&](const char* resultName, const V4Float32& resultColor, std::initializer_list<AttributeFormatCase> attributeFormats) {
		auto formatRenderableNode = renderer.CreateRenderableNode();
		std::vector<std::unique_ptr<RawVertexAttribute>> formatAttributes;
		std::vector<IVertexBuffer::unique_ptr> formatBuffers;
		formatAttributes.reserve(attributeFormats.size());
		formatBuffers.reserve(attributeFormats.size());
		for (const auto& attributeFormat : attributeFormats)
		{
			auto formatAttribute = std::make_unique<RawVertexAttribute>(attributeFormat.dataType, attributeFormat.elementCount, 1, IVertexAttribute::PerformanceHint::WriteRarely | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften, vertexPersistence);
			auto formatBuffer = renderer.CreateVertexBuffer();
			REQUIRE(formatBuffer->BindVertexAttribute(*formatAttribute));
			const size_t entrySizeInBytes = IVertexAttribute::GetDataTypeByteSize(attributeFormat.dataType) * attributeFormat.elementCount;
			std::vector<uint8_t> attributeData(entrySizeInBytes);
			REQUIRE(formatAttribute->SetInitialData(attributeData.data(), 1, entrySizeInBytes));
			REQUIRE(formatBuffer->AllocateMemory());
			REQUIRE(formatRenderableNode->BindVertexAttribute(*formatAttribute, static_cast<VertexAttributeId>(formatAttributes.size())));
			formatAttributes.push_back(std::move(formatAttribute));
			formatBuffers.push_back(std::move(formatBuffer));
		}
		addDiagnosticImage(resultName, "A centered coloured quad rendered after accepting the native vertex attribute format group.", resultColor);
	};
	verifyAttributeFormats("SignedIntegerAttributeFormats", V4Float32(1.0f, 0.25f, 0.25f, 1.0f), {
	                                                                                               {IVertexAttribute::DataType::Int8, 1},
	                                                                                               {IVertexAttribute::DataType::Int8, 2},
	                                                                                               {IVertexAttribute::DataType::Int8, 4},
	                                                                                               {IVertexAttribute::DataType::Int16, 1},
	                                                                                               {IVertexAttribute::DataType::Int16, 2},
	                                                                                               {IVertexAttribute::DataType::Int16, 4},
	                                                                                               {IVertexAttribute::DataType::Int32, 1},
	                                                                                               {IVertexAttribute::DataType::Int32, 2},
	                                                                                               {IVertexAttribute::DataType::Int32, 4},
	                                                                                             });
	verifyAttributeFormats("UnsignedIntegerAttributeFormats", V4Float32(0.25f, 1.0f, 0.25f, 1.0f), {
	                                                                                                 {IVertexAttribute::DataType::UInt8, 1},
	                                                                                                 {IVertexAttribute::DataType::UInt8, 2},
	                                                                                                 {IVertexAttribute::DataType::UInt8, 4},
	                                                                                                 {IVertexAttribute::DataType::UInt16, 1},
	                                                                                                 {IVertexAttribute::DataType::UInt16, 2},
	                                                                                                 {IVertexAttribute::DataType::UInt16, 4},
	                                                                                                 {IVertexAttribute::DataType::UInt32, 1},
	                                                                                                 {IVertexAttribute::DataType::UInt32, 2},
	                                                                                                 {IVertexAttribute::DataType::UInt32, 4},
	                                                                                               });
	verifyAttributeFormats("NormalizedAttributeFormats", V4Float32(0.25f, 0.25f, 1.0f, 1.0f), {
	                                                                                            {IVertexAttribute::DataType::Norm8, 1},
	                                                                                            {IVertexAttribute::DataType::Norm8, 2},
	                                                                                            {IVertexAttribute::DataType::Norm8, 4},
	                                                                                            {IVertexAttribute::DataType::Norm16, 1},
	                                                                                            {IVertexAttribute::DataType::Norm16, 2},
	                                                                                            {IVertexAttribute::DataType::Norm16, 4},
	                                                                                            {IVertexAttribute::DataType::UNorm8, 1},
	                                                                                            {IVertexAttribute::DataType::UNorm8, 2},
	                                                                                            {IVertexAttribute::DataType::UNorm8, 4},
	                                                                                            {IVertexAttribute::DataType::UNorm16, 1},
	                                                                                            {IVertexAttribute::DataType::UNorm16, 2},
	                                                                                            {IVertexAttribute::DataType::UNorm16, 4},
	                                                                                          });
	verifyAttributeFormats("FloatAttributeFormats", V4Float32(1.0f, 1.0f, 0.25f, 1.0f), {
	                                                                                      {IVertexAttribute::DataType::Float16, 1},
	                                                                                      {IVertexAttribute::DataType::Float16, 2},
	                                                                                      {IVertexAttribute::DataType::Float16, 4},
	                                                                                      {IVertexAttribute::DataType::Float32, 1},
	                                                                                      {IVertexAttribute::DataType::Float32, 2},
	                                                                                      {IVertexAttribute::DataType::Float32, 4},
	                                                                                    });

	// Build read-only wrapper attributes and verify they preserve attribute metadata
	ReadOnlyVertexAttribute rightReadOnlyPositionAttribute;
	ReadOnlyIndexAttribute rightReadOnlyIndexAttribute;
	REQUIRE(rightPositionAttribute.BuildReadOnlyAttribute(rightReadOnlyPositionAttribute));
	REQUIRE(rightIndexAttribute.BuildReadOnlyAttribute(rightReadOnlyIndexAttribute));
	REQUIRE(leftPositionAttribute.GetDataPersistenceFlags() == vertexPersistence);
	REQUIRE(centerPositionAttribute.GetDataPersistenceFlags() == vertexPersistence);
	REQUIRE(rightReadOnlyPositionAttribute.GetDataPersistenceFlags() == vertexPersistence);
	REQUIRE(leftIndexAttribute.GetDataPersistenceFlags() == indexPersistence);
	REQUIRE(centerIndexAttribute.GetDataPersistenceFlags() == indexPersistence);
	REQUIRE(rightReadOnlyIndexAttribute.GetDataPersistenceFlags() == indexPersistence);

	// Create our renderable nodes
	auto leftRenderableNode = renderer.CreateRenderableNode();
	REQUIRE(leftRenderableNode->BindVertexAttribute(leftPositionAttribute, positionAttributeId));
	REQUIRE(leftRenderableNode->BindIndexAttribute(leftIndexAttribute));
	REQUIRE(leftRenderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
	leftRenderableNode->SetStateValue(drawColorStateId, V4Float32(1.0f, 0.0f, 0.0f, 1.0f));

	auto centerRenderableNode = renderer.CreateRenderableNode();
	REQUIRE(centerRenderableNode->BindVertexAttribute(centerPositionAttribute, positionAttributeId));
	REQUIRE(centerRenderableNode->BindIndexAttribute(centerIndexAttribute));
	REQUIRE(centerRenderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
	centerRenderableNode->SetStateValue(drawColorStateId, V4Float32(0.0f, 0.0f, 1.0f, 1.0f));

	auto rightRenderableNode = renderer.CreateRenderableNode();
	REQUIRE(rightRenderableNode->BindVertexAttribute(rightReadOnlyPositionAttribute, positionAttributeId));
	REQUIRE(rightRenderableNode->BindIndexAttribute(rightReadOnlyIndexAttribute));
	REQUIRE(rightRenderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
	rightRenderableNode->SetStateValue(drawColorStateId, V4Float32(0.0f, 1.0f, 0.0f, 1.0f));

	// Verify that adjacency primitive modes can be selected
	auto adjacencyLineNode = renderer.CreateRenderableNode();
	REQUIRE(adjacencyLineNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Lines, false, true));
	auto adjacencyLineStripNode = renderer.CreateRenderableNode();
	REQUIRE(adjacencyLineStripNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::LineStrip, false, true));
	auto adjacencyTriangleNode = renderer.CreateRenderableNode();
	REQUIRE(adjacencyTriangleNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles, false, true));
	auto adjacencyTriangleStripNode = renderer.CreateRenderableNode();
	REQUIRE(adjacencyTriangleStripNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::TriangleStrip, false, true));
	addDiagnosticImage("AdjacencyPrimitiveModes", "A centered orange quad rendered after adjacency primitive modes were accepted.", V4Float32(1.0f, 0.5f, 0.0f, 1.0f));

	// Verify that immutable renderable state is frozen once a node is added to the render tree
	auto frozenRenderableNode = renderer.CreateRenderableNode();
	REQUIRE(frozenRenderableNode->BindVertexAttribute(leftPositionAttribute, positionAttributeId));
	REQUIRE(frozenRenderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));
	auto frozenGroupNode = renderer.CreateStateGroupNode();
	frozenGroupNode->AddChildNode(frozenRenderableNode.get());
	REQUIRE(!frozenRenderableNode->BindIndexAttribute(leftIndexAttribute));
	REQUIRE(!frozenRenderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Lines));
	addDiagnosticImage("RenderableImmutableState", "A centered white quad rendered after immutable renderable state rejected late edits.", V4Float32(1.0f, 1.0f, 1.0f, 1.0f));

	// Create our state group node
	auto groupNode = renderer.CreateStateGroupNode();
	groupNode->SetPolygonCullMode(IStateGroupNode::PolygonCullMode::None);
	groupNode->AddChildNode(leftRenderableNode.get());
	groupNode->AddChildNode(centerRenderableNode.get());
	groupNode->AddChildNode(rightRenderableNode.get());

	// Create our program node
	auto programNode = renderer.CreateProgramNode();
	REQUIRE(programNode->BindShaderProgram(shaderProgram.get()));
	programNode->AddChildNode(groupNode.get());

	// Create our render pass node
	auto renderPassNode = renderer.CreateRenderPassNode();
	renderPassNode->BindFrameBuffer(frameBuffer.get());
	renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
	renderPassNode->AddChildNode(programNode.get());

	// Bind our render tree to the renderer
	renderer.SetRenderPasses(&renderPassNode, 1);

	// Capture an image of the indexed raw, normal, and read-only attribute renderables
	auto frameBufferCapture = renderer.CreateFrameBufferOutput();
	frameBufferCapture->SetDetachAfterCapture(true);
	frameBuffer->AddOutputCaptureTarget(frameBufferCapture.get(), IFrameBuffer::AttachmentType::Color);
	DrawOneFrame();
	session.AddTestImageResult("RawNormalAndReadOnlyIndexedAttributes", "A red indexed quad using raw attributes on the left, a blue indexed quad using normal attributes in the center, and a green indexed quad using read-only wrapper attributes on the right.", std::move(frameBufferCapture), IImageDiff::Algorithm::NaiveDiff, 0.98);

	// Remove all our defined render passes
	renderer.RemoveAllRenderPasses();
	return true;
}

} // namespace cobalt::graphics::testing
