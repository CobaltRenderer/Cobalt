// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Direct3DHeaders.h"
// This needs to be after Direct3DHeaders.h so we have our defines for Windows.h set properly
#include "D3D12MemAlloc.h"
#include "DescriptorHandle.h"
#include "Direct3DCommandListPool.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <atomic>
#include <mutex>
namespace cobalt::graphics {
class Direct3DRenderer;
class Direct3DTransferBatch;
class Direct3DDataArrayOutput;

class Direct3DDataArray : public IDataArray
{
public:
	// Constructors
	Direct3DDataArray(cobalt::logging::ILogger* log, Direct3DRenderer* renderer);
	~Direct3DDataArray();

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
	bool HasCaptureTargets() const;
	void AddOutputCaptureTarget(IDataArrayOutput* captureTarget) override;
	void RemoveOutputCaptureTarget(IDataArrayOutput* captureTarget) override;
	void CaptureDataBufferOutput(ID3D12GraphicsCommandList* commandList);
	void CompleteCaptureDataBufferOutput();

	// Build state methods
	void MigrateBuildStateToDrawState();
	bool HasCounter() const;
	void ResetCounter(ID3D12GraphicsCommandList* commandList);
	size_t CompletePendingDataWrites(ID3D12GraphicsCommandList* commandList, D3DX12Residency::ResidencySet* residencySet);
	size_t CompletePendingDataTransfers(ID3D12GraphicsCommandList* commandList, D3DX12Residency::ResidencySet* residencySet);
	ID3D12Resource* GetNativeBuffer() const;
	ID3D12Resource* GetCounterBuffer() const;
	D3D12_RESOURCE_STATES GetLastResourceState() const;
	const CD3DX12_GPU_DESCRIPTOR_HANDLE& GetReadOnlyGPUDescriptorHandle(ID3D12GraphicsCommandList* commandList, D3D_SHADER_INPUT_TYPE type);
	const CD3DX12_GPU_DESCRIPTOR_HANDLE& GetReadWriteGPUDescriptorHandle(ID3D12GraphicsCommandList* commandList, D3D_SHADER_INPUT_TYPE type);
	void AddAsCurrentBuffer();

private:
	// Structures
	struct PendingWrite
	{
		explicit PendingWrite(Direct3DTransferBatch* transferBatch)
		: transferBatch(transferBatch)
		{}

		ID3D12Resource* uploadBuffer = nullptr;
		size_t targetBufferPos = 0;
		size_t uploadBufferSizeInBytes = 0;
		Direct3DTransferBatch* transferBatch;
	};
	struct PendingTransfer
	{
		explicit PendingTransfer(Direct3DDataArray* targetBuffer, Direct3DTransferBatch* transferBatch)
		: targetBuffer(targetBuffer), transferBatch(transferBatch)
		{}

		Direct3DDataArray* targetBuffer;
		Direct3DTransferBatch* transferBatch;
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
		Microsoft::WRL::ComPtr<ID3D12Resource> counterClearTransferBuffer;
		D3D12MA::Allocation* counterClearTransferBufferAllocation = nullptr;
		std::vector<Direct3DDataArrayOutput*> captureTargets;
	};

private:
	// Initialization methods
	void ReleaseMemory();

	// Build state methods
	bool CreateNativeBuffer();
	void CreateCounterClearTransferBuffer(UINT counterResetValue, Microsoft::WRL::ComPtr<ID3D12Resource>& counterClearTransferBuffer, D3D12MA::Allocation*& counterClearTransferBufferAllocation);
	size_t CompletePendingDataWritesInternal(std::vector<PendingWrite>& pendingWrites, ID3D12GraphicsCommandList* commandList, D3DX12Residency::ResidencySet* residencySet, bool performDrawStateTransition);
	void WriteDataToMappedBuffer(const PendingWrite& pendingWrite, const uint8_t* data, uint8_t* mappedMemory);
	void CompletePendingDataWrite(ID3D12GraphicsCommandList* commandList, D3DX12Residency::ResidencySet* residencySet, const PendingWrite& pendingWrite);
	void CompletePendingDataTransfer(ID3D12GraphicsCommandList* commandList, D3DX12Residency::ResidencySet* residencySet, const PendingTransfer& pendingTransfer);
	void FlagBuildStateModified();

private:
	cobalt::logging::ILogger* _log;
	mutable std::mutex _accessMutex;
	Direct3DRenderer* _renderer;
	bool _bufferLayoutSet = false;
	size_t _structureEntryCount = 0;
	size_t _structureStrideInBytes = 0;
	size_t _totalBufferSizeInBytes = 0;
	bool _hasCounter = false;
	uint32_t _counterResetValue = 0;
	D3D12_RESOURCE_STATES _counterResourceState = {};
	bool _bufferCreated = false;
	Microsoft::WRL::ComPtr<ID3D12Resource> _buffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> _counter;
	Microsoft::WRL::ComPtr<ID3D12Resource> _counterClearTransferBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> _captureDataStagingBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> _captureCounterStagingBuffer;
	D3D12MA::Allocation* _counterClearTransferBufferAllocation = nullptr;
	D3D12MA::Allocation* _captureDataStagingBufferAllocation = nullptr;
	D3D12MA::Allocation* _captureCounterStagingBufferAllocation = nullptr;
	std::unique_ptr<DescriptorHandle> _readOnlyViewHandle;
	std::unique_ptr<DescriptorHandle> _readWriteViewHandle;
	bool _createdReadOnlyView = false;
	bool _createdReadWriteView = false;
	bool _initialDataSet = false;
	const uint8_t* _initialData = nullptr;
	size_t _initialDataSizeInBytes = 0;
	bool _addedAsCurrent = false;
	UsageFlags _usageFlags = UsageFlags::Default;
	PerformanceHint _performanceHintCpu = PerformanceHint::Default;
	PerformanceHint _performanceHintGpu = PerformanceHint::Default;
	DataPersistenceFlags _dataPersistenceFlags = DataPersistenceFlags::PersistAlways;
	std::mutex _buildStateMutex;
	std::atomic_flag _stateModified = ATOMIC_FLAG_INIT;
	uint32_t _buildIndex;
	uint32_t _drawIndex;
	MutableState _state[2];
	D3D12_RESOURCE_STATES _lastResourceState = {};
	D3D12_RESOURCE_STATES _defaultResourceState = {};
};

} // namespace cobalt::graphics
