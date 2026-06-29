// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "IModuleHandle.h"
#include "IRendererPlugin.h"
#include <memory>
namespace cobalt { namespace graphics {

class RendererPlugin : public IRendererPlugin
{
public:
	// Data access methods
	inline ApiFamily GetApiFamily() const;
	inline ApiVersion GetTargetApiVersion() const;
	inline Marshal::Ret<std::string> GetName() const;
	inline Marshal::Ret<std::string> GetDisplayName() const;
	inline void SetModuleHandle(IModuleHandle::unique_ptr moduleHandle);

	// Allocation methods
	inline IGraphicsDeviceEnumerator::unique_ptr CreateGraphicsDeviceEnumerator(cobalt::logging::ILogger::unique_ptr log) const;

protected:
	// Data access methods
	inline void SetApiFamily(ApiFamily value) override;
	inline void SetTargetApiVersion(ApiVersion value) override;
	inline void SetName(const Marshal::In<std::string>& value) override;
	inline void SetDisplayName(const Marshal::In<std::string>& value) override;

	// Allocation methods
	inline void SetAllocationFunction(AllocatorPointer allocator) override;

private:
	std::shared_ptr<IModuleHandle> _moduleHandle;
	AllocatorPointer _allocator;
	ApiFamily _apiFamily = {};
	ApiVersion _apiVersion;
	std::string _name;
	std::string _displayName;
};

}} // namespace cobalt::graphics
#include "RendererPlugin.inl"
