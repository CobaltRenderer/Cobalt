// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "ImageDiff.h"
#include "ExportMacro.h"
#include "ImageRGBA.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <sstream>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
namespace internal {
extern "C" IMAGEDIFF_EXPORT IImageDiff* CreateIImageDiff(const IImageRGBA& expected, const IImageRGBA& actual)
{
	return new ImageDiff(expected, actual);
}
} // namespace internal

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
ImageDiff::ImageDiff(const IImageRGBA& expected, const IImageRGBA& actual)
: _expected(expected), _actual(actual)
{}

//----------------------------------------------------------------------------------------
// Delete method
//----------------------------------------------------------------------------------------
void ImageDiff::Delete()
{
	delete this;
}

//----------------------------------------------------------------------------------------
// Test methods
//----------------------------------------------------------------------------------------
bool ImageDiff::CompareImages(Algorithm algorithm, double passThreshold, double& diffValue, const Marshal::Out<std::map<std::string, std::string>>& explanationMetaDataString) const
{
	diffValue = 0;
	std::map<std::string, std::string> explanationMetaDataStringTemp;
	explanationMetaDataStringTemp.clear();

	auto percentToString = [](double value) {
		std::ostringstream oss;
		oss << std::fixed << std::setprecision(2) << value * 100 << '%';
		return oss.str();
	};

	if ((_expected.Size().width == _actual.Size().width) && (_expected.Size().height == _actual.Size().height))
	{
		explanationMetaDataStringTemp["Image Size"] = "Both (" + std::to_string(_expected.Size().width) + ", " + std::to_string(_expected.Size().height) + ")";
	}
	else
	{
		explanationMetaDataStringTemp["Image Size"] = "Expected: (" + std::to_string(_expected.Size().width) + ", " + std::to_string(_expected.Size().height) + ") Actual: (" + std::to_string(_actual.Size().width) + ", " + std::to_string(_actual.Size().height) + ")";
	}

	int executedTestCount = 0;
	if (algorithm == Algorithm::AllOfThem)
	{
		bool imageSizesMatch = (_expected.Size().width == _actual.Size().width) && (_expected.Size().height == _actual.Size().height);
		bool earlyTestPass = false;
		if (imageSizesMatch)
		{
			double testValue = DoBinaryCountExact(_expected, _actual, explanationMetaDataStringTemp);
			if (testValue > 0.999)
			{
				if (testValue == 1.0)
				{
					explanationMetaDataStringTemp["Algorithm"] = "All - but images are binary identical. Early pass.";
				}
				else
				{
					explanationMetaDataStringTemp["Algorithm"] = "All - but " + percentToString(testValue) + " of pixels are binary identical. Early pass.";
				}

				// We've got a good enough binary match. Use that.
				earlyTestPass = true;
			}
			else
			{
				explanationMetaDataStringTemp["BinaryCount"] = percentToString(testValue) + " binary match";
			}
			diffValue += testValue;
			++executedTestCount;
		}

		// Size independent calculations.
		if (!earlyTestPass)
		{
			auto testValue = DoRegionRanges(_expected, _actual, explanationMetaDataStringTemp);
			explanationMetaDataStringTemp["RegionRanges"] = percentToString(testValue) + " regions match";
			diffValue += testValue;
			++executedTestCount;
		}

		if (!earlyTestPass && imageSizesMatch)
		{
			auto testValueSearch = DoValueSearch(_expected, _actual, explanationMetaDataStringTemp, 4);
			explanationMetaDataStringTemp["ValueSearch"] = percentToString(testValueSearch) + " pixels found nearby";
			diffValue += testValueSearch;
			++executedTestCount;

			auto testValueNativeDiff = DoNaiveDiff(_expected, _actual, explanationMetaDataStringTemp);
			explanationMetaDataStringTemp["NaiveDiff"] = percentToString(testValueNativeDiff) + " pixels naively similar";
			diffValue += testValueNativeDiff;
			++executedTestCount;
		}
	}
	else
	{
		if ((algorithm & Algorithm::RegionRanges) == Algorithm::RegionRanges)
		{
			double testValue = DoRegionRanges(_expected, _actual, explanationMetaDataStringTemp);
			explanationMetaDataStringTemp["Algorithm"] += std::string(!explanationMetaDataStringTemp["Algorithm"].empty() ? ", " : "") + "RegionRanges";
			explanationMetaDataStringTemp["RegionRanges"] = percentToString(testValue) + " regions match";
			diffValue += testValue;
			++executedTestCount;
		}
		if ((algorithm & Algorithm::ValueSearch) == Algorithm::ValueSearch)
		{
			double testValue = DoValueSearch(_expected, _actual, explanationMetaDataStringTemp, 32);
			explanationMetaDataStringTemp["Algorithm"] += std::string(!explanationMetaDataStringTemp["Algorithm"].empty() ? ", " : "") + "ValueSearch";
			explanationMetaDataStringTemp["ValueSearch"] = percentToString(testValue) + " pixels found nearby";
			diffValue += testValue;
			++executedTestCount;
		}
		if ((algorithm & Algorithm::NaiveDiff) == Algorithm::NaiveDiff)
		{
			double testValue = DoNaiveDiff(_expected, _actual, explanationMetaDataStringTemp);
			explanationMetaDataStringTemp["Algorithm"] += std::string(!explanationMetaDataStringTemp["Algorithm"].empty() ? ", " : "") + "NaiveDiff";
			explanationMetaDataStringTemp["NaiveDiff"] = percentToString(testValue) + " pixels naively similar";
			diffValue += testValue;
			++executedTestCount;
		}
		if ((algorithm & Algorithm::BinaryCountExact) == Algorithm::BinaryCountExact)
		{
			double testValue = DoBinaryCountExact(_expected, _actual, explanationMetaDataStringTemp);
			explanationMetaDataStringTemp["Algorithm"] += std::string(!explanationMetaDataStringTemp["Algorithm"].empty() ? ", " : "") + "BinaryCountExact";
			explanationMetaDataStringTemp["BinaryCount"] = percentToString(testValue) + " binary match";
			diffValue += testValue;
			++executedTestCount;
		}
	}

	explanationMetaDataString.Set(std::move(explanationMetaDataStringTemp));

	// Average out over the number of test runs
	diffValue /= (double)executedTestCount;

	bool isPass = diffValue > passThreshold;
	return isPass;
}

