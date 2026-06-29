// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "MipmappingHelper.h"
#include "IUnitTest.h"
#include <algorithm>
#include <cmath>
namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
MipmappingHelper::MipmappingHelper(cobalt::logging::ILogger::unique_ptr log)
: _log(std::move(log))
{}

//----------------------------------------------------------------------------------------
// Dimension methods
//----------------------------------------------------------------------------------------
uint32_t MipmappingHelper::MipDimension(uint32_t baseDimension, int mipmapLevel) const
{
	return std::max(1U, baseDimension >> mipmapLevel);
}

//----------------------------------------------------------------------------------------
V2UInt32 MipmappingHelper::MipDimensions(const V2UInt32& baseDimensions, int mipmapLevel) const
{
	return V2UInt32(MipDimension(baseDimensions.X(), mipmapLevel), MipDimension(baseDimensions.Y(), mipmapLevel));
}

//----------------------------------------------------------------------------------------
V3UInt32 MipmappingHelper::MipDimensions(const V3UInt32& baseDimensions, int mipmapLevel) const
{
	return V3UInt32(MipDimension(baseDimensions.X(), mipmapLevel), MipDimension(baseDimensions.Y(), mipmapLevel), MipDimension(baseDimensions.Z(), mipmapLevel));
}

//----------------------------------------------------------------------------------------
// Image data methods
//----------------------------------------------------------------------------------------
std::array<V4UInt8, 4> MipmappingHelper::GetMipLevelPalette(int mipmapLevel) const
{
	static const std::array<std::array<V4UInt8, 4>, 4> palettes{{
	  {V4UInt8(255, 0, 254, 255), V4UInt8(255, 24, 146, 255), V4UInt8(177, 49, 95, 255), V4UInt8(221, 30, 59, 255)},
	  {V4UInt8(238, 232, 141, 255), V4UInt8(126, 129, 4, 255), V4UInt8(139, 71, 19, 255), V4UInt8(251, 255, 18, 255)},
	  {V4UInt8(16, 140, 36, 255), V4UInt8(0, 255, 129, 255), V4UInt8(0, 254, 255, 255), V4UInt8(0, 138, 139, 255)},
	  {V4UInt8(56, 139, 255, 255), V4UInt8(56, 0, 254, 255), V4UInt8(26, 0, 139, 255), V4UInt8(76, 56, 139, 255)},
	}};
	return palettes[(size_t)mipmapLevel % palettes.size()];
}

//----------------------------------------------------------------------------------------
std::vector<V4UInt8> MipmappingHelper::CreateMipLevelBandData1D(uint32_t imageWidth, int mipmapLevel) const
{
	auto palette = GetMipLevelPalette(mipmapLevel);
	std::vector<V4UInt8> imageData;
	imageData.resize(imageWidth);
	for (uint32_t posX = 0; posX < imageWidth; ++posX)
	{
		size_t bandIndex = std::min<size_t>((size_t)((posX * 4) / imageWidth), 3);
		imageData[posX] = palette[bandIndex];
	}
	return imageData;
}

//----------------------------------------------------------------------------------------
std::vector<V4UInt8> MipmappingHelper::CreateMipLevelBandData2D(const V2UInt32& imageDimensions, int mipmapLevel) const
{
	auto palette = GetMipLevelPalette(mipmapLevel);
	std::vector<V4UInt8> imageData;
	imageData.resize((size_t)imageDimensions.X() * (size_t)imageDimensions.Y());
	for (uint32_t posY = 0; posY < imageDimensions.Y(); ++posY)
	{
		for (uint32_t posX = 0; posX < imageDimensions.X(); ++posX)
		{
			size_t quadrantIndex = (posX >= (imageDimensions.X() / 2) ? 1 : 0) + (posY >= (imageDimensions.Y() / 2) ? 2 : 0);
			imageData[posX + (posY * imageDimensions.X())] = palette[quadrantIndex];
		}
	}
	return imageData;
}

//----------------------------------------------------------------------------------------
std::vector<V4UInt8> MipmappingHelper::CreateMipLevelBandData3D(const V3UInt32& imageDimensions, int mipmapLevel) const
{
	auto palette = GetMipLevelPalette(mipmapLevel);
	std::vector<V4UInt8> imageData;
	imageData.resize((size_t)imageDimensions.X() * (size_t)imageDimensions.Y() * (size_t)imageDimensions.Z());
	for (uint32_t posZ = 0; posZ < imageDimensions.Z(); ++posZ)
	{
		for (uint32_t posY = 0; posY < imageDimensions.Y(); ++posY)
		{
			for (uint32_t posX = 0; posX < imageDimensions.X(); ++posX)
			{
				size_t quadrantIndex = (posX >= (imageDimensions.X() / 2) ? 1 : 0) + (posY >= (imageDimensions.Y() / 2) ? 2 : 0);
				size_t paletteIndex = (quadrantIndex + posZ) % palette.size();
				size_t dataIndex = posX + (posY * (size_t)imageDimensions.X()) + (posZ * (size_t)imageDimensions.X() * (size_t)imageDimensions.Y());
				imageData[dataIndex] = palette[paletteIndex];
			}
		}
	}
	return imageData;
}

