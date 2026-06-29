// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "VulkanTexelArray.h"
#include "VulkanRenderer.h"
#include "VulkanTexelArrayOutput.h"
#include "VulkanTransferBatch.h"
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <Internal/TextureFormatConversion/TextureFormatConversion.pkg>
#include <Internal/RendererSupport/KnownDynamicCast.h>
#include <algorithm>
#include <cstring>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
VulkanTexelArray::VulkanTexelArray(cobalt::logging::ILogger* log, VulkanRenderer* renderer)
: _log(log), _renderer(renderer), _buildIndex(0), _drawIndex(1)
{}

//----------------------------------------------------------------------------------------
VulkanTexelArray::~VulkanTexelArray()
{
	ReleaseMemory();
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void VulkanTexelArray::Delete()
{
	_renderer->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
SuccessToken VulkanTexelArray::AllocateMemory()
{
	// Ensure the array hasn't already been created
	if (_bufferCreated)
	{
		_log->Error("Attempted to allocate memory for a texel array that has already been allocated.");
		return false;
	}

	// Ensure the buffer layout has been set
	if (!_bufferLayoutSet)
	{
		_log->Error("Attempted to allocate memory for a texel array when the buffer layout has not been defined.");
		return false;
	}

	// Calculate the total size of the required buffer
	_totalBufferSizeInBytes = _structureEntryCount * _structureStrideInBytes;

	// Ensure that the buffer size is not zero
	if (_totalBufferSizeInBytes == 0)
	{
		_log->Error("Attempted to allocate an empty texel array.");
		return false;
	}

	// Calculate the internal data format to use
	if (!GetFormatNative(_imageFormat, _dataFormat, _nativeFormat))
	{
		_log->Error("Failed to allocate memory for texel array. The combination of image format {0} and data format {1} is not supported by this renderer.", _imageFormat, _dataFormat);
		return false;
	}

	// Create the data array
	VkBufferUsageFlags usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
	VulkanMemoryManager* memoryManager = _renderer->GetMemoryManager();
	memoryManager->CreateBuffer(_totalBufferSizeInBytes, usageFlags, VMA_MEMORY_USAGE_GPU_ONLY, 0, _buffer, _bufferAllocation);

	// If initial data has been provided, stage an upload of that data into the buffer.
	if (_initialDataSet && (_initialDataSizeInBytes != _totalBufferSizeInBytes))
	{
		_log->Error("Initial texel array data of size {0} was provided, but {1} bytes are needed for the buffer.", _initialDataSizeInBytes, _totalBufferSizeInBytes);
		return false;
	}
	if (_initialDataSet)
	{
		// Obtain any initial data for the buffer, performing image format conversion if required.
		bool convertInitialData = !ImageFormatsAreBinaryEquivalent(_initialDataImageFormat, _imageFormat) || !DataFormatsAreBinaryEquivalent(_initialDataDataFormat, _dataFormat);
		if (convertInitialData && !ConvertDataFormat(_initialData, _initialDataSizeInBytes, _initialDataImageFormat, _initialDataDataFormat, _imageFormat, _dataFormat, _initialDataBuffer))
		{
			_log->Error("AllocateMemory failed to convert initial data for texel array");
			return false;
		}
		const uint8_t* initialData = (convertInitialData ? _initialDataBuffer.data() : _initialData);
		size_t initialDataSizeInBytes = (convertInitialData ? _initialDataBuffer.size() : _initialDataSizeInBytes);

		// Create an upload buffer for the data
		VkBuffer uploadBuffer;
		VmaAllocation uploadBufferAllocation;
		_renderer->CreateTemporaryUploadBuffer(initialDataSizeInBytes, uploadBuffer, uploadBufferAllocation, false);

		// Map the upload buffer into the CPU address space
		uint8_t* uploadBufferMapped;
		if (!memoryManager->MapBufferMemory(uploadBufferAllocation, uploadBufferMapped))
		{
			_log->Error("Failed to map upload buffer for texel array");
		}

		// Write the initial data into the upload buffer
		std::memcpy(uploadBufferMapped, initialData, initialDataSizeInBytes);

		// Unmap the upload buffer
		memoryManager->UnmapBufferMemory(uploadBufferAllocation);

		// Allocate a new command buffer on the build queue to handle the initial data transfer
		VkCommandBuffer commandBuffer = _renderer->GetBuildCommandBuffer();

		// Schedule a data transfer from the upload buffer to the target buffer
		VkBufferCopy copyRegion = {};
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = 0;
		copyRegion.size = _initialDataSizeInBytes;
		vkCmdCopyBuffer(commandBuffer, uploadBuffer, _buffer, 1, &copyRegion);

		// If the initial upload ran on a dedicated transfer queue, release queue ownership back to graphics now.
		if (!_renderer->IsTransferQueueSharedWithGraphics())
		{
			// Perform a release operation on the transfer queue for the buffer
			VkBufferMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = 0;
			barrier.srcQueueFamilyIndex = _renderer->GetTransferQueueFamily();
			barrier.dstQueueFamilyIndex = _renderer->GetGraphicsQueueFamily();
			barrier.buffer = _buffer;
			barrier.offset = 0;
			barrier.size = _totalBufferSizeInBytes;
			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);

			// Schedule an acquire operation on the graphics queue for the buffer
			_renderer->ScheduleGraphicsQueueAcquireOperation(this);
		}

		// Submit the command buffer
		_renderer->SubmitBuildCommandBuffer(commandBuffer);
	}

	// Release any resources related to the initial data
	_initialDataSet = false;
	_initialData = nullptr;

	// Flag that the buffer has been created
	_bufferCreated = true;
	FlagBuildStateModified();
	return true;
}

//----------------------------------------------------------------------------------------
bool VulkanTexelArray::AllocateAsAliasForBuffer(VkBuffer buffer, size_t bufferSizeInBytes)
{
	// Ensure the array hasn't already been created
	if (_bufferCreated)
	{
		_log->Error("Attempted to allocate a texel array as an alias that has already been allocated.");
		return false;
	}

	// Ensure the buffer layout has been set
	if (!_bufferLayoutSet)
	{
		_log->Error("Attempted to allocate a texel array as an alias when the buffer layout has not been defined.");
		return false;
	}

	// Ensure initial data has not been provided
	if (_initialDataSet)
	{
		_log->Error("Failed to allocate texel array as alias because initial data has been set. Initial data cannot be provided for a texel array being used as an alias.");
		return false;
	}

	// Calculate the total size of the required buffer
	_totalBufferSizeInBytes = _structureEntryCount * _structureStrideInBytes;
	if (_totalBufferSizeInBytes > bufferSizeInBytes)
	{
		_log->Error("Failed to allocate texel array as alias because the layout requires {0} bytes of memory, but only {1} bytes are available in the buffer.", _totalBufferSizeInBytes, bufferSizeInBytes);
		return false;
	}

	// Calculate the internal data format to use
	if (!GetFormatNative(_imageFormat, _dataFormat, _nativeFormat))
	{
		_log->Error("Failed to allocate memory for texel array. The combination of image format {0} and data format {1} is not supported by this renderer.", _imageFormat, _dataFormat);
		return false;
	}

	// Flag that the buffer has been created
	_buffer = buffer;
	_bufferCreated = true;
	_bufferIsAlias = true;
	FlagBuildStateModified();
	return true;
}

//----------------------------------------------------------------------------------------
void VulkanTexelArray::ReleaseMemory()
{
	// Delete our created buffer object
	auto* memoryManager = _renderer->GetMemoryManager();
	if (_bufferCreated && !_bufferIsAlias)
	{
		memoryManager->DestroyBuffer(_buffer, _bufferAllocation);
	}
	if (_captureDataStagingBuffer != VK_NULL_HANDLE)
	{
		memoryManager->DestroyBuffer(_captureDataStagingBuffer, _captureDataStagingBufferAllocation);
	}
	if (_bufferView != VK_NULL_HANDLE)
	{
		vkDestroyBufferView(_renderer->GetDevice(), _bufferView, nullptr);
	}
}

//----------------------------------------------------------------------------------------
void VulkanTexelArray::SetBufferLayout(ImageFormat imageFormat, DataFormat dataFormat, size_t entryCount)
{
	_imageFormat = imageFormat;
	_dataFormat = dataFormat;
	_structureEntryCount = entryCount;
	_structureStrideInBytes = ByteSizePerElementFromFormat(dataFormat) * ElementCountPerPixelFromFormat(imageFormat);
	_bufferLayoutSet = true;
}

//----------------------------------------------------------------------------------------
// Usage methods
//----------------------------------------------------------------------------------------
void VulkanTexelArray::SetUsageFlags(UsageFlags usageFlags)
{
	_usageFlags = usageFlags;
}

//----------------------------------------------------------------------------------------
void VulkanTexelArray::SetPerformanceHints(PerformanceHint performanceHintCpu, PerformanceHint performanceHintGpu)
{
	_performanceHintCpu = performanceHintCpu;
	_performanceHintGpu = performanceHintGpu;
}

//----------------------------------------------------------------------------------------
void VulkanTexelArray::SetDataPersistenceFlags(DataPersistenceFlags dataPersistenceFlags)
{
	_dataPersistenceFlags = dataPersistenceFlags;
}

//----------------------------------------------------------------------------------------
// Initial data methods
//----------------------------------------------------------------------------------------
SuccessToken VulkanTexelArray::SetInitialData(const void* sourceBuffer, size_t sourceBufferSizeInBytes, SourceImageFormat imageFormat, SourceDataFormat dataFormat)
{
	// Validate the current state of the buffer
	if (_bufferCreated)
	{
		_log->Error("Attempted to set initial texel array data after the buffer has already been created.");
		return false;
	}
	if (_initialDataSet)
	{
		_log->Error("Attempted to set initial texel array data when initial data has already been provided.");
		return false;
	}

	// Store the initial data for use when the buffer is allocated
	_initialData = reinterpret_cast<const uint8_t*>(sourceBuffer);
	_initialDataSizeInBytes = sourceBufferSizeInBytes;
	_initialDataImageFormat = imageFormat;
	_initialDataDataFormat = dataFormat;
	_initialDataSet = true;
	return true;
}

//----------------------------------------------------------------------------------------
// Data update methods
//----------------------------------------------------------------------------------------
SuccessToken VulkanTexelArray::QueueDataUpdate(const void* sourceBuffer, size_t sourceBufferSizeInBytes, SourceImageFormat imageFormat, SourceDataFormat dataFormat, size_t targetBufferOffset, ITransferBatch* transferBatch)
{
	// If we're targeting a transfer batch, make sure it's not already submitted.
	if ((transferBatch != nullptr) && transferBatch->IsSubmitted())
	{
		_log->Error("Attempted to queue a transfer using a transfer batch that has already been submitted");
		return false;
	}

	// Ensure the buffer is allowed to be modified
	if ((_performanceHintCpu & PerformanceHint::WriteFlagsMask) == PerformanceHint::WriteNever)
	{
		_log->Error("Attempted to update a texel array that can't be modified.");
		return false;
	}

	// Verify that the requested write is within the bounds of the buffer
	auto targetBufferOffsetInBytes = targetBufferOffset * _structureStrideInBytes;
	if ((targetBufferOffsetInBytes > _totalBufferSizeInBytes) || ((targetBufferOffsetInBytes + sourceBufferSizeInBytes) > _totalBufferSizeInBytes))
	{
		_log->Error("Attempted to perform a write outside the bounds of a texel array, with write location {0}, byte size {1}, against buffer size of {2}.", targetBufferOffsetInBytes, sourceBufferSizeInBytes, _totalBufferSizeInBytes);
		return false;
	}

	// Capture the supplied update settings and data, performing format conversion if required.
	auto* transferBatchResolved = KnownDynamicCast<VulkanTransferBatch*>(transferBatch);
	PendingWrite pendingWrite(transferBatchResolved);
	if (!ConvertDataFormat(sourceBuffer, sourceBufferSizeInBytes, imageFormat, dataFormat, _imageFormat, _dataFormat, pendingWrite.data))
	{
		_log->Error("QueueDataUpdate failed to convert source data");
		return false;
	}

	// Create an upload buffer for the data
	auto uploadBufferSizeInBytes = sourceBufferSizeInBytes;
	VkBuffer uploadBuffer;
	VmaAllocation uploadBufferAllocation;
	if (transferBatchResolved != nullptr)
	{
		transferBatchResolved->CreateTemporaryTransferBuffer(uploadBufferSizeInBytes, uploadBuffer, uploadBufferAllocation);
	}
	else
	{
		_renderer->CreateTemporaryUploadBuffer(uploadBufferSizeInBytes, uploadBuffer, uploadBufferAllocation, true);
	}

	// Map the upload buffer into the CPU address space
	uint8_t* uploadBufferMapped;
	VulkanMemoryManager* memoryManager = _renderer->GetMemoryManager();
	if (!memoryManager->MapBufferMemory(uploadBufferAllocation, uploadBufferMapped))
	{
		_log->Error("Failed to map upload buffer for texel array");
		return false;
	}

	// Write the data into the upload buffer
	std::memcpy(uploadBufferMapped, sourceBuffer, sourceBufferSizeInBytes);
	pendingWrite.uploadBuffer = uploadBuffer;
	pendingWrite.targetBufferPosInBytes = targetBufferOffsetInBytes;
	pendingWrite.uploadBufferSizeInBytes = uploadBufferSizeInBytes;

	// Unmap the upload buffer
	memoryManager->UnmapBufferMemory(uploadBufferAllocation);

	// Queue a task to update the buffer with the supplied data
	if (transferBatchResolved != nullptr)
	{
		bool separateTransferQueue = _renderer->IsTransferQueueSharedWithGraphics();
		if (separateTransferQueue)
		{
			transferBatchResolved->AddInitializeOperation(this);
		}
		transferBatchResolved->AddTransferOperation([this, pendingWrite](VkCommandBuffer commandBuffer) { CompletePendingDataWrite(commandBuffer, pendingWrite); });
		if (separateTransferQueue)
		{
			transferBatchResolved->AddFinalizeOperation(this);
		}
	}
	else
	{
		std::unique_lock<std::mutex> lock(_buildStateMutex);
		_state[_buildIndex].pendingWrites.push_back(pendingWrite);
		lock.unlock();
		FlagBuildStateModified();
	}
	return true;
}

//----------------------------------------------------------------------------------------
// Data transfer methods
//----------------------------------------------------------------------------------------
SuccessToken VulkanTexelArray::QueueDataTransfer(ITexelArray* targetBuffer, size_t transferCount, size_t sourceBufferOffset, size_t targetBufferOffset, ITransferBatch* transferBatch)
{
	// If we're targeting a transfer batch, make sure it's not already submitted.
	if ((transferBatch != nullptr) && transferBatch->IsSubmitted())
	{
		_log->Error("Attempted to queue a transfer using a transfer batch that has already been submitted");
		return false;
	}

	// Ensure the source and target buffers allow a transfer
	auto* targetBufferResolved = KnownDynamicCast<VulkanTexelArray*>(targetBuffer);
	if ((_performanceHintGpu & PerformanceHint::ReadFlagsMask) == PerformanceHint::ReadNever)
	{
		_log->Error("Attempted to transfer from a texel array that can't be read on the GPU.");
		return false;
	}
	if ((targetBufferResolved->_performanceHintGpu & PerformanceHint::WriteFlagsMask) == PerformanceHint::WriteNever)
	{
		_log->Error("Attempted to transfer to a texel array that can't be modified on the GPU.");
		return false;
	}
	if ((_usageFlags & UsageFlags::TransferSource) != UsageFlags::TransferSource)
	{
		_log->Error("Attempted to transfer from a texel array that hasn't specified the TransferSource usage type.");
		return false;
	}
	if ((targetBufferResolved->_usageFlags & UsageFlags::TransferDestination) != UsageFlags::TransferDestination)
	{
		_log->Error("Attempted to transfer to a texel array that hasn't specified the TransferDestination usage type.");
		return false;
	}
	if (!ImageFormatsAreBinaryEquivalent(_imageFormat, targetBufferResolved->_imageFormat) || !DataFormatsAreBinaryEquivalent(_dataFormat, targetBufferResolved->_dataFormat))
	{
		_log->Error("Attempted to transfer between texel arrays with incompatible formats. Formats between buffers must be binary compatible to perform transfers.");
		return false;
	}

	// Capture the supplied update settings and data
	auto sourceBufferOffsetInBytes = sourceBufferOffset * _structureStrideInBytes;
	auto targetBufferOffsetInBytes = targetBufferOffset * _structureStrideInBytes;
	auto transferCountInBytes = transferCount * _structureStrideInBytes;
	auto* transferBatchResolved = KnownDynamicCast<VulkanTransferBatch*>(transferBatch);
	PendingTransfer pendingTransfer(targetBufferResolved, transferBatchResolved);
	pendingTransfer.transferCountInBytes = transferCountInBytes;
	pendingTransfer.sourceBufferPosInBytes = sourceBufferOffsetInBytes;
	pendingTransfer.targetBufferPosInBytes = targetBufferOffsetInBytes;

	// Queue a task to perform the data transfer
	if (transferBatchResolved != nullptr)
	{
		bool separateTransferQueue = _renderer->IsTransferQueueSharedWithGraphics();
		if (separateTransferQueue)
		{
			transferBatchResolved->AddInitializeOperation(this);
			transferBatchResolved->AddInitializeOperation(targetBufferResolved);
		}
		transferBatchResolved->AddTransferOperation([this, pendingTransfer](VkCommandBuffer commandBuffer) { CompletePendingDataTransfer(commandBuffer, pendingTransfer); });
		if (separateTransferQueue)
		{
			transferBatchResolved->AddFinalizeOperation(targetBufferResolved);
			transferBatchResolved->AddFinalizeOperation(this);
		}
	}
	else
	{
		std::unique_lock<std::mutex> lock(_buildStateMutex);
		_state[_buildIndex].pendingTransfers.push_back(pendingTransfer);
		lock.unlock();
		FlagBuildStateModified();
	}
	return true;
}

//----------------------------------------------------------------------------------------
// Data conversion methods
//----------------------------------------------------------------------------------------
bool VulkanTexelArray::ConvertDataFormat(const void* sourceBuffer, size_t sourceBufferSizeInBytes, SourceImageFormat sourceImageFormat, SourceDataFormat sourceDataFormat, ImageFormat targetImageFormat, DataFormat targetDataFormat, std::vector<uint8_t>& targetBuffer) const
{
	// If the source and target image and data formats match, directly copy the source data to the target buffer.
	if (ImageFormatsAreBinaryEquivalent(sourceImageFormat, targetImageFormat) && DataFormatsAreBinaryEquivalent(sourceDataFormat, targetDataFormat))
	{
		targetBuffer.assign(reinterpret_cast<const uint8_t*>(sourceBuffer), reinterpret_cast<const uint8_t*>(sourceBuffer) + sourceBufferSizeInBytes);
		return true;
	}

	// Convert the data to the required format
	bool sourceDataFormatError = false;
	bool targetDataFormatError = false;
	bool result = TextureFormatConversion::ConvertTexelArrayInputData(sourceBuffer, sourceBufferSizeInBytes, sourceImageFormat, sourceDataFormat, targetImageFormat, targetDataFormat, targetBuffer, sourceDataFormatError, targetDataFormatError);
	if (targetDataFormatError)
	{
		_log->Critical("Data conversion wasn't handled for texel array with image format {0} and data format {1}", targetImageFormat, targetDataFormat);
	}
	if (sourceDataFormatError)
	{
		_log->Error("Attempted to write to texel array with incompatible or unsupported image format {0} and data format {1}", sourceImageFormat, sourceDataFormat);
	}
	return result;
}

//----------------------------------------------------------------------------------------
// Format methods
//----------------------------------------------------------------------------------------
constexpr bool VulkanTexelArray::GetFormatNative(ImageFormat requestedImageFormat, DataFormat requestedDataFormat, VkFormat& nativeFormat)
{
	switch (requestedDataFormat)
	{
	case DataFormat::Int8:
		switch (requestedImageFormat)
		{
		case ImageFormat::R:
		case ImageFormat::X:
			nativeFormat = VkFormat::VK_FORMAT_R8_SINT;
			return true;
		case ImageFormat::RG:
		case ImageFormat::XY:
			nativeFormat = VkFormat::VK_FORMAT_R8G8_SINT;
			return true;
		case ImageFormat::RGBA:
		case ImageFormat::XYZW:
			nativeFormat = VkFormat::VK_FORMAT_R8G8B8A8_SINT;
			return true;
		}
		break;
	case DataFormat::Int16:
		switch (requestedImageFormat)
		{
		case ImageFormat::R:
		case ImageFormat::X:
			nativeFormat = VkFormat::VK_FORMAT_R16_SINT;
			return true;
		case ImageFormat::RG:
		case ImageFormat::XY:
			nativeFormat = VkFormat::VK_FORMAT_R16G16_SINT;
			return true;
		case ImageFormat::RGBA:
		case ImageFormat::XYZW:
			nativeFormat = VkFormat::VK_FORMAT_R16G16B16A16_SINT;
			return true;
		}
		break;
	case DataFormat::Int32:
		switch (requestedImageFormat)
		{
		case ImageFormat::R:
		case ImageFormat::X:
			nativeFormat = VkFormat::VK_FORMAT_R32_SINT;
			return true;
		case ImageFormat::RG:
		case ImageFormat::XY:
			nativeFormat = VkFormat::VK_FORMAT_R32G32_SINT;
			return true;
		case ImageFormat::RGBA:
		case ImageFormat::XYZW:
			nativeFormat = VkFormat::VK_FORMAT_R32G32B32A32_SINT;
			return true;
		}
		break;
	case DataFormat::UInt8:
		switch (requestedImageFormat)
		{
		case ImageFormat::R:
		case ImageFormat::X:
			nativeFormat = VkFormat::VK_FORMAT_R8_UINT;
			return true;
		case ImageFormat::RG:
		case ImageFormat::XY:
			nativeFormat = VkFormat::VK_FORMAT_R8G8_UINT;
			return true;
		case ImageFormat::RGBA:
		case ImageFormat::XYZW:
			nativeFormat = VkFormat::VK_FORMAT_R8G8B8A8_UINT;
			return true;
		}
		break;
	case DataFormat::UInt16:
		switch (requestedImageFormat)
		{
		case ImageFormat::R:
		case ImageFormat::X:
			nativeFormat = VkFormat::VK_FORMAT_R16_UINT;
			return true;
		case ImageFormat::RG:
		case ImageFormat::XY:
			nativeFormat = VkFormat::VK_FORMAT_R16G16_UINT;
			return true;
		case ImageFormat::RGBA:
		case ImageFormat::XYZW:
			nativeFormat = VkFormat::VK_FORMAT_R16G16B16A16_UINT;
			return true;
		}
		break;
	case DataFormat::UInt32:
		switch (requestedImageFormat)
		{
		case ImageFormat::R:
		case ImageFormat::X:
			nativeFormat = VkFormat::VK_FORMAT_R32_UINT;
			return true;
		case ImageFormat::RG:
		case ImageFormat::XY:
			nativeFormat = VkFormat::VK_FORMAT_R32G32_UINT;
			return true;
		case ImageFormat::RGBA:
		case ImageFormat::XYZW:
			nativeFormat = VkFormat::VK_FORMAT_R32G32B32A32_UINT;
			return true;
		}
		break;
	case DataFormat::Norm8:
		switch (requestedImageFormat)
		{
		case ImageFormat::R:
		case ImageFormat::X:
			nativeFormat = VkFormat::VK_FORMAT_R8_SNORM;
			return true;
		case ImageFormat::RG:
		case ImageFormat::XY:
			nativeFormat = VkFormat::VK_FORMAT_R8G8_SNORM;
			return true;
		case ImageFormat::RGBA:
		case ImageFormat::XYZW:
			nativeFormat = VkFormat::VK_FORMAT_R8G8B8A8_SNORM;
			return true;
		}
		break;
	case DataFormat::UNorm8:
		switch (requestedImageFormat)
		{
		case ImageFormat::R:
		case ImageFormat::X:
			nativeFormat = VkFormat::VK_FORMAT_R8_UNORM;
			return true;
		case ImageFormat::RG:
		case ImageFormat::XY:
			nativeFormat = VkFormat::VK_FORMAT_R8G8_UNORM;
			return true;
		case ImageFormat::RGBA:
		case ImageFormat::XYZW:
			nativeFormat = VkFormat::VK_FORMAT_R8G8B8A8_UNORM;
			return true;
		}
		break;
	case DataFormat::Float16:
		switch (requestedImageFormat)
		{
		case ImageFormat::R:
		case ImageFormat::X:
			nativeFormat = VkFormat::VK_FORMAT_R16_SFLOAT;
			return true;
		case ImageFormat::RG:
		case ImageFormat::XY:
			nativeFormat = VkFormat::VK_FORMAT_R16G16_SFLOAT;
			return true;
		case ImageFormat::RGBA:
		case ImageFormat::XYZW:
			nativeFormat = VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT;
			return true;
		}
		break;
	case DataFormat::Float32:
		switch (requestedImageFormat)
		{
		case ImageFormat::R:
		case ImageFormat::X:
			nativeFormat = VkFormat::VK_FORMAT_R32_SFLOAT;
			return true;
		case ImageFormat::RG:
		case ImageFormat::XY:
			nativeFormat = VkFormat::VK_FORMAT_R32G32_SFLOAT;
			return true;
		case ImageFormat::RGBA:
		case ImageFormat::XYZW:
			nativeFormat = VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
			return true;
		}
		break;
	}
	return false;
}

//----------------------------------------------------------------------------------------
// Output capture methods
//----------------------------------------------------------------------------------------
bool VulkanTexelArray::HasCaptureTargets() const
{
	return !_state[_drawIndex].captureTargets.empty();
}

//----------------------------------------------------------------------------------------
void VulkanTexelArray::AddOutputCaptureTarget(ITexelArrayOutput* captureTarget)
{
	std::unique_lock<std::mutex> lock(_accessMutex);
	_state[_buildIndex].captureTargets.push_back(KnownDynamicCast<VulkanTexelArrayOutput*>(captureTarget));
	lock.unlock();
	FlagBuildStateModified();
}

//----------------------------------------------------------------------------------------
void VulkanTexelArray::RemoveOutputCaptureTarget(ITexelArrayOutput* captureTarget)
{
	std::unique_lock<std::mutex> lock(_accessMutex);
	auto* captureTargetResolved = KnownDynamicCast<VulkanTexelArrayOutput*>(captureTarget);
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
void VulkanTexelArray::CaptureDataBufferOutput(VkCommandBuffer commandBuffer)
{
	// If no capture targets are defined, abort any further processing.
	if (_state[_drawIndex].captureTargets.empty())
	{
		return;
	}

	// Retrieve or create a staging buffer for the captured data
	if (_captureDataStagingBuffer == VK_NULL_HANDLE)
	{
		_renderer->CreatePersistentReadbackBuffer(_totalBufferSizeInBytes, _captureDataStagingBuffer, _captureDataStagingBufferAllocation);
	}

	// Transfer each requested capture region into the data staging buffer
	for (auto* captureTarget : _state[_drawIndex].captureTargets)
	{
		// Retrieve the requested capture settings from our framebuffer output object
		size_t bufferStartPosInBytes = captureTarget->GetRequestedBufferOffset() * _structureStrideInBytes;
		size_t dataSizeInBytes = captureTarget->GetRequestedEntryCount() * _structureStrideInBytes;
		dataSizeInBytes = (dataSizeInBytes > 0 ? dataSizeInBytes : _totalBufferSizeInBytes);

		// Transfer the requested capture region into the staging buffer
		VkBufferCopy copyRegion = {};
		copyRegion.srcOffset = bufferStartPosInBytes;
		copyRegion.dstOffset = bufferStartPosInBytes;
		copyRegion.size = dataSizeInBytes;
		vkCmdCopyBuffer(commandBuffer, _buffer, _captureDataStagingBuffer, 1, &copyRegion);
	}
}

//----------------------------------------------------------------------------------------
void VulkanTexelArray::CompleteCaptureDataBufferOutput()
{
	// Reset our flag indicating this buffer has been added as a current resource buffer
	_addedAsCurrent = false;

	// If no capture targets are defined, abort any further processing.
	if (_state[_drawIndex].captureTargets.empty())
	{
		return;
	}

	// Map the data staging buffer
	VulkanMemoryManager* memoryManager = _renderer->GetMemoryManager();
	uint8_t* mappedBuffer;
	if (!memoryManager->MapBufferMemory(_captureDataStagingBufferAllocation, mappedBuffer))
	{
		_log->Error("Failed to map staging buffer when attempting to capture texel array output");
		return;
	}

	// Store the buffer data into each attached capture target
	const auto* mappedMemory = (const uint8_t*)mappedBuffer;
	for (auto* captureTarget : _state[_drawIndex].captureTargets)
	{
		// Determine the position and size of the buffer region to capture
		size_t entryOffset = captureTarget->GetRequestedBufferOffset();
		size_t entryCountToCapture = captureTarget->GetRequestedEntryCount();
		entryCountToCapture = (entryCountToCapture == 0 ? _structureEntryCount : entryCountToCapture);
		entryCountToCapture = (((entryCountToCapture + entryOffset) > _structureEntryCount) ? (_structureEntryCount - entryOffset) : entryCountToCapture);
		size_t bufferStartPosInBytes = entryOffset * _structureStrideInBytes;

		// Store the requested capture data in the capture target
		bool detachAfterCapture = captureTarget->IsDetachingAfterCapture();
		captureTarget->StoreCaptureData(mappedMemory + bufferStartPosInBytes, entryCountToCapture, _structureStrideInBytes, _imageFormat, _dataFormat);

		// Record this captured framebuffer output with the renderer
		_renderer->AddCurrentTexelArrayOutput(captureTarget);

		// Now that we've captured a frame, detach the output capture target if requested.
		if (detachAfterCapture)
		{
			RemoveOutputCaptureTarget(captureTarget);
		}
	}

	// Unmap the data staging buffer
	memoryManager->UnmapBufferMemory(_captureDataStagingBufferAllocation);
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
void VulkanTexelArray::MigrateBuildStateToDrawState()
{
	std::swap(_buildIndex, _drawIndex);
	_stateModified.clear(std::memory_order_relaxed);

	// Ensure any updated capture targets are carried over to the new build state
	_state[_buildIndex].captureTargets = _state[_drawIndex].captureTargets;
}

//----------------------------------------------------------------------------------------
void VulkanTexelArray::CompletePendingDataWrites(VkCommandBuffer commandBuffer)
{
	// Obtain the set of current pending writes. If no writes are pending, abort any further processing.
	std::vector<PendingWrite>& pendingWrites = _state[_drawIndex].pendingWrites;
	if (pendingWrites.empty())
	{
		return;
	}

	// Perform each data write into the upload buffer
	for (const PendingWrite& write : pendingWrites)
	{
		CompletePendingDataWrite(commandBuffer, write);
	}

	// Free our set of pending writes
	pendingWrites.clear();
}

//----------------------------------------------------------------------------------------
void VulkanTexelArray::CompletePendingDataWrite(VkCommandBuffer commandBuffer, const PendingWrite& pendingWrite)
{
	// Schedule a data transfer from the upload buffer to the target buffer
	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = pendingWrite.targetBufferPosInBytes;
	copyRegion.size = pendingWrite.uploadBufferSizeInBytes;
	vkCmdCopyBuffer(commandBuffer, pendingWrite.uploadBuffer, _buffer, 1, &copyRegion);
}

//----------------------------------------------------------------------------------------
void VulkanTexelArray::CompletePendingDataTransfers(VkCommandBuffer commandBuffer)
{
	// Obtain the set of pending data transfers. If no transfers are pending, abort any further processing.
	std::vector<PendingTransfer>& pendingTransfers = _state[_drawIndex].pendingTransfers;
	if (pendingTransfers.empty())
	{
		return;
	}

	// Complete each pending data transfer
	for (const PendingTransfer& transfer : pendingTransfers)
	{
		CompletePendingDataTransfer(commandBuffer, transfer);
	}

	// Clear the set of pending transfers now that they've been performed
	pendingTransfers.clear();
}

//----------------------------------------------------------------------------------------
void VulkanTexelArray::CompletePendingDataTransfer(VkCommandBuffer commandBuffer, const PendingTransfer& pendingTransfer)
{
	// Schedule a data transfer from this buffer to the target buffer
	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = pendingTransfer.sourceBufferPosInBytes;
	copyRegion.dstOffset = pendingTransfer.targetBufferPosInBytes;
	copyRegion.size = pendingTransfer.transferCountInBytes;
	vkCmdCopyBuffer(commandBuffer, _buffer, pendingTransfer.targetBuffer->_buffer, 1, &copyRegion);
}

//----------------------------------------------------------------------------------------
void VulkanTexelArray::PerformGraphicsQueueAcquireOperation(VkCommandBuffer commandBuffer)
{
	// Perform an acquire operation on the graphics queue for the buffer
	//##FIX## These barriers are overly broad for pure texel arrays. Optimize them based on whether this buffer is
	//"aliased" through index/vertex buffers.
	VkBufferMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
	barrier.srcQueueFamilyIndex = _renderer->GetTransferQueueFamily();
	barrier.dstQueueFamilyIndex = _renderer->GetGraphicsQueueFamily();
	barrier.buffer = _buffer;
	barrier.offset = 0;
	barrier.size = _totalBufferSizeInBytes;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);
}

//----------------------------------------------------------------------------------------
void VulkanTexelArray::PerformGraphicsQueueReleaseOperation(VkCommandBuffer commandBuffer)
{
	// Perform a release operation on the graphics queue for the buffer
	VkBufferMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
	barrier.dstAccessMask = 0;
	barrier.srcQueueFamilyIndex = _renderer->GetGraphicsQueueFamily();
	barrier.dstQueueFamilyIndex = _renderer->GetTransferQueueFamily();
	barrier.buffer = _buffer;
	barrier.offset = 0;
	barrier.size = _totalBufferSizeInBytes;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);
}

//----------------------------------------------------------------------------------------
bool VulkanTexelArray::PerformTransferQueueAcquireOperation(VkCommandBuffer commandBuffer, bool canDiscardCurrentContent)
{
	// Increment the transfer queue usage count for this buffer. If there was already an existing usage, abort any
	// further processing.
	uint32_t previousTransferQueueUseCount = _transferQueueUseCount.fetch_add(1, std::memory_order_acq_rel);
	if (previousTransferQueueUseCount != 0)
	{
		return false;
	}

	// Since we're transferring ownership to the transfer queue, attempt to remove this from the list of buffers pending
	// an aquire operation on the graphics queue. This can occur where this same buffer is used in a batch transfer,
	// that transfer completes, then before another frame advances it is added to another batch transfer. If the removal
	// succeeds, we know the buffer is currently still owned by the transfer queue, otherwise we perform a transfer
	// queue acquire operation here.
	if (_renderer->CancelPendingGraphicsQueueAcquireOperation(this))
	{
		return false;
	}

	// Perform an acquire operation on the transfer queue for the buffer
	VkBufferMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.srcQueueFamilyIndex = _renderer->GetGraphicsQueueFamily();
	barrier.dstQueueFamilyIndex = _renderer->GetTransferQueueFamily();
	barrier.buffer = _buffer;
	barrier.offset = 0;
	barrier.size = _totalBufferSizeInBytes;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);
	return true;
}

