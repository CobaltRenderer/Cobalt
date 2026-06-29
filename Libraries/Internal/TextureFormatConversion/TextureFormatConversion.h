// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "ImageConversion.h"
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <vector>
#ifdef _WIN32
#ifdef BUILDING_TEXTURE_FORMAT_CONVERSION_DLL
#define TEXTURE_FORMAT_CONVERSION_EXPORT __declspec(dllexport)
#else
#define TEXTURE_FORMAT_CONVERSION_EXPORT __declspec(dllimport)
#endif
#else
#define TEXTURE_FORMAT_CONVERSION_EXPORT __attribute__((visibility("default")))
#endif
namespace cobalt::graphics {

// Unlike the lower level ImageConversion class, this helper class is specific to our renderer, and provides specific
// conversion functions for the texture format conversions that are required by our renderers. This class will produce
// highly optimized conversion code for any possible combination of source and target image and data formats. Note that
// while there isn't seemingly much code contained in this class, separating this code into its own dll is critical for
// compilation times and performance. There are in excess of 10 levels of nested template calls involved in these
// routines, many of them creating numerous template instances at each level. At the time of first implementation, this
// results in the compiler spawning in excess of 13,000 functions from just two exposed non-templated functions, with
// significant compiler memory usage and CPU time involved in compilation. The current output dll is also in excess of
// 6MB in release and 30MB in debug, greater than the size of all the renderer plugins put together. Separating this
// code out of the renderers themselves allows us to only need to compile this code once, helping to improve compilation
// times. Including it in a dll rather than a static library improves link times, and ensures the renderer dlls won't be
// bloated in size by the volume of conversion code, reducing the overall size of the distribution and preventing
// additional cache misses in the renderers themselves, which could be incurred due to larger separation between other
// functions.
class TextureFormatConversion
{
public:
	// Structures
	struct FrameBufferInfo
	{
		V2UInt32 actualImageSize = {0, 0};
		V2UInt32 actualImageOffset = {0, 0};
		ITextureBuffer::ImageFormat imageFormat = ITextureBuffer::ImageFormat::R;
		ITextureBuffer::DataFormat dataFormat = ITextureBuffer::DataFormat::UInt8;
		bool isStencilComponent = false;
		size_t elementCount = 0;
		size_t elementSizeInBytes = 0;
		size_t pixelOffsetInBytes = 0;
		size_t pixelStrideInBytes = 0;
		size_t rowStrideInBytes = 0;
		bool rowsAreReversed = false;
		const uint8_t* dataBuffer = nullptr;
	};

public:
	// Texture load format conversion
	TEXTURE_FORMAT_CONVERSION_EXPORT static bool ConvertTextureInputData(const void* sourceBuffer, size_t sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat sourceImageFormat, ITextureBuffer::SourceDataFormat sourceDataFormat, ITextureBuffer::ImageFormat targetImageFormat, ITextureBuffer::DataFormat targetDataFormat, std::vector<uint8_t>& targetBuffer, bool& sourceDataFormatError, bool& targetDataFormatError);

	// Texel array format conversion
	TEXTURE_FORMAT_CONVERSION_EXPORT static bool ConvertTexelArrayInputData(const void* sourceBuffer, size_t sourceBufferSizeInBytes, ITexelArray::SourceImageFormat sourceImageFormat, ITexelArray::SourceDataFormat sourceDataFormat, ITexelArray::ImageFormat targetImageFormat, ITexelArray::DataFormat targetDataFormat, std::vector<uint8_t>& targetBuffer, bool& sourceDataFormatError, bool& targetDataFormatError);
	TEXTURE_FORMAT_CONVERSION_EXPORT static bool ConvertTexelArrayOutputData(const void* sourceBuffer, size_t sourceBufferSizeInBytes, ITexelArray::ImageFormat sourceImageFormat, ITexelArray::DataFormat sourceDataFormat, ITexelArray::SourceImageFormat targetImageFormat, ITexelArray::SourceDataFormat targetDataFormat, void* targetBuffer, bool& sourceDataFormatError, bool& targetDataFormatError);

