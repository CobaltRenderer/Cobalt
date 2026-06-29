// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
Direct3DTextureBuffer3D::Direct3DTextureBuffer3D(cobalt::logging::ILogger* log, Direct3DRenderer* renderer)
: Direct3DTextureBuffer<ITextureBuffer3D, V3UInt32>(log, renderer)
{}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void Direct3DTextureBuffer3D::Delete()
{
	Renderer()->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
void Direct3DTextureBuffer3D::FlagObjectModified()
{
	Renderer()->FlagObjectModified(this);
}

//----------------------------------------------------------------------------------------
void Direct3DTextureBuffer3D::PopulateTextureResourceDescription(CD3DX12_RESOURCE_DESC& resourceDescription, D3D12_RESOURCE_FLAGS resourceFlags, DXGI_FORMAT internalFormat)
{
	int mipmapLevels = MipmapLevelCount();
	auto imageDimensions = MipmapLevelDimensions(0);
	resourceDescription = CD3DX12_RESOURCE_DESC::Tex3D(internalFormat, imageDimensions.X(), imageDimensions.Y(), (UINT16)imageDimensions.Z(), (UINT16)mipmapLevels, resourceFlags);
}

//----------------------------------------------------------------------------------------
void Direct3DTextureBuffer3D::PopulateShaderResourceViewDescription(D3D12_SHADER_RESOURCE_VIEW_DESC& viewDescription, DXGI_FORMAT internalFormat)
{
	int mipmapLevels = MipmapLevelCount();
	viewDescription.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	viewDescription.Format = internalFormat;
	viewDescription.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
	viewDescription.Texture3D.MipLevels = (UINT16)mipmapLevels;
}

} // namespace cobalt::graphics
