// Copyright (c) 2013 Roger Sanders
// Licensed under the MIT License
#pragma once
#include "CalculateDecomposedSTLContainerSize.h"
#include "CalculateSTLContainerNestingDepth.h"
#include "CreateSTLContainerItemArray.h"
#include "CreateSTLContainerKeyMarshallers.h"
#include "DecomposeSTLContainer.h"
#include "DeleteSTLContainerItemArray.h"
#include "DeleteSTLContainerKeyMarshallers.h"
#include "GetLastNestedContainerElementType.h"
#include "IMarshalSource.h"
#include "IsLastNestedContainerElement.h"
#include "IsOnlyMovable.h"
#include "IsThisOrNestedElementLastNestedContainerElement.h"
#include "MarshalPreprocessorMacros.h"
#include "NestedContainerHasKeys.h"
#include "NestedMarshaller.h"
#include "NoBoundObjectTag.h"
namespace cobalt { namespace marshalling {

#ifdef _MSC_VER
#pragma warning(push)
// Disable warning about using "this" pointer in initializer list. Our usage here is safe, and guaranteed by the
// standard. See section 12.6.2 [class.base.init] paragraph 7 of the standard for clarification.
#pragma warning(disable : 4355)
// Disable warning about our use of type traits causing conditional expressions to be constant. The code behaves as
// intended, and the compiler is free to optimize away the dead branch.
#pragma warning(disable : 4127)
// Disable warning about classes with virtual functions without a virtual destructor. We use protected inlined
// destructors where appropriate to prevent destruction through base class pointers.
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

//-----------------------------------------------------------------------------
namespace internal {
template<class ContainerType, bool IsThisOrNextElementLastElement = marshalling::internal::is_this_or_nested_element_last_nested_container_element<ContainerType>::value, bool IsOnlyMovable = marshalling::internal::is_only_movable<typename marshalling::internal::get_last_nested_container_element_type<ContainerType>::type>::value>
class MarshalSourceInternal : public IMarshalSource<ContainerType, false, IsThisOrNextElementLastElement, IsOnlyMovable>
{
public:
	// Constructors
	explicit MarshalSourceInternal(const ContainerType& sourceObject)
	: _sourceObject(&sourceObject), _sourceDataMovable(false)
	{}
#ifdef MARSHALSUPPORT_CPP11SUPPORTED
	explicit MarshalSourceInternal(ContainerType&& sourceObject)
	: _sourceObject(&sourceObject), _sourceDataMovable(true)
	{}
#endif
	explicit MarshalSourceInternal(no_bound_object_tag*)
	{}

protected:
	// Marshalling methods
	virtual void MARSHALSUPPORT_CALLINGCONVENTION RetrieveData(size_t& elementByteSize, void*& itemArray, const size_t*& elementSizeArray, marshalling::internal::INestedMarshallerBase* const*& nestedMarshallerArray, bool allowMove) const
	{
		// Retrieve the byte size of the innermost element to be contained in our flat arrays
		elementByteSize = sizeof(typename marshalling::internal::get_last_nested_container_element_type<ContainerType>::type);

		// Calculate the required size of our flat arrays in order to hold a decomposed version of the specified object
		size_t itemArraySize = 0;
		size_t elementSizeArraySize = 0;
		marshalling::internal::CalculateDecomposedSTLContainerSize(itemArraySize, elementSizeArraySize, *_sourceObject);

		// Allocate our flat arrays to hold the decomposed object data
		itemArray = marshalling::internal::CreateSTLContainerItemArray(itemArraySize, _sourceObject);
		size_t* elementSizeArrayTemp = new size_t[elementSizeArraySize];
		elementSizeArray = elementSizeArrayTemp;

		// Allocate key marshallers for any keyed containers in the _source object
		if (marshalling::internal::nested_container_has_keys<ContainerType>::value)
		{
			MARSHALSUPPORT_CONSTEXPR size_t nestingDepth = marshalling::internal::CalculateSTLContainerNestingDepth(0, (ContainerType*)0);
			marshalling::internal::INestedMarshallerBase** nestedMarshallerArrayTemp = new marshalling::internal::INestedMarshallerBase*[nestingDepth];
			marshalling::internal::CreateSTLContainerKeyMarshallers(nestedMarshallerArrayTemp, 0, (ContainerType*)0);
			nestedMarshallerArray = nestedMarshallerArrayTemp;
		}

		// Decompose the container hierarchy into a flat array of the innermost elements, with a corresponding array of
		// sizes for the intermediate containers.
		size_t elementArrayIndex = 0;
		size_t elementSizeArrayIndex = 0;
		size_t nestedMarshallerArrayIndex = 0;
#ifdef MARSHALSUPPORT_CPP11SUPPORTED
		if (_sourceDataMovable && allowMove)
		{
			marshalling::internal::DecomposeSTLContainer(itemArray, elementSizeArrayTemp, nestedMarshallerArray, elementArrayIndex, elementSizeArrayIndex, nestedMarshallerArrayIndex, std::move(const_cast<ContainerType&>(*_sourceObject)));
		}
		else
#endif
		{
			marshalling::internal::DecomposeSTLContainer(itemArray, elementSizeArrayTemp, nestedMarshallerArray, elementArrayIndex, elementSizeArrayIndex, nestedMarshallerArrayIndex, *_sourceObject);
		}
	}
	virtual void MARSHALSUPPORT_CALLINGCONVENTION FreeDataArrays(void* itemArray, const size_t* elementSizeArray, marshalling::internal::INestedMarshallerBase* const* nestedMarshallerArray) const
	{
		// Release the allocated data arrays
		marshalling::internal::DeleteSTLContainerItemArray(itemArray, _sourceObject);
		delete[] elementSizeArray;
		if (marshalling::internal::nested_container_has_keys<ContainerType>::value)
		{
			marshalling::internal::DeleteSTLContainerKeyMarshallers(const_cast<marshalling::internal::INestedMarshallerBase**>(nestedMarshallerArray), 0, (ContainerType*)0);
			delete[] nestedMarshallerArray;
		}
	}

private:
	// Disable copying and moving
	MarshalSourceInternal(const MarshalSourceInternal& source) MARSHALSUPPORT_DELETEMETHOD;            // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
	MarshalSourceInternal& operator=(const MarshalSourceInternal& source) MARSHALSUPPORT_DELETEMETHOD; // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
#ifdef MARSHALSUPPORT_CPP11SUPPORTED
	MarshalSourceInternal(MarshalSourceInternal&& source) MARSHALSUPPORT_DELETEMETHOD;            // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
	MarshalSourceInternal& operator=(MarshalSourceInternal&& source) MARSHALSUPPORT_DELETEMETHOD; // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
#endif

private:
	const ContainerType* _sourceObject;
	bool _sourceDataMovable;
};

#ifdef MARSHALSUPPORT_CPP11SUPPORTED
//-----------------------------------------------------------------------------
template<class ContainerType, bool IsThisOrNextElementLastElement>
class MarshalSourceInternal<ContainerType, IsThisOrNextElementLastElement, true> : public IMarshalSource<ContainerType, false, IsThisOrNextElementLastElement, true>
{
public:
	// Constructors
	explicit MarshalSourceInternal(ContainerType&& sourceObject)
	: _sourceObject(&sourceObject)
	{}
	explicit MarshalSourceInternal(no_bound_object_tag*)
	{}

protected:
	// Marshalling methods
	virtual void MARSHALSUPPORT_CALLINGCONVENTION RetrieveData(size_t& elementByteSize, void*& itemArray, const size_t*& elementSizeArray, marshalling::internal::INestedMarshallerBase* const*& nestedMarshallerArray) const
	{
		// Retrieve the byte size of the innermost element to be contained in our flat arrays
		elementByteSize = sizeof(typename marshalling::internal::get_last_nested_container_element_type<ContainerType>::type);

		// Calculate the required size of our flat arrays in order to hold a decomposed version of the specified object
		size_t itemArraySize = 0;
		size_t elementSizeArraySize = 0;
		marshalling::internal::CalculateDecomposedSTLContainerSize(itemArraySize, elementSizeArraySize, *_sourceObject);

		// Allocate our flat arrays to hold the decomposed object data
		itemArray = marshalling::internal::CreateSTLContainerItemArray(itemArraySize, _sourceObject);
		size_t* elementSizeArrayTemp = new size_t[elementSizeArraySize];
		elementSizeArray = elementSizeArrayTemp;

		// Allocate key marshallers for any keyed containers in the _source object
		if (marshalling::internal::nested_container_has_keys<ContainerType>::value)
		{
			MARSHALSUPPORT_CONSTEXPR size_t nestingDepth = marshalling::internal::CalculateSTLContainerNestingDepth(0, (ContainerType*)0);
			marshalling::internal::INestedMarshallerBase** nestedMarshallerArrayTemp = new marshalling::internal::INestedMarshallerBase*[nestingDepth];
			marshalling::internal::CreateSTLContainerKeyMarshallers(nestedMarshallerArrayTemp, 0, (ContainerType*)0);
			nestedMarshallerArray = nestedMarshallerArrayTemp;
		}

		// Decompose the container hierarchy into a flat array of the innermost elements, with a corresponding array of
		// sizes for the intermediate containers.
		size_t elementArrayIndex = 0;
		size_t elementSizeArrayIndex = 0;
		size_t nestedMarshallerArrayIndex = 0;
		marshalling::internal::DecomposeSTLContainer(itemArray, elementSizeArrayTemp, nestedMarshallerArray, elementArrayIndex, elementSizeArrayIndex, nestedMarshallerArrayIndex, std::move(*_sourceObject));
	}
	virtual void MARSHALSUPPORT_CALLINGCONVENTION FreeDataArrays(void* itemArray, const size_t* elementSizeArray, marshalling::internal::INestedMarshallerBase* const* nestedMarshallerArray) const
	{
		// Release the allocated data arrays
		marshalling::internal::DeleteSTLContainerItemArray(itemArray, _sourceObject);
		delete[] elementSizeArray;
		if (marshalling::internal::nested_container_has_keys<ContainerType>::value)
		{
			marshalling::internal::DeleteSTLContainerKeyMarshallers(const_cast<marshalling::internal::INestedMarshallerBase**>(nestedMarshallerArray), 0, (ContainerType*)0);
			delete[] nestedMarshallerArray;
		}
	}

private:
	// Disable copying and moving
	MarshalSourceInternal(const MarshalSourceInternal& source) MARSHALSUPPORT_DELETEMETHOD;            // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
	MarshalSourceInternal& operator=(const MarshalSourceInternal& source) MARSHALSUPPORT_DELETEMETHOD; // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
	MarshalSourceInternal(MarshalSourceInternal&& source) MARSHALSUPPORT_DELETEMETHOD;                 // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
	MarshalSourceInternal& operator=(MarshalSourceInternal&& source) MARSHALSUPPORT_DELETEMETHOD;      // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)

private:
	ContainerType* _sourceObject;
};
#endif
} // namespace internal

//-----------------------------------------------------------------------------
template<class ContainerType, bool IsLastElement = marshalling::internal::is_last_nested_container_element<ContainerType>::value, bool IsThisOrNextElementLastElement = marshalling::internal::is_this_or_nested_element_last_nested_container_element<ContainerType>::value, bool IsOnlyMovable = marshalling::internal::is_only_movable<typename marshalling::internal::get_last_nested_container_element_type<ContainerType>::type>::value>
class MarshalSource
{};

//-----------------------------------------------------------------------------
template<class ContainerType, bool IsThisOrNextElementLastElement>
class MarshalSource<ContainerType, false, IsThisOrNextElementLastElement, false> : public marshalling::internal::MarshalSourceInternal<ContainerType, IsThisOrNextElementLastElement, false>
{
public:
	// Constructors
	explicit MarshalSource(const ContainerType& asourceObject)
	: marshalling::internal::MarshalSourceInternal<ContainerType, IsThisOrNextElementLastElement, false>(asourceObject)
	{}
#ifdef MARSHALSUPPORT_CPP11SUPPORTED
	explicit MarshalSource(ContainerType&& asourceObject)
	: marshalling::internal::MarshalSourceInternal<ContainerType, IsThisOrNextElementLastElement, false>(std::move(asourceObject))
	{}
#endif
	explicit MarshalSource(no_bound_object_tag* tag)
	: marshalling::internal::MarshalSourceInternal<ContainerType, IsThisOrNextElementLastElement, false>(tag)
	{}

private:
	// Disable copying and moving
	MarshalSource(const MarshalSource& source) MARSHALSUPPORT_DELETEMETHOD;            // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
	MarshalSource& operator=(const MarshalSource& source) MARSHALSUPPORT_DELETEMETHOD; // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
#ifdef MARSHALSUPPORT_CPP11SUPPORTED
	MarshalSource(MarshalSource&& source) MARSHALSUPPORT_DELETEMETHOD;            // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
	MarshalSource& operator=(MarshalSource&& source) MARSHALSUPPORT_DELETEMETHOD; // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
#endif
};

#ifdef MARSHALSUPPORT_CPP11SUPPORTED
//-----------------------------------------------------------------------------
template<class ContainerType, bool IsThisOrNextElementLastElement>
class MarshalSource<ContainerType, false, IsThisOrNextElementLastElement, true> : public marshalling::internal::MarshalSourceInternal<ContainerType, IsThisOrNextElementLastElement, true>
{
public:
	// Constructors
	explicit MarshalSource(const ContainerType& asourceObject)
	: marshalling::internal::MarshalSourceInternal<ContainerType, IsThisOrNextElementLastElement, true>(asourceObject)
	{}
	explicit MarshalSource(ContainerType&& asourceObject)
	: marshalling::internal::MarshalSourceInternal<ContainerType, IsThisOrNextElementLastElement, true>(std::move(asourceObject))
	{}
	explicit MarshalSource(no_bound_object_tag* tag)
	: marshalling::internal::MarshalSourceInternal<ContainerType, IsThisOrNextElementLastElement, true>(tag)
	{}

private:
	// Disable copying and moving
	MarshalSource(const MarshalSource& source) MARSHALSUPPORT_DELETEMETHOD;            // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
	MarshalSource& operator=(const MarshalSource& source) MARSHALSUPPORT_DELETEMETHOD; // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
	MarshalSource(MarshalSource&& source) MARSHALSUPPORT_DELETEMETHOD;                 // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
	MarshalSource& operator=(MarshalSource&& source) MARSHALSUPPORT_DELETEMETHOD;      // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
};
#endif

//-----------------------------------------------------------------------------
template<class ContainerType>
class MarshalSource<ContainerType, true, true, false> : public IMarshalSource<ContainerType, true, true, false>
{
public:
	// Constructors
	explicit MarshalSource(const ContainerType& sourceObject)
	: _sourceObject(&sourceObject), _sourceDataMovable(false)
	{}
#ifdef MARSHALSUPPORT_CPP11SUPPORTED
	explicit MarshalSource(ContainerType&& sourceObject)
	: _sourceObject(&sourceObject), _sourceDataMovable(true)
	{}
#endif
	explicit MarshalSource(no_bound_object_tag*)
	{}

protected:
	// Marshalling methods
	virtual bool MARSHALSUPPORT_CALLINGCONVENTION RetrieveData(size_t& elementByteSize, const ContainerType*& sourceData) const
	{
		elementByteSize = sizeof(typename marshalling::internal::get_last_nested_container_element_type<ContainerType>::type);
		sourceData = _sourceObject;
		return _sourceDataMovable;
	}

private:
	// Disable copying and moving
	MarshalSource(const MarshalSource& source) MARSHALSUPPORT_DELETEMETHOD;            // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
	MarshalSource& operator=(const MarshalSource& source) MARSHALSUPPORT_DELETEMETHOD; // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
#ifdef MARSHALSUPPORT_CPP11SUPPORTED
	MarshalSource(MarshalSource&& source) MARSHALSUPPORT_DELETEMETHOD;            // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
	MarshalSource& operator=(MarshalSource&& source) MARSHALSUPPORT_DELETEMETHOD; // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
#endif

private:
	const ContainerType* _sourceObject;
	bool _sourceDataMovable;
};

#ifdef MARSHALSUPPORT_CPP11SUPPORTED
//-----------------------------------------------------------------------------
template<class ContainerType>
class MarshalSource<ContainerType, true, true, true> : public IMarshalSource<ContainerType, true, true, true>
{
public:
	// Constructors
	explicit MarshalSource(ContainerType&& sourceObject)
	: _sourceObject(&sourceObject)
	{}
	explicit MarshalSource(no_bound_object_tag*)
	{}

protected:
	// Marshalling methods
	virtual void MARSHALSUPPORT_CALLINGCONVENTION RetrieveData(size_t& elementByteSize, ContainerType*& sourceData) const
	{
		elementByteSize = sizeof(typename marshalling::internal::get_last_nested_container_element_type<ContainerType>::type);
		sourceData = _sourceObject;
	}

private:
	// Disable copying and moving
	MarshalSource(const MarshalSource& source) MARSHALSUPPORT_DELETEMETHOD;            // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
	MarshalSource& operator=(const MarshalSource& source) MARSHALSUPPORT_DELETEMETHOD; // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
	MarshalSource(MarshalSource&& source) MARSHALSUPPORT_DELETEMETHOD;                 // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
	MarshalSource& operator=(MarshalSource&& source) MARSHALSUPPORT_DELETEMETHOD;      // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)

private:
	ContainerType* _sourceObject;
};
#endif

//-----------------------------------------------------------------------------
template<class ElementType, class Alloc>
class MarshalSource<std::vector<ElementType, Alloc>, false, true, false> : public IMarshalSource<std::vector<ElementType, Alloc>, false, true, false>
{
public:
	// Constructors
	explicit MarshalSource(const std::vector<ElementType, Alloc>& sourceObject)
	: _sourceObject(&sourceObject), _sourceDataMovable(false)
	{}
#ifdef MARSHALSUPPORT_CPP11SUPPORTED
	explicit MarshalSource(std::vector<ElementType, Alloc>&& sourceObject)
	: _sourceObject(&sourceObject), _sourceDataMovable(true)
	{}
#endif
	explicit MarshalSource(no_bound_object_tag*)
	{}

protected:
	// Marshalling methods
	virtual bool MARSHALSUPPORT_CALLINGCONVENTION RetrieveData(size_t& elementByteSize, const ElementType*& sourceData, size_t& sourceDataLength) const
	{
		elementByteSize = sizeof(typename marshalling::internal::get_last_nested_container_element_type<ElementType>::type);
#ifdef MARSHALSUPPORT_CPP11SUPPORTED
		sourceData = _sourceObject->data();
#else
		sourceData = _sourceObject->empty() ? 0 : &(*_sourceObject)[0];
#endif
		sourceDataLength = _sourceObject->size();
		return _sourceDataMovable;
	}

