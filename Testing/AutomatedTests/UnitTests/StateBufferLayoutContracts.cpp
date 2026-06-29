// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "../UnitTestBase.h"
namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

DEFINE_UNIT_TEST_WITH_BASE("Resources/StateBuffer/StateBufferLayoutContracts", UnitTestBase)
{
	// Retrieve our important objects
	auto& renderer = session.Renderer();

	// Verify that a layout cannot be constructed before a definition has been started.
	{
		auto layout = renderer.CreateStateBufferLayout();
		REQUIRE(!layout->ConstructStateLayout());
	}

	// Verify that fields are ignored until a layout definition has been started.
	{
		auto layout = renderer.CreateStateBufferLayout();
		layout->AppendField("unstartedField", IStateBufferLayout::DataType::Float32);
		REQUIRE(!layout->ConstructStateLayout());
	}

	// Verify that an empty definition is rejected.
	{
		auto layout = renderer.CreateStateBufferLayout();
		REQUIRE(layout->BeginLayoutDefinition());
		REQUIRE(!layout->ConstructStateLayout());
	}

	// Verify that a layout definition cannot be started more than once.
	{
		auto layout = renderer.CreateStateBufferLayout();
		REQUIRE(layout->BeginLayoutDefinition());
		REQUIRE(!layout->BeginLayoutDefinition());
	}

	// Verify that automatic fields, vectors, matrices and arrays can be packed into a valid layout.
	{
		auto layout = renderer.CreateStateBufferLayout();
		REQUIRE(layout->BeginLayoutDefinition());
		layout->AppendField("scalarValue", IStateBufferLayout::DataType::Float32);
		layout->AppendField("scalarArray", IStateBufferLayout::DataType::Float32, 3);
		layout->AppendVector("alignedVector", IStateBufferLayout::DataType::Float32, 4);
		layout->AppendMatrix("matrixValue", IStateBufferLayout::DataType::Float32, 3, 3);
		REQUIRE(layout->ConstructStateLayout());
	}

	// Verify that duplicate field names and mutation after construction are rejected without invalidating the layout.
	{
		auto layout = renderer.CreateStateBufferLayout();
		REQUIRE(layout->BeginLayoutDefinition());
		layout->AppendField("sharedName", IStateBufferLayout::DataType::Int32);
		layout->AppendVector("sharedName", IStateBufferLayout::DataType::Float32, 2);
		REQUIRE(layout->ConstructStateLayout());
		layout->AppendField("lateField", IStateBufferLayout::DataType::Float32);
		REQUIRE(!layout->ConstructStateLayout());
	}

	session.AddTestSuccess("StateBufferLayoutContracts", "State buffer layout construction rejected invalid state transitions and accepted a representative automatic layout.");
	return true;
}

} // namespace cobalt::graphics::testing
