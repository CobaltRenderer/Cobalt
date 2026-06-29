// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "TextureFormatConversion.h"
#include <cstring>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Texture load format conversion
//----------------------------------------------------------------------------------------
TEXTURE_FORMAT_CONVERSION_EXPORT bool TextureFormatConversion::ConvertTextureInputData(const void* sourceBuffer, size_t sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat sourceImageFormat, ITextureBuffer::SourceDataFormat sourceDataFormat, ITextureBuffer::ImageFormat targetImageFormat, ITextureBuffer::DataFormat targetDataFormat, std::vector<uint8_t>& targetBuffer, bool& sourceDataFormatError, bool& targetDataFormatError)
{
	return ConvertTextureInputDataInternalLevel1(sourceBuffer, sourceBufferSizeInBytes, sourceImageFormat, sourceDataFormat, targetImageFormat, targetDataFormat, targetBuffer, sourceDataFormatError, targetDataFormatError);
}

//----------------------------------------------------------------------------------------
bool TextureFormatConversion::ConvertTextureInputDataInternalLevel1(const void* sourceBuffer, size_t sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat sourceImageFormat, ITextureBuffer::SourceDataFormat sourceDataFormat, ITextureBuffer::ImageFormat targetImageFormat, ITextureBuffer::DataFormat targetDataFormat, std::vector<uint8_t>& targetBuffer, bool& sourceDataFormatError, bool& targetDataFormatError)
{
	// Handle depth/stencil format conversion. Only Float32 inputs are supported for this case. Note that it is unusual
	// to load data into a depth or stencil buffer in this way. Usually the buffer should be left uninitialized, and
	// cleared to a uniform value during the draw process when bound to the framebuffer.
	switch (targetDataFormat)
	{
	case ITextureBuffer::DataFormat::DepthUNorm16:
		if ((sourceDataFormat == ITextureBuffer::SourceDataFormat::Float32) && (sourceImageFormat == ITextureBuffer::SourceImageFormat::X))
		{
			size_t sourceElementSizeInBytes = 4;
			size_t targetElementSizeInBytes = 2;
			size_t sampleCount = (sourceBufferSizeInBytes / sourceElementSizeInBytes);
			targetBuffer.resize(sampleCount * targetElementSizeInBytes);
			ImageConversion::ConvertImageFormatSparse(reinterpret_cast<const ImageConversionTypes::DataVector1<ImageConversionTypes::Float32>*>(sourceBuffer), reinterpret_cast<ImageConversionTypes::DataVector1<ImageConversionTypes::UNorm16>*>(targetBuffer.data()), sampleCount, sourceElementSizeInBytes, targetElementSizeInBytes);
			return true;
		}
		break;
	case ITextureBuffer::DataFormat::DepthUNorm24:
		if ((sourceDataFormat == ITextureBuffer::SourceDataFormat::Float32) && (sourceImageFormat == ITextureBuffer::SourceImageFormat::X))
		{
			size_t sourceElementSizeInBytes = 4;
			size_t targetElementSizeInBytes = 3;
			size_t sampleCount = (sourceBufferSizeInBytes / sourceElementSizeInBytes);
			targetBuffer.resize(sampleCount * targetElementSizeInBytes);
			ImageConversion::ConvertImageFormatSparse(reinterpret_cast<const ImageConversionTypes::DataVector1<ImageConversionTypes::Float32>*>(sourceBuffer), reinterpret_cast<ImageConversionTypes::DataVector1<ImageConversionTypes::UNorm24>*>(targetBuffer.data()), sampleCount, sourceElementSizeInBytes, targetElementSizeInBytes);
			return true;
		}
		break;
	case ITextureBuffer::DataFormat::DepthFloat32:
		if ((sourceDataFormat == ITextureBuffer::SourceDataFormat::Float32) && (sourceImageFormat == ITextureBuffer::SourceImageFormat::X))
		{
			targetBuffer.resize(sourceBufferSizeInBytes);
			std::memcpy(targetBuffer.data(), sourceBuffer, sourceBufferSizeInBytes);
			return true;
		}
		break;
	case ITextureBuffer::DataFormat::DepthUNorm24StencilUInt8:
		if ((sourceDataFormat == ITextureBuffer::SourceDataFormat::Float32) && (sourceImageFormat == ITextureBuffer::SourceImageFormat::XY))
		{
			//DDDDDDDDDDDDDDDDDDDDDDDDSSSSSSSS
			size_t sourceElementSizeInBytes = 8;
			size_t targetElementSizeInBytes = 4;
			size_t sampleCount = (sourceBufferSizeInBytes / sourceElementSizeInBytes);
			targetBuffer.resize(sampleCount * targetElementSizeInBytes);

			// Copy the depth component
			ImageConversion::ConvertImageFormatSparse(reinterpret_cast<const ImageConversionTypes::DataVector1<ImageConversionTypes::Float32>*>(sourceBuffer), reinterpret_cast<ImageConversionTypes::DataVector1<ImageConversionTypes::UNorm24>*>(targetBuffer.data()), sampleCount, sourceElementSizeInBytes, targetElementSizeInBytes);

			// Copy the stencil component
			ImageConversion::ConvertImageFormatSparse(reinterpret_cast<const ImageConversionTypes::DataVector1<ImageConversionTypes::Float32>*>(reinterpret_cast<const unsigned char*>(sourceBuffer) + sizeof(ImageConversionTypes::Float32)), reinterpret_cast<ImageConversionTypes::DataVector1<ImageConversionTypes::UInt8>*>(targetBuffer.data() + sizeof(ImageConversionTypes::UNorm24)), sampleCount, sourceElementSizeInBytes, targetElementSizeInBytes);
			return true;
		}
		break;
	case ITextureBuffer::DataFormat::DepthFloat32StencilUInt8:
		if ((sourceDataFormat == ITextureBuffer::SourceDataFormat::Float32) && (sourceImageFormat == ITextureBuffer::SourceImageFormat::XY))
		{
			//DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD SSSSSSSSXXXXXXXXXXXXXXXXXXXXXXXX
			size_t sourceElementSizeInBytes = 8;
			size_t targetElementSizeInBytes = 8;
			size_t sampleCount = (sourceBufferSizeInBytes / sourceElementSizeInBytes);
			targetBuffer.resize(sampleCount * targetElementSizeInBytes);

			// Copy the depth component
			ImageConversion::ConvertImageFormatSparse(reinterpret_cast<const ImageConversionTypes::DataVector1<ImageConversionTypes::Float32>*>(sourceBuffer), reinterpret_cast<ImageConversionTypes::DataVector1<ImageConversionTypes::Float32>*>(targetBuffer.data()), sampleCount, sourceElementSizeInBytes, targetElementSizeInBytes);

			// Copy the stencil component
			ImageConversion::ConvertImageFormatSparse(reinterpret_cast<const ImageConversionTypes::DataVector1<ImageConversionTypes::UInt8>*>(reinterpret_cast<const unsigned char*>(sourceBuffer) + 4), reinterpret_cast<ImageConversionTypes::DataVector1<ImageConversionTypes::Float32>*>(targetBuffer.data() + 4), sampleCount, sourceElementSizeInBytes, targetElementSizeInBytes);
			return true;
		}
		break;
	}

	// Handle general image format conversion
	switch (sourceDataFormat)
	{
	case ITextureBuffer::SourceDataFormat::Int8:
		return ConvertTextureInputDataInternalLevel2<ImageConversionTypes::Int8>(sourceBuffer, sourceBufferSizeInBytes, sourceImageFormat, sourceDataFormat, targetImageFormat, targetDataFormat, targetBuffer, sourceDataFormatError, targetDataFormatError);
	case ITextureBuffer::SourceDataFormat::Int16:
		return ConvertTextureInputDataInternalLevel2<ImageConversionTypes::Int16>(sourceBuffer, sourceBufferSizeInBytes, sourceImageFormat, sourceDataFormat, targetImageFormat, targetDataFormat, targetBuffer, sourceDataFormatError, targetDataFormatError);
	case ITextureBuffer::SourceDataFormat::Int32:
		return ConvertTextureInputDataInternalLevel2<ImageConversionTypes::Int32>(sourceBuffer, sourceBufferSizeInBytes, sourceImageFormat, sourceDataFormat, targetImageFormat, targetDataFormat, targetBuffer, sourceDataFormatError, targetDataFormatError);
	case ITextureBuffer::SourceDataFormat::UInt8:
		return ConvertTextureInputDataInternalLevel2<ImageConversionTypes::UInt8>(sourceBuffer, sourceBufferSizeInBytes, sourceImageFormat, sourceDataFormat, targetImageFormat, targetDataFormat, targetBuffer, sourceDataFormatError, targetDataFormatError);
	case ITextureBuffer::SourceDataFormat::UInt16:
		return ConvertTextureInputDataInternalLevel2<ImageConversionTypes::UInt16>(sourceBuffer, sourceBufferSizeInBytes, sourceImageFormat, sourceDataFormat, targetImageFormat, targetDataFormat, targetBuffer, sourceDataFormatError, targetDataFormatError);
	case ITextureBuffer::SourceDataFormat::UInt32:
		return ConvertTextureInputDataInternalLevel2<ImageConversionTypes::UInt32>(sourceBuffer, sourceBufferSizeInBytes, sourceImageFormat, sourceDataFormat, targetImageFormat, targetDataFormat, targetBuffer, sourceDataFormatError, targetDataFormatError);
	case ITextureBuffer::SourceDataFormat::Norm8:
		return ConvertTextureInputDataInternalLevel2<ImageConversionTypes::Norm8>(sourceBuffer, sourceBufferSizeInBytes, sourceImageFormat, sourceDataFormat, targetImageFormat, targetDataFormat, targetBuffer, sourceDataFormatError, targetDataFormatError);
	case ITextureBuffer::SourceDataFormat::Norm16:
		return ConvertTextureInputDataInternalLevel2<ImageConversionTypes::Norm16>(sourceBuffer, sourceBufferSizeInBytes, sourceImageFormat, sourceDataFormat, targetImageFormat, targetDataFormat, targetBuffer, sourceDataFormatError, targetDataFormatError);
	case ITextureBuffer::SourceDataFormat::Norm32:
		return ConvertTextureInputDataInternalLevel2<ImageConversionTypes::Norm32>(sourceBuffer, sourceBufferSizeInBytes, sourceImageFormat, sourceDataFormat, targetImageFormat, targetDataFormat, targetBuffer, sourceDataFormatError, targetDataFormatError);
	case ITextureBuffer::SourceDataFormat::UNorm8:
		return ConvertTextureInputDataInternalLevel2<ImageConversionTypes::UNorm8>(sourceBuffer, sourceBufferSizeInBytes, sourceImageFormat, sourceDataFormat, targetImageFormat, targetDataFormat, targetBuffer, sourceDataFormatError, targetDataFormatError);
	case ITextureBuffer::SourceDataFormat::UNorm16:
		return ConvertTextureInputDataInternalLevel2<ImageConversionTypes::UNorm16>(sourceBuffer, sourceBufferSizeInBytes, sourceImageFormat, sourceDataFormat, targetImageFormat, targetDataFormat, targetBuffer, sourceDataFormatError, targetDataFormatError);
	case ITextureBuffer::SourceDataFormat::UNorm32:
		return ConvertTextureInputDataInternalLevel2<ImageConversionTypes::UNorm32>(sourceBuffer, sourceBufferSizeInBytes, sourceImageFormat, sourceDataFormat, targetImageFormat, targetDataFormat, targetBuffer, sourceDataFormatError, targetDataFormatError);
	case ITextureBuffer::SourceDataFormat::Float16:
		return ConvertTextureInputDataInternalLevel2<ImageConversionTypes::Float16>(sourceBuffer, sourceBufferSizeInBytes, sourceImageFormat, sourceDataFormat, targetImageFormat, targetDataFormat, targetBuffer, sourceDataFormatError, targetDataFormatError);
	case ITextureBuffer::SourceDataFormat::Float32:
		return ConvertTextureInputDataInternalLevel2<ImageConversionTypes::Float32>(sourceBuffer, sourceBufferSizeInBytes, sourceImageFormat, sourceDataFormat, targetImageFormat, targetDataFormat, targetBuffer, sourceDataFormatError, targetDataFormatError);
	}
	sourceDataFormatError = true;
	return false;
}