//----------------------------------------------------------------------------------------
std::vector<V4UInt8> MipmappingHelper::CreateSolidMipLevelData2D(uint32_t faceLength, const V4UInt8& color) const
{
	return std::vector<V4UInt8>((size_t)faceLength * (size_t)faceLength, color);
}

//----------------------------------------------------------------------------------------
// Scale methods
//----------------------------------------------------------------------------------------
float MipmappingHelper::ComputeQuadScaleForMipLevel(uint32_t windowSize, uint32_t textureSize, float targetMipLevel, float coordinateSpan) const
{
	return (coordinateSpan * (float)textureSize) / ((float)windowSize * std::exp2(targetMipLevel));
}

//----------------------------------------------------------------------------------------
V2Float32 MipmappingHelper::ComputeQuadScaleForMipLevel(const V2UInt32& windowSize, const V2UInt32& textureSize, float targetMipLevel, float coordinateSpanX, float coordinateSpanY) const
{
	return V2Float32(ComputeQuadScaleForMipLevel(windowSize.X(), textureSize.X(), targetMipLevel, coordinateSpanX), ComputeQuadScaleForMipLevel(windowSize.Y(), textureSize.Y(), targetMipLevel, coordinateSpanY));
}

//----------------------------------------------------------------------------------------
// Scene methods
//----------------------------------------------------------------------------------------
bool MipmappingHelper::InitializeCubeMapDisplayScene(IUnitTest* caller, ITestSession& session, GeometryHelper& geometry, const std::string& vertexShader, const std::string& fragmentShader, CubeMapDisplayScene& scene, float displayScale) const
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

	// Generate data for the object we want to render
	std::vector<V3Float32> positionVertexData;
	std::vector<V3Float32> normalVertexData;
	std::vector<V2Float32> texCoordData;
	std::vector<V1UInt32> indexData;
	geometry.CreatePrimitiveSphereAsTriangles(64, 64, positionVertexData, normalVertexData, texCoordData, indexData);
	for (auto& position : positionVertexData)
	{
		position = V3Float32(position.X() * displayScale, position.Y() * displayScale, position.Z() * displayScale);
	}

	// Create our vertex buffer and populate it with data
	scene.vertexAttributePosition = std::make_unique<VertexAttribute<V3Float32>>(positionVertexData.size(), IVertexAttribute::PerformanceHint::WriteRarely | IVertexAttribute::PerformanceHint::ReadNever, IVertexAttribute::PerformanceHint::WriteNever | IVertexAttribute::PerformanceHint::ReadOften);
	scene.vertexBuffer = renderer.CreateVertexBuffer();
	REQUIRE_EXTERNAL(caller, scene.vertexBuffer->BindVertexAttribute(*scene.vertexAttributePosition));
	REQUIRE_EXTERNAL(caller, scene.vertexAttributePosition->SetInitialData(positionVertexData));
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
void MipmappingHelper::CreateSegmentedPerspectivePlaneMesh(const PerspectivePlaneSettings& settings, uint32_t segmentCountWidth, uint32_t segmentCountDepth, std::vector<V3Float32>& vertexPositions, std::vector<V2Float32>& textureCoords, std::vector<V1UInt32>& indices) const
{
	vertexPositions.clear();
	textureCoords.clear();
	indices.clear();

	if (segmentCountWidth == 0 || segmentCountDepth == 0)
	{
		return;
	}

	const float planeDepth = settings.farY - settings.nearY;
	const auto positionFromBase = [&](float baseX, float baseY) {
		const float x = baseX * settings.width;
		const float y = ((baseY + 0.5f) * planeDepth) + settings.nearY;
		return V3Float32(x, y, settings.z);
	};
	const auto texCoordFromBase = [&](float baseX, float baseY) {
		const float u = (((baseX + 1.0f) * 0.5f) * settings.texCoordScale.X()) + settings.texCoordOffset.X();
		const float v = (((1.0f - baseY) * 0.5f) * settings.texCoordScale.Y()) + settings.texCoordOffset.Y();
		return V2Float32(u, v);
	};

	vertexPositions.reserve((size_t)segmentCountWidth * (size_t)segmentCountDepth * 4);
	textureCoords.reserve((size_t)segmentCountWidth * (size_t)segmentCountDepth * 4);
	indices.reserve((size_t)segmentCountWidth * (size_t)segmentCountDepth * 6);

	for (uint32_t segmentY = 0; segmentY < segmentCountDepth; ++segmentY)
	{
		const float baseY0 = (((float)segmentY / (float)segmentCountDepth) * 2.0f) - 1.0f;
		const float baseY1 = (((float)(segmentY + 1) / (float)segmentCountDepth) * 2.0f) - 1.0f;

		for (uint32_t segmentX = 0; segmentX < segmentCountWidth; ++segmentX)
		{
			const float baseX0 = (((float)segmentX / (float)segmentCountWidth) * 2.0f) - 1.0f;
			const float baseX1 = (((float)(segmentX + 1) / (float)segmentCountWidth) * 2.0f) - 1.0f;

			const auto bottomLeftTexCoord = texCoordFromBase(baseX0, baseY0);
			const auto topLeftTexCoord = texCoordFromBase(baseX0, baseY1);
			const auto topRightTexCoord = texCoordFromBase(baseX1, baseY1);
			const auto bottomRightTexCoord = texCoordFromBase(baseX1, baseY0);
			const float vOffset = std::floor(std::min(bottomLeftTexCoord.Y(), topLeftTexCoord.Y()));
			const auto vertexBase = (uint32_t)vertexPositions.size();

			// Duplicate each segment so V can be shifted near zero by an integer amount. Repeat sampling sees the
			// same texels, while the rasterizer only interpolates a small local coordinate range.
			vertexPositions.push_back(positionFromBase(baseX0, baseY0));
			vertexPositions.push_back(positionFromBase(baseX0, baseY1));
			vertexPositions.push_back(positionFromBase(baseX1, baseY1));
			vertexPositions.push_back(positionFromBase(baseX1, baseY0));

			textureCoords.emplace_back(bottomLeftTexCoord.X(), bottomLeftTexCoord.Y() - vOffset);
			textureCoords.emplace_back(topLeftTexCoord.X(), topLeftTexCoord.Y() - vOffset);
			textureCoords.emplace_back(topRightTexCoord.X(), topRightTexCoord.Y() - vOffset);
			textureCoords.emplace_back(bottomRightTexCoord.X(), bottomRightTexCoord.Y() - vOffset);

			indices.emplace_back(vertexBase + 0);
			indices.emplace_back(vertexBase + 1);
			indices.emplace_back(vertexBase + 2);
			indices.emplace_back(vertexBase + 0);
			indices.emplace_back(vertexBase + 2);
			indices.emplace_back(vertexBase + 3);
		}
	}
}

