// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "OpenGLDataArray.h"
#include "OpenGLDataArrayOutput.h"
#include "OpenGLDebug.h"
#include "OpenGLRenderer.h"
#include "OpenGLTransferBatch.h"
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <Internal/RendererSupport/KnownDynamicCast.h>
#include <algorithm>
#include <cstring>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
OpenGLDataArray::OpenGLDataArray(cobalt::logging::ILogger* log, OpenGLRenderer* renderer)
: _log(log), _renderer(renderer), _buildIndex(0), _drawIndex(1)
{}

//----------------------------------------------------------------------------------------
OpenGLDataArray::~OpenGLDataArray()
{
	ReleaseMemory();
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void OpenGLDataArray::Delete()
{
	_renderer->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
SuccessToken OpenGLDataArray::AllocateMemory()
{
	// Ensure the array hasn't already been created
	if (_bufferCreated)
	{
		_log->Error("Attempted to allocate memory for a data array that has already been allocated.");
		return false;
	}

	// Ensure the buffer layout has been set
	if (!_bufferLayoutSet)
	{
		_log->Error("Attempted to allocate memory for a data array when the buffer layout has not been defined.");
		return false;
	}

	// Calculate the total size of the required buffer
	_totalBufferSizeInBytes = _structureEntryCount * _structureStrideInBytes;

	// Ensure that the buffer size is not zero
	if (_totalBufferSizeInBytes == 0)
	{
		_log->Error("Attempted to allocate an empty data array.");
		return false;
	}

	// Determine the usage flag to use when defining the OpenGL buffer
	PerformanceHint performanceHintCpu = _performanceHintCpu;
	GLenum usageFlag;
	switch (performanceHintCpu & PerformanceHint::WriteFlagsMask)
	{
	case PerformanceHint::WriteNever:
	case PerformanceHint::WriteRarely:
		usageFlag = GL_STATIC_DRAW;
		break;
	default:
	case PerformanceHint::Default:
		usageFlag = GL_DYNAMIC_DRAW;
		break;
	case PerformanceHint::WriteOften:
		usageFlag = GL_STREAM_DRAW;
		break;
	}
	_usageFlag = usageFlag;

	// Obtain any initial data for the buffer
	if (_initialDataSet && (_initialDataSizeInBytes != _totalBufferSizeInBytes))
	{
		_log->Error("Initial data array data of size {0} was provided, but {1} bytes are needed for the buffer.", _initialDataSizeInBytes, _totalBufferSizeInBytes);
		return false;
	}
	if (_initialDataSet)
	{
		_initialDataBuffer.resize(_totalBufferSizeInBytes);
		uint8_t* targetEntryPos = _initialDataBuffer.data();
		std::memcpy(targetEntryPos, _initialData, _totalBufferSizeInBytes);
		_initialDataSet = false;
		_initialData = nullptr;
	}

	// Flag that the buffer has been created
	_bufferCreated = true;
	_nativeBufferCreationPending = true;
	FlagBuildStateModified();
	return true;
}

//----------------------------------------------------------------------------------------
void OpenGLDataArray::ReleaseMemory()
{
	// Delete our created buffer object
	if (_bufferCreated && !_nativeBufferCreationPending)
	{
		glDeleteBuffers(1, &_openGLBufferNo);
		CheckGLError(_log);
	}
	_bufferCreated = false;
}

//----------------------------------------------------------------------------------------
void OpenGLDataArray::SetBufferLayout(size_t entryStrideInBytes, size_t entryCount, bool hasCounter, uint32_t counterResetValue)
{
	_structureEntryCount = entryCount;
	_structureStrideInBytes = entryStrideInBytes;
	_hasCounter = hasCounter;
	_counterResetValue = counterResetValue;
	_bufferLayoutSet = true;
}

//----------------------------------------------------------------------------------------
// Usage methods
//----------------------------------------------------------------------------------------
void OpenGLDataArray::SetUsageFlags(UsageFlags usageFlags)
{
	_usageFlags = usageFlags;
}

//----------------------------------------------------------------------------------------
void OpenGLDataArray::SetPerformanceHints(PerformanceHint performanceHintCpu, PerformanceHint performanceHintGpu)
{
	_performanceHintCpu = performanceHintCpu;
	_performanceHintGpu = performanceHintGpu;
}

//----------------------------------------------------------------------------------------
void OpenGLDataArray::SetDataPersistenceFlags(DataPersistenceFlags dataPersistenceFlags)
{
	_dataPersistenceFlags = dataPersistenceFlags;
}

//----------------------------------------------------------------------------------------
// Data methods
//----------------------------------------------------------------------------------------
SuccessToken OpenGLDataArray::SetInitialData(const void* sourceBuffer, size_t sourceBufferSizeInBytes)
{
	// Validate the current state of the buffer
	if (_bufferCreated)
	{
		_log->Error("Attempted to set initial data array data after the buffer has already been created.");
		return false;
	}
	if (_initialDataSet)
	{
		_log->Error("Attempted to set initial data array data when initial data has already been provided.");
		return false;
	}

	// Store the initial data for use when the buffer is allocated
	_initialData = reinterpret_cast<const uint8_t*>(sourceBuffer);
	_initialDataSizeInBytes = sourceBufferSizeInBytes;
	_initialDataSet = true;
	return true;
}

//----------------------------------------------------------------------------------------
SuccessToken OpenGLDataArray::QueueDataUpdate(const void* sourceBuffer, size_t sourceBufferSizeInBytes, size_t targetBufferOffsetInBytes, ITransferBatch* transferBatch)
{
	// Ensure the buffer is allowed to be modified
	if ((_performanceHintCpu & PerformanceHint::WriteFlagsMask) == PerformanceHint::WriteNever)
	{
		_log->Error("Attempted to update a data array that can't be modified.");
		return false;
	}

	// Verify that the requested write is within the bounds of the buffer
	if ((targetBufferOffsetInBytes > _totalBufferSizeInBytes) || ((targetBufferOffsetInBytes + sourceBufferSizeInBytes) > _totalBufferSizeInBytes))
	{
		_log->Error("Attempted to perform a write outside the bounds of a data array, with write location {0}, byte size {1}, against buffer size of {2}.", targetBufferOffsetInBytes, sourceBufferSizeInBytes, _totalBufferSizeInBytes);
		return false;
	}

	// If a transfer batch has been supplied, ensure it hasn't already been submitted.
	auto* transferBatchResolved = KnownDynamicCast<OpenGLTransferBatch*>(transferBatch);
	if ((transferBatchResolved != nullptr) && transferBatchResolved->IsSubmitted())
	{
		_log->Error("Attempted to queue a transfer using a transfer batch that has already been submitted");
		return false;
	}

	// Capture the supplied update settings and data
	const auto* sourceBufferAsByteArray = static_cast<const uint8_t*>(sourceBuffer);
	PendingWrite pendingWrite(transferBatchResolved);
	pendingWrite.targetBufferPos = targetBufferOffsetInBytes;
	pendingWrite.data.assign(sourceBufferAsByteArray, sourceBufferAsByteArray + sourceBufferSizeInBytes);

	// If a transfer batch has been supplied, increment the usage count.
	if (transferBatchResolved != nullptr)
	{
		transferBatchResolved->IncrementUsageCount();
	}

	// Queue a task to update the buffer with the supplied data
	std::unique_lock<std::mutex> lock(_buildStateMutex);
	_state[_buildIndex].pendingWrites.push_back(pendingWrite);
	lock.unlock();
	FlagBuildStateModified();
	return true;
}

//----------------------------------------------------------------------------------------
void OpenGLDataArray::UpdateCounterResetValue(uint32_t counterResetValue)
{
	// Ensure this data array has a counter attached
	if (!_hasCounter)
	{
		_log->Error("Attempted to update the counter reset value for a data array with no counter attached");
		return;
	}

	// Update the counter reset value to use next frame
	_state[_buildIndex].newCounterResetValue = counterResetValue;
	_state[_buildIndex].updatedCounterResetValue = true;
	FlagBuildStateModified();
}

//----------------------------------------------------------------------------------------
// Data transfer methods
//----------------------------------------------------------------------------------------
SuccessToken OpenGLDataArray::QueueDataTransfer(IDataArray* targetBuffer, size_t transferCountInBytes, size_t sourceBufferOffsetInBytes, size_t targetBufferOffsetInBytes, ITransferBatch* transferBatch)
{
	// Ensure the source and target buffers allow a transfer
	auto* targetBufferResolved = KnownDynamicCast<OpenGLDataArray*>(targetBuffer);
	if ((_performanceHintGpu & PerformanceHint::ReadFlagsMask) == PerformanceHint::ReadNever)
	{
		_log->Error("Attempted to transfer from a data array that can't be read on the GPU.");
		return false;
	}
	if ((targetBufferResolved->_performanceHintGpu & PerformanceHint::WriteFlagsMask) == PerformanceHint::WriteNever)
	{
		_log->Error("Attempted to transfer to a data array that can't be modified on the GPU.");
		return false;
	}
	if ((_usageFlags & UsageFlags::TransferSource) != UsageFlags::TransferSource)
	{
		_log->Error("Attempted to transfer from a data array that hasn't specified the TransferSource usage type.");
		return false;
	}
	if ((targetBufferResolved->_usageFlags & UsageFlags::TransferDestination) != UsageFlags::TransferDestination)
	{
		_log->Error("Attempted to transfer to a data array that hasn't specified the TransferDestination usage type.");
		return false;
	}

	// If a transfer batch has been supplied, ensure it hasn't already been submitted.
	auto* transferBatchResolved = KnownDynamicCast<OpenGLTransferBatch*>(transferBatch);
	if ((transferBatchResolved != nullptr) && transferBatchResolved->IsSubmitted())
	{
		_log->Error("Attempted to queue a transfer using a transfer batch that has already been submitted");
		return false;
	}

	// Capture the supplied update settings and data
	PendingTransfer pendingTransfer(targetBufferResolved, transferBatchResolved);
	pendingTransfer.transferCountInBytes = transferCountInBytes;
	pendingTransfer.sourceBufferPosInBytes = sourceBufferOffsetInBytes;
	pendingTransfer.targetBufferPosInBytes = targetBufferOffsetInBytes;

	// If a transfer batch has been supplied, increment the usage count.
	if (transferBatchResolved != nullptr)
	{
		transferBatchResolved->IncrementUsageCount();
	}

	// Queue a task to perform the data transfer
	std::unique_lock<std::mutex> lock(_buildStateMutex);
	_state[_buildIndex].pendingTransfers.push_back(pendingTransfer);
	lock.unlock();
	FlagBuildStateModified();
	return true;
}

//----------------------------------------------------------------------------------------
// Output capture methods
//----------------------------------------------------------------------------------------
void OpenGLDataArray::AddOutputCaptureTarget(IDataArrayOutput* captureTarget)
{
	std::unique_lock<std::mutex> lock(_accessMutex);
	_state[_buildIndex].captureTargets.push_back(KnownDynamicCast<OpenGLDataArrayOutput*>(captureTarget));
	lock.unlock();
	FlagBuildStateModified();
}

//----------------------------------------------------------------------------------------
void OpenGLDataArray::RemoveOutputCaptureTarget(IDataArrayOutput* captureTarget)
{
	std::unique_lock<std::mutex> lock(_accessMutex);
	auto* captureTargetResolved = KnownDynamicCast<OpenGLDataArrayOutput*>(captureTarget);
	for (auto captureTargetsIterator = _state[_buildIndex].captureTargets.begin(); captureTargetsIterator != _state[_buildIndex].captureTargets.end(); ++captureTargetsIterator)
	{
		if (*captureTargetsIterator == captureTargetResolved)
		{
			_state[_buildIndex].captureTargets.erase(captureTargetsIterator);
			lock.unlock();
			FlagBuildStateModified();
			return;
		}
	}
}

//----------------------------------------------------------------------------------------
void OpenGLDataArray::CaptureDataBufferOutput()
{
	// Reset our flag indicating this buffer has been added as a current resource buffer
	_addedAsCurrent = false;

	// If no capture targets are defined, abort any further processing.
	if (_state[_drawIndex].captureTargets.empty())
	{
		return;
	}

	// Read the counter value if a counter is present
	uint32_t counterValue = 0;
	if (_hasCounter)
	{
		CheckGLError(_log);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, _openGLCounterBufferNo);
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(counterValue), &counterValue);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		CheckGLError(_log);
	}

	// Store the buffer data and counter values into each attached capture target
	std::vector<unsigned char> bufferData;
	for (auto* captureTarget : _state[_drawIndex].captureTargets)
	{
		// Determine the position and size of the buffer region to capture
		size_t entryOffset = captureTarget->GetRequestedBufferOffset();
		size_t entryCountToCapture = captureTarget->GetRequestedEntryCount();
		entryCountToCapture = (entryCountToCapture == 0 ? _structureEntryCount : entryCountToCapture);
		entryCountToCapture = (((entryCountToCapture + entryOffset) > _structureEntryCount) ? (_structureEntryCount - entryOffset) : entryCountToCapture);
		size_t bufferStartPosInBytes = entryOffset * _structureStrideInBytes;

		// Retrieve the requested data from the buffer
		bufferData.resize(entryCountToCapture * _structureStrideInBytes);
		CheckGLError(_log);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, _openGLBufferNo);
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, bufferStartPosInBytes, bufferData.size(), bufferData.data());
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		CheckGLError(_log);

		// Store the requested capture data in the capture target
		bool detachAfterCapture = captureTarget->IsDetachingAfterCapture();
		captureTarget->StoreCaptureData(bufferData.data(), entryCountToCapture, _structureStrideInBytes, _hasCounter, counterValue);

		// Record this captured framebuffer output with the renderer
		_renderer->AddCurrentDataArrayOutput(captureTarget);

		// Now that we've captured a frame, detach the output capture target if requested.
		if (detachAfterCapture)
		{
			RemoveOutputCaptureTarget(captureTarget);
		}
	}
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
void OpenGLDataArray::MigrateBuildStateToDrawState()
{
	std::swap(_buildIndex, _drawIndex);
	_stateModified.clear(std::memory_order_relaxed);

	// If a new reset value has been supplied for the counter, update it now.
	if (_state[_drawIndex].updatedCounterResetValue)
	{
		_state[_drawIndex].updatedCounterResetValue = false;
		_counterResetValue = _state[_drawIndex].newCounterResetValue;
	}
}

