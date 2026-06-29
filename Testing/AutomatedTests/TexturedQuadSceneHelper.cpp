// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "TexturedQuadSceneHelper.h"
#include "IUnitTest.h"
namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

namespace {
constexpr uint32_t FilterModePatchSizeValue = 20;
constexpr uint32_t FilterModePatchCenterOffsetValue = 30;
} // namespace

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
TexturedQuadSceneHelper::TexturedQuadSceneHelper(cobalt::logging::ILogger::unique_ptr log)
: _log(std::move(log))
{}

//----------------------------------------------------------------------------------------
// Shader methods
//----------------------------------------------------------------------------------------
const std::string& TexturedQuadSceneHelper::VertexShader() const
{
	static const std::string shader = R"(
struct VSInput {
    float3 position : position;
    float2 texCoord : texCoord;
};

struct VSOutput {
    float4 position : SV_POSITION;
    float2 texCoord : texCoord;
};

VSOutput main(VSInput IN)
{
    VSOutput OUT;

    OUT.position = float4(IN.position, 1.0f);
    OUT.texCoord = IN.texCoord;

    return OUT;
}
)";
	return shader;
}

//----------------------------------------------------------------------------------------
const std::string& TexturedQuadSceneHelper::PerspectiveVertexShader() const
{
	static const std::string shader = R"(
struct VSInput {
    float3 position : position;
    float2 texCoord : texCoord;
};

struct VSOutput {
    float4 position : SV_POSITION;
    float2 texCoord : texCoord;
};

uniform row_major float4x4 viewProj;

VSOutput main(VSInput IN)
{
    VSOutput OUT;

    OUT.position = mul(viewProj, float4(IN.position, 1.0f));
    OUT.texCoord = IN.texCoord;

    return OUT;
}
)";
	return shader;
}

//----------------------------------------------------------------------------------------
// Scene methods
//----------------------------------------------------------------------------------------
bool TexturedQuadSceneHelper::InitializeScene(IUnitTest* caller, ITestSession& session, GeometryHelper& geometry, const std::string& vertexShader, const std::string& fragmentShader, Scene& scene) const
{
	// Generate data for the object we want to render.
	std::vector<V3Float32> positionVertexData;
	std::vector<V3Float32> normalVertexData;
	std::vector<V2Float32> texCoordData;
	std::vector<V1UInt32> indexData;
	geometry.CreatePrimitiveSquareAsTriangles(10, positionVertexData, normalVertexData, texCoordData, indexData);
	return InitializeScene(caller, session, vertexShader, fragmentShader, positionVertexData, texCoordData, indexData, scene);
}

