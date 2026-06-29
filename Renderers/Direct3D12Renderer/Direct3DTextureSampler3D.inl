// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
Direct3DTextureSampler3D::Direct3DTextureSampler3D(cobalt::logging::ILogger* log, Direct3DRenderer* renderer)
: Direct3DTextureSampler<ITextureSampler3D>(log, renderer)
{
	FlagBuildStateModified();
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void Direct3DTextureSampler3D::Delete()
{
	Renderer()->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
// Format methods
//----------------------------------------------------------------------------------------
void Direct3DTextureSampler3D::SetTextureWrapMode(WrapMode wrapModeHorizontal, WrapMode wrapModeVertical, WrapMode wrapModeDepth)
{
	SetTextureWrapModeInternal(wrapModeHorizontal, wrapModeVertical, wrapModeDepth);
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
void Direct3DTextureSampler3D::FlagObjectModified()
{
	Renderer()->FlagObjectModified(this);
}

} // namespace cobalt::graphics
