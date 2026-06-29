// Copyright (c) 2013 Roger Sanders
// Licensed under the MIT License
#pragma once
#include "IsLastNestedContainerElement.h"
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

template<class T>
struct is_this_or_nested_element_last_nested_container_element
{
	static const bool value = true;
};
template<class ElementType, class Alloc>
struct is_this_or_nested_element_last_nested_container_element<std::vector<ElementType, Alloc>>
{
	static const bool value = is_last_nested_container_element<ElementType>::value;
};
#ifdef MARSHALSUPPORT_CPP11SUPPORTED
template<class ElementType, size_t ArraySize>
struct is_this_or_nested_element_last_nested_container_element<std::array<ElementType, ArraySize>>
{
	static const bool value = is_last_nested_container_element<ElementType>::value;
};
#endif
template<class ElementType, class Alloc>
struct is_this_or_nested_element_last_nested_container_element<std::list<ElementType, Alloc>>
{
	static const bool value = is_last_nested_container_element<ElementType>::value;
};
#ifdef MARSHALSUPPORT_CPP11SUPPORTED
template<class ElementType, class Alloc>
struct is_this_or_nested_element_last_nested_container_element<std::forward_list<ElementType, Alloc>>
{
	static const bool value = is_last_nested_container_element<ElementType>::value;
};
#endif
template<class ElementType, class Alloc>
struct is_this_or_nested_element_last_nested_container_element<std::deque<ElementType, Alloc>>
{
	static const bool value = is_last_nested_container_element<ElementType>::value;
};
template<class ElementType, class Compare, class Alloc>
struct is_this_or_nested_element_last_nested_container_element<std::set<ElementType, Compare, Alloc>>
{
	static const bool value = is_last_nested_container_element<ElementType>::value;
};
template<class ElementType, class Compare, class Alloc>
struct is_this_or_nested_element_last_nested_container_element<std::multiset<ElementType, Compare, Alloc>>
{
	static const bool value = is_last_nested_container_element<ElementType>::value;
};
#ifdef MARSHALSUPPORT_CPP11SUPPORTED
template<class ElementType, class Hash, class Pred, class Alloc>
struct is_this_or_nested_element_last_nested_container_element<std::unordered_set<ElementType, Hash, Pred, Alloc>>
{
	static const bool value = is_last_nested_container_element<ElementType>::value;
};
template<class ElementType, class Hash, class Pred, class Alloc>
struct is_this_or_nested_element_last_nested_container_element<std::unordered_multiset<ElementType, Hash, Pred, Alloc>>
{
	static const bool value = is_last_nested_container_element<ElementType>::value;
};
#endif
template<class KeyType, class ElementType, class Compare, class Alloc>
struct is_this_or_nested_element_last_nested_container_element<std::map<KeyType, ElementType, Compare, Alloc>>
{
	static const bool value = is_last_nested_container_element<ElementType>::value;
};
template<class KeyType, class ElementType, class Compare, class Alloc>
struct is_this_or_nested_element_last_nested_container_element<std::multimap<KeyType, ElementType, Compare, Alloc>>
{
	static const bool value = is_last_nested_container_element<ElementType>::value;
};
#ifdef MARSHALSUPPORT_CPP11SUPPORTED
template<class KeyType, class ElementType, class Hash, class Pred, class Alloc>
struct is_this_or_nested_element_last_nested_container_element<std::unordered_map<KeyType, ElementType, Hash, Pred, Alloc>>
{
	static const bool value = is_last_nested_container_element<ElementType>::value;
};
template<class KeyType, class ElementType, class Hash, class Pred, class Alloc>
struct is_this_or_nested_element_last_nested_container_element<std::unordered_multimap<KeyType, ElementType, Hash, Pred, Alloc>>
{
	static const bool value = is_last_nested_container_element<ElementType>::value;
};
#endif
template<class ElementType, class Container>
struct is_this_or_nested_element_last_nested_container_element<std::stack<ElementType, Container>>
{
	static const bool value = is_last_nested_container_element<ElementType>::value;
};
template<class ElementType, class Container>
struct is_this_or_nested_element_last_nested_container_element<std::queue<ElementType, Container>>
{
	static const bool value = is_last_nested_container_element<ElementType>::value;
};
template<class ElementType, class Container, class Compare>
struct is_this_or_nested_element_last_nested_container_element<std::priority_queue<ElementType, Container, Compare>>
{
	static const bool value = is_last_nested_container_element<ElementType>::value;
};
template<class ElementType, class traits, class Alloc>
struct is_this_or_nested_element_last_nested_container_element<std::basic_string<ElementType, traits, Alloc>>
{
	static const bool value = is_last_nested_container_element<ElementType>::value;
};

}}} // namespace cobalt::marshalling::internal
