// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "StateBufferLayout.h"
#include <Cobalt/RendererInterface/RendererInterface.pkg>

using namespace cobalt::graphics;

//----------------------------------------------------------------------------------------
// State layout building methods
//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_StateBufferLayout_BeginLayoutDefinition(Cobalt_StateBufferLayout layout)
{
	auto _this = reinterpret_cast<IStateBufferLayout*>(layout);

	return _this->BeginLayoutDefinition() ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBufferLayout_AppendField(Cobalt_StateBufferLayout layout, const char* fieldName, size_t fieldNameLength, Cobalt_StateBufferDataType type, size_t arraySize)
{
	auto _this = reinterpret_cast<IStateBufferLayout*>(layout);
	auto name = std::string(fieldName, fieldNameLength);

	_this->AppendField(name, (IStateBufferLayout::DataType)type, arraySize);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBufferLayout_AppendVector(Cobalt_StateBufferLayout layout, const char* fieldName, size_t fieldNameLength, Cobalt_StateBufferDataType type, size_t elementCount, size_t arraySize)
{
	auto _this = reinterpret_cast<IStateBufferLayout*>(layout);
	auto name = std::string(fieldName, fieldNameLength);

	_this->AppendVector(name, (IStateBufferLayout::DataType)type, elementCount, arraySize);
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBufferLayout_AppendMatrix(Cobalt_StateBufferLayout layout, const char* fieldName, size_t fieldNameLength, Cobalt_StateBufferDataType type, size_t width, size_t height, size_t arraySize)
{
	auto _this = reinterpret_cast<IStateBufferLayout*>(layout);
	auto name = std::string(fieldName, fieldNameLength);

	_this->AppendMatrix(name, (IStateBufferLayout::DataType)type, width, height, arraySize);
}

//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_StateBufferLayout_ConstructStateLayout(Cobalt_StateBufferLayout layout)
{
	auto _this = reinterpret_cast<IStateBufferLayout*>(layout);

	return _this->ConstructStateLayout() ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
void Cobalt_StateBufferLayout_Delete(Cobalt_StateBufferLayout layout)
{
	auto _this = reinterpret_cast<IStateBufferLayout*>(layout);

	_this->Delete();
}
