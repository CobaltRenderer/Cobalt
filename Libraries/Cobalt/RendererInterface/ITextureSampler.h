// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
namespace cobalt { namespace graphics {

class ITextureSampler
{
public:
	// Enumerations
	enum class WrapMode
	{
		ClampToEdge,
		Repeat,
		RepeatMirrored,
	};
	enum class FilterMode
	{
		Nearest,
		Linear,
	};
	enum class MipmapMode
	{
		None,
		Nearest,
		Linear,
	};

protected:
	// Constructors
	~ITextureSampler() = default;
};

}} // namespace cobalt::graphics
