// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "PerformanceTestBase.h"
namespace cobalt::graphics::testing {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
PerformanceTestBase::PerformanceTestBase(const std::string& testFullName)
: UnitTestBase(testFullName)
{}

//----------------------------------------------------------------------------------------
// Test properties
//----------------------------------------------------------------------------------------
PerformanceTestBase::Type PerformanceTestBase::GetType() const
{
	return IUnitTest::Type::PerformanceTest;
}

} // namespace cobalt::graphics::testing
