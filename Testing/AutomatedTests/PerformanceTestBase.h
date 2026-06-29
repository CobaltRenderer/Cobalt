// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "UnitTestBase.h"
namespace cobalt::graphics::testing {

class PerformanceTestBase : public UnitTestBase
{
public:
	// Constructors
	explicit PerformanceTestBase(const std::string& testFullName);

	// Test properties
	Type GetType() const override;
};

} // namespace cobalt::graphics::testing
