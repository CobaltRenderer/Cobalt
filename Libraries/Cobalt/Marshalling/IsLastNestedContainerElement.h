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
#include <memory>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#endif
namespace cobalt { namespace marshalling { namespace internal {

template<class T>
struct is_last_nested_container_element
{
private:
	// Typedefs
	typedef char (&yes)[1];
	typedef char (&no)[2];

private:
	// Check function
	template<class ElementType, class Alloc>
	static no check(std::vector<ElementType, Alloc>*);
#ifdef MARSHALSUPPORT_CPP11SUPPORTED
	template<class ElementType, size_t ArraySize>
	static no check(std::array<ElementType, ArraySize>*);
#endif
	template<class ElementType, class Alloc>
	static no check(std::list<ElementType, Alloc>*);
#ifdef MARSHALSUPPORT_CPP11SUPPORTED
	template<class ElementType, class Alloc>
	static no check(std::forward_list<ElementType, Alloc>*);
#endif
	template<class ElementType, class Alloc>
	static no check(std::deque<ElementType, Alloc>*);
	template<class ElementType, class Compare, class Alloc>
	static no check(std::set<ElementType, Compare, Alloc>*);
	template<class ElementType, class Compare, class Alloc>
	static no check(std::multiset<ElementType, Compare, Alloc>*);
#ifdef MARSHALSUPPORT_CPP11SUPPORTED
	template<class ElementType, class Hash, class Pred, class Alloc>
	static no check(std::unordered_set<ElementType, Hash, Pred, Alloc>*);
	template<class ElementType, class Hash, class Pred, class Alloc>
	static no check(std::unordered_multiset<ElementType, Hash, Pred, Alloc>*);
#endif
	template<class KeyType, class ElementType, class Compare, class Alloc>
	static no check(std::map<KeyType, ElementType, Compare, Alloc>*);
	template<class KeyType, class ElementType, class Compare, class Alloc>
	static no check(std::multimap<KeyType, ElementType, Compare, Alloc>*);
#ifdef MARSHALSUPPORT_CPP11SUPPORTED
	template<class KeyType, class ElementType, class Hash, class Pred, class Alloc>
	static no check(std::unordered_map<KeyType, ElementType, Hash, Pred, Alloc>*);
	template<class KeyType, class ElementType, class Hash, class Pred, class Alloc>
	static no check(std::unordered_multimap<KeyType, ElementType, Hash, Pred, Alloc>*);
#endif
	template<class ElementType, class Container>
	static no check(std::stack<ElementType, Container>*);
	template<class ElementType, class Container>
	static no check(std::queue<ElementType, Container>*);
	template<class ElementType, class Container, class Compare>
	static no check(std::priority_queue<ElementType, Container, Compare>*);
	template<class ElementType, class traits, class Alloc>
	static no check(std::basic_string<ElementType, traits, Alloc>*);
	template<class T1, class T2>
	static no check(std::pair<T1, T2>*);
#if defined(MARSHALSUPPORT_CPP11SUPPORTED) && !defined(MARSHALSUPPORT_NOVARIADICTEMPLATES)
	template<class... Args>
	static no check(std::tuple<Args...>*);
#endif
#ifdef MARSHALSUPPORT_CPP11SUPPORTED
	template<class ElementType, class Deleter>
	static no check(std::unique_ptr<ElementType, Deleter>*);
#endif
	template<class ElementType>
	static yes check(ElementType*);

public:
	// Result
	static const bool value = (sizeof(check((T*)0)) == sizeof(yes));
};

}}} // namespace cobalt::marshalling::internal
