// Copyright (c) 2013 Roger Sanders
// Licensed under the MIT License
#pragma once
namespace cobalt { namespace marshalling { namespace internal {

template<class B, class D>
struct is_base_of_emulator
{
private:
	// Typedefs
	typedef char (&yes)[1];
	typedef char (&no)[2];

	// Nested Types
	template<class B2, class D2>
	struct Host
	{
		operator B2*() const; // NOLINT(hicpp-explicit-conversions)
		operator D2*();       // NOLINT(hicpp-explicit-conversions)
	};

private:
	// Check function
	static no check(B*, int);
	template<class T>
	static yes check(D*, T);

public:
	// Result
	static const bool value = (sizeof(check(Host<B, D>(), int())) == sizeof(yes));
};

}}} // namespace cobalt::marshalling::internal
