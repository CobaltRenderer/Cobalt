// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "TextureHelper.h"
#include "UnicodeConversion.h"
#include <Internal/ImageDiff/ImageDiff.pkg>
#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#ifndef NDEBUG
#define NDEBUG
#define COBALT_RESTORE_NDEBUG_AFTER_GLI
#endif
#include <gli/load_dds.hpp>
#include <gli/load_ktx.hpp>
#ifdef COBALT_RESTORE_NDEBUG_AFTER_GLI
#undef NDEBUG
#undef COBALT_RESTORE_NDEBUG_AFTER_GLI
#endif
#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <sys/syslimits.h>
#elif !defined(_WIN32)
#include <climits>
#endif
namespace cobalt::graphics::testing {
using namespace cobalt::graphics;

namespace {
//----------------------------------------------------------------------------------------
bool TryMapGliCompressedFormat(gli::format format, ITextureBuffer::ImageFormat& imageFormat, ITextureBuffer::DataFormat& dataFormat, ITextureBuffer::SourceImageFormat& sourceImageFormat, ITextureBuffer::SourceDataFormat& sourceDataFormat)
{
	switch (format)
	{
	case gli::FORMAT_RGB_DXT1_UNORM_BLOCK8:
	case gli::FORMAT_RGB_DXT1_SRGB_BLOCK8:
		imageFormat = ITextureBuffer::ImageFormat::RGB;
		dataFormat = ITextureBuffer::DataFormat::DXT1;
		sourceImageFormat = ITextureBuffer::SourceImageFormat::RGB;
		sourceDataFormat = ITextureBuffer::SourceDataFormat::DXT1;
		return true;
	case gli::FORMAT_RGBA_DXT1_UNORM_BLOCK8:
	case gli::FORMAT_RGBA_DXT1_SRGB_BLOCK8:
		imageFormat = ITextureBuffer::ImageFormat::RGBA;
		dataFormat = ITextureBuffer::DataFormat::DXT1;
		sourceImageFormat = ITextureBuffer::SourceImageFormat::RGBA;
		sourceDataFormat = ITextureBuffer::SourceDataFormat::DXT1;
		return true;
	case gli::FORMAT_RGBA_DXT3_UNORM_BLOCK16:
	case gli::FORMAT_RGBA_DXT3_SRGB_BLOCK16:
		imageFormat = ITextureBuffer::ImageFormat::RGBA;
		dataFormat = ITextureBuffer::DataFormat::DXT3;
		sourceImageFormat = ITextureBuffer::SourceImageFormat::RGBA;
		sourceDataFormat = ITextureBuffer::SourceDataFormat::DXT3;
		return true;
	case gli::FORMAT_RGBA_DXT5_UNORM_BLOCK16:
	case gli::FORMAT_RGBA_DXT5_SRGB_BLOCK16:
		imageFormat = ITextureBuffer::ImageFormat::RGBA;
		dataFormat = ITextureBuffer::DataFormat::DXT5;
		sourceImageFormat = ITextureBuffer::SourceImageFormat::RGBA;
		sourceDataFormat = ITextureBuffer::SourceDataFormat::DXT5;
		return true;
	case gli::FORMAT_RGB_ETC2_UNORM_BLOCK8:
	case gli::FORMAT_RGB_ETC2_SRGB_BLOCK8:
		imageFormat = ITextureBuffer::ImageFormat::RGB;
		dataFormat = ITextureBuffer::DataFormat::ETC2;
		sourceImageFormat = ITextureBuffer::SourceImageFormat::RGB;
		sourceDataFormat = ITextureBuffer::SourceDataFormat::ETC2;
		return true;
	case gli::FORMAT_RGBA_ETC2_UNORM_BLOCK16:
	case gli::FORMAT_RGBA_ETC2_SRGB_BLOCK16:
		imageFormat = ITextureBuffer::ImageFormat::RGBA;
		dataFormat = ITextureBuffer::DataFormat::ETC2;
		sourceImageFormat = ITextureBuffer::SourceImageFormat::RGBA;
		sourceDataFormat = ITextureBuffer::SourceDataFormat::ETC2;
		return true;
	case gli::FORMAT_RGBA_BP_UNORM_BLOCK16:
	case gli::FORMAT_RGBA_BP_SRGB_BLOCK16:
		imageFormat = ITextureBuffer::ImageFormat::RGBA;
		dataFormat = ITextureBuffer::DataFormat::BPTC;
		sourceImageFormat = ITextureBuffer::SourceImageFormat::RGBA;
		sourceDataFormat = ITextureBuffer::SourceDataFormat::BPTC;
		return true;
	case gli::FORMAT_RGBA_ASTC_4X4_UNORM_BLOCK16:
	case gli::FORMAT_RGBA_ASTC_4X4_SRGB_BLOCK16:
		imageFormat = ITextureBuffer::ImageFormat::RGBA;
		dataFormat = ITextureBuffer::DataFormat::ASTC4x4;
		sourceImageFormat = ITextureBuffer::SourceImageFormat::RGBA;
		sourceDataFormat = ITextureBuffer::SourceDataFormat::ASTC4x4;
		return true;
	case gli::FORMAT_RGBA_ASTC_5X5_UNORM_BLOCK16:
	case gli::FORMAT_RGBA_ASTC_5X5_SRGB_BLOCK16:
		imageFormat = ITextureBuffer::ImageFormat::RGBA;
		dataFormat = ITextureBuffer::DataFormat::ASTC5x5;
		sourceImageFormat = ITextureBuffer::SourceImageFormat::RGBA;
		sourceDataFormat = ITextureBuffer::SourceDataFormat::ASTC5x5;
		return true;
	case gli::FORMAT_RGBA_ASTC_6X6_UNORM_BLOCK16:
	case gli::FORMAT_RGBA_ASTC_6X6_SRGB_BLOCK16:
		imageFormat = ITextureBuffer::ImageFormat::RGBA;
		dataFormat = ITextureBuffer::DataFormat::ASTC6x6;
		sourceImageFormat = ITextureBuffer::SourceImageFormat::RGBA;
		sourceDataFormat = ITextureBuffer::SourceDataFormat::ASTC6x6;
		return true;
	case gli::FORMAT_RGBA_ASTC_8X8_UNORM_BLOCK16:
	case gli::FORMAT_RGBA_ASTC_8X8_SRGB_BLOCK16:
		imageFormat = ITextureBuffer::ImageFormat::RGBA;
		dataFormat = ITextureBuffer::DataFormat::ASTC8x8;
		sourceImageFormat = ITextureBuffer::SourceImageFormat::RGBA;
		sourceDataFormat = ITextureBuffer::SourceDataFormat::ASTC8x8;
		return true;
	default:
		return false;
	}
}

//----------------------------------------------------------------------------------------
bool TryMapAstcBlockDimensions(uint32_t blockWidth, uint32_t blockHeight, ITextureBuffer::DataFormat& dataFormat, ITextureBuffer::SourceDataFormat& sourceDataFormat)
{
	if ((blockWidth == 4) && (blockHeight == 4))
	{
		dataFormat = ITextureBuffer::DataFormat::ASTC4x4;
		sourceDataFormat = ITextureBuffer::SourceDataFormat::ASTC4x4;
		return true;
	}
	if ((blockWidth == 5) && (blockHeight == 5))
	{
		dataFormat = ITextureBuffer::DataFormat::ASTC5x5;
		sourceDataFormat = ITextureBuffer::SourceDataFormat::ASTC5x5;
		return true;
	}
	if ((blockWidth == 6) && (blockHeight == 6))
	{
		dataFormat = ITextureBuffer::DataFormat::ASTC6x6;
		sourceDataFormat = ITextureBuffer::SourceDataFormat::ASTC6x6;
		return true;
	}
	if ((blockWidth == 8) && (blockHeight == 8))
	{
		dataFormat = ITextureBuffer::DataFormat::ASTC8x8;
		sourceDataFormat = ITextureBuffer::SourceDataFormat::ASTC8x8;
		return true;
	}
	return false;
}

//----------------------------------------------------------------------------------------
uint32_t ReadUInt24LittleEndian(const uint8_t* value)
{
	return static_cast<uint32_t>(value[0]) | (static_cast<uint32_t>(value[1]) << 8) | (static_cast<uint32_t>(value[2]) << 16);
}

//----------------------------------------------------------------------------------------
std::unique_ptr<TextureHelper::CompressedTextureInfo> ExtractCompressedTextureInfoFromGliTexture(const gli::texture& texture, const std::filesystem::path& path, const char* containerName, cobalt::logging::ILogger* log)
{
	if (texture.empty())
	{
		log->Error("Failed to load compressed texture from {0} file \"{1}\"", containerName, path);
		return nullptr;
	}
	if (texture.target() != gli::TARGET_2D)
	{
		log->Error("Failed to load compressed texture from {0} file \"{1}\". Only 2D textures are supported.", containerName, path);
		return nullptr;
	}
	if ((texture.layers() != 1) || (texture.faces() != 1))
	{
		log->Error("Failed to load compressed texture from {0} file \"{1}\". Texture arrays and cube maps are not supported by this helper.", containerName, path);
		return nullptr;
	}

	auto info = TextureHelper::CompressedTextureInfo();
	if (!TryMapGliCompressedFormat(texture.format(), info.imageFormat, info.dataFormat, info.sourceImageFormat, info.sourceDataFormat))
	{
		log->Error("Failed to load compressed texture from {0} file \"{1}\". The format {2} is not supported by this helper.", containerName, path, (int)texture.format());
		return nullptr;
	}

	auto baseExtent = texture.extent(0);
	info.size = V2UInt32((uint32_t)baseExtent.x, (uint32_t)baseExtent.y);
	info.mipmapLevelCount = (uint32_t)texture.levels();
	info.mipmapTextureData.reserve(info.mipmapLevelCount);
	for (uint32_t mipmapLevel = 0; mipmapLevel < info.mipmapLevelCount; ++mipmapLevel)
	{
		auto* levelData = reinterpret_cast<const uint8_t*>(texture.data(0, 0, mipmapLevel));
		info.mipmapTextureData.emplace_back(levelData, levelData + texture.size(mipmapLevel));
	}

	return std::make_unique<TextureHelper::CompressedTextureInfo>(std::move(info));
}
} // namespace

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
TextureHelper::TextureHelper(cobalt::logging::ILogger::unique_ptr log)
: _log(std::move(log))
{}

//----------------------------------------------------------------------------------------
// Load methods
//----------------------------------------------------------------------------------------
std::unique_ptr<TextureHelper::TextureInfo> TextureHelper::LoadImageFromPngFile(const std::string& fileName, bool generateMipmaps) const
{
	// Build a path to the target image resource file
	std::filesystem::path path = GetProcessDirectory() / "Resources" / fileName;

	// Load the image from the target file
	auto image = IImageRGBA::Create();
	if (!image->FromPngFile(path.native(), _log.get()))
	{
		_log->Error("Failed to load texture from file \"{0}\"", path);
		return nullptr;
	}

	// Return the image data to the caller
	auto info = TextureHelper::TextureInfo();
	auto imageSize = image->Size();
	info.size = V2UInt32(imageSize.width, imageSize.height);
	info.mipmapTextureData.emplace_back(reinterpret_cast<const V4UInt8*>(image->Data()), reinterpret_cast<const V4UInt8*>(image->Data()) + image->PixelCount());
	if (generateMipmaps)
	{
		GenerateMipmaps(info);
	}
	info.mipmapLevelCount = (uint32_t)info.mipmapTextureData.size();
	return std::make_unique<TextureHelper::TextureInfo>(std::move(info));
}

//----------------------------------------------------------------------------------------
std::unique_ptr<TextureHelper::CompressedTextureInfo> TextureHelper::LoadImageFromDdsFile(const std::string& fileName) const
{
	// Build a path to the target image resource file
	std::filesystem::path path = GetProcessDirectory() / "Resources" / fileName;

	// Load the image from the target file
	gli::texture texture = gli::load_dds(path.string());
	return ExtractCompressedTextureInfoFromGliTexture(texture, path, "DDS", _log.get());
}

//----------------------------------------------------------------------------------------
std::unique_ptr<TextureHelper::CompressedTextureInfo> TextureHelper::LoadImageFromKtxFile(const std::string& fileName) const
{
	// Build a path to the target image resource file
	std::filesystem::path path = GetProcessDirectory() / "Resources" / fileName;

	// Load the image from the target file
	gli::texture texture = gli::load_ktx(path.string());
	return ExtractCompressedTextureInfoFromGliTexture(texture, path, "KTX", _log.get());
}

//----------------------------------------------------------------------------------------
std::unique_ptr<TextureHelper::CompressedTextureInfo> TextureHelper::LoadImageFromAstcFile(const std::string& fileName) const
{
	// Build a path to the target image resource file
	std::filesystem::path path = GetProcessDirectory() / "Resources" / fileName;

	// Load the ASTC file from disk. The .astc container only stores a single image surface, so this helper exposes a
	// single mipmap level.
	std::ifstream inputFile(path, std::ios::binary);
	if (!inputFile)
	{
		_log->Error("Failed to load compressed texture from ASTC file \"{0}\"", path);
		return nullptr;
	}
	std::vector<uint8_t> fileData((std::istreambuf_iterator<char>(inputFile)), std::istreambuf_iterator<char>());
	if (fileData.size() < 16)
	{
		_log->Error("Failed to load compressed texture from ASTC file \"{0}\". The file is smaller than the ASTC header.", path);
		return nullptr;
	}
	if ((fileData[0] != 0x13) || (fileData[1] != 0xAB) || (fileData[2] != 0xA1) || (fileData[3] != 0x5C))
	{
		_log->Error("Failed to load compressed texture from ASTC file \"{0}\". The ASTC magic number is invalid.", path);
		return nullptr;
	}
	uint32_t blockWidth = fileData[4];
	uint32_t blockHeight = fileData[5];
	uint32_t blockDepth = fileData[6];
	if (blockDepth != 1)
	{
		_log->Error("Failed to load compressed texture from ASTC file \"{0}\". Only 2D ASTC textures are supported by this helper.", path);
		return nullptr;
	}
	ITextureBuffer::DataFormat dataFormat = {};
	ITextureBuffer::SourceDataFormat sourceDataFormat = {};
	if (!TryMapAstcBlockDimensions(blockWidth, blockHeight, dataFormat, sourceDataFormat))
	{
		_log->Error("Failed to load compressed texture from ASTC file \"{0}\". The ASTC block size {1}x{2} is not supported by this helper.", path, blockWidth, blockHeight);
		return nullptr;
	}

	uint32_t imageWidth = ReadUInt24LittleEndian(&fileData[7]);
	uint32_t imageHeight = ReadUInt24LittleEndian(&fileData[10]);
	uint32_t imageDepth = ReadUInt24LittleEndian(&fileData[13]);
	if ((imageWidth == 0) || (imageHeight == 0) || (imageDepth != 1))
	{
		_log->Error("Failed to load compressed texture from ASTC file \"{0}\". Only non-empty 2D ASTC textures are supported by this helper.", path);
		return nullptr;
	}

	size_t blockCountX = (imageWidth + (blockWidth - 1)) / blockWidth;
	size_t blockCountY = (imageHeight + (blockHeight - 1)) / blockHeight;
	size_t expectedPayloadSize = blockCountX * blockCountY * 16;
	if (fileData.size() != (16 + expectedPayloadSize))
	{
		_log->Error("Failed to load compressed texture from ASTC file \"{0}\". The payload has {1} bytes, but {2} bytes were expected.", path, fileData.size() - 16, expectedPayloadSize);
		return nullptr;
	}

	auto info = TextureHelper::CompressedTextureInfo();
	info.size = V2UInt32(imageWidth, imageHeight);
	info.imageFormat = ITextureBuffer::ImageFormat::RGBA;
	info.dataFormat = dataFormat;
	info.sourceImageFormat = ITextureBuffer::SourceImageFormat::RGBA;
	info.sourceDataFormat = sourceDataFormat;
	info.mipmapLevelCount = 1;
	info.mipmapTextureData.emplace_back(fileData.begin() + 16, fileData.end());
	return std::make_unique<TextureHelper::CompressedTextureInfo>(std::move(info));
}

//----------------------------------------------------------------------------------------
std::vector<V4UInt8> TextureHelper::ResampleImageBilinear(const std::vector<V4UInt8>& sourceData, const V2UInt32& sourceSize, const V2UInt32& targetSize)
{
	if ((targetSize.X() == 0) || (targetSize.Y() == 0))
	{
		return {};
	}

	auto sampleTexel = [&](int x, int y) {
		x = std::clamp(x, 0, (int)sourceSize.X() - 1);
		y = std::clamp(y, 0, (int)sourceSize.Y() - 1);
		return sourceData[(size_t)x + ((size_t)y * sourceSize.X())];
	};

	auto lerp = [](float a, float b, float t) {
		return a + ((b - a) * t);
	};

	std::vector<V4UInt8> targetData;
	targetData.resize((size_t)targetSize.X() * targetSize.Y());
	for (uint32_t y = 0; y < targetSize.Y(); ++y)
	{
		float sourceY = (((float)y + 0.5f) * (float)sourceSize.Y() / (float)targetSize.Y()) - 0.5f;
		int sourceY0 = (int)std::floor(sourceY);
		int sourceY1 = sourceY0 + 1;
		float fracY = sourceY - (float)sourceY0;
		for (uint32_t x = 0; x < targetSize.X(); ++x)
		{
			float sourceX = (((float)x + 0.5f) * (float)sourceSize.X() / (float)targetSize.X()) - 0.5f;
			int sourceX0 = (int)std::floor(sourceX);
			int sourceX1 = sourceX0 + 1;
			float fracX = sourceX - (float)sourceX0;

			auto texel00 = sampleTexel(sourceX0, sourceY0);
			auto texel10 = sampleTexel(sourceX1, sourceY0);
			auto texel01 = sampleTexel(sourceX0, sourceY1);
			auto texel11 = sampleTexel(sourceX1, sourceY1);

			V4UInt8 outPixel;
			for (size_t component = 0; component < 4; ++component)
			{
				float top = lerp((float)texel00.data()[component], (float)texel10.data()[component], fracX);
				float bottom = lerp((float)texel01.data()[component], (float)texel11.data()[component], fracX);
				outPixel.data()[component] = (uint8_t)std::clamp(std::lround(lerp(top, bottom, fracY)), 0L, 255L);
			}
			targetData[(size_t)x + ((size_t)y * targetSize.X())] = outPixel;
		}
	}
	return targetData;
}

//----------------------------------------------------------------------------------------
std::vector<V4UInt8> TextureHelper::ResampleImageBicubic(const std::vector<V4UInt8>& sourceData, const V2UInt32& sourceSize, const V2UInt32& targetSize)
{
	if ((targetSize.X() == 0) || (targetSize.Y() == 0))
	{
		return {};
	}

	auto sampleTexel = [&](int x, int y) {
		x = std::clamp(x, 0, (int)sourceSize.X() - 1);
		y = std::clamp(y, 0, (int)sourceSize.Y() - 1);
		return sourceData[(size_t)x + ((size_t)y * sourceSize.X())];
	};

	auto cubicInterpolate = [](float p0, float p1, float p2, float p3, float t) {
		float a0 = (-0.5f * p0) + (1.5f * p1) - (1.5f * p2) + (0.5f * p3);
		float a1 = p0 - (2.5f * p1) + (2.0f * p2) - (0.5f * p3);
		float a2 = (-0.5f * p0) + (0.5f * p2);
		float a3 = p1;
		return ((a0 * t + a1) * t + a2) * t + a3;
	};

	std::vector<V4UInt8> targetData;
	targetData.resize((size_t)targetSize.X() * targetSize.Y());
	for (uint32_t y = 0; y < targetSize.Y(); ++y)
	{
		float sourceY = (((float)y + 0.5f) * (float)sourceSize.Y() / (float)targetSize.Y()) - 0.5f;
		int sourceYBase = (int)std::floor(sourceY);
		float fracY = sourceY - (float)sourceYBase;
		for (uint32_t x = 0; x < targetSize.X(); ++x)
		{
			float sourceX = (((float)x + 0.5f) * (float)sourceSize.X() / (float)targetSize.X()) - 0.5f;
			int sourceXBase = (int)std::floor(sourceX);
			float fracX = sourceX - (float)sourceXBase;

			V4UInt8 outPixel;
			for (size_t component = 0; component < 4; ++component)
			{
				std::array<float, 4> rowSamples{};
				for (int rowOffset = -1; rowOffset <= 2; ++rowOffset)
				{
					std::array<float, 4> columnSamples{};
					for (int columnOffset = -1; columnOffset <= 2; ++columnOffset)
					{
						columnSamples[columnOffset + 1] = (float)sampleTexel(sourceXBase + columnOffset, sourceYBase + rowOffset).data()[component];
					}
					rowSamples[rowOffset + 1] = cubicInterpolate(columnSamples[0], columnSamples[1], columnSamples[2], columnSamples[3], fracX);
				}
				outPixel.data()[component] = (uint8_t)std::clamp(std::lround(cubicInterpolate(rowSamples[0], rowSamples[1], rowSamples[2], rowSamples[3], fracY)), 0L, 255L);
			}
			targetData[(size_t)x + ((size_t)y * targetSize.X())] = outPixel;
		}
	}
	return targetData;
}

//----------------------------------------------------------------------------------------
void TextureHelper::GenerateMipmaps(TextureInfo& textureInfo)
{
	V2UInt32 currentSize = textureInfo.size;
	while ((currentSize.X() > 1) || (currentSize.Y() > 1))
	{
		V2UInt32 nextSize(std::max(1U, currentSize.X() / 2), std::max(1U, currentSize.Y() / 2));
		textureInfo.mipmapTextureData.push_back(ResampleImageBicubic(textureInfo.mipmapTextureData.back(), currentSize, nextSize));
		currentSize = nextSize;
	}
}

//----------------------------------------------------------------------------------------
// Helper methods
//----------------------------------------------------------------------------------------
std::filesystem::path TextureHelper::GetProcessDirectory()
{
	// Get the path to the directory we're running from
#ifdef _WIN32
	wchar_t filePathBuffer[MAX_PATH];
	GetModuleFileNameW(GetModuleHandle(nullptr), &filePathBuffer[0], MAX_PATH);
	auto executablePath = std::filesystem::path(&filePathBuffer[0]);
#elif defined(__APPLE__)
	uint32_t filePathLength = 0;
	_NSGetExecutablePath(nullptr, &filePathLength);
	std::vector<char> filePathBuffer(filePathLength + 1, 0);
	// Note that _NSGetExecutablePath does NOT null terminate the output, nor does it update filePathLength when it
	// succeeds.
	_NSGetExecutablePath(filePathBuffer.data(), &filePathLength);
	auto executablePath = std::filesystem::path(std::string(filePathBuffer.data(), filePathLength));
#else
	char filePathBuffer[PATH_MAX];
	ssize_t filePathLengthInChars = readlink("/proc/self/exe", &filePathBuffer[0], PATH_MAX - 1);
	filePathBuffer[filePathLengthInChars] = 0;
	auto executablePath = std::filesystem::path(filePathBuffer);
#endif
	return executablePath.parent_path();
}

//----------------------------------------------------------------------------------------
#ifdef _WIN32
HMODULE TextureHelper::GetCurrentModule()
{
	HMODULE hModule = nullptr;
	GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCTSTR>(GetCurrentModule), &hModule);
	return hModule;
}
#endif

}; // namespace cobalt::graphics::testing
