// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
OpenGLTextureSampler2D::OpenGLTextureSampler2D(cobalt::logging::ILogger* log, OpenGLRenderer* renderer)
: OpenGLTextureSampler<ITextureSampler2D>(log, renderer)
{
	FlagBuildStateModified();
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void OpenGLTextureSampler2D::Delete()
{
	Renderer()->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
// Format methods
//----------------------------------------------------------------------------------------
void OpenGLTextureSampler2D::SetTextureWrapMode(WrapMode wrapModeHorizontal, WrapMode wrapModeVertical)
{
	SetTextureWrapModeInternal(wrapModeHorizontal, wrapModeVertical);
}

//----------------------------------------------------------------------------------------
void OpenGLTextureSampler2D::SetAnisotropicFilterMode(bool enabled, int maxAnisotropy)
{
	SetAnisotropicFilterModeInternal(enabled, maxAnisotropy);
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
void OpenGLTextureSampler2D::FlagObjectModified()
{
	Renderer()->FlagObjectModified(this);
}

} // namespace cobalt::graphics
