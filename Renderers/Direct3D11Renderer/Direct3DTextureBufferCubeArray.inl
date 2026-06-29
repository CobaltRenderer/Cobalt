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
Direct3DTextureBufferCubeArray::~Direct3DTextureBufferCubeArray()
{
	ReleaseMemory();
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void Direct3DTextureBufferCubeArray::Delete()
{
	Renderer()->DeleteObject(this);
}

//----------------------------------------------------------------------------------------
void Direct3DTextureBufferCubeArray::ReleaseMemoryInternal()
{
	_texture.Reset();
}

//----------------------------------------------------------------------------------------
// Build state methods
//----------------------------------------------------------------------------------------
void Direct3DTextureBufferCubeArray::FlagObjectModified()
{
	Renderer()->FlagObjectModified(this);
}

//----------------------------------------------------------------------------------------
ID3D11Resource* Direct3DTextureBufferCubeArray::GetTextureAsResource() const
{
	return _texture.Get();
}

//----------------------------------------------------------------------------------------
bool Direct3DTextureBufferCubeArray::CreateTextureObject(ImageFormat imageFormat, DataFormat dataFormat, DXGI_FORMAT nativeFormat, UINT bindFlags, UINT cpuFlags, D3D11_USAGE usageType, const std::vector<InitialDataEntry>& initialData)
{
	// Fill in the required information about the texture we want to create
	size_t arraySize = ArraySize();
	int mipmapLevels = MipmapLevelCount();
	auto imageDimensions = MipmapLevelDimensions(0);
	D3D11_TEXTURE2D_DESC textureDescription = {};
	textureDescription.Width = imageDimensions.X();
	textureDescription.Height = imageDimensions.Y();
	textureDescription.MipLevels = (UINT)mipmapLevels;
	textureDescription.ArraySize = (UINT)arraySize;
	textureDescription.SampleDesc.Count = 1;
	textureDescription.SampleDesc.Quality = 0;
	textureDescription.Format = nativeFormat;
	textureDescription.BindFlags = bindFlags;
	textureDescription.CPUAccessFlags = cpuFlags;
	textureDescription.Usage = usageType;
	textureDescription.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

	// Fill in the initial data for this texture if required
	std::vector<D3D11_SUBRESOURCE_DATA> subresourceDataArray;
	if (!initialData.empty())
	{
		subresourceDataArray.resize(mipmapLevels * arraySize);
		for (const auto& entry : initialData)
		{
			UINT destinationSubresource = D3D11CalcSubresource((UINT)entry.mipmapLevel, (UINT)entry.arrayIndex, (UINT)mipmapLevels);
			auto& subresourceData = subresourceDataArray[destinationSubresource];
			auto mipmapDimensions = MipmapLevelDimensions(entry.mipmapLevel);
			subresourceData.pSysMem = entry.data;
			subresourceData.SysMemPitch = CalculateSysMemPitch(imageFormat, dataFormat, mipmapDimensions.X());
			subresourceData.SysMemSlicePitch = 0;
		}
	}

	// Attempt to create the texture
	D3D11_SUBRESOURCE_DATA* subresourceDataPointer = (subresourceDataArray.empty() ? nullptr : subresourceDataArray.data());
	HRESULT createTexture2DReturn = Renderer()->GetDevice()->CreateTexture2D(&textureDescription, subresourceDataPointer, &_texture);
	if (FAILED(createTexture2DReturn))
	{
		Log()->Error("CreateTexture2D failed for Direct3DTextureBufferCubeArray with error code {0}", createTexture2DReturn);
		return false;
	}
	return true;
}

//----------------------------------------------------------------------------------------
// Data update methods
//----------------------------------------------------------------------------------------
bool Direct3DTextureBufferCubeArray::CompletePendingDataWrite(const PendingWrite& pendingWrite, ID3D11Device1* device, ID3D11DeviceContext1* context)
{
	D3D11_BOX updateRegion = {};
	D3D11_BOX* updateRegionPointer = nullptr;
	auto targetImageDimensions = MipmapLevelDimensions(pendingWrite.mipmapLevel);
	auto targetRegionDimensions = ((pendingWrite.imageRegionInPixels.X() == 0) || (pendingWrite.imageRegionInPixels.Y() == 0)) ? V2UInt32(targetImageDimensions.X() - pendingWrite.imageOffsetInPixels.X(), targetImageDimensions.Y() - pendingWrite.imageOffsetInPixels.Y()) : pendingWrite.imageRegionInPixels;
	if ((pendingWrite.imageOffsetInPixels.X() != 0) || (pendingWrite.imageOffsetInPixels.Y() != 0) || (pendingWrite.imageRegionInPixels.X() != 0) || (pendingWrite.imageRegionInPixels.Y() != 0))
	{
		updateRegion.left = (UINT)pendingWrite.imageOffsetInPixels.X();
		updateRegion.right = (UINT)(updateRegion.left + targetRegionDimensions.X());
		updateRegion.top = (UINT)pendingWrite.imageOffsetInPixels.Y();
		updateRegion.bottom = (UINT)(updateRegion.top + targetRegionDimensions.Y());
		updateRegion.front = 0;
		updateRegion.back = 1;
		updateRegionPointer = &updateRegion;
	}
	int mipmapLevels = MipmapLevelCount();
	UINT destinationSubresource = D3D11CalcSubresource((UINT)pendingWrite.mipmapLevel, (UINT)pendingWrite.arrayIndex, (UINT)mipmapLevels);
	UINT sysMemPitch = CalculateSysMemPitch(AllocatedImageFormat(), AllocatedDataFormat(), targetRegionDimensions.X());
	context->UpdateSubresource(_texture.Get(), destinationSubresource, updateRegionPointer, pendingWrite.data.data(), sysMemPitch, 0);
	return true;
}

} // namespace cobalt::graphics
