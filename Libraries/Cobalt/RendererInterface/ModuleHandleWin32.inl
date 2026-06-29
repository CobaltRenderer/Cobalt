// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
namespace cobalt { namespace graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
ModuleHandleWin32::ModuleHandleWin32(HMODULE moduleHandle)
: _moduleHandle(moduleHandle)
{}

//----------------------------------------------------------------------------------------
ModuleHandleWin32::~ModuleHandleWin32()
{
	FreeLibrary(_moduleHandle);
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
IModuleHandle::unique_ptr ModuleHandleWin32::Create(HMODULE moduleHandle)
{
	return IModuleHandle::unique_ptr(new ModuleHandleWin32(moduleHandle));
}

//----------------------------------------------------------------------------------------
IModuleHandle::unique_ptr ModuleHandleWin32::Clone() const
{
	HMODULE duplicateModuleHandle;
	GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, reinterpret_cast<LPCWSTR>(_moduleHandle), &duplicateModuleHandle);
	return IModuleHandle::unique_ptr(new ModuleHandleWin32(duplicateModuleHandle));
}

//----------------------------------------------------------------------------------------
void ModuleHandleWin32::Delete()
{
	delete this;
}

}} // namespace cobalt::graphics
