// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include <type_traits>
namespace cobalt { namespace graphics {

//----------------------------------------------------------------------------------------
// Initial data methods
//----------------------------------------------------------------------------------------
template<class T>
SuccessToken ITexelArray::SetInitialData(const std::vector<T>& sourceBuffer)
{
	return SetInitialData(sourceBuffer.data(), sourceBuffer.size());
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::SetInitialData(const V1Int8* sourceBuffer, size_t sourceBufferSize)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::R, ITexelArray::SourceDataFormat::Int8);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::SetInitialData(const V2Int8* sourceBuffer, size_t sourceBufferSize)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::RG, ITexelArray::SourceDataFormat::Int8);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::SetInitialData(const V4Int8* sourceBuffer, size_t sourceBufferSize)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::RGBA, ITexelArray::SourceDataFormat::Int8);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::SetInitialData(const V1Int16* sourceBuffer, size_t sourceBufferSize)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::R, ITexelArray::SourceDataFormat::Int16);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::SetInitialData(const V2Int16* sourceBuffer, size_t sourceBufferSize)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::RG, ITexelArray::SourceDataFormat::Int16);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::SetInitialData(const V4Int16* sourceBuffer, size_t sourceBufferSize)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::RGBA, ITexelArray::SourceDataFormat::Int16);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::SetInitialData(const V1Int32* sourceBuffer, size_t sourceBufferSize)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::R, ITexelArray::SourceDataFormat::Int32);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::SetInitialData(const V2Int32* sourceBuffer, size_t sourceBufferSize)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::RG, ITexelArray::SourceDataFormat::Int32);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::SetInitialData(const V4Int32* sourceBuffer, size_t sourceBufferSize)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::RGBA, ITexelArray::SourceDataFormat::Int32);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::SetInitialData(const V1UInt8* sourceBuffer, size_t sourceBufferSize)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::R, ITexelArray::SourceDataFormat::UInt8);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::SetInitialData(const V2UInt8* sourceBuffer, size_t sourceBufferSize)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::RG, ITexelArray::SourceDataFormat::UInt8);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::SetInitialData(const V4UInt8* sourceBuffer, size_t sourceBufferSize)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::RGBA, ITexelArray::SourceDataFormat::UInt8);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::SetInitialData(const V1UInt16* sourceBuffer, size_t sourceBufferSize)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::R, ITexelArray::SourceDataFormat::UInt16);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::SetInitialData(const V2UInt16* sourceBuffer, size_t sourceBufferSize)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::RG, ITexelArray::SourceDataFormat::UInt16);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::SetInitialData(const V4UInt16* sourceBuffer, size_t sourceBufferSize)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::RGBA, ITexelArray::SourceDataFormat::UInt16);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::SetInitialData(const V1UInt32* sourceBuffer, size_t sourceBufferSize)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::R, ITexelArray::SourceDataFormat::UInt32);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::SetInitialData(const V2UInt32* sourceBuffer, size_t sourceBufferSize)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::RG, ITexelArray::SourceDataFormat::UInt32);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::SetInitialData(const V4UInt32* sourceBuffer, size_t sourceBufferSize)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::RGBA, ITexelArray::SourceDataFormat::UInt32);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::SetInitialData(const V1Norm8* sourceBuffer, size_t sourceBufferSize)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::R, ITexelArray::SourceDataFormat::Norm8);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::SetInitialData(const V2Norm8* sourceBuffer, size_t sourceBufferSize)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::RG, ITexelArray::SourceDataFormat::Norm8);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::SetInitialData(const V4Norm8* sourceBuffer, size_t sourceBufferSize)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::RGBA, ITexelArray::SourceDataFormat::Norm8);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::SetInitialData(const V1Norm16* sourceBuffer, size_t sourceBufferSize)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::R, ITexelArray::SourceDataFormat::Norm16);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::SetInitialData(const V2Norm16* sourceBuffer, size_t sourceBufferSize)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::RG, ITexelArray::SourceDataFormat::Norm16);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::SetInitialData(const V4Norm16* sourceBuffer, size_t sourceBufferSize)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::RGBA, ITexelArray::SourceDataFormat::Norm16);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::SetInitialData(const V1UNorm8* sourceBuffer, size_t sourceBufferSize)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::R, ITexelArray::SourceDataFormat::UNorm8);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::SetInitialData(const V2UNorm8* sourceBuffer, size_t sourceBufferSize)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::RG, ITexelArray::SourceDataFormat::UNorm8);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::SetInitialData(const V4UNorm8* sourceBuffer, size_t sourceBufferSize)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::RGBA, ITexelArray::SourceDataFormat::UNorm8);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::SetInitialData(const V1UNorm16* sourceBuffer, size_t sourceBufferSize)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::R, ITexelArray::SourceDataFormat::UNorm16);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::SetInitialData(const V2UNorm16* sourceBuffer, size_t sourceBufferSize)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::RG, ITexelArray::SourceDataFormat::UNorm16);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::SetInitialData(const V4UNorm16* sourceBuffer, size_t sourceBufferSize)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::RGBA, ITexelArray::SourceDataFormat::UNorm16);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::SetInitialData(const V1Float16* sourceBuffer, size_t sourceBufferSize)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::R, ITexelArray::SourceDataFormat::Float16);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::SetInitialData(const V2Float16* sourceBuffer, size_t sourceBufferSize)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::RG, ITexelArray::SourceDataFormat::Float16);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::SetInitialData(const V4Float16* sourceBuffer, size_t sourceBufferSize)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::RGBA, ITexelArray::SourceDataFormat::Float16);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::SetInitialData(const V1Float32* sourceBuffer, size_t sourceBufferSize)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::R, ITexelArray::SourceDataFormat::Float32);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::SetInitialData(const V2Float32* sourceBuffer, size_t sourceBufferSize)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::RG, ITexelArray::SourceDataFormat::Float32);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::SetInitialData(const V4Float32* sourceBuffer, size_t sourceBufferSize)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return SetInitialData((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::RGBA, ITexelArray::SourceDataFormat::Float32);
}

