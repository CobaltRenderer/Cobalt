// Copyright (c) 2013 Roger Sanders
// Licensed under the MIT License
#pragma once
namespace cobalt { namespace marshalling { namespace internal {

class INestedMarshallerBase
{
public:
	// Constructors
	inline virtual ~INestedMarshallerBase() = 0;
};
// NOLINTNEXTLINE(hicpp-use-equals-default,modernize-use-equals-default)
INestedMarshallerBase::~INestedMarshallerBase()
{}

}}} // namespace cobalt::marshalling::internal
