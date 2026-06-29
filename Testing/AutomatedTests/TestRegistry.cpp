// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "TestRegistry.h"
#include "IUnitTest.h"
#include <algorithm>
namespace cobalt::graphics::testing {

//----------------------------------------------------------------------------------------
// Test registration methods
//----------------------------------------------------------------------------------------
void TestRegistry::AddTestToRegistry(IUnitTest* test)
{
	// Add this test to the set of registered tests
	GetRegisteredTestSet().push_back(test);
}

//----------------------------------------------------------------------------------------
void TestRegistry::ValidateTestRegistry(const cobalt::logging::ILogger& log)
{
	// Pre-sort the list of tests by name to give a natural, consistent ordering.
	auto& registeredTests = GetRegisteredTestSet();
	std::sort(registeredTests.begin(), registeredTests.end(), [](IUnitTest* lhs, IUnitTest* rhs) { return lhs->TestFullName() < rhs->TestFullName(); });

	// Iterate the list of registered tests, validating each one and passing valid ones into the new cleaned set.
	std::vector<IUnitTest*> cleanedTestSet;
	std::set<std::string> acceptedTests;
	for (auto* test : registeredTests)
	{
		// Ensure the test name is valid
		auto testFullName = test->TestFullName();
		if (testFullName.empty())
		{
			log.Error("Registration was attempted for a test without any name defined.");
			continue;
		}
		if (testFullName.find('/') == std::string::npos)
		{
			log.Error("Test with name \"{0}\" isn't in a test category and was rejected.", testFullName);
			continue;
		}

		// Ensure a test with the same name hasn't already been accepted
		if (acceptedTests.find(testFullName) != acceptedTests.end())
		{
			log.Error("Test with name \"{0}\" has been defined more than once. Following test registration was rejected.", testFullName);
			continue;
		}

		// Add this test to the cleaned set of tests
		cleanedTestSet.push_back(test);
		acceptedTests.insert(testFullName);
	}

	// Replace the set of registered tests with the cleaned set
	registeredTests = cleanedTestSet;
}

//----------------------------------------------------------------------------------------
std::vector<IUnitTest*> TestRegistry::GetAllRegisteredTests()
{
	return GetRegisteredTestSet();
}

//----------------------------------------------------------------------------------------
std::vector<IUnitTest*> TestRegistry::GetAllRegisteredUnitTests()
{
	std::vector<IUnitTest*> testSet;
	for (auto* test : GetRegisteredTestSet())
	{
		if (test->GetType() == IUnitTest::Type::UnitTest)
		{
			testSet.push_back(test);
		}
	}
	return testSet;
}

//----------------------------------------------------------------------------------------
std::vector<IUnitTest*> TestRegistry::GetAllRegisteredPerformanceTests()
{
	std::vector<IUnitTest*> testSet;
	for (auto* test : GetRegisteredTestSet())
	{
		if (test->GetType() == IUnitTest::Type::PerformanceTest)
		{
			testSet.push_back(test);
		}
	}
	return testSet;
}

//----------------------------------------------------------------------------------------
std::vector<IUnitTest*>& TestRegistry::GetRegisteredTestSet()
{
	static std::vector<IUnitTest*> registeredTests;
	return registeredTests;
}

} // namespace cobalt::graphics::testing
