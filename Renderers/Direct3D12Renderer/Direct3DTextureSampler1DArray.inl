// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
Direct3DTextureSampler1DArray::Direct3DTextureSampler1DArray(cobalt::logging::ILogger* log, Direct3DRenderer* renderer)
: Direct3DTextureSampler<ITextureSampler1DArray>(log, renderer)
{
	FlagBuildStateModified();
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void Direct3DTextureSampler1DArray::Delete()
{
	Renderer()->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
// Format methods
//----------------------------------------------------------------------------------------
void Direct3DTextureSampler1DArray::SetTextureWrapMode(WrapMode wrapModeHorizontal)
{
	SetTextureWrapModeInternal(wrapModeHorizontal);
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
void Direct3DTextureSampler1DArray::FlagObjectModified()
{
	Renderer()->FlagObjectModified(this);
}

} // namespace cobalt::graphics
