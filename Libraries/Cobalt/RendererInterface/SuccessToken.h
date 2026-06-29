// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include <cstdint>
namespace cobalt { namespace graphics {

class SuccessToken
{
public:
	// Constructors
	inline SuccessToken(bool success); // NOLINT(hicpp-explicit-conversions)
	inline SuccessToken(SuccessToken&& source);
	inline SuccessToken(const SuccessToken& source);
	inline ~SuccessToken();

	// Conversion operators
	inline operator bool() const; // NOLINT(hicpp-explicit-conversions)

	// Result methods
	inline bool Succeeded() const;
	inline bool Failed() const;
	inline void IgnoreResult();

private:
	// Result methods
	inline bool GetResultInternal() const;
	inline bool IsResultObserved() const;
	inline void SetResultObserved() const;

private:
	mutable uint8_t _result;
};

}} // namespace cobalt::graphics
#include "SuccessToken.inl"
