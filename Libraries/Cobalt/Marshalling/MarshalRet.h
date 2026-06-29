// Copyright (c) 2013 Roger Sanders
// Licensed under the MIT License
#pragma once
#include "GetLastNestedContainerElementType.h"
#include "IsAssignable.h"
#include "IsLastNestedContainerElement.h"
#include "IsOnlyMovable.h"
#include "IsThisOrNestedElementLastNestedContainerElement.h"
#include "MarshalPreprocessorMacros.h"
#include "MarshalSource.h"
#ifdef MARSHALSUPPORT_CPP11SUPPORTED
#include <memory>
#include <utility>
#endif
namespace cobalt { namespace marshalling { namespace Marshal {

// Forward declarations
template<class, bool, bool, bool, bool>
class In;
template<class, bool, bool>
class InOut;

// Disable warning about the presence of virtual functions without a virtual destructor. The class is internal, and is
// never derived from. We only use virtual functions to create code barriers between assemblies, so a virtual destructor
// is not required.
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4265)
#endif
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdelete-non-virtual-dtor"
#pragma clang diagnostic ignored "-Wnon-virtual-dtor"
#endif
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdelete-non-virtual-dtor"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#endif

namespace internal {
//----------------------------------------------------------------------------------------
// Note that logically this type should be a private member of our Ret class, but if it was defined in this way, its
// total visibility would be limited to this class definition, and as such the compiler would be fully aware that no
// types could be derived from it, theoretically allowing the compiler to optimize away virtual function calls, with
// knowledge that they couldn't be overridden by any other type. We could define this class as a protected member
// instead, to ensure that the compiler cannot know that there are no derived types, while still keeping it private to
// users of the Ret class. To make specialization of the Ret class easier however, we define it as an external type
// here.
template<class ContainerType, bool IsOnlyMovable = marshalling::internal::is_only_movable<typename marshalling::internal::get_last_nested_container_element_type<ContainerType>::type>::value>
class RetData
{
public:
	// Constructors
	inline explicit RetData(const ContainerType& source)
	: _sourceObject(source), _source(_sourceObject), _refCount(1)
	{}
#ifdef MARSHALSUPPORT_CPP11SUPPORTED
	inline explicit RetData(ContainerType&& source)
	: _sourceObject(std::move(source)), _source(std::move(_sourceObject)), _refCount(1)
	{}
#endif

	// Reference count methods
	virtual void MARSHALSUPPORT_CALLINGCONVENTION IncrementRefCount()
	{
		// Note that this method doesn't strictly need to be virtual, but it is better to make it virtual so that the
		// owning Ret object doesn't have to know anything about the layout of this object. This would allow us to make
		// changes to the structure of this type without breaking compatibility with existing compiled code.
		++_refCount;
	}
	virtual void MARSHALSUPPORT_CALLINGCONVENTION DecrementRefCount()
	{
		// Note that this method must be virtual, so that the assembly which allocated this object will be the one in
		// which the deletion is performed.
		--_refCount;
		if (_refCount <= 0)
		{
			delete this;
		}
	}

	// Marshal methods
	virtual const IMarshalSource<ContainerType>& MARSHALSUPPORT_CALLINGCONVENTION GetMarshaller() const
	{
		// Note that this method needs to be virtual, so that the calling method doesn't know the real type of the
		// marshaller object, and therefore can't optimize away any virtual function boundaries. Note that we could
		// store a reference to the marshaller object instead and make this method non-virtual, but it is better to make
		// it virtual so that the caller doesn't have to know anything about the layout of this object, as per the notes
		// in IncrementRefCount.
		return _source;
	}

private:
	// Disable copying and moving
	RetData(const RetData& source) MARSHALSUPPORT_DELETEMETHOD;            // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
	RetData& operator=(const RetData& source) MARSHALSUPPORT_DELETEMETHOD; // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
#ifdef MARSHALSUPPORT_CPP11SUPPORTED
	RetData(RetData&& source) MARSHALSUPPORT_DELETEMETHOD;            // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
	RetData& operator=(RetData&& source) MARSHALSUPPORT_DELETEMETHOD; // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
#endif

private:
	// Note that these data members MUST be listed in this order. We need the _sourceObject member to be initialized
	// before the _source member within the constructor initializer list.
	ContainerType _sourceObject;
	MarshalSource<ContainerType> _source;
	unsigned int _refCount;
};

#ifdef MARSHALSUPPORT_CPP11SUPPORTED
//----------------------------------------------------------------------------------------
template<class ContainerType>
class RetData<ContainerType, true>
{
public:
	// Constructors
	inline explicit RetData(ContainerType&& source)
	: _sourceObject(std::move(source)), _source(std::move(_sourceObject)), _refCount(1)
	{}

