// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include <Cobalt/Debug/Debug.pkg>
WARNINGS_PUSH_OFF
#ifdef _MSC_VER
#pragma warning(disable : 26454)
#endif
#include <half.hpp>
WARNINGS_POP
#include <cmath>
#include <cstring>
#include <limits>
namespace cobalt { namespace graphics {

//----------------------------------------------------------------------------------------
// Image conversion
//----------------------------------------------------------------------------------------
template<class MatchingType>
void ImageConversion::ConvertImageFormat(const std::vector<MatchingType>& sourceBuffer, std::vector<MatchingType>& targetBuffer)
{
	targetBuffer = sourceBuffer;
}

//----------------------------------------------------------------------------------------
template<class SourceType, class TargetType>
void ImageConversion::ConvertImageFormat(const std::vector<SourceType>& sourceBuffer, std::vector<TargetType>& targetBuffer)
{
	size_t sampleCount = sourceBuffer.size();
	targetBuffer.resize(sampleCount);
	ConvertImageFormat(sourceBuffer.data(), targetBuffer.data(), sampleCount);
}

//----------------------------------------------------------------------------------------
template<class MatchingType>
void ImageConversion::ConvertImageFormat(const MatchingType* sourceBuffer, MatchingType* targetBuffer, size_t sampleCount)
{
	size_t bufferSizeInBytes = sampleCount * sizeof(*sourceBuffer);
	std::memcpy(targetBuffer, sourceBuffer, bufferSizeInBytes);
}

//----------------------------------------------------------------------------------------
template<class SourceType, class TargetType>
void ImageConversion::ConvertImageFormat(const SourceType* sourceBuffer, TargetType* targetBuffer, size_t sampleCount)
{
// RJS - To save the analyzer from blowing up due to the complexity of our nested template structure here, we disable
// the following code when running either clang tidy or the Visual Studio analyzer.
#ifndef __clang_analyzer__
#ifndef _PREFAST_
	for (size_t i = 0; i < sampleCount; ++i)
	{
		SwizzleAndConvertPixelFormat(*(sourceBuffer++), *(targetBuffer++));
	}
#endif
#endif
}

//----------------------------------------------------------------------------------------
template<size_t VectorSize>
void ImageConversion::ConvertImageFormat(const ImageConversionTypes::BasicVector<ImageConversionTypes::UInt8, VectorSize>* sourceBuffer, ImageConversionTypes::BasicVector<ImageConversionTypes::UNorm8, VectorSize>* targetBuffer, size_t sampleCount)
{
	ConvertImageFormat(reinterpret_cast<const ImageConversionTypes::BasicVector<ImageConversionTypes::UNorm8, VectorSize>*>(sourceBuffer), targetBuffer, sampleCount);
}

//----------------------------------------------------------------------------------------
template<size_t VectorSize>
void ImageConversion::ConvertImageFormat(const ImageConversionTypes::BasicVector<ImageConversionTypes::UNorm8, VectorSize>* sourceBuffer, ImageConversionTypes::BasicVector<ImageConversionTypes::UInt8, VectorSize>* targetBuffer, size_t sampleCount)
{
	ConvertImageFormat(reinterpret_cast<const ImageConversionTypes::BasicVector<ImageConversionTypes::UInt8, VectorSize>*>(sourceBuffer), targetBuffer, sampleCount);
}

//----------------------------------------------------------------------------------------
template<size_t VectorSize>
void ImageConversion::ConvertImageFormat(const ImageConversionTypes::BasicVector<ImageConversionTypes::UInt16, VectorSize>* sourceBuffer, ImageConversionTypes::BasicVector<ImageConversionTypes::UNorm16, VectorSize>* targetBuffer, size_t sampleCount)
{
	ConvertImageFormat(reinterpret_cast<const ImageConversionTypes::BasicVector<ImageConversionTypes::UNorm16, VectorSize>*>(sourceBuffer), targetBuffer, sampleCount);
}

//----------------------------------------------------------------------------------------
template<size_t VectorSize>
void ImageConversion::ConvertImageFormat(const ImageConversionTypes::BasicVector<ImageConversionTypes::UNorm16, VectorSize>* sourceBuffer, ImageConversionTypes::BasicVector<ImageConversionTypes::UInt16, VectorSize>* targetBuffer, size_t sampleCount)
{
	ConvertImageFormat(reinterpret_cast<const ImageConversionTypes::BasicVector<ImageConversionTypes::UInt16, VectorSize>*>(sourceBuffer), targetBuffer, sampleCount);
}

//----------------------------------------------------------------------------------------
template<size_t VectorSize>
void ImageConversion::ConvertImageFormat(const ImageConversionTypes::BasicVector<ImageConversionTypes::UInt32, VectorSize>* sourceBuffer, ImageConversionTypes::BasicVector<ImageConversionTypes::UNorm32, VectorSize>* targetBuffer, size_t sampleCount)
{
	ConvertImageFormat(reinterpret_cast<const ImageConversionTypes::BasicVector<ImageConversionTypes::UNorm32, VectorSize>*>(sourceBuffer), targetBuffer, sampleCount);
}

//----------------------------------------------------------------------------------------
template<size_t VectorSize>
void ImageConversion::ConvertImageFormat(const ImageConversionTypes::BasicVector<ImageConversionTypes::UNorm32, VectorSize>* sourceBuffer, ImageConversionTypes::BasicVector<ImageConversionTypes::UInt32, VectorSize>* targetBuffer, size_t sampleCount)
{
	ConvertImageFormat(reinterpret_cast<const ImageConversionTypes::BasicVector<ImageConversionTypes::UInt32, VectorSize>*>(sourceBuffer), targetBuffer, sampleCount);
}

//----------------------------------------------------------------------------------------
template<class SourceType, class TargetType>
void ImageConversion::ConvertImageFormatSparse(const SourceType* sourceBuffer, TargetType* targetBuffer, size_t sampleCount, size_t sourceBufferStrideInBytes, size_t targetBufferStrideInBytes)
{
// RJS - To save the analyzer from blowing up due to the complexity of our nested template structure here, we disable
// the following code when running either clang tidy or the Visual Studio analyzer.
#ifndef __clang_analyzer__
#ifndef _PREFAST_
	for (size_t i = 0; i < sampleCount; ++i)
	{
		SwizzleAndConvertPixelFormat(*sourceBuffer, *targetBuffer);
		sourceBuffer = reinterpret_cast<const SourceType*>(reinterpret_cast<const unsigned char*>(sourceBuffer) + sourceBufferStrideInBytes);
		targetBuffer = reinterpret_cast<TargetType*>(reinterpret_cast<unsigned char*>(targetBuffer) + targetBufferStrideInBytes);
	}
#endif
#endif
}

//----------------------------------------------------------------------------------------
// Full element conversion
//----------------------------------------------------------------------------------------
template<class SourceType, class TargetType>
void ImageConversion::SwizzleAndConvertPixelFormat(const SourceType& sourceData, TargetType& targetData)
{
// RJS - To save the analyzer from blowing up due to the complexity of our nested template structure here, we disable
// the following code when running either clang tidy or the Visual Studio analyzer.
#ifndef __clang_analyzer__
#ifndef _PREFAST_
	ConvertPixelFormat<typename SourceType::ElementType, typename TargetType::ElementType, VectorElementIsAlpha3<TargetType>::value, VectorElementIsAlpha2<TargetType>::value, VectorElementIsAlpha1<TargetType>::value, VectorIndexElement0<SourceType>::value, VectorIndexElement1<SourceType>::value, VectorIndexElement2<SourceType>::value, VectorIndexElement3<SourceType>::value, VectorIndexElement0<TargetType>::value, VectorIndexElement1<TargetType>::value, VectorIndexElement2<TargetType>::value, VectorIndexElement3<TargetType>::value>(sourceData, targetData);
#endif
#endif
}

//----------------------------------------------------------------------------------------
// Element count conversion
//----------------------------------------------------------------------------------------
template<class SourceElementType, class TargetElementType, bool ElementFourIsAlpha, bool ElementThreeIsAlpha, bool ElementTwoIsAlpha, size_t SourceIndex0, size_t SourceIndex1, size_t SourceIndex2, size_t SourceIndex3, size_t TargetIndex0, size_t TargetIndex1, size_t TargetIndex2, size_t TargetIndex3>
void ImageConversion::ConvertPixelFormat(const ImageConversionTypes::BasicVector<SourceElementType, 1>& sourceData, ImageConversionTypes::BasicVector<TargetElementType, 1>& targetData)
{
	ConvertElementFormat(sourceData.data[SourceIndex0], targetData.data[TargetIndex0]);
}

//----------------------------------------------------------------------------------------
template<class SourceElementType, class TargetElementType, bool ElementFourIsAlpha, bool ElementThreeIsAlpha, bool ElementTwoIsAlpha, size_t SourceIndex0, size_t SourceIndex1, size_t SourceIndex2, size_t SourceIndex3, size_t TargetIndex0, size_t TargetIndex1, size_t TargetIndex2, size_t TargetIndex3>
void ImageConversion::ConvertPixelFormat(const ImageConversionTypes::BasicVector<SourceElementType, 1>& sourceData, ImageConversionTypes::BasicVector<TargetElementType, 2>& targetData)
{
	ConvertElementFormat(sourceData.data[SourceIndex0], targetData.data[TargetIndex0]);
	targetData.data[TargetIndex1] = (ElementTwoIsAlpha ? ImageConversion::GetDefaultAlphaValue<TargetElementType>() : ImageConversion::GetDefaultZeroValue<TargetElementType>());
}

//----------------------------------------------------------------------------------------
template<class SourceElementType, class TargetElementType, bool ElementFourIsAlpha, bool ElementThreeIsAlpha, bool ElementTwoIsAlpha, size_t SourceIndex0, size_t SourceIndex1, size_t SourceIndex2, size_t SourceIndex3, size_t TargetIndex0, size_t TargetIndex1, size_t TargetIndex2, size_t TargetIndex3>
void ImageConversion::ConvertPixelFormat(const ImageConversionTypes::BasicVector<SourceElementType, 1>& sourceData, ImageConversionTypes::BasicVector<TargetElementType, 3>& targetData)
{
	ConvertElementFormat(sourceData.data[SourceIndex0], targetData.data[TargetIndex0]);
	targetData.data[TargetIndex1] = (ElementTwoIsAlpha ? ImageConversion::GetDefaultAlphaValue<TargetElementType>() : ImageConversion::GetDefaultZeroValue<TargetElementType>());
	targetData.data[TargetIndex2] = (ElementThreeIsAlpha ? ImageConversion::GetDefaultAlphaValue<TargetElementType>() : ImageConversion::GetDefaultZeroValue<TargetElementType>());
}

//----------------------------------------------------------------------------------------
template<class SourceElementType, class TargetElementType, bool ElementFourIsAlpha, bool ElementThreeIsAlpha, bool ElementTwoIsAlpha, size_t SourceIndex0, size_t SourceIndex1, size_t SourceIndex2, size_t SourceIndex3, size_t TargetIndex0, size_t TargetIndex1, size_t TargetIndex2, size_t TargetIndex3>
void ImageConversion::ConvertPixelFormat(const ImageConversionTypes::BasicVector<SourceElementType, 1>& sourceData, ImageConversionTypes::BasicVector<TargetElementType, 4>& targetData)
{
	ConvertElementFormat(sourceData.data[SourceIndex0], targetData.data[TargetIndex0]);
	targetData.data[TargetIndex1] = (ElementTwoIsAlpha ? ImageConversion::GetDefaultAlphaValue<TargetElementType>() : ImageConversion::GetDefaultZeroValue<TargetElementType>());
	targetData.data[TargetIndex2] = (ElementThreeIsAlpha ? ImageConversion::GetDefaultAlphaValue<TargetElementType>() : ImageConversion::GetDefaultZeroValue<TargetElementType>());
	targetData.data[TargetIndex3] = (ElementFourIsAlpha ? ImageConversion::GetDefaultAlphaValue<TargetElementType>() : ImageConversion::GetDefaultZeroValue<TargetElementType>());
}

//----------------------------------------------------------------------------------------
template<class SourceElementType, class TargetElementType, bool ElementFourIsAlpha, bool ElementThreeIsAlpha, bool ElementTwoIsAlpha, size_t SourceIndex0, size_t SourceIndex1, size_t SourceIndex2, size_t SourceIndex3, size_t TargetIndex0, size_t TargetIndex1, size_t TargetIndex2, size_t TargetIndex3>
void ImageConversion::ConvertPixelFormat(const ImageConversionTypes::BasicVector<SourceElementType, 2>& sourceData, ImageConversionTypes::BasicVector<TargetElementType, 1>& targetData)
{
	ConvertElementFormat(sourceData.data[SourceIndex0], targetData.data[TargetIndex0]);
}

//----------------------------------------------------------------------------------------
template<class SourceElementType, class TargetElementType, bool ElementFourIsAlpha, bool ElementThreeIsAlpha, bool ElementTwoIsAlpha, size_t SourceIndex0, size_t SourceIndex1, size_t SourceIndex2, size_t SourceIndex3, size_t TargetIndex0, size_t TargetIndex1, size_t TargetIndex2, size_t TargetIndex3>
void ImageConversion::ConvertPixelFormat(const ImageConversionTypes::BasicVector<SourceElementType, 2>& sourceData, ImageConversionTypes::BasicVector<TargetElementType, 2>& targetData)
{
	ConvertElementFormat(sourceData.data[SourceIndex0], targetData.data[TargetIndex0]);
	ConvertElementFormat(sourceData.data[SourceIndex1], targetData.data[TargetIndex1]);
}

//----------------------------------------------------------------------------------------
template<class SourceElementType, class TargetElementType, bool ElementFourIsAlpha, bool ElementThreeIsAlpha, bool ElementTwoIsAlpha, size_t SourceIndex0, size_t SourceIndex1, size_t SourceIndex2, size_t SourceIndex3, size_t TargetIndex0, size_t TargetIndex1, size_t TargetIndex2, size_t TargetIndex3>
void ImageConversion::ConvertPixelFormat(const ImageConversionTypes::BasicVector<SourceElementType, 2>& sourceData, ImageConversionTypes::BasicVector<TargetElementType, 3>& targetData)
{
	ConvertElementFormat(sourceData.data[SourceIndex0], targetData.data[TargetIndex0]);
	ConvertElementFormat(sourceData.data[SourceIndex1], targetData.data[TargetIndex1]);
	targetData.data[TargetIndex2] = (ElementThreeIsAlpha ? ImageConversion::GetDefaultAlphaValue<TargetElementType>() : ImageConversion::GetDefaultZeroValue<TargetElementType>());
}

//----------------------------------------------------------------------------------------
template<class SourceElementType, class TargetElementType, bool ElementFourIsAlpha, bool ElementThreeIsAlpha, bool ElementTwoIsAlpha, size_t SourceIndex0, size_t SourceIndex1, size_t SourceIndex2, size_t SourceIndex3, size_t TargetIndex0, size_t TargetIndex1, size_t TargetIndex2, size_t TargetIndex3>
void ImageConversion::ConvertPixelFormat(const ImageConversionTypes::BasicVector<SourceElementType, 2>& sourceData, ImageConversionTypes::BasicVector<TargetElementType, 4>& targetData)
{
	ConvertElementFormat(sourceData.data[SourceIndex0], targetData.data[TargetIndex0]);
	ConvertElementFormat(sourceData.data[SourceIndex1], targetData.data[TargetIndex1]);
	targetData.data[TargetIndex2] = (ElementThreeIsAlpha ? ImageConversion::GetDefaultAlphaValue<TargetElementType>() : ImageConversion::GetDefaultZeroValue<TargetElementType>());
	targetData.data[TargetIndex3] = (ElementFourIsAlpha ? ImageConversion::GetDefaultAlphaValue<TargetElementType>() : ImageConversion::GetDefaultZeroValue<TargetElementType>());
}

//----------------------------------------------------------------------------------------
template<class SourceElementType, class TargetElementType, bool ElementFourIsAlpha, bool ElementThreeIsAlpha, bool ElementTwoIsAlpha, size_t SourceIndex0, size_t SourceIndex1, size_t SourceIndex2, size_t SourceIndex3, size_t TargetIndex0, size_t TargetIndex1, size_t TargetIndex2, size_t TargetIndex3>
void ImageConversion::ConvertPixelFormat(const ImageConversionTypes::BasicVector<SourceElementType, 3>& sourceData, ImageConversionTypes::BasicVector<TargetElementType, 1>& targetData)
{
	ConvertElementFormat(sourceData.data[SourceIndex0], targetData.data[TargetIndex0]);
}

//----------------------------------------------------------------------------------------
template<class SourceElementType, class TargetElementType, bool ElementFourIsAlpha, bool ElementThreeIsAlpha, bool ElementTwoIsAlpha, size_t SourceIndex0, size_t SourceIndex1, size_t SourceIndex2, size_t SourceIndex3, size_t TargetIndex0, size_t TargetIndex1, size_t TargetIndex2, size_t TargetIndex3>
void ImageConversion::ConvertPixelFormat(const ImageConversionTypes::BasicVector<SourceElementType, 3>& sourceData, ImageConversionTypes::BasicVector<TargetElementType, 2>& targetData)
{
	ConvertElementFormat(sourceData.data[SourceIndex0], targetData.data[TargetIndex0]);
	ConvertElementFormat(sourceData.data[SourceIndex1], targetData.data[TargetIndex1]);
}

//----------------------------------------------------------------------------------------
template<class SourceElementType, class TargetElementType, bool ElementFourIsAlpha, bool ElementThreeIsAlpha, bool ElementTwoIsAlpha, size_t SourceIndex0, size_t SourceIndex1, size_t SourceIndex2, size_t SourceIndex3, size_t TargetIndex0, size_t TargetIndex1, size_t TargetIndex2, size_t TargetIndex3>
void ImageConversion::ConvertPixelFormat(const ImageConversionTypes::BasicVector<SourceElementType, 3>& sourceData, ImageConversionTypes::BasicVector<TargetElementType, 3>& targetData)
{
	ConvertElementFormat(sourceData.data[SourceIndex0], targetData.data[TargetIndex0]);
	ConvertElementFormat(sourceData.data[SourceIndex1], targetData.data[TargetIndex1]);
	ConvertElementFormat(sourceData.data[SourceIndex2], targetData.data[TargetIndex2]);
}

//----------------------------------------------------------------------------------------
template<class SourceElementType, class TargetElementType, bool ElementFourIsAlpha, bool ElementThreeIsAlpha, bool ElementTwoIsAlpha, size_t SourceIndex0, size_t SourceIndex1, size_t SourceIndex2, size_t SourceIndex3, size_t TargetIndex0, size_t TargetIndex1, size_t TargetIndex2, size_t TargetIndex3>
void ImageConversion::ConvertPixelFormat(const ImageConversionTypes::BasicVector<SourceElementType, 3>& sourceData, ImageConversionTypes::BasicVector<TargetElementType, 4>& targetData)
{
	ConvertElementFormat(sourceData.data[SourceIndex0], targetData.data[TargetIndex0]);
	ConvertElementFormat(sourceData.data[SourceIndex1], targetData.data[TargetIndex1]);
	ConvertElementFormat(sourceData.data[SourceIndex2], targetData.data[TargetIndex2]);
	targetData.data[TargetIndex3] = (ElementFourIsAlpha ? ImageConversion::GetDefaultAlphaValue<TargetElementType>() : ImageConversion::GetDefaultZeroValue<TargetElementType>());
}

//----------------------------------------------------------------------------------------
template<class SourceElementType, class TargetElementType, bool ElementFourIsAlpha, bool ElementThreeIsAlpha, bool ElementTwoIsAlpha, size_t SourceIndex0, size_t SourceIndex1, size_t SourceIndex2, size_t SourceIndex3, size_t TargetIndex0, size_t TargetIndex1, size_t TargetIndex2, size_t TargetIndex3>
void ImageConversion::ConvertPixelFormat(const ImageConversionTypes::BasicVector<SourceElementType, 4>& sourceData, ImageConversionTypes::BasicVector<TargetElementType, 1>& targetData)
{
	ConvertElementFormat(sourceData.data[SourceIndex0], targetData.data[TargetIndex0]);
}

//----------------------------------------------------------------------------------------
template<class SourceElementType, class TargetElementType, bool ElementFourIsAlpha, bool ElementThreeIsAlpha, bool ElementTwoIsAlpha, size_t SourceIndex0, size_t SourceIndex1, size_t SourceIndex2, size_t SourceIndex3, size_t TargetIndex0, size_t TargetIndex1, size_t TargetIndex2, size_t TargetIndex3>
void ImageConversion::ConvertPixelFormat(const ImageConversionTypes::BasicVector<SourceElementType, 4>& sourceData, ImageConversionTypes::BasicVector<TargetElementType, 2>& targetData)
{
	ConvertElementFormat(sourceData.data[SourceIndex0], targetData.data[TargetIndex0]);
	ConvertElementFormat(sourceData.data[SourceIndex1], targetData.data[TargetIndex1]);
}

//----------------------------------------------------------------------------------------
template<class SourceElementType, class TargetElementType, bool ElementFourIsAlpha, bool ElementThreeIsAlpha, bool ElementTwoIsAlpha, size_t SourceIndex0, size_t SourceIndex1, size_t SourceIndex2, size_t SourceIndex3, size_t TargetIndex0, size_t TargetIndex1, size_t TargetIndex2, size_t TargetIndex3>
void ImageConversion::ConvertPixelFormat(const ImageConversionTypes::BasicVector<SourceElementType, 4>& sourceData, ImageConversionTypes::BasicVector<TargetElementType, 3>& targetData)
{
	ConvertElementFormat(sourceData.data[SourceIndex0], targetData.data[TargetIndex0]);
	ConvertElementFormat(sourceData.data[SourceIndex1], targetData.data[TargetIndex1]);
	ConvertElementFormat(sourceData.data[SourceIndex2], targetData.data[TargetIndex2]);
}

//----------------------------------------------------------------------------------------
template<class SourceElementType, class TargetElementType, bool ElementFourIsAlpha, bool ElementThreeIsAlpha, bool ElementTwoIsAlpha, size_t SourceIndex0, size_t SourceIndex1, size_t SourceIndex2, size_t SourceIndex3, size_t TargetIndex0, size_t TargetIndex1, size_t TargetIndex2, size_t TargetIndex3>
void ImageConversion::ConvertPixelFormat(const ImageConversionTypes::BasicVector<SourceElementType, 4>& sourceData, ImageConversionTypes::BasicVector<TargetElementType, 4>& targetData)
{
	ConvertElementFormat(sourceData.data[SourceIndex0], targetData.data[TargetIndex0]);
	ConvertElementFormat(sourceData.data[SourceIndex1], targetData.data[TargetIndex1]);
	ConvertElementFormat(sourceData.data[SourceIndex2], targetData.data[TargetIndex2]);
	ConvertElementFormat(sourceData.data[SourceIndex3], targetData.data[TargetIndex3]);
}

//----------------------------------------------------------------------------------------
// Sparse element count conversion
//----------------------------------------------------------------------------------------
template<class SourceElementType, class TargetElementType, bool ElementFourIsAlpha, bool ElementThreeIsAlpha, bool ElementTwoIsAlpha>
void ImageConversion::ConvertPixelFormatSparse(const SourceElementType* sourceData, size_t sourceElementCount, size_t sourceElementStrideInBytes, TargetElementType* targetData, size_t targetElementCount, size_t targetElementStrideInBytes)
{
	for (size_t targetElementNo = 0; targetElementNo < targetElementCount; ++targetElementNo)
	{
		if (targetElementNo >= sourceElementCount)
		{
			bool fillDefaultAlphaValue = ((ElementFourIsAlpha && (targetElementCount == 4)) || (ElementThreeIsAlpha && (targetElementCount == 3)) || (ElementTwoIsAlpha && (targetElementCount == 2)));
			*targetData = (fillDefaultAlphaValue ? ImageConversion::GetDefaultAlphaValue<TargetElementType>() : ImageConversion::GetDefaultZeroValue<TargetElementType>());
		}
		else
		{
			ConvertElementFormat(*sourceData, *targetData);
		}
		sourceData = reinterpret_cast<const SourceElementType*>(reinterpret_cast<const unsigned char*>(sourceData) + sourceElementStrideInBytes);
		targetData = reinterpret_cast<TargetElementType*>(reinterpret_cast<unsigned char*>(targetData) + targetElementStrideInBytes);
	}
}

//----------------------------------------------------------------------------------------
// Element type conversion
//----------------------------------------------------------------------------------------
void ImageConversion::ConvertElementFormat(ImageConversionTypes::UNorm8 sourceElement, ImageConversionTypes::UInt8& targetElement)
{
	targetElement = *reinterpret_cast<std::remove_reference<decltype(targetElement)>::type*>(&sourceElement);
}

//----------------------------------------------------------------------------------------
void ImageConversion::ConvertElementFormat(ImageConversionTypes::UNorm16 sourceElement, ImageConversionTypes::UInt16& targetElement)
{
	targetElement = *reinterpret_cast<std::remove_reference<decltype(targetElement)>::type*>(&sourceElement);
}

//----------------------------------------------------------------------------------------
void ImageConversion::ConvertElementFormat(ImageConversionTypes::UNorm32 sourceElement, ImageConversionTypes::UInt32& targetElement)
{
	targetElement = *reinterpret_cast<std::remove_reference<decltype(targetElement)>::type*>(&sourceElement);
}

//----------------------------------------------------------------------------------------
void ImageConversion::ConvertElementFormat(ImageConversionTypes::UInt8 sourceElement, ImageConversionTypes::UNorm8& targetElement)
{
	targetElement = *reinterpret_cast<std::remove_reference<decltype(targetElement)>::type*>(&sourceElement);
}

//----------------------------------------------------------------------------------------
void ImageConversion::ConvertElementFormat(ImageConversionTypes::UInt16 sourceElement, ImageConversionTypes::UNorm16& targetElement)
{
	targetElement = *reinterpret_cast<std::remove_reference<decltype(targetElement)>::type*>(&sourceElement);
}

//----------------------------------------------------------------------------------------
void ImageConversion::ConvertElementFormat(ImageConversionTypes::UInt32 sourceElement, ImageConversionTypes::UNorm32& targetElement)
{
	targetElement = *reinterpret_cast<std::remove_reference<decltype(targetElement)>::type*>(&sourceElement);
}

//----------------------------------------------------------------------------------------
template<class MatchingType>
void ImageConversion::ConvertElementFormat(MatchingType sourceElement, MatchingType& targetElement)
{
	targetElement = sourceElement;
}

//----------------------------------------------------------------------------------------
template<class SourceSignedIntegralType, class TargetSignedIntegralType, typename std::enable_if<std::is_integral<SourceSignedIntegralType>::value, int>::type, typename std::enable_if<std::is_integral<TargetSignedIntegralType>::value, int>::type, typename std::enable_if<std::is_signed<SourceSignedIntegralType>::value, int>::type, typename std::enable_if<std::is_signed<TargetSignedIntegralType>::value, int>::type, typename std::enable_if<std::integral_constant<bool, !std::is_same<SourceSignedIntegralType, TargetSignedIntegralType>::value>::value, int>::type>
void ImageConversion::ConvertElementFormat(SourceSignedIntegralType sourceElement, TargetSignedIntegralType& targetElement)
{
	targetElement = static_cast<TargetSignedIntegralType>(std::max(std::min(static_cast<long long>(std::numeric_limits<TargetSignedIntegralType>::max()), static_cast<long long>(sourceElement)), static_cast<long long>(std::numeric_limits<TargetSignedIntegralType>::min())));
}

//----------------------------------------------------------------------------------------
template<class SourceUnsignedIntegralType, class TargetUnsignedIntegralType, typename std::enable_if<std::is_integral<SourceUnsignedIntegralType>::value, int>::type, typename std::enable_if<std::is_integral<TargetUnsignedIntegralType>::value, int>::type, typename std::enable_if<std::integral_constant<bool, !std::is_signed<SourceUnsignedIntegralType>::value>::value, int>::type, typename std::enable_if<std::integral_constant<bool, !std::is_signed<TargetUnsignedIntegralType>::value>::value, int>::type, typename std::enable_if<std::integral_constant<bool, !std::is_same<SourceUnsignedIntegralType, TargetUnsignedIntegralType>::value>::value, int>::type>
void ImageConversion::ConvertElementFormat(SourceUnsignedIntegralType sourceElement, TargetUnsignedIntegralType& targetElement)
{
	targetElement = static_cast<TargetUnsignedIntegralType>(std::min(static_cast<long long>(std::numeric_limits<TargetUnsignedIntegralType>::max()), static_cast<long long>(sourceElement)));
}

//----------------------------------------------------------------------------------------
template<class SourceSignedIntegralType, class TargetUnsignedIntegralType, typename std::enable_if<std::is_integral<SourceSignedIntegralType>::value, int>::type, typename std::enable_if<std::is_integral<TargetUnsignedIntegralType>::value, int>::type, typename std::enable_if<std::is_signed<SourceSignedIntegralType>::value, int>::type, typename std::enable_if<std::integral_constant<bool, !std::is_signed<TargetUnsignedIntegralType>::value>::value, int>::type>
void ImageConversion::ConvertElementFormat(SourceSignedIntegralType sourceElement, TargetUnsignedIntegralType& targetElement)
{
	targetElement = static_cast<TargetUnsignedIntegralType>(std::max(std::min(static_cast<long long>(std::numeric_limits<TargetUnsignedIntegralType>::max()), static_cast<long long>(sourceElement)), static_cast<long long>(0)));
}

//----------------------------------------------------------------------------------------
template<class SourceUnsignedIntegralType, class TargetSignedIntegralType, typename std::enable_if<std::is_integral<SourceUnsignedIntegralType>::value, int>::type, typename std::enable_if<std::is_integral<TargetSignedIntegralType>::value, int>::type, typename std::enable_if<std::integral_constant<bool, !std::is_signed<SourceUnsignedIntegralType>::value>::value, int>::type, typename std::enable_if<std::is_signed<TargetSignedIntegralType>::value, int>::type>
void ImageConversion::ConvertElementFormat(SourceUnsignedIntegralType sourceElement, TargetSignedIntegralType& targetElement)
{
	targetElement = static_cast<TargetSignedIntegralType>(std::min(static_cast<long long>(std::numeric_limits<TargetSignedIntegralType>::max()), static_cast<long long>(sourceElement)));
}

//----------------------------------------------------------------------------------------
template<class SourceFloatingPointType, class TargetFloatingPointType, typename std::enable_if<std::is_floating_point<SourceFloatingPointType>::value, int>::type, typename std::enable_if<std::is_floating_point<TargetFloatingPointType>::value, int>::type, typename std::enable_if<std::integral_constant<bool, !std::is_same<SourceFloatingPointType, TargetFloatingPointType>::value>::value, int>::type>
void ImageConversion::ConvertElementFormat(SourceFloatingPointType sourceElement, TargetFloatingPointType& targetElement)
{
	targetElement = static_cast<TargetFloatingPointType>(sourceElement);
}

//----------------------------------------------------------------------------------------
template<class SourceNonNativeType, class TargetNonNativeType, typename std::enable_if<std::is_class<SourceNonNativeType>::value, int>::type, typename std::enable_if<std::is_class<TargetNonNativeType>::value, int>::type, typename std::enable_if<std::integral_constant<bool, !std::is_same<SourceNonNativeType, TargetNonNativeType>::value>::value, int>::type>
void ImageConversion::ConvertElementFormat(SourceNonNativeType sourceElement, TargetNonNativeType& targetElement)
{
	ImageConversionTypes::Float64 sourceElementAsDouble;
	ConvertNonNativeType(sourceElement, sourceElementAsDouble);
	ConvertNonNativeType(sourceElementAsDouble, targetElement);
}

//----------------------------------------------------------------------------------------
template<class SourceSignedIntegralType, class TargetFloatingPointType, typename std::enable_if<std::is_integral<SourceSignedIntegralType>::value, int>::type, typename std::enable_if<std::is_signed<SourceSignedIntegralType>::value, int>::type, typename std::enable_if<std::is_floating_point<TargetFloatingPointType>::value, int>::type>
void ImageConversion::ConvertElementFormat(SourceSignedIntegralType sourceElement, TargetFloatingPointType& targetElement)
{
	targetElement = static_cast<TargetFloatingPointType>(static_cast<double>(sourceElement) / (sourceElement < 0 ? -static_cast<double>(std::numeric_limits<SourceSignedIntegralType>::min()) : static_cast<double>(std::numeric_limits<SourceSignedIntegralType>::max())));
}

//----------------------------------------------------------------------------------------
template<class SourceUnsignedIntegralType, class TargetFloatingPointType, typename std::enable_if<std::is_integral<SourceUnsignedIntegralType>::value, int>::type, typename std::enable_if<std::integral_constant<bool, !std::is_signed<SourceUnsignedIntegralType>::value>::value, int>::type, typename std::enable_if<std::is_floating_point<TargetFloatingPointType>::value, int>::type>
void ImageConversion::ConvertElementFormat(SourceUnsignedIntegralType sourceElement, TargetFloatingPointType& targetElement)
{
	targetElement = static_cast<TargetFloatingPointType>(static_cast<double>(sourceElement) / static_cast<double>(std::numeric_limits<SourceUnsignedIntegralType>::max()));
}

//----------------------------------------------------------------------------------------
template<class SourceSignedIntegralType, class TargetNonNativeType, typename std::enable_if<std::is_integral<SourceSignedIntegralType>::value, int>::type, typename std::enable_if<std::is_signed<SourceSignedIntegralType>::value, int>::type, typename std::enable_if<std::is_class<TargetNonNativeType>::value, int>::type>
void ImageConversion::ConvertElementFormat(SourceSignedIntegralType sourceElement, TargetNonNativeType& targetElement)
{
	ImageConversionTypes::Float64 sourceElementAsDouble;
	ConvertElementFormat(sourceElement, sourceElementAsDouble);
	ConvertNonNativeType(sourceElementAsDouble, targetElement);
}

//----------------------------------------------------------------------------------------
template<class SourceUnsignedIntegralType, class TargetNonNativeType, typename std::enable_if<std::is_integral<SourceUnsignedIntegralType>::value, int>::type, typename std::enable_if<std::integral_constant<bool, !std::is_signed<SourceUnsignedIntegralType>::value>::value, int>::type, typename std::enable_if<std::is_class<TargetNonNativeType>::value, int>::type>
void ImageConversion::ConvertElementFormat(SourceUnsignedIntegralType sourceElement, TargetNonNativeType& targetElement)
{
	ImageConversionTypes::Float64 sourceElementAsDouble;
	ConvertElementFormat(sourceElement, sourceElementAsDouble);
	ConvertNonNativeType(sourceElementAsDouble, targetElement);
}

//----------------------------------------------------------------------------------------
template<class SourceFloatingPointType, class TargetSignedIntegralType, typename std::enable_if<std::is_floating_point<SourceFloatingPointType>::value, int>::type, typename std::enable_if<std::is_integral<TargetSignedIntegralType>::value, int>::type, typename std::enable_if<std::is_signed<TargetSignedIntegralType>::value, int>::type>
void ImageConversion::ConvertElementFormat(SourceFloatingPointType sourceElement, TargetSignedIntegralType& targetElement)
{
	targetElement = static_cast<TargetSignedIntegralType>(std::round(std::max(std::min(static_cast<double>(sourceElement), 1.0), -1.0) * (sourceElement < 0 ? -static_cast<double>(std::numeric_limits<TargetSignedIntegralType>::min()) : static_cast<double>(std::numeric_limits<TargetSignedIntegralType>::max()))));
}

//----------------------------------------------------------------------------------------
template<class SourceFloatingPointType, class TargetUnsignedIntegralType, typename std::enable_if<std::is_floating_point<SourceFloatingPointType>::value, int>::type, typename std::enable_if<std::is_integral<TargetUnsignedIntegralType>::value, int>::type, typename std::enable_if<std::integral_constant<bool, !std::is_signed<TargetUnsignedIntegralType>::value>::value, int>::type>
void ImageConversion::ConvertElementFormat(SourceFloatingPointType sourceElement, TargetUnsignedIntegralType& targetElement)
{
	targetElement = static_cast<TargetUnsignedIntegralType>(std::round(std::max(std::min(static_cast<double>(sourceElement), 1.0), 0.0) * static_cast<double>(std::numeric_limits<TargetUnsignedIntegralType>::max())));
}

//----------------------------------------------------------------------------------------
template<class SourceFloatingPointType, class TargetNonNativeType, typename std::enable_if<std::is_floating_point<SourceFloatingPointType>::value, int>::type, typename std::enable_if<std::is_class<TargetNonNativeType>::value, int>::type>
void ImageConversion::ConvertElementFormat(SourceFloatingPointType sourceElement, TargetNonNativeType& targetElement)
{
	ImageConversionTypes::Float64 sourceElementAsDouble;
	ConvertElementFormat(sourceElement, sourceElementAsDouble);
	ConvertNonNativeType(sourceElementAsDouble, targetElement);
}

//----------------------------------------------------------------------------------------
template<class SourceNonNativeType, class TargetSignedIntegralType, typename std::enable_if<std::is_class<SourceNonNativeType>::value, int>::type, typename std::enable_if<std::is_integral<TargetSignedIntegralType>::value, int>::type, typename std::enable_if<std::is_signed<TargetSignedIntegralType>::value, int>::type>
void ImageConversion::ConvertElementFormat(SourceNonNativeType sourceElement, TargetSignedIntegralType& targetElement)
{
	ImageConversionTypes::Float64 sourceElementAsDouble;
	ConvertNonNativeType(sourceElement, sourceElementAsDouble);
	ConvertElementFormat(sourceElementAsDouble, targetElement);
}

//----------------------------------------------------------------------------------------
template<class SourceNonNativeType, class TargetUnsignedIntegralType, typename std::enable_if<std::is_class<SourceNonNativeType>::value, int>::type, typename std::enable_if<std::is_integral<TargetUnsignedIntegralType>::value, int>::type, typename std::enable_if<std::integral_constant<bool, !std::is_signed<TargetUnsignedIntegralType>::value>::value, int>::type>
void ImageConversion::ConvertElementFormat(SourceNonNativeType sourceElement, TargetUnsignedIntegralType& targetElement)
{
	ImageConversionTypes::Float64 sourceElementAsDouble;
	ConvertNonNativeType(sourceElement, sourceElementAsDouble);
	ConvertElementFormat(sourceElementAsDouble, targetElement);
}

//----------------------------------------------------------------------------------------
template<class SourceNonNativeType, class TargetFloatingPointType, typename std::enable_if<std::is_class<SourceNonNativeType>::value, int>::type, typename std::enable_if<std::is_floating_point<TargetFloatingPointType>::value, int>::type>
void ImageConversion::ConvertElementFormat(SourceNonNativeType sourceElement, TargetFloatingPointType& targetElement)
{
	ImageConversionTypes::Float64 sourceElementAsDouble;
	ConvertNonNativeType(sourceElement, sourceElementAsDouble);
	ConvertElementFormat(sourceElementAsDouble, targetElement);
}

//----------------------------------------------------------------------------------------
// Non-native type conversion
//----------------------------------------------------------------------------------------
void ImageConversion::ConvertNonNativeType(ImageConversionTypes::Norm8 sourceElement, ImageConversionTypes::Float64& targetElement)
{
	ImageConversionTypes::Int8 sourceElementAsInt = *reinterpret_cast<ImageConversionTypes::Int8*>(&sourceElement);
	sourceElementAsInt = (sourceElementAsInt == std::numeric_limits<decltype(sourceElementAsInt)>::min()) ? (std::numeric_limits<decltype(sourceElementAsInt)>::min() + 1) : sourceElementAsInt;
	targetElement = static_cast<std::remove_reference<decltype(targetElement)>::type>(static_cast<double>(sourceElementAsInt) / std::numeric_limits<decltype(sourceElementAsInt)>::max());
}

//----------------------------------------------------------------------------------------
void ImageConversion::ConvertNonNativeType(ImageConversionTypes::Norm16 sourceElement, ImageConversionTypes::Float64& targetElement)
{
	ImageConversionTypes::Int16 sourceElementAsInt = *reinterpret_cast<ImageConversionTypes::Int16*>(&sourceElement);
	sourceElementAsInt = (sourceElementAsInt == std::numeric_limits<decltype(sourceElementAsInt)>::min()) ? (std::numeric_limits<decltype(sourceElementAsInt)>::min() + 1) : sourceElementAsInt;
	targetElement = static_cast<std::remove_reference<decltype(targetElement)>::type>(static_cast<double>(sourceElementAsInt) / std::numeric_limits<decltype(sourceElementAsInt)>::max());
}

//----------------------------------------------------------------------------------------
void ImageConversion::ConvertNonNativeType(ImageConversionTypes::Norm24 sourceElement, ImageConversionTypes::Float64& targetElement)
{
	static const ImageConversionTypes::Int32 sourceElementMaxValue = 0x7FFFFF;
	static const ImageConversionTypes::Int32 sourceElementMinValue = -(sourceElementMaxValue + 1);
	ImageConversionTypes::Int32 sourceElementAsInt = static_cast<ImageConversionTypes::Int32>(sourceElement.data[0]) | (static_cast<ImageConversionTypes::Int32>(sourceElement.data[1]) << 8) | (static_cast<ImageConversionTypes::Int32>(static_cast<int>(static_cast<signed char>(sourceElement.data[2]))) << 16);
	sourceElementAsInt = (sourceElementAsInt == sourceElementMinValue) ? (sourceElementMinValue + 1) : sourceElementAsInt;
	targetElement = static_cast<std::remove_reference<decltype(targetElement)>::type>(static_cast<double>(sourceElementAsInt) / sourceElementMaxValue);
}

//----------------------------------------------------------------------------------------
void ImageConversion::ConvertNonNativeType(ImageConversionTypes::Norm32 sourceElement, ImageConversionTypes::Float64& targetElement)
{
	ImageConversionTypes::Int32 sourceElementAsInt = *reinterpret_cast<ImageConversionTypes::Int32*>(&sourceElement);
	sourceElementAsInt = (sourceElementAsInt == std::numeric_limits<decltype(sourceElementAsInt)>::min()) ? (std::numeric_limits<decltype(sourceElementAsInt)>::min() + 1) : sourceElementAsInt;
	targetElement = static_cast<std::remove_reference<decltype(targetElement)>::type>(static_cast<double>(sourceElementAsInt) / std::numeric_limits<decltype(sourceElementAsInt)>::max());
}

//----------------------------------------------------------------------------------------
void ImageConversion::ConvertNonNativeType(ImageConversionTypes::Float64 sourceElement, ImageConversionTypes::Norm8& targetElement)
{
	ImageConversionTypes::Int8 targetElementAsInt = static_cast<ImageConversionTypes::Int8>(std::round(std::max(std::min(static_cast<double>(sourceElement), 1.0), -1.0) * static_cast<double>(std::numeric_limits<ImageConversionTypes::Int8>::max())));
	targetElement = *reinterpret_cast<std::remove_reference<decltype(targetElement)>::type*>(&targetElementAsInt);
}

//----------------------------------------------------------------------------------------
void ImageConversion::ConvertNonNativeType(ImageConversionTypes::Float64 sourceElement, ImageConversionTypes::Norm16& targetElement)
{
	ImageConversionTypes::Int16 targetElementAsInt = static_cast<ImageConversionTypes::Int16>(std::round(std::max(std::min(static_cast<double>(sourceElement), 1.0), -1.0) * static_cast<double>(std::numeric_limits<ImageConversionTypes::Int16>::max())));
	targetElement = *reinterpret_cast<std::remove_reference<decltype(targetElement)>::type*>(&targetElementAsInt);
}

//----------------------------------------------------------------------------------------
void ImageConversion::ConvertNonNativeType(ImageConversionTypes::Float64 sourceElement, ImageConversionTypes::Norm24& targetElement)
{
	static const ImageConversionTypes::Int32 targetElementMaxValue = 0x7FFFFF;
	ImageConversionTypes::Int32 targetElementAsInt = static_cast<ImageConversionTypes::Int32>(std::round(std::max(std::min(static_cast<double>(sourceElement), 1.0), -1.0) * static_cast<double>(targetElementMaxValue)));
	targetElement.data[0] = static_cast<unsigned char>(targetElementAsInt & 0xFF);
	targetElement.data[1] = static_cast<unsigned char>((targetElementAsInt >> 8) & 0xFF);
	targetElement.data[2] = static_cast<unsigned char>((targetElementAsInt >> 16) & 0xFF);
}

//----------------------------------------------------------------------------------------
void ImageConversion::ConvertNonNativeType(ImageConversionTypes::Float64 sourceElement, ImageConversionTypes::Norm32& targetElement)
{
	ImageConversionTypes::Int32 targetElementAsInt = static_cast<ImageConversionTypes::Int32>(std::round(std::max(std::min(static_cast<double>(sourceElement), 1.0), -1.0) * static_cast<double>(std::numeric_limits<ImageConversionTypes::Int32>::max())));
	targetElement = *reinterpret_cast<std::remove_reference<decltype(targetElement)>::type*>(&targetElementAsInt);
}

//----------------------------------------------------------------------------------------
void ImageConversion::ConvertNonNativeType(ImageConversionTypes::UNorm8 sourceElement, ImageConversionTypes::Float64& targetElement)
{
	ImageConversionTypes::UInt8 sourceElementAsUInt = *reinterpret_cast<ImageConversionTypes::UInt8*>(&sourceElement);
	targetElement = static_cast<std::remove_reference<decltype(targetElement)>::type>(static_cast<double>(sourceElementAsUInt) / std::numeric_limits<decltype(sourceElementAsUInt)>::max());
}

//----------------------------------------------------------------------------------------
void ImageConversion::ConvertNonNativeType(ImageConversionTypes::UNorm16 sourceElement, ImageConversionTypes::Float64& targetElement)
{
	ImageConversionTypes::UInt16 sourceElementAsUInt = *reinterpret_cast<ImageConversionTypes::UInt16*>(&sourceElement);
	targetElement = static_cast<std::remove_reference<decltype(targetElement)>::type>(static_cast<double>(sourceElementAsUInt) / std::numeric_limits<decltype(sourceElementAsUInt)>::max());
}

//----------------------------------------------------------------------------------------
void ImageConversion::ConvertNonNativeType(ImageConversionTypes::UNorm24 sourceElement, ImageConversionTypes::Float64& targetElement)
{
	static const ImageConversionTypes::UInt32 sourceElementMaxValue = 0xFFFFFF;
	ImageConversionTypes::UInt32 sourceElementAsUInt = static_cast<ImageConversionTypes::UInt32>(sourceElement.data[0]) | (static_cast<ImageConversionTypes::UInt32>(sourceElement.data[2]) << 16) | (static_cast<ImageConversionTypes::UInt32>(sourceElement.data[1]) << 8);
	targetElement = static_cast<std::remove_reference<decltype(targetElement)>::type>(static_cast<double>(sourceElementAsUInt) / sourceElementMaxValue);
}

//----------------------------------------------------------------------------------------
void ImageConversion::ConvertNonNativeType(ImageConversionTypes::UNorm32 sourceElement, ImageConversionTypes::Float64& targetElement)
{
	ImageConversionTypes::UInt32 sourceElementAsUInt = *reinterpret_cast<ImageConversionTypes::UInt32*>(&sourceElement);
	targetElement = static_cast<std::remove_reference<decltype(targetElement)>::type>(static_cast<double>(sourceElementAsUInt) / std::numeric_limits<decltype(sourceElementAsUInt)>::max());
}

//----------------------------------------------------------------------------------------
void ImageConversion::ConvertNonNativeType(ImageConversionTypes::Float64 sourceElement, ImageConversionTypes::UNorm8& targetElement)
{
	ImageConversionTypes::UInt8 targetElementAsUInt = static_cast<ImageConversionTypes::UInt8>(std::round(std::max(std::min(static_cast<double>(sourceElement), 1.0), 0.0) * static_cast<double>(std::numeric_limits<ImageConversionTypes::UInt8>::max())));
	targetElement = *reinterpret_cast<std::remove_reference<decltype(targetElement)>::type*>(&targetElementAsUInt);
}

//----------------------------------------------------------------------------------------
void ImageConversion::ConvertNonNativeType(ImageConversionTypes::Float64 sourceElement, ImageConversionTypes::UNorm16& targetElement)
{
	ImageConversionTypes::UInt16 targetElementAsUInt = static_cast<ImageConversionTypes::UInt16>(std::round(std::max(std::min(static_cast<double>(sourceElement), 1.0), 0.0) * static_cast<double>(std::numeric_limits<ImageConversionTypes::UInt16>::max())));
	targetElement = *reinterpret_cast<std::remove_reference<decltype(targetElement)>::type*>(&targetElementAsUInt);
}

//----------------------------------------------------------------------------------------
void ImageConversion::ConvertNonNativeType(ImageConversionTypes::Float64 sourceElement, ImageConversionTypes::UNorm24& targetElement)
{
	static const ImageConversionTypes::UInt32 targetElementMaxValue = 0x7FFFFF;
	ImageConversionTypes::UInt32 targetElementAsUInt = static_cast<ImageConversionTypes::UInt32>(std::round(std::max(std::min(static_cast<double>(sourceElement), 1.0), 0.0) * static_cast<double>(targetElementMaxValue)));
	targetElement.data[0] = static_cast<unsigned char>(targetElementAsUInt & 0xFF);
	targetElement.data[1] = static_cast<unsigned char>((targetElementAsUInt >> 8) & 0xFF);
	targetElement.data[2] = static_cast<unsigned char>((targetElementAsUInt >> 16) & 0xFF);
}

//----------------------------------------------------------------------------------------
void ImageConversion::ConvertNonNativeType(ImageConversionTypes::Float64 sourceElement, ImageConversionTypes::UNorm32& targetElement)
{
	ImageConversionTypes::UInt32 targetElementAsUInt = static_cast<ImageConversionTypes::UInt32>(std::round(std::max(std::min(static_cast<double>(sourceElement), 1.0), 0.0) * static_cast<double>(std::numeric_limits<ImageConversionTypes::UInt32>::max())));
	targetElement = *reinterpret_cast<std::remove_reference<decltype(targetElement)>::type*>(&targetElementAsUInt);
}

//----------------------------------------------------------------------------------------
void ImageConversion::ConvertNonNativeType(ImageConversionTypes::Float16 sourceElement, ImageConversionTypes::Float64& targetElement)
{
	targetElement = half_float::half_cast<half_float::half>(*reinterpret_cast<const half_float::half*>(&sourceElement));
}

//----------------------------------------------------------------------------------------
void ImageConversion::ConvertNonNativeType(ImageConversionTypes::Float64 sourceElement, ImageConversionTypes::Float16& targetElement)
{
	*reinterpret_cast<half_float::half*>(&targetElement) = half_float::half_cast<half_float::half>(sourceElement);
}

//----------------------------------------------------------------------------------------
// Default alpha values
//----------------------------------------------------------------------------------------
template<class T>
T ImageConversion::GetDefaultAlphaValue()
{
	// This use of "TypeWrapper" is a workaround for a lack of partial template specialization in C++. On the proper
	// signature for this method, we take no arguments and want to specialize based on return type. While the caller can
	// provide the desired type as an explicit template parameter, if we try and provide overloads which themselves have
	// template arguments, as with the "NormBase" and "UNormBase" types with a template argument for the size, we end up
	// with two function base templates which are equally specialized, and as a result which one ends up getting called
	// is undefined. This can legally compile, but we have no guarantees that our specialized overloads will actually be
	// used in this case. We solve this problem here by using a dummy type argument. This allows overload resolution to
	// kick in, which will correctly dispatch to our desired functions, even where they are also templates (IE, pseudo
	// partial specializations). Since the argument is never used by any of the implementations, the optimizer should
	// strip it out in this case under release builds.
	return GetDefaultAlphaValueInternal(TypeWrapper<T>{});
}

//----------------------------------------------------------------------------------------
template<class T>
T ImageConversion::GetDefaultAlphaValueInternal(TypeWrapper<T>) // NOLINT(readability-named-parameter)
{
	return std::numeric_limits<T>::max();
}

//----------------------------------------------------------------------------------------
float ImageConversion::GetDefaultAlphaValueInternal(TypeWrapper<float>) // NOLINT(readability-named-parameter)
{
	return 1.0f;
}

//----------------------------------------------------------------------------------------
double ImageConversion::GetDefaultAlphaValueInternal(TypeWrapper<double>) // NOLINT(readability-named-parameter)
{
	return 1.0;
}

//----------------------------------------------------------------------------------------
ImageConversionTypes::Float16 ImageConversion::GetDefaultAlphaValueInternal(TypeWrapper<ImageConversionTypes::Float16>) // NOLINT(readability-named-parameter)
{
	auto result = half_float::half_cast<half_float::half>(1.0f);
	return *reinterpret_cast<const ImageConversionTypes::Float16*>(&result);
}

//----------------------------------------------------------------------------------------
template<size_t S>
ImageConversionTypes::NormBase<S> ImageConversion::GetDefaultAlphaValueInternal(TypeWrapper<ImageConversionTypes::NormBase<S>>) // NOLINT(readability-named-parameter)
{
	ImageConversionTypes::NormBase<S> result; // NOLINT(cppcoreguidelines-pro-type-member-init, hicpp-member-init)
	result.data[0] = 0x7F;
	for (size_t i = 1; i < S; ++i)
	{
		result.data[i] = 0xFF;
	}
	return result;
}

//----------------------------------------------------------------------------------------
template<size_t S>
ImageConversionTypes::UNormBase<S> ImageConversion::GetDefaultAlphaValueInternal(TypeWrapper<ImageConversionTypes::UNormBase<S>>) // NOLINT(readability-named-parameter)
{
	ImageConversionTypes::UNormBase<S> result; // NOLINT(cppcoreguidelines-pro-type-member-init, hicpp-member-init)
	for (size_t i = 0; i < S; ++i)
	{
		result.data[i] = 0xFF;
	}
	return result;
}

//----------------------------------------------------------------------------------------
// Default zero values
//----------------------------------------------------------------------------------------
template<class T>
T ImageConversion::GetDefaultZeroValue()
{
	// This use of "TypeWrapper" is a workaround for a lack of partial template specialization in C++. On the proper
	// signature for this method, we take no arguments and want to specialize based on return type. While the caller can
	// provide the desired type as an explicit template parameter, if we try and provide overloads which themselves have
	// template arguments, as with the "NormBase" and "UNormBase" types with a template argument for the size, we end up
	// with two function base templates which are equally specialized, and as a result which one ends up getting called
	// is undefined. This can legally compile, but we have no guarantees that our specialized overloads will actually be
	// used in this case. We solve this problem here by using a dummy type argument. This allows overload resolution to
	// kick in, which will correctly dispatch to our desired functions, even where they are also templates (IE, pseudo
	// partial specializations). Since the argument is never used by any of the implementations, the optimizer should
	// strip it out in this case under release builds.
	return GetDefaultZeroValueInternal(TypeWrapper<T>{});
}

//----------------------------------------------------------------------------------------
template<class T>
T ImageConversion::GetDefaultZeroValueInternal(TypeWrapper<T>) // NOLINT(readability-named-parameter)
{
	return T(0);
}

//----------------------------------------------------------------------------------------
ImageConversionTypes::Float16 ImageConversion::GetDefaultZeroValueInternal(TypeWrapper<ImageConversionTypes::Float16>) // NOLINT(readability-named-parameter)
{
	auto result = half_float::half_cast<half_float::half>(0.0f);
	return *reinterpret_cast<const ImageConversionTypes::Float16*>(&result);
}

//----------------------------------------------------------------------------------------
template<size_t S>
ImageConversionTypes::NormBase<S> ImageConversion::GetDefaultZeroValueInternal(TypeWrapper<ImageConversionTypes::NormBase<S>>) // NOLINT(readability-named-parameter)
{
	ImageConversionTypes::NormBase<S> result; // NOLINT(cppcoreguidelines-pro-type-member-init, hicpp-member-init)
	for (size_t i = 0; i < S; ++i)
	{
		result.data[i] = 0;
	}
	return result;
}

//----------------------------------------------------------------------------------------
template<size_t S>
ImageConversionTypes::UNormBase<S> ImageConversion::GetDefaultZeroValueInternal(TypeWrapper<ImageConversionTypes::UNormBase<S>>) // NOLINT(readability-named-parameter)
{
	ImageConversionTypes::UNormBase<S> result; // NOLINT(cppcoreguidelines-pro-type-member-init, hicpp-member-init)
	for (size_t i = 0; i < S; ++i)
	{
		result.data[i] = 0;
	}
	return result;
}

}} // namespace cobalt::graphics