//----------------------------------------------------------------------------------------
template<class ElementType>
bool TextureFormatConversion::ConvertTextureInputDataInternalLevel2(const void* sourceBuffer, size_t sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat sourceImageFormat, ITextureBuffer::SourceDataFormat sourceDataFormat, ITextureBuffer::ImageFormat targetImageFormat, ITextureBuffer::DataFormat targetDataFormat, std::vector<uint8_t>& targetBuffer, bool& sourceDataFormatError, bool& targetDataFormatError)
{
	// Note that we've deliberately consolidated binary equivalent types here. While it would be cleaner to pass our
	// data formats separately from our pixel formats in all cases (IE, using DataVector3 for XYZ instead of PixelRGB),
	// that leads to more unique template instances and makes more work for the compiler, which is already heavily taxed
	// by the number of levels of template calls we have. There's no difference at runtime from combining them here. We
	// keep the XYZW format using DataVector4, as that's critical to ensure the fourth element is not considered an
	// alpha value, and therefore defaulted to 1, when performing conversion from a format that doesn't include alpha.
	switch (sourceImageFormat)
	{
	case ITextureBuffer::SourceImageFormat::R:
	case ITextureBuffer::SourceImageFormat::X:
		return ConvertTextureInputDataInternalLevel3(static_cast<const ImageConversionTypes::PixelR<ElementType>*>(sourceBuffer), sourceBufferSizeInBytes, targetImageFormat, targetDataFormat, targetBuffer, targetDataFormatError);
	case ITextureBuffer::SourceImageFormat::RG:
	case ITextureBuffer::SourceImageFormat::XY:
		return ConvertTextureInputDataInternalLevel3(static_cast<const ImageConversionTypes::PixelRG<ElementType>*>(sourceBuffer), sourceBufferSizeInBytes, targetImageFormat, targetDataFormat, targetBuffer, targetDataFormatError);
	case ITextureBuffer::SourceImageFormat::RGB:
	case ITextureBuffer::SourceImageFormat::XYZ:
		return ConvertTextureInputDataInternalLevel3(static_cast<const ImageConversionTypes::PixelRGB<ElementType>*>(sourceBuffer), sourceBufferSizeInBytes, targetImageFormat, targetDataFormat, targetBuffer, targetDataFormatError);
	case ITextureBuffer::SourceImageFormat::RGBA:
		return ConvertTextureInputDataInternalLevel3(static_cast<const ImageConversionTypes::PixelRGBA<ElementType>*>(sourceBuffer), sourceBufferSizeInBytes, targetImageFormat, targetDataFormat, targetBuffer, targetDataFormatError);
	case ITextureBuffer::SourceImageFormat::BGR:
		return ConvertTextureInputDataInternalLevel3(static_cast<const ImageConversionTypes::PixelBGR<ElementType>*>(sourceBuffer), sourceBufferSizeInBytes, targetImageFormat, targetDataFormat, targetBuffer, targetDataFormatError);
	case ITextureBuffer::SourceImageFormat::BGRA:
		return ConvertTextureInputDataInternalLevel3(static_cast<const ImageConversionTypes::PixelBGRA<ElementType>*>(sourceBuffer), sourceBufferSizeInBytes, targetImageFormat, targetDataFormat, targetBuffer, targetDataFormatError);
	case ITextureBuffer::SourceImageFormat::XYZW:
		return ConvertTextureInputDataInternalLevel3(static_cast<const ImageConversionTypes::DataVector4<ElementType>*>(sourceBuffer), sourceBufferSizeInBytes, targetImageFormat, targetDataFormat, targetBuffer, targetDataFormatError);
	}
	sourceDataFormatError = true;
	return false;
}

//----------------------------------------------------------------------------------------
template<class SourceType>
bool TextureFormatConversion::ConvertTextureInputDataInternalLevel3(const SourceType* sourceBuffer, size_t sourceBufferSizeInBytes, ITextureBuffer::ImageFormat targetImageFormat, ITextureBuffer::DataFormat targetDataFormat, std::vector<uint8_t>& targetBuffer, bool& targetDataFormatError)
{
	switch (targetDataFormat)
	{
	case ITextureBuffer::DataFormat::Int8:
		return ConvertTextureInputDataInternalLevel4<SourceType, ImageConversionTypes::Int8>(sourceBuffer, (sourceBufferSizeInBytes / sizeof(SourceType)), targetImageFormat, targetDataFormat, targetBuffer, targetDataFormatError);
	case ITextureBuffer::DataFormat::Int16:
		return ConvertTextureInputDataInternalLevel4<SourceType, ImageConversionTypes::Int16>(sourceBuffer, (sourceBufferSizeInBytes / sizeof(SourceType)), targetImageFormat, targetDataFormat, targetBuffer, targetDataFormatError);
	case ITextureBuffer::DataFormat::Int32:
		return ConvertTextureInputDataInternalLevel4<SourceType, ImageConversionTypes::Int32>(sourceBuffer, (sourceBufferSizeInBytes / sizeof(SourceType)), targetImageFormat, targetDataFormat, targetBuffer, targetDataFormatError);
	case ITextureBuffer::DataFormat::UInt8:
		return ConvertTextureInputDataInternalLevel4<SourceType, ImageConversionTypes::UInt8>(sourceBuffer, (sourceBufferSizeInBytes / sizeof(SourceType)), targetImageFormat, targetDataFormat, targetBuffer, targetDataFormatError);
	case ITextureBuffer::DataFormat::UInt16:
		return ConvertTextureInputDataInternalLevel4<SourceType, ImageConversionTypes::UInt16>(sourceBuffer, (sourceBufferSizeInBytes / sizeof(SourceType)), targetImageFormat, targetDataFormat, targetBuffer, targetDataFormatError);
	case ITextureBuffer::DataFormat::UInt32:
		return ConvertTextureInputDataInternalLevel4<SourceType, ImageConversionTypes::UInt32>(sourceBuffer, (sourceBufferSizeInBytes / sizeof(SourceType)), targetImageFormat, targetDataFormat, targetBuffer, targetDataFormatError);
	case ITextureBuffer::DataFormat::Norm8:
		return ConvertTextureInputDataInternalLevel4<SourceType, ImageConversionTypes::Norm8>(sourceBuffer, (sourceBufferSizeInBytes / sizeof(SourceType)), targetImageFormat, targetDataFormat, targetBuffer, targetDataFormatError);
	case ITextureBuffer::DataFormat::Norm16:
		return ConvertTextureInputDataInternalLevel4<SourceType, ImageConversionTypes::Norm16>(sourceBuffer, (sourceBufferSizeInBytes / sizeof(SourceType)), targetImageFormat, targetDataFormat, targetBuffer, targetDataFormatError);
	case ITextureBuffer::DataFormat::UNorm8:
		return ConvertTextureInputDataInternalLevel4<SourceType, ImageConversionTypes::UNorm8>(sourceBuffer, (sourceBufferSizeInBytes / sizeof(SourceType)), targetImageFormat, targetDataFormat, targetBuffer, targetDataFormatError);
	case ITextureBuffer::DataFormat::UNorm16:
		return ConvertTextureInputDataInternalLevel4<SourceType, ImageConversionTypes::UNorm16>(sourceBuffer, (sourceBufferSizeInBytes / sizeof(SourceType)), targetImageFormat, targetDataFormat, targetBuffer, targetDataFormatError);
	case ITextureBuffer::DataFormat::Float16:
		return ConvertTextureInputDataInternalLevel4<SourceType, ImageConversionTypes::Float16>(sourceBuffer, (sourceBufferSizeInBytes / sizeof(SourceType)), targetImageFormat, targetDataFormat, targetBuffer, targetDataFormatError);
	case ITextureBuffer::DataFormat::Float32:
		return ConvertTextureInputDataInternalLevel4<SourceType, ImageConversionTypes::Float32>(sourceBuffer, (sourceBufferSizeInBytes / sizeof(SourceType)), targetImageFormat, targetDataFormat, targetBuffer, targetDataFormatError);
	}
	targetDataFormatError = true;
	return false;
}