	// Framebuffer output format conversion
	TEXTURE_FORMAT_CONVERSION_EXPORT static bool ConvertFrameBufferOutputData(void* targetBuffer, size_t targetBufferSizeInBytes, ITextureBuffer::SourceImageFormat imageFormat, ITextureBuffer::SourceDataFormat dataFormat, V2UInt32 imageOffsetInPixels, V2UInt32 imageSizeToCopy, const FrameBufferInfo& frameBufferInfo, bool& sourceDataFormatError, bool& targetDataFormatError);

private:
	// Texture load format conversion
	static bool ConvertTextureInputDataInternalLevel1(const void* sourceBuffer, size_t sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat sourceImageFormat, ITextureBuffer::SourceDataFormat sourceDataFormat, ITextureBuffer::ImageFormat targetImageFormat, ITextureBuffer::DataFormat targetDataFormat, std::vector<uint8_t>& targetBuffer, bool& sourceDataFormatError, bool& targetDataFormatError);
	template<class ElementType>
	static bool ConvertTextureInputDataInternalLevel2(const void* sourceBuffer, size_t sourceBufferSizeInBytes, ITextureBuffer::SourceImageFormat sourceImageFormat, ITextureBuffer::SourceDataFormat sourceDataFormat, ITextureBuffer::ImageFormat targetImageFormat, ITextureBuffer::DataFormat targetDataFormat, std::vector<uint8_t>& targetBuffer, bool& sourceDataFormatError, bool& targetDataFormatError);
	template<class SourceType>
	static bool ConvertTextureInputDataInternalLevel3(const SourceType* sourceBuffer, size_t sourceBufferSizeInBytes, ITextureBuffer::ImageFormat targetImageFormat, ITextureBuffer::DataFormat targetDataFormat, std::vector<uint8_t>& targetBuffer, bool& targetDataFormatError);
	template<class SourceType, class ElementType>
	static bool ConvertTextureInputDataInternalLevel4(const SourceType* sourceBuffer, size_t sourceBufferSize, ITextureBuffer::ImageFormat targetImageFormat, ITextureBuffer::DataFormat targetDataFormat, std::vector<uint8_t>& targetBuffer, bool& targetDataFormatError);
	template<class SourceType, class TargetType>
	static bool ConvertTextureInputDataInternalFinal(const SourceType* sourceBuffer, std::vector<uint8_t>& targetBuffer, size_t sampleCount);

