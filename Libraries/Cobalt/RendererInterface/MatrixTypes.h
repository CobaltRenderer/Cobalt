// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#ifndef COBALT_USE_CUSTOM_MATH_TYPES
#include "VectorTypes.h"
#include <Cobalt/MathTypes/MathTypes.pkg>
namespace cobalt { namespace graphics {

// If custom math types haven't been defined, include our supplied MathTypes library and define our matrix type names.
// Where custom math types are being used, these type names will be defined externally.
using M2Float32 = BasicMatrix<float, 2>;
using M3Float32 = BasicMatrix<float, 3>;
using M4Float32 = BasicMatrix<float, 4>;
using M2Float64 = BasicMatrix<double, 2>;
using M3Float64 = BasicMatrix<double, 3>;
using M4Float64 = BasicMatrix<double, 4>;

}} // namespace cobalt::graphics
#endif
