// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "ExportMacro.h"
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
namespace internal {
extern "C" IMAGEDIFF_EXPORT IImageRGBA* CreateIImageRGBA();
} // namespace internal

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
inline IImageRGBA::unique_ptr IImageRGBA::Create()
{
	return unique_ptr(internal::CreateIImageRGBA());
}

} // namespace cobalt::graphics
