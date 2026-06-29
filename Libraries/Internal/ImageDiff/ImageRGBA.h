// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "IImageRGBA.h"
namespace cobalt::graphics {

class ImageRGBA : public IImageRGBA
{
public:
	// Delete method
	void Delete() override;

	// Initialization methods
	void ClearAndResize(const ImageSize& newSize) override;

	// Format methods
	ImageSize Size() const override;
	uint32_t PixelCount() const override;

	// Data access methods
	PixelEntry ReadPixel(uint32_t posX, uint32_t posY) const override;
	void ReadPixel(uint32_t posX, uint32_t posY, PixelEntry& pixelData) const override;
	void WritePixel(uint32_t posX, uint32_t posY, const PixelEntry& pixelData) override;
	const PixelEntry* Data() const override;

	// Data import methods
	void FromPixelData(const ImageSize& newSize, const Marshal::In<std::vector<PixelEntry>>& data) override;
	void FromPixelData(const ImageSize& newSize, const PixelEntry* data) override;
	bool FromFile(const Marshal::In<std::filesystem::path::string_type>& filePath, cobalt::logging::ILogger* log) override;
	bool FromPngFile(const Marshal::In<std::filesystem::path::string_type>& filePath, cobalt::logging::ILogger* log) override;
	bool FromPngData(const uint8_t* data, size_t size, cobalt::logging::ILogger* log) override;

	// Data export methods
	bool ToPngFile(const Marshal::In<std::filesystem::path::string_type>& filePath, cobalt::logging::ILogger* log) const override;
	bool ToPngData(const Marshal::Out<std::vector<uint8_t>>& data, cobalt::logging::ILogger* log) const override;

private:
	std::vector<PixelEntry> _data;
	ImageSize _size = {0, 0};
	uint32_t _pixelCount = 0;
};

} // namespace cobalt::graphics
