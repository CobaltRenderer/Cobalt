// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <map>
#include <vector>
namespace cobalt::graphics {
class OpenGLRenderer;

class OpenGLStateBufferLayout : public IStateBufferLayout
{
public:
	// Structures
	struct FieldEntry
	{
		std::string name;
		size_t bufferOffsetInBytes = 0;
		size_t baseAlignment = 0;
		size_t dataTypeByteSize = 0;
		size_t totalFieldEntryByteSize = 0;
		DataType type = DataType::Null;
		bool isVector = false;
		bool isMatrix = false;
		bool isArray = false;
		size_t width = 0;
		size_t height = 0;
		std::vector<size_t> arraySizes;
		std::vector<size_t> arrayStridesInBytes;
	};

public:
	// Constructors
	OpenGLStateBufferLayout(cobalt::logging::ILogger* log, OpenGLRenderer* renderer);

	// Initialization methods
	void Delete() override;

	// State layout building methods
	SuccessToken BeginLayoutDefinition() override;
	void AppendField(const Marshal::In<std::string>& fieldName, DataType type, size_t arraySize) override;
	void AppendVector(const Marshal::In<std::string>& fieldName, DataType type, size_t elementCount, size_t arraySize) override;
	void AppendMatrix(const Marshal::In<std::string>& fieldName, DataType type, size_t width, size_t height, size_t arraySize) override;
	bool BeginManualLayoutDefinition();
	void AddManualField(const std::string& fieldName, size_t bufferOffsetInBytes, DataType type, size_t arraySize, size_t arrayStrideInBytes, const std::vector<size_t>& leadingArraySizes, const std::vector<size_t>& leadingArrayStridesInBytes);
	void AddManualVector(const std::string& fieldName, size_t bufferOffsetInBytes, DataType type, size_t elementCount, size_t arraySize, size_t arrayStrideInBytes, const std::vector<size_t>& leadingArraySizes, const std::vector<size_t>& leadingArrayStridesInBytes);
	void AddManualMatrix(const std::string& fieldName, size_t bufferOffsetInBytes, DataType type, size_t width, size_t height, size_t arraySize, size_t arrayStrideInBytes, const std::vector<size_t>& leadingArraySizes, const std::vector<size_t>& leadingArrayStridesInBytes);
	SuccessToken ConstructStateLayout() override;

	// State layout query methods
	size_t GetTotalLayoutSizeInBytes() const;
	StateValueId GetFieldId(const std::string& fieldName) const;
	const FieldEntry* GetFieldEntryInfo(StateValueId fieldID) const;

private:
	// State layout building methods
	bool BeginLayoutDefinition(bool manualLayoutMode);
	void AddFieldEntry(const std::string& fieldName, bool useAutoOffset, size_t bufferOffsetInBytes, DataType type, bool isVector, bool isMatrix, size_t width, size_t height, size_t arraySize, size_t arrayStrideInBytes, const std::vector<size_t>& leadingArraySizes, const std::vector<size_t>& leadingArrayStridesInBytes);
	size_t GetDataTypeByteSize(DataType dataType) const;

private:
	cobalt::logging::ILogger* _log;
	OpenGLRenderer* _renderer;
	std::vector<FieldEntry> _fieldEntries;
	std::map<std::string, StateValueId> _fieldNameToID;
	bool _layoutBuildInProgress;
	bool _manualLayoutMode = false;
	bool _layoutBuilt;
	size_t _nextAutoFieldOffsetInBytes;
};

} // namespace cobalt::graphics