//----------------------------------------------------------------------------------------
double ImageDiff::DoBinaryCountExact(const IImageRGBA& expected, const IImageRGBA& actual, std::map<std::string, std::string>& explanationMetaDataString)
{
	auto dim = expected.Size();
	auto actualSize = actual.Size();
	if ((dim.width != actualSize.width) || (dim.height != actualSize.height))
	{
		return 0;
	}

	if (std::memcmp(expected.Data(), actual.Data(), expected.PixelCount() * sizeof(IImageRGBA::PixelEntry)) == 0)
	{
		return 1.0;
	}

	auto pixelEquals = [&](IImageRGBA::PixelEntry a, IImageRGBA::PixelEntry b) {
		return (a.r == b.r) && (a.g == b.g) && (a.b == b.b) && (a.a == b.a);
	};

	uint32_t hits = 0;
	uint32_t total = 0;

	for (uint32_t imagePosY = 0; imagePosY < dim.height; ++imagePosY)
	{
		for (uint32_t imagePosX = 0; imagePosX < dim.width; ++imagePosX)
		{
			if (pixelEquals(expected.ReadPixel(imagePosX, imagePosY), actual.ReadPixel(imagePosX, imagePosY)))
			{
				hits++;
			}
			total++;
		}
	}

	return (double)hits / (double)total;
}

//----------------------------------------------------------------------------------------
double ImageDiff::DoNaiveDiff(const IImageRGBA& expected, const IImageRGBA& actual, std::map<std::string, std::string>& explanationMetaDataString)
{
	auto dim = expected.Size();
	auto actualSize = actual.Size();
	if ((dim.width != actualSize.width) || (dim.height != actualSize.height))
	{
		return 0;
	}

	auto score = 0.0f;

	for (uint32_t imagePosY = 0; imagePosY < dim.height; ++imagePosY)
	{
		for (uint32_t imagePosX = 0; imagePosX < dim.width; ++imagePosX)
		{
			auto exp = expected.ReadPixel(imagePosX, imagePosY);
			auto act = actual.ReadPixel(imagePosX, imagePosY);

			float diff = (std::fabs((float)exp.r - (float)act.r) + std::fabs((float)exp.g - (float)act.g) + std::fabs((float)exp.b - (float)act.b) + std::fabs((float)exp.a - (float)act.a)) * (1.0f / (255.0f * 4.0f));

#ifdef __GNUC__
			// Workaround for GCC bug in libstdc++. See https://stackoverflow.com/questions/5483930/powf-is-not-a-member-of-std
			score += ::powf(1.0f - diff, 3);
#else
			score += std::powf(1.0f - diff, 3);
#endif
		}
	}

	return score / (float)(dim.width * dim.height);
}

