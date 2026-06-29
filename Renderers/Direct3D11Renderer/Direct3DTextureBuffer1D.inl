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
Direct3DTextureBuffer1D::~Direct3DTextureBuffer1D()
{
	ReleaseMemory();
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void Direct3DTextureBuffer1D::Delete()
{
	Renderer()->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
void Direct3DTextureBuffer1D::ReleaseMemoryInternal()
{
	_texture.Reset();
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
void Direct3DTextureBuffer1D::FlagObjectModified()
{
	Renderer()->FlagObjectModified(this);
}

//----------------------------------------------------------------------------------------
ID3D11Resource* Direct3DTextureBuffer1D::GetTextureAsResource() const
{
	return _texture.Get();
}

//----------------------------------------------------------------------------------------
bool Direct3DTextureBuffer1D::CreateTextureObject(ImageFormat imageFormat, DataFormat dataFormat, DXGI_FORMAT nativeFormat, UINT bindFlags, UINT cpuFlags, D3D11_USAGE usageType, const std::vector<InitialDataEntry>& initialData)
{
	// Fill in the required information about the texture we want to create
	int mipmapLevels = MipmapLevelCount();
	auto imageDimensions = MipmapLevelDimensions(0);
	D3D11_TEXTURE1D_DESC textureDescription = {};
	textureDescription.Width = imageDimensions.X();
	textureDescription.MipLevels = mipmapLevels;
	textureDescription.ArraySize = 1;
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
			subresourceData.pSysMem = entry.data;
			subresourceData.SysMemPitch = 0;
			subresourceData.SysMemSlicePitch = 0;
		}
	}

	// Attempt to create the texture
	D3D11_SUBRESOURCE_DATA* subresourceDataPointer = (subresourceDataArray.empty() ? nullptr : subresourceDataArray.data());
	HRESULT createTexture1DReturn = Renderer()->GetDevice()->CreateTexture1D(&textureDescription, subresourceDataPointer, &_texture);
	if (FAILED(createTexture1DReturn))
	{
		Log()->Error("CreateTexture1D failed for Direct3DTextureBuffer1D with error code {0}", createTexture1DReturn);
		return false;
	}
	return true;
}

//----------------------------------------------------------------------------------------
// Data update methods
//----------------------------------------------------------------------------------------
bool Direct3DTextureBuffer1D::CompletePendingDataWrite(const PendingWrite& pendingWrite, ID3D11Device1* device, ID3D11DeviceContext1* context)
{
	D3D11_BOX updateRegion = {};
	D3D11_BOX* updateRegionPointer = nullptr;
	auto targetImageDimensions = MipmapLevelDimensions(pendingWrite.mipmapLevel);
	auto targetRegionDimensions = (pendingWrite.imageRegionInPixels.X() == 0) ? V1UInt32(targetImageDimensions.X() - pendingWrite.imageOffsetInPixels.X()) : pendingWrite.imageRegionInPixels;
	if ((pendingWrite.imageOffsetInPixels.X() != 0) || (pendingWrite.imageRegionInPixels.X() != 0))
	{
		updateRegion.left = (UINT)pendingWrite.imageOffsetInPixels.X();
		updateRegion.right = (UINT)(updateRegion.left + targetRegionDimensions.X());
		updateRegion.top = 0;
		updateRegion.bottom = 1;
		updateRegion.front = 0;
		updateRegion.back = 1;
		updateRegionPointer = &updateRegion;
	}
	int mipmapLevels = MipmapLevelCount();
	UINT destinationSubresource = D3D11CalcSubresource((UINT)pendingWrite.mipmapLevel, 0, (UINT)mipmapLevels);
	UINT sysMemPitch = CalculateSysMemPitch(AllocatedImageFormat(), AllocatedDataFormat(), targetRegionDimensions.X());
	context->UpdateSubresource(_texture.Get(), destinationSubresource, updateRegionPointer, pendingWrite.data.data(), sysMemPitch, 0);
	return true;
}

} // namespace cobalt::graphics
