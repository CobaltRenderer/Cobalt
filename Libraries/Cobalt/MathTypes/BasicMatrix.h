// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include <cstddef>
namespace cobalt { namespace graphics {

// Define a basic no frills matrix type. This is never intended to be anything more than a simple container for the
// data. If the calling application desires more complete functionality without needing to reinterpret cast or convert
// values into and out of the renderer API, the COBALT_USE_CUSTOM_MATH_TYPES define can be used to suppress the
// use of this type. By providing the appropriate set of typedefs, the calling application can substitute their own math
// types and make them appear as the native types used on the renderer API, as long as a type with a compatible memory
// layout is used. Note that this matrix type is neither row major or column major, it is agnostic to the layout of the
// matrix in memory. The supplied matrix data can be in any form, and the renderers themselves don't need to know which
// form is used, the data is simply passed on to the application supplied shaders to interpret as they see fit.
template<class T, size_t D>
struct BasicMatrix
{
	// Template parameters
	using ElementType = T;
	static const size_t Dimensions = D;

	// Constructors
	BasicMatrix() = default; // NOLINT(hicpp-member-init)
	BasicMatrix(const BasicMatrix& other) = default;
	BasicMatrix(BasicMatrix&& other) = default;

	// Operators
	BasicMatrix& operator=(const BasicMatrix& other) = default;
	BasicMatrix& operator=(BasicMatrix&& other) = default;

	// Access methods
	T* data()
	{
		return &val[0];
	}
	const T* data() const
	{
		return &val[0];
	}
	size_t size() const
	{
		return Dimensions * Dimensions;
	}

	// Public data members
	T val[Dimensions * Dimensions];
};

}} // namespace cobalt::graphics
