// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include <cstdint>
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
inline void ConvertColorToFloat(uint32_t color, float* outColor)
{
	// Colors are in ARGB format
	// Float array is in RGBA format
	outColor[0] = (float)((color >> 16) & 0xFF) / 255.0f;
	outColor[1] = (float)((color >> 8) & 0xFF) / 255.0f;
	outColor[2] = (float)(color & 0xFF) / 255.0f;
	outColor[3] = (float)((color >> 24) & 0xFF) / 255.0f;
}

//----------------------------------------------------------------------------------------
inline uint32_t RenderPassMarkerColor()
{
	return 0xFF4545E6; // Blue
}

//----------------------------------------------------------------------------------------
inline uint32_t ProgramMarkerColor()
{
	return 0xFF45E65A; // Green
}

//----------------------------------------------------------------------------------------
inline uint32_t GroupMarkerColor()
{
	return 0xFFE64545; // Red
}

//----------------------------------------------------------------------------------------
inline uint32_t PipelineMarkerColor()
{
	return 0xFFE6B045; // Orange
}

//----------------------------------------------------------------------------------------
inline uint32_t RenderableMarkerColor()
{
	return 0xFFE6E6E6; // White
}

//----------------------------------------------------------------------------------------
inline uint32_t SetupMarkerColor()
{
	return 0xFF808080; // Gray
}

} // namespace cobalt::graphics