	// Texel array format conversion
	static bool ConvertTexelArrayInputDataInternalLevel1(const void* sourceBuffer, size_t sourceBufferSizeInBytes, ITexelArray::SourceImageFormat sourceImageFormat, ITexelArray::SourceDataFormat sourceDataFormat, ITexelArray::ImageFormat targetImageFormat, ITexelArray::DataFormat targetDataFormat, std::vector<uint8_t>& targetBuffer, bool& sourceDataFormatError, bool& targetDataFormatError);
	template<class ElementType>
	static bool ConvertTexelArrayInputDataInternalLevel2(const void* sourceBuffer, size_t sourceBufferSizeInBytes, ITexelArray::SourceImageFormat sourceImageFormat, ITexelArray::SourceDataFormat sourceDataFormat, ITexelArray::ImageFormat targetImageFormat, ITexelArray::DataFormat targetDataFormat, std::vector<uint8_t>& targetBuffer, bool& sourceDataFormatError, bool& targetDataFormatError);
	template<class SourceType>
	static bool ConvertTexelArrayInputDataInternalLevel3(const SourceType* sourceBuffer, size_t sourceBufferSizeInBytes, ITexelArray::ImageFormat targetImageFormat, ITexelArray::DataFormat targetDataFormat, std::vector<uint8_t>& targetBuffer, bool& targetDataFormatError);
	template<class SourceType, class ElementType>
	static bool ConvertTexelArrayInputDataInternalLevel4(const SourceType* sourceBuffer, size_t sourceBufferSize, ITexelArray::ImageFormat targetImageFormat, ITexelArray::DataFormat targetDataFormat, std::vector<uint8_t>& targetBuffer, bool& targetDataFormatError);
	template<class SourceType, class TargetType>
	static bool ConvertTexelArrayInputDataInternalFinal(const SourceType* sourceBuffer, std::vector<uint8_t>& targetBuffer, size_t sampleCount);
	static bool ConvertTexelArrayOutputDataInternalLevel1(const void* sourceBuffer, size_t sourceBufferSizeInBytes, ITexelArray::ImageFormat sourceImageFormat, ITexelArray::DataFormat sourceDataFormat, ITexelArray::SourceImageFormat targetImageFormat, ITexelArray::SourceDataFormat targetDataFormat, void* targetBuffer, bool& sourceDataFormatError, bool& targetDataFormatError);
	template<class ElementType>
	static bool ConvertTexelArrayOutputDataInternalLevel2(const void* sourceBuffer, size_t sourceBufferSizeInBytes, ITexelArray::ImageFormat sourceImageFormat, ITexelArray::DataFormat sourceDataFormat, ITexelArray::SourceImageFormat targetImageFormat, ITexelArray::SourceDataFormat targetDataFormat, void* targetBuffer, bool& sourceDataFormatError, bool& targetDataFormatError);
	template<class SourceType>
	static bool ConvertTexelArrayOutputDataInternalLevel3(const SourceType* sourceBuffer, size_t sourceBufferSizeInBytes, ITexelArray::SourceImageFormat targetImageFormat, ITexelArray::SourceDataFormat targetDataFormat, void* targetBuffer, bool& targetDataFormatError);
	template<class SourceType, class ElementType>
	static bool ConvertTexelArrayOutputDataInternalLevel4(const SourceType* sourceBuffer, size_t sourceBufferSize, ITexelArray::SourceImageFormat targetImageFormat, ITexelArray::SourceDataFormat targetDataFormat, void* targetBuffer, bool& targetDataFormatError);
	template<class SourceType, class TargetType>
	static bool ConvertTexelArrayOutputDataInternalFinal(const SourceType* sourceBuffer, void* targetBuffer, size_t sampleCount);

	// Framebuffer output format conversion
	static bool ConvertFrameBufferOutputDataInternalLevel1(void* targetBuffer, size_t targetBufferSizeInBytes, ITextureBuffer::SourceImageFormat imageFormat, ITextureBuffer::SourceDataFormat dataFormat, V2UInt32 imageOffsetInPixels, V2UInt32 imageSizeToCopy, const FrameBufferInfo& frameBufferInfo, bool& sourceDataFormatError, bool& targetDataFormatError);
	template<class ElementType>
	static bool ConvertFrameBufferOutputDataInternalLevel2(void* targetBuffer, size_t targetBufferSizeInBytes, ITextureBuffer::SourceImageFormat imageFormat, ITextureBuffer::SourceDataFormat dataFormat, V2UInt32 imageOffsetInPixels, V2UInt32 imageSizeToCopy, const FrameBufferInfo& frameBufferInfo, bool& sourceDataFormatError, bool& targetDataFormatError);
	template<class TargetType>
	static bool ConvertFrameBufferOutputDataInternalLevel3(TargetType* targetBuffer, size_t targetBufferSizeInBytes, V2UInt32 imageOffsetInPixels, V2UInt32 imageSizeToCopy, const FrameBufferInfo& frameBufferInfo, bool& sourceDataFormatError);
	template<class TargetType, class ElementType>
	static bool ConvertFrameBufferOutputDataInternalLevel4(TargetType* targetBuffer, size_t targetBufferSize, V2UInt32 imageOffsetInPixels, V2UInt32 imageSizeToCopy, const FrameBufferInfo& frameBufferInfo, bool& sourceDataFormatError);
	template<class SourceType, class TargetType>
	static bool ConvertFrameBufferOutputDataInternalFinal(const SourceType* sourceBuffer, TargetType* targetBuffer, size_t targetBufferSize, V2UInt32 imageOffsetInPixels, V2UInt32 imageSizeToCopy, const FrameBufferInfo& frameBufferInfo);
};

} // namespace cobalt::graphics
