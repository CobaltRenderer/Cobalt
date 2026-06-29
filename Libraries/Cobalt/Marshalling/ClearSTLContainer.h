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
#include <vector>
#ifdef MARSHALSUPPORT_CPP11SUPPORTED
#include <array>
#include <forward_list>
#include <unordered_map>
#include <unordered_set>
#endif
namespace cobalt { namespace marshalling { namespace internal {

template<class ElementType, class Alloc>
inline void ClearSTLContainer(std::vector<ElementType, Alloc>& element);
#ifdef MARSHALSUPPORT_CPP11SUPPORTED
template<class ElementType, size_t ArraySize>
inline void ClearSTLContainer(std::array<ElementType, ArraySize>& element);
#endif
template<class ElementType, class Alloc>
inline void ClearSTLContainer(std::list<ElementType, Alloc>& element);
#ifdef MARSHALSUPPORT_CPP11SUPPORTED
template<class ElementType, class Alloc>
inline void ClearSTLContainer(std::forward_list<ElementType, Alloc>& element);
#endif
template<class ElementType, class Alloc>
inline void ClearSTLContainer(std::deque<ElementType, Alloc>& element);
template<class ElementType, class Compare, class Alloc>
inline void ClearSTLContainer(std::set<ElementType, Compare, Alloc>& element);
template<class ElementType, class Compare, class Alloc>
inline void ClearSTLContainer(std::multiset<ElementType, Compare, Alloc>& element);
#ifdef MARSHALSUPPORT_CPP11SUPPORTED
template<class ElementType, class Hash, class Pred, class Alloc>
inline void ClearSTLContainer(std::unordered_set<ElementType, Hash, Pred, Alloc>& element);
template<class ElementType, class Hash, class Pred, class Alloc>
inline void ClearSTLContainer(std::unordered_multiset<ElementType, Hash, Pred, Alloc>& element);
#endif
template<class KeyType, class ElementType, class Compare, class Alloc>
inline void ClearSTLContainer(std::map<KeyType, ElementType, Compare, Alloc>& element);
template<class KeyType, class ElementType, class Compare, class Alloc>
inline void ClearSTLContainer(std::multimap<KeyType, ElementType, Compare, Alloc>& element);
#ifdef MARSHALSUPPORT_CPP11SUPPORTED
template<class KeyType, class ElementType, class Hash, class Pred, class Alloc>
inline void ClearSTLContainer(std::unordered_map<KeyType, ElementType, Hash, Pred, Alloc>& element);
template<class KeyType, class ElementType, class Hash, class Pred, class Alloc>
inline void ClearSTLContainer(std::unordered_multimap<KeyType, ElementType, Hash, Pred, Alloc>& element);
#endif
template<class ElementType, class Container>
inline void ClearSTLContainer(std::stack<ElementType, Container>& element);
template<class ElementType, class Container>
inline void ClearSTLContainer(std::queue<ElementType, Container>& element);
template<class ElementType, class Container, class Compare>
inline void ClearSTLContainer(std::priority_queue<ElementType, Container, Compare>& element);
template<class ElementType, class traits, class Alloc>
inline void ClearSTLContainer(std::basic_string<ElementType, traits, Alloc>& element);
template<class ElementType>
inline void ClearSTLContainer(ElementType& element);

}}} // namespace cobalt::marshalling::internal
#include "ClearSTLContainer.inl"
