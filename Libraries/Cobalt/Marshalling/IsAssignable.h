// Copyright (c) 2013 Roger Sanders
// Licensed under the MIT License
#pragma once
#include "MarshalPreprocessorMacros.h"
#ifdef MARSHALSUPPORT_CPP11SUPPORTED
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#endif
namespace cobalt { namespace marshalling { namespace internal {

#ifdef MARSHALSUPPORT_CPP11SUPPORTED
#ifndef MARSHALSUPPORT_NOVARIADICTEMPLATES
template<class T, class... Args>
struct is_assignable
{
	static const bool value = std::integral_constant < bool, is_assignable<T>::value || is_assignable<Args...>::value > ::value;
};
template<class T>
struct is_assignable<T>
{
	static const bool value = std::integral_constant < bool, std::is_move_assignable<T>::value || std::is_copy_assignable<T>::value > ::value;
};
#else
template<class T>
struct is_assignable
{
	static const bool value = std::integral_constant < bool, std::is_move_assignable<T>::value || std::is_copy_assignable<T>::value > ::value;
};
#endif
template<class T1, class T2>
struct is_assignable<std::pair<T1, T2>>
{
	static const bool value = std::integral_constant < bool, is_assignable<T1>::value || is_assignable<T2>::value > ::value;
};
#ifndef MARSHALSUPPORT_NOVARIADICTEMPLATES
template<class... Args>
struct is_assignable<std::tuple<Args...>>
{
	static const bool value = is_assignable<Args...>::value;
};
template<>
struct is_assignable<std::tuple<>>
{
	static const bool value = true;
};
#endif
template<class T, class Deleter>
struct is_assignable<std::unique_ptr<T, Deleter>>
{
	static const bool value = true;
};
#else
template<class T>
struct is_assignable
{
	static const bool value = true;
};
#endif

}}} // namespace cobalt::marshalling::internal