//----------------------------------------------------------------------------------------
bool VulkanTexelArray::PerformTransferQueueReleaseOperation(VkCommandBuffer commandBuffer)
{
	// Decrement the transfer queue usage count for this buffer. If there is at least one user remaining, abort any
	// further processing.
	uint32_t previousTransferQueueUseCount = _transferQueueUseCount.fetch_sub(1, std::memory_order_acq_rel);
	if (previousTransferQueueUseCount != 1)
	{
		return false;
	}

	// Perform a release operation on the transfer queue for the buffer
	VkBufferMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = 0;
	barrier.srcQueueFamilyIndex = _renderer->GetTransferQueueFamily();
	barrier.dstQueueFamilyIndex = _renderer->GetGraphicsQueueFamily();
	barrier.buffer = _buffer;
	barrier.offset = 0;
	barrier.size = _totalBufferSizeInBytes;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);
	return true;
}

//----------------------------------------------------------------------------------------
VkBufferView VulkanTexelArray::GetBufferView()
{
	// Create the buffer view if required
	if (_bufferView == VK_NULL_HANDLE)
	{
		VkBufferViewCreateInfo bufferViewCreateInfo{};
		bufferViewCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
		bufferViewCreateInfo.pNext = nullptr;
		bufferViewCreateInfo.flags = 0;
		bufferViewCreateInfo.buffer = _buffer;
		bufferViewCreateInfo.format = _nativeFormat;
		bufferViewCreateInfo.offset = 0;
		bufferViewCreateInfo.range = _totalBufferSizeInBytes;
		VkResult vkCreateBufferViewResult = vkCreateBufferView(_renderer->GetDevice(), &bufferViewCreateInfo, nullptr, &_bufferView);
		if (vkCreateBufferViewResult != VK_SUCCESS)
		{
			_log->Error("vkCreateBufferView failed for texel array with error code {0}", vkCreateBufferViewResult);
		}
	}

	// Return the buffer view to the caller
	return _bufferView;
}

//----------------------------------------------------------------------------------------
void VulkanTexelArray::FlagBuildStateModified()
{
	if (!_stateModified.test_and_set(std::memory_order_acquire))
	{
		_renderer->FlagObjectModified(this);
	}
}

//----------------------------------------------------------------------------------------
VkBuffer VulkanTexelArray::GetNativeBuffer() const
{
	return _buffer;
}

//----------------------------------------------------------------------------------------
size_t VulkanTexelArray::GetBufferSizeInBytes() const
{
	return _totalBufferSizeInBytes;
}

//----------------------------------------------------------------------------------------
void VulkanTexelArray::AddAsCurrentBuffer()
{
	if (!_addedAsCurrent)
	{
		_renderer->AddCurrentTexelArray(this);
		_addedAsCurrent = true;
	}
}

} // namespace cobalt::graphics
