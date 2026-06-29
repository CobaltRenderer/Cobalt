// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <memory>
#include <vector>
namespace cobalt::graphics {
class OpenGLRenderer;

class OpenGLFrameBufferOutput : public IFrameBufferOutput
{
public:
	// Constructors
	OpenGLFrameBufferOutput(cobalt::logging::ILogger* log, OpenGLRenderer* renderer);

	// Initialization methods
	void Delete() override;

	// Configuration methods
	void SetDetachAfterCapture(bool state) override;
	void SetFrameBufferCaptureRegion(const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels) override;

	// Data methods
	bool HasCapturedOutput() const override;
	void ClearCapturedOutput() override;
	V2UInt32 GetImageDimensions() const override;
	V2UInt32 GetCroppedImageDimensions(const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels) const override;
	static V2UInt32 CalculateCroppedImageDimensions(const V2UInt32& imageSizeInPixels, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels);
	ITextureBuffer::SourceImageFormat GetOptimalImageFormat() const override;
	ITextureBuffer::SourceDataFormat GetOptimalDataFormat() const override;
	SuccessToken ReadBufferData(void* targetBuffer, size_t targetBufferSizeInBytes, ITextureBuffer::SourceImageFormat imageFormat, ITextureBuffer::SourceDataFormat dataFormat, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels) const override;

	// Capture methods
	bool IsDetachingAfterCapture() const;
	V2UInt32 GetRequestedImageOffset() const;
	V2UInt32 GetRequestedImageSize() const;
	void StoreCaptureData(const V2UInt32& imageSize, const unsigned char* imageData, size_t imageDataSizeInBytes, ITextureBuffer::ImageFormat imageFormat, ITextureBuffer::DataFormat dataFormat, ITextureBuffer::SourceImageFormat optimalSourceImageFormat, ITextureBuffer::SourceDataFormat optimalSourceDataFormat, bool isStencilComponent);
	void EnableCaptureReadFromCurrentDrawBuffer();

	// Build state methods
	void MigrateBuildStateToDrawState();
	uint32_t ReadIndex() const;

protected:
	// Data methods
	size_t CalculatePixelCountForRegion(const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels) const override;

private:
	// Structures
	struct MutableState
	{
		bool hasCapturedOutput = false;
		bool detachAfterCapture = false;
		bool isStencilComponent = false;

		V2UInt32 requestedImageSize = {0, 0};
		V2UInt32 requestedImageOffset = {0, 0};
		V2UInt32 actualImageSize = {0, 0};
		V2UInt32 actualImageOffset = {0, 0};

		ITextureBuffer::ImageFormat imageFormat = ITextureBuffer::ImageFormat::R;
		ITextureBuffer::DataFormat dataFormat = ITextureBuffer::DataFormat::UInt8;
		ITextureBuffer::SourceImageFormat optimalSourceImageFormat = ITextureBuffer::SourceImageFormat::R;
		ITextureBuffer::SourceDataFormat optimalSourceDataFormat = ITextureBuffer::SourceDataFormat::UInt8;
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
