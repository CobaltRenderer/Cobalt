// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "OpenGLHeaders.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <atomic>
#include <mutex>
namespace cobalt::graphics {
class OpenGLRenderer;
class OpenGLTransferBatch;

template<class InterfaceType, class DimensionVectorType>
class OpenGLTextureBuffer : public InterfaceType
{
public:
	// Constructors
	OpenGLTextureBuffer(cobalt::logging::ILogger* log, OpenGLRenderer* renderer, GLenum textureTarget);
	~OpenGLTextureBuffer();

	// Initialization methods
	SuccessToken AllocateMemory() final;

	// Usage methods
	typename InterfaceType::UsageFlags GetUsageFlags() const;
	typename InterfaceType::PerformanceHint GetPerformanceHintCpu() const;
	typename InterfaceType::PerformanceHint GetPerformanceHintGpu() const;
	typename InterfaceType::DataPersistenceFlags GetDataPersistenceFlags() const;
	void SetUsageFlags(typename InterfaceType::UsageFlags usageFlags) final;
	void SetPerformanceHints(typename InterfaceType::PerformanceHint performanceHintCpu, typename InterfaceType::PerformanceHint performanceHintGpu) final;
	void SetDataPersistenceFlags(typename InterfaceType::DataPersistenceFlags dataPersistenceFlags) final;

	// Format methods
	void SetTextureFormat(typename InterfaceType::ImageFormat imageFormat, typename InterfaceType::DataFormat dataFormat) final;
	using InterfaceType::SetTextureDimensions;
	// NOLINTNEXTLINE(hicpp-use-override, modernize-use-override)
	virtual void SetTextureDimensions(uint32_t faceLength, int mipmapLevelCount) final;
	// NOLINTNEXTLINE(hicpp-use-override, modernize-use-override)
	virtual void SetTextureDimensions(uint32_t faceLength, size_t arraySize, int mipmapLevelCount) final;
	// NOLINTNEXTLINE(hicpp-use-override, modernize-use-override)
	virtual void SetTextureDimensions(const DimensionVectorType& imageDimensions, int mipmapLevelCount) final;
	// NOLINTNEXTLINE(hicpp-use-override, modernize-use-override)
	virtual void SetTextureDimensions(const DimensionVectorType& imageDimensions, size_t arraySize, int mipmapLevelCount) final;
	typename InterfaceType::SampleCount GetSampleCount() const;
	// NOLINTNEXTLINE(hicpp-use-override, modernize-use-override)
	virtual bool IsSampleCountSupported(typename InterfaceType::ImageFormat imageFormat, typename InterfaceType::DataFormat dataFormat, typename InterfaceType::SampleCount sampleCount) const final;
	// NOLINTNEXTLINE(hicpp-use-override, modernize-use-override)
	virtual void SetSampleCount(typename InterfaceType::SampleCount sampleCount) final;
	static GLsizei GetNativeSampleCountFromSampleCount(typename InterfaceType::SampleCount sampleCount);
	typename InterfaceType::ImageFormat AllocatedImageFormat() const final;
	typename InterfaceType::DataFormat AllocatedDataFormat() const final;
	bool IsCompressedTexture() const;
	size_t ArraySize() const;
	int MipmapLevelCount() const final;
	DimensionVectorType MipmapLevelDimensions(int mipmapLevel) const final;
	constexpr static bool IsIntegerDataFormat(typename InterfaceType::DataFormat dataFormat);
	constexpr static GLenum GetImageFormatNative(typename InterfaceType::ImageFormat imageFormat, typename InterfaceType::DataFormat dataFormat, bool stencilComponent);
	constexpr static GLenum GetDataFormatNative(typename InterfaceType::DataFormat dataFormat, bool stencilComponent);
	constexpr static void GetImageAllocationFormatNative(typename InterfaceType::ImageFormat imageFormat, typename InterfaceType::DataFormat dataFormat, GLenum& nativeImageFormat, GLenum& nativeDataFormat);
	constexpr static void GetOptimalSourceFormat(typename InterfaceType::ImageFormat imageFormat, typename InterfaceType::DataFormat dataFormat, typename InterfaceType::ImageFormat& selectedImageFormat, typename InterfaceType::DataFormat& selectedDataFormat, typename InterfaceType::SourceImageFormat& sourceImageFormat, typename InterfaceType::SourceDataFormat& sourceDataFormat, GLenum& nativeDataFormat, GLenum& nativeDataType, bool stencilComponent);

