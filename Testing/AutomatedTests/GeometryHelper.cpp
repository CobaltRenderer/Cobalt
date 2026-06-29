// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "GeometryHelper.h"
#include <cmath>
#include <utility>
namespace cobalt::graphics::testing {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
GeometryHelper::GeometryHelper(cobalt::logging::ILogger::unique_ptr log)
: _log(std::move(log))
{}

//----------------------------------------------------------------------------------------
// Primitive creation methods
//----------------------------------------------------------------------------------------
void GeometryHelper::CreatePrimitiveCubeAsPoints(std::vector<V3Float32>& vertexPositions, std::vector<V1UInt32>& indices) const
{
	std::vector<V3Float32> vertexNormals;
	std::vector<V2Float32> textureCoords;
	CreatePrimitiveCube(IRenderableNode::PrimitiveMode::Points, vertexPositions, vertexNormals, textureCoords, indices);
}

//----------------------------------------------------------------------------------------
void GeometryHelper::CreatePrimitiveCubeAsLines(std::vector<V3Float32>& vertexPositions, std::vector<V1UInt32>& indices) const
{
	std::vector<V3Float32> vertexNormals;
	std::vector<V2Float32> textureCoords;
	CreatePrimitiveCube(IRenderableNode::PrimitiveMode::Lines, vertexPositions, vertexNormals, textureCoords, indices);
}

//----------------------------------------------------------------------------------------
void GeometryHelper::CreatePrimitiveCubeAsTriangles(std::vector<V3Float32>& vertexPositions, std::vector<V3Float32>& vertexNormals, std::vector<V2Float32>& textureCoords, std::vector<V1UInt32>& indices) const
{
	CreatePrimitiveCube(IRenderableNode::PrimitiveMode::Triangles, vertexPositions, vertexNormals, textureCoords, indices);
}

//----------------------------------------------------------------------------------------
void GeometryHelper::CreatePrimitiveCube(IRenderableNode::PrimitiveMode primitiveMode, std::vector<V3Float32>& vertexPositions, std::vector<V3Float32>& vertexNormals, std::vector<V2Float32>& textureCoords, std::vector<V1UInt32>& indices) const
{
	const uint32_t pointCount = 8;
	V3Float32 points[pointCount] = {{0, 0, 1}, {1, 0, 1}, {0, 1, 1}, {1, 1, 1}, {0, 1, 0}, {1, 1, 0}, {0, 0, 0}, {1, 0, 0}};
	V2Float32 texCoords[pointCount] = {{0, 0}, {1, 0}, {0, 1}, {1, 1}, {0, 1}, {1, 1}, {0, 0}, {1, 0}};
	V1UInt32 indicesPoints[] = {{0}, {1}, {2}, {3}, {4}, {5}, {6}, {7}};
	V1UInt32 indicesLines[] = {{0}, {1}, {1}, {3}, {3}, {2}, {2}, {0}, {4}, {5}, {5}, {7}, {7}, {6}, {6}, {4}, {0}, {6}, {1}, {7}, {2}, {4}, {3}, {5}};
	V1UInt32 indicesTriangles[] = {{0}, {1}, {2}, {2}, {1}, {3}, {2}, {3}, {4}, {4}, {3}, {5}, {4}, {5}, {6}, {6}, {5}, {7}, {6}, {7}, {0}, {0}, {7}, {1}, {1}, {7}, {3}, {3}, {7}, {5}, {6}, {0}, {4}, {4}, {0}, {2}};

	vertexNormals.clear();
	indices.clear();
	vertexPositions.resize(pointCount);
	textureCoords.resize(pointCount);
	for (uint32_t i = 0; i < pointCount; ++i)
	{
		vertexPositions[i] = points[i];
		textureCoords[i] = texCoords[i];
	}
	Normalize(vertexPositions);

	if (primitiveMode == IRenderableNode::PrimitiveMode::Points)
	{
		auto primitiveCount = sizeof(indicesPoints) / sizeof(indicesPoints[0]);
		indices.resize(primitiveCount);
		for (uint32_t i = 0; i < primitiveCount; ++i)
		{
			indices[i] = indicesPoints[i];
		}
	}
	else if (primitiveMode == IRenderableNode::PrimitiveMode::Lines)
	{
		auto primitiveCount = sizeof(indicesLines) / sizeof(indicesLines[0]) / 2;
		indices.resize(primitiveCount * 2);
		for (uint32_t i = 0; i < primitiveCount; ++i)
		{
			indices[(i * 2) + 0] = indicesLines[(i * 2) + 0];
			indices[(i * 2) + 1] = indicesLines[(i * 2) + 1];
		}
	}
	else if (primitiveMode == IRenderableNode::PrimitiveMode::Triangles)
	{
		auto primitiveCount = sizeof(indicesTriangles) / sizeof(indicesTriangles[0]) / 3;
		indices.resize(primitiveCount * 3);
		for (uint32_t i = 0; i < primitiveCount; ++i)
		{
			indices[(i * 3) + 0] = indicesTriangles[(i * 3) + 0];
			indices[(i * 3) + 1] = indicesTriangles[(i * 3) + 1];
			indices[(i * 3) + 2] = indicesTriangles[(i * 3) + 2];
		}
		BuildVertexNormalsTriangles(vertexPositions, indices, vertexNormals, textureCoords);
	}
}

//----------------------------------------------------------------------------------------
void GeometryHelper::CreatePrimitiveSphereAsPoints(uint32_t vertexCountWidth, uint32_t vertexCountHeight, std::vector<V3Float32>& vertexPositions, std::vector<V1UInt32>& indices) const
{
	std::vector<V3Float32> vertexNormals;
	std::vector<V2Float32> textureCoords;
	CreatePrimitiveSphere(IRenderableNode::PrimitiveMode::Points, vertexCountWidth, vertexCountHeight, vertexPositions, vertexNormals, textureCoords, indices);
}

//----------------------------------------------------------------------------------------
void GeometryHelper::CreatePrimitiveSphereAsLines(uint32_t vertexCountWidth, uint32_t vertexCountHeight, std::vector<V3Float32>& vertexPositions, std::vector<V1UInt32>& indices) const
{
	std::vector<V3Float32> vertexNormals;
	std::vector<V2Float32> textureCoords;
	CreatePrimitiveSphere(IRenderableNode::PrimitiveMode::Lines, vertexCountWidth, vertexCountHeight, vertexPositions, vertexNormals, textureCoords, indices);
}

//----------------------------------------------------------------------------------------
void GeometryHelper::CreatePrimitiveSphereAsTriangles(uint32_t vertexCountWidth, uint32_t vertexCountHeight, std::vector<V3Float32>& vertexPositions, std::vector<V3Float32>& vertexNormals, std::vector<V2Float32>& textureCoords, std::vector<V1UInt32>& indices) const
{
	CreatePrimitiveSphere(IRenderableNode::PrimitiveMode::Triangles, vertexCountWidth, vertexCountHeight, vertexPositions, vertexNormals, textureCoords, indices);
}

//----------------------------------------------------------------------------------------
void GeometryHelper::CreatePrimitiveSphere(IRenderableNode::PrimitiveMode primitiveMode, uint32_t vertexCountWidth, uint32_t vertexCountHeight, std::vector<V3Float32>& vertexPositions, std::vector<V3Float32>& vertexNormals, std::vector<V2Float32>& textureCoords, std::vector<V1UInt32>& indices) const
{
	const double pi = 3.14159265358979323846;
	auto vertexCount = vertexCountWidth * vertexCountHeight;
	vertexPositions.resize(vertexCount);
	textureCoords.resize(vertexCount);
	vertexNormals.clear();
	indices.clear();
	for (uint32_t pointX = 0; pointX < vertexCountWidth; ++pointX)
	{
		// Calculate the x and z position of the next point in the circle
		double angle = ((double)pointX / (double)vertexCountWidth) * pi * 2.0;
		double xpos = std::sin(angle);
		double zpos = std::cos(angle);

		for (uint32_t pointY = 0; pointY < vertexCountHeight; ++pointY)
		{
			// Calculate the scale of the circle and y position of the final point
			double heightProgression = (((double)pointY / (double)(vertexCountHeight - 1)) * pi);
			double circleScale = std::sin(heightProgression);
			double ypos = std::cos(heightProgression);

			// Calculate the final coordinates of this circle in the sphere
			uint32_t pointIndexNo = (pointX * vertexCountHeight) + pointY;
			vertexPositions[pointIndexNo] = V3Float32((float)(circleScale * xpos), (float)ypos, (float)(circleScale * zpos));
			textureCoords[pointIndexNo] = V2Float32((float)pointX / (float)(vertexCountWidth - 1), (float)pointY / (float)(vertexCountHeight - 1));
		}
	}

	if (primitiveMode == IRenderableNode::PrimitiveMode::Points)
	{
		indices.resize(vertexCount);
		for (uint32_t pointX = 0; pointX < vertexCountWidth; ++pointX)
		{
			for (uint32_t pointY = 0; pointY < vertexCountHeight; ++pointY)
			{
				uint32_t pointIndexNo = (pointX * vertexCountHeight) + pointY;
				indices[pointIndexNo] = V1UInt32(pointIndexNo);
			}
		}
	}
	else if (primitiveMode == IRenderableNode::PrimitiveMode::Lines)
	{
		uint32_t primitiveCount;
		primitiveCount = ((vertexCountWidth * vertexCountHeight) - 1) * 2;
		indices.resize((size_t)primitiveCount * 2);
		uint32_t lineIndexNo = 0;
		for (uint32_t pointX = 0; pointX < vertexCountWidth; ++pointX)
		{
			for (uint32_t pointY = 1; pointY < vertexCountHeight; ++pointY)
			{
				uint32_t pointIndexNo1 = (pointX * vertexCountHeight) + (pointY - 1);
				uint32_t pointIndexNo2 = (pointX * vertexCountHeight) + pointY;
				indices[lineIndexNo++] = V1UInt32(pointIndexNo1);
				indices[lineIndexNo++] = V1UInt32(pointIndexNo2);
			}
		}
		for (uint32_t pointY = 0; pointY < vertexCountHeight; ++pointY)
		{
			for (uint32_t pointX = 1; pointX < vertexCountWidth; ++pointX)
			{
				uint32_t pointIndexNo1 = ((pointX - 1) * vertexCountHeight) + pointY;
				uint32_t pointIndexNo2 = (pointX * vertexCountHeight) + pointY;
				indices[lineIndexNo++] = V1UInt32(pointIndexNo1);
				indices[lineIndexNo++] = V1UInt32(pointIndexNo2);
			}
			uint32_t pointIndexNo1 = (0 * vertexCountHeight) + pointY;
			uint32_t pointIndexNo2 = ((vertexCountWidth - 1) * vertexCountHeight) + pointY;
			indices[lineIndexNo++] = V1UInt32(pointIndexNo1);
			indices[lineIndexNo++] = V1UInt32(pointIndexNo2);
		}
	}
	else if (primitiveMode == IRenderableNode::PrimitiveMode::Triangles)
	{
		uint32_t primitiveCount;
		primitiveCount = (vertexCountWidth * vertexCountHeight) * 2;
		indices.resize((size_t)primitiveCount * 3);
		uint32_t triIndexNo = 0;
		for (uint32_t pointX = 0; pointX < vertexCountWidth; ++pointX)
		{
			for (uint32_t pointY = 0; pointY < vertexCountHeight; ++pointY)
			{
				uint32_t indexPointX1 = (pointX > 0) ? pointX - 1 : vertexCountWidth - 1;
				uint32_t indexPointX2 = (pointX < vertexCountWidth) ? pointX : 0;
				uint32_t indexPointY1 = (pointY > 0) ? pointY - 1 : vertexCountHeight - 1;
				uint32_t indexPointY2 = (pointY < vertexCountHeight) ? pointY : 0;
				uint32_t pointIndexNo1;
				uint32_t pointIndexNo2;
				uint32_t pointIndexNo3;

				pointIndexNo1 = (indexPointX1 * vertexCountHeight) + indexPointY1;
				pointIndexNo2 = (indexPointX1 * vertexCountHeight) + indexPointY2;
				pointIndexNo3 = (indexPointX2 * vertexCountHeight) + indexPointY2;
				indices[triIndexNo++] = V1UInt32(pointIndexNo1);
				indices[triIndexNo++] = V1UInt32(pointIndexNo2);
				indices[triIndexNo++] = V1UInt32(pointIndexNo3);

				pointIndexNo1 = (indexPointX1 * vertexCountHeight) + indexPointY1;
				pointIndexNo2 = (indexPointX2 * vertexCountHeight) + indexPointY2;
				pointIndexNo3 = (indexPointX2 * vertexCountHeight) + indexPointY1;
				indices[triIndexNo++] = V1UInt32(pointIndexNo1);
				indices[triIndexNo++] = V1UInt32(pointIndexNo2);
				indices[triIndexNo++] = V1UInt32(pointIndexNo3);
			}
		}

		// Since we have a normalized sphere around the origin, our vertex normals are literally just our vertex
		// coordinates.
		vertexNormals.resize(vertexPositions.size());
		for (uint32_t i = 0; i < vertexPositions.size(); ++i)
		{
			vertexNormals[i] = vertexPositions[i];
		}
	}
}

//----------------------------------------------------------------------------------------
void GeometryHelper::CreatePrimitiveCircleAsPoints(uint32_t primitiveCount, std::vector<V3Float32>& vertexPositions, std::vector<V1UInt32>& indices) const
{
	std::vector<V3Float32> vertexNormals;
	std::vector<V2Float32> textureCoords;
	CreatePrimitiveCircle(IRenderableNode::PrimitiveMode::Points, primitiveCount, vertexPositions, vertexNormals, textureCoords, indices);
}

//----------------------------------------------------------------------------------------
void GeometryHelper::CreatePrimitiveCircleAsLines(uint32_t primitiveCount, std::vector<V3Float32>& vertexPositions, std::vector<V1UInt32>& indices) const
{
	std::vector<V3Float32> vertexNormals;
	std::vector<V2Float32> textureCoords;
	CreatePrimitiveCircle(IRenderableNode::PrimitiveMode::Lines, primitiveCount, vertexPositions, vertexNormals, textureCoords, indices);
}

//----------------------------------------------------------------------------------------
void GeometryHelper::CreatePrimitiveCircleAsLineStrip(uint32_t primitiveCount, std::vector<V3Float32>& vertexPositions, std::vector<V1UInt32>& indices) const
{
	std::vector<V3Float32> vertexNormals;
	std::vector<V2Float32> textureCoords;
	CreatePrimitiveCircle(IRenderableNode::PrimitiveMode::LineStrip, primitiveCount, vertexPositions, vertexNormals, textureCoords, indices);
}

//----------------------------------------------------------------------------------------
void GeometryHelper::CreatePrimitiveCircleAsTriangles(uint32_t primitiveCount, std::vector<V3Float32>& vertexPositions, std::vector<V3Float32>& vertexNormals, std::vector<V2Float32>& textureCoords, std::vector<V1UInt32>& indices) const
{
	CreatePrimitiveCircle(IRenderableNode::PrimitiveMode::Triangles, primitiveCount, vertexPositions, vertexNormals, textureCoords, indices);
}

//----------------------------------------------------------------------------------------
void GeometryHelper::CreatePrimitiveCircle(IRenderableNode::PrimitiveMode primitiveMode, uint32_t primitiveCount, std::vector<V3Float32>& vertexPositions, std::vector<V3Float32>& vertexNormals, std::vector<V2Float32>& textureCoords, std::vector<V1UInt32>& indices) const
{
	vertexPositions.clear();
	vertexNormals.clear();
	indices.clear();
	const double pi = 3.14159265358979323846;
	if (primitiveMode == IRenderableNode::PrimitiveMode::Points)
	{
		vertexPositions.resize(primitiveCount);
		indices.resize(primitiveCount);
		for (uint32_t i = 0; i < primitiveCount; ++i)
		{
			vertexPositions[i] = V3Float32((float)std::sin(((double)i / (double)primitiveCount) * pi * 2), (float)std::cos(((double)i / (double)primitiveCount) * pi * 2), (float)0.0);
			indices[i] = V1UInt32(i);
		}
	}
	else if (primitiveMode == IRenderableNode::PrimitiveMode::Lines)
	{
		vertexPositions.resize(primitiveCount);
		indices.resize((size_t)primitiveCount * 2);
		for (uint32_t i = 0; i < primitiveCount; ++i)
		{
			vertexPositions[i] = V3Float32((float)std::sin(((double)i / (double)primitiveCount) * pi * 2), (float)std::cos(((double)i / (double)primitiveCount) * pi * 2), (float)0.0);

			if (i > 0)
			{
				indices[((i - 1) * 2) + 0] = V1UInt32(i - 1);
				indices[((i - 1) * 2) + 1] = V1UInt32(i);
			}
		}
		indices[((primitiveCount - 1) * 2) + 0] = V1UInt32(primitiveCount - 1);
		indices[((primitiveCount - 1) * 2) + 1] = V1UInt32(0);
	}
	else if (primitiveMode == IRenderableNode::PrimitiveMode::LineStrip)
	{
		vertexPositions.resize(primitiveCount);
		indices.resize(primitiveCount + 1);
		for (uint32_t i = 0; i < primitiveCount; ++i)
		{
			vertexPositions[i] = V3Float32((float)std::sin(((double)i / (double)primitiveCount) * pi * 2), (float)std::cos(((double)i / (double)primitiveCount) * pi * 2), (float)0.0);

			indices[i] = V1UInt32(i);
		}
		indices[primitiveCount] = V1UInt32(0);
	}
	else if (primitiveMode == IRenderableNode::PrimitiveMode::Triangles)
	{
		vertexPositions.resize(primitiveCount + 1);
		vertexNormals.resize(primitiveCount + 1);
		indices.resize((size_t)primitiveCount * 3);
		textureCoords.resize(primitiveCount + 1);
		for (uint32_t i = 0; i < primitiveCount; ++i)
		{
			vertexPositions[i] = V3Float32((float)std::sin(((double)i / (double)primitiveCount) * pi * 2), (float)std::cos(((double)i / (double)primitiveCount) * pi * 2), (float)0.0);
			textureCoords[i] = V2Float32(vertexPositions[i].X(), vertexPositions[i].Y());
			vertexNormals[i] = V3Float32(0.0f, 0.0f, -1.0f);

			if (i > 0)
			{
				indices[((i - 1) * 3) + 0] = V1UInt32(i - 1);
				indices[((i - 1) * 3) + 1] = V1UInt32(i);
				indices[((i - 1) * 3) + 2] = V1UInt32(primitiveCount);
			}
		}
		indices[((primitiveCount - 1) * 3) + 0] = V1UInt32(primitiveCount - 1);
		indices[((primitiveCount - 1) * 3) + 1] = V1UInt32(0);
		indices[((primitiveCount - 1) * 3) + 2] = V1UInt32(primitiveCount);

		vertexPositions[primitiveCount] = V3Float32(0.0f, 0.0f, 0.0f);
		vertexNormals[primitiveCount] = V3Float32(0.0f, 0.0f, -1.0f);
		textureCoords[primitiveCount] = V2Float32(0.0f, 0.0f);
	}
}

//----------------------------------------------------------------------------------------
void GeometryHelper::CreatePrimitiveSquareAsTriangles(uint32_t vertexCountWidth, std::vector<V3Float32>& vertexPositions, std::vector<V3Float32>& vertexNormals, std::vector<V2Float32>& textureCoords, std::vector<V1UInt32>& indices) const
{
	CreatePrimitiveSquare(IRenderableNode::PrimitiveMode::Triangles, vertexCountWidth, vertexPositions, vertexNormals, textureCoords, indices);
}

//----------------------------------------------------------------------------------------
void GeometryHelper::CreatePrimitiveSquareAsTriangleStrip(uint32_t vertexCountWidth, std::vector<V3Float32>& vertexPositions, std::vector<V3Float32>& vertexNormals, std::vector<V2Float32>& textureCoords, std::vector<V1UInt32>& indices) const
{
	CreatePrimitiveSquare(IRenderableNode::PrimitiveMode::TriangleStrip, vertexCountWidth, vertexPositions, vertexNormals, textureCoords, indices);
}

//----------------------------------------------------------------------------------------
void GeometryHelper::CreatePrimitiveSquare(IRenderableNode::PrimitiveMode primitiveMode, uint32_t vertexCountWidth, std::vector<V3Float32>& vertexPositions, std::vector<V3Float32>& vertexNormals, std::vector<V2Float32>& textureCoords, std::vector<V1UInt32>& indices) const
{
	vertexPositions.resize((size_t)vertexCountWidth * 2);
	vertexNormals.resize((size_t)vertexCountWidth * 2);
	textureCoords.resize((size_t)vertexCountWidth * 2);
	indices.clear();
	if (primitiveMode == IRenderableNode::PrimitiveMode::Triangles)
	{
		indices.resize((size_t)((vertexCountWidth * 2) - 2) * 3);
		for (uint32_t i = 0; i < vertexCountWidth; ++i)
		{
			float xpos = ((float)i * (2.0f / (float)(vertexCountWidth - 1))) - 1.0f;

			vertexPositions[(i * 2) + 0] = V3Float32(xpos, 1.0f, 0.0f);
			vertexPositions[(i * 2) + 1] = V3Float32(xpos, -1.0f, 0.0f);

			vertexNormals[(i * 2) + 0] = V3Float32(0.0f, 0.0f, 1.0f);
			vertexNormals[(i * 2) + 1] = V3Float32(0.0f, 0.0f, 1.0f);

			textureCoords[(i * 2) + 0] = V2Float32((float)i / (float)(vertexCountWidth - 1), 0.0f);
			textureCoords[(i * 2) + 1] = V2Float32((float)i / (float)(vertexCountWidth - 1), 1.0f);

			if (i > 0)
			{
				indices[(((i * 2) - 2) * 3) + 0] = V1UInt32((i * 2) - 1);
				indices[(((i * 2) - 2) * 3) + 1] = V1UInt32((i * 2) - 2);
				indices[(((i * 2) - 2) * 3) + 2] = V1UInt32((i * 2) + 0);

				indices[(((i * 2) - 1) * 3) + 0] = V1UInt32((i * 2) - 1);
				indices[(((i * 2) - 1) * 3) + 1] = V1UInt32((i * 2) + 0);
				indices[(((i * 2) - 1) * 3) + 2] = V1UInt32((i * 2) + 1);
			}
		}
	}
	else if (primitiveMode == IRenderableNode::PrimitiveMode::TriangleStrip)
	{
		indices.resize((size_t)vertexCountWidth * 2);
		for (uint32_t i = 0; i < vertexCountWidth; ++i)
		{
			float xpos = ((float)i * (2.0f / (float)(vertexCountWidth - 1))) - 1.0f;

			vertexPositions[(i * 2) + 0] = V3Float32(xpos, -1.0f, 0.0f);

			vertexPositions[(i * 2) + 1] = V3Float32(xpos, 1.0f, 0.0f);

			vertexNormals[(i * 2) + 0] = V3Float32(0.0f, 0.0f, 1.0f);

			vertexNormals[(i * 2) + 1] = V3Float32(0.0f, 0.0f, 1.0f);

			textureCoords[(i * 2) + 0] = V2Float32((float)i / (float)(vertexCountWidth - 1), 1.0f);
			textureCoords[(i * 2) + 1] = V2Float32((float)i / (float)(vertexCountWidth - 1), 0.0f);

			indices[(i * 2) + 0] = V1UInt32((i * 2) + 0);
			indices[(i * 2) + 1] = V1UInt32((i * 2) + 1);
		}
	}
}

//----------------------------------------------------------------------------------------
// Test data creation methods
//----------------------------------------------------------------------------------------
void GeometryHelper::CreateRGBTrianglePositions(float clipSpaceDepth, std::vector<V4Float32>& vertexPositions) const
{
	vertexPositions = {
	  V4Float32(-0.5f, 0.5f, clipSpaceDepth, 1.0f),
	  V4Float32(0.0f, -0.5f, clipSpaceDepth, 1.0f),
	  V4Float32(0.5f, 0.5f, clipSpaceDepth, 1.0f),
	};
}

//----------------------------------------------------------------------------------------
void GeometryHelper::CreateRGBTriangleColors(std::vector<V3Float32>& vertexColors) const
{
	vertexColors = {
	  V3Float32(1.0f, 0.0f, 0.0f),
	  V3Float32(0.0f, 1.0f, 0.0f),
	  V3Float32(0.0f, 0.0f, 1.0f),
	};
}

//----------------------------------------------------------------------------------------
void GeometryHelper::CreateUpperLeftTriangle(float clipSpaceDepth, std::vector<V4Float32>& vertexPositions) const
{
	vertexPositions = {
	  V4Float32(-1.0f, 1.0f, clipSpaceDepth, 1.0f),
	  V4Float32(-1.0f, 0.0f, clipSpaceDepth, 1.0f),
	  V4Float32(0.0f, 1.0f, clipSpaceDepth, 1.0f),
	};
}

//----------------------------------------------------------------------------------------
void GeometryHelper::CreateUpperRightTriangle(float clipSpaceDepth, std::vector<V4Float32>& vertexPositions) const
{
	vertexPositions = {
	  V4Float32(0.0f, 1.0f, clipSpaceDepth, 1.0f),
	  V4Float32(1.0f, 1.0f, clipSpaceDepth, 1.0f),
	  V4Float32(1.0f, 0.0f, clipSpaceDepth, 1.0f),
	};
}

//----------------------------------------------------------------------------------------
void GeometryHelper::CreateLowerLeftTriangle(float clipSpaceDepth, std::vector<V4Float32>& vertexPositions) const
{
	vertexPositions = {
	  V4Float32(-1.0f, 0.0f, clipSpaceDepth, 1.0f),
	  V4Float32(-1.0f, -1.0f, clipSpaceDepth, 1.0f),
	  V4Float32(0.0f, -1.0f, clipSpaceDepth, 1.0f),
	};
}

//----------------------------------------------------------------------------------------
void GeometryHelper::CreateCenteredQuad(float clipSpaceDepth, float halfSize, std::vector<V4Float32>& vertexPositions) const
{
	vertexPositions = {
	  V4Float32(-halfSize, -halfSize, clipSpaceDepth, 1.0f),
	  V4Float32(-halfSize, halfSize, clipSpaceDepth, 1.0f),
	  V4Float32(halfSize, halfSize, clipSpaceDepth, 1.0f),
	  V4Float32(-halfSize, -halfSize, clipSpaceDepth, 1.0f),
	  V4Float32(halfSize, halfSize, clipSpaceDepth, 1.0f),
	  V4Float32(halfSize, -halfSize, clipSpaceDepth, 1.0f),
	};
}

//----------------------------------------------------------------------------------------
void GeometryHelper::CreateFullscreenQuad(float clipSpaceDepth, std::vector<V4Float32>& vertexPositions) const
{
	vertexPositions = {
	  V4Float32(-1.0f, -1.0f, clipSpaceDepth, 1.0f),
	  V4Float32(-1.0f, 1.0f, clipSpaceDepth, 1.0f),
	  V4Float32(1.0f, 1.0f, clipSpaceDepth, 1.0f),
	  V4Float32(-1.0f, -1.0f, clipSpaceDepth, 1.0f),
	  V4Float32(1.0f, 1.0f, clipSpaceDepth, 1.0f),
	  V4Float32(1.0f, -1.0f, clipSpaceDepth, 1.0f),
	};
}

//----------------------------------------------------------------------------------------
void GeometryHelper::CreateIndexedQuad(float clipSpaceDepth, float minX, float maxX, float minY, float maxY, std::vector<V4Float32>& vertexPositions, std::vector<V1UInt16>& indices) const
{
	vertexPositions = {
	  V4Float32(minX, maxY, clipSpaceDepth, 1.0f),
	  V4Float32(minX, minY, clipSpaceDepth, 1.0f),
	  V4Float32(maxX, minY, clipSpaceDepth, 1.0f),
	  V4Float32(maxX, maxY, clipSpaceDepth, 1.0f),
	};
	indices = {V1UInt16(0), V1UInt16(1), V1UInt16(2), V1UInt16(0), V1UInt16(2), V1UInt16(3)};
}

//----------------------------------------------------------------------------------------
void GeometryHelper::CreateIndexedQuad(float clipSpaceDepth, float minX, float maxX, float minY, float maxY, std::vector<V4Float32>& vertexPositions, std::vector<V1UInt32>& indices) const
{
	vertexPositions = {
	  V4Float32(minX, maxY, clipSpaceDepth, 1.0f),
	  V4Float32(minX, minY, clipSpaceDepth, 1.0f),
	  V4Float32(maxX, minY, clipSpaceDepth, 1.0f),
	  V4Float32(maxX, maxY, clipSpaceDepth, 1.0f),
	};
	indices = {V1UInt32(0), V1UInt32(1), V1UInt32(2), V1UInt32(0), V1UInt32(2), V1UInt32(3)};
}

//----------------------------------------------------------------------------------------
void GeometryHelper::CreateDiagonalLineVertices(std::vector<V4Float32>& vertexPositions) const
{
	vertexPositions = {
	  V4Float32(-1.0f, -0.95f, 0.5f, 1.0f),
	  V4Float32(0.95f, 1.0f, 0.5f, 1.0f),
	  V4Float32(-1.0f, -0.45f, 0.5f, 1.0f),
	  V4Float32(0.45f, 1.0f, 0.5f, 1.0f),
	  V4Float32(-0.45f, -1.0f, 0.5f, 1.0f),
	  V4Float32(1.0f, 0.45f, 0.5f, 1.0f),
	  V4Float32(-1.0f, 0.45f, 0.5f, 1.0f),
	  V4Float32(-0.45f, 1.0f, 0.5f, 1.0f),
	  V4Float32(0.45f, -1.0f, 0.5f, 1.0f),
	  V4Float32(1.0f, -0.45f, 0.5f, 1.0f),
	};
}

//----------------------------------------------------------------------------------------
// Model creation methods
//----------------------------------------------------------------------------------------
void GeometryHelper::CreateModelStanfordBunny(std::vector<V3Float32>& vertexPositions, std::vector<V3Float32>& vertexNormals, std::vector<V1UInt32>& indices) const
{
	vertexPositions = GetStanfordBunnyVertices();
	indices = GetStanfordBunnyIndices();
	std::vector<V2Float32> textureCoords(vertexPositions.size(), {0.0f, 0.0f});
	Normalize(vertexPositions);
	BuildVertexNormalsTriangles(vertexPositions, indices, vertexNormals, textureCoords);
}

//----------------------------------------------------------------------------------------
void GeometryHelper::CreateModelUtahTeapot(std::vector<V3Float32>& vertexPositions) const
{
	vertexPositions = GetUtahTeapotVertices();
	Normalize(vertexPositions);
}

//----------------------------------------------------------------------------------------
// Helper methods
//----------------------------------------------------------------------------------------
void GeometryHelper::GetExtents(const std::vector<V3Float32>& vertexPositions, V3Float64& minExtents, V3Float64& maxExtents, bool extentsHaveExistingValues)
{
	// Make sure at least one vertex has been defined
	if (vertexPositions.empty())
	{
		return;
	}

	// Calculate the extents of the triangulation
	if (!extentsHaveExistingValues)
	{
		maxExtents = V3Float64((double)vertexPositions[0].X(), (double)vertexPositions[0].Y(), (double)vertexPositions[0].Z());
		minExtents = maxExtents;
	}
	for (const auto& vertex : vertexPositions)
	{
		maxExtents = V3Float64((((double)vertex.X() > maxExtents.X()) ? (double)vertex.X() : maxExtents.X()), (((double)vertex.Y() > maxExtents.Y()) ? (double)vertex.Y() : maxExtents.Y()), (((double)vertex.Z() > maxExtents.Z()) ? (double)vertex.Z() : maxExtents.Z()));
		minExtents = V3Float64((((double)vertex.X() < minExtents.X()) ? (double)vertex.X() : minExtents.X()), (((double)vertex.Y() < minExtents.Y()) ? (double)vertex.Y() : minExtents.Y()), (((double)vertex.Z() < minExtents.Z()) ? (double)vertex.Z() : minExtents.Z()));
	}
}

//----------------------------------------------------------------------------------------
void GeometryHelper::Normalize(std::vector<V3Float32>& vertexPositions, V3Float64& minExtents, V3Float64& maxExtents)
{
	// Calculate the total dimensions of the model
	V3Float64 modelDimensions = V3Float64(maxExtents.X() - minExtents.X(), maxExtents.Y() - minExtents.Y(), maxExtents.Z() - minExtents.Z());

	// Determine the largest dimension of the model
	double largestDimension = 0;
	largestDimension = (largestDimension < modelDimensions.X()) ? modelDimensions.X() : largestDimension;
	largestDimension = (largestDimension < modelDimensions.Y()) ? modelDimensions.Y() : largestDimension;
	largestDimension = (largestDimension < modelDimensions.Z()) ? modelDimensions.Z() : largestDimension;

	// Normalize the mesh within the range -1.0 to 1.0, centered on the origin.
	V3Float64 offset(modelDimensions.X() * (1.0 / largestDimension), modelDimensions.Y() * (1.0 / largestDimension), modelDimensions.Z() * (1.0 / largestDimension));
	auto scaleFactor = (2.0 / largestDimension);
	for (auto& vertex : vertexPositions)
	{
		vertex = V3Float32((float)((((double)vertex.X() - minExtents.X()) * scaleFactor) - offset.X()), (float)((((double)vertex.Y() - minExtents.Y()) * scaleFactor) - offset.Y()), (float)((((double)vertex.Z() - minExtents.Z()) * scaleFactor) - offset.Z()));
	}
}

//----------------------------------------------------------------------------------------
void GeometryHelper::Normalize(std::vector<V3Float32>& vertexPositions)
{
	V3Float64 minExtents;
	V3Float64 maxExtents;
	GetExtents(vertexPositions, minExtents, maxExtents);
	Normalize(vertexPositions, minExtents, maxExtents);
}

//----------------------------------------------------------------------------------------
void GeometryHelper::BuildVertexNormalsTriangles(std::vector<V3Float32>& vertexPositions, std::vector<V1UInt32>& indices, std::vector<V3Float32>& vertexNormals, std::vector<V2Float32>& textureCoords, bool useSmoothShading, float angleCutoffInDegrees)
{
	// Ensure at least one triangle has been defined
	if (indices.size() < 3)
	{
		return;
	}

	// Build a list of which points are referenced by which surfaces, and build the surface normals for all triangles in
	// the model.
	uint32_t triangleCount = (uint32_t)indices.size() / 3;
	std::vector<std::list<uint32_t>> pointReferenceArray(useSmoothShading ? vertexPositions.size() : 0);
	std::vector<V3Float32> surfaceNormals(triangleCount);
	for (uint32_t triangleNo = 0, pointNo = 0; triangleNo < triangleCount; ++triangleNo, pointNo += 3)
	{
		// Add references to this surface in the point reference array
		auto pointIndex1 = indices[pointNo + 0].X();
		auto pointIndex2 = indices[pointNo + 1].X();
		auto pointIndex3 = indices[pointNo + 2].X();
		if (useSmoothShading)
		{
			pointReferenceArray[pointIndex1].push_back(triangleNo);
			pointReferenceArray[pointIndex2].push_back(triangleNo);
			pointReferenceArray[pointIndex3].push_back(triangleNo);
		}

		// Calculate two direction vectors on the surface to define the surface plane
		V3Float32 surfaceVector1;
		surfaceVector1.X() = vertexPositions[pointIndex1].X() - vertexPositions[pointIndex3].X();
		surfaceVector1.Y() = vertexPositions[pointIndex1].Y() - vertexPositions[pointIndex3].Y();
		surfaceVector1.Z() = vertexPositions[pointIndex1].Z() - vertexPositions[pointIndex3].Z();
		V3Float32 surfaceVector2;
		surfaceVector2.X() = vertexPositions[pointIndex2].X() - vertexPositions[pointIndex3].X();
		surfaceVector2.Y() = vertexPositions[pointIndex2].Y() - vertexPositions[pointIndex3].Y();
		surfaceVector2.Z() = vertexPositions[pointIndex2].Z() - vertexPositions[pointIndex3].Z();

		// Calculate the normal for the plane representing the surface using the vector cross product
		V3Float32 surfaceNormal;
		surfaceNormal.X() = (surfaceVector1.Y() * surfaceVector2.Z()) - (surfaceVector1.Z() * surfaceVector2.Y());
		surfaceNormal.Y() = (surfaceVector1.Z() * surfaceVector2.X()) - (surfaceVector1.X() * surfaceVector2.Z());
		surfaceNormal.Z() = (surfaceVector1.X() * surfaceVector2.Y()) - (surfaceVector1.Y() * surfaceVector2.X());

		// Normalize the vector
		double normalMagnitude = std::sqrt((double)(surfaceNormal.X() * surfaceNormal.X()) + (double)(surfaceNormal.Y() * surfaceNormal.Y()) + (double)(surfaceNormal.Z() * surfaceNormal.Z()));
		if (normalMagnitude != 0)
		{
			surfaceNormal.X() /= (float)normalMagnitude;
			surfaceNormal.Y() /= (float)normalMagnitude;
			surfaceNormal.Z() /= (float)normalMagnitude;
		}
		surfaceNormals[triangleNo] = surfaceNormal;
	}

	// Calculate the cosine cutoff value for smoothing normals between surfaces
	const double pi = 3.141592653589793238;
	float cosineCutoff = 0;
	if (useSmoothShading)
	{
		cosineCutoff = std::cos(angleCutoffInDegrees * (float)(pi / 180.0));
	}

	// Create unique vertices per surface to represent the surface normals correctly
	std::vector<V3Float32> pointsNew;
	pointsNew.reserve((size_t)triangleCount * 3);
	std::vector<V3Float32> normalsNew;
	normalsNew.reserve((size_t)triangleCount * 3);
	std::vector<V2Float32> texCoordNew;
	texCoordNew.reserve((size_t)triangleCount * 3);
	std::vector<V1UInt32> indicesNew(indices.size());
	for (uint32_t triangleNo = 0, pointNo = 0; triangleNo < triangleCount; ++triangleNo, pointNo += 3)
	{
		uint32_t triangleIndices[3];
		triangleIndices[0] = indices[pointNo + 0].X();
		triangleIndices[1] = indices[pointNo + 1].X();
		triangleIndices[2] = indices[pointNo + 2].X();
		const V3Float32& surfaceNormal = surfaceNormals[triangleNo];

		// If we're not using smooth shading for this object, use the surface normal directly for each point in this
		// surface, and continue to the next surface.
		if (!useSmoothShading)
		{
			pointsNew[pointNo + 0] = vertexPositions[triangleIndices[0]];
			pointsNew[pointNo + 1] = vertexPositions[triangleIndices[1]];
			pointsNew[pointNo + 2] = vertexPositions[triangleIndices[2]];
			normalsNew[pointNo + 0] = surfaceNormal;
			normalsNew[pointNo + 1] = surfaceNormal;
			normalsNew[pointNo + 2] = surfaceNormal;
			indicesNew[pointNo + 0].X() = pointNo + 0;
			indicesNew[pointNo + 1].X() = pointNo + 1;
			indicesNew[pointNo + 2].X() = pointNo + 2;
			continue;
		}

		// Calculate normals for each vertex in this surface
		for (uint32_t pointOffsetInTriangle = 0; pointOffsetInTriangle < 3; ++pointOffsetInTriangle)
		{
			// Calculate the total extents of the different normals which share this vertex
			V3Float32 normalExtentLower = surfaceNormal;
			V3Float32 normalExtentUpper = surfaceNormal;
			const std::list<uint32_t>& surfaceReferencesForPoint = pointReferenceArray[triangleIndices[pointOffsetInTriangle]];
			for (auto surfaceNo : surfaceReferencesForPoint)
			{
				if (surfaceNo != triangleNo)
				{
					const V3Float32& targetSurfaceNormal = surfaceNormals[surfaceNo];
					float angleBetweenCurrentSurfaceNormal = surfaceNormal.X() * targetSurfaceNormal.X() + surfaceNormal.Y() * targetSurfaceNormal.Y() + surfaceNormal.Z() * targetSurfaceNormal.Z();
					if (angleBetweenCurrentSurfaceNormal > cosineCutoff)
					{
						normalExtentLower.X() = (targetSurfaceNormal.X() < normalExtentLower.X()) ? targetSurfaceNormal.X() : normalExtentLower.X();
						normalExtentLower.Y() = (targetSurfaceNormal.Y() < normalExtentLower.Y()) ? targetSurfaceNormal.Y() : normalExtentLower.Y();
						normalExtentLower.Z() = (targetSurfaceNormal.Z() < normalExtentLower.Z()) ? targetSurfaceNormal.Z() : normalExtentLower.Z();
						normalExtentUpper.X() = (targetSurfaceNormal.X() > normalExtentUpper.X()) ? targetSurfaceNormal.X() : normalExtentUpper.X();
						normalExtentUpper.Y() = (targetSurfaceNormal.Y() > normalExtentUpper.Y()) ? targetSurfaceNormal.Y() : normalExtentUpper.Y();
						normalExtentUpper.Z() = (targetSurfaceNormal.Z() > normalExtentUpper.Z()) ? targetSurfaceNormal.Z() : normalExtentUpper.Z();
					}
				}
			}

			// Calculate the vertex normal
			V3Float32 vertexNormal;
			vertexNormal.X() = (normalExtentLower.X() + ((normalExtentUpper.X() - normalExtentLower.X()) / 2.0f));
			vertexNormal.Y() = (normalExtentLower.Y() + ((normalExtentUpper.Y() - normalExtentLower.Y()) / 2.0f));
			vertexNormal.Z() = (normalExtentLower.Z() + ((normalExtentUpper.Z() - normalExtentLower.Z()) / 2.0f));

			// Normalize the vector
			double normalMagnitude = std::sqrt((double)(vertexNormal.X() * vertexNormal.X()) + (double)(vertexNormal.Y() * vertexNormal.Y()) + (double)(vertexNormal.Z() * vertexNormal.Z()));
			if (normalMagnitude != 0)
			{
				vertexNormal.X() /= (float)normalMagnitude;
				vertexNormal.Y() /= (float)normalMagnitude;
				vertexNormal.Z() /= (float)normalMagnitude;
			}

			// Suppress duplicate vertices
			bool vertexIsDuplicate = false;
			uint32_t newIndexToShare = 0;
			auto surfaceReferenceIterator = surfaceReferencesForPoint.begin();
			while (!vertexIsDuplicate && (surfaceReferenceIterator != surfaceReferencesForPoint.end()))
			{
				uint32_t surfaceNo = *surfaceReferenceIterator;
				if (surfaceNo < triangleNo)
				{
					uint32_t searchPointNo = 0;
					while (!vertexIsDuplicate && (searchPointNo < 3))
					{
						if (indices[(surfaceNo * 3) + searchPointNo].X() == triangleIndices[pointOffsetInTriangle])
						{
							auto otherIndex = indicesNew[(surfaceNo * 3) + searchPointNo].X();
							const V3Float32& otherNormal = normalsNew[otherIndex];
							if ((vertexNormal.X() == otherNormal.X()) && (vertexNormal.Y() == otherNormal.Y()) && (vertexNormal.Z() == otherNormal.Z()))
							{
								vertexIsDuplicate = true;
								newIndexToShare = otherIndex;
								continue;
							}
						}
						++searchPointNo;
					}
				}
				++surfaceReferenceIterator;
			}

			// Output this vertex
			if (!vertexIsDuplicate)
			{
				indicesNew[pointNo + pointOffsetInTriangle].X() = (uint32_t)pointsNew.size();
				pointsNew.push_back(vertexPositions[triangleIndices[pointOffsetInTriangle]]);
				normalsNew.push_back(vertexNormal);
				texCoordNew.push_back(textureCoords[triangleIndices[pointOffsetInTriangle]]);
			}
			else
			{
				indicesNew[pointNo + pointOffsetInTriangle].X() = newIndexToShare;
			}
		}
	}

	// Update the geometric data for this object
	vertexPositions = std::move(pointsNew);
	vertexNormals = std::move(normalsNew);
	indices = std::move(indicesNew);
	textureCoords = std::move(texCoordNew);
}

//----------------------------------------------------------------------------------------
void GeometryHelper::BuildVertexNormalsTriStrip(std::vector<V3Float32>& vertexPositions, std::vector<V1UInt32>& indices, std::vector<V3Float32>& vertexNormals)
{
	// Ensure at least one triangle has been defined
	if (indices.size() < 3)
	{
		return;
	}

	// Build a list of which points are referenced by which surfaces, and build the surface normals for all triangles in
	// the model.
	auto indexCount = (uint32_t)indices.size();
	std::vector<std::list<uint32_t>> pointReferenceArray(vertexPositions.size());
	std::vector<V3Float32> surfaceNormals(indexCount - 2);
	uint32_t triangleNo = 0;
	uint32_t pointIndex1 = indices[0].X();
	uint32_t pointIndex2 = indices[1].X();
	uint32_t pointIndex3;
	auto primitiveRestartValue = IndexAttribute<V1UInt32>::GetPrimitiveRestartValue().X();
	for (uint32_t indexNo = 2; indexNo < indexCount; ++indexNo)
	{
		// Grab the point defining the next triangle in the strip
		pointIndex3 = indices[indexNo].X();

		// Handle primitive restart
		if (pointIndex3 == primitiveRestartValue)
		{
			if ((indexNo + 2) < indexCount)
			{
				pointIndex1 = indices[indexNo + 1].X();
				pointIndex2 = indices[indexNo + 2].X();
				indexNo += 2;
			}
			continue;
		}

		// Add references to this surface in the point reference array
		pointReferenceArray[pointIndex1].push_back(triangleNo);
		pointReferenceArray[pointIndex2].push_back(triangleNo);
		pointReferenceArray[pointIndex3].push_back(triangleNo);

		// Calculate two direction vectors on the surface to define the surface plane
		V3Float32 surfaceVector1;
		surfaceVector1.X() = vertexPositions[pointIndex1].X() - vertexPositions[pointIndex3].X();
		surfaceVector1.Y() = vertexPositions[pointIndex1].Y() - vertexPositions[pointIndex3].Y();
		surfaceVector1.Z() = vertexPositions[pointIndex1].Z() - vertexPositions[pointIndex3].Z();
		V3Float32 surfaceVector2;
		surfaceVector2.X() = vertexPositions[pointIndex2].X() - vertexPositions[pointIndex3].X();
		surfaceVector2.Y() = vertexPositions[pointIndex2].Y() - vertexPositions[pointIndex3].Y();
		surfaceVector2.Z() = vertexPositions[pointIndex2].Z() - vertexPositions[pointIndex3].Z();

		// Calculate the normal for the plane representing the surface using the vector cross product
		V3Float32 surfaceNormal;
		surfaceNormal.X() = (surfaceVector1.Y() * surfaceVector2.Z()) - (surfaceVector1.Z() * surfaceVector2.Y());
		surfaceNormal.Y() = (surfaceVector1.Z() * surfaceVector2.X()) - (surfaceVector1.X() * surfaceVector2.Z());
		surfaceNormal.Z() = (surfaceVector1.X() * surfaceVector2.Y()) - (surfaceVector1.Y() * surfaceVector2.X());

		// Normalize the vector
		double normalMagnitude = std::sqrt((double)(surfaceNormal.X() * surfaceNormal.X()) + (double)(surfaceNormal.Y() * surfaceNormal.Y()) + (double)(surfaceNormal.Z() * surfaceNormal.Z()));
		if (normalMagnitude != 0)
		{
			surfaceNormal.X() /= (float)normalMagnitude;
			surfaceNormal.Y() /= (float)normalMagnitude;
			surfaceNormal.Z() /= (float)normalMagnitude;
		}

		// Since the winding order changes with each successive triangle in a tristrip, we explicitly reverse the
		// surface normal for every second triangle here, to ensure all triangles in the strip have the same
		// definition of front and back.
		if ((triangleNo % 2) != 0)
		{
			surfaceNormal.X() = -surfaceNormal.X();
			surfaceNormal.Y() = -surfaceNormal.Y();
			surfaceNormal.Z() = -surfaceNormal.Z();
		}
		surfaceNormals[triangleNo] = surfaceNormal;

		// Shift the indices down ready for the next primitive
		pointIndex1 = pointIndex2;
		pointIndex2 = pointIndex3;

		++triangleNo;
	}

	// Now that we've computed the surface normals, compute the vertex normals by averaging the surface normals over all
	// surfaces which share each vertex.
	vertexNormals.resize(vertexPositions.size());
	V3Float32 normalExtentLower{};
	V3Float32 normalExtentUpper{};
	for (uint32_t pointNo = 0; pointNo < pointReferenceArray.size(); ++pointNo)
	{
		// Initialize the normal for this vertex
		vertexNormals[pointNo].X() = 0;
		vertexNormals[pointNo].Y() = 0;
		vertexNormals[pointNo].Z() = 0;

		// If no surfaces reference this vertex, skip it.
		if (pointReferenceArray[pointNo].empty())
		{
			continue;
		}

		// Calculate the total extents of the different normals which share this vertex
		bool extentInitialized = false;
		for (auto pointReference : pointReferenceArray[pointNo])
		{
			normalExtentLower.X() = (float)((!extentInitialized || (surfaceNormals[pointReference].X() < normalExtentLower.X())) ? surfaceNormals[pointReference].X() : normalExtentLower.X());
			normalExtentLower.Y() = (float)((!extentInitialized || (surfaceNormals[pointReference].Y() < normalExtentLower.Y())) ? surfaceNormals[pointReference].Y() : normalExtentLower.Y());
			normalExtentLower.Z() = (float)((!extentInitialized || (surfaceNormals[pointReference].Z() < normalExtentLower.Z())) ? surfaceNormals[pointReference].Z() : normalExtentLower.Z());
			normalExtentUpper.X() = (float)((!extentInitialized || (surfaceNormals[pointReference].X() > normalExtentUpper.X())) ? surfaceNormals[pointReference].X() : normalExtentUpper.X());
			normalExtentUpper.Y() = (float)((!extentInitialized || (surfaceNormals[pointReference].Y() > normalExtentUpper.Y())) ? surfaceNormals[pointReference].Y() : normalExtentUpper.Y());
			normalExtentUpper.Z() = (float)((!extentInitialized || (surfaceNormals[pointReference].Z() > normalExtentUpper.Z())) ? surfaceNormals[pointReference].Z() : normalExtentUpper.Z());
			extentInitialized = true;
		}

		// Calculate the vertex normal
		vertexNormals[pointNo].X() = (normalExtentLower.X() + ((normalExtentUpper.X() - normalExtentLower.X()) / 2.0f));
		vertexNormals[pointNo].Y() = (normalExtentLower.Y() + ((normalExtentUpper.Y() - normalExtentLower.Y()) / 2.0f));
		vertexNormals[pointNo].Z() = (normalExtentLower.Z() + ((normalExtentUpper.Z() - normalExtentLower.Z()) / 2.0f));

		// Normalize the vector
		double normalMagnitude = std::sqrt((double)(vertexNormals[pointNo].X() * vertexNormals[pointNo].X()) + (double)(vertexNormals[pointNo].Y() * vertexNormals[pointNo].Y()) + (double)(vertexNormals[pointNo].Z() * vertexNormals[pointNo].Z()));
		if (normalMagnitude != 0)
		{
			vertexNormals[pointNo].X() /= (float)normalMagnitude;
			vertexNormals[pointNo].Y() /= (float)normalMagnitude;
			vertexNormals[pointNo].Z() /= (float)normalMagnitude;
		}
	}
}

} // namespace cobalt::graphics::testing
