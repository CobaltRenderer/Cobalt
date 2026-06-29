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

class OpenGLVertexBuffer : public IVertexBuffer
{
public:
	// Structures
	struct VertexAttributeInfo
	{
		IVertexAttribute::DataType dataType;
		size_t dataTypeByteSize;
		size_t elementCount;
		size_t vertexCount;
		IVertexAttribute::PerformanceHint performanceHintCpu;
		IVertexAttribute::PerformanceHint performanceHintGpu;
		IVertexAttribute::DataPersistenceFlags persistenceFlags;
		size_t bufferOffsetInBytes;
		size_t vertexPaddingAtStart;
		size_t vertexPaddingAtEnd;
		size_t bufferStartPosInBytes;
		size_t bufferStrideInBytes;
		bool initialDataSet;
		const uint8_t* initialData;
		size_t initialDataEntryStrideInBytes;
	};

public:
	// Constructors
	OpenGLVertexBuffer(cobalt::logging::ILogger* log, OpenGLRenderer* renderer);
	~OpenGLVertexBuffer();

	// Initialization methods
	void Delete() override;
	SuccessToken AllocateMemory() override;
	SuccessToken AllocateMemoryWithAlias(ITexelArray* texelArray) override;
	bool IsAllocated() const;

	// Binding methods
	SuccessToken BindVertexAttribute(IVertexAttribute& vertexAttribute) override;
	SuccessToken BindVertexAttributeManualLayout(IVertexAttribute& vertexAttribute, size_t bufferOffsetInBytes, size_t bufferStrideInBytes) override;
	const VertexAttributeInfo* GetVertexAttributeInfo(size_t attributeIndex) const;

	// Data methods
	SuccessToken SetRawInitialData(const uint8_t* data, size_t dataSizeInBytes) override;
	SuccessToken QueueRawDataUpdate(const uint8_t* data, size_t dataSizeInBytes, size_t bufferOffsetInBytes, ITransferBatch* transferBatch) override;

	// Build state methods
	void MigrateBuildStateToDrawState();
	void CompletePendingDataWrites();
	GLuint GetOpenGLBufferNo() const;

protected:
	// Data methods
	SuccessToken SetInitialData(size_t attributeIndex, const uint8_t* data, size_t entryCount, size_t entryStrideInBytes) override;
	SuccessToken QueueDataUpdate(size_t attributeIndex, const uint8_t* data, size_t entryCount, size_t initialVertexNo, size_t entryStrideInBytes, ITransferBatch* transferBatch) override;

private:
	// Structures
	struct PendingWrite
	{
		PendingWrite(const VertexAttributeInfo& attributeInfo, size_t initialBufferPosInBytes, size_t dataSizeInBytes, OpenGLTransferBatch* transferBatch)
		: attributeInfo(attributeInfo), entryCount(dataSizeInBytes), initialVertexNo(initialBufferPosInBytes), entryStrideInBytes(1), rawDataWrite(true), transferBatch(transferBatch)
		{}
		PendingWrite(const VertexAttributeInfo& attributeInfo, size_t entryCount, size_t initialVertexNo, size_t entryStrideInBytes, OpenGLTransferBatch* transferBatch)
		: attributeInfo(attributeInfo), entryCount(entryCount), initialVertexNo(initialVertexNo), entryStrideInBytes(entryStrideInBytes), rawDataWrite(false), transferBatch(transferBatch)
		{}

		bool rawDataWrite;
		const VertexAttributeInfo& attributeInfo;
		size_t entryCount;
		size_t initialVertexNo;
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

	// Binding methods
	bool BindVertexAttributeInternal(IVertexAttribute& vertexAttribute, bool manualLayout, size_t bufferOffsetInBytes, size_t bufferStrideInBytes);

	// Build state methods
	void CreateNativeBuffer();
	bool CompletePendingDataWrite(const PendingWrite& pendingWrite);
	void FlagBuildStateModified();

private:
	cobalt::logging::ILogger* _log;
	OpenGLRenderer* _renderer;
	std::vector<VertexAttributeInfo> _vertexAttributeInfo;
	size_t _vertexAttributeCount;
	size_t _totalBufferSizeInBytes = 0;
	GLenum _usageFlag = {};
	bool _vertexBufferCreated;
	bool _nativeBufferCreationPending;
	GLuint _openGLBufferNo = 0;
	bool _bufferInterleaved = false;
	size_t _interleavedVertexSizeInBytes;
	bool _rawInitialDataSet = false;
	bool _manualBufferLayout = false;
	std::vector<uint8_t> _initialDataBuffer;
	std::mutex _buildStateMutex;
	std::atomic_flag _stateModified = ATOMIC_FLAG_INIT;
	uint32_t _buildIndex;
	uint32_t _drawIndex;
	MutableState _state[2];
};

} // namespace cobalt::graphics
