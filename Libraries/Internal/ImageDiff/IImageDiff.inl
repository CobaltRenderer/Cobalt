// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "ExportMacro.h"
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Enumerations
//----------------------------------------------------------------------------------------
enum class IImageDiff::Algorithm
{
	// Bytes equal / Bytes total.
	//
	// This is the only one that can handle non-2D / non-colour data, such as
	// a picking test.
	BinaryCountExact = 0x01,

	// For each pixel, for each channel, record the differences and sum them,
	// then normalise. So 0.0 would be an entirely inverted image, 1.0 is
	// identical.
	//
	// Good for complex but very smooth scenes, e.g. a very blurry photo.
	NaiveDiff = 0x02,

	// For each pixel, if it's different, search the surrounding area for a
	// matching pixel. If found, record a small penalty. Otherwise look for
	// a similar pixel in the surrounding area, and if found, record a medium
	// penalty. If nothing similar is found at all, the pixel is completely
	// penalised.
	//
	// This is the only one that can handle a test with only a single pixel being
	// set.
	ValueSearch = 0x04,

	// We check that for within any region, the channel ranges remain
	// consistent (We use hundreds of overlapping regions, so that a 1 pixel
	// shift on a border doesn't sometimes massively influence results).
	//
	// Best for thin single-pixel lines, and subtle gradients.
	//
	// Also works when the images are not of equal size.
	RegionRanges = 0x08,

	// Find corners of high contrast using the F.A.S.T. Feature finder.
	//
	// Best for general triangulations, non-linear / non-circular CAD, large
	// text, and any shape sharply contrasting against a background.
	//
	// Also works when the images are not of equal size.
	//BlobCorners,

	// Find edges of high contrast.
	//
	// Best for sharp photos, small text, and CAD filled arcs and circles in
	// computer generated imagery.
	//
	// Also works when the images are not of equal size.
	//BlobEdges,

	// Do like 90% of the JPEG algorithm, but keep reducing the per-block
	// quantisation matrix element by element, and repeat for all blocks,
	// until the images are equal in frequency domain. The more
	// block-quantisation steps needed, the more different the images are.
	//
	// Good for photos, triangulations and gradients. Not good for linear CAD.
	//InverseDiscreteCosineTransform,

	// Try to run all methods, and average the results.
	//
	// This should be good enough for the majority of common, simple cases.
	//
	// This will early exit if the images are binary identical, or contain
	// more than 99.99% identical pixels. This may not be desired if testing
	// single pixel / thin line data.
	AllOfThem = 0x0F,
};

//----------------------------------------------------------------------------------------
namespace internal {
extern "C" IMAGEDIFF_EXPORT IImageDiff* CreateIImageDiff(const IImageRGBA& expected, const IImageRGBA& actual);
} // namespace internal

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
inline IImageDiff::unique_ptr IImageDiff::Create(const IImageRGBA& expected, const IImageRGBA& actual)
{
	return unique_ptr(internal::CreateIImageDiff(expected, actual));
}

//----------------------------------------------------------------------------------------
// Enumeration operators
//----------------------------------------------------------------------------------------
inline IImageDiff::Algorithm operator|(IImageDiff::Algorithm left, IImageDiff::Algorithm right)
{
	return (IImageDiff::Algorithm)((std::underlying_type<IImageDiff::Algorithm>::type)left | (std::underlying_type<IImageDiff::Algorithm>::type)right);
}

//----------------------------------------------------------------------------------------
inline IImageDiff::Algorithm& operator|=(IImageDiff::Algorithm& left, IImageDiff::Algorithm right)
{
	left = (left | right);
	return left;
}

//----------------------------------------------------------------------------------------
inline IImageDiff::Algorithm operator&(IImageDiff::Algorithm left, IImageDiff::Algorithm right)
{
	return (IImageDiff::Algorithm)((std::underlying_type<IImageDiff::Algorithm>::type)left & (std::underlying_type<IImageDiff::Algorithm>::type)right);
}

//----------------------------------------------------------------------------------------
inline IImageDiff::Algorithm& operator&=(IImageDiff::Algorithm& left, IImageDiff::Algorithm right)
{
	left = (left & right);
	return left;
}

} // namespace cobalt::graphics