	// Initial data methods
	using InterfaceType::SetInitialData;
	// NOLINTNEXTLINE(hicpp-use-override, modernize-use-override)
	virtual SuccessToken SetInitialData(const void* sourceBuffer, size_t sourceBufferSizeInBytes, typename InterfaceType::SourceImageFormat imageFormat, typename InterfaceType::SourceDataFormat dataFormat, typename InterfaceType::CubeMapFace targetFace, int mipmapLevel) final;
	// NOLINTNEXTLINE(hicpp-use-override, modernize-use-override)
	virtual SuccessToken SetInitialData(const void* sourceBuffer, size_t sourceBufferSizeInBytes, typename InterfaceType::SourceImageFormat imageFormat, typename InterfaceType::SourceDataFormat dataFormat, typename InterfaceType::CubeMapFace targetFace, size_t arrayIndex, int mipmapLevel) final;
	// NOLINTNEXTLINE(hicpp-use-override, modernize-use-override)
	virtual SuccessToken SetInitialData(const void* sourceBuffer, size_t sourceBufferSizeInBytes, typename InterfaceType::SourceImageFormat imageFormat, typename InterfaceType::SourceDataFormat dataFormat, int mipmapLevel) final;
	// NOLINTNEXTLINE(hicpp-use-override, modernize-use-override)
	virtual SuccessToken SetInitialData(const void* sourceBuffer, size_t sourceBufferSizeInBytes, typename InterfaceType::SourceImageFormat imageFormat, typename InterfaceType::SourceDataFormat dataFormat, size_t arrayIndex, int mipmapLevel) final;

	// Data update methods
	using InterfaceType::QueueDataUpdate;
	// NOLINTNEXTLINE(hicpp-use-override, modernize-use-override)
	virtual SuccessToken QueueDataUpdate(const void* sourceBuffer, size_t sourceBufferSizeInBytes, typename InterfaceType::SourceImageFormat imageFormat, typename InterfaceType::SourceDataFormat dataFormat, typename InterfaceType::CubeMapFace targetFace, int mipmapLevel, const DimensionVectorType& imageOffsetInPixels, const DimensionVectorType& imageRegionInPixels, ITransferBatch* transferBatch) final;
	// NOLINTNEXTLINE(hicpp-use-override, modernize-use-override)
	virtual SuccessToken QueueDataUpdate(const void* sourceBuffer, size_t sourceBufferSizeInBytes, typename InterfaceType::SourceImageFormat imageFormat, typename InterfaceType::SourceDataFormat dataFormat, typename InterfaceType::CubeMapFace targetFace, size_t arrayIndex, int mipmapLevel, const DimensionVectorType& imageOffsetInPixels, const DimensionVectorType& imageRegionInPixels, ITransferBatch* transferBatch) final;
	// NOLINTNEXTLINE(hicpp-use-override, modernize-use-override)
	virtual SuccessToken QueueDataUpdate(const void* sourceBuffer, size_t sourceBufferSizeInBytes, typename InterfaceType::SourceImageFormat imageFormat, typename InterfaceType::SourceDataFormat dataFormat, int mipmapLevel, const DimensionVectorType& imageOffsetInPixels, const DimensionVectorType& imageRegionInPixels, ITransferBatch* transferBatch) final;
	// NOLINTNEXTLINE(hicpp-use-override, modernize-use-override)
	virtual SuccessToken QueueDataUpdate(const void* sourceBuffer, size_t sourceBufferSizeInBytes, typename InterfaceType::SourceImageFormat imageFormat, typename InterfaceType::SourceDataFormat dataFormat, size_t arrayIndex, int mipmapLevel, const DimensionVectorType& imageOffsetInPixels, const DimensionVectorType& imageRegionInPixels, ITransferBatch* transferBatch) final;
	void CompletePendingDataWrites();

