// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "OpenGLHeaders.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <atomic>
#include <mutex>
#include <vector>
namespace cobalt::graphics {
class OpenGLRenderer;
class OpenGLTransferBatch;
class OpenGLTexelArray;

class OpenGLIndexBuffer : public IIndexBuffer
{
public:
	// Structures
	struct IndexAttributeInfo
	{
		IIndexAttribute::DataType dataType;
		size_t dataTypeByteSize;
		size_t indexCount;
		IIndexAttribute::PerformanceHint performanceHintCpu;
		IIndexAttribute::PerformanceHint performanceHintGpu;
		size_t bufferOffsetInBytes;
		uint32_t minIndexValue;
		uint32_t maxIndexValue;
		bool setMinMaxIndexValue;
		size_t bufferStartPosInBytes;
	};

public:
	// Constructors
	OpenGLIndexBuffer(cobalt::logging::ILogger* log, OpenGLRenderer* renderer);
	~OpenGLIndexBuffer();

	// Initialization methods
	void Delete() override;
	SuccessToken AllocateMemory() override;
	SuccessToken AllocateMemoryWithAlias(ITexelArray* texelArray) override;
	bool IsAllocated() const;

	// Binding methods
	SuccessToken BindIndexAttribute(IIndexAttribute& indexAttribute) override;
	SuccessToken BindIndexAttributeManualLayout(IIndexAttribute& indexAttribute, size_t bufferOffsetInBytes, size_t bufferStrideInBytes) override;
	const IndexAttributeInfo* GetIndexAttributeInfo(size_t attributeIndex) const;

	// Data methods
	SuccessToken SetRawInitialData(const uint8_t* data, size_t dataSizeInBytes) override;
	SuccessToken QueueRawDataUpdate(const uint8_t* data, size_t dataSizeInBytes, size_t bufferOffsetInBytes, ITransferBatch* transferBatch) override;

	// Build state methods
	void MigrateBuildStateToDrawState();
	void CompletePendingDataWrites();
	GLuint GetOpenGLBufferNo() const;

protected:
	// Data methods
	SuccessToken SetInitialData(const uint8_t* data, size_t entryCount, size_t entryStrideInBytes) override;
	SuccessToken QueueDataUpdate(const uint8_t* data, size_t entryCount, size_t initialIndexNo, size_t entryStrideInBytes, ITransferBatch* transferBatch) override;

private:
	// Structures
	struct PendingWrite
	{
		PendingWrite(IndexAttributeInfo& attributeInfo, size_t initialBufferPosInBytes, size_t dataSizeInBytes, OpenGLTransferBatch* transferBatch)
		: attributeInfo(attributeInfo), entryCount(dataSizeInBytes), initialIndexNo(initialBufferPosInBytes), entryStrideInBytes(1), rawDataWrite(true), transferBatch(transferBatch)
		{}
		PendingWrite(IndexAttributeInfo& attributeInfo, size_t entryCount, size_t initialIndexNo, size_t entryStrideInBytes, OpenGLTransferBatch* transferBatch)
		: attributeInfo(attributeInfo), entryCount(entryCount), initialIndexNo(initialIndexNo), entryStrideInBytes(entryStrideInBytes), rawDataWrite(false), transferBatch(transferBatch)
		{}

		bool rawDataWrite;
		IndexAttributeInfo& attributeInfo;
		size_t entryCount;
		size_t initialIndexNo;
		size_t entryStrideInBytes;
		std::vector<uint8_t> data;
		OpenGLTransferBatch* transferBatch;
	};
	struct MutableState
	{
		std::vector<PendingWrite> pendingWrites;
	};

private:
	// Initialization methods
	bool AllocateMemoryInternal(OpenGLTexelArray* texelArray);
	void ReleaseMemory();

	// Data methods
	static void UpdateMinMaxIndexValueForNewData(IndexAttributeInfo& info, const uint8_t* data, size_t entryCount, size_t entryStrideInBytes);

	// Build state methods
	void CreateNativeBuffer();
	bool CompletePendingDataWrite(const PendingWrite& pendingWrite);
	void FlagBuildStateModified();

private:
	cobalt::logging::ILogger* _log;
	OpenGLRenderer* _renderer;
	IndexAttributeInfo _indexAttributeInfo = {};
	GLenum _usageFlag = {};
	bool _indexAttributeAdded = false;
	bool _indexBufferCreated = false;
	bool _nativeBufferCreationPending = false;
	GLuint _openGLBufferNo = 0;
	size_t _totalBufferSizeInBytes = 0;
	bool _initialDataSet = false;
	bool _manualBufferLayout = false;
	const uint8_t* _initialData = nullptr;
	size_t _initialDataEntryCount = 0;
	size_t _initialDataEntryStrideInBytes = 0;
	size_t _initialDataEntrySizeInBytes = 0;
	std::vector<uint8_t> _initialDataBuffer;
	std::mutex _buildStateMutex;
	std::atomic_flag _stateModified = ATOMIC_FLAG_INIT;
	uint32_t _buildIndex;
	uint32_t _drawIndex;
	MutableState _state[2];
};

} // namespace cobalt::graphics
