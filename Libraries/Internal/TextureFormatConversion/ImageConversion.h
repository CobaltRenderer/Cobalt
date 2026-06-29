// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "ImageConversionTypes.h"
#include <type_traits>
#include <vector>
namespace cobalt { namespace graphics {

class ImageConversion
{
public:
	// Image conversion
	template<class MatchingType>
	static inline void ConvertImageFormat(const std::vector<MatchingType>& sourceBuffer, std::vector<MatchingType>& targetBuffer);
	template<class SourceType, class TargetType>
	static inline void ConvertImageFormat(const std::vector<SourceType>& sourceBuffer, std::vector<TargetType>& targetBuffer);
	template<class MatchingType>
	static inline void ConvertImageFormat(const MatchingType* sourceBuffer, MatchingType* targetBuffer, size_t sampleCount);
	template<class SourceType, class TargetType>
	static inline void ConvertImageFormat(const SourceType* sourceBuffer, TargetType* targetBuffer, size_t sampleCount);
	template<size_t VectorSize>
	static inline void ConvertImageFormat(const ImageConversionTypes::BasicVector<ImageConversionTypes::UInt8, VectorSize>* sourceBuffer, ImageConversionTypes::BasicVector<ImageConversionTypes::UNorm8, VectorSize>* targetBuffer, size_t sampleCount);
	template<size_t VectorSize>
	static inline void ConvertImageFormat(const ImageConversionTypes::BasicVector<ImageConversionTypes::UNorm8, VectorSize>* sourceBuffer, ImageConversionTypes::BasicVector<ImageConversionTypes::UInt8, VectorSize>* targetBuffer, size_t sampleCount);
	template<size_t VectorSize>
	static inline void ConvertImageFormat(const ImageConversionTypes::BasicVector<ImageConversionTypes::UInt16, VectorSize>* sourceBuffer, ImageConversionTypes::BasicVector<ImageConversionTypes::UNorm16, VectorSize>* targetBuffer, size_t sampleCount);
	template<size_t VectorSize>
	static inline void ConvertImageFormat(const ImageConversionTypes::BasicVector<ImageConversionTypes::UNorm16, VectorSize>* sourceBuffer, ImageConversionTypes::BasicVector<ImageConversionTypes::UInt16, VectorSize>* targetBuffer, size_t sampleCount);
	template<size_t VectorSize>
	static inline void ConvertImageFormat(const ImageConversionTypes::BasicVector<ImageConversionTypes::UInt32, VectorSize>* sourceBuffer, ImageConversionTypes::BasicVector<ImageConversionTypes::UNorm32, VectorSize>* targetBuffer, size_t sampleCount);
	template<size_t VectorSize>
	static inline void ConvertImageFormat(const ImageConversionTypes::BasicVector<ImageConversionTypes::UNorm32, VectorSize>* sourceBuffer, ImageConversionTypes::BasicVector<ImageConversionTypes::UInt32, VectorSize>* targetBuffer, size_t sampleCount);
	template<class SourceType, class TargetType>
	static inline void ConvertImageFormatSparse(const SourceType* sourceBuffer, TargetType* targetBuffer, size_t sampleCount, size_t sourceBufferStrideInBytes, size_t targetBufferStrideInBytes);

	// Full element conversion
	template<class SourceType, class TargetType>
	static inline void SwizzleAndConvertPixelFormat(const SourceType& sourceData, TargetType& targetData);

