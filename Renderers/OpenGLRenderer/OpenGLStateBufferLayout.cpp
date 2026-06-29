// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "OpenGLStateBufferLayout.h"
#include "OpenGLRenderer.h"
#include <algorithm>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
OpenGLStateBufferLayout::OpenGLStateBufferLayout(cobalt::logging::ILogger* log, OpenGLRenderer* renderer)
: _log(log), _renderer(renderer), _layoutBuilt(false), _layoutBuildInProgress(false), _nextAutoFieldOffsetInBytes(0)
{}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void OpenGLStateBufferLayout::Delete()
{
	_renderer->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
// State layout building methods
//----------------------------------------------------------------------------------------
SuccessToken OpenGLStateBufferLayout::BeginLayoutDefinition()
{
	return BeginLayoutDefinition(false);
}

//----------------------------------------------------------------------------------------
bool OpenGLStateBufferLayout::BeginManualLayoutDefinition()
{
	return BeginLayoutDefinition(true);
}

//----------------------------------------------------------------------------------------
bool OpenGLStateBufferLayout::BeginLayoutDefinition(bool manualLayoutMode)
{
	// Validate the current state of the layout
	if (_layoutBuilt || _layoutBuildInProgress)
	{
		_log->Error("Attempted to start defining a state buffer layout which was already built or being defined");
		return false;
	}

	// Flag that the layout build is in progress
	_layoutBuildInProgress = true;
	_manualLayoutMode = manualLayoutMode;
	return true;
}

//----------------------------------------------------------------------------------------
void OpenGLStateBufferLayout::AppendField(const Marshal::In<std::string>& fieldName, DataType type, size_t arraySize)
{
	std::vector<size_t> dummyVector;
	AddFieldEntry(fieldName, true, 0, type, false, false, 0, 0, arraySize, 0, dummyVector, dummyVector);
}

//----------------------------------------------------------------------------------------
void OpenGLStateBufferLayout::AppendVector(const Marshal::In<std::string>& fieldName, DataType type, size_t elementCount, size_t arraySize)
{
	std::vector<size_t> dummyVector;
	AddFieldEntry(fieldName, true, 0, type, true, false, elementCount, 0, arraySize, 0, dummyVector, dummyVector);
}

//----------------------------------------------------------------------------------------
void OpenGLStateBufferLayout::AppendMatrix(const Marshal::In<std::string>& fieldName, DataType type, size_t width, size_t height, size_t arraySize)
{
	std::vector<size_t> dummyVector;
	AddFieldEntry(fieldName, true, 0, type, false, true, width, height, arraySize, 0, dummyVector, dummyVector);
}

//----------------------------------------------------------------------------------------
void OpenGLStateBufferLayout::AddManualField(const std::string& fieldName, size_t bufferOffsetInBytes, DataType type, size_t arraySize, size_t arrayStrideInBytes, const std::vector<size_t>& leadingArraySizes, const std::vector<size_t>& leadingArrayStridesInBytes)
{
	AddFieldEntry(fieldName, false, bufferOffsetInBytes, type, false, false, 0, 0, arraySize, arrayStrideInBytes, leadingArraySizes, leadingArrayStridesInBytes);
}

//----------------------------------------------------------------------------------------
void OpenGLStateBufferLayout::AddManualVector(const std::string& fieldName, size_t bufferOffsetInBytes, DataType type, size_t elementCount, size_t arraySize, size_t arrayStrideInBytes, const std::vector<size_t>& leadingArraySizes, const std::vector<size_t>& leadingArrayStridesInBytes)
{
	AddFieldEntry(fieldName, false, bufferOffsetInBytes, type, true, false, elementCount, 0, arraySize, arrayStrideInBytes, leadingArraySizes, leadingArrayStridesInBytes);
}

//----------------------------------------------------------------------------------------
void OpenGLStateBufferLayout::AddManualMatrix(const std::string& fieldName, size_t bufferOffsetInBytes, DataType type, size_t width, size_t height, size_t arraySize, size_t arrayStrideInBytes, const std::vector<size_t>& leadingArraySizes, const std::vector<size_t>& leadingArrayStridesInBytes)
{
	AddFieldEntry(fieldName, false, bufferOffsetInBytes, type, false, true, width, height, arraySize, arrayStrideInBytes, leadingArraySizes, leadingArrayStridesInBytes);
}

//----------------------------------------------------------------------------------------
void OpenGLStateBufferLayout::AddFieldEntry(const std::string& fieldName, bool useAutoOffset, size_t bufferOffsetInBytes, DataType type, bool isVector, bool isMatrix, size_t width, size_t height, size_t arraySize, size_t arrayStrideInBytes, const std::vector<size_t>& leadingArraySizes, const std::vector<size_t>& leadingArrayStridesInBytes)
{
	// Validate the current state of the layout
	if (!_layoutBuildInProgress)
	{
		_log->Error("Attempted to add field entry with name {0} to a state buffer layout which was constructed or did not have a build in progress", fieldName);
		return;
	}

	// Validate the supplied field information
	if (useAutoOffset == _manualLayoutMode)
	{
		_log->Error("Attempted to add field entry with name {0} to a state buffer layout with the wrong layout mode", fieldName);
		return;
	}
	if (_fieldNameToID.find(fieldName) != _fieldNameToID.end())
	{
		_log->Error("Attempted to add field entry with name {0} to a state buffer layout when a field with that name has already been added", fieldName);
		return;
	}

	// Create a new entry for this field
	FieldEntry fieldEntry;
	fieldEntry.name = fieldName;
	fieldEntry.type = type;
	fieldEntry.isVector = isVector && (width > 1);
	fieldEntry.isMatrix = isMatrix && (width > 1) && (height > 1);
	fieldEntry.width = width;
	fieldEntry.height = height;
	fieldEntry.isArray = (arraySize > 1);
	fieldEntry.dataTypeByteSize = GetDataTypeByteSize(type);

	// Calculate the alignment and size of the field. The alignment rules here follow the OpenGL "std140" layout mode,
	// as defined in the OpenGL 4.5 specification, section 7.6.2.2 "Standard Uniform Block Layout", as detailed here:
	// https://www.khronos.org/registry/OpenGL/specs/gl/glspec45.core.pdf#page=159
	if (fieldEntry.isArray || fieldEntry.isMatrix)
	{
		fieldEntry.baseAlignment = fieldEntry.dataTypeByteSize * 4;
		arrayStrideInBytes = (arrayStrideInBytes > 0) ? arrayStrideInBytes : (fieldEntry.baseAlignment * (fieldEntry.isMatrix ? fieldEntry.height : 1));
		fieldEntry.totalFieldEntryByteSize = arrayStrideInBytes * (fieldEntry.isArray ? arraySize : 1);
	}
	else if (fieldEntry.isVector)
	{
		auto paddedElementCount = fieldEntry.width;
		paddedElementCount += (paddedElementCount == 3) ? 1 : 0;
		fieldEntry.baseAlignment = paddedElementCount * fieldEntry.dataTypeByteSize;
		fieldEntry.totalFieldEntryByteSize = fieldEntry.baseAlignment;
	}
	else
	{
		fieldEntry.baseAlignment = fieldEntry.dataTypeByteSize;
		fieldEntry.totalFieldEntryByteSize = fieldEntry.dataTypeByteSize;
	}

	// Calculate the position of the field in the layout
	assert(fieldEntry.baseAlignment > 0);
	if (useAutoOffset)
	{
		fieldEntry.bufferOffsetInBytes = ((_nextAutoFieldOffsetInBytes + (fieldEntry.baseAlignment - 1)) / fieldEntry.baseAlignment) * fieldEntry.baseAlignment;
	}
	else
	{
		fieldEntry.bufferOffsetInBytes = bufferOffsetInBytes;
	}

	// Calculate the end position of this field in the buffer
	size_t followingFieldOffsetInBuffer = fieldEntry.bufferOffsetInBytes + fieldEntry.totalFieldEntryByteSize;
	for (size_t i = 0; i < leadingArraySizes.size(); ++i)
	{
		followingFieldOffsetInBuffer += (leadingArraySizes[i] - 1) * leadingArrayStridesInBytes[i];
	}

	// Store the array sizes and strides for this field
	fieldEntry.arraySizes = leadingArraySizes;
	fieldEntry.arrayStridesInBytes = leadingArrayStridesInBytes;
	if (fieldEntry.isArray)
	{
		fieldEntry.arraySizes.push_back(arraySize);
		fieldEntry.arrayStridesInBytes.push_back(arrayStrideInBytes);
	}

	// Insert this field into the layout
	_fieldNameToID[fieldName] = (StateValueId)_fieldEntries.size();
	_fieldEntries.push_back(fieldEntry);

	// Calculate the auto position of the next field to insert into the layout. Note that this is also used to track the
	// overall size of the layout, even in manual layout mode.
	_nextAutoFieldOffsetInBytes = std::max(_nextAutoFieldOffsetInBytes, followingFieldOffsetInBuffer);
}

//----------------------------------------------------------------------------------------
size_t OpenGLStateBufferLayout::GetDataTypeByteSize(DataType dataType) const
{
	switch (dataType)
	{
	case DataType::Boolean:
	case DataType::Int32:
		return sizeof(int);
	case DataType::UInt32:
		return sizeof(uint32_t);
	case DataType::Float32:
		return sizeof(float);
	case DataType::Float64:
		return sizeof(double);
	}
	UNREACHABLE();
	return 0;
}

//----------------------------------------------------------------------------------------
SuccessToken OpenGLStateBufferLayout::ConstructStateLayout()
{
	// Validate the current state of the layout
	if (!_layoutBuildInProgress)
	{
		_log->Error("Attempted to construct a state buffer layout which was constructed or did not have a build in progress");
		return false;
	}

	// Flag that a layout build is no longer in progress
	_layoutBuildInProgress = false;

	// Ensure that at least one field entry has been added
	if (_fieldEntries.empty())
	{
		_log->Error("Attempted to construct a state buffer layout which contained no fields");
		return false;
	}

	// Flag that the layout has been successfully built, and return true to the caller.
	_layoutBuilt = true;
	return true;
}

//----------------------------------------------------------------------------------------
// State layout query methods
//----------------------------------------------------------------------------------------
size_t OpenGLStateBufferLayout::GetTotalLayoutSizeInBytes() const
{
	return _nextAutoFieldOffsetInBytes;
}

//----------------------------------------------------------------------------------------
StateValueId OpenGLStateBufferLayout::GetFieldId(const std::string& fieldName) const
{
	auto fieldNameIterator = _fieldNameToID.find(fieldName);
	if (fieldNameIterator == _fieldNameToID.end())
	{
		return StateValueId::Null;
	}
	return fieldNameIterator->second;
}

//----------------------------------------------------------------------------------------
const OpenGLStateBufferLayout::FieldEntry* OpenGLStateBufferLayout::GetFieldEntryInfo(StateValueId fieldID) const
{
	if ((fieldID == StateValueId::Null) || ((size_t)fieldID >= _fieldEntries.size()))
	{
		return nullptr;
	}
	return &_fieldEntries[(size_t)fieldID];
}

} // namespace cobalt::graphics
