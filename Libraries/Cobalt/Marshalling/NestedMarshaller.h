// Copyright (c) 2013 Roger Sanders
// Licensed under the MIT License
#pragma once
#include "GetLastNestedContainerElementType.h"
#include "HasMarshalConstructor.h"
#include "INestedMarshaller.h"
#include "IsLastNestedContainerElement.h"
#include "IsOnlyMovable.h"
#include "MarshalPreprocessorMacros.h"
#include <cassert>
#include <queue>
namespace cobalt { namespace marshalling { namespace internal {

//-----------------------------------------------------------------------------
template<class KeyType, bool IsLastElement = is_last_nested_container_element<KeyType>::value, bool HasMarshalConstructor = has_marshal_constructor<KeyType>::value, bool IsOnlyMovable = marshalling::internal::is_only_movable<typename marshalling::internal::get_last_nested_container_element_type<KeyType>::type>::value>
class NestedMarshaller : public INestedMarshaller<KeyType>
{
public:
	// Key methods
	inline void AddKey(const KeyType& key)
	{
		keyList.push(key);
	}
	inline NestedMarshaller* AddKeyWithReturn(const KeyType& key)
	{
		keyList.push(key);
		return this;
	}
#ifdef MARSHALSUPPORT_CPP11SUPPORTED
	inline void AddKey(KeyType&& key)
	{
		keyList.push(std::move(key));
	}
	inline NestedMarshaller* AddKeyWithReturn(KeyType&& key)
	{
		keyList.push(std::move(key));
		return this;
	}
#endif

protected:
	// Key methods
	inline virtual Marshal::Ret<KeyType, IsOnlyMovable> MARSHALSUPPORT_CALLINGCONVENTION RemoveKeyInternal()
	{
		KeyType key(MARSHALSUPPORT_MOVE(keyList.front()));
		keyList.pop();
		return key;
	}

private:
	std::queue<KeyType> keyList;
};

#ifdef MARSHALSUPPORT_CPP11SUPPORTED
//-----------------------------------------------------------------------------
template<class KeyType, bool IsLastElement, bool HasMarshalConstructor>
class NestedMarshaller<KeyType, IsLastElement, HasMarshalConstructor, true> : public INestedMarshaller<KeyType>
{
public:
	// Key methods
	inline void AddKey(KeyType&& key)
	{
		keyList.push(std::move(key));
	}
	inline NestedMarshaller* AddKeyWithReturn(KeyType&& key)
	{
		keyList.push(std::move(key));
		return *this;
	}

protected:
	// Key methods
	inline virtual Marshal::Ret<KeyType, true> MARSHALSUPPORT_CALLINGCONVENTION RemoveKeyInternal()
	{
		KeyType key(MARSHALSUPPORT_MOVE(keyList.front()));
		keyList.pop();
		return key;
	}

private:
	std::queue<KeyType> keyList;
};
#endif

//-----------------------------------------------------------------------------
template<class KeyType>
class NestedMarshaller<KeyType, true, false, false> : public INestedMarshaller<KeyType>
{
public:
	// Key methods
	inline void AddKey(const KeyType& key)
	{
		keyList.push(key);
	}
	inline NestedMarshaller* AddKeyWithReturn(const KeyType& key)
	{
		keyList.push(key);
		return this;
	}
#ifdef MARSHALSUPPORT_CPP11SUPPORTED
	inline void AddKey(KeyType&& key)
	{
		keyList.push(std::move(key));
	}
	inline NestedMarshaller* AddKeyWithReturn(KeyType&& key)
	{
		keyList.push(std::move(key));
		return this;
	}
#endif

protected:
	// Integrity check methods
	virtual void MARSHALSUPPORT_CALLINGCONVENTION EnsureSizeMatchInternal(size_t elementByteSize) const
	{
		assert(sizeof(KeyType) == elementByteSize);
	}

	// Key methods
	inline virtual KeyType MARSHALSUPPORT_CALLINGCONVENTION RemoveKeyInternal()
	{
		KeyType key(MARSHALSUPPORT_MOVE(keyList.front()));
		keyList.pop();
		return key;
	}

private:
	std::queue<KeyType> keyList;
};

#ifdef MARSHALSUPPORT_CPP11SUPPORTED
//-----------------------------------------------------------------------------
template<class KeyType>
class NestedMarshaller<KeyType, true, false, true> : public INestedMarshaller<KeyType>
{
public:
	// Key methods
	inline void AddKey(KeyType&& key)
	{
		keyList.push(std::move(key));
	}
	inline NestedMarshaller* AddKeyWithReturn(KeyType&& key)
	{
		keyList.push(std::move(key));
		return this;
	}

protected:
	// Integrity check methods
	virtual void MARSHALSUPPORT_CALLINGCONVENTION EnsureSizeMatchInternal(size_t elementByteSize) const
	{
		assert(sizeof(KeyType) == elementByteSize);
	}

	// Key methods
	inline virtual KeyType MARSHALSUPPORT_CALLINGCONVENTION RemoveKeyInternal()
	{
		KeyType key(MARSHALSUPPORT_MOVE(keyList.front()));
		keyList.pop();
		return key;
	}

private:
	std::queue<KeyType> keyList;
};
#endif

}}} // namespace cobalt::marshalling::internal
