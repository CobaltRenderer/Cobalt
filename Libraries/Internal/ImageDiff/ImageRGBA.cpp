// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
// Don't let png lib jump out - use the call stack.
#define PNG_SETJMP_NOT_SUPPORTED
#include "ImageRGBA.h"
#include "ExportMacro.h"
#include <Cobalt/Debug/Debug.pkg>
#include <algorithm>
#include <cstring>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <png.h>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
namespace internal {
extern "C" IMAGEDIFF_EXPORT IImageRGBA* CreateIImageRGBA()
{
	return new ImageRGBA();
}
} // namespace internal

//----------------------------------------------------------------------------------------
// Delete method
//----------------------------------------------------------------------------------------
void ImageRGBA::Delete()
{
	delete this;
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void ImageRGBA::ClearAndResize(const ImageSize& newSize)
{
	_size = newSize;
	_pixelCount = _size.width * _size.height;
	_data.resize((size_t)newSize.width * (size_t)newSize.height);
}

//----------------------------------------------------------------------------------------
// Format methods
//----------------------------------------------------------------------------------------
ImageRGBA::ImageSize ImageRGBA::Size() const
{
	ASSERT(_data.size() == PixelCount());
	return _size;
}

//----------------------------------------------------------------------------------------
uint32_t ImageRGBA::PixelCount() const
{
	return _pixelCount;
}

//----------------------------------------------------------------------------------------
// Data access methods
//----------------------------------------------------------------------------------------
ImageRGBA::PixelEntry ImageRGBA::ReadPixel(uint32_t posX, uint32_t posY) const
{
	ASSERT(_data.size() == PixelCount());
	return _data[posX + posY * _size.width];
}

//----------------------------------------------------------------------------------------
void ImageRGBA::ReadPixel(uint32_t posX, uint32_t posY, PixelEntry& pixelData) const
{
	ASSERT(_data.size() == PixelCount());
	pixelData = _data[posX + posY * _size.width];
}

//----------------------------------------------------------------------------------------
void ImageRGBA::WritePixel(uint32_t posX, uint32_t posY, const PixelEntry& pixelData)
{
	ASSERT(_data.size() == PixelCount());
	_data[posX + posY * _size.width] = pixelData;
}

//----------------------------------------------------------------------------------------
const ImageRGBA::PixelEntry* ImageRGBA::Data() const
{
	return _data.data();
}

//----------------------------------------------------------------------------------------
// Data import methods
//----------------------------------------------------------------------------------------
void ImageRGBA::FromPixelData(const ImageSize& newSize, const Marshal::In<std::vector<PixelEntry>>& data)
{
	_data = data.Get();
	_size = newSize;
	_pixelCount = _size.width * _size.height;
}

//----------------------------------------------------------------------------------------
void ImageRGBA::FromPixelData(const ImageSize& newSize, const PixelEntry* data)
{
	_size = newSize;
	_pixelCount = _size.width * _size.height;
	_data.resize((size_t)newSize.width * (size_t)newSize.height);
	std::memcpy(_data.data(), data, (size_t)newSize.width * (size_t)newSize.height * sizeof(PixelEntry));
}

//----------------------------------------------------------------------------------------
bool ImageRGBA::FromFile(const Marshal::In<std::filesystem::path::string_type>& filePath, cobalt::logging::ILogger* log)
{
	auto stringEndsWithCaseInsensitive = [](const std::filesystem::path::string_type& str, const std::filesystem::path::string_type& suffix) { return (str.size() >= suffix.size()) && (std::equal(str.begin() + (str.size() - suffix.size()), str.end(), suffix.begin(), suffix.end(), [](std::filesystem::path::string_type::value_type a, std::filesystem::path::string_type::value_type b) { return std::toupper((int)a) == std::toupper((int)b); })); };
	if (stringEndsWithCaseInsensitive(filePath, std::filesystem::path(".png").native()))
	{
		return FromPngFile(filePath, log);
	}

	if (log != nullptr)
	{
		log->Error("Unknown image file type: {0}", filePath.Get());
	}
	return false;
}

//----------------------------------------------------------------------------------------
bool ImageRGBA::FromPngFile(const Marshal::In<std::filesystem::path::string_type>& filePath, cobalt::logging::ILogger* log)
{
	std::ifstream file;
	file.open(std::filesystem::path(filePath.Get()), std::ios::in | std::ios::binary | std::ios::ate);
	if (!file.is_open())
	{
		if (log != nullptr)
		{
			log->Error("Can't open image file: {0}", filePath.Get());
		}
		return false;
	}

	std::streamsize fileSizeInBytes = file.tellg();
	file.seekg(0, std::ios::beg);
	std::vector<uint8_t> buffer((size_t)fileSizeInBytes);
	file.read(reinterpret_cast<char*>(buffer.data()), fileSizeInBytes);
	if (file.fail())
	{
		if (log != nullptr)
		{
			log->Error("Can't open image file: {0}", filePath.Get());
		}
		return false;
	}

	if (!FromPngData(buffer.data(), buffer.size(), log))
	{
		if (log != nullptr)
		{
			log->Error("Failed to load image file: {0}", filePath.Get());
		}
		return false;
	}
	return true;
}

//----------------------------------------------------------------------------------------
bool ImageRGBA::FromPngData(const uint8_t* data, size_t size, cobalt::logging::ILogger* log)
{
	const auto pngHeaderSize = 8;
	bool isPng = (size > pngHeaderSize) && (png_sig_cmp(const_cast<png_bytep>(data), 0, pngHeaderSize) == 0);
	if (!isPng)
	{
		if (log != nullptr)
		{
			log->Error("Invalid PNG file data.");
		}
		return false;
	}

	png_structp pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	if (pngPtr == nullptr)
	{
		UNREACHABLE();
		return false;
	}
	png_infop infoPtr = png_create_info_struct(pngPtr);
	if (infoPtr == nullptr)
	{
		UNREACHABLE();
		png_destroy_read_struct(&pngPtr, nullptr, nullptr);
		return false;
	}

	struct PngReader
	{
		png_bytep dataPtr = nullptr;
		png_bytep endPtr = nullptr;

		static void ReadDataFromInputStream(png_structp pngPtr, png_bytep outBytes, png_size_t byteCountToRead)
		{
			png_voidp ioPtr = png_get_io_ptr(pngPtr);
			if (ioPtr == nullptr)
			{
				UNREACHABLE();
			}

			auto& object = *reinterpret_cast<PngReader*>(ioPtr);

			auto bytesLeft = object.endPtr - object.dataPtr;

			if (bytesLeft < (int64_t)byteCountToRead)
			{
				byteCountToRead = bytesLeft;
			}

			std::memcpy(outBytes, object.dataPtr, byteCountToRead);
			object.dataPtr += byteCountToRead;
		}
	};

	PngReader reader;
	reader.dataPtr = const_cast<png_bytep>(data);
	reader.endPtr = const_cast<png_bytep>(data) + size;

	png_set_read_fn(pngPtr, &reader, PngReader::ReadDataFromInputStream);

	png_read_info(pngPtr, infoPtr);

	png_uint_32 width = 0;
	png_uint_32 height = 0;
	int bitDepth = 0;
	int colorType = -1;
	png_uint_32 retval = png_get_IHDR(pngPtr, infoPtr, &width, &height, &bitDepth, &colorType, nullptr, nullptr, nullptr);

	if (retval != 1)
	{
		if (log != nullptr)
		{
			log->Error("Can't parse PNG image - png_get_IHDR failed");
		}
		return false;
	}

	ClearAndResize({width, height});

	const auto bytesPerRow = png_get_rowbytes(pngPtr, infoPtr);
	std::vector<png_byte> rowData(bytesPerRow);
	if (colorType == PNG_COLOR_TYPE_RGB)
	{
		for (auto rowIdx = 0u; rowIdx < height; ++rowIdx)
		{
			png_read_row(pngPtr, rowData.data(), nullptr);

			auto byteIndex = 0u;
			for (auto colIdx = 0u; colIdx < width; ++colIdx)
			{
				const auto red = rowData[byteIndex++];
				const auto green = rowData[byteIndex++];
				const auto blue = rowData[byteIndex++];
				WritePixel(colIdx, rowIdx, PixelEntry{red, green, blue, 255});
			}
			ASSERT(byteIndex == bytesPerRow);
		}
	}
	else if (colorType == PNG_COLOR_TYPE_RGBA)
	{
		for (auto rowIdx = 0u; rowIdx < height; ++rowIdx)
		{
			png_read_row(pngPtr, reinterpret_cast<png_bytep>(_data.data() + ((size_t)rowIdx * (size_t)_size.width)), nullptr);
		}
	}
	else if (colorType == PNG_COLOR_TYPE_GRAY)
	{
		for (auto rowIdx = 0u; rowIdx < height; ++rowIdx)
		{
			png_read_row(pngPtr, rowData.data(), nullptr);

			auto byteIndex = 0u;
			for (auto colIdx = 0u; colIdx < width; ++colIdx)
			{
				const auto grey = rowData[byteIndex++];
				WritePixel(colIdx, rowIdx, PixelEntry{grey, grey, grey, 255});
			}
			ASSERT(byteIndex == bytesPerRow);
		}
	}
	else if (colorType == PNG_COLOR_TYPE_GRAY_ALPHA)
	{
		for (auto rowIdx = 0u; rowIdx < height; ++rowIdx)
		{
			png_read_row(pngPtr, rowData.data(), nullptr);

			auto byteIndex = 0u;
			for (auto colIdx = 0u; colIdx < width; ++colIdx)
			{
				const auto grey = rowData[byteIndex++];
				const auto alpha = rowData[byteIndex++];
				WritePixel(colIdx, rowIdx, PixelEntry{grey, grey, grey, alpha});
			}
			ASSERT(byteIndex == bytesPerRow);
		}
	}
	else
	{
		if (log != nullptr)
		{
			log->Error("Can't parse PNG image - unknown color type");
		}
		return false;
	}
	png_destroy_read_struct(&pngPtr, &infoPtr, nullptr);

	ASSERT(_data.size() == PixelCount());
	ASSERT(!_data.empty());
	return true;
}

//----------------------------------------------------------------------------------------
// Data export methods
//----------------------------------------------------------------------------------------
bool ImageRGBA::ToPngFile(const Marshal::In<std::filesystem::path::string_type>& filePath, cobalt::logging::ILogger* log) const
{
	std::vector<uint8_t> data;
	if (!ToPngData(data, log))
	{
		if (log != nullptr)
		{
			log->Error("Failed to generate PNG data for image");
		}
		return false;
	}

	std::ofstream file;
	file.open(std::filesystem::path(filePath.Get()), std::ios::out | std::ios::binary);
	file.write(reinterpret_cast<char*>(data.data()), data.size());
	file.close();
	if (file.fail())
	{
		if (log != nullptr)
		{
			log->Error("Failed to write PNG data to file {0}", filePath.Get());
		}
		return false;
	}
	return true;
}

//----------------------------------------------------------------------------------------
bool ImageRGBA::ToPngData(const Marshal::Out<std::vector<uint8_t>>& data, cobalt::logging::ILogger* log) const
{
	struct MemEncode
	{
		std::vector<uint8_t>* data = nullptr;

		static void WritePngData(png_structp pngPtr, png_bytep pngData, png_size_t length)
		{
			auto* p = reinterpret_cast<MemEncode*>(png_get_io_ptr(pngPtr));

			p->data->insert(p->data->end(), pngData, pngData + length);
		}

		static void PngFlush(png_structp pngPtr)
		{}
	};

	// Initialize the write struct
	auto* pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	if (pngPtr == nullptr)
	{
		UNREACHABLE();
		return false;
	}

	// Initialize the info struct
	auto* infoPtr = png_create_info_struct(pngPtr);
	if (infoPtr == nullptr)
	{
		UNREACHABLE();
		return false;
	}

	// Set image attributes
	png_set_IHDR(pngPtr, infoPtr, Size().width, Size().height, 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	std::vector<png_bytep> rowPointers(Size().height);
	for (auto i = 0u; i < Size().height; ++i)
	{
		rowPointers[i] = png_bytep(const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(_data.data() + ((size_t)Size().width * (size_t)i))));
	}

	MemEncode state = {};
	std::vector<uint8_t> dataTemp;
	state.data = &dataTemp;

	png_set_write_fn(pngPtr, &state, MemEncode::WritePngData, MemEncode::PngFlush);

	png_set_rows(pngPtr, infoPtr, rowPointers.data());
	png_write_png(pngPtr, infoPtr, PNG_TRANSFORM_IDENTITY, nullptr);

	png_destroy_write_struct(&pngPtr, &infoPtr);

	data.Set(std::move(dataTemp));
	return true;
}

} // namespace cobalt::graphics