//----------------------------------------------------------------------------------------
template<class SourceType, class ElementType>
bool TextureFormatConversion::ConvertTextureInputDataInternalLevel4(const SourceType* sourceBuffer, size_t sourceBufferSize, ITextureBuffer::ImageFormat targetImageFormat, ITextureBuffer::DataFormat targetDataFormat, std::vector<uint8_t>& targetBuffer, bool& targetDataFormatError)
{
	// Note that we've deliberately consolidated binary equivalent types here. While it would be cleaner to pass our
	// data formats separately from our pixel formats in all cases (IE, using DataVector3 for XYZ instead of PixelRGB),
	// that leads to more unique template instances and makes more work for the compiler, which is already heavily taxed
	// by the number of levels of template calls we have. There's no difference at runtime from combining them here. We
	// keep the XYZW format using DataVector4, as that's critical to ensure the fourth element is not considered an
	// alpha value, and therefore defaulted to 1, when performing conversion from a format that doesn't include alpha.
	switch (targetImageFormat)
	{
	case ITextureBuffer::ImageFormat::R:
	case ITextureBuffer::ImageFormat::X:
	case ITextureBuffer::ImageFormat::Depth:
		return ConvertTextureInputDataInternalFinal<SourceType, ImageConversionTypes::PixelR<ElementType>>(sourceBuffer, targetBuffer, sourceBufferSize);
	case ITextureBuffer::ImageFormat::RG:
	case ITextureBuffer::ImageFormat::XY:
	case ITextureBuffer::ImageFormat::DepthAndStencil:
		return ConvertTextureInputDataInternalFinal<SourceType, ImageConversionTypes::PixelRG<ElementType>>(sourceBuffer, targetBuffer, sourceBufferSize);
	case ITextureBuffer::ImageFormat::RGB:
	case ITextureBuffer::ImageFormat::XYZ:
		return ConvertTextureInputDataInternalFinal<SourceType, ImageConversionTypes::PixelRGB<ElementType>>(sourceBuffer, targetBuffer, sourceBufferSize);
	case ITextureBuffer::ImageFormat::RGBA:
		return ConvertTextureInputDataInternalFinal<SourceType, ImageConversionTypes::PixelRGBA<ElementType>>(sourceBuffer, targetBuffer, sourceBufferSize);
	case ITextureBuffer::ImageFormat::BGR:
		return ConvertTextureInputDataInternalFinal<SourceType, ImageConversionTypes::PixelBGR<ElementType>>(sourceBuffer, targetBuffer, sourceBufferSize);
	case ITextureBuffer::ImageFormat::BGRA:
		return ConvertTextureInputDataInternalFinal<SourceType, ImageConversionTypes::PixelBGRA<ElementType>>(sourceBuffer, targetBuffer, sourceBufferSize);
	case ITextureBuffer::ImageFormat::XYZW:
		return ConvertTextureInputDataInternalFinal<SourceType, ImageConversionTypes::DataVector4<ElementType>>(sourceBuffer, targetBuffer, sourceBufferSize);
	}
	targetDataFormatError = true;
	return false;
}

//----------------------------------------------------------------------------------------
template<class SourceType, class TargetType>
bool TextureFormatConversion::ConvertTextureInputDataInternalFinal(const SourceType* sourceBuffer, std::vector<uint8_t>& targetBuffer, size_t sampleCount)
{
	targetBuffer.resize(sampleCount * sizeof(TargetType));
	ImageConversion::ConvertImageFormat(sourceBuffer, reinterpret_cast<TargetType*>(targetBuffer.data()), sampleCount);
	return true;
}

//----------------------------------------------------------------------------------------
// Texel array format conversion
//----------------------------------------------------------------------------------------
TEXTURE_FORMAT_CONVERSION_EXPORT bool TextureFormatConversion::ConvertTexelArrayInputData(const void* sourceBuffer, size_t sourceBufferSizeInBytes, ITexelArray::SourceImageFormat sourceImageFormat, ITexelArray::SourceDataFormat sourceDataFormat, ITexelArray::ImageFormat targetImageFormat, ITexelArray::DataFormat targetDataFormat, std::vector<uint8_t>& targetBuffer, bool& sourceDataFormatError, bool& targetDataFormatError)
{
	return ConvertTexelArrayInputDataInternalLevel1(sourceBuffer, sourceBufferSizeInBytes, sourceImageFormat, sourceDataFormat, targetImageFormat, targetDataFormat, targetBuffer, sourceDataFormatError, targetDataFormatError);
}

//----------------------------------------------------------------------------------------
bool TextureFormatConversion::ConvertTexelArrayInputDataInternalLevel1(const void* sourceBuffer, size_t sourceBufferSizeInBytes, ITexelArray::SourceImageFormat sourceImageFormat, ITexelArray::SourceDataFormat sourceDataFormat, ITexelArray::ImageFormat targetImageFormat, ITexelArray::DataFormat targetDataFormat, std::vector<uint8_t>& targetBuffer, bool& sourceDataFormatError, bool& targetDataFormatError)
{
	// Handle general image format conversion
	switch (sourceDataFormat)
	{
	case ITexelArray::SourceDataFormat::Int8:
		return ConvertTexelArrayInputDataInternalLevel2<ImageConversionTypes::Int8>(sourceBuffer, sourceBufferSizeInBytes, sourceImageFormat, sourceDataFormat, targetImageFormat, targetDataFormat, targetBuffer, sourceDataFormatError, targetDataFormatError);
	case ITexelArray::SourceDataFormat::Int16:
		return ConvertTexelArrayInputDataInternalLevel2<ImageConversionTypes::Int16>(sourceBuffer, sourceBufferSizeInBytes, sourceImageFormat, sourceDataFormat, targetImageFormat, targetDataFormat, targetBuffer, sourceDataFormatError, targetDataFormatError);
	case ITexelArray::SourceDataFormat::Int32:
		return ConvertTexelArrayInputDataInternalLevel2<ImageConversionTypes::Int32>(sourceBuffer, sourceBufferSizeInBytes, sourceImageFormat, sourceDataFormat, targetImageFormat, targetDataFormat, targetBuffer, sourceDataFormatError, targetDataFormatError);
	case ITexelArray::SourceDataFormat::UInt8:
		return ConvertTexelArrayInputDataInternalLevel2<ImageConversionTypes::UInt8>(sourceBuffer, sourceBufferSizeInBytes, sourceImageFormat, sourceDataFormat, targetImageFormat, targetDataFormat, targetBuffer, sourceDataFormatError, targetDataFormatError);
	case ITexelArray::SourceDataFormat::UInt16:
		return ConvertTexelArrayInputDataInternalLevel2<ImageConversionTypes::UInt16>(sourceBuffer, sourceBufferSizeInBytes, sourceImageFormat, sourceDataFormat, targetImageFormat, targetDataFormat, targetBuffer, sourceDataFormatError, targetDataFormatError);
	case ITexelArray::SourceDataFormat::UInt32:
		return ConvertTexelArrayInputDataInternalLevel2<ImageConversionTypes::UInt32>(sourceBuffer, sourceBufferSizeInBytes, sourceImageFormat, sourceDataFormat, targetImageFormat, targetDataFormat, targetBuffer, sourceDataFormatError, targetDataFormatError);
	case ITexelArray::SourceDataFormat::Norm8:
		return ConvertTexelArrayInputDataInternalLevel2<ImageConversionTypes::Norm8>(sourceBuffer, sourceBufferSizeInBytes, sourceImageFormat, sourceDataFormat, targetImageFormat, targetDataFormat, targetBuffer, sourceDataFormatError, targetDataFormatError);
	case ITexelArray::SourceDataFormat::Norm16:
		return ConvertTexelArrayInputDataInternalLevel2<ImageConversionTypes::Norm16>(sourceBuffer, sourceBufferSizeInBytes, sourceImageFormat, sourceDataFormat, targetImageFormat, targetDataFormat, targetBuffer, sourceDataFormatError, targetDataFormatError);
	case ITexelArray::SourceDataFormat::Norm32:
		return ConvertTexelArrayInputDataInternalLevel2<ImageConversionTypes::Norm32>(sourceBuffer, sourceBufferSizeInBytes, sourceImageFormat, sourceDataFormat, targetImageFormat, targetDataFormat, targetBuffer, sourceDataFormatError, targetDataFormatError);
	case ITexelArray::SourceDataFormat::UNorm8:
		return ConvertTexelArrayInputDataInternalLevel2<ImageConversionTypes::UNorm8>(sourceBuffer, sourceBufferSizeInBytes, sourceImageFormat, sourceDataFormat, targetImageFormat, targetDataFormat, targetBuffer, sourceDataFormatError, targetDataFormatError);
	case ITexelArray::SourceDataFormat::UNorm16:
		return ConvertTexelArrayInputDataInternalLevel2<ImageConversionTypes::UNorm16>(sourceBuffer, sourceBufferSizeInBytes, sourceImageFormat, sourceDataFormat, targetImageFormat, targetDataFormat, targetBuffer, sourceDataFormatError, targetDataFormatError);
	case ITexelArray::SourceDataFormat::UNorm32:
		return ConvertTexelArrayInputDataInternalLevel2<ImageConversionTypes::UNorm32>(sourceBuffer, sourceBufferSizeInBytes, sourceImageFormat, sourceDataFormat, targetImageFormat, targetDataFormat, targetBuffer, sourceDataFormatError, targetDataFormatError);
	case ITexelArray::SourceDataFormat::Float16:
		return ConvertTexelArrayInputDataInternalLevel2<ImageConversionTypes::Float16>(sourceBuffer, sourceBufferSizeInBytes, sourceImageFormat, sourceDataFormat, targetImageFormat, targetDataFormat, targetBuffer, sourceDataFormatError, targetDataFormatError);
	case ITexelArray::SourceDataFormat::Float32:
		return ConvertTexelArrayInputDataInternalLevel2<ImageConversionTypes::Float32>(sourceBuffer, sourceBufferSizeInBytes, sourceImageFormat, sourceDataFormat, targetImageFormat, targetDataFormat, targetBuffer, sourceDataFormatError, targetDataFormatError);
	}
	sourceDataFormatError = true;
	return false;
}

