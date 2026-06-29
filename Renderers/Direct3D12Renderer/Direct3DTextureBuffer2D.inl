// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
namespace cobalt::graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
Direct3DTextureBuffer2D::Direct3DTextureBuffer2D(cobalt::logging::ILogger* log, Direct3DRenderer* renderer)
: Direct3DTextureBuffer<ITextureBuffer2D, V2UInt32>(log, renderer)
{}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void Direct3DTextureBuffer2D::Delete()
{
	Renderer()->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
void Direct3DTextureBuffer2D::FlagObjectModified()
{
	Renderer()->FlagObjectModified(this);
}

//----------------------------------------------------------------------------------------
void Direct3DTextureBuffer2D::PopulateTextureResourceDescription(CD3DX12_RESOURCE_DESC& resourceDescription, D3D12_RESOURCE_FLAGS resourceFlags, DXGI_FORMAT internalFormat)
{
	int mipmapLevels = MipmapLevelCount();
	auto imageDimensions = MipmapLevelDimensions(0);
	auto sampleCount = GetNativeSampleCountFromSampleCount(GetSampleCount());
	//##TODO## Determine what we should be setting "Quality" to here for mulitsampled textures. The quality level and
	//sample count needs to match for all textures in a framebuffer. The D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS
	//structure allows us to obtain information on the supported sample counts and corresponding maximum quality levels.
	//The documentation mentions that "The higher the quality, the lower the performance", but it gives no guide for
	//exactly how the quality level affects the way the samples are generated, nor does it give any advice on how to
	//select an appropriate level based on your requirements. Hardware we've tested on so far only has one quality level
	//supported, which an index of 0 here selects. Since a quality level of 0 is guaranteed to be valid if multisampling
	//is supported, we always pick that level here, but if and when more information becomes available about how this
	//number affects image quality, this should be re-evaluated.
	UINT sampleQuality = 0;
	resourceDescription = CD3DX12_RESOURCE_DESC::Tex2D(internalFormat, imageDimensions.X(), imageDimensions.Y(), 1, (UINT16)mipmapLevels, sampleCount, sampleQuality, resourceFlags);
}

//----------------------------------------------------------------------------------------
void Direct3DTextureBuffer2D::PopulateShaderResourceViewDescription(D3D12_SHADER_RESOURCE_VIEW_DESC& viewDescription, DXGI_FORMAT internalFormat)
{
	int mipmapLevels = MipmapLevelCount();
	viewDescription.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	viewDescription.Format = internalFormat;
	viewDescription.ViewDimension = (GetSampleCount() == SampleCount::SampleCount1 ? D3D12_SRV_DIMENSION_TEXTURE2D : D3D12_SRV_DIMENSION_TEXTURE2DMS);
	viewDescription.Texture2D.MipLevels = (UINT16)mipmapLevels;
}

} // namespace cobalt::graphics
