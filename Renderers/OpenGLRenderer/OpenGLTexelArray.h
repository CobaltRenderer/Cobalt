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
class OpenGLTexelArrayOutput;
class OpenGLIndexBuffer;
class OpenGLVertexBuffer;

class OpenGLTexelArray : public ITexelArray
{
public:
	// Constructors
	OpenGLTexelArray(cobalt::logging::ILogger* log, OpenGLRenderer* renderer);
	~OpenGLTexelArray();

	// Initialization methods
	void Delete() override;
	SuccessToken AllocateMemory() override;
	void SetBufferLayout(ImageFormat imageFormat, DataFormat dataFormat, size_t entryCount) override;
	bool AllocateAsAliasForBuffer(OpenGLIndexBuffer* buffer, size_t bufferSizeInBytes);
	bool AllocateAsAliasForBuffer(OpenGLVertexBuffer* buffer, size_t bufferSizeInBytes);

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
	void CaptureDataBufferOutput();

	// Build state methods
	void MigrateBuildStateToDrawState();
	void CompletePendingDataWrites();
	void CompletePendingDataTransfers();
	GLuint GetTextureNo() const;
	GLenum GetTextureFormat() const;
	void AddAsCurrentBuffer();

private:
	// Structures
	struct PendingWrite
	{
		explicit PendingWrite(OpenGLTransferBatch* transferBatch)
		: transferBatch(transferBatch)
		{}

		size_t targetBufferPosInBytes = 0;
		std::vector<uint8_t> data;
		OpenGLTransferBatch* transferBatch;
	};
	struct PendingTransfer
	{
		explicit PendingTransfer(OpenGLTexelArray* targetBuffer, OpenGLTransferBatch* transferBatch)
		: targetBuffer(targetBuffer), transferBatch(transferBatch)
		{}

		OpenGLTexelArray* targetBuffer;
		OpenGLTransferBatch* transferBatch;
		size_t transferCountInBytes = 0;
		size_t sourceBufferPosInBytes = 0;
		size_t targetBufferPosInBytes = 0;
	};
	struct MutableState
	{
		std::vector<PendingWrite> pendingWrites;
		std::vector<PendingTransfer> pendingTransfers;
		std::vector<OpenGLTexelArrayOutput*> captureTargets;
	};

private:
	// Initialization methods
	void ReleaseMemory();
	bool AllocateAsAliasForBufferInternal(size_t bufferSizeInBytes);

	// Data conversion methods
	bool ConvertDataFormat(const void* sourceBuffer, size_t sourceBufferSizeInBytes, SourceImageFormat sourceImageFormat, SourceDataFormat sourceDataFormat, ImageFormat targetImageFormat, DataFormat targetDataFormat, std::vector<uint8_t>& targetBuffer) const;

	// Format methods
	constexpr static bool GetFormatNative(ImageFormat requestedImageFormat, DataFormat requestedDataFormat, ImageFormat& actualImageFormat, DataFormat& actualDataFormat, GLenum& nativeFormat);

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
	ImageFormat _imageFormat = {};
	DataFormat _dataFormat = {};
	GLenum _nativeFormat = {};
	bool _bufferCreated = false;
	GLenum _usageFlag = {};
	bool _nativeBufferCreationPending = false;
	GLuint _openGLBufferNo = 0;
	GLuint _textureNo = 0;
	bool _bufferIsAlias = false;
	OpenGLIndexBuffer* _bufferAliasSourceIndex = nullptr;
	OpenGLVertexBuffer* _bufferAliasSourceVertex = nullptr;
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
	std::mutex _buildStateMutex;
	std::atomic_flag _stateModified = ATOMIC_FLAG_INIT;
	uint32_t _buildIndex;
	uint32_t _drawIndex;
	MutableState _state[2];
};

} // namespace cobalt::graphics