//----------------------------------------------------------------------------------------
bool TexturedQuadSceneHelper::InitializeScene(IUnitTest* caller, ITestSession& session, const std::string& vertexShader, const std::string& fragmentShader, const std::vector<V3Float32>& positionVertexData, const std::vector<V2Float32>& texCoordData, const std::vector<V1UInt32>& indexData, Scene& scene) const
{
	// Retrieve our important objects
	auto& renderer = session.Renderer();
	auto& uiThread = session.UIThread();
	auto& testWindowInfo = *session.TestWindowPlatformInfo();

	// Define the framebuffer
	scene.frameBuffer = renderer.CreateFrameBuffer();
	scene.frameBuffer->DefineViewportRegion(V2UInt32(0, 0), session.TestWindowSize());
	REQUIRE_EXTERNAL(caller, uiThread.InvokeSync([&] { return scene.frameBuffer->BindWindow(testWindowInfo, IFrameBuffer::WindowDepthStencilMode::DepthUNorm24); }));

	// Create and compile our shader program
	scene.shaderProgram = renderer.CreateShaderProgram();
	REQUIRE_EXTERNAL(caller, scene.shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Vertex, ShaderSourceInfoHLSL(vertexShader)));
	REQUIRE_EXTERNAL(caller, scene.shaderProgram->LoadShaderStage(IShaderProgram::ShaderStage::Fragment, ShaderSourceInfoHLSL(fragmentShader)));
	REQUIRE_EXTERNAL(caller, scene.shaderProgram->CompileProgram());

	// Retrieve our shader attribute IDs
	auto positionAttributeId = scene.shaderProgram->GetVertexAttributeId("position");
	auto texCoordAttributeId = scene.shaderProgram->GetVertexAttributeId("texCoord");

	// Retain the mesh data so later test steps can update or restore it.
	scene.basePositionData = positionVertexData;
	scene.baseTexCoordData = texCoordData;

	// Create our vertex buffer and populate it with data
	scene.vertexAttributePosition = std::make_unique<VertexAttribute<V3Float32>>(positionVertexData.size(), IVertexAttribute::PerformanceHint::WriteRarely | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	scene.vertexAttributeTexCoord = std::make_unique<VertexAttribute<V2Float32>>(texCoordData.size(), IVertexAttribute::PerformanceHint::WriteRarely | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	scene.vertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE_EXTERNAL(caller, scene.vertexBuffer->BindVertexAttribute(*scene.vertexAttributePosition));
	REQUIRE_EXTERNAL(caller, scene.vertexAttributePosition->SetInitialData(positionVertexData));
	REQUIRE_EXTERNAL(caller, scene.vertexBuffer->BindVertexAttribute(*scene.vertexAttributeTexCoord));
	REQUIRE_EXTERNAL(caller, scene.vertexAttributeTexCoord->SetInitialData(texCoordData));
	REQUIRE_EXTERNAL(caller, scene.vertexBuffer->AllocateMemory());

	// Create our index buffer and populate it with data
	scene.indexAttribute = std::make_unique<IndexAttribute<V1UInt32>>(indexData.size(), IIndexAttribute::PerformanceHint::WriteNever | IIndexAttribute::PerformanceHint::ReadNever, IIndexAttribute::PerformanceHint::WriteNever | IIndexAttribute::PerformanceHint::ReadOften);
	scene.indexBuffer = renderer.CreateIndexBuffer();
	REQUIRE_EXTERNAL(caller, scene.indexBuffer->BindIndexAttribute(*scene.indexAttribute));
	REQUIRE_EXTERNAL(caller, scene.indexAttribute->SetInitialData(indexData));
	REQUIRE_EXTERNAL(caller, scene.indexBuffer->AllocateMemory());

	// Create our renderable node
	scene.renderableNode = renderer.CreateRenderableNode();
	REQUIRE_EXTERNAL(caller, scene.renderableNode->BindVertexAttribute(*scene.vertexAttributePosition, positionAttributeId));
	if (texCoordAttributeId != VertexAttributeId::Null)
	{
		REQUIRE_EXTERNAL(caller, scene.renderableNode->BindVertexAttribute(*scene.vertexAttributeTexCoord, texCoordAttributeId));
	}
	REQUIRE_EXTERNAL(caller, scene.renderableNode->BindIndexAttribute(*scene.indexAttribute));
	REQUIRE_EXTERNAL(caller, scene.renderableNode->SetPrimitiveMode(IRenderableNode::PrimitiveMode::Triangles));

	// Create our state group node
	scene.groupNode = renderer.CreateStateGroupNode();
	scene.groupNode->AddChildNode(scene.renderableNode.get());

	// Create our program node
	scene.programNode = renderer.CreateProgramNode();
	REQUIRE_EXTERNAL(caller, scene.programNode->BindShaderProgram(scene.shaderProgram.get()));
	scene.programNode->AddChildNode(scene.groupNode.get());

	// Create our render pass node
	scene.renderPassNode = renderer.CreateRenderPassNode();
	scene.renderPassNode->BindFrameBuffer(scene.frameBuffer.get());
	scene.renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Color, 0, V4Float32(0.0f, 0.0f, 0.0f, 1.0f));
	scene.renderPassNode->SetAttachmentClearData(IFrameBuffer::AttachmentType::Depth, 0, V4Float32(1.0f, 0.0f, 0.0f, 0.0f));
	scene.renderPassNode->AddChildNode(scene.programNode.get());

	// Bind our render tree to the renderer
	renderer.SetRenderPasses(&scene.renderPassNode, 1);
	return true;
}

