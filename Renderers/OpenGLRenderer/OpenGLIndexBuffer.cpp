// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "OpenGLIndexBuffer.h"
#include "OpenGLDebug.h"
#include "OpenGLRenderer.h"
#include "OpenGLTransferBatch.h"
#ifdef GL_VERSION_4_3
#include "OpenGLTexelArray.h"
#endif
#include <Internal/RendererSupport/KnownDynamicCast.h>
#include <algorithm>
#include <cstring>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
OpenGLIndexBuffer::OpenGLIndexBuffer(cobalt::logging::ILogger* log, OpenGLRenderer* renderer)
: _log(log), _renderer(renderer), _buildIndex(0), _drawIndex(1)
{}

//----------------------------------------------------------------------------------------
OpenGLIndexBuffer::~OpenGLIndexBuffer()
{
	ReleaseMemory();
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void OpenGLIndexBuffer::Delete()
{
	_renderer->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
SuccessToken OpenGLIndexBuffer::AllocateMemory()
{
	return AllocateMemoryInternal(nullptr);
}

//----------------------------------------------------------------------------------------
SuccessToken OpenGLIndexBuffer::AllocateMemoryWithAlias(ITexelArray* texelArray)
{
#ifdef GL_VERSION_4_3
	auto* texelArrayResolved = KnownDynamicCast<OpenGLTexelArray*>(texelArray);
	if (texelArrayResolved == nullptr)
	{
		_log->Error("Attempted to allocate memory for an index buffer with an alias, where no alias object was supplied.");
		return false;
	}
	return AllocateMemoryInternal(texelArrayResolved);
#else
	return false;
#endif
}

//----------------------------------------------------------------------------------------
bool OpenGLIndexBuffer::AllocateMemoryInternal(OpenGLTexelArray* texelArray)
{
	// Ensure the array hasn't already been created
	if (_indexBufferCreated)
	{
		_log->Error("Attempted to allocate memory for an index buffer that has already been allocated.");
		return false;
	}
	if (!_indexAttributeAdded)
	{
		_log->Error("Attempted to allocate memory for an index buffer that contains no index attributes.");
		return false;
	}

	// Ensure that a manual memory layout has been defined if an alias is being used
	bool aliasAsTexelArray = (texelArray != nullptr);
	if (aliasAsTexelArray && !_manualBufferLayout)
	{
		_log->Error("Attempted to allocate memory for an index buffer with an alias, when a manual buffer layout was not defined. A manual layout must be used if an alias is present, or the memory structure cannot be known, and therefore cannot be shared in a well defined manner.");
		return false;
	}

	// Calculate the total size of the required buffer
	_totalBufferSizeInBytes = _indexAttributeInfo.dataTypeByteSize * _indexAttributeInfo.indexCount;

	// Determine the usage flag to use when defining the OpenGL buffer
	IIndexAttribute::PerformanceHint performanceHintCpu = _indexAttributeInfo.performanceHintCpu;
	GLenum usageFlag;
	switch (performanceHintCpu & IIndexAttribute::PerformanceHint::WriteFlagsMask)
	{
	case IIndexAttribute::PerformanceHint::WriteNever:
	case IIndexAttribute::PerformanceHint::WriteRarely:
		usageFlag = GL_STATIC_DRAW;
		break;
	default:
	case IIndexAttribute::PerformanceHint::Default:
		usageFlag = GL_DYNAMIC_DRAW;
		break;
	case IIndexAttribute::PerformanceHint::WriteOften:
		usageFlag = GL_STREAM_DRAW;
		break;
	}
	_usageFlag = usageFlag;

	// Obtain any initial data for the buffer
	if (_initialDataSet)
	{
		_initialDataBuffer.resize(_totalBufferSizeInBytes);
		if (_initialDataEntrySizeInBytes == 1)
		{
			if (_initialDataEntryCount != _totalBufferSizeInBytes)
			{
				_log->Error("Raw initial index data of size {0} was provided, but {1} bytes are needed for the buffer.", _initialDataEntryCount, _totalBufferSizeInBytes);
				return false;
			}
			uint8_t* targetEntryPos = _initialDataBuffer.data();
			std::memcpy(targetEntryPos, _initialData, _totalBufferSizeInBytes);
		}
		else if (_initialDataEntryStrideInBytes == _initialDataEntrySizeInBytes)
		{
			uint8_t* targetEntryPos = _initialDataBuffer.data();
			std::memcpy(targetEntryPos, _initialData, _totalBufferSizeInBytes);
		}
		else
		{
			size_t entryCount = _initialDataEntryCount;
			const uint8_t* sourceEntryPos = _initialData;
			uint8_t* targetEntryPos = _initialDataBuffer.data();
			while (entryCount > 0)
			{
				std::memcpy(targetEntryPos, sourceEntryPos, _initialDataEntrySizeInBytes);
				targetEntryPos += _initialDataEntrySizeInBytes;
				sourceEntryPos += _initialDataEntryStrideInBytes;
				--entryCount;
			}
		}
		_initialDataSet = false;
		_initialData = nullptr;
	}

#ifdef GL_VERSION_4_3
	// Add an alias for this index buffer if requested
	if (aliasAsTexelArray && !texelArray->AllocateAsAliasForBuffer(this, _totalBufferSizeInBytes))
	{
		_log->Error("Failed to allocate alias for index buffer.");
		return false;
	}
#endif

	// Flag that the array has been created
	_indexBufferCreated = true;
	_nativeBufferCreationPending = true;
	FlagBuildStateModified();
	return true;
}

//----------------------------------------------------------------------------------------
void OpenGLIndexBuffer::ReleaseMemory()
{
	// Delete our created buffer object
	if (_indexBufferCreated && !_nativeBufferCreationPending)
	{
		glDeleteBuffers(1, &_openGLBufferNo);
		CheckGLError(_log);
	}
	_indexBufferCreated = false;
}

//----------------------------------------------------------------------------------------
bool OpenGLIndexBuffer::IsAllocated() const
{
	return _indexBufferCreated;
}

//----------------------------------------------------------------------------------------
// Binding methods
//----------------------------------------------------------------------------------------
SuccessToken OpenGLIndexBuffer::BindIndexAttribute(IIndexAttribute& indexAttribute)
{
	// Verify that the array hasn't already been created
	if (_indexBufferCreated)
	{
		_log->Error("Attempted to bind a new index attribute after the index buffer has already been created.");
		return false;
	}

	// Verify that an index attribute hasn't already been bound
	if (_indexAttributeAdded)
	{
		_log->Error("Attempted to bind a new index attribute to a buffer which already has an index attribute defined. Index buffers currently only support a single index attribute.");
		return false;
	}

	// Verify that there's at least one entry defined in the supplied attribute
	if (indexAttribute.GetIndexCount() == 0)
	{
		_log->Error("Attempted to bind an index attribute with an index count of zero.");
		return false;
	}

	// Verify that the supplied attribute hasn't already been bound to a buffer
	if (indexAttribute.IsBoundToBuffer())
	{
		_log->Error("Attempted to bind an index attribute which has already been bound to another buffer.");
		return false;
	}

	// Bind the index attribute to the array
	AttachIndexAttributeToThisArray(indexAttribute, 0);

	// Define the attribute info
	IndexAttributeInfo attributeInfoEntry = {};
	attributeInfoEntry.dataType = indexAttribute.GetDataType();
	attributeInfoEntry.dataTypeByteSize = IIndexAttribute::GetDataTypeByteSize(attributeInfoEntry.dataType);
	attributeInfoEntry.indexCount = indexAttribute.GetIndexCount();
	attributeInfoEntry.performanceHintCpu = indexAttribute.GetPerformanceHintCpu();
	attributeInfoEntry.performanceHintGpu = indexAttribute.GetPerformanceHintGpu();
	attributeInfoEntry.bufferOffsetInBytes = 0;
	attributeInfoEntry.bufferStartPosInBytes = 0;
	attributeInfoEntry.setMinMaxIndexValue = false;
	_indexAttributeInfo = attributeInfoEntry;
	_indexAttributeAdded = true;
	return true;
}

//----------------------------------------------------------------------------------------
SuccessToken OpenGLIndexBuffer::BindIndexAttributeManualLayout(IIndexAttribute& indexAttribute, size_t bufferOffsetInBytes, size_t bufferStrideInBytes)
{
	// Ensure the supplied index attribute is being bound at offset 0, tightly packed.
	auto dataType = indexAttribute.GetDataType();
	auto calculatedStrideInBytes = IIndexAttribute::GetDataTypeByteSize(dataType);
	if ((bufferOffsetInBytes != 0) || (bufferStrideInBytes != calculatedStrideInBytes))
	{
		_log->Error("Attempted to bind an index attribute with offset {0} and stride {1}, but index buffers only currently support an offset of 0 and a tightly packed stride.", bufferOffsetInBytes, bufferStrideInBytes);
		return false;
	}

	// Bind the index attribute
	if (!BindIndexAttribute(indexAttribute))
	{
		return false;
	}
	_manualBufferLayout = true;
	return true;
}

//----------------------------------------------------------------------------------------
const OpenGLIndexBuffer::IndexAttributeInfo* OpenGLIndexBuffer::GetIndexAttributeInfo(size_t attributeIndex) const
{
	ASSERT(attributeIndex == 0);
	return &_indexAttributeInfo;
}

//----------------------------------------------------------------------------------------
// Data methods
//----------------------------------------------------------------------------------------
SuccessToken OpenGLIndexBuffer::SetInitialData(const uint8_t* data, size_t entryCount, size_t entryStrideInBytes)
{
	// Validate the current state of the buffer
	if (_indexBufferCreated)
	{
		_log->Error("Attempted to set initial index data after the buffer has already been created.");
		return false;
	}
	if (_indexAttributeInfo.indexCount != entryCount)
	{
		_log->Error("Attempted to set initial index data with index count {0}, which is different to the defined index count of {1}.", entryCount, _indexAttributeInfo.indexCount);
		return false;
	}
	if (_initialDataSet)
	{
		_log->Error("Attempted to set initial index data when initial data has already been provided.");
		return false;
	}

	// Update the known minimum and maximum index values. We do this to allow us to use glDrawRangeElements, for a
	// potential performance boost when rendering.
	UpdateMinMaxIndexValueForNewData(_indexAttributeInfo, data, entryCount, entryStrideInBytes);

	// Store the initial data for use when the buffer is allocated
	_initialData = data;
	_initialDataEntryCount = entryCount;
	_initialDataEntryStrideInBytes = entryStrideInBytes;
	_initialDataEntrySizeInBytes = _indexAttributeInfo.dataTypeByteSize;
	_initialDataSet = true;
	return true;
}

//----------------------------------------------------------------------------------------
SuccessToken OpenGLIndexBuffer::SetRawInitialData(const uint8_t* data, size_t dataSizeInBytes)
{
	// Validate the current state of the buffer
	if (_indexBufferCreated)
	{
		_log->Error("Attempted to set initial raw index buffer data after the buffer has already been created.");
		return false;
	}
	if (_initialDataSet)
	{
		_log->Error("Attempted to set initial raw index buffer data when initial data has already been provided.");
		return false;
	}
	if (!_manualBufferLayout)
	{
		_log->Error("Attempted to set initial raw index buffer data when manual layout index attributes have not been bound.");
		return false;
	}

	// Store the initial data for use when the buffer is allocated
	_initialData = data;
	_initialDataEntryCount = dataSizeInBytes;
	_initialDataEntryStrideInBytes = 1;
	_initialDataEntrySizeInBytes = 1;
	_initialDataSet = true;
	return true;
}

//----------------------------------------------------------------------------------------
SuccessToken OpenGLIndexBuffer::QueueDataUpdate(const uint8_t* data, size_t entryCount, size_t initialIndexNo, size_t entryStrideInBytes, ITransferBatch* transferBatch)
{
	// Ensure the target attribute is allowed to be modified
	IndexAttributeInfo& info = _indexAttributeInfo;
	if ((info.performanceHintCpu & IIndexAttribute::PerformanceHint::WriteFlagsMask) == IIndexAttribute::PerformanceHint::WriteNever)
	{
		_log->Error("Attempted to update an index attribute that can't be modified.");
		return false;
	}

	// Ensure the write region is within the bounds of the buffer
	if ((initialIndexNo >= info.indexCount) || ((initialIndexNo + entryCount) > info.indexCount))
	{
		_log->Error("Attempted index attribute write at index {0} for {1} entries, when only {2} entries are present.", initialIndexNo, entryCount, info.indexCount);
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
	size_t entrySizeInBytes = info.dataTypeByteSize;
	size_t totalDataSize = (entryCount > 0) ? entrySizeInBytes + ((entryCount - 1) * entryStrideInBytes) : 0;
	PendingWrite pendingWrite(info, entryCount, initialIndexNo, entryStrideInBytes, transferBatchResolved);
	pendingWrite.data.assign(data, data + totalDataSize);

	// If a transfer batch has been supplied, increment the usage count.
	if (transferBatchResolved != nullptr)
	{
		transferBatchResolved->IncrementUsageCount();
	}

	// Queue a task to update the buffer with the supplied data
	std::unique_lock<std::mutex> lock(_buildStateMutex);
	_state[_buildIndex].pendingWrites.push_back(std::move(pendingWrite));
	lock.unlock();
	FlagBuildStateModified();
	return true;
}

//----------------------------------------------------------------------------------------
SuccessToken OpenGLIndexBuffer::QueueRawDataUpdate(const uint8_t* data, size_t dataSizeInBytes, size_t bufferOffsetInBytes, ITransferBatch* transferBatch)
{
	// Validate the current state of the buffer
	if (!_indexBufferCreated)
	{
		_log->Error("Attempted to queue a raw data update before the index buffer has been allocated.");
		return false;
	}
	if (!_manualBufferLayout)
	{
		_log->Error("Attempted to queue a raw data update when manual layout index attributes have not been bound.");
		return false;
	}

	// Verify that the requested write is within the bounds of the buffer
	if ((bufferOffsetInBytes > _totalBufferSizeInBytes) || ((bufferOffsetInBytes + dataSizeInBytes) > _totalBufferSizeInBytes))
	{
		_log->Error("Attempted to perform a raw write outside the bounds of an index buffer, with write location {0}, byte size {1}, against buffer size of {2}.", bufferOffsetInBytes, dataSizeInBytes, _totalBufferSizeInBytes);
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
	PendingWrite pendingWrite(_indexAttributeInfo, bufferOffsetInBytes, dataSizeInBytes, transferBatchResolved);
	pendingWrite.data.assign(data, data + dataSizeInBytes);

	// If a transfer batch has been supplied, increment the usage count.
	if (transferBatchResolved != nullptr)
	{
		transferBatchResolved->IncrementUsageCount();
	}

	// Queue a task to update the buffer with the supplied data
	std::unique_lock<std::mutex> lock(_buildStateMutex);
	_state[_buildIndex].pendingWrites.push_back(std::move(pendingWrite));
	lock.unlock();
	FlagBuildStateModified();
	return true;
}

//----------------------------------------------------------------------------------------
void OpenGLIndexBuffer::UpdateMinMaxIndexValueForNewData(IndexAttributeInfo& info, const uint8_t* data, size_t entryCount, size_t entryStrideInBytes)
{
	// Disable cast alignment warnings. We're just restoring the pointer type from a byte pointer here, so we expect the
	// caller is already respecting alignment requirements for this platform.
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-align"
#endif

	// Update the known minimum and maximum index values. We do this to allow us to use glDrawRangeElements, for a
	// potential performance boost when rendering.
	if (info.dataTypeByteSize == 2)
	{
		for (size_t i = 0; i < entryCount; ++i)
		{
			auto indexValue = (uint32_t)(*reinterpret_cast<const uint16_t*>((data + (i * entryStrideInBytes))));
			info.minIndexValue = info.setMinMaxIndexValue ? std::min(info.minIndexValue, indexValue) : indexValue;
			info.maxIndexValue = info.setMinMaxIndexValue ? std::max(info.maxIndexValue, indexValue) : indexValue;
			info.setMinMaxIndexValue = true;
		}
	}
	else if (info.dataTypeByteSize == 4)
	{
		for (size_t i = 0; i < entryCount; ++i)
		{
			uint32_t indexValue = *reinterpret_cast<const uint32_t*>(data + (i * entryStrideInBytes));
			info.minIndexValue = info.setMinMaxIndexValue ? std::min(info.minIndexValue, indexValue) : indexValue;
			info.maxIndexValue = info.setMinMaxIndexValue ? std::max(info.maxIndexValue, indexValue) : indexValue;
			info.setMinMaxIndexValue = true;
		}
	}

	// Restore the disabled warnings
#ifdef __clang__
#pragma clang diagnostic pop
#endif
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
void OpenGLIndexBuffer::MigrateBuildStateToDrawState()
{
	std::swap(_buildIndex, _drawIndex);
	_stateModified.clear(std::memory_order_relaxed);
}

//----------------------------------------------------------------------------------------
void OpenGLIndexBuffer::CreateNativeBuffer()
{
	// Calculate the total size of the required buffer
	auto totalBufferSize = _indexAttributeInfo.dataTypeByteSize * _indexAttributeInfo.indexCount;

	// Retrieve the initial data buffer if required
	const uint8_t* initialData = nullptr;
	if (!_initialDataBuffer.empty())
	{
		initialData = _initialDataBuffer.data();
	}

	// Allocate the index buffer
	glGenBuffers(1, &_openGLBufferNo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _openGLBufferNo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, totalBufferSize, initialData, _usageFlag);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	CheckGLError(_log);

	// Release any resources related to the initial data. We don't use clear() and shrink_to_fit() here because this
	// data could be very large, and shrink_to_fit() isn't guaranteed to do anything. This approach is guaranteed to do
	// what we want, which is actually release the allocated memory for this buffer, since we won't need it again.
	std::vector<unsigned char>().swap(_initialDataBuffer);
	_initialDataBuffer = std::vector<unsigned char>();
}

//----------------------------------------------------------------------------------------
void OpenGLIndexBuffer::CompletePendingDataWrites()
{
	// Create the native buffer if its creation is pending
	if (_nativeBufferCreationPending)
	{
		CreateNativeBuffer();
		_nativeBufferCreationPending = false;
	}

	// Complete any pending writes we need to perform in the buffer
	std::vector<PendingWrite>& pendingWrites = _state[_drawIndex].pendingWrites;
	if (!pendingWrites.empty())
	{
		// Split pending writes into those that are ready to run now, and those that must remain queued until their
		// batch has been submitted.
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

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _openGLBufferNo);
		CheckGLError(_log);

		for (const PendingWrite& write : readyWrites)
		{
			CompletePendingDataWrite(write);
		}

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		CheckGLError(_log);
	}
}

//----------------------------------------------------------------------------------------
bool OpenGLIndexBuffer::CompletePendingDataWrite(const PendingWrite& pendingWrite)
{
	IndexAttributeInfo& info = pendingWrite.attributeInfo;
	size_t entryCount = pendingWrite.entryCount;
	size_t initialIndexNo = pendingWrite.initialIndexNo;
	size_t entryStrideInBytes = pendingWrite.entryStrideInBytes;
	const uint8_t* data = pendingWrite.data.data();

	// Update the known minimum and maximum index values. We do this to allow us to use glDrawRangeElements, for a
	// potential performance boost when rendering.
	UpdateMinMaxIndexValueForNewData(info, data, entryCount, entryStrideInBytes);

	// Calculate the region of the buffer being modified
	auto entrySizeInBytes = info.dataTypeByteSize;
	auto bufferStartPosInBytes = info.bufferOffsetInBytes + (initialIndexNo * entrySizeInBytes);
	if (pendingWrite.rawDataWrite)
	{
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, pendingWrite.initialIndexNo, pendingWrite.entryCount, reinterpret_cast<const GLvoid*>(data));
	}
	else if (entryStrideInBytes == info.dataTypeByteSize)
	{
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, bufferStartPosInBytes, entryCount * entrySizeInBytes, reinterpret_cast<const GLvoid*>(data));
	}
	else
	{
		auto* mappedMemory = reinterpret_cast<unsigned char*>(glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, bufferStartPosInBytes, entryCount * entrySizeInBytes, GL_MAP_WRITE_BIT));
		if (mappedMemory == nullptr)
		{
			_log->Error("glMapBufferRange failed with error code {0}", glGetError());
			return false;
		}

		while (entryCount > 0)
		{
			std::memcpy(mappedMemory, data, entrySizeInBytes);
			mappedMemory += entrySizeInBytes;
			data += entryStrideInBytes;
			--entryCount;
		}

		glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
	}
	CheckGLError(_log);

	// If a transfer batch has been supplied, decrement the usage count.
	if (pendingWrite.transferBatch != nullptr)
	{
		pendingWrite.transferBatch->DecrementUsageCount();
	}
	return true;
}

//----------------------------------------------------------------------------------------
void OpenGLIndexBuffer::FlagBuildStateModified()
{
	if (!_stateModified.test_and_set(std::memory_order_acquire))
	{
		_renderer->FlagObjectModified(this);
	}
}

//----------------------------------------------------------------------------------------
GLuint OpenGLIndexBuffer::GetOpenGLBufferNo() const
{
	return _openGLBufferNo;
}

} // namespace cobalt::graphics