	// Reference count methods
	virtual void MARSHALSUPPORT_CALLINGCONVENTION IncrementRefCount()
	{
		++_refCount;
	}
	virtual void MARSHALSUPPORT_CALLINGCONVENTION DecrementRefCount()
	{
		--_refCount;
		if (_refCount <= 0)
		{
			delete this;
		}
	}

	// Marshal methods
	virtual const IMarshalSource<ContainerType>& MARSHALSUPPORT_CALLINGCONVENTION GetMarshaller() const
	{
		return _source;
	}

private:
	// Disable copying and moving
	RetData(const RetData& source) MARSHALSUPPORT_DELETEMETHOD;            // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
	RetData& operator=(const RetData& source) MARSHALSUPPORT_DELETEMETHOD; // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
	RetData(RetData&& source) MARSHALSUPPORT_DELETEMETHOD;                 // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
	RetData& operator=(RetData&& source) MARSHALSUPPORT_DELETEMETHOD;      // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)

private:
	// Note that these data members MUST be listed in this order. We need the _sourceObject member to be initialized
	// before the _source member within the constructor initializer list.
	ContainerType _sourceObject;
	MarshalSource<ContainerType> _source;
	unsigned int _refCount;
};
#endif
} // namespace internal

//----------------------------------------------------------------------------------------
template<class ContainerType, bool IsOnlyMovable = marshalling::internal::is_only_movable<typename marshalling::internal::get_last_nested_container_element_type<ContainerType>::type>::value>
class Ret
{
public:
	// Friend declarations
	template<class, bool, bool, bool, bool>
	friend class In;
	template<class, bool, bool>
	friend class InOut;

public:
	// Constructors
	inline Ret(const ContainerType& source) // NOLINT(hicpp-explicit-conversions)
	: _data(new internal::RetData<ContainerType>(source))
	{}
	inline Ret(const In<ContainerType, marshalling::internal::is_only_movable<typename marshalling::internal::get_last_nested_container_element_type<ContainerType>::type>::value, marshalling::internal::is_assignable<ContainerType>::value, marshalling::internal::is_last_nested_container_element<ContainerType>::value, marshalling::internal::is_this_or_nested_element_last_nested_container_element<ContainerType>::value>& source) // NOLINT(hicpp-explicit-conversions)
	: _data(new internal::RetData<ContainerType>(source))
	{}
	inline Ret(const InOut<ContainerType, marshalling::internal::is_last_nested_container_element<ContainerType>::value, marshalling::internal::is_this_or_nested_element_last_nested_container_element<ContainerType>::value>& source) // NOLINT(hicpp-explicit-conversions)
	: _data(new internal::RetData<ContainerType>(source))
	{}
	inline Ret(const Ret& source)
	: _data(source._data)
	{
		_data->IncrementRefCount();
	}
#ifdef MARSHALSUPPORT_CPP11SUPPORTED
	inline Ret(ContainerType&& source) // NOLINT(hicpp-explicit-conversions)
	: _data(new internal::RetData<ContainerType>(std::move(source)))
	{}
	inline Ret(Ret&& source)
	: _data(source._data)
	{
		source._data = 0;
	}
#endif
	inline ~Ret()
	{
		if (_data != 0)
		{
			_data->DecrementRefCount();
		}
	}