	// Element count conversion
	template<class SourceElementType, class TargetElementType, bool ElementFourIsAlpha = false, bool ElementThreeIsAlpha = false, bool ElementTwoIsAlpha = false, size_t SourceIndex0 = 0, size_t SourceIndex1 = 1, size_t SourceIndex2 = 2, size_t SourceIndex3 = 3, size_t TargetIndex0 = 0, size_t TargetIndex1 = 1, size_t TargetIndex2 = 2, size_t TargetIndex3 = 3>
	static inline void ConvertPixelFormat(const ImageConversionTypes::BasicVector<SourceElementType, 1>& sourceData, ImageConversionTypes::BasicVector<TargetElementType, 1>& targetData);
	template<class SourceElementType, class TargetElementType, bool ElementFourIsAlpha = false, bool ElementThreeIsAlpha = false, bool ElementTwoIsAlpha = false, size_t SourceIndex0 = 0, size_t SourceIndex1 = 1, size_t SourceIndex2 = 2, size_t SourceIndex3 = 3, size_t TargetIndex0 = 0, size_t TargetIndex1 = 1, size_t TargetIndex2 = 2, size_t TargetIndex3 = 3>
	static inline void ConvertPixelFormat(const ImageConversionTypes::BasicVector<SourceElementType, 1>& sourceData, ImageConversionTypes::BasicVector<TargetElementType, 2>& targetData);
	template<class SourceElementType, class TargetElementType, bool ElementFourIsAlpha = false, bool ElementThreeIsAlpha = false, bool ElementTwoIsAlpha = false, size_t SourceIndex0 = 0, size_t SourceIndex1 = 1, size_t SourceIndex2 = 2, size_t SourceIndex3 = 3, size_t TargetIndex0 = 0, size_t TargetIndex1 = 1, size_t TargetIndex2 = 2, size_t TargetIndex3 = 3>
	static inline void ConvertPixelFormat(const ImageConversionTypes::BasicVector<SourceElementType, 1>& sourceData, ImageConversionTypes::BasicVector<TargetElementType, 3>& targetData);
	template<class SourceElementType, class TargetElementType, bool ElementFourIsAlpha = false, bool ElementThreeIsAlpha = false, bool ElementTwoIsAlpha = false, size_t SourceIndex0 = 0, size_t SourceIndex1 = 1, size_t SourceIndex2 = 2, size_t SourceIndex3 = 3, size_t TargetIndex0 = 0, size_t TargetIndex1 = 1, size_t TargetIndex2 = 2, size_t TargetIndex3 = 3>
	static inline void ConvertPixelFormat(const ImageConversionTypes::BasicVector<SourceElementType, 1>& sourceData, ImageConversionTypes::BasicVector<TargetElementType, 4>& targetData);
	template<class SourceElementType, class TargetElementType, bool ElementFourIsAlpha = false, bool ElementThreeIsAlpha = false, bool ElementTwoIsAlpha = false, size_t SourceIndex0 = 0, size_t SourceIndex1 = 1, size_t SourceIndex2 = 2, size_t SourceIndex3 = 3, size_t TargetIndex0 = 0, size_t TargetIndex1 = 1, size_t TargetIndex2 = 2, size_t TargetIndex3 = 3>
	static inline void ConvertPixelFormat(const ImageConversionTypes::BasicVector<SourceElementType, 2>& sourceData, ImageConversionTypes::BasicVector<TargetElementType, 1>& targetData);
	template<class SourceElementType, class TargetElementType, bool ElementFourIsAlpha = false, bool ElementThreeIsAlpha = false, bool ElementTwoIsAlpha = false, size_t SourceIndex0 = 0, size_t SourceIndex1 = 1, size_t SourceIndex2 = 2, size_t SourceIndex3 = 3, size_t TargetIndex0 = 0, size_t TargetIndex1 = 1, size_t TargetIndex2 = 2, size_t TargetIndex3 = 3>
	static inline void ConvertPixelFormat(const ImageConversionTypes::BasicVector<SourceElementType, 2>& sourceData, ImageConversionTypes::BasicVector<TargetElementType, 2>& targetData);
	template<class SourceElementType, class TargetElementType, bool ElementFourIsAlpha = false, bool ElementThreeIsAlpha = false, bool ElementTwoIsAlpha = false, size_t SourceIndex0 = 0, size_t SourceIndex1 = 1, size_t SourceIndex2 = 2, size_t SourceIndex3 = 3, size_t TargetIndex0 = 0, size_t TargetIndex1 = 1, size_t TargetIndex2 = 2, size_t TargetIndex3 = 3>
	static inline void ConvertPixelFormat(const ImageConversionTypes::BasicVector<SourceElementType, 2>& sourceData, ImageConversionTypes::BasicVector<TargetElementType, 3>& targetData);
	template<class SourceElementType, class TargetElementType, bool ElementFourIsAlpha = false, bool ElementThreeIsAlpha = false, bool ElementTwoIsAlpha = false, size_t SourceIndex0 = 0, size_t SourceIndex1 = 1, size_t SourceIndex2 = 2, size_t SourceIndex3 = 3, size_t TargetIndex0 = 0, size_t TargetIndex1 = 1, size_t TargetIndex2 = 2, size_t TargetIndex3 = 3>
	static inline void ConvertPixelFormat(const ImageConversionTypes::BasicVector<SourceElementType, 2>& sourceData, ImageConversionTypes::BasicVector<TargetElementType, 4>& targetData);
	template<class SourceElementType, class TargetElementType, bool ElementFourIsAlpha = false, bool ElementThreeIsAlpha = false, bool ElementTwoIsAlpha = false, size_t SourceIndex0 = 0, size_t SourceIndex1 = 1, size_t SourceIndex2 = 2, size_t SourceIndex3 = 3, size_t TargetIndex0 = 0, size_t TargetIndex1 = 1, size_t TargetIndex2 = 2, size_t TargetIndex3 = 3>
	static inline void ConvertPixelFormat(const ImageConversionTypes::BasicVector<SourceElementType, 3>& sourceData, ImageConversionTypes::BasicVector<TargetElementType, 1>& targetData);
	template<class SourceElementType, class TargetElementType, bool ElementFourIsAlpha = false, bool ElementThreeIsAlpha = false, bool ElementTwoIsAlpha = false, size_t SourceIndex0 = 0, size_t SourceIndex1 = 1, size_t SourceIndex2 = 2, size_t SourceIndex3 = 3, size_t TargetIndex0 = 0, size_t TargetIndex1 = 1, size_t TargetIndex2 = 2, size_t TargetIndex3 = 3>
	static inline void ConvertPixelFormat(const ImageConversionTypes::BasicVector<SourceElementType, 3>& sourceData, ImageConversionTypes::BasicVector<TargetElementType, 2>& targetData);
	template<class SourceElementType, class TargetElementType, bool ElementFourIsAlpha = false, bool ElementThreeIsAlpha = false, bool ElementTwoIsAlpha = false, size_t SourceIndex0 = 0, size_t SourceIndex1 = 1, size_t SourceIndex2 = 2, size_t SourceIndex3 = 3, size_t TargetIndex0 = 0, size_t TargetIndex1 = 1, size_t TargetIndex2 = 2, size_t TargetIndex3 = 3>
	static inline void ConvertPixelFormat(const ImageConversionTypes::BasicVector<SourceElementType, 3>& sourceData, ImageConversionTypes::BasicVector<TargetElementType, 3>& targetData);
	template<class SourceElementType, class TargetElementType, bool ElementFourIsAlpha = false, bool ElementThreeIsAlpha = false, bool ElementTwoIsAlpha = false, size_t SourceIndex0 = 0, size_t SourceIndex1 = 1, size_t SourceIndex2 = 2, size_t SourceIndex3 = 3, size_t TargetIndex0 = 0, size_t TargetIndex1 = 1, size_t TargetIndex2 = 2, size_t TargetIndex3 = 3>
	static inline void ConvertPixelFormat(const ImageConversionTypes::BasicVector<SourceElementType, 3>& sourceData, ImageConversionTypes::BasicVector<TargetElementType, 4>& targetData);
	template<class SourceElementType, class TargetElementType, bool ElementFourIsAlpha = false, bool ElementThreeIsAlpha = false, bool ElementTwoIsAlpha = false, size_t SourceIndex0 = 0, size_t SourceIndex1 = 1, size_t SourceIndex2 = 2, size_t SourceIndex3 = 3, size_t TargetIndex0 = 0, size_t TargetIndex1 = 1, size_t TargetIndex2 = 2, size_t TargetIndex3 = 3>
	static inline void ConvertPixelFormat(const ImageConversionTypes::BasicVector<SourceElementType, 4>& sourceData, ImageConversionTypes::BasicVector<TargetElementType, 1>& targetData);
	template<class SourceElementType, class TargetElementType, bool ElementFourIsAlpha = false, bool ElementThreeIsAlpha = false, bool ElementTwoIsAlpha = false, size_t SourceIndex0 = 0, size_t SourceIndex1 = 1, size_t SourceIndex2 = 2, size_t SourceIndex3 = 3, size_t TargetIndex0 = 0, size_t TargetIndex1 = 1, size_t TargetIndex2 = 2, size_t TargetIndex3 = 3>
	static inline void ConvertPixelFormat(const ImageConversionTypes::BasicVector<SourceElementType, 4>& sourceData, ImageConversionTypes::BasicVector<TargetElementType, 2>& targetData);
	template<class SourceElementType, class TargetElementType, bool ElementFourIsAlpha = false, bool ElementThreeIsAlpha = false, bool ElementTwoIsAlpha = false, size_t SourceIndex0 = 0, size_t SourceIndex1 = 1, size_t SourceIndex2 = 2, size_t SourceIndex3 = 3, size_t TargetIndex0 = 0, size_t TargetIndex1 = 1, size_t TargetIndex2 = 2, size_t TargetIndex3 = 3>
	static inline void ConvertPixelFormat(const ImageConversionTypes::BasicVector<SourceElementType, 4>& sourceData, ImageConversionTypes::BasicVector<TargetElementType, 3>& targetData);
	template<class SourceElementType, class TargetElementType, bool ElementFourIsAlpha = false, bool ElementThreeIsAlpha = false, bool ElementTwoIsAlpha = false, size_t SourceIndex0 = 0, size_t SourceIndex1 = 1, size_t SourceIndex2 = 2, size_t SourceIndex3 = 3, size_t TargetIndex0 = 0, size_t TargetIndex1 = 1, size_t TargetIndex2 = 2, size_t TargetIndex3 = 3>
	static inline void ConvertPixelFormat(const ImageConversionTypes::BasicVector<SourceElementType, 4>& sourceData, ImageConversionTypes::BasicVector<TargetElementType, 4>& targetData);

