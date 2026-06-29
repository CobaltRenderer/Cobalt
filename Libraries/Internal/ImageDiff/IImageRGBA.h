// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/Marshalling/Marshalling.pkg>
#include <filesystem>
#include <memory>
#include <vector>
namespace cobalt::graphics {
using namespace cobalt::marshalling::operators;

class IImageRGBA
{
public:
	// Structures
	struct ImageSize
	{
		unsigned int width;
		unsigned int height;
	};
	struct PixelEntry
	{
		uint8_t r;
		uint8_t g;
		uint8_t b;
		uint8_t a;
	};

	// Nested types
	struct Deleter
	{
		inline void operator()(IImageRGBA* target)
		{
			target->Delete();
		}
	};
	typedef std::unique_ptr<IImageRGBA, Deleter> unique_ptr;

public:
	// Constructors
	static inline unique_ptr Create();

	// Delete method
	virtual void Delete() = 0;

	// Initialization methods
	virtual void ClearAndResize(const ImageSize& newSize) = 0;

	// Format methods
	virtual ImageSize Size() const = 0;
	virtual uint32_t PixelCount() const = 0;

	// Data access methods
	virtual PixelEntry ReadPixel(uint32_t posX, uint32_t posY) const = 0;
	virtual void ReadPixel(uint32_t posX, uint32_t posY, PixelEntry& pixelData) const = 0;
	virtual void WritePixel(uint32_t posX, uint32_t posY, const PixelEntry& pixelData) = 0;
	virtual const PixelEntry* Data() const = 0;

	// Data import methods
	virtual void FromPixelData(const ImageSize& newSize, const Marshal::In<std::vector<PixelEntry>>& data) = 0;
	virtual void FromPixelData(const ImageSize& newSize, const PixelEntry* data) = 0;
	virtual bool FromFile(const Marshal::In<std::filesystem::path::string_type>& filePath, cobalt::logging::ILogger* log = nullptr) = 0;
	virtual bool FromPngFile(const Marshal::In<std::filesystem::path::string_type>& filePath, cobalt::logging::ILogger* log = nullptr) = 0;
	virtual bool FromPngData(const uint8_t* data, size_t size, cobalt::logging::ILogger* log = nullptr) = 0;

	// Data export methods
	virtual bool ToPngFile(const Marshal::In<std::filesystem::path::string_type>& filePath, cobalt::logging::ILogger* log = nullptr) const = 0;
	virtual bool ToPngData(const Marshal::Out<std::vector<uint8_t>>& data, cobalt::logging::ILogger* log = nullptr) const = 0;

protected:
	// Constructors
	inline ~IImageRGBA() = default;
};

} // namespace cobalt::graphics
#include "IImageRGBA.inl"
