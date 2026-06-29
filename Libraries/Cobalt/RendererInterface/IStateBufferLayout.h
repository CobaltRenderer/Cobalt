// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Deleter.h"
#include "SuccessToken.h"
#include <Cobalt/Marshalling/Marshalling.pkg>
#include <memory>
#include <string>
namespace cobalt { namespace graphics {
using namespace cobalt::marshalling::operators;

class IStateBufferLayout
{
public:
	// Enumerations
	enum class DataType
	{
		Null = 0,
		Boolean,
		Int32,
		UInt32,
		Float32,
		Float64,
	};

	// Typedefs
	typedef std::unique_ptr<IStateBufferLayout, Deleter<IStateBufferLayout>> unique_ptr;

public:
	// Initialization methods
	virtual void Delete() = 0;

	// State layout building methods
	virtual SuccessToken BeginLayoutDefinition() = 0;
	virtual void AppendField(const Marshal::In<std::string>& fieldName, DataType type, size_t arraySize = 0) = 0;
	virtual void AppendVector(const Marshal::In<std::string>& fieldName, DataType type, size_t elementCount, size_t arraySize = 0) = 0;
	virtual void AppendMatrix(const Marshal::In<std::string>& fieldName, DataType type, size_t width, size_t height, size_t arraySize = 0) = 0;
	virtual SuccessToken ConstructStateLayout() = 0;

protected:
	// Constructors
	~IStateBufferLayout() = default;
};

}} // namespace cobalt::graphics
