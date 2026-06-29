// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
Direct3DTextureBuffer2DArray::Direct3DTextureBuffer2DArray(cobalt::logging::ILogger* log, Direct3DRenderer* renderer)
: Direct3DTextureBuffer<ITextureBuffer2DArray, V2UInt32>(log, renderer)
{}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void Direct3DTextureBuffer2DArray::Delete()
{
	Renderer()->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
void Direct3DTextureBuffer2DArray::FlagObjectModified()
{
	Renderer()->FlagObjectModified(this);
}

//----------------------------------------------------------------------------------------
void Direct3DTextureBuffer2DArray::PopulateTextureResourceDescription(CD3DX12_RESOURCE_DESC& resourceDescription, D3D12_RESOURCE_FLAGS resourceFlags, DXGI_FORMAT internalFormat)
{
	size_t arraySize = ArraySize();
	int mipmapLevels = MipmapLevelCount();
	auto imageDimensions = MipmapLevelDimensions(0);
	resourceDescription = CD3DX12_RESOURCE_DESC::Tex2D(internalFormat, imageDimensions.X(), imageDimensions.Y(), (UINT16)arraySize, (UINT16)mipmapLevels, 1, 0, resourceFlags);
}

//----------------------------------------------------------------------------------------
void Direct3DTextureBuffer2DArray::PopulateShaderResourceViewDescription(D3D12_SHADER_RESOURCE_VIEW_DESC& viewDescription, DXGI_FORMAT internalFormat)
{
	size_t arraySize = ArraySize();
	int mipmapLevels = MipmapLevelCount();
	viewDescription.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	viewDescription.Format = internalFormat;
	viewDescription.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
	viewDescription.Texture2DArray.MipLevels = (UINT16)mipmapLevels;
	viewDescription.Texture2DArray.ArraySize = (UINT16)arraySize;
}

} // namespace cobalt::graphics
