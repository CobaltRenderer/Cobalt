// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
namespace cobalt { namespace graphics {

//----------------------------------------------------------------------------------------
// Initial data methods
//----------------------------------------------------------------------------------------
template<class T>
SuccessToken ITextureBufferCube::SetInitialData(const std::vector<T>& sourceBuffer, CubeMapFace targetFace, int mipmapLevel)
{
	return SetInitialData(sourceBuffer.data(), sourceBuffer.size(), targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V1Int8* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::Int8, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V2Int8* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::Int8, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V3Int8* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::Int8, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V4Int8* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::Int8, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V1Int16* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::Int16, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V2Int16* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::Int16, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V3Int16* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::Int16, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V4Int16* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::Int16, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V1Int32* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::Int32, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V2Int32* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::Int32, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V3Int32* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::Int32, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V4Int32* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::Int32, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V1UInt8* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::UInt8, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V2UInt8* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::UInt8, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V3UInt8* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::UInt8, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V4UInt8* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::UInt8, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V1UInt16* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::UInt16, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V2UInt16* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::UInt16, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V3UInt16* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::UInt16, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V4UInt16* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::UInt16, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V1UInt32* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::UInt32, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V2UInt32* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::UInt32, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V3UInt32* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::UInt32, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V4UInt32* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::UInt32, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V1Norm8* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::Norm8, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V2Norm8* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::Norm8, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V3Norm8* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::Norm8, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V4Norm8* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::Norm8, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V1Norm16* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::Norm16, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V2Norm16* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::Norm16, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V3Norm16* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::Norm16, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V4Norm16* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::Norm16, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V1Norm32* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::Norm32, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V2Norm32* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::Norm32, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V3Norm32* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::Norm32, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V4Norm32* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::Norm32, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V1UNorm8* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::UNorm8, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V2UNorm8* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::UNorm8, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V3UNorm8* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::UNorm8, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V4UNorm8* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::UNorm8, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V1UNorm16* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::UNorm16, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V2UNorm16* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::UNorm16, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V3UNorm16* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::UNorm16, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V4UNorm16* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::UNorm16, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V1UNorm32* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::UNorm32, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V2UNorm32* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::UNorm32, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V3UNorm32* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::UNorm32, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V4UNorm32* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::UNorm32, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V1Float16* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::Float16, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V2Float16* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::Float16, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V3Float16* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::Float16, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V4Float16* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::Float16, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V1Float32* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::Float32, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V2Float32* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::Float32, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V3Float32* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::Float32, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::SetInitialData(const V4Float32* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::Float32, targetFace, mipmapLevel);
}

//----------------------------------------------------------------------------------------
// Data update methods
//----------------------------------------------------------------------------------------
template<class T>
SuccessToken ITextureBufferCube::QueueDataUpdate(const std::vector<T>& sourceBuffer, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	return QueueDataUpdate(sourceBuffer.data(), sourceBuffer.size(), targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V1Int8* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::Int8, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V2Int8* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::Int8, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V3Int8* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::Int8, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V4Int8* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::Int8, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V1Int16* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::Int16, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V2Int16* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::Int16, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V3Int16* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::Int16, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V4Int16* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::Int16, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V1Int32* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::Int32, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V2Int32* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::Int32, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V3Int32* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::Int32, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V4Int32* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::Int32, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V1UInt8* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::UInt8, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V2UInt8* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::UInt8, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V3UInt8* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::UInt8, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V4UInt8* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::UInt8, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V1UInt16* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::UInt16, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V2UInt16* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::UInt16, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V3UInt16* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::UInt16, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V4UInt16* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::UInt16, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V1UInt32* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::UInt32, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V2UInt32* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::UInt32, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V3UInt32* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::UInt32, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V4UInt32* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::UInt32, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V1Norm8* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::Norm8, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V2Norm8* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::Norm8, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V3Norm8* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::Norm8, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V4Norm8* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::Norm8, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V1Norm16* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::Norm16, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V2Norm16* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::Norm16, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V3Norm16* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::Norm16, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V4Norm16* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::Norm16, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V1Norm32* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::Norm32, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V2Norm32* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::Norm32, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V3Norm32* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::Norm32, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V4Norm32* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::Norm32, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V1UNorm8* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::UNorm8, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V2UNorm8* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::UNorm8, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V3UNorm8* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::UNorm8, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V4UNorm8* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::UNorm8, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V1UNorm16* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::UNorm16, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V2UNorm16* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::UNorm16, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V3UNorm16* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::UNorm16, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V4UNorm16* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::UNorm16, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V1UNorm32* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::UNorm32, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V2UNorm32* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::UNorm32, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V3UNorm32* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::UNorm32, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V4UNorm32* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::UNorm32, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V1Float16* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::Float16, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V2Float16* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::Float16, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V3Float16* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::Float16, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V4Float16* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::Float16, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V1Float32* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::Float32, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V2Float32* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::Float32, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V3Float32* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::Float32, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITextureBufferCube::QueueDataUpdate(const V4Float32* sourceBuffer, size_t sourceBufferSize, CubeMapFace targetFace, int mipmapLevel, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::Float32, targetFace, mipmapLevel, imageOffsetInPixels, imageRegionInPixels, transferBatch);
}

}} // namespace cobalt::graphics
