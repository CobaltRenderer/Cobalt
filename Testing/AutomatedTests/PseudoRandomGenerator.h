// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include <cstdint>
namespace cobalt::graphics::testing {

class PseudoRandomGenerator
{
public:
	// Constructors
	explicit PseudoRandomGenerator(uint32_t seed = 1);

	// Random number methods
	uint32_t GetNext();
	uint32_t GetNext(uint32_t exclusiveUpperBound);
	float GetNext(float min, float max);
	float GetNextNormalized();
	uint32_t GetMaxValue() const;

private:
	uint32_t _seed;
};

} // namespace cobalt::graphics::testing