//----------------------------------------------------------------------------------------
template<class ElementType>
bool TextureFormatConversion::ConvertTexelArrayInputDataInternalLevel2(const void* sourceBuffer, size_t sourceBufferSizeInBytes, ITexelArray::SourceImageFormat sourceImageFormat, ITexelArray::SourceDataFormat sourceDataFormat, ITexelArray::ImageFormat targetImageFormat, ITexelArray::DataFormat targetDataFormat, std::vector<uint8_t>& targetBuffer, bool& sourceDataFormatError, bool& targetDataFormatError)
{
	// Note that we've deliberately consolidated binary equivalent types here. While it would be cleaner to pass our
	// data formats separately from our pixel formats in all cases (IE, using DataVector3 for XYZ instead of PixelRGB),
	// that leads to more unique template instances and makes more work for the compiler, which is already heavily taxed
	// by the number of levels of template calls we have. There's no difference at runtime from combining them here. We
	// keep the XYZW format using DataVector4, as that's critical to ensure the fourth element is not considered an
	// alpha value, and therefore defaulted to 1, when performing conversion from a format that doesn't include alpha.
	switch (sourceImageFormat)
	{
	case ITexelArray::SourceImageFormat::R:
	case ITexelArray::SourceImageFormat::X:
		return ConvertTexelArrayInputDataInternalLevel3(static_cast<const ImageConversionTypes::PixelR<ElementType>*>(sourceBuffer), sourceBufferSizeInBytes, targetImageFormat, targetDataFormat, targetBuffer, targetDataFormatError);
	case ITexelArray::SourceImageFormat::RG:
	case ITexelArray::SourceImageFormat::XY:
		return ConvertTexelArrayInputDataInternalLevel3(static_cast<const ImageConversionTypes::PixelRG<ElementType>*>(sourceBuffer), sourceBufferSizeInBytes, targetImageFormat, targetDataFormat, targetBuffer, targetDataFormatError);
	case ITexelArray::SourceImageFormat::RGBA:
		return ConvertTexelArrayInputDataInternalLevel3(static_cast<const ImageConversionTypes::PixelRGBA<ElementType>*>(sourceBuffer), sourceBufferSizeInBytes, targetImageFormat, targetDataFormat, targetBuffer, targetDataFormatError);
	case ITexelArray::SourceImageFormat::XYZW:
		return ConvertTexelArrayInputDataInternalLevel3(static_cast<const ImageConversionTypes::DataVector4<ElementType>*>(sourceBuffer), sourceBufferSizeInBytes, targetImageFormat, targetDataFormat, targetBuffer, targetDataFormatError);
	}
	sourceDataFormatError = true;
	return false;
}

//----------------------------------------------------------------------------------------
template<class SourceType>
bool TextureFormatConversion::ConvertTexelArrayInputDataInternalLevel3(const SourceType* sourceBuffer, size_t sourceBufferSizeInBytes, ITexelArray::ImageFormat targetImageFormat, ITexelArray::DataFormat targetDataFormat, std::vector<uint8_t>& targetBuffer, bool& targetDataFormatError)
{
	switch (targetDataFormat)
	{
	case ITexelArray::DataFormat::Int8:
		return ConvertTexelArrayInputDataInternalLevel4<SourceType, ImageConversionTypes::Int8>(sourceBuffer, (sourceBufferSizeInBytes / sizeof(SourceType)), targetImageFormat, targetDataFormat, targetBuffer, targetDataFormatError);
	case ITexelArray::DataFormat::Int16:
		return ConvertTexelArrayInputDataInternalLevel4<SourceType, ImageConversionTypes::Int16>(sourceBuffer, (sourceBufferSizeInBytes / sizeof(SourceType)), targetImageFormat, targetDataFormat, targetBuffer, targetDataFormatError);
	case ITexelArray::DataFormat::Int32:
		return ConvertTexelArrayInputDataInternalLevel4<SourceType, ImageConversionTypes::Int32>(sourceBuffer, (sourceBufferSizeInBytes / sizeof(SourceType)), targetImageFormat, targetDataFormat, targetBuffer, targetDataFormatError);
	case ITexelArray::DataFormat::UInt8:
		return ConvertTexelArrayInputDataInternalLevel4<SourceType, ImageConversionTypes::UInt8>(sourceBuffer, (sourceBufferSizeInBytes / sizeof(SourceType)), targetImageFormat, targetDataFormat, targetBuffer, targetDataFormatError);
	case ITexelArray::DataFormat::UInt16:
		return ConvertTexelArrayInputDataInternalLevel4<SourceType, ImageConversionTypes::UInt16>(sourceBuffer, (sourceBufferSizeInBytes / sizeof(SourceType)), targetImageFormat, targetDataFormat, targetBuffer, targetDataFormatError);
	case ITexelArray::DataFormat::UInt32:
		return ConvertTexelArrayInputDataInternalLevel4<SourceType, ImageConversionTypes::UInt32>(sourceBuffer, (sourceBufferSizeInBytes / sizeof(SourceType)), targetImageFormat, targetDataFormat, targetBuffer, targetDataFormatError);
	case ITexelArray::DataFormat::Norm8:
		return ConvertTexelArrayInputDataInternalLevel4<SourceType, ImageConversionTypes::Norm8>(sourceBuffer, (sourceBufferSizeInBytes / sizeof(SourceType)), targetImageFormat, targetDataFormat, targetBuffer, targetDataFormatError);
	case ITexelArray::DataFormat::UNorm8:
		return ConvertTexelArrayInputDataInternalLevel4<SourceType, ImageConversionTypes::UNorm8>(sourceBuffer, (sourceBufferSizeInBytes / sizeof(SourceType)), targetImageFormat, targetDataFormat, targetBuffer, targetDataFormatError);
	case ITexelArray::DataFormat::Float16:
		return ConvertTexelArrayInputDataInternalLevel4<SourceType, ImageConversionTypes::Float16>(sourceBuffer, (sourceBufferSizeInBytes / sizeof(SourceType)), targetImageFormat, targetDataFormat, targetBuffer, targetDataFormatError);
	case ITexelArray::DataFormat::Float32:
		return ConvertTexelArrayInputDataInternalLevel4<SourceType, ImageConversionTypes::Float32>(sourceBuffer, (sourceBufferSizeInBytes / sizeof(SourceType)), targetImageFormat, targetDataFormat, targetBuffer, targetDataFormatError);
	}
	targetDataFormatError = true;
	return false;
}

//----------------------------------------------------------------------------------------
template<class SourceType, class ElementType>
bool TextureFormatConversion::ConvertTexelArrayInputDataInternalLevel4(const SourceType* sourceBuffer, size_t sourceBufferSize, ITexelArray::ImageFormat targetImageFormat, ITexelArray::DataFormat targetDataFormat, std::vector<uint8_t>& targetBuffer, bool& targetDataFormatError)
{
	// Note that we've deliberately consolidated binary equivalent types here. While it would be cleaner to pass our
	// data formats separately from our pixel formats in all cases (IE, using DataVector3 for XYZ instead of PixelRGB),
	// that leads to more unique template instances and makes more work for the compiler, which is already heavily taxed
	// by the number of levels of template calls we have. There's no difference at runtime from combining them here. We
	// keep the XYZW format using DataVector4, as that's critical to ensure the fourth element is not considered an
	// alpha value, and therefore defaulted to 1, when performing conversion from a format that doesn't include alpha.
	switch (targetImageFormat)
	{
	case ITexelArray::ImageFormat::R:
	case ITexelArray::ImageFormat::X:
		return ConvertTexelArrayInputDataInternalFinal<SourceType, ImageConversionTypes::PixelR<ElementType>>(sourceBuffer, targetBuffer, sourceBufferSize);
	case ITexelArray::ImageFormat::RG:
	case ITexelArray::ImageFormat::XY:
		return ConvertTexelArrayInputDataInternalFinal<SourceType, ImageConversionTypes::PixelRG<ElementType>>(sourceBuffer, targetBuffer, sourceBufferSize);
	case ITexelArray::ImageFormat::RGBA:
		return ConvertTexelArrayInputDataInternalFinal<SourceType, ImageConversionTypes::PixelRGBA<ElementType>>(sourceBuffer, targetBuffer, sourceBufferSize);
	case ITexelArray::ImageFormat::XYZW:
		return ConvertTexelArrayInputDataInternalFinal<SourceType, ImageConversionTypes::DataVector4<ElementType>>(sourceBuffer, targetBuffer, sourceBufferSize);
	}
	targetDataFormatError = true;
	return false;
}

//----------------------------------------------------------------------------------------
template<class SourceType, class TargetType>
bool TextureFormatConversion::ConvertTexelArrayInputDataInternalFinal(const SourceType* sourceBuffer, std::vector<uint8_t>& targetBuffer, size_t sampleCount)
{
	targetBuffer.resize(sampleCount * sizeof(TargetType));
	ImageConversion::ConvertImageFormat(sourceBuffer, reinterpret_cast<TargetType*>(targetBuffer.data()), sampleCount);
	return true;
}

//----------------------------------------------------------------------------------------
TEXTURE_FORMAT_CONVERSION_EXPORT bool TextureFormatConversion::ConvertTexelArrayOutputData(const void* sourceBuffer, size_t sourceBufferSizeInBytes, ITexelArray::ImageFormat sourceImageFormat, ITexelArray::DataFormat sourceDataFormat, ITexelArray::SourceImageFormat targetImageFormat, ITexelArray::SourceDataFormat targetDataFormat, void* targetBuffer, bool& sourceDataFormatError, bool& targetDataFormatError)
{
	return ConvertTexelArrayOutputDataInternalLevel1(sourceBuffer, sourceBufferSizeInBytes, sourceImageFormat, sourceDataFormat, targetImageFormat, targetDataFormat, targetBuffer, sourceDataFormatError, targetDataFormatError);
}