	// Size methods
	virtual size_t MARSHALSUPPORT_CALLINGCONVENTION GetSize() const
	{
		return static_cast<size_t>(_sourceObject->size());
	}
	virtual size_t MARSHALSUPPORT_CALLINGCONVENTION GetReservedSize() const
	{
		return static_cast<size_t>(_sourceObject->capacity());
	}

private:
	// Disable copying and moving
	MarshalSource(const MarshalSource& source) MARSHALSUPPORT_DELETEMETHOD;            // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
	MarshalSource& operator=(const MarshalSource& source) MARSHALSUPPORT_DELETEMETHOD; // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
#ifdef MARSHALSUPPORT_CPP11SUPPORTED
	MarshalSource(MarshalSource&& source) MARSHALSUPPORT_DELETEMETHOD;            // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
	MarshalSource& operator=(MarshalSource&& source) MARSHALSUPPORT_DELETEMETHOD; // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
#endif

private:
	const std::vector<ElementType, Alloc>* _sourceObject;
	bool _sourceDataMovable;
};

#ifdef MARSHALSUPPORT_CPP11SUPPORTED
//-----------------------------------------------------------------------------
template<class ElementType, class Alloc>
class MarshalSource<std::vector<ElementType, Alloc>, false, true, true> : public IMarshalSource<std::vector<ElementType, Alloc>, false, true, true>
{
public:
	// Constructors
	explicit MarshalSource(std::vector<ElementType, Alloc>&& sourceObject)
	: _sourceObject(&sourceObject)
	{}
	explicit MarshalSource(no_bound_object_tag*)
	{}

protected:
	// Marshalling methods
	virtual void MARSHALSUPPORT_CALLINGCONVENTION RetrieveData(size_t& elementByteSize, ElementType*& sourceData, size_t& sourceDataLength) const
	{
		elementByteSize = sizeof(typename marshalling::internal::get_last_nested_container_element_type<ElementType>::type);
		sourceData = _sourceObject->data();
		sourceDataLength = _sourceObject->size();
	}

