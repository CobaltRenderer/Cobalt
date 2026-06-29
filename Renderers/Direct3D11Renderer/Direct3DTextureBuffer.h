// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Direct3DHeaders.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <atomic>
#include <mutex>
namespace cobalt::graphics {
class Direct3DRenderer;
class Direct3DTransferBatch;

template<class InterfaceType, class DimensionVectorType>
class Direct3DTextureBuffer : public InterfaceType
{
public:
	// Constructors
	Direct3DTextureBuffer(cobalt::logging::ILogger* log, Direct3DRenderer* renderer);

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
	void GetTextureFormatInfo(ITextureBuffer::ImageFormat& imageFormat, ITextureBuffer::DataFormat& dataFormat, size_t& elementCount, size_t& elementSizeInBytes, size_t& pixelOffsetInBytes, size_t& pixelStrideInBytes, bool stencilComponent) const;
	DXGI_FORMAT GetNativeTextureFormat() const;
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
	static UINT GetNativeSampleCountFromSampleCount(typename InterfaceType::SampleCount sampleCount);
	typename InterfaceType::ImageFormat AllocatedImageFormat() const final;
	typename InterfaceType::DataFormat AllocatedDataFormat() const final;
	size_t ArraySize() const;
	int MipmapLevelCount() const final;
	DimensionVectorType MipmapLevelDimensions(int mipmapLevel) const final;

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
	void CompletePendingDataWrites(ID3D11Device1* device, ID3D11DeviceContext1* context);

	// Build state methods
	void MigrateBuildStateToDrawState();
	ID3D11ShaderResourceView* GetShaderResourceView(ID3D11Device1* device);

protected:
	// Structures
	struct PendingWrite
	{
		PendingWrite(size_t arrayIndex, int mipmapLevel, DimensionVectorType imageOffsetInPixels, DimensionVectorType imageRegionInPixels, Direct3DTransferBatch* transferBatch)
		: arrayIndex(arrayIndex), mipmapLevel(mipmapLevel), imageOffsetInPixels(imageOffsetInPixels), imageRegionInPixels(imageRegionInPixels), transferBatch(transferBatch)
		{}

		size_t arrayIndex = 0;
		int mipmapLevel = 0;
		DimensionVectorType imageOffsetInPixels;
		DimensionVectorType imageRegionInPixels;
		std::vector<uint8_t> data;
		Direct3DTransferBatch* transferBatch;
	};
	struct InitialDataEntry
	{
		InitialDataEntry() = default;
		InitialDataEntry(typename InterfaceType::SourceImageFormat imageFormat, typename InterfaceType::SourceDataFormat dataFormat, size_t arrayIndex, int mipmapLevel)
		: imageFormat(imageFormat), dataFormat(dataFormat), arrayIndex(arrayIndex), mipmapLevel(mipmapLevel)
		{}

		typename InterfaceType::SourceImageFormat imageFormat = InterfaceType::SourceImageFormat::R;
		typename InterfaceType::SourceDataFormat dataFormat = InterfaceType::SourceDataFormat::UInt8;
		size_t arrayIndex = 0;
		int mipmapLevel = 0;
		const void* data = nullptr;
		size_t dataSizeInBytes = 0;
		std::vector<uint8_t> convertedData;
	};

protected:
	// Initialization methods
	void ReleaseMemory();
	virtual void ReleaseMemoryInternal() = 0;

	// Format methods
	constexpr static bool GetFormatNative(typename InterfaceType::ImageFormat requestedImageFormat, typename InterfaceType::DataFormat requestedDataFormat, typename InterfaceType::ImageFormat& selectedImageFormat, typename InterfaceType::DataFormat& selectedDataFormat, DXGI_FORMAT& nativeFormat, size_t& elementCount, size_t& elementSizeInBytes, size_t& pixelOffsetInBytes, size_t& pixelStrideInBytes, bool setElementParams = false);
	constexpr static void CalculateBufferSettings(typename InterfaceType::UsageFlags usageFlags, typename InterfaceType::ImageFormat imageFormat, typename InterfaceType::PerformanceHint performanceHintCpu, typename InterfaceType::PerformanceHint performanceHintGpu, bool initialDataSet, UINT& bindFlags, UINT& cpuFlags, D3D11_USAGE& usageType);
	constexpr static UINT CalculateSysMemPitch(typename InterfaceType::ImageFormat imageFormat, typename InterfaceType::DataFormat dataFormat, unsigned int width);
	constexpr static void CalculateSysMemPitchAndDepthPitch(typename InterfaceType::ImageFormat imageFormat, typename InterfaceType::DataFormat dataFormat, unsigned int width, unsigned int height, UINT& sysMemPitch, UINT& depthPitch);

	// Data update methods
	virtual bool CompletePendingDataWrite(const PendingWrite& pendingWrite, ID3D11Device1* device, ID3D11DeviceContext1* context) = 0;

	// Data conversion methods
	bool ConvertDataFormat(const void* sourceBuffer, size_t sourceBufferSizeInBytes, typename InterfaceType::SourceImageFormat sourceImageFormat, typename InterfaceType::SourceDataFormat sourceDataFormat, typename InterfaceType::ImageFormat targetImageFormat, typename InterfaceType::DataFormat targetDataFormat, std::vector<uint8_t>& targetBuffer) const;

	// Build state methods
	virtual void FlagObjectModified() = 0;
	virtual bool CreateTextureObject(typename InterfaceType::ImageFormat imageFormat, typename InterfaceType::DataFormat dataFormat, DXGI_FORMAT nativeFormat, UINT bindFlags, UINT cpuFlags, D3D11_USAGE usageType, const std::vector<InitialDataEntry>& initialData) = 0;
	virtual ID3D11Resource* GetTextureAsResource() const = 0;
	Direct3DRenderer* Renderer() const;
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

	// Build state methods
	void FlagBuildStateModified();

private:
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> _resourceView;
	cobalt::logging::ILogger* _log = nullptr;
	Direct3DRenderer* _renderer = nullptr;
	typename InterfaceType::UsageFlags _usageFlags;
	typename InterfaceType::PerformanceHint _performanceHintCpu;
	typename InterfaceType::PerformanceHint _performanceHintGpu;
	typename InterfaceType::DataPersistenceFlags _dataPersistenceFlags;
	typename InterfaceType::SampleCount _sampleCount = InterfaceType::SampleCount::SampleCount1;
	bool _createdResourceView = false;
	bool _isMemoryAllocated = false;
	bool _isInitialDataSet = false;
	typename InterfaceType::ImageFormat _requestedImageFormat = {};
	typename InterfaceType::DataFormat _requestedDataFormat = {};
	typename InterfaceType::ImageFormat _imageFormat = {};
	typename InterfaceType::DataFormat _dataFormat = {};
	DXGI_FORMAT _internalFormat = {};
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
	std::vector<DimensionVectorType> _mipmapDimensions;
	std::vector<InitialDataEntry> _initialData;
};

} // namespace cobalt::graphics
#include "Direct3DTextureBuffer.inl"
