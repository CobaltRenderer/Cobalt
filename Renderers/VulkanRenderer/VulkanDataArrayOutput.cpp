// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "VulkanDataArrayOutput.h"
#include "VulkanRenderer.h"
#include <cstring>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
VulkanDataArrayOutput::VulkanDataArrayOutput(cobalt::logging::ILogger* log, VulkanRenderer* renderer)
: _log(log), _renderer(renderer), _buildIndex(0), _drawIndex(1), _readFromCurrentDrawBufferEnabled(false)
{
	_state[_drawIndex].hasCapturedOutput = false;
	_state[_buildIndex].hasCapturedOutput = false;
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void VulkanDataArrayOutput::Delete()
{
	_renderer->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
// Configuration methods
//----------------------------------------------------------------------------------------
void VulkanDataArrayOutput::SetDetachAfterCapture(bool state)
{
	_state[_buildIndex].detachAfterCapture = state;
	_renderer->FlagObjectModified(this);
}

//----------------------------------------------------------------------------------------
void VulkanDataArrayOutput::SetArrayCaptureRegion(size_t captureEntryCount, size_t bufferOffset, bool captureCounterValue)
{
	_state[_buildIndex].requestedCaptureEntryCount = captureEntryCount;
	_state[_buildIndex].requestedBufferOffset = bufferOffset;
	_state[_buildIndex].forceCaptureCounterValue = true;
	_state[_buildIndex].captureCounterValue = captureCounterValue;
	_renderer->FlagObjectModified(this);
}

//----------------------------------------------------------------------------------------
// Data methods
//----------------------------------------------------------------------------------------
bool VulkanDataArrayOutput::HasCapturedOutput() const
{
	return _state[ReadIndex()].hasCapturedOutput;
}

//----------------------------------------------------------------------------------------
bool VulkanDataArrayOutput::HasCapturedCounterValue() const
{
	return _state[ReadIndex()].hasCapturedOutput && _state[ReadIndex()].hasCapturedCounterValue;
}

//----------------------------------------------------------------------------------------
void VulkanDataArrayOutput::ClearCapturedOutput()
{
	_state[ReadIndex()].dataBuffer.clear();
	_state[ReadIndex()].hasCapturedOutput = false;
}

//----------------------------------------------------------------------------------------
size_t VulkanDataArrayOutput::GetEntryCount() const
{
	if (!_state[ReadIndex()].hasCapturedOutput)
	{
		return 0;
	}
	return _state[ReadIndex()].actualCaptureEntryCount;
}

//----------------------------------------------------------------------------------------
size_t VulkanDataArrayOutput::GetEntrySizeInBytes() const
{
	if (!_state[ReadIndex()].hasCapturedOutput)
	{
		return 0;
	}
	return _state[ReadIndex()].actualCaptureEntrySizeInBytes;
}

//----------------------------------------------------------------------------------------
SuccessToken VulkanDataArrayOutput::ReadBufferData(void* targetBuffer, size_t targetBufferSizeInBytes) const
{
	// Ensure we have a captured output to work with
	if (!HasCapturedOutput())
	{
		_log->Error("Failed to read data array output. A read was attempted when no captured data exists in the buffer.");
		return false;
	}

	// Determine the size of the buffer region to copy, and verify the output buffer is large enough to receive it.
	auto readIndex = ReadIndex();
	size_t capturedDataSizeInBytes = _state[readIndex].dataBuffer.size();
	size_t readEndPos = targetBufferSizeInBytes;
	if (readEndPos > capturedDataSizeInBytes)
	{
		_log->Error("Failed to read data array output. The requested read requires {0} bytes, but the captured data only contains {1} bytes.", targetBufferSizeInBytes, capturedDataSizeInBytes);
		return false;
	}

	// Copy the requested data into the target buffer
	std::memcpy(targetBuffer, _state[readIndex].dataBuffer.data(), targetBufferSizeInBytes);
	return true;
}

//----------------------------------------------------------------------------------------
SuccessToken VulkanDataArrayOutput::ReadCounterValue(uint32_t& counterValue) const
{
	const auto& readState = _state[ReadIndex()];
	if (!readState.hasCapturedOutput || !readState.hasCapturedCounterValue)
	{
		_log->Error("Attempted to read counter value when no counter value has been captured");
		counterValue = 0;
		return false;
	}
	counterValue = readState.counterValue;
	return true;
}

//----------------------------------------------------------------------------------------
// Capture methods
//----------------------------------------------------------------------------------------
bool VulkanDataArrayOutput::IsDetachingAfterCapture() const
{
	return _state[_drawIndex].detachAfterCapture;
}

//----------------------------------------------------------------------------------------
size_t VulkanDataArrayOutput::GetRequestedEntryCount() const
{
	return _state[_drawIndex].requestedCaptureEntryCount;
}

//----------------------------------------------------------------------------------------
size_t VulkanDataArrayOutput::GetRequestedBufferOffset() const
{
	return _state[_drawIndex].requestedBufferOffset;
}

//----------------------------------------------------------------------------------------
void VulkanDataArrayOutput::StoreCaptureData(const unsigned char* bufferData, size_t bufferEntryCount, size_t bufferEntrySizeInBytes, bool hasCounterValue, uint32_t counterValue)
{
	// If a counter value has been specifically requested to be captured, and a counter wasn't attached to the target
	// buffer, log an error and abort any further processing.
	if (!hasCounterValue && _state[_drawIndex].forceCaptureCounterValue && _state[_drawIndex].captureCounterValue)
	{
		_log->Error("Attempted to capture data array output counter value, when no counter was attached to the target array.");
		return;
	}

	// Store the provided capture data
	_state[_drawIndex].dataBuffer.assign(bufferData, bufferData + (bufferEntryCount * bufferEntrySizeInBytes));
	_state[_drawIndex].actualCaptureEntryCount = bufferEntryCount;
	_state[_drawIndex].actualCaptureEntrySizeInBytes = bufferEntrySizeInBytes;
	_state[_drawIndex].counterValue = counterValue;
	_state[_drawIndex].hasCapturedOutput = true;
	_state[_drawIndex].hasCapturedCounterValue = hasCounterValue && (!_state[_drawIndex].forceCaptureCounterValue || _state[_drawIndex].captureCounterValue);
	_renderer->FlagObjectModified(this);
}

//----------------------------------------------------------------------------------------
void VulkanDataArrayOutput::EnableCaptureReadFromCurrentDrawBuffer()
{
	_readFromCurrentDrawBufferEnabled = true;
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
void VulkanDataArrayOutput::MigrateBuildStateToDrawState()
{
	std::swap(_buildIndex, _drawIndex);
	_state[_drawIndex].hasCapturedOutput = false;
	_state[_buildIndex].detachAfterCapture = _state[_drawIndex].detachAfterCapture;
	_state[_buildIndex].forceCaptureCounterValue = _state[_drawIndex].forceCaptureCounterValue;
	_state[_buildIndex].captureCounterValue = _state[_drawIndex].captureCounterValue;
	_state[_buildIndex].requestedBufferOffset = _state[_drawIndex].requestedBufferOffset;
	_state[_buildIndex].requestedCaptureEntryCount = _state[_drawIndex].requestedCaptureEntryCount;
	_readFromCurrentDrawBufferEnabled = false;
}

//----------------------------------------------------------------------------------------
uint32_t VulkanDataArrayOutput::ReadIndex() const
{
	return (_readFromCurrentDrawBufferEnabled ? _drawIndex : _buildIndex);
}

} // namespace cobalt::graphics
