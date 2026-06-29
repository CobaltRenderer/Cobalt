// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
Direct3DTextureSamplerCubeArray::Direct3DTextureSamplerCubeArray(cobalt::logging::ILogger* log, Direct3DRenderer* renderer)
: Direct3DTextureSampler<ITextureSamplerCubeArray>(log, renderer)
{
	FlagBuildStateModified();
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void Direct3DTextureSamplerCubeArray::Delete()
{
	Renderer()->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
// Format methods
//----------------------------------------------------------------------------------------
void Direct3DTextureSamplerCubeArray::SetTextureWrapMode(WrapMode wrapModeHorizontal, WrapMode wrapModeVertical)
{
	SetTextureWrapModeInternal(wrapModeHorizontal, wrapModeVertical);
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
void Direct3DTextureSamplerCubeArray::FlagObjectModified()
{
	Renderer()->FlagObjectModified(this);
}

} // namespace cobalt::graphics
