// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <vector>
namespace cobalt::graphics {
class OpenGLRenderer;

class OpenGLTexelArrayOutput : public ITexelArrayOutput
{
public:
	// Constructors
	OpenGLTexelArrayOutput(cobalt::logging::ILogger* log, OpenGLRenderer* renderer);

	// Initialization methods
	void Delete() override;

	// Configuration methods
	void SetDetachAfterCapture(bool state) override;
	void SetArrayCaptureRegion(size_t captureEntryCount, size_t bufferOffset) override;

	// Data methods
	bool HasCapturedOutput() const override;
	void ClearCapturedOutput() override;
	size_t GetEntryCount() const override;
	ITexelArray::SourceImageFormat GetOptimalImageFormat() const override;
	ITexelArray::SourceDataFormat GetOptimalDataFormat() const override;
	SuccessToken ReadBufferData(void* targetBuffer, size_t targetBufferSizeInBytes, ITexelArray::SourceImageFormat imageFormat, ITexelArray::SourceDataFormat dataFormat) const override;

	// Capture methods
	bool IsDetachingAfterCapture() const;
	size_t GetRequestedEntryCount() const;
	size_t GetRequestedBufferOffset() const;
	void StoreCaptureData(const unsigned char* bufferData, size_t bufferEntryCount, size_t bufferEntrySizeInBytes, ITexelArray::ImageFormat imageFormat, ITexelArray::DataFormat dataFormat);
	void EnableCaptureReadFromCurrentDrawBuffer();

	// Build state methods
	void MigrateBuildStateToDrawState();
	uint32_t ReadIndex() const;

private:
	// Structures
	struct MutableState
	{
		bool hasCapturedOutput = false;
		bool detachAfterCapture = false;
		size_t actualCaptureEntryCount = 0;
		size_t actualCaptureEntrySizeInBytes = 0;
		size_t requestedCaptureEntryCount = 0;
		size_t requestedBufferOffset = 0;
		ITexelArray::ImageFormat imageFormat = {};
		ITexelArray::DataFormat dataFormat = {};
		std::vector<uint8_t> dataBuffer;
	};

private:
	cobalt::logging::ILogger* _log;
	OpenGLRenderer* _renderer;
	bool _readFromCurrentDrawBufferEnabled;
	uint32_t _buildIndex;
	uint32_t _drawIndex;
	MutableState _state[2];
};

} // namespace cobalt::graphics