//----------------------------------------------------------------------------------------
void OpenGLDataArray::CreateNativeBuffer()
{
	// Retrieve the initial data buffer if required
	const uint8_t* initialData = nullptr;
	if (!_initialDataBuffer.empty())
	{
		initialData = _initialDataBuffer.data();
	}

	// Allocate the data array
	CheckGLError(_log);
	glGenBuffers(1, &_openGLBufferNo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, _openGLBufferNo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, _totalBufferSizeInBytes, initialData, _usageFlag);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	CheckGLError(_log);

	// Allocate the counter buffer if required
	if (_hasCounter)
	{
		CheckGLError(_log);
		glGenBuffers(1, &_openGLCounterBufferNo);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, _openGLCounterBufferNo);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(_counterResetValue), &_counterResetValue, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		CheckGLError(_log);
	}

	// Release any resources related to the initial data. We don't use clear() and shrink_to_fit() here because this
	// data could be very large, and shrink_to_fit() isn't guaranteed to do anything. This approach is guaranteed to do
	// what we want, which is actually release the allocated memory for this buffer, since we won't need it again.
	std::vector<unsigned char>().swap(_initialDataBuffer);
	_initialDataBuffer = std::vector<unsigned char>();
}

//----------------------------------------------------------------------------------------
void OpenGLDataArray::CompletePendingDataWrites()
{
	// Create the native buffer if its creation is pending
	if (_nativeBufferCreationPending)
	{
		CreateNativeBuffer();
		_nativeBufferCreationPending = false;
	}

	// Obtain the set of pending data writes. If no writes are pending, abort any further processing.
	std::vector<PendingWrite>& pendingWrites = _state[_drawIndex].pendingWrites;
	if (pendingWrites.empty())
	{
		return;
	}

	// Split pending writes into those that are ready to run now, and those that must remain queued until their batch
	// has been submitted.
	std::vector<PendingWrite> readyWrites;
	std::vector<PendingWrite> deferredWrites;
	readyWrites.reserve(pendingWrites.size());
	deferredWrites.reserve(pendingWrites.size());
	for (PendingWrite& write : pendingWrites)
	{
		if ((write.transferBatch != nullptr) && !write.transferBatch->IsSubmitted())
		{
			deferredWrites.push_back(std::move(write));
			continue;
		}
		readyWrites.push_back(std::move(write));
	}
	pendingWrites.clear();

	// Re-queue any deferred writes onto the build state so they remain pending for a later frame.
	if (!deferredWrites.empty())
	{
		std::unique_lock<std::mutex> lock(_buildStateMutex);
		auto& buildPendingWrites = _state[_buildIndex].pendingWrites;
		for (PendingWrite& write : deferredWrites)
		{
			buildPendingWrites.push_back(std::move(write));
		}
		lock.unlock();
		FlagBuildStateModified();
	}
	if (readyWrites.empty())
	{
		return;
	}

	// Complete any pending writes we need to perform in the buffer
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, _openGLBufferNo);
	CheckGLError(_log);
	for (const PendingWrite& write : readyWrites)
	{
		CompletePendingDataWrite(write);
	}
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	CheckGLError(_log);
}

