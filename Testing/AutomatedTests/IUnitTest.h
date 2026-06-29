// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "TestRegistry.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <string>
#include <vector>
namespace cobalt::graphics::testing {
class ITestSession;

class IUnitTest
{
public:
	// Enumerations
	enum class Type
	{
		UnitTest,
		PerformanceTest,
	};

public:
	// Test properties
	virtual std::string TestFullName() const = 0;
	virtual Type GetType() const = 0;

	// Test methods
	virtual bool ExecuteTest(ITestSession& session) = 0;
	virtual void RequireInternal(const std::string& fileName, size_t lineNumber, const std::string& expressionAsString, bool expression) = 0;

protected:
	// Test methods
	virtual bool ExecuteTestInternal(ITestSession& session) = 0;
};

// Macro definition for simplifying the definition and registration of a unit test
#define DEFINE_UNIT_TEST_WITH_BASE(TEST_PATH, FRAMEWORK)          \
	namespace {                                                   \
	struct TestRegisterer : FRAMEWORK                             \
	{                                                             \
		TestRegisterer()                                          \
		: FRAMEWORK(TEST_PATH)                                    \
		{                                                         \
			TestRegistry::AddTestToRegistry(this);                \
		}                                                         \
		bool ExecuteTestInternal(ITestSession& session) override; \
	};                                                            \
	}                                                             \
	TestRegisterer registerA;                                     \
	bool TestRegisterer::ExecuteTestInternal(ITestSession& session)

// Macro definitions for minimal traditional unit test functionality
#define REQUIRE(...) RequireInternal(__FILE__, (size_t)__LINE__, #__VA_ARGS__, static_cast<bool>(!!(__VA_ARGS__)))                          // NOLINT(readability-simplify-boolean-expr)
#define REQUIRE_EXTERNAL(object, ...) object->RequireInternal(__FILE__, (size_t)__LINE__, #__VA_ARGS__, static_cast<bool>(!!(__VA_ARGS__))) // NOLINT(readability-simplify-boolean-expr)

} // namespace cobalt::graphics::testing