//----------------------------------------------------------------------------------------
bool TextureFormatConversion::ConvertTexelArrayOutputDataInternalLevel1(const void* sourceBuffer, size_t sourceBufferSizeInBytes, ITexelArray::ImageFormat sourceImageFormat, ITexelArray::DataFormat sourceDataFormat, ITexelArray::SourceImageFormat targetImageFormat, ITexelArray::SourceDataFormat targetDataFormat, void* targetBuffer, bool& sourceDataFormatError, bool& targetDataFormatError)
{
	// Handle general image format conversion
	switch (sourceDataFormat)
	{
	case ITexelArray::DataFormat::Int8:
		return ConvertTexelArrayOutputDataInternalLevel2<ImageConversionTypes::Int8>(sourceBuffer, sourceBufferSizeInBytes, sourceImageFormat, sourceDataFormat, targetImageFormat, targetDataFormat, targetBuffer, sourceDataFormatError, targetDataFormatError);
	case ITexelArray::DataFormat::Int16:
		return ConvertTexelArrayOutputDataInternalLevel2<ImageConversionTypes::Int16>(sourceBuffer, sourceBufferSizeInBytes, sourceImageFormat, sourceDataFormat, targetImageFormat, targetDataFormat, targetBuffer, sourceDataFormatError, targetDataFormatError);
	case ITexelArray::DataFormat::Int32:
		return ConvertTexelArrayOutputDataInternalLevel2<ImageConversionTypes::Int32>(sourceBuffer, sourceBufferSizeInBytes, sourceImageFormat, sourceDataFormat, targetImageFormat, targetDataFormat, targetBuffer, sourceDataFormatError, targetDataFormatError);
	case ITexelArray::DataFormat::UInt8:
		return ConvertTexelArrayOutputDataInternalLevel2<ImageConversionTypes::UInt8>(sourceBuffer, sourceBufferSizeInBytes, sourceImageFormat, sourceDataFormat, targetImageFormat, targetDataFormat, targetBuffer, sourceDataFormatError, targetDataFormatError);
	case ITexelArray::DataFormat::UInt16:
		return ConvertTexelArrayOutputDataInternalLevel2<ImageConversionTypes::UInt16>(sourceBuffer, sourceBufferSizeInBytes, sourceImageFormat, sourceDataFormat, targetImageFormat, targetDataFormat, targetBuffer, sourceDataFormatError, targetDataFormatError);
	case ITexelArray::DataFormat::UInt32:
		return ConvertTexelArrayOutputDataInternalLevel2<ImageConversionTypes::UInt32>(sourceBuffer, sourceBufferSizeInBytes, sourceImageFormat, sourceDataFormat, targetImageFormat, targetDataFormat, targetBuffer, sourceDataFormatError, targetDataFormatError);
	case ITexelArray::DataFormat::Norm8:
		return ConvertTexelArrayOutputDataInternalLevel2<ImageConversionTypes::Norm8>(sourceBuffer, sourceBufferSizeInBytes, sourceImageFormat, sourceDataFormat, targetImageFormat, targetDataFormat, targetBuffer, sourceDataFormatError, targetDataFormatError);
	case ITexelArray::DataFormat::UNorm8:
		return ConvertTexelArrayOutputDataInternalLevel2<ImageConversionTypes::UNorm8>(sourceBuffer, sourceBufferSizeInBytes, sourceImageFormat, sourceDataFormat, targetImageFormat, targetDataFormat, targetBuffer, sourceDataFormatError, targetDataFormatError);
	case ITexelArray::DataFormat::Float16:
		return ConvertTexelArrayOutputDataInternalLevel2<ImageConversionTypes::Float16>(sourceBuffer, sourceBufferSizeInBytes, sourceImageFormat, sourceDataFormat, targetImageFormat, targetDataFormat, targetBuffer, sourceDataFormatError, targetDataFormatError);
	case ITexelArray::DataFormat::Float32:
		return ConvertTexelArrayOutputDataInternalLevel2<ImageConversionTypes::Float32>(sourceBuffer, sourceBufferSizeInBytes, sourceImageFormat, sourceDataFormat, targetImageFormat, targetDataFormat, targetBuffer, sourceDataFormatError, targetDataFormatError);
	}
	sourceDataFormatError = true;
	return false;
}

//----------------------------------------------------------------------------------------
template<class ElementType>
bool TextureFormatConversion::ConvertTexelArrayOutputDataInternalLevel2(const void* sourceBuffer, size_t sourceBufferSizeInBytes, ITexelArray::ImageFormat sourceImageFormat, ITexelArray::DataFormat sourceDataFormat, ITexelArray::SourceImageFormat targetImageFormat, ITexelArray::SourceDataFormat targetDataFormat, void* targetBuffer, bool& sourceDataFormatError, bool& targetDataFormatError)
{
	// Note that we've deliberately consolidated binary equivalent types here. While it would be cleaner to pass our
	// data formats separately from our pixel formats in all cases (IE, using DataVector3 for XYZ instead of PixelRGB),
	// that leads to more unique template instances and makes more work for the compiler, which is already heavily taxed
	// by the number of levels of template calls we have. There's no difference at runtime from combining them here. We
	// keep the XYZW format using DataVector4, as that's critical to ensure the fourth element is not considered an
	// alpha value, and therefore defaulted to 1, when performing conversion from a format that doesn't include alpha.
	switch (sourceImageFormat)
	{
	case ITexelArray::ImageFormat::R:
	case ITexelArray::ImageFormat::X:
		return ConvertTexelArrayOutputDataInternalLevel3(static_cast<const ImageConversionTypes::PixelR<ElementType>*>(sourceBuffer), sourceBufferSizeInBytes, targetImageFormat, targetDataFormat, targetBuffer, targetDataFormatError);
	case ITexelArray::ImageFormat::RG:
	case ITexelArray::ImageFormat::XY:
		return ConvertTexelArrayOutputDataInternalLevel3(static_cast<const ImageConversionTypes::PixelRG<ElementType>*>(sourceBuffer), sourceBufferSizeInBytes, targetImageFormat, targetDataFormat, targetBuffer, targetDataFormatError);
	case ITexelArray::ImageFormat::RGBA:
		return ConvertTexelArrayOutputDataInternalLevel3(static_cast<const ImageConversionTypes::PixelRGBA<ElementType>*>(sourceBuffer), sourceBufferSizeInBytes, targetImageFormat, targetDataFormat, targetBuffer, targetDataFormatError);
	case ITexelArray::ImageFormat::XYZW:
		return ConvertTexelArrayOutputDataInternalLevel3(static_cast<const ImageConversionTypes::DataVector4<ElementType>*>(sourceBuffer), sourceBufferSizeInBytes, targetImageFormat, targetDataFormat, targetBuffer, targetDataFormatError);
	}
	sourceDataFormatError = true;
	return false;
}

//----------------------------------------------------------------------------------------
template<class SourceType>
bool TextureFormatConversion::ConvertTexelArrayOutputDataInternalLevel3(const SourceType* sourceBuffer, size_t sourceBufferSizeInBytes, ITexelArray::SourceImageFormat targetImageFormat, ITexelArray::SourceDataFormat targetDataFormat, void* targetBuffer, bool& targetDataFormatError)
{
	switch (targetDataFormat)
	{
	case ITexelArray::SourceDataFormat::Int8:
		return ConvertTexelArrayOutputDataInternalLevel4<SourceType, ImageConversionTypes::Int8>(sourceBuffer, (sourceBufferSizeInBytes / sizeof(SourceType)), targetImageFormat, targetDataFormat, targetBuffer, targetDataFormatError);
	case ITexelArray::SourceDataFormat::Int16:
		return ConvertTexelArrayOutputDataInternalLevel4<SourceType, ImageConversionTypes::Int16>(sourceBuffer, (sourceBufferSizeInBytes / sizeof(SourceType)), targetImageFormat, targetDataFormat, targetBuffer, targetDataFormatError);
	case ITexelArray::SourceDataFormat::Int32:
		return ConvertTexelArrayOutputDataInternalLevel4<SourceType, ImageConversionTypes::Int32>(sourceBuffer, (sourceBufferSizeInBytes / sizeof(SourceType)), targetImageFormat, targetDataFormat, targetBuffer, targetDataFormatError);
	case ITexelArray::SourceDataFormat::UInt8:
		return ConvertTexelArrayOutputDataInternalLevel4<SourceType, ImageConversionTypes::UInt8>(sourceBuffer, (sourceBufferSizeInBytes / sizeof(SourceType)), targetImageFormat, targetDataFormat, targetBuffer, targetDataFormatError);
	case ITexelArray::SourceDataFormat::UInt16:
		return ConvertTexelArrayOutputDataInternalLevel4<SourceType, ImageConversionTypes::UInt16>(sourceBuffer, (sourceBufferSizeInBytes / sizeof(SourceType)), targetImageFormat, targetDataFormat, targetBuffer, targetDataFormatError);
	case ITexelArray::SourceDataFormat::UInt32:
		return ConvertTexelArrayOutputDataInternalLevel4<SourceType, ImageConversionTypes::UInt32>(sourceBuffer, (sourceBufferSizeInBytes / sizeof(SourceType)), targetImageFormat, targetDataFormat, targetBuffer, targetDataFormatError);
	case ITexelArray::SourceDataFormat::Norm8:
		return ConvertTexelArrayOutputDataInternalLevel4<SourceType, ImageConversionTypes::Norm8>(sourceBuffer, (sourceBufferSizeInBytes / sizeof(SourceType)), targetImageFormat, targetDataFormat, targetBuffer, targetDataFormatError);
	case ITexelArray::SourceDataFormat::Norm16:
		return ConvertTexelArrayOutputDataInternalLevel4<SourceType, ImageConversionTypes::Norm16>(sourceBuffer, (sourceBufferSizeInBytes / sizeof(SourceType)), targetImageFormat, targetDataFormat, targetBuffer, targetDataFormatError);
	case ITexelArray::SourceDataFormat::UNorm8:
		return ConvertTexelArrayOutputDataInternalLevel4<SourceType, ImageConversionTypes::UNorm8>(sourceBuffer, (sourceBufferSizeInBytes / sizeof(SourceType)), targetImageFormat, targetDataFormat, targetBuffer, targetDataFormatError);
	case ITexelArray::SourceDataFormat::UNorm16:
		return ConvertTexelArrayOutputDataInternalLevel4<SourceType, ImageConversionTypes::UNorm16>(sourceBuffer, (sourceBufferSizeInBytes / sizeof(SourceType)), targetImageFormat, targetDataFormat, targetBuffer, targetDataFormatError);
	case ITexelArray::SourceDataFormat::Float16:
		return ConvertTexelArrayOutputDataInternalLevel4<SourceType, ImageConversionTypes::Float16>(sourceBuffer, (sourceBufferSizeInBytes / sizeof(SourceType)), targetImageFormat, targetDataFormat, targetBuffer, targetDataFormatError);
	case ITexelArray::SourceDataFormat::Float32:
		return ConvertTexelArrayOutputDataInternalLevel4<SourceType, ImageConversionTypes::Float32>(sourceBuffer, (sourceBufferSizeInBytes / sizeof(SourceType)), targetImageFormat, targetDataFormat, targetBuffer, targetDataFormatError);
	}
	targetDataFormatError = true;
	return false;
}

