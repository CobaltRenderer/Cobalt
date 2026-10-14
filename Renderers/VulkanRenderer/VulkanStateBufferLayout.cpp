// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "VulkanStateBufferLayout.h"
#include "VulkanRenderer.h"
#include <algorithm>
#include <cstring>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
VulkanStateBufferLayout::VulkanStateBufferLayout(cobalt::logging::ILogger* log, VulkanRenderer* renderer)
: _log(log), _renderer(renderer), _layoutBuilt(false), _layoutBuildInProgress(false), _nextAutoFieldOffsetInBytes(0)
{}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void VulkanStateBufferLayout::Delete()
{
	_renderer->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
// State layout building methods
//----------------------------------------------------------------------------------------
SuccessToken VulkanStateBufferLayout::BeginLayoutDefinition()
{
	return BeginLayoutDefinition(false);
}

//----------------------------------------------------------------------------------------
bool VulkanStateBufferLayout::BeginManualLayoutDefinition()
{
	return BeginLayoutDefinition(true);
}

//----------------------------------------------------------------------------------------
bool VulkanStateBufferLayout::BeginLayoutDefinition(bool manualLayoutMode)
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
void VulkanStateBufferLayout::AppendField(const Marshal::In<std::string>& fieldName, DataType type, size_t arraySize)
{
	std::vector<size_t> dummyVector;
	AddFieldEntry(fieldName, true, 0, type, false, false, 0, 0, arraySize, 0, dummyVector, dummyVector);
}

//----------------------------------------------------------------------------------------
void VulkanStateBufferLayout::AppendVector(const Marshal::In<std::string>& fieldName, DataType type, size_t elementCount, size_t arraySize)
{
	std::vector<size_t> dummyVector;
	AddFieldEntry(fieldName, true, 0, type, true, false, elementCount, 0, arraySize, 0, dummyVector, dummyVector);
}

//----------------------------------------------------------------------------------------
void VulkanStateBufferLayout::AppendMatrix(const Marshal::In<std::string>& fieldName, DataType type, size_t width, size_t height, size_t arraySize)
{
	std::vector<size_t> dummyVector;
	AddFieldEntry(fieldName, true, 0, type, false, true, width, height, arraySize, 0, dummyVector, dummyVector);
}

//----------------------------------------------------------------------------------------
void VulkanStateBufferLayout::AddManualField(const std::string& fieldName, size_t bufferOffsetInBytes, DataType type, size_t arraySize, size_t arrayStrideInBytes, const std::vector<size_t>& leadingArraySizes, const std::vector<size_t>& leadingArrayStridesInBytes)
{
	AddFieldEntry(fieldName, false, bufferOffsetInBytes, type, false, false, 0, 0, arraySize, arrayStrideInBytes, leadingArraySizes, leadingArrayStridesInBytes);
}

//----------------------------------------------------------------------------------------
void VulkanStateBufferLayout::AddManualVector(const std::string& fieldName, size_t bufferOffsetInBytes, DataType type, size_t elementCount, size_t arraySize, size_t arrayStrideInBytes, const std::vector<size_t>& leadingArraySizes, const std::vector<size_t>& leadingArrayStridesInBytes)
{
	AddFieldEntry(fieldName, false, bufferOffsetInBytes, type, true, false, elementCount, 0, arraySize, arrayStrideInBytes, leadingArraySizes, leadingArrayStridesInBytes);
}

//----------------------------------------------------------------------------------------
void VulkanStateBufferLayout::AddManualMatrix(const std::string& fieldName, size_t bufferOffsetInBytes, DataType type, size_t width, size_t height, size_t arraySize, size_t arrayStrideInBytes, const std::vector<size_t>& leadingArraySizes, const std::vector<size_t>& leadingArrayStridesInBytes, size_t matrixStrideInBytes)
{
	AddFieldEntry(fieldName, false, bufferOffsetInBytes, type, false, true, width, height, arraySize, arrayStrideInBytes, leadingArraySizes, leadingArrayStridesInBytes, matrixStrideInBytes);
}

//----------------------------------------------------------------------------------------
void VulkanStateBufferLayout::AddFieldEntry(const std::string& fieldName, bool useAutoOffset, size_t bufferOffsetInBytes, DataType type, bool isVector, bool isMatrix, size_t width, size_t height, size_t arraySize, size_t arrayStrideInBytes, const std::vector<size_t>& leadingArraySizes, const std::vector<size_t>& leadingArrayStridesInBytes, size_t matrixStrideInBytes)
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
	fieldEntry.isVector = isVector && (width > 0);
	fieldEntry.isMatrix = isMatrix && (width > 0) && (height > 0);
	fieldEntry.width = width;
	fieldEntry.height = height;
	fieldEntry.isArray = (arraySize > 0);
	fieldEntry.dataTypeByteSize = GetDataTypeByteSize(type);

	// Automatic layouts use std140. Reflected layouts supply their actual offsets and strides.
	// A vec3 has vec4 alignment but only three writable components. Arrays and matrices
	// reserve their final padding, that padding is never part of a SetStateValue write.
	const size_t vectorWidth = width > 0 ? width : 1;
	fieldEntry.baseAlignment = fieldEntry.dataTypeByteSize * (vectorWidth == 3 ? 4 : vectorWidth);
	if (fieldEntry.isArray || fieldEntry.isMatrix)
	{
		fieldEntry.baseAlignment = std::max(size_t(16), fieldEntry.baseAlignment);
	}
	fieldEntry.matrixStrideInBytes = fieldEntry.isMatrix ? (matrixStrideInBytes > 0 ? matrixStrideInBytes : fieldEntry.baseAlignment) : 0;
	fieldEntry.entrySizeInBytes = ElementSize(fieldEntry.dataTypeByteSize, width, height, fieldEntry.matrixStrideInBytes);
	fieldEntry.totalFieldEntryByteSize = fieldEntry.isMatrix ? fieldEntry.matrixStrideInBytes * height : fieldEntry.entrySizeInBytes;
	if (fieldEntry.isArray)
	{
		arrayStrideInBytes = arrayStrideInBytes > 0 ? arrayStrideInBytes : ((fieldEntry.totalFieldEntryByteSize + fieldEntry.baseAlignment - 1) / fieldEntry.baseAlignment) * fieldEntry.baseAlignment;
		fieldEntry.totalFieldEntryByteSize = arrayStrideInBytes * arraySize;
	}
	if (!useAutoOffset)
	{
		// Reflection may describe a packed layout with a following field in the final padding.
		fieldEntry.totalFieldEntryByteSize = fieldEntry.entrySizeInBytes + (fieldEntry.isArray ? (arraySize - 1) * arrayStrideInBytes : 0);
	}

	// Calculate the position of the field in the layout
	fieldEntry.bufferOffsetInBytes = useAutoOffset ? ((_nextAutoFieldOffsetInBytes + fieldEntry.baseAlignment - 1) / fieldEntry.baseAlignment) * fieldEntry.baseAlignment : bufferOffsetInBytes;

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
SuccessToken VulkanStateBufferLayout::ConstructStateLayout()
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
size_t VulkanStateBufferLayout::GetTotalLayoutSizeInBytes() const
{
	return _nextAutoFieldOffsetInBytes;
}

//----------------------------------------------------------------------------------------
StateValueId VulkanStateBufferLayout::GetFieldId(const std::string& fieldName) const
{
	auto fieldNameIterator = _fieldNameToID.find(fieldName);
	if (fieldNameIterator == _fieldNameToID.end())
	{
		return StateValueId::Null;
	}
	return fieldNameIterator->second;
}

//----------------------------------------------------------------------------------------
const VulkanStateBufferLayout::FieldEntry* VulkanStateBufferLayout::GetFieldEntryInfo(StateValueId fieldID) const
{
	if ((fieldID == StateValueId::Null) || ((size_t)fieldID >= _fieldEntries.size()))
	{
		return nullptr;
	}
	return &_fieldEntries[(size_t)fieldID];
}

//----------------------------------------------------------------------------------------
size_t VulkanStateBufferLayout::VectorSize(size_t scalarSize, size_t width)
{
	return scalarSize * (width > 0 ? width : 1);
}

//----------------------------------------------------------------------------------------
size_t VulkanStateBufferLayout::ElementSize(size_t scalarSize, size_t width, size_t height, size_t matrixStride)
{
	return (height > 0 ? height - 1 : 0) * matrixStride + VectorSize(scalarSize, width);
}

//----------------------------------------------------------------------------------------
void VulkanStateBufferLayout::CopyValue(uint8_t* destination, const uint8_t* source, size_t sourceSize, size_t scalarSize, size_t width, size_t height, size_t matrixStride)
{
	const size_t vectorSize = VectorSize(scalarSize, width);
	const size_t vectorCount = height > 0 ? height : 1;
	for (size_t i = 0; i < vectorCount && sourceSize > 0; ++i)
	{
		const size_t copySize = std::min(sourceSize, vectorSize);
		std::memcpy(destination + i * matrixStride, source, copySize);
		source += copySize;
		sourceSize -= copySize;
	}
}

} // namespace cobalt::graphics