	// Marshal methods
	inline ContainerType Get() const
	{
		return _data->GetMarshaller().MarshalTo();
	}
	inline void Get(ContainerType& targetObject) const
	{
		_data->GetMarshaller().MarshalTo(targetObject);
	}
	inline ContainerType GetWithoutMove() const
	{
		return _data->GetMarshaller().MarshalToWithoutMove();
	}
	inline void GetWithoutMove(ContainerType& targetObject) const
	{
		_data->GetMarshaller().MarshalToWithoutMove(targetObject);
	}

	// Implicit conversions
	inline operator ContainerType() const // NOLINT(hicpp-explicit-conversions)
	{
		return _data->GetMarshaller().MarshalTo();
	}

private:
	internal::RetData<ContainerType>* _data;
};

//----------------------------------------------------------------------------------------
#ifdef MARSHALSUPPORT_CPP11SUPPORTED
template<class ContainerType>
class Ret<ContainerType, true>
{
public:
	// Friend declarations
	template<class, bool, bool, bool, bool>
	friend class In;

public:
	// Constructors
	inline Ret(const In<ContainerType, marshalling::internal::is_only_movable<typename marshalling::internal::get_last_nested_container_element_type<ContainerType>::type>::value, marshalling::internal::is_assignable<ContainerType>::value, marshalling::internal::is_last_nested_container_element<ContainerType>::value, marshalling::internal::is_this_or_nested_element_last_nested_container_element<ContainerType>::value>& source) // NOLINT(hicpp-explicit-conversions)
	: _data(new internal::RetData<ContainerType>(source))
	{}
	inline Ret(ContainerType&& source) // NOLINT(hicpp-explicit-conversions)
	: _data(new internal::RetData<ContainerType>(std::move(source)))
	{}
	inline Ret(Ret&& source)
	: _data(source._data)
	{
		source._data = nullptr;
	}
	inline ~Ret()
	{
		if (_data != nullptr)
		{
			_data->DecrementRefCount();
		}
	}

	// Marshal methods
	inline ContainerType Get() const
	{
		return _data->GetMarshaller().MarshalTo();
	}
	inline void Get(ContainerType& targetObject) const
	{
		_data->GetMarshaller().MarshalTo(targetObject);
	}

	// Implicit conversions
	inline operator ContainerType() const // NOLINT(hicpp-explicit-conversions)
	{
		return _data->GetMarshaller().MarshalTo();
	}

private:
	internal::RetData<ContainerType>* _data;
};
#endif

//----------------------------------------------------------------------------------------
template<class ElementType, class traits, class Alloc>
class Ret<std::basic_string<ElementType, traits, Alloc>, false>
{
public:
	// Friend declarations
	template<class, bool, bool, bool, bool>
	friend class In;
	template<class, bool, bool>
	friend class InOut;

public:
	// Constructors
	inline Ret(const std::basic_string<ElementType, traits, Alloc>& source) // NOLINT(hicpp-explicit-conversions)
	: _data(new internal::RetData<std::basic_string<ElementType, traits, Alloc>>(source))
	{}
	inline Ret(const In<std::basic_string<ElementType, traits, Alloc>, marshalling::internal::is_only_movable<typename marshalling::internal::get_last_nested_container_element_type<std::basic_string<ElementType, traits, Alloc>>::type>::value, marshalling::internal::is_assignable<std::basic_string<ElementType, traits, Alloc>>::value, marshalling::internal::is_last_nested_container_element<std::basic_string<ElementType, traits, Alloc>>::value, marshalling::internal::is_this_or_nested_element_last_nested_container_element<std::basic_string<ElementType, traits, Alloc>>::value>& source) // NOLINT(hicpp-explicit-conversions)
	: _data(new internal::RetData<std::basic_string<ElementType, traits, Alloc>>(source))
	{}
	inline Ret(const InOut<std::basic_string<ElementType, traits, Alloc>, marshalling::internal::is_last_nested_container_element<std::basic_string<ElementType, traits, Alloc>>::value, marshalling::internal::is_this_or_nested_element_last_nested_container_element<std::basic_string<ElementType, traits, Alloc>>::value>& source) // NOLINT(hicpp-explicit-conversions)
	: _data(new internal::RetData<std::basic_string<ElementType, traits, Alloc>>(source))
	{}
	inline Ret(const ElementType* source) // NOLINT(hicpp-explicit-conversions)
	: _data(new internal::RetData<std::basic_string<ElementType, traits, Alloc>>(source))
	{}
	inline Ret(const Ret& source)
	: _data(source._data)
	{
		_data->IncrementRefCount();
	}
#ifdef MARSHALSUPPORT_CPP11SUPPORTED
	inline Ret(std::basic_string<ElementType, traits, Alloc>&& source) // NOLINT(hicpp-explicit-conversions)
	: _data(new internal::RetData<std::basic_string<ElementType, traits, Alloc>>(std::move(source)))
	{}
	inline Ret(Ret&& source)
	: _data(source._data)
	{
		source._data = 0;
	}
#endif
	inline ~Ret()
	{
		if (_data != 0)
		{
			_data->DecrementRefCount();
		}
	}

