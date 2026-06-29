// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include <Cobalt/Debug/Debug.pkg>
#include <type_traits>
namespace cobalt { namespace graphics {

//----------------------------------------------------------------------------------------
// Size methods
//----------------------------------------------------------------------------------------
constexpr size_t ITextureBuffer::ElementCountPerPixelFromFormat(SourceImageFormat imageFormat)
{
	switch (imageFormat)
	{
	case SourceImageFormat::R:
	case SourceImageFormat::X:
		return 1;
	case SourceImageFormat::RG:
	case SourceImageFormat::XY:
		return 2;
	case SourceImageFormat::RGB:
	case SourceImageFormat::BGR:
	case SourceImageFormat::XYZ:
		return 3;
	case SourceImageFormat::RGBA:
	case SourceImageFormat::BGRA:
	case SourceImageFormat::XYZW:
		return 4;
	}
	UNREACHABLE();
	return 0;
}

//----------------------------------------------------------------------------------------
constexpr size_t ITextureBuffer::ElementCountPerPixelFromFormat(ImageFormat imageFormat)
{
	switch (imageFormat)
	{
	case ImageFormat::Depth:
	case ImageFormat::DepthAndStencil:
	case ImageFormat::R:
	case ImageFormat::X:
		return 1;
	case ImageFormat::RG:
	case ImageFormat::XY:
		return 2;
	case ImageFormat::RGB:
	case ImageFormat::BGR:
	case ImageFormat::XYZ:
		return 3;
	case ImageFormat::RGBA:
	case ImageFormat::BGRA:
	case ImageFormat::XYZW:
		return 4;
	}
	UNREACHABLE();
	return 0;
}

//----------------------------------------------------------------------------------------
constexpr size_t ITextureBuffer::ByteSizePerElementFromFormat(SourceDataFormat dataFormat)
{
	switch (dataFormat)
	{
	case SourceDataFormat::Int8:
	case SourceDataFormat::UInt8:
	case SourceDataFormat::Norm8:
	case SourceDataFormat::UNorm8:
		return 1;
	case SourceDataFormat::Int16:
	case SourceDataFormat::UInt16:
	case SourceDataFormat::Norm16:
	case SourceDataFormat::UNorm16:
	case SourceDataFormat::Float16:
		return 2;
	case SourceDataFormat::Int32:
	case SourceDataFormat::UInt32:
	case SourceDataFormat::Norm32:
	case SourceDataFormat::UNorm32:
	case SourceDataFormat::Float32:
		return 4;
	}
	// Note that we deliberately don't cover compressed textures here, as asking for the element size in bytes is
	// invalid for these formats. We return 0 here in the case of an unsupported data format being specified.
	return 0;
}

//----------------------------------------------------------------------------------------
constexpr size_t ITextureBuffer::ByteSizePerElementFromFormat(DataFormat dataFormat)
{
	switch (dataFormat)
	{
	case DataFormat::Int8:
	case DataFormat::UInt8:
	case DataFormat::Norm8:
	case DataFormat::UNorm8:
		return 1;
	case DataFormat::Int16:
	case DataFormat::UInt16:
	case DataFormat::Norm16:
	case DataFormat::UNorm16:
	case DataFormat::Float16:
		return 2;
	case DataFormat::Int32:
	case DataFormat::UInt32:
	case DataFormat::Float32:
		return 4;
	case DataFormat::DepthUNorm16:
		return 2;
	case DataFormat::DepthUNorm24:
	case DataFormat::DepthUNorm24StencilUInt8:
	case DataFormat::DepthFloat32:
		return 4;
	case DataFormat::DepthFloat32StencilUInt8:
		return 8;
	}
	// Note that we deliberately don't cover compressed textures here, as asking for the element size in bytes is
	// invalid for these formats. We return 0 here in the case of an unsupported data format being specified.
	return 0;
}

//----------------------------------------------------------------------------------------
constexpr bool ITextureBuffer::IsCompressedTextureFormat(SourceDataFormat dataFormat)
{
	switch (dataFormat)
	{
	case SourceDataFormat::DXT1:
	case SourceDataFormat::DXT3:
	case SourceDataFormat::DXT5:
	case SourceDataFormat::BPTC:
	case SourceDataFormat::ETC2:
	case SourceDataFormat::ASTC4x4:
	case SourceDataFormat::ASTC5x5:
	case SourceDataFormat::ASTC6x6:
	case SourceDataFormat::ASTC8x8:
		return true;
	}
	return false;
}

//----------------------------------------------------------------------------------------
constexpr bool ITextureBuffer::IsCompressedTextureFormat(DataFormat dataFormat)
{
	switch (dataFormat)
	{
	case DataFormat::DXT1:
	case DataFormat::DXT3:
	case DataFormat::DXT5:
	case DataFormat::BPTC:
	case DataFormat::ETC2:
	case DataFormat::ASTC4x4:
	case DataFormat::ASTC5x5:
	case DataFormat::ASTC6x6:
	case DataFormat::ASTC8x8:
		return true;
	}
	return false;
}

//----------------------------------------------------------------------------------------
V2UInt32 ITextureBuffer::CellDimensionsInPixelsFromFormat(SourceDataFormat dataFormat)
{
	switch (dataFormat)
	{
	case SourceDataFormat::DXT1:
	case SourceDataFormat::DXT3:
	case SourceDataFormat::DXT5:
	case SourceDataFormat::BPTC:
	case SourceDataFormat::ETC2:
	case SourceDataFormat::ASTC4x4:
		return V2UInt32(4, 4);
	case SourceDataFormat::ASTC5x5:
		return V2UInt32(5, 5);
	case SourceDataFormat::ASTC6x6:
		return V2UInt32(6, 6);
	case SourceDataFormat::ASTC8x8:
		return V2UInt32(8, 8);
	}
	// Uncompressed texture formats have a cell size of 1x1. Compressed texture cell sizes are handled above.
	return V2UInt32(1, 1);
}

//----------------------------------------------------------------------------------------
V2UInt32 ITextureBuffer::CellDimensionsInPixelsFromFormat(DataFormat dataFormat)
{
	switch (dataFormat)
	{
	case DataFormat::DXT1:
	case DataFormat::DXT3:
	case DataFormat::DXT5:
	case DataFormat::BPTC:
	case DataFormat::ETC2:
	case DataFormat::ASTC4x4:
		return V2UInt32(4, 4);
	case DataFormat::ASTC5x5:
		return V2UInt32(5, 5);
	case DataFormat::ASTC6x6:
		return V2UInt32(6, 6);
	case DataFormat::ASTC8x8:
		return V2UInt32(8, 8);
	}
	// Uncompressed texture formats have a cell size of 1x1. Compressed texture cell sizes are handled above.
	return V2UInt32(1, 1);
}

//----------------------------------------------------------------------------------------
constexpr size_t ITextureBuffer::CellSizeInBytesFromFormat(SourceImageFormat imageFormat, SourceDataFormat dataFormat)
{
	switch (dataFormat)
	{
	case SourceDataFormat::DXT1:
		return 8;
	case SourceDataFormat::DXT3:
	case SourceDataFormat::DXT5:
	case SourceDataFormat::BPTC:
	case SourceDataFormat::ETC2:
	case SourceDataFormat::ASTC4x4:
	case SourceDataFormat::ASTC5x5:
	case SourceDataFormat::ASTC6x6:
	case SourceDataFormat::ASTC8x8:
		return 16;
	}
	// Return the pixel size in bytes for uncompressed texture formats
	return ElementCountPerPixelFromFormat(imageFormat) * ByteSizePerElementFromFormat(dataFormat);
}

//----------------------------------------------------------------------------------------
constexpr size_t ITextureBuffer::CellSizeInBytesFromFormat(ImageFormat imageFormat, DataFormat dataFormat)
{
	switch (dataFormat)
	{
	case DataFormat::DXT1:
		return 8;
	case DataFormat::DXT3:
	case DataFormat::DXT5:
	case DataFormat::BPTC:
	case DataFormat::ETC2:
	case DataFormat::ASTC4x4:
	case DataFormat::ASTC5x5:
	case DataFormat::ASTC6x6:
	case DataFormat::ASTC8x8:
		return 16;
	}
	// Return the pixel size in bytes for uncompressed texture formats
	return ElementCountPerPixelFromFormat(imageFormat) * ByteSizePerElementFromFormat(dataFormat);
}

//----------------------------------------------------------------------------------------
// Format methods
//----------------------------------------------------------------------------------------
constexpr bool ITextureBuffer::ImageFormatsAreBinaryEquivalent(SourceImageFormat sourceImageFormat, ImageFormat imageFormat)
{
	switch (sourceImageFormat)
	{
	case SourceImageFormat::R:
	case SourceImageFormat::X:
		return (imageFormat == ImageFormat::R) || (imageFormat == ImageFormat::X) || (imageFormat == ImageFormat::Depth);
	case SourceImageFormat::RG:
	case SourceImageFormat::XY:
		return (imageFormat == ImageFormat::RG) || (imageFormat == ImageFormat::XY);
	case SourceImageFormat::RGB:
	case SourceImageFormat::XYZ:
		return (imageFormat == ImageFormat::RGB) || (imageFormat == ImageFormat::XYZ);
	case SourceImageFormat::RGBA:
	case SourceImageFormat::XYZW:
		return (imageFormat == ImageFormat::RGBA) || (imageFormat == ImageFormat::XYZW);
	case SourceImageFormat::BGR:
		return imageFormat == ImageFormat::BGR;
	case SourceImageFormat::BGRA:
		return imageFormat == ImageFormat::BGRA;
	}
	return false;
}

//----------------------------------------------------------------------------------------
constexpr bool ITextureBuffer::DataFormatsAreBinaryEquivalent(SourceDataFormat sourceDataFormat, DataFormat dataFormat)
{
	switch (sourceDataFormat)
	{
	case SourceDataFormat::Int8:
		return dataFormat == DataFormat::Int8;
	case SourceDataFormat::Int16:
		return dataFormat == DataFormat::Int16;
	case SourceDataFormat::Int32:
		return dataFormat == DataFormat::Int32;
	case SourceDataFormat::UNorm8:
	case SourceDataFormat::UInt8:
		return (dataFormat == DataFormat::UInt8) || (dataFormat == DataFormat::UNorm8);
	case SourceDataFormat::UNorm16:
	case SourceDataFormat::UInt16:
		return (dataFormat == DataFormat::UInt16) || (dataFormat == DataFormat::UNorm16) || (dataFormat == DataFormat::DepthUNorm16);
	case SourceDataFormat::UInt32:
		return dataFormat == DataFormat::UInt32;
	case SourceDataFormat::Norm8:
		return dataFormat == DataFormat::Norm8;
	case SourceDataFormat::Norm16:
		return dataFormat == DataFormat::Norm16;
	case SourceDataFormat::Float16:
		return dataFormat == DataFormat::Float16;
	case SourceDataFormat::Float32:
		return (dataFormat == DataFormat::Float32) || (dataFormat == DataFormat::DepthFloat32);
	case SourceDataFormat::DXT1:
		return dataFormat == DataFormat::DXT1;
	case SourceDataFormat::DXT3:
		return dataFormat == DataFormat::DXT3;
	case SourceDataFormat::DXT5:
		return dataFormat == DataFormat::DXT5;
	case SourceDataFormat::BPTC:
		return dataFormat == DataFormat::BPTC;
	case SourceDataFormat::ETC2:
		return dataFormat == DataFormat::ETC2;
	case SourceDataFormat::ASTC4x4:
		return dataFormat == DataFormat::ASTC4x4;
	case SourceDataFormat::ASTC5x5:
		return dataFormat == DataFormat::ASTC5x5;
	case SourceDataFormat::ASTC6x6:
		return dataFormat == DataFormat::ASTC6x6;
	case SourceDataFormat::ASTC8x8:
		return dataFormat == DataFormat::ASTC8x8;
	}
	return false;
}

//----------------------------------------------------------------------------------------
// Enumeration operators
//----------------------------------------------------------------------------------------
inline ITextureBuffer::UsageFlags operator|(ITextureBuffer::UsageFlags left, ITextureBuffer::UsageFlags right)
{
	return (ITextureBuffer::UsageFlags)((std::underlying_type<ITextureBuffer::UsageFlags>::type)left | (std::underlying_type<ITextureBuffer::UsageFlags>::type)right);
}

//----------------------------------------------------------------------------------------
inline ITextureBuffer::UsageFlags& operator|=(ITextureBuffer::UsageFlags& left, ITextureBuffer::UsageFlags right)
{
	left = (left | right);
	return left;
}

//----------------------------------------------------------------------------------------
inline ITextureBuffer::UsageFlags operator&(ITextureBuffer::UsageFlags left, ITextureBuffer::UsageFlags right)
{
	return (ITextureBuffer::UsageFlags)((std::underlying_type<ITextureBuffer::UsageFlags>::type)left & (std::underlying_type<ITextureBuffer::UsageFlags>::type)right);
}

//----------------------------------------------------------------------------------------
inline ITextureBuffer::UsageFlags& operator&=(ITextureBuffer::UsageFlags& left, ITextureBuffer::UsageFlags right)
{
	left = (left & right);
	return left;
}

//----------------------------------------------------------------------------------------
inline ITextureBuffer::PerformanceHint operator|(ITextureBuffer::PerformanceHint left, ITextureBuffer::PerformanceHint right)
{
	return (ITextureBuffer::PerformanceHint)((std::underlying_type<ITextureBuffer::PerformanceHint>::type)left | (std::underlying_type<ITextureBuffer::PerformanceHint>::type)right);
}

//----------------------------------------------------------------------------------------
inline ITextureBuffer::PerformanceHint& operator|=(ITextureBuffer::PerformanceHint& left, ITextureBuffer::PerformanceHint right)
{
	left = (left | right);
	return left;
}

//----------------------------------------------------------------------------------------
inline ITextureBuffer::PerformanceHint operator&(ITextureBuffer::PerformanceHint left, ITextureBuffer::PerformanceHint right)
{
	return (ITextureBuffer::PerformanceHint)((std::underlying_type<ITextureBuffer::PerformanceHint>::type)left & (std::underlying_type<ITextureBuffer::PerformanceHint>::type)right);
}

//----------------------------------------------------------------------------------------
inline ITextureBuffer::PerformanceHint& operator&=(ITextureBuffer::PerformanceHint& left, ITextureBuffer::PerformanceHint right)
{
	left = (left & right);
	return left;
}

//----------------------------------------------------------------------------------------
inline ITextureBuffer::DataPersistenceFlags operator|(ITextureBuffer::DataPersistenceFlags left, ITextureBuffer::DataPersistenceFlags right)
{
	return (ITextureBuffer::DataPersistenceFlags)((std::underlying_type<ITextureBuffer::DataPersistenceFlags>::type)left | (std::underlying_type<ITextureBuffer::DataPersistenceFlags>::type)right);
}

//----------------------------------------------------------------------------------------
inline ITextureBuffer::DataPersistenceFlags& operator|=(ITextureBuffer::DataPersistenceFlags& left, ITextureBuffer::DataPersistenceFlags right)
{
	left = (left | right);
	return left;
}

//----------------------------------------------------------------------------------------
inline ITextureBuffer::DataPersistenceFlags operator&(ITextureBuffer::DataPersistenceFlags left, ITextureBuffer::DataPersistenceFlags right)
{
	return (ITextureBuffer::DataPersistenceFlags)((std::underlying_type<ITextureBuffer::DataPersistenceFlags>::type)left & (std::underlying_type<ITextureBuffer::DataPersistenceFlags>::type)right);
}

//----------------------------------------------------------------------------------------
inline ITextureBuffer::DataPersistenceFlags& operator&=(ITextureBuffer::DataPersistenceFlags& left, ITextureBuffer::DataPersistenceFlags right)
{
	left = (left & right);
	return left;
}

}} // namespace cobalt::graphics
