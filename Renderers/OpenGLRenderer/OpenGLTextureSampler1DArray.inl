// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
OpenGLTextureSampler1DArray::OpenGLTextureSampler1DArray(cobalt::logging::ILogger* log, OpenGLRenderer* renderer)
: OpenGLTextureSampler<ITextureSampler1DArray>(log, renderer)
{
	FlagBuildStateModified();
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void OpenGLTextureSampler1DArray::Delete()
{
	Renderer()->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
// Format methods
//----------------------------------------------------------------------------------------
void OpenGLTextureSampler1DArray::SetTextureWrapMode(WrapMode wrapModeHorizontal)
{
	SetTextureWrapModeInternal(wrapModeHorizontal);
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
void OpenGLTextureSampler1DArray::FlagObjectModified()
{
	Renderer()->FlagObjectModified(this);
}

} // namespace cobalt::graphics
