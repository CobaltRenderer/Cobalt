// Copyright (c) 2013 Roger Sanders
// Licensed under the MIT License
#pragma once
#include "GetLastNestedContainerElementType.h"
#include "HasMarshalConstructor.h"
#include "INestedMarshallerBase.h"
#include "IsLastNestedContainerElement.h"
#include "IsOnlyMovable.h"
#include "MarshalPreprocessorMacros.h"
namespace cobalt { namespace marshalling {
// Forward declare our Marshal::Ret class
namespace Marshal {
template<class, bool>
class Ret;
}
namespace internal {

//-----------------------------------------------------------------------------
template<class KeyType, bool IsLastElement = is_last_nested_container_element<KeyType>::value, bool HasMarshalConstructor = has_marshal_constructor<KeyType>::value, bool IsOnlyMovable = marshalling::internal::is_only_movable<typename marshalling::internal::get_last_nested_container_element_type<KeyType>::type>::value>
class INestedMarshaller : public INestedMarshallerBase
{
public:
	// Integrity check methods
	inline void EnsureSizeMatch(size_t elementByteSize) const
	{
		// We marshal the contained value, so there's nothing to do here. Integrity checking will be done as part of the
		// marshal operation.
	}

	// Key methods
	inline KeyType RemoveKey()
	{
		return RemoveKeyInternal().Get();
	}
	inline KeyType RemoveKey(size_t elementByteSize)
	{
		return RemoveKeyInternal().Get();
	}

protected:
	// Key methods
	virtual Marshal::Ret<KeyType, IsOnlyMovable> MARSHALSUPPORT_CALLINGCONVENTION RemoveKeyInternal() = 0;
};

//-----------------------------------------------------------------------------
template<class KeyType, bool IsOnlyMovable>
class INestedMarshaller<KeyType, true, false, IsOnlyMovable> : public INestedMarshallerBase
{
public:
	// Integrity check methods
	inline void EnsureSizeMatch(size_t elementByteSize) const
	{
		EnsureSizeMatchInternal(elementByteSize);
	}

	// Key methods
	inline KeyType RemoveKey()
	{
		return RemoveKeyInternal();
	}
	inline KeyType RemoveKey(size_t elementByteSize)
	{
		EnsureSizeMatch(elementByteSize);
		return RemoveKeyInternal();
	}

protected:
	// Integrity check methods
	virtual void MARSHALSUPPORT_CALLINGCONVENTION EnsureSizeMatchInternal(size_t elementByteSize) const = 0;

	// Key methods
	virtual KeyType MARSHALSUPPORT_CALLINGCONVENTION RemoveKeyInternal() = 0;
};

} // namespace internal
}} // namespace cobalt::marshalling
