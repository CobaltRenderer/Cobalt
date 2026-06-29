// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Direct3DHeaders.h"
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
	void AddOutputCaptureTarget(IDataArrayOutput* captureTarget) override;
	void RemoveOutputCaptureTarget(IDataArrayOutput* captureTarget) override;
	void CaptureDataBufferOutput(ID3D11Device1* device, ID3D11DeviceContext1* context);

	// Build state methods
	void MigrateBuildStateToDrawState();
	void CompletePendingDataWrites(ID3D11Device1* device, ID3D11DeviceContext1* context);
	void CompletePendingDataTransfers(ID3D11Device1* device, ID3D11DeviceContext1* context);
	ID3D11Buffer* GetNativeBuffer() const;
	ID3D11ShaderResourceView* GetReadOnlyView(D3D_SHADER_INPUT_TYPE type);
	ID3D11UnorderedAccessView* GetReadWriteView(D3D_SHADER_INPUT_TYPE type);
	bool HasCounter() const;
	UINT GetCounterResetValue() const;
	void AddAsCurrentBuffer();
	UINT GetCurrentCounterValue(ID3D11Device1* device, ID3D11DeviceContext1* context);
	void GetCurrentBufferData(ID3D11Device1* device, ID3D11DeviceContext1* context, size_t bufferOffsetInBytes, void* targetBuffer, size_t byteCount);

private:
	// Structures
	struct PendingWrite
	{
		explicit PendingWrite(Direct3DTransferBatch* transferBatch)
		: transferBatch(transferBatch)
		{}

		size_t targetBufferPos = 0;
		std::vector<uint8_t> data;
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
		bool updatedCounterResetValue = false;
		uint32_t newCounterResetValue = 0;
		std::vector<PendingWrite> pendingWrites;
		std::vector<PendingTransfer> pendingTransfers;
		std::vector<Direct3DDataArrayOutput*> captureTargets;
	};

private:
	// Initialization methods
	void ReleaseMemory();

	// Build state methods
	bool CreateNativeBuffer(const uint8_t* initialData);
	void CompletePendingDataWrite(const PendingWrite& pendingWrite, ID3D11Device1* device, ID3D11DeviceContext1* context);
	void CompletePendingDataTransfer(const PendingTransfer& pendingTransfer, ID3D11Device1* device, ID3D11DeviceContext1* context);
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
	bool _bufferCreated = false;
	bool _nativeBufferCreationPending = false;
	Microsoft::WRL::ComPtr<ID3D11Buffer> _buffer;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> _readOnlyView;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> _readWriteView;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> _readWriteViewWithCounter;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> _readWriteViewAppend;
	Microsoft::WRL::ComPtr<ID3D11Buffer> _captureDataStagingBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> _captureCounterStagingBuffer;
	bool _createdReadOnlyView = false;
	bool _createdReadWriteView = false;
	bool _createdReadWriteViewWithCounter = false;
	bool _createdReadWriteViewAppend = false;
	bool _initialDataSet = false;
	bool _addedAsCurrent = false;
	const uint8_t* _initialData = nullptr;
	size_t _initialDataSizeInBytes = 0;
	UsageFlags _usageFlags = UsageFlags::Default;
	PerformanceHint _performanceHintCpu = PerformanceHint::Default;
	PerformanceHint _performanceHintGpu = PerformanceHint::Default;
	DataPersistenceFlags _dataPersistenceFlags = DataPersistenceFlags::PersistAlways;
	std::vector<uint8_t> _initialDataBuffer;
	UINT _cpuFlags = {};
	D3D11_USAGE _usageType = D3D11_USAGE_DEFAULT;
	std::mutex _buildStateMutex;
	std::atomic_flag _stateModified = ATOMIC_FLAG_INIT;
	uint32_t _buildIndex;
	uint32_t _drawIndex;
	MutableState _state[2];
};

} // namespace cobalt::graphics
