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
class OpenGLDataArrayOutput;

class OpenGLDataArray : public IDataArray
{
public:
	// Constructors
	OpenGLDataArray(cobalt::logging::ILogger* log, OpenGLRenderer* renderer);
	~OpenGLDataArray();

	// Initialization methods
	void Delete() override;
	SuccessToken AllocateMemory() override;
	void SetBufferLayout(size_t entryStrideInBytes, size_t entryCount, bool hasCounter, uint32_t counterResetValue) override;

	// Usage methods
	void SetUsageFlags(UsageFlags usageFlags) override;
	void SetPerformanceHints(PerformanceHint performanceHintCpu, PerformanceHint performanceHintGpu) override;
	void SetDataPersistenceFlags(DataPersistenceFlags dataPersistenceFlags) override;

	// Initial data methods
	SuccessToken SetInitialData(const void* sourceBuffer, size_t sourceBufferSizeInBytes) override;

	// Data update methods
	SuccessToken QueueDataUpdate(const void* sourceBuffer, size_t sourceBufferSizeInBytes, size_t targetBufferOffsetInBytes, ITransferBatch* transferBatch) override;
	void UpdateCounterResetValue(uint32_t counterResetValue) override;

	// Data transfer methods
	SuccessToken QueueDataTransfer(IDataArray* targetBuffer, size_t transferCountInBytes, size_t sourceBufferOffsetInBytes, size_t targetBufferOffsetInBytes, ITransferBatch* transferBatch) override;

	// Output capture methods
	void AddOutputCaptureTarget(IDataArrayOutput* captureTarget) override;
	void RemoveOutputCaptureTarget(IDataArrayOutput* captureTarget) override;
	void CaptureDataBufferOutput();

	// Build state methods
	void MigrateBuildStateToDrawState();
	void CompletePendingDataWrites();
	void CompletePendingDataTransfers();
	bool HasCounter() const;
	void ResetCounter();
	GLuint GetBufferNo() const;
	GLuint GetCounterBufferNo() const;
	void AddAsCurrentBuffer();
	uint32_t GetCurrentCounterValue();
	void GetCurrentBufferData(size_t bufferOffsetInBytes, void* targetBuffer, size_t byteCount);

private:
	// Structures
	struct PendingWrite
	{
		explicit PendingWrite(OpenGLTransferBatch* transferBatch)
		: transferBatch(transferBatch)
		{}

		size_t targetBufferPos = 0;
		std::vector<uint8_t> data;
		OpenGLTransferBatch* transferBatch;
	};
	struct PendingTransfer
	{
		explicit PendingTransfer(OpenGLDataArray* targetBuffer, OpenGLTransferBatch* transferBatch)
		: targetBuffer(targetBuffer), transferBatch(transferBatch)
		{}

		OpenGLDataArray* targetBuffer;
		OpenGLTransferBatch* transferBatch;
		size_t transferCountInBytes = 0;
		size_t sourceBufferPosInBytes = 0;
		size_t targetBufferPosInBytes = 0;
	};
	struct MutableState
	{
		std::vector<PendingWrite> pendingWrites;
		std::vector<PendingTransfer> pendingTransfers;
		bool updatedCounterResetValue = false;
		uint32_t newCounterResetValue = 0;
		std::vector<OpenGLDataArrayOutput*> captureTargets;
	};

private:
	// Initialization methods
	void ReleaseMemory();

	// Build state methods
	void CreateNativeBuffer();
	void CompletePendingDataWrite(const PendingWrite& pendingWrite);
	void CompletePendingDataTransfer(const PendingTransfer& pendingTransfer);
	void FlagBuildStateModified();

private:
	cobalt::logging::ILogger* _log;
	mutable std::mutex _accessMutex;
	OpenGLRenderer* _renderer;
	bool _bufferLayoutSet = false;
	size_t _structureEntryCount = 0;
	size_t _structureStrideInBytes = 0;
	size_t _totalBufferSizeInBytes = 0;
	bool _hasCounter = false;
	uint32_t _counterResetValue = 0;
	bool _bufferCreated = false;
	GLenum _usageFlag = {};
	bool _nativeBufferCreationPending = false;
	GLuint _openGLBufferNo = 0;
	GLuint _openGLCounterBufferNo = 0;
	bool _initialDataSet = false;
	const uint8_t* _initialData = nullptr;
	size_t _initialDataSizeInBytes = 0;
	bool _addedAsCurrent = false;
	UsageFlags _usageFlags = UsageFlags::Default;
	PerformanceHint _performanceHintCpu = PerformanceHint::Default;
	PerformanceHint _performanceHintGpu = PerformanceHint::Default;
	DataPersistenceFlags _dataPersistenceFlags = DataPersistenceFlags::PersistAlways;
	std::vector<uint8_t> _initialDataBuffer;
	std::mutex _buildStateMutex;
	std::atomic_flag _stateModified = ATOMIC_FLAG_INIT;
	uint32_t _buildIndex;
	uint32_t _drawIndex;
	MutableState _state[2];
};

} // namespace cobalt::graphics
