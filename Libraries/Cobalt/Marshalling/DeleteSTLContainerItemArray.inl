// Copyright (c) 2013 Roger Sanders
// Licensed under the MIT License
namespace cobalt { namespace marshalling { namespace internal {

//-----------------------------------------------------------------------------
template<class ElementType>
void DeleteSTLContainerItemArray(void* itemArray, const ElementType* elementPointer)
{
	// Note that we allocated this memory buffer as a raw byte array, so we free it in the same form.
	unsigned char* itemArrayAsByteArray = (unsigned char*)itemArray;
	delete[] itemArrayAsByteArray;
}

}}} // namespace cobalt::marshalling::internal
