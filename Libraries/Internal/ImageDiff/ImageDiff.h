// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "IImageDiff.h"
#include <cstdint>
namespace cobalt::graphics {

class ImageDiff : public IImageDiff
{
public:
	// Constructors
	ImageDiff(const IImageRGBA& expected, const IImageRGBA& actual);

	// Delete method
	void Delete() override;

	// Test methods
	bool CompareImages(Algorithm algorithm, double passThreshold, double& diffValue, const Marshal::Out<std::map<std::string, std::string>>& explanationMetaDataString) const override;

private:
	// Test methods
	static double DoBinaryCountExact(const IImageRGBA& expected, const IImageRGBA& actual, std::map<std::string, std::string>& explanationMetaDataString);
	static double DoNaiveDiff(const IImageRGBA& expected, const IImageRGBA& actual, std::map<std::string, std::string>& explanationMetaDataString);
	static double DoValueSearch(const IImageRGBA& expected, const IImageRGBA& actual, std::map<std::string, std::string>& explanationMetaDataString, uint16_t maxRange);
	static double DoRegionRanges(const IImageRGBA& expected, const IImageRGBA& actual, std::map<std::string, std::string>& explanationMetaDataString);

private:
	const IImageRGBA& _expected;
	const IImageRGBA& _actual;
};

} // namespace cobalt::graphics