//----------------------------------------------------------------------------------------
void OpenGLDataArray::CompletePendingDataWrite(const PendingWrite& pendingWrite)
{
	// Calculate the region of the buffer being modified
	const uint8_t* data = pendingWrite.data.data();
	auto dataSizeInBytes = pendingWrite.data.size();
	auto bufferStartPosInBytes = pendingWrite.targetBufferPos;

	// Update the target region of the buffer
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, bufferStartPosInBytes, dataSizeInBytes, reinterpret_cast<const GLvoid*>(data));

	// If a transfer batch has been supplied, decrement the usage count.
	if (pendingWrite.transferBatch != nullptr)
	{
		pendingWrite.transferBatch->DecrementUsageCount();
	}
}

//----------------------------------------------------------------------------------------
void OpenGLDataArray::CompletePendingDataTransfers()
{
	// Create the native buffer if its creation is pending
	if (_nativeBufferCreationPending)
	{
		CreateNativeBuffer();
		_nativeBufferCreationPending = false;
	}

	// Obtain the set of pending data transfers. If no transfers are pending, abort any further processing.
	std::vector<PendingTransfer>& pendingTransfers = _state[_drawIndex].pendingTransfers;
	if (pendingTransfers.empty())
	{
		return;
	}

	// Split pending transfers into those that are ready to run now, and those that must remain queued until their
	// batch has been submitted.
	std::vector<PendingTransfer> readyTransfers;
	std::vector<PendingTransfer> deferredTransfers;
	readyTransfers.reserve(pendingTransfers.size());
	deferredTransfers.reserve(pendingTransfers.size());
	for (PendingTransfer& transfer : pendingTransfers)
	{
		if ((transfer.transferBatch != nullptr) && !transfer.transferBatch->IsSubmitted())
		{
			deferredTransfers.push_back(transfer);
			continue;
		}
		readyTransfers.push_back(transfer);
	}
	pendingTransfers.clear();

	// Re-queue any deferred transfers onto the build state so they remain pending for a later frame.
	if (!deferredTransfers.empty())
	{
		std::unique_lock<std::mutex> lock(_buildStateMutex);
		auto& buildPendingTransfers = _state[_buildIndex].pendingTransfers;
		for (PendingTransfer& transfer : deferredTransfers)
		{
			buildPendingTransfers.push_back(transfer);
		}
		lock.unlock();
		FlagBuildStateModified();
	}
	if (readyTransfers.empty())
	{
		return;
	}

	// Complete any pending transfers we need to perform from the buffer
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, _openGLBufferNo);
	CheckGLError(_log);
	for (const PendingTransfer& transfer : readyTransfers)
	{
		CompletePendingDataTransfer(transfer);
	}
	glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	CheckGLError(_log);
}

