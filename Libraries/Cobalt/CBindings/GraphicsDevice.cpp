// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "GraphicsDevice.h"
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <cstring>

using namespace cobalt::graphics;

//----------------------------------------------------------------------------------------
// Info methods
//----------------------------------------------------------------------------------------
void Cobalt_GraphicsDevice_GetDeviceType(Cobalt_GraphicsDevice device, Cobalt_DeviceType* deviceType)
{
	auto _this = reinterpret_cast<IGraphicsDevice*>(device);

	*deviceType = (Cobalt_DeviceType)_this->GetDeviceType();
}

//----------------------------------------------------------------------------------------
void Cobalt_GraphicsDevice_GetVendor(Cobalt_GraphicsDevice device, Cobalt_Vendor* vendor)
{
	auto _this = reinterpret_cast<IGraphicsDevice*>(device);

	*vendor = (Cobalt_Vendor)_this->GetVendor();
}

//----------------------------------------------------------------------------------------
void Cobalt_GraphicsDevice_GetVendorName(Cobalt_GraphicsDevice device, char* vendorName, size_t* vendorNameLength)
{
	auto _this = reinterpret_cast<IGraphicsDevice*>(device);

	auto _vendorName = _this->GetVendorName().Get();
	if (_vendorName.size() <= *vendorNameLength)
	{
		std::memcpy(vendorName, _vendorName.data(), _vendorName.size()); // NOLINT(bugprone-not-null-terminated-result)
	}
	*vendorNameLength = _vendorName.size();
}

//----------------------------------------------------------------------------------------
void Cobalt_GraphicsDevice_GetDeviceName(Cobalt_GraphicsDevice device, char* deviceName, size_t* deviceNameLength)
{
	auto _this = reinterpret_cast<IGraphicsDevice*>(device);

	auto _deviceName = _this->GetDeviceName().Get();
	if (_deviceName.size() <= *deviceNameLength)
	{
		std::memcpy(deviceName, _deviceName.data(), _deviceName.size()); // NOLINT(bugprone-not-null-terminated-result)
	}
	*deviceNameLength = _deviceName.size();
}

//----------------------------------------------------------------------------------------
void Cobalt_GraphicsDevice_GetDriverInfo(Cobalt_GraphicsDevice device, char* driverInfo, size_t* driverInfoLength)
{
	auto _this = reinterpret_cast<IGraphicsDevice*>(device);

	auto _driverInfo = _this->GetDriverInfo().Get();
	if (_driverInfo.size() <= *driverInfoLength)
	{
		std::memcpy(driverInfo, _driverInfo.data(), _driverInfo.size()); // NOLINT(bugprone-not-null-terminated-result)
	}
	*driverInfoLength = _driverInfo.size();
}

//----------------------------------------------------------------------------------------
size_t Cobalt_GraphicsDevice_GetMemorySizeInBytes(Cobalt_GraphicsDevice device, Cobalt_MemoryType memoryType)
{
	auto _this = reinterpret_cast<IGraphicsDevice*>(device);

	return _this->GetMemorySizeInBytes((IGraphicsDevice::MemoryType)memoryType);
}

//----------------------------------------------------------------------------------------
// Limit methods
//----------------------------------------------------------------------------------------
void Cobalt_GraphicsDevice_GetImageLimits(Cobalt_GraphicsDevice device, Cobalt_ImageLimits* imageLimits)
{
	auto _this = reinterpret_cast<IGraphicsDevice*>(device);

	auto limits = _this->GetImageLimits();
	std::memcpy(imageLimits, &limits, sizeof(Cobalt_ImageLimits));
}

//----------------------------------------------------------------------------------------
void Cobalt_GraphicsDevice_GetShaderLimits(Cobalt_GraphicsDevice device, Cobalt_ShaderLimits* shaderLimits)
{
	auto _this = reinterpret_cast<IGraphicsDevice*>(device);

	auto limits = _this->GetShaderLimits();
	std::memcpy(shaderLimits, &limits, sizeof(Cobalt_ShaderLimits));
}

//----------------------------------------------------------------------------------------
void Cobalt_GraphicsDevice_GetDrawLimits(Cobalt_GraphicsDevice device, Cobalt_DrawLimits* drawLimits)
{
	auto _this = reinterpret_cast<IGraphicsDevice*>(device);

	auto limits = _this->GetDrawLimits();
	std::memcpy(drawLimits, &limits, sizeof(Cobalt_DrawLimits));
}

