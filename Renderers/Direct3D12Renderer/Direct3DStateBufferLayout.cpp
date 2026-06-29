// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "Direct3DStateBufferLayout.h"
#include "Direct3DRenderer.h"
#include <algorithm>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
Direct3DStateBufferLayout::Direct3DStateBufferLayout(cobalt::logging::ILogger* log, Direct3DRenderer* renderer)
: _log(log), _renderer(renderer), _layoutBuilt(false), _layoutBuildInProgress(false), _nextAutoFieldOffsetInBytes(0)
{}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void Direct3DStateBufferLayout::Delete()
{
	_renderer->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
// State layout building methods
//----------------------------------------------------------------------------------------
SuccessToken Direct3DStateBufferLayout::BeginLayoutDefinition()
{
	return BeginLayoutDefinition(false);
}

//----------------------------------------------------------------------------------------
bool Direct3DStateBufferLayout::BeginManualLayoutDefinition()
{
	return BeginLayoutDefinition(true);
}

//----------------------------------------------------------------------------------------
bool Direct3DStateBufferLayout::BeginLayoutDefinition(bool manualLayoutMode)
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
void Direct3DStateBufferLayout::AppendField(const Marshal::In<std::string>& fieldName, DataType type, size_t arraySize)
{
	std::vector<size_t> dummyVector;
	AddFieldEntry(fieldName, true, 0, type, false, false, 0, 0, arraySize, 0, dummyVector, dummyVector);
}

//----------------------------------------------------------------------------------------
void Direct3DStateBufferLayout::AppendVector(const Marshal::In<std::string>& fieldName, DataType type, size_t elementCount, size_t arraySize)
{
	std::vector<size_t> dummyVector;
	AddFieldEntry(fieldName, true, 0, type, true, false, elementCount, 0, arraySize, 0, dummyVector, dummyVector);
}

//----------------------------------------------------------------------------------------
void Direct3DStateBufferLayout::AppendMatrix(const Marshal::In<std::string>& fieldName, DataType type, size_t width, size_t height, size_t arraySize)
{
	std::vector<size_t> dummyVector;
	AddFieldEntry(fieldName, true, 0, type, false, true, width, height, arraySize, 0, dummyVector, dummyVector);
}

//----------------------------------------------------------------------------------------
void Direct3DStateBufferLayout::AddManualField(const std::string& fieldName, size_t bufferOffsetInBytes, DataType type, size_t arraySize, size_t arrayStrideInBytes, const std::vector<size_t>& leadingArraySizes, const std::vector<size_t>& leadingArrayStridesInBytes)
{
	AddFieldEntry(fieldName, false, bufferOffsetInBytes, type, false, false, 0, 0, arraySize, arrayStrideInBytes, leadingArraySizes, leadingArrayStridesInBytes);
}

//----------------------------------------------------------------------------------------
void Direct3DStateBufferLayout::AddManualVector(const std::string& fieldName, size_t bufferOffsetInBytes, DataType type, size_t elementCount, size_t arraySize, size_t arrayStrideInBytes, const std::vector<size_t>& leadingArraySizes, const std::vector<size_t>& leadingArrayStridesInBytes)
{
	AddFieldEntry(fieldName, false, bufferOffsetInBytes, type, true, false, elementCount, 0, arraySize, arrayStrideInBytes, leadingArraySizes, leadingArrayStridesInBytes);
}

//----------------------------------------------------------------------------------------
void Direct3DStateBufferLayout::AddManualMatrix(const std::string& fieldName, size_t bufferOffsetInBytes, DataType type, size_t width, size_t height, size_t arraySize, size_t arrayStrideInBytes, const std::vector<size_t>& leadingArraySizes, const std::vector<size_t>& leadingArrayStridesInBytes)
{
	AddFieldEntry(fieldName, false, bufferOffsetInBytes, type, false, true, width, height, arraySize, arrayStrideInBytes, leadingArraySizes, leadingArrayStridesInBytes);
}

//----------------------------------------------------------------------------------------
void Direct3DStateBufferLayout::AddFieldEntry(const std::string& fieldName, bool useAutoOffset, size_t bufferOffsetInBytes, DataType type, bool isVector, bool isMatrix, size_t width, size_t height, size_t arraySize, size_t arrayStrideInBytes, const std::vector<size_t>& leadingArraySizes, const std::vector<size_t>& leadingArrayStridesInBytes)
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

	// Calculate the size of the field. The alignment rules here follow the Direct3D packing rules as defined here:
	// https://docs.microsoft.com/en-us/windows/desktop/direct3dhlsl/dx-graphics-hlsl-packing-rules
	if (fieldEntry.isArray)
	{
		arrayStrideInBytes = (arrayStrideInBytes > 0) ? arrayStrideInBytes : (fieldEntry.dataTypeByteSize * 4 * (fieldEntry.isMatrix ? fieldEntry.height : 1));
		fieldEntry.totalFieldEntryByteSize = arrayStrideInBytes * (fieldEntry.isArray ? arraySize : 1);
	}
	else if (fieldEntry.isMatrix)
	{
		fieldEntry.totalFieldEntryByteSize = (4 * fieldEntry.dataTypeByteSize * (fieldEntry.height - 1)) + (fieldEntry.width * fieldEntry.dataTypeByteSize);
	}
	else if (fieldEntry.isVector)
	{
		fieldEntry.totalFieldEntryByteSize = fieldEntry.width * fieldEntry.dataTypeByteSize;
	}
	else
	{
		fieldEntry.totalFieldEntryByteSize = fieldEntry.dataTypeByteSize;
	}

	// Calculate the position of the field in the layout
	if (useAutoOffset)
	{
		const uint32_t alignmentSizeInBytes = 16;
		size_t offsetIntoAlignmentWindow = (_nextAutoFieldOffsetInBytes % alignmentSizeInBytes);
		auto remainingBytesInAlignmentWindow = alignmentSizeInBytes - offsetIntoAlignmentWindow;
		if ((offsetIntoAlignmentWindow > 0) && (remainingBytesInAlignmentWindow < fieldEntry.totalFieldEntryByteSize))
		{
			fieldEntry.bufferOffsetInBytes = _nextAutoFieldOffsetInBytes + remainingBytesInAlignmentWindow;
		}
		else
		{
			fieldEntry.bufferOffsetInBytes = _nextAutoFieldOffsetInBytes;
		}
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
SuccessToken Direct3DStateBufferLayout::ConstructStateLayout()
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
size_t Direct3DStateBufferLayout::GetTotalLayoutSizeInBytes() const
{
	return _nextAutoFieldOffsetInBytes;
}

//----------------------------------------------------------------------------------------
StateValueId Direct3DStateBufferLayout::GetFieldId(const std::string& fieldName) const
{
	auto fieldNameIterator = _fieldNameToID.find(fieldName);
	if (fieldNameIterator == _fieldNameToID.end())
	{
		return StateValueId::Null;
	}
	return fieldNameIterator->second;
}

//----------------------------------------------------------------------------------------
const Direct3DStateBufferLayout::FieldEntry* Direct3DStateBufferLayout::GetFieldEntryInfo(StateValueId fieldID) const
{
	if ((fieldID == StateValueId::Null) || ((size_t)fieldID >= _fieldEntries.size()))
	{
		return nullptr;
	}
	return &_fieldEntries[(size_t)fieldID];
}

} // namespace cobalt::graphics
