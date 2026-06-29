// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// State layout building methods
//----------------------------------------------------------------------------------------
constexpr size_t VulkanStateBufferLayout::GetDataTypeByteSize(DataType dataType)
{
	switch (dataType)
	{
	case DataType::Boolean:
	case DataType::Int32:
		return sizeof(int);
	case DataType::UInt32:
		return sizeof(unsigned int);
	case DataType::Float32:
		return sizeof(float);
	case DataType::Float64:
		return sizeof(double);
	default:
		return 0;
	}
}

} // namespace cobalt::graphics
