// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
namespace cobalt::graphics {

class OpenGLGraphicsDevice : public IGraphicsDevice
{
public:
	// Constructors
#ifdef __linux__
	OpenGLGraphicsDevice(cobalt::logging::ILogger* log, bool usingSoftwareRenderer, int deviceIndex, const std::string& renderNodePath);
#else
	OpenGLGraphicsDevice(cobalt::logging::ILogger* log, bool usingSoftwareRenderer);
#endif

	// Info methods
	bool ReadDeviceInfo();
	DeviceType GetDeviceType() const override;
	Vendor GetVendor() const override;
	Marshal::Ret<std::string> GetVendorName() const override;
	Marshal::Ret<std::string> GetDeviceName() const override;
	Marshal::Ret<std::string> GetDriverInfo() const override;
	size_t GetMemorySizeInBytes(MemoryType memoryType) const override;

	// Limit methods
	ImageLimits GetImageLimits() const override;
	ShaderLimits GetShaderLimits() const override;
	DrawLimits GetDrawLimits() const override;
	FrameBufferLimits GetFrameBufferLimits() const override;
	DataBufferLimits GetDataBufferLimits() const override;

	// Feature methods
	bool IsFeatureSupported(Feature feature) const override;
	bool AreAllFeaturesSupported(const Marshal::In<std::set<Feature>>& featureSet) const override;
	Marshal::Ret<std::set<Feature>> GetAllSupportedFeatures() const override;
	bool IsTextureFormatSupported(ITextureBuffer::ImageFormat imageFormat, ITextureBuffer::DataFormat dataFormat) const override;

	// Renderer methods
	IRenderer::unique_ptr CreateRenderer(const Marshal::In<std::set<Feature>>& enabledFeatures, const Marshal::In<std::set<IRenderer::Options>>& enabledOptions) override;

private:
	// Memory methods
	void FetchMemoryInfoInternal(Vendor vendor);
#ifdef __APPLE__
	static bool TryReadMacOSOpenGLVideoMemory(size_t& videoMemorySizeInBytes);
#endif
#if defined(__linux__) && !defined(OPENGL_USE_EGL)
	static bool TryReadGLXMesaRendererMemory(size_t& dedicatedMemorySizeInBytes, size_t& sharedMemorySizeInBytes);
#endif
#ifdef __linux__
	static bool TryReadLinuxDrmMemoryInfo(const std::string& renderNodePath, size_t& dedicatedMemorySizeInBytes, size_t& sharedMemorySizeInBytes);
	static bool TryReadUnsignedIntegerFile(const std::string& filePath, size_t& value);
#endif

	// Limit methods
	void FetchImageLimitsInternal();
	void FetchShaderLimitsInternal();
	void FetchDrawLimitsInternal();
	void FetchFrameBufferLimitsInternal();
	void FetchDataBufferLimitsInternal();

private:
	cobalt::logging::ILogger* _log = nullptr;
	bool _usingSoftwareRenderer = false;
	int _deviceIndex = 0;
	std::string _renderNodePath;
	size_t _dedicatedMemorySizeInBytes = {};
	size_t _sharedMemorySizeInBytes = {};
	ImageLimits _imageLimits = {};
	ShaderLimits _shaderLimits = {};
	DrawLimits _drawLimits = {};
	FrameBufferLimits _frameBufferLimits = {};
	DataBufferLimits _dataBufferLimits = {};
	std::string _vendorName;
	std::string _deviceName;
	std::string _versionString;
	std::set<Feature> _supportedFeatureSet;
};

} // namespace cobalt::graphics
