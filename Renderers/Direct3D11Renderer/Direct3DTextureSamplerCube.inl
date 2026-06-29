// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
Direct3DTextureSamplerCube::Direct3DTextureSamplerCube(cobalt::logging::ILogger* log, Direct3DRenderer* renderer)
: Direct3DTextureSampler<ITextureSamplerCube>(log, renderer)
{
	FlagBuildStateModified();
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void Direct3DTextureSamplerCube::Delete()
{
	Renderer()->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
// Format methods
//----------------------------------------------------------------------------------------
void Direct3DTextureSamplerCube::SetTextureWrapMode(WrapMode wrapModeHorizontal, WrapMode wrapModeVertical)
{
	SetTextureWrapModeInternal(wrapModeHorizontal, wrapModeVertical);
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
void Direct3DTextureSamplerCube::FlagObjectModified()
{
	Renderer()->FlagObjectModified(this);
}

} // namespace cobalt::graphics
