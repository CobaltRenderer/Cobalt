// Copyright (c) 2013 Roger Sanders
// Licensed under the MIT License
#pragma once
#include "MarshalPreprocessorMacros.h"
namespace cobalt { namespace marshalling { namespace internal {

template<class ElementType>
inline void DeleteSTLContainerItemArray(void* itemArray, const ElementType* elementPointer);

}}} // namespace cobalt::marshalling::internal
#include "DeleteSTLContainerItemArray.inl"
