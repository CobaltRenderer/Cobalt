// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
namespace cobalt { namespace graphics {

//----------------------------------------------------------------------------------------
// Data methods
//----------------------------------------------------------------------------------------
template<class T>
SuccessToken ITexelArrayOutput::ReadBufferData(std::vector<T>& targetBuffer) const
{
	size_t bufferSize = GetEntryCount();
	targetBuffer.resize(bufferSize);
	if (!ReadBufferData(targetBuffer.data(), bufferSize))
	{
		targetBuffer.clear();
		return false;
	}
	return true;
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArrayOutput::ReadBufferData(V1Int8* targetBuffer, size_t targetBufferSize) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITexelArray::SourceImageFormat::R, ITexelArray::SourceDataFormat::Int8);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArrayOutput::ReadBufferData(V2Int8* targetBuffer, size_t targetBufferSize) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITexelArray::SourceImageFormat::RG, ITexelArray::SourceDataFormat::Int8);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArrayOutput::ReadBufferData(V4Int8* targetBuffer, size_t targetBufferSize) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITexelArray::SourceImageFormat::RGBA, ITexelArray::SourceDataFormat::Int8);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArrayOutput::ReadBufferData(V1Int16* targetBuffer, size_t targetBufferSize) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITexelArray::SourceImageFormat::R, ITexelArray::SourceDataFormat::Int16);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArrayOutput::ReadBufferData(V2Int16* targetBuffer, size_t targetBufferSize) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITexelArray::SourceImageFormat::RG, ITexelArray::SourceDataFormat::Int16);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArrayOutput::ReadBufferData(V4Int16* targetBuffer, size_t targetBufferSize) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITexelArray::SourceImageFormat::RGBA, ITexelArray::SourceDataFormat::Int16);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArrayOutput::ReadBufferData(V1Int32* targetBuffer, size_t targetBufferSize) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITexelArray::SourceImageFormat::R, ITexelArray::SourceDataFormat::Int32);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArrayOutput::ReadBufferData(V2Int32* targetBuffer, size_t targetBufferSize) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITexelArray::SourceImageFormat::RG, ITexelArray::SourceDataFormat::Int32);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArrayOutput::ReadBufferData(V4Int32* targetBuffer, size_t targetBufferSize) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITexelArray::SourceImageFormat::RGBA, ITexelArray::SourceDataFormat::Int32);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArrayOutput::ReadBufferData(V1UInt8* targetBuffer, size_t targetBufferSize) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITexelArray::SourceImageFormat::R, ITexelArray::SourceDataFormat::UInt8);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArrayOutput::ReadBufferData(V2UInt8* targetBuffer, size_t targetBufferSize) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITexelArray::SourceImageFormat::RG, ITexelArray::SourceDataFormat::UInt8);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArrayOutput::ReadBufferData(V4UInt8* targetBuffer, size_t targetBufferSize) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITexelArray::SourceImageFormat::RGBA, ITexelArray::SourceDataFormat::UInt8);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArrayOutput::ReadBufferData(V1UInt16* targetBuffer, size_t targetBufferSize) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITexelArray::SourceImageFormat::R, ITexelArray::SourceDataFormat::UInt16);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArrayOutput::ReadBufferData(V2UInt16* targetBuffer, size_t targetBufferSize) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITexelArray::SourceImageFormat::RG, ITexelArray::SourceDataFormat::UInt16);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArrayOutput::ReadBufferData(V4UInt16* targetBuffer, size_t targetBufferSize) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITexelArray::SourceImageFormat::RGBA, ITexelArray::SourceDataFormat::UInt16);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArrayOutput::ReadBufferData(V1UInt32* targetBuffer, size_t targetBufferSize) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITexelArray::SourceImageFormat::R, ITexelArray::SourceDataFormat::UInt32);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArrayOutput::ReadBufferData(V2UInt32* targetBuffer, size_t targetBufferSize) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITexelArray::SourceImageFormat::RG, ITexelArray::SourceDataFormat::UInt32);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArrayOutput::ReadBufferData(V4UInt32* targetBuffer, size_t targetBufferSize) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITexelArray::SourceImageFormat::RGBA, ITexelArray::SourceDataFormat::UInt32);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArrayOutput::ReadBufferData(V1Float16* targetBuffer, size_t targetBufferSize) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITexelArray::SourceImageFormat::R, ITexelArray::SourceDataFormat::Float16);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArrayOutput::ReadBufferData(V2Float16* targetBuffer, size_t targetBufferSize) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITexelArray::SourceImageFormat::RG, ITexelArray::SourceDataFormat::Float16);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArrayOutput::ReadBufferData(V4Float16* targetBuffer, size_t targetBufferSize) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITexelArray::SourceImageFormat::RGBA, ITexelArray::SourceDataFormat::Float16);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArrayOutput::ReadBufferData(V1Float32* targetBuffer, size_t targetBufferSize) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITexelArray::SourceImageFormat::R, ITexelArray::SourceDataFormat::Float32);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArrayOutput::ReadBufferData(V2Float32* targetBuffer, size_t targetBufferSize) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITexelArray::SourceImageFormat::RG, ITexelArray::SourceDataFormat::Float32);
}

//----------------------------------------------------------------------------------------
SuccessToken ITexelArrayOutput::ReadBufferData(V4Float32* targetBuffer, size_t targetBufferSize) const
{
	size_t targetBufferSizeInBytes = targetBufferSize * sizeof(*targetBuffer);
	return ReadBufferData((void*)targetBuffer, targetBufferSizeInBytes, ITexelArray::SourceImageFormat::RGBA, ITexelArray::SourceDataFormat::Float32);
}

}} // namespace cobalt::graphics
