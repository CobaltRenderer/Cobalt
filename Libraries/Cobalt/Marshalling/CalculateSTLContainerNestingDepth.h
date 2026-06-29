// Copyright (c) 2013 Roger Sanders
// Licensed under the MIT License
#pragma once
#include "MarshalPreprocessorMacros.h"
#include <deque>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <stack>
#include <string>
#include <utility>
#include <vector>
#ifdef MARSHALSUPPORT_CPP11SUPPORTED
#include <array>
#include <forward_list>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#endif
namespace cobalt { namespace marshalling { namespace internal {

template<class ElementType, class Alloc>
inline MARSHALSUPPORT_CONSTEXPR size_t CalculateSTLContainerNestingDepth(size_t currentNestingDepth, const std::vector<ElementType, Alloc>* elementPointer);
#ifdef MARSHALSUPPORT_CPP11SUPPORTED
template<class ElementType, size_t ArraySize>
inline MARSHALSUPPORT_CONSTEXPR size_t CalculateSTLContainerNestingDepth(size_t currentNestingDepth, const std::array<ElementType, ArraySize>* elementPointer);
#endif
template<class ElementType, class Alloc>
inline MARSHALSUPPORT_CONSTEXPR size_t CalculateSTLContainerNestingDepth(size_t currentNestingDepth, const std::list<ElementType, Alloc>* elementPointer);
#ifdef MARSHALSUPPORT_CPP11SUPPORTED
template<class ElementType, class Alloc>
inline MARSHALSUPPORT_CONSTEXPR size_t CalculateSTLContainerNestingDepth(size_t currentNestingDepth, const std::forward_list<ElementType, Alloc>* elementPointer);
#endif
template<class ElementType, class Alloc>
inline MARSHALSUPPORT_CONSTEXPR size_t CalculateSTLContainerNestingDepth(size_t currentNestingDepth, const std::deque<ElementType, Alloc>* elementPointer);
template<class ElementType, class Compare, class Alloc>
inline MARSHALSUPPORT_CONSTEXPR size_t CalculateSTLContainerNestingDepth(size_t currentNestingDepth, const std::set<ElementType, Compare, Alloc>* elementPointer);
template<class ElementType, class Compare, class Alloc>
inline MARSHALSUPPORT_CONSTEXPR size_t CalculateSTLContainerNestingDepth(size_t currentNestingDepth, const std::multiset<ElementType, Compare, Alloc>* elementPointer);
#ifdef MARSHALSUPPORT_CPP11SUPPORTED
template<class ElementType, class Hash, class Pred, class Alloc>
inline MARSHALSUPPORT_CONSTEXPR size_t CalculateSTLContainerNestingDepth(size_t currentNestingDepth, const std::unordered_set<ElementType, Hash, Pred, Alloc>* elementPointer);
template<class ElementType, class Hash, class Pred, class Alloc>
inline MARSHALSUPPORT_CONSTEXPR size_t CalculateSTLContainerNestingDepth(size_t currentNestingDepth, const std::unordered_multiset<ElementType, Hash, Pred, Alloc>* elementPointer);
#endif
template<class KeyType, class ElementType, class Compare, class Alloc>
inline MARSHALSUPPORT_CONSTEXPR size_t CalculateSTLContainerNestingDepth(size_t currentNestingDepth, const std::map<KeyType, ElementType, Compare, Alloc>* elementPointer);
template<class KeyType, class ElementType, class Compare, class Alloc>
inline MARSHALSUPPORT_CONSTEXPR size_t CalculateSTLContainerNestingDepth(size_t currentNestingDepth, const std::multimap<KeyType, ElementType, Compare, Alloc>* elementPointer);
#ifdef MARSHALSUPPORT_CPP11SUPPORTED
template<class KeyType, class ElementType, class Hash, class Pred, class Alloc>
inline MARSHALSUPPORT_CONSTEXPR size_t CalculateSTLContainerNestingDepth(size_t currentNestingDepth, const std::unordered_map<KeyType, ElementType, Hash, Pred, Alloc>* elementPointer);
template<class KeyType, class ElementType, class Hash, class Pred, class Alloc>
inline MARSHALSUPPORT_CONSTEXPR size_t CalculateSTLContainerNestingDepth(size_t currentNestingDepth, const std::unordered_multimap<KeyType, ElementType, Hash, Pred, Alloc>* elementPointer);
#endif
template<class ElementType, class Container>
inline MARSHALSUPPORT_CONSTEXPR size_t CalculateSTLContainerNestingDepth(size_t currentNestingDepth, const std::stack<ElementType, Container>* elementPointer);
template<class ElementType, class Container>
inline MARSHALSUPPORT_CONSTEXPR size_t CalculateSTLContainerNestingDepth(size_t currentNestingDepth, const std::queue<ElementType, Container>* elementPointer);
template<class ElementType, class Container, class Compare>
inline MARSHALSUPPORT_CONSTEXPR size_t CalculateSTLContainerNestingDepth(size_t currentNestingDepth, const std::priority_queue<ElementType, Container, Compare>* elementPointer);
template<class ElementType, class traits, class Alloc>
inline MARSHALSUPPORT_CONSTEXPR size_t CalculateSTLContainerNestingDepth(size_t currentNestingDepth, const std::basic_string<ElementType, traits, Alloc>* elementPointer);
template<class T1, class T2>
inline MARSHALSUPPORT_CONSTEXPR size_t CalculateSTLContainerNestingDepth(size_t currentNestingDepth, const std::pair<T1, T2>* elementPointer);
#if defined(MARSHALSUPPORT_CPP11SUPPORTED) && !defined(MARSHALSUPPORT_NOVARIADICTEMPLATES)
template<class... Args>
inline MARSHALSUPPORT_CONSTEXPR size_t CalculateSTLContainerNestingDepth(size_t currentNestingDepth, const std::tuple<Args...>* elementPointer);
#endif
template<class ElementType>
inline MARSHALSUPPORT_CONSTEXPR size_t CalculateSTLContainerNestingDepth(size_t currentNestingDepth, const ElementType* elementPointer);

}}} // namespace cobalt::marshalling::internal
#include "CalculateSTLContainerNestingDepth.inl"
