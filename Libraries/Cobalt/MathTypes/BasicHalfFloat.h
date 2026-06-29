// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
namespace cobalt { namespace graphics {

// This basic 2-byte structure is a stand-in for a 16-bit half float, as defined by IEEE 754, suitable for use in
// computer graphics. This type has no native implementation in C++, so it is left to the application to generate and
// provide this data in an appropriate form.
struct BasicHalfFloat
{
	unsigned char data[2];
};

}} // namespace cobalt::graphics