//----------------------------------------------------------------------------------------
template<class SourceType, class ElementType>
bool TextureFormatConversion::ConvertTexelArrayOutputDataInternalLevel4(const SourceType* sourceBuffer, size_t sourceBufferSize, ITexelArray::SourceImageFormat targetImageFormat, ITexelArray::SourceDataFormat targetDataFormat, void* targetBuffer, bool& targetDataFormatError)
{
	// Note that we've deliberately consolidated binary equivalent types here. While it would be cleaner to pass our
	// data formats separately from our pixel formats in all cases (IE, using DataVector3 for XYZ instead of PixelRGB),
	// that leads to more unique template instances and makes more work for the compiler, which is already heavily taxed
	// by the number of levels of template calls we have. There's no difference at runtime from combining them here. We
	// keep the XYZW format using DataVector4, as that's critical to ensure the fourth element is not considered an
	// alpha value, and therefore defaulted to 1, when performing conversion from a format that doesn't include alpha.
	switch (targetImageFormat)
	{
	case ITexelArray::SourceImageFormat::R:
	case ITexelArray::SourceImageFormat::X:
		return ConvertTexelArrayOutputDataInternalFinal<SourceType, ImageConversionTypes::PixelR<ElementType>>(sourceBuffer, targetBuffer, sourceBufferSize);
	case ITexelArray::SourceImageFormat::RG:
	case ITexelArray::SourceImageFormat::XY:
		return ConvertTexelArrayOutputDataInternalFinal<SourceType, ImageConversionTypes::PixelRG<ElementType>>(sourceBuffer, targetBuffer, sourceBufferSize);
	case ITexelArray::SourceImageFormat::RGBA:
		return ConvertTexelArrayOutputDataInternalFinal<SourceType, ImageConversionTypes::PixelRGBA<ElementType>>(sourceBuffer, targetBuffer, sourceBufferSize);
	case ITexelArray::SourceImageFormat::XYZW:
		return ConvertTexelArrayOutputDataInternalFinal<SourceType, ImageConversionTypes::DataVector4<ElementType>>(sourceBuffer, targetBuffer, sourceBufferSize);
	}
	targetDataFormatError = true;
	return false;
}

//----------------------------------------------------------------------------------------
template<class SourceType, class TargetType>
bool TextureFormatConversion::ConvertTexelArrayOutputDataInternalFinal(const SourceType* sourceBuffer, void* targetBuffer, size_t sampleCount)
{
	ImageConversion::ConvertImageFormat(sourceBuffer, reinterpret_cast<TargetType*>(targetBuffer), sampleCount);
	return true;
}

//----------------------------------------------------------------------------------------
// Framebuffer output format conversion
//----------------------------------------------------------------------------------------
TEXTURE_FORMAT_CONVERSION_EXPORT bool TextureFormatConversion::ConvertFrameBufferOutputData(void* targetBuffer, size_t targetBufferSizeInBytes, ITextureBuffer::SourceImageFormat imageFormat, ITextureBuffer::SourceDataFormat dataFormat, V2UInt32 imageOffsetInPixels, V2UInt32 imageSizeToCopy, const FrameBufferInfo& frameBufferInfo, bool& sourceDataFormatError, bool& targetDataFormatError)
{
	return ConvertFrameBufferOutputDataInternalLevel1(targetBuffer, targetBufferSizeInBytes, imageFormat, dataFormat, imageOffsetInPixels, imageSizeToCopy, frameBufferInfo, sourceDataFormatError, targetDataFormatError);
}

//----------------------------------------------------------------------------------------
bool TextureFormatConversion::ConvertFrameBufferOutputDataInternalLevel1(void* targetBuffer, size_t targetBufferSizeInBytes, ITextureBuffer::SourceImageFormat imageFormat, ITextureBuffer::SourceDataFormat dataFormat, V2UInt32 imageOffsetInPixels, V2UInt32 imageSizeToCopy, const FrameBufferInfo& frameBufferInfo, bool& sourceDataFormatError, bool& targetDataFormatError)
{
	switch (dataFormat)
	{
	case ITextureBuffer::SourceDataFormat::Int8:
		return ConvertFrameBufferOutputDataInternalLevel2<ImageConversionTypes::Int8>(targetBuffer, targetBufferSizeInBytes, imageFormat, dataFormat, imageOffsetInPixels, imageSizeToCopy, frameBufferInfo, sourceDataFormatError, targetDataFormatError);
	case ITextureBuffer::SourceDataFormat::Int16:
		return ConvertFrameBufferOutputDataInternalLevel2<ImageConversionTypes::Int16>(targetBuffer, targetBufferSizeInBytes, imageFormat, dataFormat, imageOffsetInPixels, imageSizeToCopy, frameBufferInfo, sourceDataFormatError, targetDataFormatError);
	case ITextureBuffer::SourceDataFormat::Int32:
		return ConvertFrameBufferOutputDataInternalLevel2<ImageConversionTypes::Int32>(targetBuffer, targetBufferSizeInBytes, imageFormat, dataFormat, imageOffsetInPixels, imageSizeToCopy, frameBufferInfo, sourceDataFormatError, targetDataFormatError);
	case ITextureBuffer::SourceDataFormat::UInt8:
		return ConvertFrameBufferOutputDataInternalLevel2<ImageConversionTypes::UInt8>(targetBuffer, targetBufferSizeInBytes, imageFormat, dataFormat, imageOffsetInPixels, imageSizeToCopy, frameBufferInfo, sourceDataFormatError, targetDataFormatError);
	case ITextureBuffer::SourceDataFormat::UInt16:
		return ConvertFrameBufferOutputDataInternalLevel2<ImageConversionTypes::UInt16>(targetBuffer, targetBufferSizeInBytes, imageFormat, dataFormat, imageOffsetInPixels, imageSizeToCopy, frameBufferInfo, sourceDataFormatError, targetDataFormatError);
	case ITextureBuffer::SourceDataFormat::UInt32:
		return ConvertFrameBufferOutputDataInternalLevel2<ImageConversionTypes::UInt32>(targetBuffer, targetBufferSizeInBytes, imageFormat, dataFormat, imageOffsetInPixels, imageSizeToCopy, frameBufferInfo, sourceDataFormatError, targetDataFormatError);
	case ITextureBuffer::SourceDataFormat::Norm8:
		return ConvertFrameBufferOutputDataInternalLevel2<ImageConversionTypes::Norm8>(targetBuffer, targetBufferSizeInBytes, imageFormat, dataFormat, imageOffsetInPixels, imageSizeToCopy, frameBufferInfo, sourceDataFormatError, targetDataFormatError);
	case ITextureBuffer::SourceDataFormat::Norm16:
		return ConvertFrameBufferOutputDataInternalLevel2<ImageConversionTypes::Norm16>(targetBuffer, targetBufferSizeInBytes, imageFormat, dataFormat, imageOffsetInPixels, imageSizeToCopy, frameBufferInfo, sourceDataFormatError, targetDataFormatError);
	case ITextureBuffer::SourceDataFormat::Norm32:
		return ConvertFrameBufferOutputDataInternalLevel2<ImageConversionTypes::Norm32>(targetBuffer, targetBufferSizeInBytes, imageFormat, dataFormat, imageOffsetInPixels, imageSizeToCopy, frameBufferInfo, sourceDataFormatError, targetDataFormatError);
	case ITextureBuffer::SourceDataFormat::UNorm8:
		return ConvertFrameBufferOutputDataInternalLevel2<ImageConversionTypes::UNorm8>(targetBuffer, targetBufferSizeInBytes, imageFormat, dataFormat, imageOffsetInPixels, imageSizeToCopy, frameBufferInfo, sourceDataFormatError, targetDataFormatError);
	case ITextureBuffer::SourceDataFormat::UNorm16:
		return ConvertFrameBufferOutputDataInternalLevel2<ImageConversionTypes::UNorm16>(targetBuffer, targetBufferSizeInBytes, imageFormat, dataFormat, imageOffsetInPixels, imageSizeToCopy, frameBufferInfo, sourceDataFormatError, targetDataFormatError);
	case ITextureBuffer::SourceDataFormat::UNorm32:
		return ConvertFrameBufferOutputDataInternalLevel2<ImageConversionTypes::UNorm32>(targetBuffer, targetBufferSizeInBytes, imageFormat, dataFormat, imageOffsetInPixels, imageSizeToCopy, frameBufferInfo, sourceDataFormatError, targetDataFormatError);
	case ITextureBuffer::SourceDataFormat::Float16:
		return ConvertFrameBufferOutputDataInternalLevel2<ImageConversionTypes::Float16>(targetBuffer, targetBufferSizeInBytes, imageFormat, dataFormat, imageOffsetInPixels, imageSizeToCopy, frameBufferInfo, sourceDataFormatError, targetDataFormatError);
	case ITextureBuffer::SourceDataFormat::Float32:
		return ConvertFrameBufferOutputDataInternalLevel2<ImageConversionTypes::Float32>(targetBuffer, targetBufferSizeInBytes, imageFormat, dataFormat, imageOffsetInPixels, imageSizeToCopy, frameBufferInfo, sourceDataFormatError, targetDataFormatError);
	}
	targetDataFormatError = true;
	return false;
}

