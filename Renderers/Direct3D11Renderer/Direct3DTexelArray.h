// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Direct3DHeaders.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <atomic>
#include <mutex>
#include <vector>
namespace cobalt::graphics {
class Direct3DRenderer;
class Direct3DTransferBatch;
class Direct3DTexelArrayOutput;

class Direct3DTexelArray : public ITexelArray
{
public:
	// Constructors
	Direct3DTexelArray(cobalt::logging::ILogger* log, Direct3DRenderer* renderer);
	~Direct3DTexelArray();

	// Initialization methods
	void Delete() override;
	SuccessToken AllocateMemory() override;
	bool AllocateAsAliasForBuffer(ID3D11Buffer* buffer, size_t bufferSizeInBytes);
	void SetBufferLayout(ImageFormat imageFormat, DataFormat dataFormat, size_t entryCount) override;

	// Usage methods
	void SetUsageFlags(UsageFlags usageFlags) override;
	void SetPerformanceHints(PerformanceHint performanceHintCpu, PerformanceHint performanceHintGpu) override;
	void SetDataPersistenceFlags(DataPersistenceFlags dataPersistenceFlags) override;

	// Initial data methods
	SuccessToken SetInitialData(const void* sourceBuffer, size_t sourceBufferSizeInBytes, SourceImageFormat imageFormat, SourceDataFormat dataFormat) override;

	// Data update methods
	SuccessToken QueueDataUpdate(const void* sourceBuffer, size_t sourceBufferSizeInBytes, SourceImageFormat imageFormat, SourceDataFormat dataFormat, size_t targetBufferOffset, ITransferBatch* transferBatch) override;

	// Data transfer methods
	SuccessToken QueueDataTransfer(ITexelArray* targetBuffer, size_t transferCount, size_t sourceBufferOffset, size_t targetBufferOffset, ITransferBatch* transferBatch) override;

	// Output capture methods
	void AddOutputCaptureTarget(ITexelArrayOutput* captureTarget) override;
	void RemoveOutputCaptureTarget(ITexelArrayOutput* captureTarget) override;
	void CaptureDataBufferOutput(ID3D11Device1* device, ID3D11DeviceContext1* context);

	// Build state methods
	void MigrateBuildStateToDrawState();
	void CompletePendingDataWrites(ID3D11Device1* device, ID3D11DeviceContext1* context);
	void CompletePendingDataTransfers(ID3D11Device1* device, ID3D11DeviceContext1* context);
	ID3D11ShaderResourceView* GetReadOnlyView(D3D_SHADER_INPUT_TYPE type);
	ID3D11UnorderedAccessView* GetReadWriteView(D3D_SHADER_INPUT_TYPE type);
	void AddAsCurrentBuffer();

private:
	// Structures
	struct PendingWrite
	{
		explicit PendingWrite(Direct3DTransferBatch* transferBatch)
		: transferBatch(transferBatch)
		{}

		size_t targetBufferPosInBytes = 0;
		std::vector<uint8_t> data;
		Direct3DTransferBatch* transferBatch;
	};
	struct PendingTransfer
	{
		explicit PendingTransfer(Direct3DTexelArray* targetBuffer, Direct3DTransferBatch* transferBatch)
		: targetBuffer(targetBuffer), transferBatch(transferBatch)
		{}

		Direct3DTexelArray* targetBuffer;
		Direct3DTransferBatch* transferBatch;
		size_t transferCountInBytes = 0;
		size_t sourceBufferPosInBytes = 0;
		size_t targetBufferPosInBytes = 0;
	};
	struct MutableState
	{
		std::vector<PendingWrite> pendingWrites;
		std::vector<PendingTransfer> pendingTransfers;
		std::vector<Direct3DTexelArrayOutput*> captureTargets;
	};

private:
	// Initialization methods
	void ReleaseMemory();

	// Data conversion methods
	bool ConvertDataFormat(const void* sourceBuffer, size_t sourceBufferSizeInBytes, SourceImageFormat sourceImageFormat, SourceDataFormat sourceDataFormat, ImageFormat targetImageFormat, DataFormat targetDataFormat, std::vector<uint8_t>& targetBuffer) const;

	// Format methods
	constexpr static bool GetFormatNative(ImageFormat requestedImageFormat, DataFormat requestedDataFormat, DXGI_FORMAT& nativeFormat);

	// Build state methods
	bool CreateNativeBuffer(const uint8_t* initialData);
	void CompletePendingDataWrite(const PendingWrite& pendingWrite, ID3D11Device1* device, ID3D11DeviceContext1* context, void* mappedBuffer);
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
	ImageFormat _imageFormat = {};
	DataFormat _dataFormat = {};
	DXGI_FORMAT _nativeFormat = {};
	bool _bufferCreated = false;
	bool _nativeBufferCreationPending = false;
	Microsoft::WRL::ComPtr<ID3D11Buffer> _buffer;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> _readOnlyView;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> _readWriteView;
	Microsoft::WRL::ComPtr<ID3D11Buffer> _captureDataStagingBuffer;
	ID3D11Buffer* _bufferRawPointer = nullptr;
	bool _createdReadOnlyView = false;
	bool _createdReadWriteView = false;
	bool _usingDynamicBuffer = false;
	bool _allowDiscardOnPartialWrite = false;
	bool _initialDataSet = false;
	bool _addedAsCurrent = false;
	const uint8_t* _initialData = nullptr;
	size_t _initialDataSizeInBytes = 0;
	SourceImageFormat _initialDataImageFormat = {};
	SourceDataFormat _initialDataDataFormat = {};
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
