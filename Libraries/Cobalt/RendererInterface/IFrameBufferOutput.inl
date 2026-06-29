// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
namespace cobalt { namespace graphics {

//----------------------------------------------------------------------------------------
// Data methods
//----------------------------------------------------------------------------------------
template<class T>
SuccessToken IFrameBufferOutput::ReadBufferData(std::vector<T>& targetBuffer, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels) const
{
	size_t bufferSize = CalculatePixelCountForRegion(imageOffsetInPixels, imageRegionInPixels);
	targetBuffer.resize(bufferSize);
	if (!ReadBufferData(targetBuffer.data(), bufferSize, imageOffsetInPixels, imageRegionInPixels))
	{
		targetBuffer.clear();
		return false;
	}
	return true;
}

//----------------------------------------------------------------------------------------
SuccessToken IFrameBufferOutput::ReadBufferData(V1Int8* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::Int8, imageOffsetInPixels, imageRegionInPixels);
}

//----------------------------------------------------------------------------------------
SuccessToken IFrameBufferOutput::ReadBufferData(V2Int8* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::Int8, imageOffsetInPixels, imageRegionInPixels);
}

//----------------------------------------------------------------------------------------
SuccessToken IFrameBufferOutput::ReadBufferData(V3Int8* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::Int8, imageOffsetInPixels, imageRegionInPixels);
}

//----------------------------------------------------------------------------------------
SuccessToken IFrameBufferOutput::ReadBufferData(V4Int8* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::Int8, imageOffsetInPixels, imageRegionInPixels);
}

//----------------------------------------------------------------------------------------
SuccessToken IFrameBufferOutput::ReadBufferData(V1Int16* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::Int16, imageOffsetInPixels, imageRegionInPixels);
}

//----------------------------------------------------------------------------------------
SuccessToken IFrameBufferOutput::ReadBufferData(V2Int16* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::Int16, imageOffsetInPixels, imageRegionInPixels);
}

//----------------------------------------------------------------------------------------
SuccessToken IFrameBufferOutput::ReadBufferData(V3Int16* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::Int16, imageOffsetInPixels, imageRegionInPixels);
}

//----------------------------------------------------------------------------------------
SuccessToken IFrameBufferOutput::ReadBufferData(V4Int16* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::Int16, imageOffsetInPixels, imageRegionInPixels);
}

//----------------------------------------------------------------------------------------
SuccessToken IFrameBufferOutput::ReadBufferData(V1Int32* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::Int32, imageOffsetInPixels, imageRegionInPixels);
}

//----------------------------------------------------------------------------------------
SuccessToken IFrameBufferOutput::ReadBufferData(V2Int32* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::Int32, imageOffsetInPixels, imageRegionInPixels);
}

//----------------------------------------------------------------------------------------
SuccessToken IFrameBufferOutput::ReadBufferData(V3Int32* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::Int32, imageOffsetInPixels, imageRegionInPixels);
}

//----------------------------------------------------------------------------------------
SuccessToken IFrameBufferOutput::ReadBufferData(V4Int32* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::Int32, imageOffsetInPixels, imageRegionInPixels);
}

//----------------------------------------------------------------------------------------
SuccessToken IFrameBufferOutput::ReadBufferData(V1UInt8* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::UInt8, imageOffsetInPixels, imageRegionInPixels);
}

//----------------------------------------------------------------------------------------
SuccessToken IFrameBufferOutput::ReadBufferData(V2UInt8* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::UInt8, imageOffsetInPixels, imageRegionInPixels);
}

//----------------------------------------------------------------------------------------
SuccessToken IFrameBufferOutput::ReadBufferData(V3UInt8* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::UInt8, imageOffsetInPixels, imageRegionInPixels);
}

//----------------------------------------------------------------------------------------
SuccessToken IFrameBufferOutput::ReadBufferData(V4UInt8* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::UInt8, imageOffsetInPixels, imageRegionInPixels);
}

//----------------------------------------------------------------------------------------
SuccessToken IFrameBufferOutput::ReadBufferData(V1UInt16* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::UInt16, imageOffsetInPixels, imageRegionInPixels);
}

//----------------------------------------------------------------------------------------
SuccessToken IFrameBufferOutput::ReadBufferData(V2UInt16* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::UInt16, imageOffsetInPixels, imageRegionInPixels);
}

//----------------------------------------------------------------------------------------
SuccessToken IFrameBufferOutput::ReadBufferData(V3UInt16* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::UInt16, imageOffsetInPixels, imageRegionInPixels);
}

//----------------------------------------------------------------------------------------
SuccessToken IFrameBufferOutput::ReadBufferData(V4UInt16* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::UInt16, imageOffsetInPixels, imageRegionInPixels);
}

//----------------------------------------------------------------------------------------
SuccessToken IFrameBufferOutput::ReadBufferData(V1UInt32* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::UInt32, imageOffsetInPixels, imageRegionInPixels);
}

//----------------------------------------------------------------------------------------
SuccessToken IFrameBufferOutput::ReadBufferData(V2UInt32* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::UInt32, imageOffsetInPixels, imageRegionInPixels);
}

//----------------------------------------------------------------------------------------
SuccessToken IFrameBufferOutput::ReadBufferData(V3UInt32* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::UInt32, imageOffsetInPixels, imageRegionInPixels);
}

//----------------------------------------------------------------------------------------
SuccessToken IFrameBufferOutput::ReadBufferData(V4UInt32* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::UInt32, imageOffsetInPixels, imageRegionInPixels);
}

//----------------------------------------------------------------------------------------
SuccessToken IFrameBufferOutput::ReadBufferData(V1Float16* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::Float16, imageOffsetInPixels, imageRegionInPixels);
}

//----------------------------------------------------------------------------------------
SuccessToken IFrameBufferOutput::ReadBufferData(V2Float16* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::Float16, imageOffsetInPixels, imageRegionInPixels);
}

//----------------------------------------------------------------------------------------
SuccessToken IFrameBufferOutput::ReadBufferData(V3Float16* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::Float16, imageOffsetInPixels, imageRegionInPixels);
}

//----------------------------------------------------------------------------------------
SuccessToken IFrameBufferOutput::ReadBufferData(V4Float16* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::Float16, imageOffsetInPixels, imageRegionInPixels);
}

//----------------------------------------------------------------------------------------
SuccessToken IFrameBufferOutput::ReadBufferData(V1Float32* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITextureBuffer::SourceImageFormat::R, ITextureBuffer::SourceDataFormat::Float32, imageOffsetInPixels, imageRegionInPixels);
}

//----------------------------------------------------------------------------------------
SuccessToken IFrameBufferOutput::ReadBufferData(V2Float32* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RG, ITextureBuffer::SourceDataFormat::Float32, imageOffsetInPixels, imageRegionInPixels);
}

//----------------------------------------------------------------------------------------
SuccessToken IFrameBufferOutput::ReadBufferData(V3Float32* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGB, ITextureBuffer::SourceDataFormat::Float32, imageOffsetInPixels, imageRegionInPixels);
}

//----------------------------------------------------------------------------------------
SuccessToken IFrameBufferOutput::ReadBufferData(V4Float32* targetBuffer, size_t targetBufferSize, const V2UInt32& imageOffsetInPixels, const V2UInt32& imageRegionInPixels) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITextureBuffer::SourceImageFormat::RGBA, ITextureBuffer::SourceDataFormat::Float32, imageOffsetInPixels, imageRegionInPixels);
}

}} // namespace cobalt::graphics
