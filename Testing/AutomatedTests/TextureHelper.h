// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif
namespace cobalt::graphics::testing {

class TextureHelper
{
public:
	// Structures

	// 2D RGBA texture with optional generated mipmap levels.
	struct TextureInfo
	{
		cobalt::graphics::V2UInt32 size = {};
		uint32_t mipmapLevelCount = 0;
		std::vector<std::vector<cobalt::graphics::V4UInt8>> mipmapTextureData;
	};
	struct CompressedTextureInfo
	{
		cobalt::graphics::V2UInt32 size = {};
		cobalt::graphics::ITextureBuffer::ImageFormat imageFormat = cobalt::graphics::ITextureBuffer::ImageFormat::RGBA;
		cobalt::graphics::ITextureBuffer::DataFormat dataFormat = cobalt::graphics::ITextureBuffer::DataFormat::DXT1;
		cobalt::graphics::ITextureBuffer::SourceImageFormat sourceImageFormat = cobalt::graphics::ITextureBuffer::SourceImageFormat::RGBA;
		cobalt::graphics::ITextureBuffer::SourceDataFormat sourceDataFormat = cobalt::graphics::ITextureBuffer::SourceDataFormat::DXT1;
		uint32_t mipmapLevelCount = 0;
		std::vector<std::vector<uint8_t>> mipmapTextureData;
	};

public:
	// Constructors
	explicit TextureHelper(cobalt::logging::ILogger::unique_ptr log);

	// Load methods
	std::unique_ptr<TextureInfo> LoadImageFromPngFile(const std::string& fileName, bool generateMipmaps = false) const;
	std::unique_ptr<CompressedTextureInfo> LoadImageFromDdsFile(const std::string& fileName) const;
	std::unique_ptr<CompressedTextureInfo> LoadImageFromKtxFile(const std::string& fileName) const;
	std::unique_ptr<CompressedTextureInfo> LoadImageFromAstcFile(const std::string& fileName) const;

private:
	// Helper methods
	static std::vector<cobalt::graphics::V4UInt8> ResampleImageBilinear(const std::vector<cobalt::graphics::V4UInt8>& sourceData, const cobalt::graphics::V2UInt32& sourceSize, const cobalt::graphics::V2UInt32& targetSize);
	static std::vector<cobalt::graphics::V4UInt8> ResampleImageBicubic(const std::vector<cobalt::graphics::V4UInt8>& sourceData, const cobalt::graphics::V2UInt32& sourceSize, const cobalt::graphics::V2UInt32& targetSize);
	static void GenerateMipmaps(TextureInfo& textureInfo);
	static std::filesystem::path GetProcessDirectory();
#ifdef _WIN32
	static HMODULE GetCurrentModule();
#endif

private:
	cobalt::logging::ILogger::unique_ptr _log;
};

}; // namespace cobalt::graphics::testing
