// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include <algorithm>
#include <cstring>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// State layout building methods
//----------------------------------------------------------------------------------------
constexpr size_t Direct3DStateBufferLayout::GetDataTypeByteSize(DataType dataType)
{
	switch (dataType)
	{
	case DataType::Boolean:
	case DataType::Int32:
		return sizeof(int);
	case DataType::UInt32:
		return sizeof(uint32_t);
	case DataType::Float32:
		return sizeof(float);
	case DataType::Float64:
		return sizeof(double);
	}
	UNREACHABLE_CONSTEXPR();
	return 0;
}

//----------------------------------------------------------------------------------------
// Buffer packing helpers
//----------------------------------------------------------------------------------------
constexpr size_t Direct3DStateBufferLayout::AlignToRow(size_t size)
{
	return ((size + 15) / 16) * 16;
}

// Width is the number of components in a storage vector, height the number of storage vectors.
// Reflection maps row-major matrices to rows and column-major matrices to columns. Zero dimensions
// represent scalar/vector fields. CPU values retain their supplied order, only padding is inserted.
constexpr size_t Direct3DStateBufferLayout::VectorSize(size_t scalarSize, size_t width)
{
	return scalarSize * (width > 0 ? width : 1);
}

constexpr size_t Direct3DStateBufferLayout::ElementSize(size_t scalarSize, size_t width, size_t height)
{
	const size_t vectorSize = VectorSize(scalarSize, width);
	return (height > 0 ? height - 1 : 0) * AlignToRow(vectorSize) + vectorSize;
}

constexpr size_t Direct3DStateBufferLayout::ArrayStride(size_t scalarSize, size_t width, size_t height)
{
	return AlignToRow(ElementSize(scalarSize, width, height));
}

inline void Direct3DStateBufferLayout::CopyValue(uint8_t* destination, const uint8_t* source, size_t sourceSize, size_t scalarSize, size_t width, size_t height)
{
	const size_t vectorSize = VectorSize(scalarSize, width);
	const size_t vectorCount = height > 0 ? height : 1;
	const size_t vectorStride = AlignToRow(vectorSize);
	for (size_t i = 0; i < vectorCount && sourceSize > 0; ++i)
	{
		const size_t copySize = std::min(sourceSize, vectorSize);
		std::memcpy(destination + i * vectorStride, source, copySize);
		source += copySize;
		sourceSize -= copySize;
	}
}

} // namespace cobalt::graphics