//----------------------------------------------------------------------------------------
double ImageDiff::DoValueSearch(const IImageRGBA& expected, const IImageRGBA& actual, std::map<std::string, std::string>& explanationMetaDataString, uint16_t maxRange)
{
	//:TODO: This isn't needed if we use fractional coordinates.
	auto dim = expected.Size();
	auto actualSize = actual.Size();
	if ((dim.width != actualSize.width) || (dim.height != actualSize.height))
	{
		return 0;
	}

	auto score = 0.0f;

	auto pixelEquals = [&](IImageRGBA::PixelEntry a, IImageRGBA::PixelEntry b) {
		return (a.r == b.r) && (a.g == b.g) && (a.b == b.b) && (a.a == b.a);
	};
	auto percentToString = [](double value) {
		std::ostringstream oss;
		oss << std::fixed << std::setprecision(2) << value * 100 << '%';
		return oss.str();
	};

	// We filter out the background (if we can detect what it is) as it weights
	// the results the wrong way. E.g. if Expected is a single blue pixel, and
	// Actual is a single red pixel, and there's 640 x 480 - 1 = 307199 other
	// identical black pixels, don't score the image as 99.99967% similar.

	std::vector<IImageRGBA::PixelEntry> backgroundFilter;
	backgroundFilter.push_back(actual.ReadPixel(0, 0));
	backgroundFilter.push_back(actual.ReadPixel(0, dim.height - 1));
	backgroundFilter.push_back(actual.ReadPixel(dim.width - 1, dim.height - 1));
	backgroundFilter.push_back(actual.ReadPixel(dim.width - 1, 0));
	backgroundFilter.push_back(expected.ReadPixel(0, 0));
	backgroundFilter.push_back(expected.ReadPixel(0, dim.height - 1));
	backgroundFilter.push_back(expected.ReadPixel(dim.width - 1, dim.height - 1));
	backgroundFilter.push_back(expected.ReadPixel(dim.width - 1, 0));

	if (std::count_if(backgroundFilter.begin(), backgroundFilter.end(), [&](auto i) { return pixelEquals(i, backgroundFilter.front()); }) != 8)
	{
		backgroundFilter.clear();
	}

	auto colorDiffScore = [&](IImageRGBA::PixelEntry a, IImageRGBA::PixelEntry b) {
		float d = (std::fabs((float)a.r - (float)b.r) + std::fabs((float)a.g - (float)b.g) + std::fabs((float)a.b - (float)b.b) + std::fabs((float)a.a - (float)b.a)) * (1.0f / (255.0f * 4.0f));
		return d;
	};

	auto loopCount = 0u;

	for (uint32_t imagePosY = 0; imagePosY < dim.height; ++imagePosY)
	{
		for (uint32_t imagePosX = 0; imagePosX < dim.width; ++imagePosX)
		{
			auto e = expected.ReadPixel(imagePosX, imagePosY);
			auto a = actual.ReadPixel(imagePosX, imagePosY);

			if (!backgroundFilter.empty() && (pixelEquals(e, backgroundFilter.front()) || pixelEquals(a, backgroundFilter.front())))
			{
				// Special handling if the background colour is involved. Don't count
				// matches as wins, but count mismatches as failures.
				if (!pixelEquals(e, a))
				{
					loopCount++;
				}
				continue;
			}

			loopCount++;

			if (pixelEquals(e, a))
			{
				score += 1.0f;
				continue;
			}

			auto bestMatchScore = 0.0f;

			uint32_t comparePosMinY = (uint32_t)std::max((int)imagePosY - (int)maxRange, 0);
			uint32_t comparePosMaxY = std::min(imagePosY + (uint32_t)maxRange + 1, dim.height);
			uint32_t comparePosMinX = (uint32_t)std::max((int)imagePosX - (int)maxRange, 0);
			uint32_t comparePosMaxX = std::min(imagePosX + (uint32_t)maxRange + 1, dim.width);
			for (uint32_t comparePosY = comparePosMinY; comparePosY < comparePosMaxY; ++comparePosY)
			{
				for (uint32_t comparePosX = comparePosMinX; comparePosX < comparePosMaxX; ++comparePosX)
				{
					auto pixelColor = expected.ReadPixel(comparePosX, comparePosY);
					if (pixelEquals(a, pixelColor))
					{
						bestMatchScore = 1.0;
					}

					auto diffScore = colorDiffScore(a, pixelColor);
					if (diffScore > bestMatchScore)
					{
						bestMatchScore = diffScore;
					}
				}
			}

			if (bestMatchScore == 1.0)
			{
				// We found the pixel, just it moved.
				score += 0.5f;
			}
			else if (bestMatchScore < 0.5)
			{
				// We found no match.
				if ((loopCount > std::max(dim.width, dim.height)) && ((score / (float)loopCount) < 0.5f))
				{
					explanationMetaDataString["ValueSearchEarlyExit"] = "ValueSearch early exited after " + std::to_string(loopCount) + " failures. Current score: " + percentToString(score / (float)loopCount);
					return score / (float)loopCount;
				}
			}
			else
			{
				// We have a partial match, a similar colour was found some distance away.
				score += 0.2f;
			}
		}
	}

	if (loopCount == 0)
	{
		// If the image is a solid colour, and that matches, it's a perfect match.
		return 1.0;
	}

	return score / (float)loopCount;
}

