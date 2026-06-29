// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
VulkanTextureSamplerCubeArray::VulkanTextureSamplerCubeArray(cobalt::logging::ILogger* log, VulkanRenderer* renderer)
: VulkanTextureSampler<ITextureSamplerCubeArray>(log, renderer)
{
	FlagBuildStateModified();
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void VulkanTextureSamplerCubeArray::Delete()
{
	Renderer()->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
// Format methods
//----------------------------------------------------------------------------------------
void VulkanTextureSamplerCubeArray::SetTextureWrapMode(WrapMode wrapModeHorizontal, WrapMode wrapModeVertical)
{
	SetTextureWrapModeInternal(wrapModeHorizontal, wrapModeVertical);
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
void VulkanTextureSamplerCubeArray::FlagObjectModified()
{
	Renderer()->FlagObjectModified(this);
}

} // namespace cobalt::graphics