	// Sparse element count conversion
	template<class SourceElementType, class TargetElementType, bool ElementFourIsAlpha = false, bool ElementThreeIsAlpha = false, bool ElementTwoIsAlpha = false>
	static inline void ConvertPixelFormatSparse(const SourceElementType* sourceData, size_t sourceElementCount, size_t sourceElementStride, TargetElementType* targetData, size_t targetElementCount, size_t targetElementStride);

	// Element type conversion
	static inline void ConvertElementFormat(ImageConversionTypes::UNorm8 sourceElement, ImageConversionTypes::UInt8& targetElement);
	static inline void ConvertElementFormat(ImageConversionTypes::UNorm16 sourceElement, ImageConversionTypes::UInt16& targetElement);
	static inline void ConvertElementFormat(ImageConversionTypes::UNorm32 sourceElement, ImageConversionTypes::UInt32& targetElement);
	static inline void ConvertElementFormat(ImageConversionTypes::UInt8 sourceElement, ImageConversionTypes::UNorm8& targetElement);
	static inline void ConvertElementFormat(ImageConversionTypes::UInt16 sourceElement, ImageConversionTypes::UNorm16& targetElement);
	static inline void ConvertElementFormat(ImageConversionTypes::UInt32 sourceElement, ImageConversionTypes::UNorm32& targetElement);
	template<class MatchingType>
	static inline void ConvertElementFormat(MatchingType sourceElement, MatchingType& targetElement);
	template<class SourceSignedIntegralType, class TargetSignedIntegralType, typename std::enable_if<std::is_integral<SourceSignedIntegralType>::value, int>::type = 0, typename std::enable_if<std::is_integral<TargetSignedIntegralType>::value, int>::type = 0, typename std::enable_if<std::is_signed<SourceSignedIntegralType>::value, int>::type = 0, typename std::enable_if<std::is_signed<TargetSignedIntegralType>::value, int>::type = 0, typename std::enable_if<std::integral_constant<bool, !std::is_same<SourceSignedIntegralType, TargetSignedIntegralType>::value>::value, int>::type = 0>
	static inline void ConvertElementFormat(SourceSignedIntegralType sourceElement, TargetSignedIntegralType& targetElement);
	template<class SourceUnsignedIntegralType, class TargetUnsignedIntegralType, typename std::enable_if<std::is_integral<SourceUnsignedIntegralType>::value, int>::type = 0, typename std::enable_if<std::is_integral<TargetUnsignedIntegralType>::value, int>::type = 0, typename std::enable_if<std::integral_constant<bool, !std::is_signed<SourceUnsignedIntegralType>::value>::value, int>::type = 0, typename std::enable_if<std::integral_constant<bool, !std::is_signed<TargetUnsignedIntegralType>::value>::value, int>::type = 0, typename std::enable_if<std::integral_constant<bool, !std::is_same<SourceUnsignedIntegralType, TargetUnsignedIntegralType>::value>::value, int>::type = 0>
	static inline void ConvertElementFormat(SourceUnsignedIntegralType sourceElement, TargetUnsignedIntegralType& targetElement);
	template<class SourceSignedIntegralType, class TargetUnsignedIntegralType, typename std::enable_if<std::is_integral<SourceSignedIntegralType>::value, int>::type = 0, typename std::enable_if<std::is_integral<TargetUnsignedIntegralType>::value, int>::type = 0, typename std::enable_if<std::is_signed<SourceSignedIntegralType>::value, int>::type = 0, typename std::enable_if<std::integral_constant<bool, !std::is_signed<TargetUnsignedIntegralType>::value>::value, int>::type = 0>
	static inline void ConvertElementFormat(SourceSignedIntegralType sourceElement, TargetUnsignedIntegralType& targetElement);
	template<class SourceUnsignedIntegralType, class TargetSignedIntegralType, typename std::enable_if<std::is_integral<SourceUnsignedIntegralType>::value, int>::type = 0, typename std::enable_if<std::is_integral<TargetSignedIntegralType>::value, int>::type = 0, typename std::enable_if<std::integral_constant<bool, !std::is_signed<SourceUnsignedIntegralType>::value>::value, int>::type = 0, typename std::enable_if<std::is_signed<TargetSignedIntegralType>::value, int>::type = 0>
	static inline void ConvertElementFormat(SourceUnsignedIntegralType sourceElement, TargetSignedIntegralType& targetElement);
	template<class SourceFloatingPointType, class TargetFloatingPointType, typename std::enable_if<std::is_floating_point<SourceFloatingPointType>::value, int>::type = 0, typename std::enable_if<std::is_floating_point<TargetFloatingPointType>::value, int>::type = 0, typename std::enable_if<std::integral_constant<bool, !std::is_same<SourceFloatingPointType, TargetFloatingPointType>::value>::value, int>::type = 0>
	static inline void ConvertElementFormat(SourceFloatingPointType sourceElement, TargetFloatingPointType& targetElement);
	template<class SourceNonNativeType, class TargetNonNativeType, typename std::enable_if<std::is_class<SourceNonNativeType>::value, int>::type = 0, typename std::enable_if<std::is_class<TargetNonNativeType>::value, int>::type = 0, typename std::enable_if<std::integral_constant<bool, !std::is_same<SourceNonNativeType, TargetNonNativeType>::value>::value, int>::type = 0>
	static inline void ConvertElementFormat(SourceNonNativeType sourceElement, TargetNonNativeType& targetElement);
	template<class SourceSignedIntegralType, class TargetFloatingPointType, typename std::enable_if<std::is_integral<SourceSignedIntegralType>::value, int>::type = 0, typename std::enable_if<std::is_signed<SourceSignedIntegralType>::value, int>::type = 0, typename std::enable_if<std::is_floating_point<TargetFloatingPointType>::value, int>::type = 0>
	static inline void ConvertElementFormat(SourceSignedIntegralType sourceElement, TargetFloatingPointType& targetElement);
	template<class SourceUnsignedIntegralType, class TargetFloatingPointType, typename std::enable_if<std::is_integral<SourceUnsignedIntegralType>::value, int>::type = 0, typename std::enable_if<std::integral_constant<bool, !std::is_signed<SourceUnsignedIntegralType>::value>::value, int>::type = 0, typename std::enable_if<std::is_floating_point<TargetFloatingPointType>::value, int>::type = 0>
	static inline void ConvertElementFormat(SourceUnsignedIntegralType sourceElement, TargetFloatingPointType& targetElement);
	template<class SourceSignedIntegralType, class TargetNonNativeType, typename std::enable_if<std::is_integral<SourceSignedIntegralType>::value, int>::type = 0, typename std::enable_if<std::is_signed<SourceSignedIntegralType>::value, int>::type = 0, typename std::enable_if<std::is_class<TargetNonNativeType>::value, int>::type = 0>
	static inline void ConvertElementFormat(SourceSignedIntegralType sourceElement, TargetNonNativeType& targetElement);
	template<class SourceUnsignedIntegralType, class TargetNonNativeType, typename std::enable_if<std::is_integral<SourceUnsignedIntegralType>::value, int>::type = 0, typename std::enable_if<std::integral_constant<bool, !std::is_signed<SourceUnsignedIntegralType>::value>::value, int>::type = 0, typename std::enable_if<std::is_class<TargetNonNativeType>::value, int>::type = 0>
	static inline void ConvertElementFormat(SourceUnsignedIntegralType sourceElement, TargetNonNativeType& targetElement);
	template<class SourceFloatingPointType, class TargetSignedIntegralType, typename std::enable_if<std::is_floating_point<SourceFloatingPointType>::value, int>::type = 0, typename std::enable_if<std::is_integral<TargetSignedIntegralType>::value, int>::type = 0, typename std::enable_if<std::is_signed<TargetSignedIntegralType>::value, int>::type = 0>
	static inline void ConvertElementFormat(SourceFloatingPointType sourceElement, TargetSignedIntegralType& targetElement);
	template<class SourceFloatingPointType, class TargetUnsignedIntegralType, typename std::enable_if<std::is_floating_point<SourceFloatingPointType>::value, int>::type = 0, typename std::enable_if<std::is_integral<TargetUnsignedIntegralType>::value, int>::type = 0, typename std::enable_if<std::integral_constant<bool, !std::is_signed<TargetUnsignedIntegralType>::value>::value, int>::type = 0>
	static inline void ConvertElementFormat(SourceFloatingPointType sourceElement, TargetUnsignedIntegralType& targetElement);
	template<class SourceFloatingPointType, class TargetNonNativeType, typename std::enable_if<std::is_floating_point<SourceFloatingPointType>::value, int>::type = 0, typename std::enable_if<std::is_class<TargetNonNativeType>::value, int>::type = 0>
	static inline void ConvertElementFormat(SourceFloatingPointType sourceElement, TargetNonNativeType& targetElement);
	template<class SourceNonNativeType, class TargetSignedIntegralType, typename std::enable_if<std::is_class<SourceNonNativeType>::value, int>::type = 0, typename std::enable_if<std::is_integral<TargetSignedIntegralType>::value, int>::type = 0, typename std::enable_if<std::is_signed<TargetSignedIntegralType>::value, int>::type = 0>
	static inline void ConvertElementFormat(SourceNonNativeType sourceElement, TargetSignedIntegralType& targetElement);
	template<class SourceNonNativeType, class TargetUnsignedIntegralType, typename std::enable_if<std::is_class<SourceNonNativeType>::value, int>::type = 0, typename std::enable_if<std::is_integral<TargetUnsignedIntegralType>::value, int>::type = 0, typename std::enable_if<std::integral_constant<bool, !std::is_signed<TargetUnsignedIntegralType>::value>::value, int>::type = 0>
	static inline void ConvertElementFormat(SourceNonNativeType sourceElement, TargetUnsignedIntegralType& targetElement);
	template<class SourceNonNativeType, class TargetFloatingPointType, typename std::enable_if<std::is_class<SourceNonNativeType>::value, int>::type = 0, typename std::enable_if<std::is_floating_point<TargetFloatingPointType>::value, int>::type = 0>
	static inline void ConvertElementFormat(SourceNonNativeType sourceElement, TargetFloatingPointType& targetElement);

private:
	// Swizzle converter helper types
	template<class T>
	struct VectorIndexElement0 : std::integral_constant<int, 0>
	{};
	template<class T>
	struct VectorIndexElement1 : std::integral_constant<int, 1>
	{};
	template<class T>
	struct VectorIndexElement2 : std::integral_constant<int, 2>
	{};
	template<class T>
	struct VectorIndexElement3 : std::integral_constant<int, 3>
	{};
	template<class T>
	struct VectorIndexElement0<ImageConversionTypes::PixelBGR<T>> : std::integral_constant<int, 2>
	{};
	template<class T>
	struct VectorIndexElement0<ImageConversionTypes::PixelBGRA<T>> : std::integral_constant<int, 2>
	{};
	template<class T>
	struct VectorIndexElement2<ImageConversionTypes::PixelBGR<T>> : std::integral_constant<int, 0>
	{};
	template<class T>
	struct VectorIndexElement2<ImageConversionTypes::PixelBGRA<T>> : std::integral_constant<int, 0>
	{};

