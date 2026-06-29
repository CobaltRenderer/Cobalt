// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Deleter.h"
#include "IGraphicsDevice.h"
#include <Cobalt/Marshalling/Marshalling.pkg>
#include <memory>
#include <set>
#include <vector>
namespace cobalt { namespace graphics {
using namespace cobalt::marshalling::operators;

class IGraphicsDeviceEnumerator
{
public:
	// Enumerations
	enum class EnumerationFlags : uint64_t
	{
		None = 0,
		NativeApiValidation = 0x00000001,
		HeadlessRendering = 0x00000002,
	};

	// Typedefs
	typedef std::unique_ptr<IGraphicsDeviceEnumerator, Deleter<IGraphicsDeviceEnumerator>> unique_ptr;

public:
	// Initialization methods
	virtual void Delete() = 0;

	// Device methods
	virtual SuccessToken EnumerateDevices(EnumerationFlags flags = EnumerationFlags::None) = 0;
	virtual bool FoundDevice() const = 0;
	virtual Marshal::Ret<std::vector<IGraphicsDevice*>> GetAllDevices() const = 0;
	virtual Marshal::Ret<std::vector<IGraphicsDevice*>> GetFilteredDevices() const = 0;
	virtual IGraphicsDevice* GetPreferredDevice() const = 0;

	// Filtering methods
	virtual void FilterDevice(IGraphicsDevice* targetDevice) = 0;
	virtual void FilterDevicesOfType(IGraphicsDevice::DeviceType type) = 0;
	virtual void FilterDevicesNotOfType(IGraphicsDevice::DeviceType type) = 0;
	virtual void FilterDevicesWithoutFeature(IGraphicsDevice::Feature feature) = 0;
	virtual void FilterDevicesWithoutAllFeatures(const Marshal::In<std::set<IGraphicsDevice::Feature>>& featureSet) = 0;
	virtual void ClearDeviceFilters() = 0;

protected:
	// Constructors
	~IGraphicsDeviceEnumerator() = default;
};

}} // namespace cobalt::graphics
#include "IGraphicsDeviceEnumerator.inl"
