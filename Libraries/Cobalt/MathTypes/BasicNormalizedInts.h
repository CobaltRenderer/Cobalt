// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include <cstddef>
namespace cobalt { namespace graphics {

// These basic structures are stand-ins for signed and unsigned normalized integer types, suitable for use in computer
// graphics. This type has no native implementation in C++, so it is left to the application to generate and provide
// this data in an appropriate form.
template<size_t Size>
struct BasicNormBase
{
	unsigned char data[Size];
};
template<size_t Size>
struct BasicUNormBase
{
	unsigned char data[Size];
};
using BasicNorm8 = BasicNormBase<1>;
using BasicNorm16 = BasicNormBase<2>;
using BasicNorm32 = BasicNormBase<4>;
using BasicUNorm8 = BasicUNormBase<1>;
using BasicUNorm16 = BasicUNormBase<2>;
using BasicUNorm32 = BasicUNormBase<4>;

}} // namespace cobalt::graphics
