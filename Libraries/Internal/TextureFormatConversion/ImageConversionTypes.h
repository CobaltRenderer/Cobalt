// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include <cstddef>
namespace cobalt { namespace graphics {

class ImageConversionTypes
{
public:
	// Primitive element types
	using Int8 = signed char;
	using Int16 = short;
	using Int32 = int;
	using UInt8 = unsigned char;
	using UInt16 = unsigned short;
	using UInt32 = unsigned int;
	using Float32 = float;
	using Float64 = double;

	// Non-native element types
	template<size_t Size>
	struct NormBase
	{
		unsigned char data[Size];
	};
	template<size_t Size>
	struct UNormBase
	{
		unsigned char data[Size];
	};
	using Norm8 = NormBase<1>;
	using Norm16 = NormBase<2>;
	using Norm24 = NormBase<3>;
	using Norm32 = NormBase<4>;
	using UNorm8 = UNormBase<1>;
	using UNorm16 = UNormBase<2>;
	using UNorm24 = UNormBase<3>;
	using UNorm32 = UNormBase<4>;
	struct Float16
	{
		unsigned char data[2];
	};

	// Vector types
	template<class T, size_t S>
	struct BasicVector
	{
		T data[S];
		using ElementType = T;
	};

	// Shorthand vector type names
	using V1Int8 = BasicVector<Int8, 1>;
	using V1Int16 = BasicVector<Int16, 1>;
	using V1Int32 = BasicVector<Int32, 1>;
	using V1UInt8 = BasicVector<UInt8, 1>;
	using V1UInt16 = BasicVector<UInt16, 1>;
	using V1UInt32 = BasicVector<UInt32, 1>;
	using V1Norm8 = BasicVector<Norm8, 1>;
	using V1Norm16 = BasicVector<Norm16, 1>;
	using V1Norm32 = BasicVector<Norm32, 1>;
	using V1UNorm8 = BasicVector<UNorm8, 1>;
	using V1UNorm16 = BasicVector<UNorm16, 1>;
	using V1UNorm24 = BasicVector<UNorm24, 1>;
	using V1UNorm32 = BasicVector<UNorm32, 1>;
	using V1Float16 = BasicVector<Float16, 1>;
	using V1Float32 = BasicVector<Float32, 1>;
	using V1Float64 = BasicVector<Float64, 1>;
	using V2Int8 = BasicVector<Int8, 2>;
	using V2Int16 = BasicVector<Int16, 2>;
	using V2Int32 = BasicVector<Int32, 2>;
	using V2UInt8 = BasicVector<UInt8, 2>;
	using V2UInt16 = BasicVector<UInt16, 2>;
	using V2UInt32 = BasicVector<UInt32, 2>;
	using V2Norm8 = BasicVector<Norm8, 2>;
	using V2Norm16 = BasicVector<Norm16, 2>;
	using V2Norm32 = BasicVector<Norm32, 2>;
	using V2UNorm8 = BasicVector<UNorm8, 2>;
	using V2UNorm16 = BasicVector<UNorm16, 2>;
	using V2UNorm24 = BasicVector<UNorm24, 2>;
	using V2UNorm32 = BasicVector<UNorm32, 2>;
	using V2Float16 = BasicVector<Float16, 2>;
	using V2Float32 = BasicVector<Float32, 2>;
	using V2Float64 = BasicVector<Float64, 2>;
	using V3Int8 = BasicVector<Int8, 3>;
	using V3Int16 = BasicVector<Int16, 3>;
	using V3Int32 = BasicVector<Int32, 3>;
	using V3UInt8 = BasicVector<UInt8, 3>;
	using V3UInt16 = BasicVector<UInt16, 3>;
	using V3UInt32 = BasicVector<UInt32, 3>;
	using V3Norm8 = BasicVector<Norm8, 3>;
	using V3Norm16 = BasicVector<Norm16, 3>;
	using V3Norm32 = BasicVector<Norm32, 3>;
	using V3UNorm8 = BasicVector<UNorm8, 3>;
	using V3UNorm16 = BasicVector<UNorm16, 3>;
	using V3UNorm24 = BasicVector<UNorm24, 3>;
	using V3UNorm32 = BasicVector<UNorm32, 3>;
	using V3Float16 = BasicVector<Float16, 3>;
	using V3Float32 = BasicVector<Float32, 3>;
	using V3Float64 = BasicVector<Float64, 3>;
	using V4Int8 = BasicVector<Int8, 4>;
	using V4Int16 = BasicVector<Int16, 4>;
	using V4Int32 = BasicVector<Int32, 4>;
	using V4UInt8 = BasicVector<UInt8, 4>;
	using V4UInt16 = BasicVector<UInt16, 4>;
	using V4UInt32 = BasicVector<UInt32, 4>;
	using V4Norm8 = BasicVector<Norm8, 4>;
	using V4Norm16 = BasicVector<Norm16, 4>;
	using V4Norm32 = BasicVector<Norm32, 4>;
	using V4UNorm8 = BasicVector<UNorm8, 4>;
	using V4UNorm16 = BasicVector<UNorm16, 4>;
	using V4UNorm24 = BasicVector<UNorm24, 4>;
	using V4UNorm32 = BasicVector<UNorm32, 4>;
	using V4Float16 = BasicVector<Float16, 4>;
	using V4Float32 = BasicVector<Float32, 4>;
	using V4Float64 = BasicVector<Float64, 4>;

	// Strongly typed and ordered pixel format names
	template<class T>
	struct PixelR : public BasicVector<T, 1>
	{};
	template<class T>
	struct PixelRG : public BasicVector<T, 2>
	{};
	template<class T>
	struct PixelRGB : public BasicVector<T, 3>
	{};
	template<class T>
	struct PixelRGBA : public BasicVector<T, 4>
	{};
	template<class T>
	struct PixelBGR : public BasicVector<T, 3>
	{};
	template<class T>
	struct PixelBGRA : public BasicVector<T, 4>
	{};
	template<class T>
	struct DataVector1 : public BasicVector<T, 1>
	{};
	template<class T>
	struct DataVector2 : public BasicVector<T, 2>
	{};
	template<class T>
	struct DataVector3 : public BasicVector<T, 3>
	{};
	template<class T>
	struct DataVector4 : public BasicVector<T, 4>
	{};
};

}} // namespace cobalt::graphics