	// Size methods
	virtual size_t MARSHALSUPPORT_CALLINGCONVENTION GetSize() const
	{
		return static_cast<size_t>(_sourceObject->size());
	}
	virtual size_t MARSHALSUPPORT_CALLINGCONVENTION GetReservedSize() const
	{
		return static_cast<size_t>(_sourceObject->capacity());
	}

private:
	// Disable copying and moving
	MarshalSource(const MarshalSource& source) MARSHALSUPPORT_DELETEMETHOD;            // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
	MarshalSource& operator=(const MarshalSource& source) MARSHALSUPPORT_DELETEMETHOD; // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
	MarshalSource(MarshalSource&& source) MARSHALSUPPORT_DELETEMETHOD;                 // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
	MarshalSource& operator=(MarshalSource&& source) MARSHALSUPPORT_DELETEMETHOD;      // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)

private:
	std::vector<ElementType, Alloc>* _sourceObject;
};
#endif

//-----------------------------------------------------------------------------
template<class Alloc>
class MarshalSource<std::vector<bool, Alloc>, false, true, false> : public IMarshalSource<std::vector<bool, Alloc>, false, true, false>
{
public:
	// Constructors
	explicit MarshalSource(const std::vector<bool, Alloc>& sourceObject)
	: _sourceObject(&sourceObject)
	{}
	explicit MarshalSource(no_bound_object_tag*)
	{}

protected:
	// Marshalling methods
	virtual void MARSHALSUPPORT_CALLINGCONVENTION RetrieveData(const bool*& sourceData, size_t& sourceDataLength) const
	{
		// Since the C++ standard defines a rather poor specialization of std::vector for the bool type which is
		// intended to allow multiple boolean values to be packed into shared integer values values, and therefore can't
		// provide a reference to its underlying data members like the vector container normally can, we need to repack
		// the elements of this boolean vector here into a flat array so that we can marshal it.
		size_t sourceObjectLength = _sourceObject->size();
		bool* itemArray = new bool[sourceObjectLength];
		for (size_t i = 0; i < sourceObjectLength; ++i)
		{
			itemArray[i] = (*_sourceObject)[(typename std::vector<bool, Alloc>::size_type)i];
		}

		// Return the raw vector data to the caller
		sourceData = itemArray;
		sourceDataLength = sourceObjectLength;
	}
	virtual void MARSHALSUPPORT_CALLINGCONVENTION FreeDataArrays(const bool* sourceData) const
	{
		delete[] sourceData;
	}
	virtual void MARSHALSUPPORT_CALLINGCONVENTION RetrieveElement(size_t elementIndex, bool& element) const
	{
		element = (*_sourceObject)[(typename std::vector<bool, Alloc>::size_type)elementIndex];
	}

