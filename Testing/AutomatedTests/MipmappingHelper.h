// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "TexturedQuadSceneHelper.h"
#include "TransformHelper.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <array>
#include <memory>
#include <string>
#include <vector>
namespace cobalt::graphics::testing {
class IUnitTest;

class MipmappingHelper
{
public:
	// Structures
	struct CubeMapDisplayScene
	{
		cobalt::graphics::IFrameBuffer::unique_ptr frameBuffer;
		cobalt::graphics::IShaderProgram::unique_ptr shaderProgram;
		cobalt::graphics::IVertexBuffer::unique_ptr vertexBuffer;
		cobalt::graphics::IIndexBuffer::unique_ptr indexBuffer;
		cobalt::graphics::IRenderableNode::unique_ptr renderableNode;
		cobalt::graphics::IStateGroupNode::unique_ptr groupNode;
		cobalt::graphics::IProgramNode::unique_ptr programNode;
		cobalt::graphics::IRenderPassNode::unique_ptr renderPassNode;
		std::unique_ptr<cobalt::graphics::VertexAttribute<cobalt::graphics::V3Float32>> vertexAttributePosition;
		std::unique_ptr<cobalt::graphics::IndexAttribute<cobalt::graphics::V1UInt32>> indexAttribute;
	};

	struct PerspectivePlaneSettings
	{
		float width = 8.0f;
		float nearY = 0.2f;
		float farY = 18.0f;
		float z = 0.0f;
		cobalt::graphics::V3Float32 cameraPosition = cobalt::graphics::V3Float32(0.0f, -2.25f, 1.25f);
		float fov = 60.0f;
		float nearClip = 0.001f;
		float farClip = 0.0f;
		cobalt::graphics::V2Float32 texCoordScale = cobalt::graphics::V2Float32(1.0f, 1.0f);
		cobalt::graphics::V2Float32 texCoordOffset = cobalt::graphics::V2Float32(0.0f, 0.0f);
	};

public:
	// Constructors
	explicit MipmappingHelper(cobalt::logging::ILogger::unique_ptr log);

	// Dimension methods
	uint32_t MipDimension(uint32_t baseDimension, int mipmapLevel) const;
	cobalt::graphics::V2UInt32 MipDimensions(const cobalt::graphics::V2UInt32& baseDimensions, int mipmapLevel) const;
	cobalt::graphics::V3UInt32 MipDimensions(const cobalt::graphics::V3UInt32& baseDimensions, int mipmapLevel) const;

	// Image data methods
	std::array<cobalt::graphics::V4UInt8, 4> GetMipLevelPalette(int mipmapLevel) const;
	std::vector<cobalt::graphics::V4UInt8> CreateMipLevelBandData1D(uint32_t imageWidth, int mipmapLevel) const;
	std::vector<cobalt::graphics::V4UInt8> CreateMipLevelBandData2D(const cobalt::graphics::V2UInt32& imageDimensions, int mipmapLevel) const;
	std::vector<cobalt::graphics::V4UInt8> CreateMipLevelBandData3D(const cobalt::graphics::V3UInt32& imageDimensions, int mipmapLevel) const;
	std::vector<cobalt::graphics::V4UInt8> CreateSolidMipLevelData2D(uint32_t faceLength, const cobalt::graphics::V4UInt8& color) const;

	// Scale methods
	float ComputeQuadScaleForMipLevel(uint32_t windowSize, uint32_t textureSize, float targetMipLevel, float coordinateSpan = 1.0f) const;
	cobalt::graphics::V2Float32 ComputeQuadScaleForMipLevel(const cobalt::graphics::V2UInt32& windowSize, const cobalt::graphics::V2UInt32& textureSize, float targetMipLevel, float coordinateSpanX = 1.0f, float coordinateSpanY = 1.0f) const;

	// Scene methods
	bool InitializeCubeMapDisplayScene(IUnitTest* caller, ITestSession& session, GeometryHelper& geometry, const std::string& vertexShader, const std::string& fragmentShader, CubeMapDisplayScene& scene, float displayScale = 2.5f) const;
	void CreateSegmentedPerspectivePlaneMesh(const PerspectivePlaneSettings& settings, uint32_t segmentCountWidth, uint32_t segmentCountDepth, std::vector<cobalt::graphics::V3Float32>& vertexPositions, std::vector<cobalt::graphics::V2Float32>& textureCoords, std::vector<cobalt::graphics::V1UInt32>& indices) const;
	std::vector<cobalt::graphics::V3Float32> CreatePerspectivePlanePositions(const TexturedQuadSceneHelper::Scene& scene, const PerspectivePlaneSettings& settings) const;
	bool ConfigurePerspectivePlaneScene(TexturedQuadSceneHelper::Scene& scene, TransformHelper& transform, ITestSession& session, const PerspectivePlaneSettings& settings, const TexturedQuadSceneHelper& texturedQuadSceneHelper) const;

private:
	cobalt::logging::ILogger::unique_ptr _log;
};

} // namespace cobalt::graphics::testing
