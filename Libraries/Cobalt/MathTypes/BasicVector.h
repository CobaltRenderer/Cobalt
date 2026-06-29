// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include <cstddef>
namespace cobalt { namespace graphics {

// Define a basic no frills vector type. This is never intended to be anything more than a simple container for the
// data. If the calling application desires more complete functionality without needing to reinterpret cast or convert
// values into and out of the renderer API, the COBALT_USE_CUSTOM_MATH_TYPES define can be used to suppress the
// use of this type. By providing the appropriate set of typedefs, the calling application can substitute their own math
// types and make them appear as the native types used on the renderer API, as long as a type with a compatible memory
// layout is used.
template<class T, size_t D>
struct BasicVector
{};

template<class T>
struct BasicVector<T, 1>
{
	// Template parameters
	using ElementType = T;
	static const size_t Dimensions = 1;

	// Constructors
	BasicVector() = default;
	BasicVector(const BasicVector& other) = default;
	BasicVector(BasicVector&& other) = default;
	BasicVector(T x) // NOLINT(hicpp-explicit-conversions)
	: val{{x}}
	{}

	// Operators
	BasicVector& operator=(const BasicVector& other) = default;
	BasicVector& operator=(BasicVector&& other) = default;
	bool operator==(const BasicVector& other) const
	{
		return (val[0] == other.val[0]);
	}
	bool operator!=(const BasicVector& other) const
	{
		return !(*this == other);
	}

	// Access methods
	T& X()
	{
		return val[0];
	}
	const T& X() const
	{
		return val[0];
	}
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
		return Dimensions;
	}

	// Public data members
	T val[Dimensions];
};

template<class T>
struct BasicVector<T, 2>
{
	// Template parameters
	using ElementType = T;
	static const size_t Dimensions = 2;

	// Constructors
	BasicVector() = default;
	BasicVector(const BasicVector& other) = default;
	BasicVector(BasicVector&& other) = default;
	BasicVector(T x, T y)
	: val{{x}, {y}}
	{}

	// Operators
	BasicVector& operator=(const BasicVector& other) = default;
	BasicVector& operator=(BasicVector&& other) = default;
	bool operator==(const BasicVector& other) const
	{
		return (val[0] == other.val[0]) && (val[1] == other.val[1]);
	}
	bool operator!=(const BasicVector& other) const
	{
		return !(*this == other);
	}

	// Access methods
	T& X()
	{
		return val[0];
	}
	T& Y()
	{
		return val[1];
	}
	const T& X() const
	{
		return val[0];
	}
	const T& Y() const
	{
		return val[1];
	}
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
		return Dimensions;
	}

	// Public data members
	T val[Dimensions];
};

template<class T>
struct BasicVector<T, 3>
{
	// Template parameters
	using ElementType = T;
	static const size_t Dimensions = 3;

	// Constructors
	BasicVector() = default;
	BasicVector(const BasicVector& other) = default;
	BasicVector(BasicVector&& other) = default;
	BasicVector(T x, T y, T z)
	: val{{x}, {y}, {z}}
	{}

	// Operators
	BasicVector& operator=(const BasicVector& other) = default;
	BasicVector& operator=(BasicVector&& other) = default;
	bool operator==(const BasicVector& other) const
	{
		return (val[0] == other.val[0]) && (val[1] == other.val[1]) && (val[2] == other.val[2]);
	}
	bool operator!=(const BasicVector& other) const
	{
		return !(*this == other);
	}

	// Access methods
	T& X()
	{
		return val[0];
	}
	T& Y()
	{
		return val[1];
	}
	T& Z()
	{
		return val[2];
	}
	const T& X() const
	{
		return val[0];
	}
	const T& Y() const
	{
		return val[1];
	}
	const T& Z() const
	{
		return val[2];
	}
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
		return Dimensions;
	}

	// Public data members
	T val[Dimensions];
};

template<class T>
struct BasicVector<T, 4>
{
	// Template parameters
	using ElementType = T;
	static const size_t Dimensions = 4;

	// Constructors
	BasicVector() = default;
	BasicVector(const BasicVector& other) = default;
	BasicVector(BasicVector&& other) = default;
	BasicVector(T x, T y, T z, T w)
	: val{{x}, {y}, {z}, {w}}
	{}

	// Operators
	BasicVector& operator=(const BasicVector& other) = default;
	BasicVector& operator=(BasicVector&& other) = default;
	bool operator==(const BasicVector& other) const
	{
		return (val[0] == other.val[0]) && (val[1] == other.val[1]) && (val[2] == other.val[2]) && (val[3] == other.val[3]);
	}
	bool operator!=(const BasicVector& other) const
	{
		return !(*this == other);
	}

	// Access methods
	T& X()
	{
		return val[0];
	}
	T& Y()
	{
		return val[1];
	}
	T& Z()
	{
		return val[2];
	}
	T& W()
	{
		return val[3];
	}
	const T& X() const
	{
		return val[0];
	}
	const T& Y() const
	{
		return val[1];
	}
	const T& Z() const
	{
		return val[2];
	}
	const T& W() const
	{
		return val[3];
	}
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
		return Dimensions;
	}

	// Public data members
	T val[Dimensions];
};

}} // namespace cobalt::graphics
