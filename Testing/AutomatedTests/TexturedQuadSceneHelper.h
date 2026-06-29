// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "GeometryHelper.h"
#include "ITestSession.h"
#include "PseudoRandomGenerator.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <memory>
#include <string>
#include <vector>
namespace cobalt::graphics::testing {
class IUnitTest;

class TexturedQuadSceneHelper
{
public:
	// Structures
	struct Scene
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
		std::unique_ptr<cobalt::graphics::VertexAttribute<cobalt::graphics::V2Float32>> vertexAttributeTexCoord;
		std::unique_ptr<cobalt::graphics::IndexAttribute<cobalt::graphics::V1UInt32>> indexAttribute;
		std::vector<cobalt::graphics::V3Float32> basePositionData;
		std::vector<cobalt::graphics::V2Float32> baseTexCoordData;
	};

public:
	// Constructors
	explicit TexturedQuadSceneHelper(cobalt::logging::ILogger::unique_ptr log);

	// Shader methods
	const std::string& VertexShader() const;
	const std::string& PerspectiveVertexShader() const;

	// Scene methods
	bool InitializeScene(IUnitTest* caller, ITestSession& session, const std::string& vertexShader, const std::string& fragmentShader, const std::vector<cobalt::graphics::V3Float32>& positionVertexData, const std::vector<cobalt::graphics::V2Float32>& texCoordData, const std::vector<cobalt::graphics::V1UInt32>& indexData, Scene& scene) const;
	bool InitializeScene(IUnitTest* caller, ITestSession& session, GeometryHelper& geometry, const std::string& vertexShader, const std::string& fragmentShader, Scene& scene) const;
	bool InitializeScene(IUnitTest* caller, ITestSession& session, GeometryHelper& geometry, const std::string& fragmentShader, Scene& scene) const;

	// Image data methods
	std::vector<cobalt::graphics::V4UInt8> CreateSolidColorImageData(size_t pixelCount, const cobalt::graphics::V4UInt8& color) const;
	void ReplaceRowRange(std::vector<cobalt::graphics::V4UInt8>& imageData, uint32_t imageWidth, uint32_t rowStart, uint32_t rowCount, const cobalt::graphics::V4UInt8& color) const;
	void ReplaceColumnRange(std::vector<cobalt::graphics::V4UInt8>& imageData, uint32_t imageWidth, uint32_t imageHeight, uint32_t columnStart, uint32_t columnCount, const cobalt::graphics::V4UInt8& color) const;

	// Filter test methods
	uint32_t PatchSize() const;
	uint32_t PatchCenterOffset() const;
	uint32_t GetPatchStart(uint32_t axisSize) const;
	std::vector<cobalt::graphics::V4UInt8> CreateRandomPatchData(size_t pixelCount) const;
	std::vector<cobalt::graphics::V4UInt8> CreateRandomPatchData(uint32_t imageWidth, uint32_t imageHeight) const;
	cobalt::graphics::V2Float32 GetTexCoordScale(uint32_t imageWidth) const;
	cobalt::graphics::V2Float32 GetTexCoordOffset(uint32_t imageWidth) const;
	cobalt::graphics::V2Float32 GetTexCoordScale(const cobalt::graphics::V2UInt32& imageDimensions) const;
	cobalt::graphics::V2Float32 GetTexCoordOffset(const cobalt::graphics::V2UInt32& imageDimensions) const;

	// Transformation methods
	std::vector<cobalt::graphics::V2Float32> CreateScaledOffsetTexCoords(const std::vector<cobalt::graphics::V2Float32>& baseTexCoordData, const cobalt::graphics::V2Float32& texCoordScale, const cobalt::graphics::V2Float32& texCoordOffset) const;
	bool UpdateTexCoords(Scene& scene, const cobalt::graphics::V2Float32& texCoordScale, const cobalt::graphics::V2Float32& texCoordOffset) const;
	bool UpdatePositionScale(Scene& scene, const cobalt::graphics::V2Float32& positionScale) const;
	bool UpdatePositions(Scene& scene, const std::vector<cobalt::graphics::V3Float32>& positionData) const;
	void ResetPositions(Scene& scene) const;
	void ResetTexCoords(Scene& scene) const;

private:
	cobalt::logging::ILogger::unique_ptr _log;
};

} // namespace cobalt::graphics::testing
