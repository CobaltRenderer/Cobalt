// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
Direct3DTextureBuffer1D::Direct3DTextureBuffer1D(cobalt::logging::ILogger* log, Direct3DRenderer* renderer)
: Direct3DTextureBuffer<ITextureBuffer1D, V1UInt32>(log, renderer)
{}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void Direct3DTextureBuffer1D::Delete()
{
	Renderer()->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
void Direct3DTextureBuffer1D::FlagObjectModified()
{
	Renderer()->FlagObjectModified(this);
}

//----------------------------------------------------------------------------------------
void Direct3DTextureBuffer1D::PopulateTextureResourceDescription(CD3DX12_RESOURCE_DESC& resourceDescription, D3D12_RESOURCE_FLAGS resourceFlags, DXGI_FORMAT internalFormat)
{
	int mipmapLevels = MipmapLevelCount();
	auto imageDimensions = MipmapLevelDimensions(0);
	resourceDescription = CD3DX12_RESOURCE_DESC::Tex1D(internalFormat, imageDimensions.X(), 1, (UINT16)mipmapLevels, resourceFlags);
}

//----------------------------------------------------------------------------------------
void Direct3DTextureBuffer1D::PopulateShaderResourceViewDescription(D3D12_SHADER_RESOURCE_VIEW_DESC& viewDescription, DXGI_FORMAT internalFormat)
{
	int mipmapLevels = MipmapLevelCount();
	viewDescription.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	viewDescription.Format = internalFormat;
	viewDescription.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
	viewDescription.Texture1D.MipLevels = (UINT16)mipmapLevels;
}

} // namespace cobalt::graphics
