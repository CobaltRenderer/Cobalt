// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "PseudoRandomGenerator.h"
namespace cobalt::graphics::testing {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
PseudoRandomGenerator::PseudoRandomGenerator(uint32_t seed)
: _seed(seed)
{}

//----------------------------------------------------------------------------------------
// Random number methods
//----------------------------------------------------------------------------------------
uint32_t PseudoRandomGenerator::GetNext()
{
	const static uint32_t a = 214013U;
	const static uint32_t c = 2531011U;
	_seed = _seed * a + c;
	return (_seed >> 16) & 0x7FFF;
}

//----------------------------------------------------------------------------------------
uint32_t PseudoRandomGenerator::GetNext(uint32_t exclusiveUpperBound)
{
	auto next = 0u;
	for (auto i = 0u; i <= exclusiveUpperBound / (GetMaxValue() + 1); ++i)
	{
		next *= (GetMaxValue() + 1);
		next += GetNext();
	}
	return (next % exclusiveUpperBound);
}

//----------------------------------------------------------------------------------------
float PseudoRandomGenerator::GetNext(float min, float max)
{
	// This is known to not produce a perfectly even distribution, however that's not our main concern. We need to
	// reproduce the same sequence of numbers on all platforms, so that our tests can be validated against expected
	// results, which this is sufficient for. Note that we can't use std::uniform_real_distribution, as its output is
	// not consistent.
	auto span = max - min;
	return GetNextNormalized() * span + min;
}

//----------------------------------------------------------------------------------------
float PseudoRandomGenerator::GetNextNormalized()
{
	return (float)GetNext() / (float)GetMaxValue();
}

//----------------------------------------------------------------------------------------
uint32_t PseudoRandomGenerator::GetMaxValue() const
{
	return 0x7FFF;
}

} // namespace cobalt::graphics::testing