	// Size methods
	virtual size_t MARSHALSUPPORT_CALLINGCONVENTION GetSize() const
	{
		return static_cast<size_t>(_sourceObject->size());
	}
	virtual size_t MARSHALSUPPORT_CALLINGCONVENTION GetReservedSize() const
	{
		return static_cast<size_t>(_sourceObject->capacity());
	}

private:
	// Disable copying and moving
	MarshalSource(const MarshalSource& source) MARSHALSUPPORT_DELETEMETHOD;            // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
	MarshalSource& operator=(const MarshalSource& source) MARSHALSUPPORT_DELETEMETHOD; // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
#ifdef MARSHALSUPPORT_CPP11SUPPORTED
	MarshalSource(MarshalSource&& source) MARSHALSUPPORT_DELETEMETHOD;            // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
	MarshalSource& operator=(MarshalSource&& source) MARSHALSUPPORT_DELETEMETHOD; // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
#endif

private:
	const std::vector<bool, Alloc>* _sourceObject;
};

#ifdef MARSHALSUPPORT_CPP11SUPPORTED
//-----------------------------------------------------------------------------
template<class ElementType, size_t ArraySize>
class MarshalSource<std::array<ElementType, ArraySize>, false, true, false> : public IMarshalSource<std::array<ElementType, ArraySize>, false, true, false>
{
public:
	// Constructors
	explicit MarshalSource(const std::array<ElementType, ArraySize>& sourceObject)
	: _sourceObject(&sourceObject), _sourceDataMovable(false)
	{}
	explicit MarshalSource(std::array<ElementType, ArraySize>&& sourceObject)
	: _sourceObject(&sourceObject), _sourceDataMovable(true)
	{}
	explicit MarshalSource(no_bound_object_tag*)
	{}

protected:
	// Marshalling methods
	virtual bool MARSHALSUPPORT_CALLINGCONVENTION RetrieveData(size_t& elementByteSize, const ElementType*& sourceData) const
	{
		elementByteSize = sizeof(typename marshalling::internal::get_last_nested_container_element_type<ElementType>::type);
		sourceData = _sourceObject->data();
		return _sourceDataMovable;
	}

private:
	// Disable copying and moving
	MarshalSource(const MarshalSource& source) MARSHALSUPPORT_DELETEMETHOD;            // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
	MarshalSource& operator=(const MarshalSource& source) MARSHALSUPPORT_DELETEMETHOD; // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
	MarshalSource(MarshalSource&& source) MARSHALSUPPORT_DELETEMETHOD;                 // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
	MarshalSource& operator=(MarshalSource&& source) MARSHALSUPPORT_DELETEMETHOD;      // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)

private:
	const std::array<ElementType, ArraySize>* _sourceObject;
	bool _sourceDataMovable;
};

//-----------------------------------------------------------------------------
template<class ElementType, size_t ArraySize>
class MarshalSource<std::array<ElementType, ArraySize>, false, true, true> : public IMarshalSource<std::array<ElementType, ArraySize>, false, true, true>
{
public:
	// Constructors
	explicit MarshalSource(std::array<ElementType, ArraySize>&& sourceObject)
	: _sourceObject(&sourceObject)
	{}
	explicit MarshalSource(no_bound_object_tag*)
	{}

protected:
	// Marshalling methods
	virtual void MARSHALSUPPORT_CALLINGCONVENTION RetrieveData(size_t& elementByteSize, ElementType*& sourceData) const
	{
		elementByteSize = sizeof(typename marshalling::internal::get_last_nested_container_element_type<ElementType>::type);
		sourceData = _sourceObject->data();
	}

private:
	// Disable copying and moving
	MarshalSource(const MarshalSource& source) MARSHALSUPPORT_DELETEMETHOD;            // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
	MarshalSource& operator=(const MarshalSource& source) MARSHALSUPPORT_DELETEMETHOD; // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
	MarshalSource(MarshalSource&& source) MARSHALSUPPORT_DELETEMETHOD;                 // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
	MarshalSource& operator=(MarshalSource&& source) MARSHALSUPPORT_DELETEMETHOD;      // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)

private:
	std::array<ElementType, ArraySize>* _sourceObject;
};
#endif

//-----------------------------------------------------------------------------
template<class ElementType, class traits, class Alloc>
class MarshalSource<std::basic_string<ElementType, traits, Alloc>, false, true, false> : public IMarshalSource<std::basic_string<ElementType, traits, Alloc>, false, true, false>
{
public:
	// Constructors
	explicit MarshalSource(const std::basic_string<ElementType, traits, Alloc>& sourceObject)
	: _sourceObject(&sourceObject), _sourceDataMovable(false)
	{}
	explicit MarshalSource(const ElementType* sourceObject)
	: _tempString(sourceObject), _sourceObject(&_tempString), _sourceDataMovable(false)
	{}
#ifdef MARSHALSUPPORT_CPP11SUPPORTED
	explicit MarshalSource(std::basic_string<ElementType, traits, Alloc>&& sourceObject)
	: _sourceObject(&sourceObject), _sourceDataMovable(true)
	{}
#endif
	explicit MarshalSource(no_bound_object_tag*)
	{}

protected:
	// Marshalling methods
	virtual bool MARSHALSUPPORT_CALLINGCONVENTION RetrieveData(size_t& elementByteSize, const ElementType*& sourceData, size_t& sourceDataLength) const
	{
		elementByteSize = sizeof(typename marshalling::internal::get_last_nested_container_element_type<ElementType>::type);
		sourceData = _sourceObject->data();
		sourceDataLength = _sourceObject->size();
		return _sourceDataMovable;
	}

private:
	// Disable copying and moving
	MarshalSource(const MarshalSource& source) MARSHALSUPPORT_DELETEMETHOD;            // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
	MarshalSource& operator=(const MarshalSource& source) MARSHALSUPPORT_DELETEMETHOD; // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
#ifdef MARSHALSUPPORT_CPP11SUPPORTED
	MarshalSource(MarshalSource&& source) MARSHALSUPPORT_DELETEMETHOD;            // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
	MarshalSource& operator=(MarshalSource&& source) MARSHALSUPPORT_DELETEMETHOD; // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
#endif

private:
	std::basic_string<ElementType, traits, Alloc> _tempString;
	const std::basic_string<ElementType, traits, Alloc>* _sourceObject;
	bool _sourceDataMovable;
};

//-----------------------------------------------------------------------------
template<class T1, class T2>
class MarshalSource<std::pair<T1, T2>, false, true, false> : public IMarshalSource<std::pair<T1, T2>, false, true, false>
{
public:
	// Constructors
	explicit MarshalSource(const std::pair<T1, T2>& sourceObject)
	: _sourceObject(&sourceObject), _sourceDataMovable(false)
	{}
#ifdef MARSHALSUPPORT_CPP11SUPPORTED
	explicit MarshalSource(std::pair<T1, T2>&& sourceObject)
	: _sourceObject(&sourceObject), _sourceDataMovable(true)
	{}
#endif
	explicit MarshalSource(no_bound_object_tag*)
	{}

protected:
	// Marshalling methods
	virtual Marshal::Ret<T1, false> MARSHALSUPPORT_CALLINGCONVENTION RetrieveFirstElement(bool allowMove) const
	{
#ifdef MARSHALSUPPORT_CPP11SUPPORTED
		if (_sourceDataMovable && allowMove)
		{
			return std::move(const_cast<T1&>(_sourceObject->first));
		}
#endif
		return _sourceObject->first;
	}
	virtual Marshal::Ret<T2, false> MARSHALSUPPORT_CALLINGCONVENTION RetrieveSecondElement(bool allowMove) const
	{
#ifdef MARSHALSUPPORT_CPP11SUPPORTED
		if (_sourceDataMovable && allowMove)
		{
			return std::move(const_cast<T2&>(_sourceObject->second));
		}
#endif
		return _sourceObject->second;
	}

private:
	// Disable copying and moving
	MarshalSource(const MarshalSource& source) MARSHALSUPPORT_DELETEMETHOD;            // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
	MarshalSource& operator=(const MarshalSource& source) MARSHALSUPPORT_DELETEMETHOD; // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
#ifdef MARSHALSUPPORT_CPP11SUPPORTED
	MarshalSource(MarshalSource&& source) MARSHALSUPPORT_DELETEMETHOD;            // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
	MarshalSource& operator=(MarshalSource&& source) MARSHALSUPPORT_DELETEMETHOD; // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
#endif

private:
	const std::pair<T1, T2>* _sourceObject;
	bool _sourceDataMovable;
};

//-----------------------------------------------------------------------------
#ifdef MARSHALSUPPORT_CPP11SUPPORTED
template<class T1, class T2>
class MarshalSource<std::pair<T1, T2>, false, true, true> : public IMarshalSource<std::pair<T1, T2>, false, true, true>
{
public:
	// Constructors
	explicit MarshalSource(std::pair<T1, T2>&& sourceObject)
	: _sourceObject(&sourceObject)
	{}
	explicit MarshalSource(no_bound_object_tag*)
	{}

protected:
	// Marshalling methods
	virtual Marshal::Ret<T1, true> MARSHALSUPPORT_CALLINGCONVENTION RetrieveFirstElement() const
	{
		return std::move(_sourceObject->first);
	}
	virtual Marshal::Ret<T2, true> MARSHALSUPPORT_CALLINGCONVENTION RetrieveSecondElement() const
	{
		return std::move(_sourceObject->second);
	}

private:
	// Disable copying and moving
	MarshalSource(const MarshalSource& source) MARSHALSUPPORT_DELETEMETHOD;            // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
	MarshalSource& operator=(const MarshalSource& source) MARSHALSUPPORT_DELETEMETHOD; // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
	MarshalSource(MarshalSource&& source) MARSHALSUPPORT_DELETEMETHOD;                 // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
	MarshalSource& operator=(MarshalSource&& source) MARSHALSUPPORT_DELETEMETHOD;      // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)

private:
	std::pair<T1, T2>* _sourceObject;
};
#endif

#if defined(MARSHALSUPPORT_CPP11SUPPORTED) && !defined(MARSHALSUPPORT_NOVARIADICTEMPLATES)
//-----------------------------------------------------------------------------
template<>
class MarshalSource<std::tuple<>, false, true, false> : public IMarshalSource<std::tuple<>, false, true, false>
{
public:
	// Constructors
	explicit MarshalSource(const std::tuple<>& sourceObject)
	{}
	explicit MarshalSource(std::tuple<>&& sourceObject)
	{}
	explicit MarshalSource(no_bound_object_tag*)
	{}

private:
	// Disable copying and moving
	MarshalSource(const MarshalSource& source) MARSHALSUPPORT_DELETEMETHOD;            // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
	MarshalSource& operator=(const MarshalSource& source) MARSHALSUPPORT_DELETEMETHOD; // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
	MarshalSource(MarshalSource&& source) MARSHALSUPPORT_DELETEMETHOD;                 // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
	MarshalSource& operator=(MarshalSource&& source) MARSHALSUPPORT_DELETEMETHOD;      // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
};

//-----------------------------------------------------------------------------
template<class... Args>
class MarshalSource<std::tuple<Args...>, false, true, false> : public IMarshalSource<std::tuple<Args...>, false, true, false>
{
public:
	// Constructors
	explicit MarshalSource(const std::tuple<Args...>& sourceObject)
	: _sourceObject(&sourceObject), _sourceDataMovable(false)
	{}
	explicit MarshalSource(std::tuple<Args...>&& sourceObject)
	: _sourceObject(&sourceObject), _sourceDataMovable(true)
	{}
	explicit MarshalSource(no_bound_object_tag*)
	{}

protected:
	// Marshalling methods
	virtual void MARSHALSUPPORT_CALLINGCONVENTION CreateElementMarshallers(marshalling::internal::INestedMarshallerBase* (&elementMarshallers)[std::tuple_size<std::tuple<Args...>>::value], bool allowMove) const
	{
		if (_sourceDataMovable && allowMove)
		{
			CreateElementMarshallersHelper<true, std::tuple_size<std::tuple<Args...>>::value - 1, Args...>::CreateElementMarshallers(elementMarshallers, const_cast<std::tuple<Args...>&>(*_sourceObject));
		}
		else
		{
			CreateElementMarshallersHelper<false, std::tuple_size<std::tuple<Args...>>::value - 1, Args...>::CreateElementMarshallers(elementMarshallers, *_sourceObject);
		}
	}
	virtual void MARSHALSUPPORT_CALLINGCONVENTION DeleteElementMarshallers(marshalling::internal::INestedMarshallerBase* (&elementMarshallers)[std::tuple_size<std::tuple<Args...>>::value]) const
	{
		for (size_t i = 0; i < std::tuple_size<std::tuple<Args...>>::value; ++i)
		{
			delete elementMarshallers[i];
		}
	}

private:
	// Marshalling methods
	template<bool performMove, size_t TupleElementNo, class... Args2>
	class CreateElementMarshallersHelper
	{};
	template<size_t TupleElementNo, class... Args2>
	class CreateElementMarshallersHelper<true, TupleElementNo, Args2...>
	{
	public:
		static void CreateElementMarshallers(marshalling::internal::INestedMarshallerBase* (&elementMarshallers)[std::tuple_size<std::tuple<Args...>>::value], std::tuple<Args...>& sourceObject)
		{
			elementMarshallers[TupleElementNo] = (new marshalling::internal::NestedMarshaller<typename std::tuple_element<TupleElementNo, std::tuple<Args...>>::type>())->AddKeyWithReturn(std::get<TupleElementNo>(std::move(sourceObject)));
			CreateElementMarshallersHelper<true, TupleElementNo - 1, Args2...>::CreateElementMarshallers(elementMarshallers, sourceObject);
		}
	};
	template<class... Args2>
	class CreateElementMarshallersHelper<true, 0, Args2...>
	{
	public:
		static void CreateElementMarshallers(marshalling::internal::INestedMarshallerBase* (&elementMarshallers)[std::tuple_size<std::tuple<Args...>>::value], std::tuple<Args...>& sourceObject)
		{
			elementMarshallers[0] = (new marshalling::internal::NestedMarshaller<typename std::tuple_element<0, std::tuple<Args...>>::type>())->AddKeyWithReturn(std::get<0>(std::move(sourceObject)));
		}
	};
	template<size_t TupleElementNo, class... Args2>
	class CreateElementMarshallersHelper<false, TupleElementNo, Args2...>
	{
	public:
		static void CreateElementMarshallers(marshalling::internal::INestedMarshallerBase* (&elementMarshallers)[std::tuple_size<std::tuple<Args...>>::value], const std::tuple<Args...>& sourceObject)
		{
			elementMarshallers[TupleElementNo] = (new marshalling::internal::NestedMarshaller<typename std::tuple_element<TupleElementNo, std::tuple<Args...>>::type>())->AddKeyWithReturn(std::get<TupleElementNo>(sourceObject));
			CreateElementMarshallersHelper<false, TupleElementNo - 1, Args2...>::CreateElementMarshallers(elementMarshallers, sourceObject);
		}
	};
	template<class... Args2>
	class CreateElementMarshallersHelper<false, 0, Args2...>
	{
	public:
		static void CreateElementMarshallers(marshalling::internal::INestedMarshallerBase* (&elementMarshallers)[std::tuple_size<std::tuple<Args...>>::value], const std::tuple<Args...>& sourceObject)
		{
			elementMarshallers[0] = (new marshalling::internal::NestedMarshaller<typename std::tuple_element<0, std::tuple<Args...>>::type>())->AddKeyWithReturn(std::get<0>(sourceObject));
		}
	};

