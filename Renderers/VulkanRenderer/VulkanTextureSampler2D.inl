// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
VulkanTextureSampler2D::VulkanTextureSampler2D(cobalt::logging::ILogger* log, VulkanRenderer* renderer)
: VulkanTextureSampler<ITextureSampler2D>(log, renderer)
{
	FlagBuildStateModified();
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void VulkanTextureSampler2D::Delete()
{
	Renderer()->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
// Format methods
//----------------------------------------------------------------------------------------
void VulkanTextureSampler2D::SetTextureWrapMode(WrapMode wrapModeHorizontal, WrapMode wrapModeVertical)
{
	SetTextureWrapModeInternal(wrapModeHorizontal, wrapModeVertical);
}

//----------------------------------------------------------------------------------------
void VulkanTextureSampler2D::SetAnisotropicFilterMode(bool enabled, int maxAnisotropy)
{
	SetAnisotropicFilterModeInternal(enabled, maxAnisotropy);
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
void VulkanTextureSampler2D::FlagObjectModified()
{
	Renderer()->FlagObjectModified(this);
}

} // namespace cobalt::graphics