	// Alpha value detector helper types
	template<class T>
	struct VectorElementIsAlpha1 : std::false_type
	{};
	template<class T>
	struct VectorElementIsAlpha2 : std::false_type
	{};
	template<class T>
	struct VectorElementIsAlpha3 : std::false_type
	{};
	template<class T>
	struct VectorElementIsAlpha3<ImageConversionTypes::PixelRGBA<T>> : std::true_type
	{};
	template<class T>
	struct VectorElementIsAlpha3<ImageConversionTypes::PixelBGRA<T>> : std::true_type
	{};

	// Overload resolution helper types
	template<class T>
	struct TypeWrapper
	{};

private:
	// Non-native type conversion
	static inline void ConvertNonNativeType(ImageConversionTypes::Norm8 sourceElement, ImageConversionTypes::Float64& targetElement);
	static inline void ConvertNonNativeType(ImageConversionTypes::Norm16 sourceElement, ImageConversionTypes::Float64& targetElement);
	static inline void ConvertNonNativeType(ImageConversionTypes::Norm24 sourceElement, ImageConversionTypes::Float64& targetElement);
	static inline void ConvertNonNativeType(ImageConversionTypes::Norm32 sourceElement, ImageConversionTypes::Float64& targetElement);
	static inline void ConvertNonNativeType(ImageConversionTypes::Float64 sourceElement, ImageConversionTypes::Norm8& targetElement);
	static inline void ConvertNonNativeType(ImageConversionTypes::Float64 sourceElement, ImageConversionTypes::Norm16& targetElement);
	static inline void ConvertNonNativeType(ImageConversionTypes::Float64 sourceElement, ImageConversionTypes::Norm24& targetElement);
	static inline void ConvertNonNativeType(ImageConversionTypes::Float64 sourceElement, ImageConversionTypes::Norm32& targetElement);
	static inline void ConvertNonNativeType(ImageConversionTypes::UNorm8 sourceElement, ImageConversionTypes::Float64& targetElement);
	static inline void ConvertNonNativeType(ImageConversionTypes::UNorm16 sourceElement, ImageConversionTypes::Float64& targetElement);
	static inline void ConvertNonNativeType(ImageConversionTypes::UNorm24 sourceElement, ImageConversionTypes::Float64& targetElement);
	static inline void ConvertNonNativeType(ImageConversionTypes::UNorm32 sourceElement, ImageConversionTypes::Float64& targetElement);
	static inline void ConvertNonNativeType(ImageConversionTypes::Float64 sourceElement, ImageConversionTypes::UNorm8& targetElement);
	static inline void ConvertNonNativeType(ImageConversionTypes::Float64 sourceElement, ImageConversionTypes::UNorm16& targetElement);
	static inline void ConvertNonNativeType(ImageConversionTypes::Float64 sourceElement, ImageConversionTypes::UNorm24& targetElement);
	static inline void ConvertNonNativeType(ImageConversionTypes::Float64 sourceElement, ImageConversionTypes::UNorm32& targetElement);
	static inline void ConvertNonNativeType(ImageConversionTypes::Float16 sourceElement, ImageConversionTypes::Float64& targetElement);
	static inline void ConvertNonNativeType(ImageConversionTypes::Float64 sourceElement, ImageConversionTypes::Float16& targetElement);

