// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <vector>
namespace cobalt::graphics::testing {
class IUnitTest;

class TestRegistry
{
public:
	// Test registration methods
	static void AddTestToRegistry(IUnitTest* test);
	static void ValidateTestRegistry(const cobalt::logging::ILogger& log);
	static std::vector<IUnitTest*> GetAllRegisteredTests();
	static std::vector<IUnitTest*> GetAllRegisteredUnitTests();
	static std::vector<IUnitTest*> GetAllRegisteredPerformanceTests();

private:
	// Test registration methods
	static std::vector<IUnitTest*>& GetRegisteredTestSet();
};

} // namespace cobalt::graphics::testing
