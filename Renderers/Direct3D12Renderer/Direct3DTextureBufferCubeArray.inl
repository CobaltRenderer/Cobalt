// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
Direct3DTextureBufferCubeArray::Direct3DTextureBufferCubeArray(cobalt::logging::ILogger* log, Direct3DRenderer* renderer)
: Direct3DTextureBuffer<ITextureBufferCubeArray, V2UInt32>(log, renderer)
{}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void Direct3DTextureBufferCubeArray::Delete()
{
	Renderer()->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
void Direct3DTextureBufferCubeArray::FlagObjectModified()
{
	Renderer()->FlagObjectModified(this);
}

//----------------------------------------------------------------------------------------
void Direct3DTextureBufferCubeArray::PopulateTextureResourceDescription(CD3DX12_RESOURCE_DESC& resourceDescription, D3D12_RESOURCE_FLAGS resourceFlags, DXGI_FORMAT internalFormat)
{
	size_t arraySize = ArraySize();
	int mipmapLevels = MipmapLevelCount();
	auto imageDimensions = MipmapLevelDimensions(0);
	resourceDescription = CD3DX12_RESOURCE_DESC::Tex2D(internalFormat, imageDimensions.X(), imageDimensions.Y(), (UINT16)arraySize, (UINT16)mipmapLevels, 1, 0, resourceFlags);
}

//----------------------------------------------------------------------------------------
void Direct3DTextureBufferCubeArray::PopulateShaderResourceViewDescription(D3D12_SHADER_RESOURCE_VIEW_DESC& viewDescription, DXGI_FORMAT internalFormat)
{
	size_t arraySize = ArraySize();
	int mipmapLevels = MipmapLevelCount();
	viewDescription.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	viewDescription.Format = internalFormat;
	viewDescription.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
	viewDescription.TextureCubeArray.MipLevels = (UINT)mipmapLevels;
	viewDescription.TextureCubeArray.NumCubes = (UINT)arraySize / 6;
}

} // namespace cobalt::graphics
