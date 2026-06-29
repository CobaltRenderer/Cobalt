// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
VulkanTextureSampler1D::VulkanTextureSampler1D(cobalt::logging::ILogger* log, VulkanRenderer* renderer)
: VulkanTextureSampler<ITextureSampler1D>(log, renderer)
{
	FlagBuildStateModified();
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void VulkanTextureSampler1D::Delete()
{
	Renderer()->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
// Format methods
//----------------------------------------------------------------------------------------
void VulkanTextureSampler1D::SetTextureWrapMode(WrapMode wrapModeHorizontal)
{
	SetTextureWrapModeInternal(wrapModeHorizontal);
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
void VulkanTextureSampler1D::FlagObjectModified()
{
	Renderer()->FlagObjectModified(this);
}

} // namespace cobalt::graphics
