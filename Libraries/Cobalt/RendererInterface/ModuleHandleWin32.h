// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#ifdef _WIN32
#include "IModuleHandle.h"
#include "Win32Headers.h"
namespace cobalt { namespace graphics {

class ModuleHandleWin32 : public IModuleHandle
{
public:
	// Initialization methods
	inline static IModuleHandle::unique_ptr Create(HMODULE moduleHandle);
	inline IModuleHandle::unique_ptr Clone() const override;
	inline void Delete() override;

private:
	// Constructors
	explicit inline ModuleHandleWin32(HMODULE moduleHandle);
	inline ~ModuleHandleWin32();

private:
	HMODULE _moduleHandle = nullptr;
};

}} // namespace cobalt::graphics
#include "ModuleHandleWin32.inl"
#endif