//----------------------------------------------------------------------------------------
void OpenGLDataArray::CompletePendingDataTransfer(const PendingTransfer& pendingTransfer)
{
	// Bind the target buffer to the write target
	glBindBuffer(GL_COPY_WRITE_BUFFER, pendingTransfer.targetBuffer->_openGLBufferNo);

	// Update the target region of the buffer
	glCopyBufferSubData(GL_SHADER_STORAGE_BUFFER, GL_COPY_WRITE_BUFFER, GLintptr(pendingTransfer.sourceBufferPosInBytes), GLintptr(pendingTransfer.targetBufferPosInBytes), GLsizeiptr(pendingTransfer.transferCountInBytes));

	// If a transfer batch has been supplied, decrement the usage count.
	if (pendingTransfer.transferBatch != nullptr)
	{
		pendingTransfer.transferBatch->DecrementUsageCount();
	}
}

//----------------------------------------------------------------------------------------
void OpenGLDataArray::FlagBuildStateModified()
{
	if (!_stateModified.test_and_set(std::memory_order_acquire))
	{
		_renderer->FlagObjectModified(this);
	}
}

//----------------------------------------------------------------------------------------
bool OpenGLDataArray::HasCounter() const
{
	return _hasCounter;
}

//----------------------------------------------------------------------------------------
void OpenGLDataArray::ResetCounter()
{
	//##FIX## Profile performance of this approach. Reportedly it might be better to use glInvalidateBufferData followed
	//by glClearBufferData, but this is old information and should be tested. See the following:
	//https://community.khronos.org/t/fastest-way-to-reset-an-atomic-counter/72804/2
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, _openGLCounterBufferNo);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(_counterResetValue), &_counterResetValue);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

