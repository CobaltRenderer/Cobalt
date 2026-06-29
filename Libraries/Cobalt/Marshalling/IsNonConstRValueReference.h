// Copyright (c) 2013 Roger Sanders
// Licensed under the MIT License
#pragma once
#include "MarshalPreprocessorMacros.h"
#ifdef MARSHALSUPPORT_CPP11SUPPORTED
#include <type_traits>
#endif
namespace cobalt { namespace marshalling { namespace internal {

template<class T>
struct is_non_const_rvalue_reference
{
public:
	// Result
#ifdef MARSHALSUPPORT_CPP11SUPPORTED
	static const bool value = std::is_rvalue_reference<T>::value && !std::is_const<T>::value;
#else
	static const bool value = false;
#endif
};

}}} // namespace cobalt::marshalling::internal