//----------------------------------------------------------------------------------------
double ImageDiff::DoRegionRanges(const IImageRGBA& expected, const IImageRGBA& actual, std::map<std::string, std::string>& explanationMetaDataString)
{
	auto score = 0.0f;

	for (auto i = 0; i < 500; ++i)
	{
		IImageRGBA::PixelEntry minEPixel{255, 255, 255, 255};
		IImageRGBA::PixelEntry maxEPixel{0, 0, 0, 0};
		float avgEPixel[4]{0, 0, 0, 0};

		float lowerRangeFactorX = std::fmod((float)i * 17.9f, 0.9f);
		float lowerRangeFactorY = std::fmod((float)i * 71.3f, 0.9f);
		float upperRangeFactorX = lowerRangeFactorX + 0.1f;
		float upperRangeFactorY = lowerRangeFactorY + 0.1f;

		auto expectedImageSize = expected.Size();
		auto comparePosMinY = (uint32_t)((float)expectedImageSize.height * lowerRangeFactorY);
		auto comparePosMaxY = (uint32_t)((float)expectedImageSize.height * upperRangeFactorY);
		auto comparePosMinX = (uint32_t)((float)expectedImageSize.width * lowerRangeFactorX);
		auto comparePosMaxX = (uint32_t)((float)expectedImageSize.width * upperRangeFactorX);
		for (uint32_t comparePosY = comparePosMinY; comparePosY < comparePosMaxY; ++comparePosY)
		{
			for (uint32_t comparePosX = comparePosMinX; comparePosX < comparePosMaxX; ++comparePosX)
			{
				auto ePixelValue = expected.ReadPixel(comparePosX, comparePosY);
				minEPixel.r = std::min(minEPixel.r, ePixelValue.r);
				minEPixel.g = std::min(minEPixel.g, ePixelValue.g);
				minEPixel.b = std::min(minEPixel.b, ePixelValue.b);
				minEPixel.a = std::min(minEPixel.a, ePixelValue.a);

				maxEPixel.r = std::max(maxEPixel.r, ePixelValue.r);
				maxEPixel.g = std::max(maxEPixel.g, ePixelValue.g);
				maxEPixel.b = std::max(maxEPixel.b, ePixelValue.b);
				maxEPixel.a = std::max(maxEPixel.a, ePixelValue.a);

				avgEPixel[0] += (float)ePixelValue.r;
				avgEPixel[1] += (float)ePixelValue.g;
				avgEPixel[2] += (float)ePixelValue.b;
				avgEPixel[3] += (float)ePixelValue.a;
			}
		}

		auto compareRangePixelCount = (float)((comparePosMaxX - comparePosMinX) * (comparePosMaxY - comparePosMinY));
		avgEPixel[0] /= compareRangePixelCount;
		avgEPixel[1] /= compareRangePixelCount;
		avgEPixel[2] /= compareRangePixelCount;
		avgEPixel[3] /= compareRangePixelCount;

		IImageRGBA::PixelEntry minAPixel{255, 255, 255, 255};
		IImageRGBA::PixelEntry maxAPixel{0, 0, 0, 0};
		float avgAPixel[4]{0, 0, 0, 0};

		auto actualImageSize = actual.Size();
		comparePosMinY = (uint32_t)((float)actualImageSize.height * lowerRangeFactorY);
		comparePosMaxY = (uint32_t)((float)actualImageSize.height * upperRangeFactorY);
		comparePosMinX = (uint32_t)((float)actualImageSize.width * lowerRangeFactorX);
		comparePosMaxX = (uint32_t)((float)actualImageSize.width * upperRangeFactorX);
		for (uint32_t comparePosY = comparePosMinY; comparePosY < comparePosMaxY; ++comparePosY)
		{
			for (uint32_t comparePosX = comparePosMinX; comparePosX < comparePosMaxX; ++comparePosX)
			{
				auto aPixelValue = actual.ReadPixel(comparePosX, comparePosY);

				minAPixel.r = std::min(minAPixel.r, aPixelValue.r);
				minAPixel.g = std::min(minAPixel.g, aPixelValue.g);
				minAPixel.b = std::min(minAPixel.b, aPixelValue.b);
				minAPixel.a = std::min(minAPixel.a, aPixelValue.a);

				maxAPixel.r = std::max(maxAPixel.r, aPixelValue.r);
				maxAPixel.g = std::max(maxAPixel.g, aPixelValue.g);
				maxAPixel.b = std::max(maxAPixel.b, aPixelValue.b);
				maxAPixel.a = std::max(maxAPixel.a, aPixelValue.a);

				avgAPixel[0] += (float)aPixelValue.r;
				avgAPixel[1] += (float)aPixelValue.g;
				avgAPixel[2] += (float)aPixelValue.b;
				avgAPixel[3] += (float)aPixelValue.a;
			}
		}

		avgAPixel[0] /= compareRangePixelCount;
		avgAPixel[1] /= compareRangePixelCount;
		avgAPixel[2] /= compareRangePixelCount;
		avgAPixel[3] /= compareRangePixelCount;

		float maxDiff = (std::fabs((float)maxAPixel.r - (float)maxEPixel.r) + std::fabs((float)maxAPixel.g - (float)maxEPixel.g) + std::fabs((float)maxAPixel.b - (float)maxEPixel.b) + std::fabs((float)maxAPixel.a - (float)maxEPixel.a)) * (1.0f / (255.0f * 4.0f));
		float minDiff = (std::fabs((float)minAPixel.r - (float)minEPixel.r) + std::fabs((float)minAPixel.g - (float)minEPixel.g) + std::fabs((float)minAPixel.b - (float)minEPixel.b) + std::fabs((float)minAPixel.a - (float)minEPixel.a)) * (1.0f / (255.0f * 4.0f));
		float avgDiff = (std::fabs(avgAPixel[0] - avgEPixel[0]) + std::fabs(avgAPixel[1] - avgEPixel[1]) + std::fabs(avgAPixel[2] - avgEPixel[2]) + std::fabs(avgAPixel[3] - avgEPixel[3])) * (1.0f / (255.0f * 4.0f));

		if (maxDiff == 0 && minDiff == 0 && avgDiff == 0)
		{
			score += 1.0f;
		}
		else
		{
			auto delta = 1.0f - std::max(maxDiff, std::max(minDiff, avgDiff));
			// Since we're only checking statistics here, a difference in those values
			// must translate to a pretty big difference to a human eye. So penalise
			// the diff by making anything below 100% decay towards 0 by the 10th
			// power.
#ifdef __GNUC__
			// Workaround for GCC bug in libstdc++. See https://stackoverflow.com/questions/5483930/powf-is-not-a-member-of-std
			delta = ::powf(delta, 10);
#else
			delta = std::powf(delta, 10);
#endif
			score += delta;
		}
	}

	score /= 500;

	return score;
}

} // namespace cobalt::graphics