	// Build state methods
	void MigrateBuildStateToDrawState();
	GLuint GetTextureNo() const;

protected:
	// Structures
	struct PendingWrite
	{
		PendingWrite(size_t arrayIndex, int mipmapLevel, DimensionVectorType imageOffsetInPixels, DimensionVectorType imageRegionInPixels, OpenGLTransferBatch* transferBatch)
		: arrayIndex(arrayIndex), mipmapLevel(mipmapLevel), imageOffsetInPixels(imageOffsetInPixels), imageRegionInPixels(imageRegionInPixels), transferBatch(transferBatch)
		{}

		GLenum nativeImageFormat = {};
		GLenum nativeDataFormat = {};
		size_t arrayIndex;
		int mipmapLevel;
		DimensionVectorType imageOffsetInPixels;
		DimensionVectorType imageRegionInPixels;
		std::vector<uint8_t> data;
		GLenum stencilNativeImageFormat = {};
		GLenum stencilNativeDataFormat = {};
		std::vector<uint8_t> convertedStencilData;
		OpenGLTransferBatch* transferBatch;
	};
	struct InitialDataEntry
	{
		InitialDataEntry() = default;
		InitialDataEntry(typename InterfaceType::SourceImageFormat imageFormat, typename InterfaceType::SourceDataFormat dataFormat, size_t arrayIndex, int mipmapLevel)
		: imageFormat(imageFormat), dataFormat(dataFormat), arrayIndex(arrayIndex), mipmapLevel(mipmapLevel)
		{}

		typename InterfaceType::SourceImageFormat imageFormat = InterfaceType::SourceImageFormat::R;
		typename InterfaceType::SourceDataFormat dataFormat = InterfaceType::SourceDataFormat::UInt8;
		GLenum nativeImageFormat = {};
		GLenum nativeDataFormat = {};
		size_t arrayIndex = 0;
		int mipmapLevel = 0;
		const void* data = nullptr;
		size_t dataSizeInBytes = 0;
		std::vector<uint8_t> convertedData;
		GLenum stencilNativeImageFormat = {};
		GLenum stencilNativeDataFormat = {};
		std::vector<uint8_t> convertedStencilData;
	};

protected:
	// Initialization methods
	void ReleaseMemory();

	// Data update methods
	virtual bool CompletePendingDataWrite(const PendingWrite& pendingWrite) = 0;

	// Data conversion methods
	bool ConvertDataFormat(const void* sourceBuffer, size_t sourceBufferSizeInBytes, typename InterfaceType::SourceImageFormat sourceImageFormat, typename InterfaceType::SourceDataFormat sourceDataFormat, typename InterfaceType::ImageFormat targetImageFormat, typename InterfaceType::DataFormat targetDataFormat, std::vector<uint8_t>& targetBuffer, std::vector<uint8_t>& stencilTargetBuffer) const;