//----------------------------------------------------------------------------------------
bool TexturedQuadSceneHelper::InitializeScene(IUnitTest* caller, ITestSession& session, GeometryHelper& geometry, const std::string& fragmentShader, Scene& scene) const
{
	return InitializeScene(caller, session, geometry, VertexShader(), fragmentShader, scene);
}

//----------------------------------------------------------------------------------------
// Image data methods
//----------------------------------------------------------------------------------------
std::vector<V4UInt8> TexturedQuadSceneHelper::CreateSolidColorImageData(size_t pixelCount, const V4UInt8& color) const
{
	return std::vector<V4UInt8>(pixelCount, color);
}

//----------------------------------------------------------------------------------------
void TexturedQuadSceneHelper::ReplaceRowRange(std::vector<V4UInt8>& imageData, uint32_t imageWidth, uint32_t rowStart, uint32_t rowCount, const V4UInt8& color) const
{
	for (uint32_t posY = rowStart; posY < (rowStart + rowCount); ++posY)
	{
		for (uint32_t posX = 0; posX < imageWidth; ++posX)
		{
			imageData[posX + (posY * imageWidth)] = color;
		}
	}
}

//----------------------------------------------------------------------------------------
void TexturedQuadSceneHelper::ReplaceColumnRange(std::vector<V4UInt8>& imageData, uint32_t imageWidth, uint32_t imageHeight, uint32_t columnStart, uint32_t columnCount, const V4UInt8& color) const
{
	for (uint32_t posY = 0; posY < imageHeight; ++posY)
	{
		for (uint32_t posX = columnStart; posX < (columnStart + columnCount); ++posX)
		{
			imageData[posX + (posY * imageWidth)] = color;
		}
	}
}

//----------------------------------------------------------------------------------------
// Filter test methods
//----------------------------------------------------------------------------------------
uint32_t TexturedQuadSceneHelper::PatchSize() const
{
	return FilterModePatchSizeValue;
}

//----------------------------------------------------------------------------------------
uint32_t TexturedQuadSceneHelper::PatchCenterOffset() const
{
	return FilterModePatchCenterOffsetValue;
}

//----------------------------------------------------------------------------------------
uint32_t TexturedQuadSceneHelper::GetPatchStart(uint32_t axisSize) const
{
	return ((axisSize / 2) + PatchCenterOffset()) - (PatchSize() / 2);
}

//----------------------------------------------------------------------------------------
std::vector<V4UInt8> TexturedQuadSceneHelper::CreateRandomPatchData(size_t pixelCount) const
{
	PseudoRandomGenerator randomGenerator;
	std::vector<V4UInt8> imageData;
	imageData.resize(pixelCount);
	for (size_t i = 0; i < pixelCount; ++i)
	{
		const auto red = static_cast<uint8_t>(randomGenerator.GetNext(256));
		const auto green = static_cast<uint8_t>(randomGenerator.GetNext(256));
		const auto blue = static_cast<uint8_t>(randomGenerator.GetNext(256));
		imageData[i] = V4UInt8(red, green, blue, 255);
	}
	return imageData;
}

//----------------------------------------------------------------------------------------
std::vector<V4UInt8> TexturedQuadSceneHelper::CreateRandomPatchData(uint32_t imageWidth, uint32_t imageHeight) const
{
	return CreateRandomPatchData((size_t)imageWidth * (size_t)imageHeight);
}

//----------------------------------------------------------------------------------------
V2Float32 TexturedQuadSceneHelper::GetTexCoordScale(uint32_t imageWidth) const
{
	return V2Float32((float)PatchSize() / (float)imageWidth, 1.0f);
}

