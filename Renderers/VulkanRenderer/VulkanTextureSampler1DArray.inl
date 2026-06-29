// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
VulkanTextureSampler1DArray::VulkanTextureSampler1DArray(cobalt::logging::ILogger* log, VulkanRenderer* renderer)
: VulkanTextureSampler<ITextureSampler1DArray>(log, renderer)
{
	FlagBuildStateModified();
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void VulkanTextureSampler1DArray::Delete()
{
	Renderer()->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
// Format methods
//----------------------------------------------------------------------------------------
void VulkanTextureSampler1DArray::SetTextureWrapMode(WrapMode wrapModeHorizontal)
{
	SetTextureWrapModeInternal(wrapModeHorizontal);
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
void VulkanTextureSampler1DArray::FlagObjectModified()
{
	Renderer()->FlagObjectModified(this);
}

} // namespace cobalt::graphics