	// Default alpha values
	template<class T>
	static inline T GetDefaultAlphaValue();
	template<class T>
	static inline T GetDefaultAlphaValueInternal(TypeWrapper<T>);                                                         // NOLINT(readability-named-parameter)
	static inline float GetDefaultAlphaValueInternal(TypeWrapper<float>);                                                 // NOLINT(readability-named-parameter)
	static inline double GetDefaultAlphaValueInternal(TypeWrapper<double>);                                               // NOLINT(readability-named-parameter)
	static inline ImageConversionTypes::Float16 GetDefaultAlphaValueInternal(TypeWrapper<ImageConversionTypes::Float16>); // NOLINT(readability-named-parameter)
	template<size_t S>
	static inline ImageConversionTypes::NormBase<S> GetDefaultAlphaValueInternal(TypeWrapper<ImageConversionTypes::NormBase<S>>); // NOLINT(readability-named-parameter)
	template<size_t S>
	static inline ImageConversionTypes::UNormBase<S> GetDefaultAlphaValueInternal(TypeWrapper<ImageConversionTypes::UNormBase<S>>); // NOLINT(readability-named-parameter)

	// Default zero values
	template<class T>
	static inline T GetDefaultZeroValue();
	template<class T>
	static inline T GetDefaultZeroValueInternal(TypeWrapper<T>);                                                         // NOLINT(readability-named-parameter)
	static inline ImageConversionTypes::Float16 GetDefaultZeroValueInternal(TypeWrapper<ImageConversionTypes::Float16>); // NOLINT(readability-named-parameter)
	template<size_t S>
	static inline ImageConversionTypes::NormBase<S> GetDefaultZeroValueInternal(TypeWrapper<ImageConversionTypes::NormBase<S>>); // NOLINT(readability-named-parameter)
	template<size_t S>
	static inline ImageConversionTypes::UNormBase<S> GetDefaultZeroValueInternal(TypeWrapper<ImageConversionTypes::UNormBase<S>>); // NOLINT(readability-named-parameter)
};

}} // namespace cobalt::graphics
#include "ImageConversion.inl"