//----------------------------------------------------------------------------------------
GLuint OpenGLDataArray::GetBufferNo() const
{
	return _openGLBufferNo;
}

//----------------------------------------------------------------------------------------
GLuint OpenGLDataArray::GetCounterBufferNo() const
{
	return _openGLCounterBufferNo;
}

//----------------------------------------------------------------------------------------
void OpenGLDataArray::AddAsCurrentBuffer()
{
	if (!_addedAsCurrent)
	{
		_renderer->AddCurrentDataArray(this);
		_addedAsCurrent = true;
	}
}

//----------------------------------------------------------------------------------------
uint32_t OpenGLDataArray::GetCurrentCounterValue()
{
	// Read the current counter value
	CheckGLError(_log);
	uint32_t counterValue;
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, _openGLCounterBufferNo);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(counterValue), &counterValue);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	CheckGLError(_log);
	return counterValue;
}

//----------------------------------------------------------------------------------------
void OpenGLDataArray::GetCurrentBufferData(size_t bufferOffsetInBytes, void* targetBuffer, size_t byteCount)
{
	// Retrieve the requested data from the buffer
	CheckGLError(_log);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, _openGLBufferNo);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, bufferOffsetInBytes, byteCount, targetBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	CheckGLError(_log);
}

} // namespace cobalt::graphics