	// Marshal methods
	inline std::basic_string<ElementType, traits, Alloc> Get() const
	{
		return _data->GetMarshaller().MarshalTo();
	}
	inline void Get(std::basic_string<ElementType, traits, Alloc>& targetObject) const
	{
		_data->GetMarshaller().MarshalTo(targetObject);
	}
	inline std::basic_string<ElementType, traits, Alloc> GetWithoutMove() const
	{
		return _data->GetMarshaller().MarshalToWithoutMove();
	}
	inline void GetWithoutMove(std::basic_string<ElementType, traits, Alloc>& targetObject) const
	{
		_data->GetMarshaller().MarshalToWithoutMove(targetObject);
	}

	// Implicit conversions
	inline operator std::basic_string<ElementType, traits, Alloc>() const // NOLINT(hicpp-explicit-conversions)
	{
		return _data->GetMarshaller().MarshalTo();
	}

private:
	internal::RetData<std::basic_string<ElementType, traits, Alloc>>* _data;
};

//----------------------------------------------------------------------------------------
#ifdef MARSHALSUPPORT_CPP11SUPPORTED
template<class T, class Deleter>
class Ret<std::unique_ptr<T, Deleter>, false>
{
public:
	// Constructors
	inline Ret(std::unique_ptr<T, Deleter>&& source) // NOLINT(hicpp-explicit-conversions)
	: _deleter(source.get_deleter())
	{
		_data = source.release();
	}
	inline Ret(T* source) // NOLINT(hicpp-explicit-conversions)
	{
		_data = source;
	}
	inline Ret(Ret&& source)
	: _data(source._data), _deleter(std::move(source._deleter))
	{
		source._data = nullptr;
	}
	inline ~Ret()
	{
		if (_data != nullptr)
		{
			_deleter(_data);
		}
	}

	// Marshal methods
	inline std::unique_ptr<T, Deleter> Get() const
	{
		T* data = _data;
		_data = nullptr;
		return std::unique_ptr<T, Deleter>(data, _deleter);
	}

	// Implicit conversions
	inline operator std::unique_ptr<T, Deleter>() const // NOLINT(hicpp-explicit-conversions)
	{
		return Get();
	}

private:
	mutable T* _data;
	Deleter _deleter;
};
#endif

// Restore the disabled warnings
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#ifdef __clang__
#pragma clang diagnostic pop
#endif
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

}}} // namespace cobalt::marshalling::Marshal
#include "MarshalRetStringOperators.inl"
