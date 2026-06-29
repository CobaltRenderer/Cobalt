// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
namespace cobalt { namespace graphics {

//----------------------------------------------------------------------------------------
// Initial data methods
//----------------------------------------------------------------------------------------
template<class T>
SuccessToken ITextureBuffer1D::SetInitialData(const std::vector<T>& sourceBuffer, int mipmapLevel)
{
	return SetInitialData(sourceBuffer.data(), sourceBuffer.size(), mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V1Int8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::Int8, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V2Int8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::Int8, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V3Int8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::Int8, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V4Int8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::Int8, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V1Int16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::Int16, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V2Int16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::Int16, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V3Int16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::Int16, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V4Int16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::Int16, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V1Int32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::Int32, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V2Int32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::Int32, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V3Int32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::Int32, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V4Int32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::Int32, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V1UInt8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::UInt8, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V2UInt8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::UInt8, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V3UInt8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::UInt8, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V4UInt8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::UInt8, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V1UInt16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::UInt16, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V2UInt16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::UInt16, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V3UInt16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::UInt16, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V4UInt16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::UInt16, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V1UInt32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::UInt32, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V2UInt32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::UInt32, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V3UInt32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::UInt32, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V4UInt32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::UInt32, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V1Norm8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::Norm8, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V2Norm8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::Norm8, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V3Norm8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::Norm8, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V4Norm8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::Norm8, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V1Norm16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::Norm16, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V2Norm16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::Norm16, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V3Norm16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::Norm16, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V4Norm16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::Norm16, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V1Norm32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::Norm32, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V2Norm32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::Norm32, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V3Norm32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::Norm32, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V4Norm32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::Norm32, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V1UNorm8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::UNorm8, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V2UNorm8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::UNorm8, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V3UNorm8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::UNorm8, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V4UNorm8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::UNorm8, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V1UNorm16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::UNorm16, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V2UNorm16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::UNorm16, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V3UNorm16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::UNorm16, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V4UNorm16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::UNorm16, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V1UNorm32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::UNorm32, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V2UNorm32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::UNorm32, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V3UNorm32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::UNorm32, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V4UNorm32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::UNorm32, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V1Float16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::Float16, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V2Float16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::Float16, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V3Float16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::Float16, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V4Float16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::Float16, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V1Float32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::Float32, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V2Float32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::Float32, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V3Float32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::Float32, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::SetInitialData(const V4Float32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::Float32, mipmapLevel);
}

//----------------------------------------------------------------------------------------
// Data update methods
//----------------------------------------------------------------------------------------
template<class T>
SuccessToken ITextureBuffer1D::QueueDataUpdate(const std::vector<T>& sourceBuffer, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	return QueueDataUpdate(sourceBuffer.data(), sourceBuffer.size(), mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V1Int8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::Int8, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V2Int8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::Int8, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V3Int8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::Int8, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V4Int8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::Int8, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V1Int16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::Int16, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V2Int16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::Int16, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V3Int16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::Int16, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V4Int16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::Int16, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V1Int32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::Int32, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V2Int32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::Int32, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V3Int32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::Int32, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V4Int32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::Int32, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V1UInt8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::UInt8, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V2UInt8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::UInt8, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V3UInt8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::UInt8, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V4UInt8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::UInt8, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V1UInt16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::UInt16, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V2UInt16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::UInt16, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V3UInt16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::UInt16, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V4UInt16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::UInt16, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V1UInt32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::UInt32, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V2UInt32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::UInt32, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V3UInt32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::UInt32, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V4UInt32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::UInt32, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V1Norm8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::Norm8, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V2Norm8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::Norm8, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V3Norm8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::Norm8, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V4Norm8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::Norm8, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V1Norm16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::Norm16, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V2Norm16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::Norm16, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V3Norm16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::Norm16, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V4Norm16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::Norm16, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V1Norm32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::Norm32, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V2Norm32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::Norm32, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V3Norm32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::Norm32, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V4Norm32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::Norm32, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V1UNorm8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::UNorm8, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V2UNorm8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::UNorm8, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V3UNorm8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::UNorm8, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V4UNorm8* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::UNorm8, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V1UNorm16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::UNorm16, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V2UNorm16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::UNorm16, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V3UNorm16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::UNorm16, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V4UNorm16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::UNorm16, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V1UNorm32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::UNorm32, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V2UNorm32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::UNorm32, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V3UNorm32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::UNorm32, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V4UNorm32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::UNorm32, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V1Float16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::Float16, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V2Float16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::Float16, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V3Float16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::Float16, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V4Float16* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::Float16, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V1Float32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::Float32, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V2Float32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::Float32, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V3Float32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::Float32, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBuffer1D::QueueDataUpdate(const V4Float32* sourceBuffer, size_t sourceBufferSize, int mipmapLevel, const V1UInt32& imageOffsetInPixels, const V1UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::Float32, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

}} // namespace cobalt::graphics
