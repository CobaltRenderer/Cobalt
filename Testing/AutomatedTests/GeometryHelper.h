// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <cstdint>
#include <vector>
namespace cobalt::graphics::testing {

class GeometryHelper
{
public:
	// Constructors
	explicit GeometryHelper(cobalt::logging::ILogger::unique_ptr log);

	// Primitive creation methods
	void CreatePrimitiveCubeAsPoints(std::vector<V3Float32>& vertexPositions, std::vector<V1UInt32>& indices) const;
	void CreatePrimitiveCubeAsLines(std::vector<V3Float32>& vertexPositions, std::vector<V1UInt32>& indices) const;
	void CreatePrimitiveCubeAsTriangles(std::vector<V3Float32>& vertexPositions, std::vector<V3Float32>& vertexNormals, std::vector<V2Float32>& textureCoords, std::vector<V1UInt32>& indices) const;
	void CreatePrimitiveSphereAsPoints(uint32_t vertexCountWidth, uint32_t vertexCountHeight, std::vector<V3Float32>& vertexPositions, std::vector<V1UInt32>& indices) const;
	void CreatePrimitiveSphereAsLines(uint32_t vertexCountWidth, uint32_t vertexCountHeight, std::vector<V3Float32>& vertexPositions, std::vector<V1UInt32>& indices) const;
	void CreatePrimitiveSphereAsTriangles(uint32_t vertexCountWidth, uint32_t vertexCountHeight, std::vector<V3Float32>& vertexPositions, std::vector<V3Float32>& vertexNormals, std::vector<V2Float32>& textureCoords, std::vector<V1UInt32>& indices) const;
	void CreatePrimitiveCircleAsPoints(uint32_t primitiveCount, std::vector<V3Float32>& vertexPositions, std::vector<V1UInt32>& indices) const;
	void CreatePrimitiveCircleAsLines(uint32_t primitiveCount, std::vector<V3Float32>& vertexPositions, std::vector<V1UInt32>& indices) const;
	void CreatePrimitiveCircleAsLineStrip(uint32_t primitiveCount, std::vector<V3Float32>& vertexPositions, std::vector<V1UInt32>& indices) const;
	void CreatePrimitiveCircleAsTriangles(uint32_t primitiveCount, std::vector<V3Float32>& vertexPositions, std::vector<V3Float32>& vertexNormals, std::vector<V2Float32>& textureCoords, std::vector<V1UInt32>& indices) const;
	void CreatePrimitiveSquareAsTriangles(uint32_t vertexCountWidth, std::vector<V3Float32>& vertexPositions, std::vector<V3Float32>& vertexNormals, std::vector<V2Float32>& textureCoords, std::vector<V1UInt32>& indices) const;
	void CreatePrimitiveSquareAsTriangleStrip(uint32_t vertexCountWidth, std::vector<V3Float32>& vertexPositions, std::vector<V3Float32>& vertexNormals, std::vector<V2Float32>& textureCoords, std::vector<V1UInt32>& indices) const;

	// Test data creation methods
	void CreateRGBTrianglePositions(float clipSpaceDepth, std::vector<V4Float32>& vertexPositions) const;
	void CreateRGBTriangleColors(std::vector<V3Float32>& vertexColors) const;
	void CreateUpperLeftTriangle(float clipSpaceDepth, std::vector<V4Float32>& vertexPositions) const;
	void CreateUpperRightTriangle(float clipSpaceDepth, std::vector<V4Float32>& vertexPositions) const;
	void CreateLowerLeftTriangle(float clipSpaceDepth, std::vector<V4Float32>& vertexPositions) const;
	void CreateCenteredQuad(float clipSpaceDepth, float halfSize, std::vector<V4Float32>& vertexPositions) const;
	void CreateFullscreenQuad(float clipSpaceDepth, std::vector<V4Float32>& vertexPositions) const;
	void CreateIndexedQuad(float clipSpaceDepth, float minX, float maxX, float minY, float maxY, std::vector<V4Float32>& vertexPositions, std::vector<V1UInt16>& indices) const;
	void CreateIndexedQuad(float clipSpaceDepth, float minX, float maxX, float minY, float maxY, std::vector<V4Float32>& vertexPositions, std::vector<V1UInt32>& indices) const;
	void CreateDiagonalLineVertices(std::vector<V4Float32>& vertexPositions) const;

	// Model creation methods
	void CreateModelStanfordBunny(std::vector<V3Float32>& vertexPositions, std::vector<V3Float32>& vertexNormals, std::vector<V1UInt32>& indices) const;
	void CreateModelUtahTeapot(std::vector<V3Float32>& vertexPositions) const;

private:
	// Model creation methods
	static const std::vector<V3Float32>& GetStanfordBunnyVertices();
	static const std::vector<V1UInt32>& GetStanfordBunnyIndices();
	static const std::vector<V3Float32>& GetUtahTeapotVertices();

	// Primitive creation methods
	void CreatePrimitiveCube(IRenderableNode::PrimitiveMode primitiveMode, std::vector<V3Float32>& vertexPositions, std::vector<V3Float32>& vertexNormals, std::vector<V2Float32>& textureCoords, std::vector<V1UInt32>& indices) const;
	void CreatePrimitiveSphere(IRenderableNode::PrimitiveMode primitiveMode, uint32_t vertexCountWidth, uint32_t vertexCountHeight, std::vector<V3Float32>& vertexPositions, std::vector<V3Float32>& vertexNormals, std::vector<V2Float32>& textureCoords, std::vector<V1UInt32>& indices) const;
	void CreatePrimitiveCircle(IRenderableNode::PrimitiveMode primitiveMode, uint32_t primitiveCount, std::vector<V3Float32>& vertexPositions, std::vector<V3Float32>& vertexNormals, std::vector<V2Float32>& textureCoords, std::vector<V1UInt32>& indices) const;
	void CreatePrimitiveSquare(IRenderableNode::PrimitiveMode primitiveMode, uint32_t vertexCountWidth, std::vector<V3Float32>& vertexPositions, std::vector<V3Float32>& vertexNormals, std::vector<V2Float32>& textureCoords, std::vector<V1UInt32>& indices) const;

	// Helper methods
	static void GetExtents(const std::vector<V3Float32>& vertexPositions, V3Float64& minExtents, V3Float64& maxExtents, bool extentsHaveExistingValues = false);
	static void Normalize(std::vector<V3Float32>& vertexPositions);
	static void Normalize(std::vector<V3Float32>& vertexPositions, V3Float64& minExtents, V3Float64& maxExtents);
	static void BuildVertexNormalsTriangles(std::vector<V3Float32>& vertexPositions, std::vector<V1UInt32>& indices, std::vector<V3Float32>& vertexNormals, std::vector<V2Float32>& textureCoords, bool useSmoothShading = true, float angleCutoffInDegrees = 45.0f);
	static void BuildVertexNormalsTriStrip(std::vector<V3Float32>& vertexPositions, std::vector<V1UInt32>& indices, std::vector<V3Float32>& vertexNormals);

private:
	cobalt::logging::ILogger::unique_ptr _log;
};

} // namespace cobalt::graphics::testing
