// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#ifndef _WIN32
#include "IModuleHandle.h"
#include <string>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <dlfcn.h>
#else
#include <climits>
#include <dlfcn.h>
#endif
namespace cobalt { namespace graphics {

class ModuleHandlePosix : public IModuleHandle
{
public:
	// Initialization methods
	inline static IModuleHandle::unique_ptr Create(void* moduleHandle, const std::string& modulePath);
	inline IModuleHandle::unique_ptr Clone() const override;
	inline void Delete() override;

private:
	// Constructors
	inline ModuleHandlePosix(void* moduleHandle, const std::string& modulePath);
	inline ~ModuleHandlePosix();

private:
	void* _moduleHandle = nullptr;
	std::string _modulePath;
};

}} // namespace cobalt::graphics
#include "ModuleHandlePosix.inl"
#endif
