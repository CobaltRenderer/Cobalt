// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "IGraphicsDeviceEnumerator.h"
#include "IModuleHandle.h"
#include <Cobalt/LoggingInterface/LoggingInterface.pkg>
#include <Cobalt/Marshalling/Marshalling.pkg>
#include <string>
namespace cobalt { namespace graphics {
using namespace cobalt::marshalling::operators;

class IRendererPlugin
{
public:
	// Enumerations
	enum class ApiFamily
	{
		OpenGL,
		OpenGLES,
		Direct3D,
		Vulkan,
		Metal,
	};

	// Structures
	struct ApiVersion
	{
		ApiVersion()
		: major(0), minor(0)
		{}
		ApiVersion(uint32_t major, uint32_t minor)
		: major(major), minor(minor)
		{}

		uint32_t major;
		uint32_t minor;
	};

	// Typedefs
	typedef IGraphicsDeviceEnumerator* (*AllocatorPointer)(cobalt::logging::ILogger* log);

	// Using statements
	using GetRendererPluginFunctionType = bool(unsigned int indexNo, IRendererPlugin& rendererPlugin);

public:
	// Data access methods
	virtual void SetApiFamily(ApiFamily value) = 0;
	virtual void SetTargetApiVersion(ApiVersion value) = 0;
	virtual void SetName(const Marshal::In<std::string>& value) = 0;
	virtual void SetDisplayName(const Marshal::In<std::string>& value) = 0;

	// Allocation methods
	virtual void SetAllocationFunction(AllocatorPointer allocator) = 0;
};

}} // namespace cobalt::graphics
