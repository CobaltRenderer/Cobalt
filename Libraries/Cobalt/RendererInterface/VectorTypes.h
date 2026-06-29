// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#ifndef COBALT_USE_CUSTOM_MATH_TYPES
#include <Cobalt/MathTypes/MathTypes.pkg>
namespace cobalt { namespace graphics {

// If custom math types haven't been defined, include our supplied MathTypes library and define our vector type names.
// Where custom math types are being used, these type names will be defined externally.
using V1Int8 = BasicVector<signed char, 1>;
using V1Int16 = BasicVector<short, 1>;
using V1Int32 = BasicVector<int, 1>;
using V1Int64 = BasicVector<long long, 1>;
using V1UInt8 = BasicVector<unsigned char, 1>;
using V1UInt16 = BasicVector<unsigned short, 1>;
using V1UInt32 = BasicVector<unsigned int, 1>;
using V1UInt64 = BasicVector<unsigned long long, 1>;
using V1Norm8 = BasicVector<BasicNorm8, 1>;
using V1Norm16 = BasicVector<BasicNorm16, 1>;
using V1Norm32 = BasicVector<BasicNorm32, 1>;
using V1UNorm8 = BasicVector<BasicUNorm8, 1>;
using V1UNorm16 = BasicVector<BasicUNorm16, 1>;
using V1UNorm32 = BasicVector<BasicUNorm32, 1>;
using V1Float16 = BasicVector<BasicHalfFloat, 1>;
using V1Float32 = BasicVector<float, 1>;
using V1Float64 = BasicVector<double, 1>;
using V2Int8 = BasicVector<signed char, 2>;
using V2Int16 = BasicVector<short, 2>;
using V2Int32 = BasicVector<int, 2>;
using V2Int64 = BasicVector<long long, 2>;
using V2UInt8 = BasicVector<unsigned char, 2>;
using V2UInt16 = BasicVector<unsigned short, 2>;
using V2UInt32 = BasicVector<unsigned int, 2>;
using V2UInt64 = BasicVector<unsigned long long, 2>;
using V2Norm8 = BasicVector<BasicNorm8, 2>;
using V2Norm16 = BasicVector<BasicNorm16, 2>;
using V2Norm32 = BasicVector<BasicNorm32, 2>;
using V2UNorm8 = BasicVector<BasicUNorm8, 2>;
using V2UNorm16 = BasicVector<BasicUNorm16, 2>;
using V2UNorm32 = BasicVector<BasicUNorm32, 2>;
using V2Float16 = BasicVector<BasicHalfFloat, 2>;
using V2Float32 = BasicVector<float, 2>;
using V2Float64 = BasicVector<double, 2>;
using V3Int8 = BasicVector<signed char, 3>;
using V3Int16 = BasicVector<short, 3>;
using V3Int32 = BasicVector<int, 3>;
using V3Int64 = BasicVector<long long, 3>;
using V3UInt8 = BasicVector<unsigned char, 3>;
using V3UInt16 = BasicVector<unsigned short, 3>;
using V3UInt32 = BasicVector<unsigned int, 3>;
using V3UInt64 = BasicVector<unsigned long long, 3>;
using V3Norm8 = BasicVector<BasicNorm8, 3>;
using V3Norm16 = BasicVector<BasicNorm16, 3>;
using V3Norm32 = BasicVector<BasicNorm32, 3>;
using V3UNorm8 = BasicVector<BasicUNorm8, 3>;
using V3UNorm16 = BasicVector<BasicUNorm16, 3>;
using V3UNorm32 = BasicVector<BasicUNorm32, 3>;
using V3Float16 = BasicVector<BasicHalfFloat, 3>;
using V3Float32 = BasicVector<float, 3>;
using V3Float64 = BasicVector<double, 3>;
using V4Int8 = BasicVector<signed char, 4>;
using V4Int16 = BasicVector<short, 4>;
using V4Int32 = BasicVector<int, 4>;
using V4Int64 = BasicVector<long long, 4>;
using V4UInt8 = BasicVector<unsigned char, 4>;
using V4UInt16 = BasicVector<unsigned short, 4>;
using V4UInt32 = BasicVector<unsigned int, 4>;
using V4UInt64 = BasicVector<unsigned long long, 4>;
using V4Norm8 = BasicVector<BasicNorm8, 4>;
using V4Norm16 = BasicVector<BasicNorm16, 4>;
using V4Norm32 = BasicVector<BasicNorm32, 4>;
using V4UNorm8 = BasicVector<BasicUNorm8, 4>;
using V4UNorm16 = BasicVector<BasicUNorm16, 4>;
using V4UNorm32 = BasicVector<BasicUNorm32, 4>;
using V4Float16 = BasicVector<BasicHalfFloat, 4>;
using V4Float32 = BasicVector<float, 4>;
using V4Float64 = BasicVector<double, 4>;

}} // namespace cobalt::graphics
#endif
