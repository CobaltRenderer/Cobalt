// Copyright (c) 2013 Roger Sanders
// Licensed under the MIT License
#pragma once
#include "MarshalPreprocessorMacros.h"
#ifdef MARSHALSUPPORT_CPP11SUPPORTED
#include "MarshalIn.h"
#include "MarshalInOut.h"
#include "MarshalOut.h"
#include "MarshalRet.h"
namespace cobalt { namespace marshalling { namespace operators { namespace Marshal {

// Define alias templates for our marshal helpers. We do this to provide a "clean" namespace where we guarantee that
// only these symbols are present, and nothing else will be imported through a "using namespace" operation other than
// these types. The intended usage is for code to include a line within header files of the following form:
// using namespace cobalt::marshalling::operators;
// The marshalling operators can then be referenced on method declarations in the form "Marshal::In", "Marshal::Out",
// and so forth.
template<class ContainerType, bool IsOnlyMovable = marshalling::internal::is_only_movable<typename marshalling::internal::get_last_nested_container_element_type<ContainerType>::type>::value, bool IsAssignable = marshalling::internal::is_assignable<ContainerType>::value, bool IsLastElement = marshalling::internal::is_last_nested_container_element<ContainerType>::value, bool IsThisOrNextElementLastElement = marshalling::internal::is_this_or_nested_element_last_nested_container_element<ContainerType>::value>
using In = marshalling::Marshal::In<ContainerType, IsOnlyMovable, IsAssignable, IsLastElement, IsThisOrNextElementLastElement>;

template<class ContainerType, bool IsOnlyMovable = marshalling::internal::is_only_movable<typename marshalling::internal::get_last_nested_container_element_type<ContainerType>::type>::value, bool IsLastElement = marshalling::internal::is_last_nested_container_element<ContainerType>::value, bool IsThisOrNextElementLastElement = marshalling::internal::is_this_or_nested_element_last_nested_container_element<ContainerType>::value>
using Out = marshalling::Marshal::Out<ContainerType, IsOnlyMovable, IsLastElement, IsThisOrNextElementLastElement>;

template<class ContainerType, bool IsLastElement = marshalling::internal::is_last_nested_container_element<ContainerType>::value, bool IsThisOrNextElementLastElement = marshalling::internal::is_this_or_nested_element_last_nested_container_element<ContainerType>::value>
using InOut = marshalling::Marshal::InOut<ContainerType, IsLastElement, IsThisOrNextElementLastElement>;

template<class ContainerType, bool IsOnlyMovable = marshalling::internal::is_only_movable<typename marshalling::internal::get_last_nested_container_element_type<ContainerType>::type>::value>
using Ret = marshalling::Marshal::Ret<ContainerType, IsOnlyMovable>;

}}}} // namespace cobalt::marshalling::operators::Marshal
#endif