//----------------------------------------------------------------------------------------
template<class ElementType>
bool TextureFormatConversion::ConvertFrameBufferOutputDataInternalLevel2(void* targetBuffer, size_t targetBufferSizeInBytes, ITextureBuffer::SourceImageFormat imageFormat, ITextureBuffer::SourceDataFormat dataFormat, V2UInt32 imageOffsetInPixels, V2UInt32 imageSizeToCopy, const FrameBufferInfo& frameBufferInfo, bool& sourceDataFormatError, bool& targetDataFormatError)
{
	// Note that we've deliberately consolidated binary equivalent types here. While it would be cleaner to pass our
	// data formats separately from our pixel formats in all cases (IE, using DataVector3 for XYZ instead of PixelRGB),
	// that leads to more unique template instances and makes more work for the compiler, which is already heavily taxed
	// by the number of levels of template calls we have. There's no difference at runtime from combining them here. We
	// keep the XYZW format using DataVector4, as that's critical to ensure the fourth element is not considered an
	// alpha value, and therefore defaulted to 1, when performing conversion from a format that doesn't include alpha.
	switch (imageFormat)
	{
	case ITextureBuffer::SourceImageFormat::R:
	case ITextureBuffer::SourceImageFormat::X:
		return ConvertFrameBufferOutputDataInternalLevel3(static_cast<ImageConversionTypes::PixelR<ElementType>*>(targetBuffer), targetBufferSizeInBytes, imageOffsetInPixels, imageSizeToCopy, frameBufferInfo, sourceDataFormatError);
	case ITextureBuffer::SourceImageFormat::RG:
	case ITextureBuffer::SourceImageFormat::XY:
		return ConvertFrameBufferOutputDataInternalLevel3(static_cast<ImageConversionTypes::PixelRG<ElementType>*>(targetBuffer), targetBufferSizeInBytes, imageOffsetInPixels, imageSizeToCopy, frameBufferInfo, sourceDataFormatError);
	case ITextureBuffer::SourceImageFormat::RGB:
	case ITextureBuffer::SourceImageFormat::XYZ:
		return ConvertFrameBufferOutputDataInternalLevel3(static_cast<ImageConversionTypes::PixelRGB<ElementType>*>(targetBuffer), targetBufferSizeInBytes, imageOffsetInPixels, imageSizeToCopy, frameBufferInfo, sourceDataFormatError);
	case ITextureBuffer::SourceImageFormat::RGBA:
		return ConvertFrameBufferOutputDataInternalLevel3(static_cast<ImageConversionTypes::PixelRGBA<ElementType>*>(targetBuffer), targetBufferSizeInBytes, imageOffsetInPixels, imageSizeToCopy, frameBufferInfo, sourceDataFormatError);
	case ITextureBuffer::SourceImageFormat::BGR:
		return ConvertFrameBufferOutputDataInternalLevel3(static_cast<ImageConversionTypes::PixelBGR<ElementType>*>(targetBuffer), targetBufferSizeInBytes, imageOffsetInPixels, imageSizeToCopy, frameBufferInfo, sourceDataFormatError);
	case ITextureBuffer::SourceImageFormat::BGRA:
		return ConvertFrameBufferOutputDataInternalLevel3(static_cast<ImageConversionTypes::PixelBGRA<ElementType>*>(targetBuffer), targetBufferSizeInBytes, imageOffsetInPixels, imageSizeToCopy, frameBufferInfo, sourceDataFormatError);
	case ITextureBuffer::SourceImageFormat::XYZW:
		return ConvertFrameBufferOutputDataInternalLevel3(static_cast<ImageConversionTypes::DataVector4<ElementType>*>(targetBuffer), targetBufferSizeInBytes, imageOffsetInPixels, imageSizeToCopy, frameBufferInfo, sourceDataFormatError);
	}
	targetDataFormatError = true;
	return false;
}

//----------------------------------------------------------------------------------------
template<class TargetType>
bool TextureFormatConversion::ConvertFrameBufferOutputDataInternalLevel3(TargetType* targetBuffer, size_t targetBufferSizeInBytes, V2UInt32 imageOffsetInPixels, V2UInt32 imageSizeToCopy, const FrameBufferInfo& frameBufferInfo, bool& sourceDataFormatError)
{
	switch (frameBufferInfo.dataFormat)
	{
	case ITextureBuffer::DataFormat::Int8:
		return ConvertFrameBufferOutputDataInternalLevel4<TargetType, ImageConversionTypes::Int8>(targetBuffer, (targetBufferSizeInBytes / sizeof(TargetType)), imageOffsetInPixels, imageSizeToCopy, frameBufferInfo, sourceDataFormatError);
	case ITextureBuffer::DataFormat::Int16:
		return ConvertFrameBufferOutputDataInternalLevel4<TargetType, ImageConversionTypes::Int16>(targetBuffer, (targetBufferSizeInBytes / sizeof(TargetType)), imageOffsetInPixels, imageSizeToCopy, frameBufferInfo, sourceDataFormatError);
	case ITextureBuffer::DataFormat::Int32:
		return ConvertFrameBufferOutputDataInternalLevel4<TargetType, ImageConversionTypes::Int32>(targetBuffer, (targetBufferSizeInBytes / sizeof(TargetType)), imageOffsetInPixels, imageSizeToCopy, frameBufferInfo, sourceDataFormatError);
	case ITextureBuffer::DataFormat::UInt8:
		return ConvertFrameBufferOutputDataInternalLevel4<TargetType, ImageConversionTypes::UInt8>(targetBuffer, (targetBufferSizeInBytes / sizeof(TargetType)), imageOffsetInPixels, imageSizeToCopy, frameBufferInfo, sourceDataFormatError);
	case ITextureBuffer::DataFormat::UInt16:
		return ConvertFrameBufferOutputDataInternalLevel4<TargetType, ImageConversionTypes::UInt16>(targetBuffer, (targetBufferSizeInBytes / sizeof(TargetType)), imageOffsetInPixels, imageSizeToCopy, frameBufferInfo, sourceDataFormatError);
	case ITextureBuffer::DataFormat::UInt32:
		return ConvertFrameBufferOutputDataInternalLevel4<TargetType, ImageConversionTypes::UInt32>(targetBuffer, (targetBufferSizeInBytes / sizeof(TargetType)), imageOffsetInPixels, imageSizeToCopy, frameBufferInfo, sourceDataFormatError);
	case ITextureBuffer::DataFormat::Norm8:
		return ConvertFrameBufferOutputDataInternalLevel4<TargetType, ImageConversionTypes::Norm8>(targetBuffer, (targetBufferSizeInBytes / sizeof(TargetType)), imageOffsetInPixels, imageSizeToCopy, frameBufferInfo, sourceDataFormatError);
	case ITextureBuffer::DataFormat::Norm16:
		return ConvertFrameBufferOutputDataInternalLevel4<TargetType, ImageConversionTypes::Norm16>(targetBuffer, (targetBufferSizeInBytes / sizeof(TargetType)), imageOffsetInPixels, imageSizeToCopy, frameBufferInfo, sourceDataFormatError);
	case ITextureBuffer::DataFormat::UNorm8:
		return ConvertFrameBufferOutputDataInternalLevel4<TargetType, ImageConversionTypes::UNorm8>(targetBuffer, (targetBufferSizeInBytes / sizeof(TargetType)), imageOffsetInPixels, imageSizeToCopy, frameBufferInfo, sourceDataFormatError);
	case ITextureBuffer::DataFormat::UNorm16:
	case ITextureBuffer::DataFormat::DepthUNorm16:
		return ConvertFrameBufferOutputDataInternalLevel4<TargetType, ImageConversionTypes::UNorm16>(targetBuffer, (targetBufferSizeInBytes / sizeof(TargetType)), imageOffsetInPixels, imageSizeToCopy, frameBufferInfo, sourceDataFormatError);
	case ITextureBuffer::DataFormat::DepthUNorm24:
		return ConvertFrameBufferOutputDataInternalLevel4<TargetType, ImageConversionTypes::UNorm24>(targetBuffer, (targetBufferSizeInBytes / sizeof(TargetType)), imageOffsetInPixels, imageSizeToCopy, frameBufferInfo, sourceDataFormatError);
	case ITextureBuffer::DataFormat::DepthUNorm24StencilUInt8:
		if (frameBufferInfo.isStencilComponent)
		{
			return ConvertFrameBufferOutputDataInternalLevel4<TargetType, ImageConversionTypes::UInt8>(targetBuffer, (targetBufferSizeInBytes / sizeof(TargetType)), imageOffsetInPixels, imageSizeToCopy, frameBufferInfo, sourceDataFormatError);
		}
		return ConvertFrameBufferOutputDataInternalLevel4<TargetType, ImageConversionTypes::UNorm24>(targetBuffer, (targetBufferSizeInBytes / sizeof(TargetType)), imageOffsetInPixels, imageSizeToCopy, frameBufferInfo, sourceDataFormatError);
	case ITextureBuffer::DataFormat::Float16:
		return ConvertFrameBufferOutputDataInternalLevel4<TargetType, ImageConversionTypes::Float16>(targetBuffer, (targetBufferSizeInBytes / sizeof(TargetType)), imageOffsetInPixels, imageSizeToCopy, frameBufferInfo, sourceDataFormatError);
	case ITextureBuffer::DataFormat::Float32:
	case ITextureBuffer::DataFormat::DepthFloat32:
		return ConvertFrameBufferOutputDataInternalLevel4<TargetType, ImageConversionTypes::Float32>(targetBuffer, (targetBufferSizeInBytes / sizeof(TargetType)), imageOffsetInPixels, imageSizeToCopy, frameBufferInfo, sourceDataFormatError);
	case ITextureBuffer::DataFormat::DepthFloat32StencilUInt8:
		if (frameBufferInfo.isStencilComponent)
		{
			return ConvertFrameBufferOutputDataInternalLevel4<TargetType, ImageConversionTypes::UInt8>(targetBuffer, (targetBufferSizeInBytes / sizeof(TargetType)), imageOffsetInPixels, imageSizeToCopy, frameBufferInfo, sourceDataFormatError);
		}
		return ConvertFrameBufferOutputDataInternalLevel4<TargetType, ImageConversionTypes::Float32>(targetBuffer, (targetBufferSizeInBytes / sizeof(TargetType)), imageOffsetInPixels, imageSizeToCopy, frameBufferInfo, sourceDataFormatError);
	}
	sourceDataFormatError = true;
	return false;
}