//----------------------------------------------------------------------------------------
std::vector<V3Float32> MipmappingHelper::CreatePerspectivePlanePositions(const TexturedQuadSceneHelper::Scene& scene, const PerspectivePlaneSettings& settings) const
{
	std::vector<V3Float32> transformedPositionData;
	transformedPositionData.resize(scene.basePositionData.size());
	for (size_t i = 0; i < scene.basePositionData.size(); ++i)
	{
		float x = scene.basePositionData[i].X() * settings.width;
		float y = ((scene.basePositionData[i].Y() + 0.5f) * (settings.farY - settings.nearY)) + settings.nearY;
		transformedPositionData[i] = V3Float32(x, y, settings.z);
	}
	return transformedPositionData;
}

//----------------------------------------------------------------------------------------
bool MipmappingHelper::ConfigurePerspectivePlaneScene(TexturedQuadSceneHelper::Scene& scene, TransformHelper& transform, ITestSession& session, const PerspectivePlaneSettings& settings, const TexturedQuadSceneHelper& texturedQuadSceneHelper) const
{
	auto transformedPositionData = CreatePerspectivePlanePositions(scene, settings);
	auto viewProj = (settings.farClip > settings.nearClip)
	  ? transform.LookAtCenterPerspective(session.TestWindowSizeAsFloat(), settings.cameraPosition, settings.fov, settings.nearClip, settings.farClip)
	  : transform.LookAtCenterPerspective(session.TestWindowSizeAsFloat(), settings.cameraPosition, settings.fov);
	scene.renderableNode->SetStateValue(scene.shaderProgram->GetStateValueId("viewProj"), viewProj);
	return texturedQuadSceneHelper.UpdatePositions(scene, transformedPositionData) && texturedQuadSceneHelper.UpdateTexCoords(scene, settings.texCoordScale, settings.texCoordOffset);
}

} // namespace cobalt::graphics::testing
