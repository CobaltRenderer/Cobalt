// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include <Cobalt/Debug/Debug.pkg>
namespace cobalt::graphics {

// Zero-cost dynamic_cast when we know the type. Asserts the type is as expected.
template<class TOut, class TIn>
TOut KnownDynamicCast(TIn input)
{
	static_assert(std::is_pointer<TOut>::value);
	static_assert(std::is_pointer<TIn>::value);
	static_assert(std::is_const<typename std::remove_pointer<TOut>::type>::value == std::is_const<typename std::remove_pointer<TIn>::type>::value);
#ifdef _DEBUG
	auto out = dynamic_cast<TOut>(input);
	ASSERT((input == nullptr) || (out != nullptr));
#else
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4946)
#endif
	auto out = reinterpret_cast<TOut>(input);
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#endif
	return out;
}

} // namespace cobalt::graphics