//----------------------------------------------------------------------------------------
void Cobalt_GraphicsDevice_GetFrameBufferLimits(Cobalt_GraphicsDevice device, Cobalt_FrameBufferLimits* frameBufferLimits)
{
	auto _this = reinterpret_cast<IGraphicsDevice*>(device);

	auto limits = _this->GetFrameBufferLimits();
	std::memcpy(frameBufferLimits, &limits, sizeof(Cobalt_FrameBufferLimits));
}

//----------------------------------------------------------------------------------------
void Cobalt_GraphicsDevice_GetDataBufferLimits(Cobalt_GraphicsDevice device, Cobalt_DataBufferLimits* dataBufferLimits)
{
	auto _this = reinterpret_cast<IGraphicsDevice*>(device);

	auto limits = _this->GetDataBufferLimits();
	std::memcpy(dataBufferLimits, &limits, sizeof(Cobalt_DataBufferLimits));
}

//----------------------------------------------------------------------------------------
// Feature methods
//----------------------------------------------------------------------------------------
char Cobalt_GraphicsDevice_IsFeatureSupported(Cobalt_GraphicsDevice device, Cobalt_Feature feature)
{
	auto _this = reinterpret_cast<IGraphicsDevice*>(device);

	return _this->IsFeatureSupported((IGraphicsDevice::Feature)feature) ? 1 : 0;
}

//----------------------------------------------------------------------------------------
char Cobalt_GraphicsDevice_AreAllFeaturesSupported(Cobalt_GraphicsDevice device, const Cobalt_Feature* featureSet, size_t featureSetLength)
{
	auto _this = reinterpret_cast<IGraphicsDevice*>(device);
	auto _featureSet = reinterpret_cast<const IGraphicsDevice::Feature*>(featureSet);

	auto set = std::set<IGraphicsDevice::Feature>(_featureSet, _featureSet + featureSetLength);
	return _this->AreAllFeaturesSupported(set) ? 1 : 0;
}

//----------------------------------------------------------------------------------------
void Cobalt_GraphicsDevice_GetAllSupportedFeatures(Cobalt_GraphicsDevice device, Cobalt_Feature* featureSet, size_t* featureSetLength)
{
	auto _this = reinterpret_cast<IGraphicsDevice*>(device);

	auto set = _this->GetAllSupportedFeatures().Get();
	if (set.size() <= *featureSetLength)
	{
		size_t i = 0;
		for (auto& feature : set)
		{
			featureSet[i++] = (Cobalt_Feature)feature;
		}
	}
	*featureSetLength = set.size();
}

//----------------------------------------------------------------------------------------
char Cobalt_GraphicsDevice_IsTextureFormatSupported(Cobalt_GraphicsDevice device, Cobalt_ImageFormat imageFormat, Cobalt_DataFormat dataFormat)
{
	auto _this = reinterpret_cast<IGraphicsDevice*>(device);

	return _this->IsTextureFormatSupported((ITextureBuffer::ImageFormat)imageFormat, (ITextureBuffer::DataFormat)dataFormat) ? 1 : 0;
}

//----------------------------------------------------------------------------------------
// Renderer methods
//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_GraphicsDevice_CreateRenderer(Cobalt_GraphicsDevice device, const Cobalt_Feature* enabledFeatures, size_t enabledFeaturesLength, const Cobalt_RendererOption* enabledOptions, size_t enabledOptionsLength, Cobalt_Renderer* renderer)
{
	auto _this = reinterpret_cast<IGraphicsDevice*>(device);
	auto _enabledFeatures = reinterpret_cast<const IGraphicsDevice::Feature*>(enabledFeatures);
	auto _enabledOptions = reinterpret_cast<const IRenderer::Options*>(enabledOptions);

	auto featureSet = std::set<IGraphicsDevice::Feature>(_enabledFeatures, _enabledFeatures + enabledFeaturesLength);
	auto optionSet = std::set<IRenderer::Options>(_enabledOptions, _enabledOptions + enabledOptionsLength);

	auto _renderer = _this->CreateRenderer(featureSet, optionSet);
	*renderer = reinterpret_cast<Cobalt_Renderer>(_renderer.release());
	return COBALT_SUCCESS;
}
