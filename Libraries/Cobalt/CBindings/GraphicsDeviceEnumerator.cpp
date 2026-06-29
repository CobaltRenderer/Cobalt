// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "GraphicsDeviceEnumerator.h"
#include <Cobalt/RendererInterface/RendererInterface.pkg>
#include <cstring>

using namespace cobalt::graphics;

//----------------------------------------------------------------------------------------
// Device methods
//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_GraphicsDeviceEnumerator_EnumerateDevices(Cobalt_GraphicsDeviceEnumerator enumerator, Cobalt_DeviceEnumerationFlags enumerationFlags)
{
	auto _this = reinterpret_cast<IGraphicsDeviceEnumerator*>(enumerator);
	auto _enumerationFlags = (IGraphicsDeviceEnumerator::EnumerationFlags)enumerationFlags;

	return _this->EnumerateDevices(_enumerationFlags) ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
char Cobalt_GraphicsDeviceEnumerator_FoundDevice(Cobalt_GraphicsDeviceEnumerator enumerator)
{
	auto _this = reinterpret_cast<IGraphicsDeviceEnumerator*>(enumerator);

	return _this->FoundDevice() ? 1 : 0;
}

//----------------------------------------------------------------------------------------
void Cobalt_GraphicsDeviceEnumerator_GetAllDevices(Cobalt_GraphicsDeviceEnumerator enumerator, Cobalt_GraphicsDevice* devices, size_t* devicesLength)
{
	auto _this = reinterpret_cast<IGraphicsDeviceEnumerator*>(enumerator);

	auto _devices = _this->GetAllDevices().Get();
	if (_devices.size() <= *devicesLength)
	{
		std::memcpy(devices, _devices.data(), _devices.size() * sizeof(Cobalt_GraphicsDevice));
	}
	*devicesLength = _devices.size();
}

//----------------------------------------------------------------------------------------
void Cobalt_GraphicsDeviceEnumerator_GetFilteredDevices(Cobalt_GraphicsDeviceEnumerator enumerator, Cobalt_GraphicsDevice* devices, size_t* devicesLength)
{
	auto _this = reinterpret_cast<IGraphicsDeviceEnumerator*>(enumerator);

	auto _devices = _this->GetFilteredDevices().Get();
	if (_devices.size() <= *devicesLength)
	{
		std::memcpy(devices, _devices.data(), _devices.size() * sizeof(Cobalt_GraphicsDevice));
	}
	*devicesLength = _devices.size();
}

//----------------------------------------------------------------------------------------
void Cobalt_GraphicsDeviceEnumerator_GetPreferredDevice(Cobalt_GraphicsDeviceEnumerator enumerator, Cobalt_GraphicsDevice* preferredDevice)
{
	auto _this = reinterpret_cast<IGraphicsDeviceEnumerator*>(enumerator);

	*preferredDevice = reinterpret_cast<Cobalt_GraphicsDevice>(_this->GetPreferredDevice());
}

//----------------------------------------------------------------------------------------
// Filtering methods
//----------------------------------------------------------------------------------------
void Cobalt_GraphicsDeviceEnumerator_FilterDevice(Cobalt_GraphicsDeviceEnumerator enumerator, Cobalt_GraphicsDevice targetDevice)
{
	auto _this = reinterpret_cast<IGraphicsDeviceEnumerator*>(enumerator);
	auto _targetDevice = reinterpret_cast<IGraphicsDevice*>(targetDevice);

	_this->FilterDevice(_targetDevice);
}

//----------------------------------------------------------------------------------------
void Cobalt_GraphicsDeviceEnumerator_FilterDevicesOfType(Cobalt_GraphicsDeviceEnumerator enumerator, Cobalt_DeviceType type)
{
	auto _this = reinterpret_cast<IGraphicsDeviceEnumerator*>(enumerator);
	auto _type = (IGraphicsDevice::DeviceType)type;

	_this->FilterDevicesOfType(_type);
}

//----------------------------------------------------------------------------------------
void Cobalt_GraphicsDeviceEnumerator_FilterDevicesNotOfType(Cobalt_GraphicsDeviceEnumerator enumerator, Cobalt_DeviceType type)
{
	auto _this = reinterpret_cast<IGraphicsDeviceEnumerator*>(enumerator);
	auto _type = (IGraphicsDevice::DeviceType)type;

	_this->FilterDevicesNotOfType(_type);
}

//----------------------------------------------------------------------------------------
void Cobalt_GraphicsDeviceEnumerator_FilterDevicesWithoutFeature(Cobalt_GraphicsDeviceEnumerator enumerator, Cobalt_Feature feature)
{
	auto _this = reinterpret_cast<IGraphicsDeviceEnumerator*>(enumerator);
	auto _feature = (IGraphicsDevice::Feature)feature;

	_this->FilterDevicesWithoutFeature(_feature);
}

//----------------------------------------------------------------------------------------
void Cobalt_GraphicsDeviceEnumerator_FilterDevicesWithoutAllFeatures(Cobalt_GraphicsDeviceEnumerator enumerator, const Cobalt_Feature* featureSet, size_t featureSetLength)
{
	auto _this = reinterpret_cast<IGraphicsDeviceEnumerator*>(enumerator);
	auto _featureSet = reinterpret_cast<const IGraphicsDevice::Feature*>(featureSet);

	auto set = std::set<IGraphicsDevice::Feature>(_featureSet, _featureSet + featureSetLength);

	_this->FilterDevicesWithoutAllFeatures(set);
}

//----------------------------------------------------------------------------------------
void Cobalt_GraphicsDeviceEnumerator_ClearDeviceFilters(Cobalt_GraphicsDeviceEnumerator enumerator)
{
	auto _this = reinterpret_cast<IGraphicsDeviceEnumerator*>(enumerator);

	_this->ClearDeviceFilters();
}

//----------------------------------------------------------------------------------------
void Cobalt_GraphicsDeviceEnumerator_Delete(Cobalt_GraphicsDeviceEnumerator enumerator)
{
	auto _this = reinterpret_cast<IGraphicsDeviceEnumerator*>(enumerator);

	_this->Delete();
}