//----------------------------------------------------------------------------------------
// Data update methods
//----------------------------------------------------------------------------------------
template<class T>
SuccessToken ITexelArray::QueueDataUpdate(const std::vector<T>& sourceBuffer, size_t targetBufferOffset, ITransferBatch* transferBatch)
{
	return QueueDataUpdate(sourceBuffer.data(), sourceBuffer.size(), targetBufferOffset, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::QueueDataUpdate(const V1Int8* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::R, ITexelArray::SourceDataFormat::Int8, targetBufferOffset, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::QueueDataUpdate(const V2Int8* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::RG, ITexelArray::SourceDataFormat::Int8, targetBufferOffset, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::QueueDataUpdate(const V4Int8* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::RGBA, ITexelArray::SourceDataFormat::Int8, targetBufferOffset, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::QueueDataUpdate(const V1Int16* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::R, ITexelArray::SourceDataFormat::Int16, targetBufferOffset, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::QueueDataUpdate(const V2Int16* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::RG, ITexelArray::SourceDataFormat::Int16, targetBufferOffset, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::QueueDataUpdate(const V4Int16* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::RGBA, ITexelArray::SourceDataFormat::Int16, targetBufferOffset, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::QueueDataUpdate(const V1Int32* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::R, ITexelArray::SourceDataFormat::Int32, targetBufferOffset, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::QueueDataUpdate(const V2Int32* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::RG, ITexelArray::SourceDataFormat::Int32, targetBufferOffset, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::QueueDataUpdate(const V4Int32* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::RGBA, ITexelArray::SourceDataFormat::Int32, targetBufferOffset, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::QueueDataUpdate(const V1UInt8* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::R, ITexelArray::SourceDataFormat::UInt8, targetBufferOffset, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::QueueDataUpdate(const V2UInt8* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::RG, ITexelArray::SourceDataFormat::UInt8, targetBufferOffset, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::QueueDataUpdate(const V4UInt8* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::RGBA, ITexelArray::SourceDataFormat::UInt8, targetBufferOffset, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::QueueDataUpdate(const V1UInt16* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::R, ITexelArray::SourceDataFormat::UInt16, targetBufferOffset, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::QueueDataUpdate(const V2UInt16* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::RG, ITexelArray::SourceDataFormat::UInt16, targetBufferOffset, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::QueueDataUpdate(const V4UInt16* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::RGBA, ITexelArray::SourceDataFormat::UInt16, targetBufferOffset, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::QueueDataUpdate(const V1UInt32* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::R, ITexelArray::SourceDataFormat::UInt32, targetBufferOffset, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::QueueDataUpdate(const V2UInt32* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::RG, ITexelArray::SourceDataFormat::UInt32, targetBufferOffset, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::QueueDataUpdate(const V4UInt32* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::RGBA, ITexelArray::SourceDataFormat::UInt32, targetBufferOffset, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::QueueDataUpdate(const V1Norm8* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::R, ITexelArray::SourceDataFormat::Norm8, targetBufferOffset, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::QueueDataUpdate(const V2Norm8* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::RG, ITexelArray::SourceDataFormat::Norm8, targetBufferOffset, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::QueueDataUpdate(const V4Norm8* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::RGBA, ITexelArray::SourceDataFormat::Norm8, targetBufferOffset, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::QueueDataUpdate(const V1Norm16* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::R, ITexelArray::SourceDataFormat::Norm16, targetBufferOffset, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::QueueDataUpdate(const V2Norm16* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::RG, ITexelArray::SourceDataFormat::Norm16, targetBufferOffset, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::QueueDataUpdate(const V4Norm16* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::RGBA, ITexelArray::SourceDataFormat::Norm16, targetBufferOffset, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::QueueDataUpdate(const V1UNorm8* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::R, ITexelArray::SourceDataFormat::UNorm8, targetBufferOffset, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::QueueDataUpdate(const V2UNorm8* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::RG, ITexelArray::SourceDataFormat::UNorm8, targetBufferOffset, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::QueueDataUpdate(const V4UNorm8* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::RGBA, ITexelArray::SourceDataFormat::UNorm8, targetBufferOffset, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::QueueDataUpdate(const V1UNorm16* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::R, ITexelArray::SourceDataFormat::UNorm16, targetBufferOffset, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::QueueDataUpdate(const V2UNorm16* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::RG, ITexelArray::SourceDataFormat::UNorm16, targetBufferOffset, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::QueueDataUpdate(const V4UNorm16* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::RGBA, ITexelArray::SourceDataFormat::UNorm16, targetBufferOffset, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::QueueDataUpdate(const V1Float16* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::R, ITexelArray::SourceDataFormat::Float16, targetBufferOffset, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::QueueDataUpdate(const V2Float16* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::RG, ITexelArray::SourceDataFormat::Float16, targetBufferOffset, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::QueueDataUpdate(const V4Float16* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::RGBA, ITexelArray::SourceDataFormat::Float16, targetBufferOffset, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::QueueDataUpdate(const V1Float32* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::R, ITexelArray::SourceDataFormat::Float32, targetBufferOffset, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::QueueDataUpdate(const V2Float32* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::RG, ITexelArray::SourceDataFormat::Float32, targetBufferOffset, transferBatch);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArray::QueueDataUpdate(const V4Float32* sourceBuffer, size_t sourceBufferSize, size_t targetBufferOffset, ITransferBatch* transferBatch)
{
	size_t sourceBufferSizeInBytes = sourceBufferSize * sizeof(*sourceBuffer);
	return QueueDataUpdate((void*)sourceBuffer, sourceBufferSizeInBytes, ITexelArray::SourceImageFormat::RGBA, ITexelArray::SourceDataFormat::Float32, targetBufferOffset, transferBatch);
}

//----------------------------------------------------------------------------------------
// Size methods
//----------------------------------------------------------------------------------------
constexpr size_t ITexelArray::ElementCountPerPixelFromFormat(SourceImageFormat imageFormat)
{
	switch (imageFormat)
	{
	case SourceImageFormat::R:
	case SourceImageFormat::X:
		return 1;
	case SourceImageFormat::RG:
	case SourceImageFormat::XY:
		return 2;
	case SourceImageFormat::RGBA:
	case SourceImageFormat::XYZW:
		return 4;
	}
	UNREACHABLE();
	return {};
}

//----------------------------------------------------------------------------------------
constexpr size_t ITexelArray::ElementCountPerPixelFromFormat(ImageFormat imageFormat)
{
	switch (imageFormat)
	{
	case ImageFormat::R:
	case ImageFormat::X:
		return 1;
	case ImageFormat::RG:
	case ImageFormat::XY:
		return 2;
	case ImageFormat::RGBA:
	case ImageFormat::XYZW:
		return 4;
	}
	UNREACHABLE();
	return {};
}

//----------------------------------------------------------------------------------------
constexpr size_t ITexelArray::ByteSizePerElementFromFormat(SourceDataFormat dataFormat)
{
	switch (dataFormat)
	{
	case SourceDataFormat::Int8:
	case SourceDataFormat::UInt8:
	case SourceDataFormat::Norm8:
	case SourceDataFormat::UNorm8:
		return 1;
	case SourceDataFormat::Int16:
	case SourceDataFormat::UInt16:
	case SourceDataFormat::Norm16:
	case SourceDataFormat::UNorm16:
	case SourceDataFormat::Float16:
		return 2;
	case SourceDataFormat::Int32:
	case SourceDataFormat::UInt32:
	case SourceDataFormat::Norm32:
	case SourceDataFormat::UNorm32:
	case SourceDataFormat::Float32:
		return 4;
	}
	return 0;
}

//----------------------------------------------------------------------------------------
constexpr size_t ITexelArray::ByteSizePerElementFromFormat(DataFormat dataFormat)
{
	switch (dataFormat)
	{
	case DataFormat::Int8:
	case DataFormat::UInt8:
	case DataFormat::Norm8:
	case DataFormat::UNorm8:
		return 1;
	case DataFormat::Int16:
	case DataFormat::UInt16:
	case DataFormat::Float16:
		return 2;
	case DataFormat::Int32:
	case DataFormat::UInt32:
	case DataFormat::Float32:
		return 4;
	}
	return 0;
}

//----------------------------------------------------------------------------------------
// Format methods
//----------------------------------------------------------------------------------------
constexpr bool ITexelArray::ImageFormatsAreBinaryEquivalent(SourceImageFormat sourceImageFormat, ImageFormat imageFormat)
{
	switch (sourceImageFormat)
	{
	case SourceImageFormat::R:
	case SourceImageFormat::X:
		return (imageFormat == ImageFormat::R) || (imageFormat == ImageFormat::X);
	case SourceImageFormat::RG:
	case SourceImageFormat::XY:
		return (imageFormat == ImageFormat::RG) || (imageFormat == ImageFormat::XY);
	case SourceImageFormat::RGBA:
	case SourceImageFormat::XYZW:
		return (imageFormat == ImageFormat::RGBA) || (imageFormat == ImageFormat::XYZW);
	}
	return false;
}

//----------------------------------------------------------------------------------------
constexpr bool ITexelArray::ImageFormatsAreBinaryEquivalent(ImageFormat sourceImageFormat, ImageFormat imageFormat)
{
	switch (sourceImageFormat)
	{
	case ImageFormat::R:
	case ImageFormat::X:
		return (imageFormat == ImageFormat::R) || (imageFormat == ImageFormat::X);
	case ImageFormat::RG:
	case ImageFormat::XY:
		return (imageFormat == ImageFormat::RG) || (imageFormat == ImageFormat::XY);
	case ImageFormat::RGBA:
	case ImageFormat::XYZW:
		return (imageFormat == ImageFormat::RGBA) || (imageFormat == ImageFormat::XYZW);
	}
	return false;
}

//----------------------------------------------------------------------------------------
constexpr bool ITexelArray::DataFormatsAreBinaryEquivalent(SourceDataFormat sourceDataFormat, DataFormat dataFormat)
{
	switch (sourceDataFormat)
	{
	case SourceDataFormat::Int8:
		return dataFormat == DataFormat::Int8;
	case SourceDataFormat::Int16:
		return dataFormat == DataFormat::Int16;
	case SourceDataFormat::Int32:
		return dataFormat == DataFormat::Int32;
	case SourceDataFormat::UNorm8:
	case SourceDataFormat::UInt8:
		return (dataFormat == DataFormat::UInt8) || (dataFormat == DataFormat::UNorm8);
	case SourceDataFormat::UNorm16:
	case SourceDataFormat::UInt16:
		return (dataFormat == DataFormat::UInt16);
	case SourceDataFormat::UInt32:
		return dataFormat == DataFormat::UInt32;
	case SourceDataFormat::Norm8:
		return dataFormat == DataFormat::Norm8;
	case SourceDataFormat::Float16:
		return dataFormat == DataFormat::Float16;
	case SourceDataFormat::Float32:
		return (dataFormat == DataFormat::Float32);
	}
	return false;
}

//----------------------------------------------------------------------------------------
constexpr bool ITexelArray::DataFormatsAreBinaryEquivalent(DataFormat sourceDataFormat, DataFormat dataFormat)
{
	switch (sourceDataFormat)
	{
	case DataFormat::Int8:
		return dataFormat == DataFormat::Int8;
	case DataFormat::Int16:
		return dataFormat == DataFormat::Int16;
	case DataFormat::Int32:
		return dataFormat == DataFormat::Int32;
	case DataFormat::UNorm8:
	case DataFormat::UInt8:
		return (dataFormat == DataFormat::UInt8) || (dataFormat == DataFormat::UNorm8);
	case DataFormat::UInt16:
		return (dataFormat == DataFormat::UInt16);
	case DataFormat::UInt32:
		return dataFormat == DataFormat::UInt32;
	case DataFormat::Norm8:
		return dataFormat == DataFormat::Norm8;
	case DataFormat::Float16:
		return dataFormat == DataFormat::Float16;
	case DataFormat::Float32:
		return (dataFormat == DataFormat::Float32);
	}
	return false;
}

//----------------------------------------------------------------------------------------
// Enumeration operators
//----------------------------------------------------------------------------------------
inline ITexelArray::UsageFlags operator|(ITexelArray::UsageFlags left, ITexelArray::UsageFlags right)
{
	return (ITexelArray::UsageFlags)((std::underlying_type<ITexelArray::UsageFlags>::type)left | (std::underlying_type<ITexelArray::UsageFlags>::type)right);
}

//----------------------------------------------------------------------------------------
inline ITexelArray::UsageFlags& operator|=(ITexelArray::UsageFlags& left, ITexelArray::UsageFlags right)
{
	left = (left | right);
	return left;
}

//----------------------------------------------------------------------------------------
inline ITexelArray::UsageFlags operator&(ITexelArray::UsageFlags left, ITexelArray::UsageFlags right)
{
	return (ITexelArray::UsageFlags)((std::underlying_type<ITexelArray::UsageFlags>::type)left & (std::underlying_type<ITexelArray::UsageFlags>::type)right);
}

//----------------------------------------------------------------------------------------
inline ITexelArray::UsageFlags& operator&=(ITexelArray::UsageFlags& left, ITexelArray::UsageFlags right)
{
	left = (left & right);
	return left;
}

}} // namespace cobalt::graphics
