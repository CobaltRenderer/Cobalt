// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <vector>
namespace cobalt::graphics {
class VulkanRenderer;

class VulkanDataArrayOutput : public IDataArrayOutput
{
public:
	// Constructors
	VulkanDataArrayOutput(cobalt::logging::ILogger* log, VulkanRenderer* renderer);

	// Initialization methods
	void Delete() override;

	// Configuration methods
	void SetDetachAfterCapture(bool state) override;
	void SetArrayCaptureRegion(size_t captureEntryCount, size_t bufferOffset, bool captureCounterValue) override;

	// Data methods
	bool HasCapturedOutput() const override;
	bool HasCapturedCounterValue() const override;
	void ClearCapturedOutput() override;
	size_t GetEntryCount() const override;
	size_t GetEntrySizeInBytes() const override;
	SuccessToken ReadBufferData(void* targetBuffer, size_t targetBufferSizeInBytes) const override;
	SuccessToken ReadCounterValue(uint32_t& counterValue) const override;

	// Capture methods
	bool IsDetachingAfterCapture() const;
	size_t GetRequestedEntryCount() const;
	size_t GetRequestedBufferOffset() const;
	void StoreCaptureData(const unsigned char* bufferData, size_t bufferEntryCount, size_t bufferEntrySizeInBytes, bool hasCounterValue, uint32_t counterValue);
	void EnableCaptureReadFromCurrentDrawBuffer();

	// Build state methods
	void MigrateBuildStateToDrawState();
	uint32_t ReadIndex() const;

private:
	// Structures
	struct MutableState
	{
		bool hasCapturedOutput = false;
		bool hasCapturedCounterValue = false;
		bool detachAfterCapture = false;
		bool forceCaptureCounterValue = false;
		bool captureCounterValue = false;
		size_t actualCaptureEntryCount = 0;
		size_t actualCaptureEntrySizeInBytes = 0;
		size_t requestedCaptureEntryCount = 0;
		size_t requestedBufferOffset = 0;
		uint32_t counterValue = 0;
		std::vector<uint8_t> dataBuffer;
	};

private:
	cobalt::logging::ILogger* _log;
	VulkanRenderer* _renderer;
	bool _readFromCurrentDrawBufferEnabled;
	uint32_t _buildIndex;
	uint32_t _drawIndex;
	MutableState _state[2];
};

} // namespace cobalt::graphics