	// Build state methods
	virtual void FlagObjectModified() = 0;
	virtual bool CreateTextureObject(typename InterfaceType::ImageFormat imageFormat, typename InterfaceType::DataFormat dataFormat, GLint internalFormat, const std::vector<InitialDataEntry>& initialData) = 0;
	OpenGLRenderer* Renderer() const;
	cobalt::logging::ILogger* Log() const;

private:
	// Structures
	struct MutableState
	{
		std::vector<PendingWrite> pendingWrites;
	};

private:
	// Format methods
	static int CalculateMipmapLevelCount(const V1UInt32& imageDimensions);
	static int CalculateMipmapLevelCount(const V2UInt32& imageDimensions);
	static int CalculateMipmapLevelCount(const V3UInt32& imageDimensions);
	static V1UInt32 CalculateMipmapLevelDimensions(const V1UInt32& parentLevelDimensions);
	static V2UInt32 CalculateMipmapLevelDimensions(const V2UInt32& parentLevelDimensions);
	static V3UInt32 CalculateMipmapLevelDimensions(const V3UInt32& parentLevelDimensions);
	static uint32_t GetSmallestImageDimension(const V1UInt32& imageDimensions);
	static uint32_t GetSmallestImageDimension(const V2UInt32& imageDimensions);
	static uint32_t GetSmallestImageDimension(const V3UInt32& imageDimensions);
	static bool RegionIsWithinImageDimensions(const V1UInt32& imageDimensions, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels);
	static bool RegionIsWithinImageDimensions(const V2UInt32& imageDimensions, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels);
	static bool RegionIsWithinImageDimensions(const V3UInt32& imageDimensions, const V3UInt32& imageOffsetInPixels, const V3UInt32& imageRegionInPixels);
	static std::string ImageDimensionAsString(const V1UInt32& imageDimensions);
	static std::string ImageDimensionAsString(const V2UInt32& imageDimensions);
	static std::string ImageDimensionAsString(const V3UInt32& imageDimensions);
	constexpr static bool GetFormatNative(typename InterfaceType::ImageFormat requestedImageFormat, typename InterfaceType::DataFormat requestedDataFormat, GLint& internalFormat);
	void AdjustSourceDataTargetFormat(typename InterfaceType::SourceImageFormat sourceImageFormat, typename InterfaceType::SourceDataFormat sourceDataFormat, typename InterfaceType::ImageFormat targetImageFormat, typename InterfaceType::DataFormat targetDataFormat, typename InterfaceType::ImageFormat& newTargetImageFormat, typename InterfaceType::DataFormat& newTargetDataFormat);

	// Build state methods
	void FlagBuildStateModified();
	void CreateNativeBuffer();

private:
	cobalt::logging::ILogger* _log = nullptr;
	OpenGLRenderer* _renderer = nullptr;
	typename InterfaceType::UsageFlags _usageFlags;
	typename InterfaceType::PerformanceHint _performanceHintCpu;
	typename InterfaceType::PerformanceHint _performanceHintGpu;
	typename InterfaceType::DataPersistenceFlags _dataPersistenceFlags;
	typename InterfaceType::SampleCount _sampleCount = InterfaceType::SampleCount::SampleCount1;
	bool _isMemoryAllocated = false;
	bool _isInitialDataSet = false;
	typename InterfaceType::ImageFormat _requestedImageFormat = {};
	typename InterfaceType::DataFormat _requestedDataFormat = {};
	typename InterfaceType::ImageFormat _imageFormat = {};
	typename InterfaceType::DataFormat _dataFormat = {};
	GLenum _textureTarget = {};
	GLuint _textureNo = {};
	GLint _internalFormat = {};
	size_t _elementCount = {};
	size_t _elementSizeInBytes = {};
	size_t _pixelOffsetInBytes = {};
	size_t _pixelStrideInBytes = {};
	std::mutex _buildStateMutex;
	std::atomic_flag _stateModified = ATOMIC_FLAG_INIT;
	uint32_t _buildIndex = 0;
	uint32_t _drawIndex = 1;
	MutableState _state[2];
	int _mipmapLevels = 0;
	size_t _arraySize = 0;
	bool _formatSet = false;
	bool _isCompressedTexture = false;
	bool _nativeBufferCreationPending = false;
	std::vector<DimensionVectorType> _mipmapDimensions;
	std::vector<InitialDataEntry> _initialData;
};

} // namespace cobalt::graphics
#include "OpenGLTextureBuffer.inl"