//----------------------------------------------------------------------------------------
V2Float32 TexturedQuadSceneHelper::GetTexCoordOffset(uint32_t imageWidth) const
{
	return V2Float32((float)GetPatchStart(imageWidth) / (float)imageWidth, 0.0f);
}

//----------------------------------------------------------------------------------------
V2Float32 TexturedQuadSceneHelper::GetTexCoordScale(const V2UInt32& imageDimensions) const
{
	return V2Float32((float)PatchSize() / (float)imageDimensions.X(), (float)PatchSize() / (float)imageDimensions.Y());
}

//----------------------------------------------------------------------------------------
V2Float32 TexturedQuadSceneHelper::GetTexCoordOffset(const V2UInt32& imageDimensions) const
{
	return V2Float32((float)GetPatchStart(imageDimensions.X()) / (float)imageDimensions.X(), (float)GetPatchStart(imageDimensions.Y()) / (float)imageDimensions.Y());
}

//----------------------------------------------------------------------------------------
// Transformation methods
//----------------------------------------------------------------------------------------
std::vector<V2Float32> TexturedQuadSceneHelper::CreateScaledOffsetTexCoords(const std::vector<V2Float32>& baseTexCoordData, const V2Float32& texCoordScale, const V2Float32& texCoordOffset) const
{
	std::vector<V2Float32> transformedTexCoordData;
	transformedTexCoordData.resize(baseTexCoordData.size());
	for (size_t i = 0; i < baseTexCoordData.size(); ++i)
	{
		transformedTexCoordData[i] = V2Float32((baseTexCoordData[i].X() * texCoordScale.X()) + texCoordOffset.X(), (baseTexCoordData[i].Y() * texCoordScale.Y()) + texCoordOffset.Y());
	}
	return transformedTexCoordData;
}

//----------------------------------------------------------------------------------------
bool TexturedQuadSceneHelper::UpdateTexCoords(Scene& scene, const V2Float32& texCoordScale, const V2Float32& texCoordOffset) const
{
	auto transformedTexCoordData = CreateScaledOffsetTexCoords(scene.baseTexCoordData, texCoordScale, texCoordOffset);
	return scene.vertexAttributeTexCoord->QueueDataUpdate(transformedTexCoordData.data(), transformedTexCoordData.size());
}

//----------------------------------------------------------------------------------------
bool TexturedQuadSceneHelper::UpdatePositionScale(Scene& scene, const V2Float32& positionScale) const
{
	std::vector<V3Float32> transformedPositionData;
	transformedPositionData.resize(scene.basePositionData.size());
	for (size_t i = 0; i < scene.basePositionData.size(); ++i)
	{
		transformedPositionData[i] = V3Float32(scene.basePositionData[i].X() * positionScale.X(), scene.basePositionData[i].Y() * positionScale.Y(), scene.basePositionData[i].Z());
	}
	return scene.vertexAttributePosition->QueueDataUpdate(transformedPositionData.data(), transformedPositionData.size());
}

//----------------------------------------------------------------------------------------
bool TexturedQuadSceneHelper::UpdatePositions(Scene& scene, const std::vector<V3Float32>& positionData) const
{
	return scene.vertexAttributePosition->QueueDataUpdate(positionData.data(), positionData.size());
}

//----------------------------------------------------------------------------------------
void TexturedQuadSceneHelper::ResetPositions(Scene& scene) const
{
	scene.vertexAttributePosition->QueueDataUpdate(scene.basePositionData.data(), scene.basePositionData.size());
}

//----------------------------------------------------------------------------------------
void TexturedQuadSceneHelper::ResetTexCoords(Scene& scene) const
{
	scene.vertexAttributeTexCoord->QueueDataUpdate(scene.baseTexCoordData.data(), scene.baseTexCoordData.size());
}

} // namespace cobalt::graphics::testing
