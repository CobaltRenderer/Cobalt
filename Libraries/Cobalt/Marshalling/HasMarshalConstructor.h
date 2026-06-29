// Copyright (c) 2013 Roger Sanders
// Licensed under the MIT License
#pragma once
#include "MarshalObjectTag.h"
#include "MarshalPreprocessorMacros.h"
#ifdef MARSHALSUPPORT_CPP11SUPPORTED
#include <type_traits>
#else
#include "IMarshallingObject.h"
#include "IsBaseOfEmulator.h"
#endif
namespace cobalt { namespace marshalling { namespace internal {

template<class T>
struct has_marshal_constructor
{
public:
	// Result
#ifdef MARSHALSUPPORT_CPP11SUPPORTED
	static const bool value = std::is_constructible<T, marshal_object_tag, const T&>::value;
#else
	static const bool value = is_base_of_emulator<IMarshallingObject, T>::value;
#endif
};

}}} // namespace cobalt::marshalling::internal
