// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "OpenGLHeaders.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <atomic>
#include <mutex>
namespace cobalt::graphics {
class OpenGLRenderer;
class OpenGLStateBufferLayout;

class OpenGLStateBuffer : public IStateBuffer
{
public:
	// Constructors
	OpenGLStateBuffer(cobalt::logging::ILogger* log, OpenGLRenderer* renderer);
	~OpenGLStateBuffer();

	// Initialization methods
	void Delete() override;
	SuccessToken AllocateMemory() override;

	// Usage methods
	PerformanceHint GetPerformanceHintCpu() const;
	PerformanceHint GetPerformanceHintGpu() const;
	void SetPerformanceHints(PerformanceHint performanceHintCpu, PerformanceHint performanceHintGpu) override;

	// Format methods
	void SetManualPageSize(size_t pageSizeInBytes) override;
	SuccessToken BindBufferLayout(IStateBufferLayout* stateBufferLayout) override;
	void SetPageSettings(uint32_t initialPageCount, bool allowDynamicResize) override;
	SuccessToken ResizePageCount(uint32_t pageCount) override;

	// Binding methods
	bool BindStateBufferPage(GLuint bindingPoint, uint32_t pageNo);

	// State value methods
	StateValueId GetStateValueId(const Marshal::In<std::string>& name) const override;
	void SetRawPageData(uint32_t pageNo, const uint8_t* data, size_t dataSizeInBytes, size_t dataOffsetInBytes) override;

	// Build state methods
	void MigrateBuildStateToDrawState();
	void CompletePendingDataWrites();
	void CompletePendingDataWritesForPageBlock(uint32_t targetPageBlockNo);

protected:
	// State value methods
	void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, bool value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V1Int8& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V1Int16& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V1Int32& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V1UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V1UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V1UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V1Float32& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V1Float64& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V2Int8& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V2Int16& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V2Int32& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V2UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V2UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V2UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V2Float32& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V2Float64& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V3Int8& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V3Int16& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V3Int32& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V3UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V3UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V3UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V3Float32& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V3Float64& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V4Int8& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V4Int16& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V4Int32& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V4UInt8& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V4UInt16& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V4UInt32& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V4Float32& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const V4Float64& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const M2Float32& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const M3Float32& value, const size_t* arrayIndices, size_t arrayIndexCount) override;
	void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const M4Float32& value, const size_t* arrayIndices, size_t arrayIndexCount) override;

private:
	// Structures
	struct PageBlock
	{
		GLuint bufferNo;
		uint8_t* memoryBuffers[2];
		bool hasUnsavedChanges[2];
		bool nativeBufferAllocated;
	};
	struct MutableState
	{
		uint32_t pageCount;
	};

private:
	// Initialization methods
	void ReleaseMemory();

	// Format methods
	void ResizePageBlockCount(uint32_t pageBlockCount);
	static uint32_t GetHighestSetBitNumber(uint32_t data);

	// State value methods
	void SetStateValueInternal(uint32_t pageNo, StateValueId stateId, const size_t* arrayIndices, size_t arrayIndexCount, const uint8_t* data, size_t dataSizeInBytes);

	// Build state methods
	void CompletePendingDataWritesForPageBlock(PageBlock& pageBlock);
	void FlagBuildStateModified();

private:
	cobalt::logging::ILogger* _log;
	OpenGLRenderer* _renderer;
	OpenGLStateBufferLayout* _stateBufferLayout = nullptr;
	PerformanceHint _performanceHintCpu;
	PerformanceHint _performanceHintGpu;
	size_t _manualPageSizeInBytes = 0;
	bool _manualPageSizeSpecified = false;
	bool _stateBufferSizeSet = false;
	bool _memoryAllocated = false;
	bool _initialDataSet = false;
	const uint8_t* _initialData = nullptr;
	size_t _pageSizeInBytes = 0;
	size_t _usedPageSizeInBytes = 0;
	bool _allowDynamicResize = false;
	size_t _pageBlockSizeInBytes = 0;
	std::vector<PageBlock> _pageBlocks;
	GLenum _usageFlag = {};
	uint32_t _pagesPerPageBlock;
	uint32_t _pageBlockShiftCount;
	uint32_t _pageBlockEntryMask;
	std::mutex _buildStateMutex;
	std::atomic_flag _stateModified = ATOMIC_FLAG_INIT;
	uint32_t _buildIndex;
	uint32_t _drawIndex;
	MutableState _state[2] = {};
};

} // namespace cobalt::graphics