	// Disable copying and moving
	MarshalSource(const MarshalSource& source) MARSHALSUPPORT_DELETEMETHOD;            // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
	MarshalSource& operator=(const MarshalSource& source) MARSHALSUPPORT_DELETEMETHOD; // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
	MarshalSource(MarshalSource&& source) MARSHALSUPPORT_DELETEMETHOD;                 // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
	MarshalSource& operator=(MarshalSource&& source) MARSHALSUPPORT_DELETEMETHOD;      // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)

private:
	const std::tuple<Args...>* _sourceObject;
	bool _sourceDataMovable;
};

//-----------------------------------------------------------------------------
template<class... Args>
class MarshalSource<std::tuple<Args...>, false, true, true> : public IMarshalSource<std::tuple<Args...>, false, true, true>
{
public:
	// Constructors
	explicit MarshalSource(std::tuple<Args...>&& sourceObject)
	: _sourceObject(&sourceObject)
	{}
	explicit MarshalSource(no_bound_object_tag*)
	{}

protected:
	// Marshalling methods
	virtual void MARSHALSUPPORT_CALLINGCONVENTION CreateElementMarshallers(marshalling::internal::INestedMarshallerBase* (&elementMarshallers)[std::tuple_size<std::tuple<Args...>>::value]) const
	{
		CreateElementMarshallersHelper<std::tuple_size<std::tuple<Args...>>::value - 1, Args...>::CreateElementMarshallers(elementMarshallers, *_sourceObject);
	}
	virtual void MARSHALSUPPORT_CALLINGCONVENTION DeleteElementMarshallers(marshalling::internal::INestedMarshallerBase* (&elementMarshallers)[std::tuple_size<std::tuple<Args...>>::value]) const
	{
		for (size_t i = 0; i < std::tuple_size<std::tuple<Args...>>::value; ++i)
		{
			delete elementMarshallers[i];
		}
	}

private:
	// Marshalling methods
	template<size_t TupleElementNo, class... Args2>
	class CreateElementMarshallersHelper
	{
	public:
		static void CreateElementMarshallers(marshalling::internal::INestedMarshallerBase* (&elementMarshallers)[std::tuple_size<std::tuple<Args...>>::value], std::tuple<Args...>& sourceObject)
		{
			elementMarshallers[TupleElementNo] = (new marshalling::internal::NestedMarshaller<typename std::tuple_element<TupleElementNo, std::tuple<Args...>>::type>())->AddKeyWithReturn(std::get<TupleElementNo>(std::move(sourceObject)));
			CreateElementMarshallersHelper<TupleElementNo - 1, Args2...>::CreateElementMarshallers(elementMarshallers, sourceObject);
		}
	};
	template<class... Args2>
	class CreateElementMarshallersHelper<0, Args2...>
	{
	public:
		static void CreateElementMarshallers(marshalling::internal::INestedMarshallerBase* (&elementMarshallers)[std::tuple_size<std::tuple<Args...>>::value], std::tuple<Args...>& sourceObject)
		{
			elementMarshallers[0] = (new marshalling::internal::NestedMarshaller<typename std::tuple_element<0, std::tuple<Args...>>::type>())->AddKeyWithReturn(std::get<0>(std::move(sourceObject)));
		}
	};

	// Disable copying and moving
	MarshalSource(const MarshalSource& source) MARSHALSUPPORT_DELETEMETHOD;            // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
	MarshalSource& operator=(const MarshalSource& source) MARSHALSUPPORT_DELETEMETHOD; // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
	MarshalSource(MarshalSource&& source) MARSHALSUPPORT_DELETEMETHOD;                 // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)
	MarshalSource& operator=(MarshalSource&& source) MARSHALSUPPORT_DELETEMETHOD;      // NOLINT(hicpp-use-equals-delete,modernize-use-equals-delete)

private:
	std::tuple<Args...>* _sourceObject;
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

}} // namespace cobalt::marshalling
