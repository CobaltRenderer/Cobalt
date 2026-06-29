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
Direct3DTextureBuffer3D::~Direct3DTextureBuffer3D()
{
	ReleaseMemory();
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void Direct3DTextureBuffer3D::Delete()
{
	Renderer()->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
void Direct3DTextureBuffer3D::ReleaseMemoryInternal()
{
	_texture.Reset();
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
void Direct3DTextureBuffer3D::FlagObjectModified()
{
	Renderer()->FlagObjectModified(this);
}

//----------------------------------------------------------------------------------------
ID3D11Resource* Direct3DTextureBuffer3D::GetTextureAsResource() const
{
	return _texture.Get();
}

//----------------------------------------------------------------------------------------
bool Direct3DTextureBuffer3D::CreateTextureObject(ImageFormat imageFormat, DataFormat dataFormat, DXGI_FORMAT nativeFormat, UINT bindFlags, UINT cpuFlags, D3D11_USAGE usageType, const std::vector<InitialDataEntry>& initialData)
{
	// Fill in the required information about the texture we want to create
	int mipmapLevels = MipmapLevelCount();
	auto imageDimensions = MipmapLevelDimensions(0);
	D3D11_TEXTURE3D_DESC textureDescription = {};
	textureDescription.Width = imageDimensions.X();
	textureDescription.Height = imageDimensions.Y();
	textureDescription.Depth = imageDimensions.Z();
	textureDescription.MipLevels = (UINT)mipmapLevels;
	textureDescription.Format = nativeFormat;
	textureDescription.BindFlags = bindFlags;
	textureDescription.CPUAccessFlags = cpuFlags;
	textureDescription.Usage = usageType;

	// Fill in the initial data for this texture if required
	std::vector<D3D11_SUBRESOURCE_DATA> subresourceDataArray;
	if (!initialData.empty())
	{
		subresourceDataArray.resize(mipmapLevels);
		for (const auto& entry : initialData)
		{
			UINT destinationSubresource = D3D11CalcSubresource((UINT)entry.mipmapLevel, 0, (UINT)mipmapLevels);
			auto& subresourceData = subresourceDataArray[destinationSubresource];
			auto mipmapDimensions = MipmapLevelDimensions(entry.mipmapLevel);
			subresourceData.pSysMem = entry.data;
			UINT sysMemPitch;
			UINT depthPitch;
			CalculateSysMemPitchAndDepthPitch(imageFormat, dataFormat, mipmapDimensions.X(), mipmapDimensions.Y(), sysMemPitch, depthPitch);
			subresourceData.SysMemPitch = sysMemPitch;
			subresourceData.SysMemSlicePitch = depthPitch;
		}
	}

	// Attempt to create the texture
	D3D11_SUBRESOURCE_DATA* subresourceDataPointer = (subresourceDataArray.empty() ? nullptr : subresourceDataArray.data());
	HRESULT createTexture3DReturn = Renderer()->GetDevice()->CreateTexture3D(&textureDescription, subresourceDataPointer, &_texture);
	if (FAILED(createTexture3DReturn))
	{
		Log()->Error("CreateTexture3D failed for Direct3DTextureBuffer3D with error code {0}", createTexture3DReturn);
		return false;
	}
	return true;
}

//----------------------------------------------------------------------------------------
// Data update methods
//----------------------------------------------------------------------------------------
bool Direct3DTextureBuffer3D::CompletePendingDataWrite(const PendingWrite& pendingWrite, ID3D11Device1* device, ID3D11DeviceContext1* context)
{
	D3D11_BOX updateRegion = {};
	D3D11_BOX* updateRegionPointer = nullptr;
	auto targetImageDimensions = MipmapLevelDimensions(pendingWrite.mipmapLevel);
	auto targetRegionDimensions = ((pendingWrite.imageRegionInPixels.X() == 0) || (pendingWrite.imageRegionInPixels.Y() == 0) || (pendingWrite.imageRegionInPixels.Z() == 0)) ? V3UInt32(targetImageDimensions.X() - pendingWrite.imageOffsetInPixels.X(), targetImageDimensions.Y() - pendingWrite.imageOffsetInPixels.Y(), targetImageDimensions.Z() - pendingWrite.imageOffsetInPixels.Z()) : pendingWrite.imageRegionInPixels;
	if ((pendingWrite.imageOffsetInPixels.X() != 0) || (pendingWrite.imageOffsetInPixels.Y() != 0) || (pendingWrite.imageOffsetInPixels.Z() != 0) || (pendingWrite.imageRegionInPixels.X() != 0) || (pendingWrite.imageRegionInPixels.Y() != 0) || (pendingWrite.imageRegionInPixels.Z() != 0))
	{
		updateRegion.left = (UINT)pendingWrite.imageOffsetInPixels.X();
		updateRegion.right = (UINT)(updateRegion.left + targetRegionDimensions.X());
		updateRegion.top = (UINT)pendingWrite.imageOffsetInPixels.Y();
		updateRegion.bottom = (UINT)(updateRegion.top + targetRegionDimensions.Y());
		updateRegion.front = (UINT)pendingWrite.imageOffsetInPixels.Z();
		updateRegion.back = (UINT)(updateRegion.front + targetRegionDimensions.Z());
		updateRegionPointer = &updateRegion;
	}
	int mipmapLevels = MipmapLevelCount();
	UINT destinationSubresource = D3D11CalcSubresource((UINT)pendingWrite.mipmapLevel, 0, (UINT)mipmapLevels);
	UINT sysMemPitch;
	UINT depthPitch;
	CalculateSysMemPitchAndDepthPitch(AllocatedImageFormat(), AllocatedDataFormat(), targetRegionDimensions.X(), targetRegionDimensions.Y(), sysMemPitch, depthPitch);
	context->UpdateSubresource(_texture.Get(), destinationSubresource, updateRegionPointer, pendingWrite.data.data(), sysMemPitch, depthPitch);
	return true;
}

} // namespace cobalt::graphics
