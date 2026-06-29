// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
Direct3DTextureBufferCube::Direct3DTextureBufferCube(cobalt::logging::ILogger* log, Direct3DRenderer* renderer)
: Direct3DTextureBuffer<ITextureBufferCube, V2UInt32>(log, renderer)
{}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void Direct3DTextureBufferCube::Delete()
{
	Renderer()->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
void Direct3DTextureBufferCube::FlagObjectModified()
{
	Renderer()->FlagObjectModified(this);
}

//----------------------------------------------------------------------------------------
void Direct3DTextureBufferCube::PopulateTextureResourceDescription(CD3DX12_RESOURCE_DESC& resourceDescription, D3D12_RESOURCE_FLAGS resourceFlags, DXGI_FORMAT internalFormat)
{
	size_t arraySize = ArraySize();
	int mipmapLevels = MipmapLevelCount();
	auto imageDimensions = MipmapLevelDimensions(0);
	resourceDescription = CD3DX12_RESOURCE_DESC::Tex2D(internalFormat, imageDimensions.X(), imageDimensions.Y(), (UINT16)arraySize, (UINT16)mipmapLevels, 1, 0, resourceFlags);
}

//----------------------------------------------------------------------------------------
void Direct3DTextureBufferCube::PopulateShaderResourceViewDescription(D3D12_SHADER_RESOURCE_VIEW_DESC& viewDescription, DXGI_FORMAT internalFormat)
{
	int mipmapLevels = MipmapLevelCount();
	viewDescription.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	viewDescription.Format = internalFormat;
	viewDescription.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	viewDescription.TextureCube.MipLevels = (UINT)mipmapLevels;
}

} // namespace cobalt::graphics