//----------------------------------------------------------------------------------------
template<class TargetType, class ElementType>
bool TextureFormatConversion::ConvertFrameBufferOutputDataInternalLevel4(TargetType* targetBuffer, size_t targetBufferSize, V2UInt32 imageOffsetInPixels, V2UInt32 imageSizeToCopy, const FrameBufferInfo& frameBufferInfo, bool& sourceDataFormatError)
{
	// Note that we've deliberately consolidated binary equivalent types here. While it would be cleaner to pass our
	// data formats separately from our pixel formats in all cases (IE, using DataVector3 for XYZ instead of PixelRGB),
	// that leads to more unique template instances and makes more work for the compiler, which is already heavily taxed
	// by the number of levels of template calls we have. There's no difference at runtime from combining them here. We
	// keep the XYZW format using DataVector4, as that's critical to ensure the fourth element is not considered an
	// alpha value, and therefore defaulted to 1, when performing conversion from a format that doesn't include alpha.
	switch (frameBufferInfo.imageFormat)
	{
	case ITextureBuffer::ImageFormat::R:
	case ITextureBuffer::ImageFormat::Depth:
	case ITextureBuffer::ImageFormat::DepthAndStencil:
	case ITextureBuffer::ImageFormat::X:
		return ConvertFrameBufferOutputDataInternalFinal(reinterpret_cast<const ImageConversionTypes::PixelR<ElementType>*>(frameBufferInfo.dataBuffer), targetBuffer, targetBufferSize, imageOffsetInPixels, imageSizeToCopy, frameBufferInfo);
	case ITextureBuffer::ImageFormat::RG:
	case ITextureBuffer::ImageFormat::XY:
		return ConvertFrameBufferOutputDataInternalFinal(reinterpret_cast<const ImageConversionTypes::PixelRG<ElementType>*>(frameBufferInfo.dataBuffer), targetBuffer, targetBufferSize, imageOffsetInPixels, imageSizeToCopy, frameBufferInfo);
	case ITextureBuffer::ImageFormat::RGB:
	case ITextureBuffer::ImageFormat::XYZ:
		return ConvertFrameBufferOutputDataInternalFinal(reinterpret_cast<const ImageConversionTypes::PixelRGB<ElementType>*>(frameBufferInfo.dataBuffer), targetBuffer, targetBufferSize, imageOffsetInPixels, imageSizeToCopy, frameBufferInfo);
	case ITextureBuffer::ImageFormat::RGBA:
		return ConvertFrameBufferOutputDataInternalFinal(reinterpret_cast<const ImageConversionTypes::PixelRGBA<ElementType>*>(frameBufferInfo.dataBuffer), targetBuffer, targetBufferSize, imageOffsetInPixels, imageSizeToCopy, frameBufferInfo);
	case ITextureBuffer::ImageFormat::BGR:
		return ConvertFrameBufferOutputDataInternalFinal(reinterpret_cast<const ImageConversionTypes::PixelBGR<ElementType>*>(frameBufferInfo.dataBuffer), targetBuffer, targetBufferSize, imageOffsetInPixels, imageSizeToCopy, frameBufferInfo);
	case ITextureBuffer::ImageFormat::BGRA:
		return ConvertFrameBufferOutputDataInternalFinal(reinterpret_cast<const ImageConversionTypes::PixelBGRA<ElementType>*>(frameBufferInfo.dataBuffer), targetBuffer, targetBufferSize, imageOffsetInPixels, imageSizeToCopy, frameBufferInfo);
	case ITextureBuffer::ImageFormat::XYZW:
		return ConvertFrameBufferOutputDataInternalFinal(reinterpret_cast<const ImageConversionTypes::DataVector4<ElementType>*>(frameBufferInfo.dataBuffer), targetBuffer, targetBufferSize, imageOffsetInPixels, imageSizeToCopy, frameBufferInfo);
	}
	sourceDataFormatError = true;
	return false;
}

//----------------------------------------------------------------------------------------
template<class SourceType, class TargetType>
bool TextureFormatConversion::ConvertFrameBufferOutputDataInternalFinal(const SourceType* sourceBuffer, TargetType* targetBuffer, size_t targetBufferSize, V2UInt32 imageOffsetInPixels, V2UInt32 imageSizeToCopy, const FrameBufferInfo& frameBufferInfo)
{
	// Transfer the image content, doing a direct copy where possible, and performing image element count and type
	// conversions, and swizzling, where required.
	V2UInt32 actualImageSize = frameBufferInfo.actualImageSize;
	size_t entrySizeInBytes = frameBufferInfo.elementCount * frameBufferInfo.elementSizeInBytes;
	size_t rowStrideTightlyPacked = entrySizeInBytes * actualImageSize.X();
	size_t pixelStrideTightlyPacked = frameBufferInfo.elementSizeInBytes * frameBufferInfo.elementCount;
	bool hasPaddingBetweenRows = frameBufferInfo.rowStrideInBytes != rowStrideTightlyPacked;
	bool hasPaddingBetweenPixels = frameBufferInfo.pixelStrideInBytes != pixelStrideTightlyPacked;
	if (frameBufferInfo.rowsAreReversed || hasPaddingBetweenRows || hasPaddingBetweenPixels || (imageSizeToCopy.X() != actualImageSize.X()) || (imageSizeToCopy.Y() != actualImageSize.Y()))
	{
		size_t copyWidth = imageSizeToCopy.X();
		size_t copyHeight = imageSizeToCopy.Y();
		size_t copyStartPosX = imageOffsetInPixels.X();
		size_t copyStartPosY = imageOffsetInPixels.Y();
		size_t sourceStartPosY = frameBufferInfo.rowsAreReversed ? (actualImageSize.Y() - 1 - copyStartPosY) : copyStartPosY;
		size_t rowStrideSourceInBytes = frameBufferInfo.rowStrideInBytes;
		size_t rowStrideTargetInSamples = copyWidth;
		size_t pixelStrideSourceInBytes = frameBufferInfo.pixelStrideInBytes;
		size_t pixelStrideTargetInBytes = sizeof(TargetType);
		auto sourceRowWithOffset = reinterpret_cast<const SourceType*>(reinterpret_cast<const unsigned char*>(sourceBuffer) + frameBufferInfo.pixelOffsetInBytes + (sourceStartPosY * rowStrideSourceInBytes) + (copyStartPosX * pixelStrideSourceInBytes));
		TargetType* targetRow = targetBuffer;
		if (hasPaddingBetweenPixels)
		{
			if (!frameBufferInfo.rowsAreReversed)
			{
				for (size_t outputImageRowNo = 0; outputImageRowNo < copyHeight; ++outputImageRowNo)
				{
					ImageConversion::ConvertImageFormatSparse(sourceRowWithOffset, targetRow, copyWidth, pixelStrideSourceInBytes, pixelStrideTargetInBytes);
					sourceRowWithOffset = reinterpret_cast<const SourceType*>(reinterpret_cast<const unsigned char*>(sourceRowWithOffset) + rowStrideSourceInBytes);
					targetRow += rowStrideTargetInSamples;
				}
			}
			else
			{
				for (size_t outputImageRowNo = 0; outputImageRowNo < copyHeight; ++outputImageRowNo)
				{
					ImageConversion::ConvertImageFormatSparse(sourceRowWithOffset, targetRow, copyWidth, pixelStrideSourceInBytes, pixelStrideTargetInBytes);
					sourceRowWithOffset = reinterpret_cast<const SourceType*>(reinterpret_cast<const unsigned char*>(sourceRowWithOffset) - rowStrideSourceInBytes);
					targetRow += rowStrideTargetInSamples;
				}
			}
		}
		else
		{
			if (!frameBufferInfo.rowsAreReversed)
			{
				for (size_t outputImageRowNo = 0; outputImageRowNo < copyHeight; ++outputImageRowNo)
				{
					ImageConversion::ConvertImageFormat(sourceRowWithOffset, targetRow, copyWidth);
					sourceRowWithOffset = reinterpret_cast<const SourceType*>(reinterpret_cast<const unsigned char*>(sourceRowWithOffset) + rowStrideSourceInBytes);
					targetRow += rowStrideTargetInSamples;
				}
			}
			else
			{
				for (size_t outputImageRowNo = 0; outputImageRowNo < copyHeight; ++outputImageRowNo)
				{
					ImageConversion::ConvertImageFormat(sourceRowWithOffset, targetRow, copyWidth);
					sourceRowWithOffset = reinterpret_cast<const SourceType*>(reinterpret_cast<const unsigned char*>(sourceRowWithOffset) - rowStrideSourceInBytes);
					targetRow += rowStrideTargetInSamples;
				}
			}
		}
	}
	else
	{
		ImageConversion::ConvertImageFormat(sourceBuffer, targetBuffer, targetBufferSize);
	}
	return true;
}

} // namespace cobalt::graphics
