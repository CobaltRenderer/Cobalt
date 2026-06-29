// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
namespace cobalt { namespace graphics {

//----------------------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------------------
ModuleHandlePosix::ModuleHandlePosix(void* moduleHandle, const std::string& modulePath)
: _moduleHandle(moduleHandle), _modulePath(modulePath)
{}

//----------------------------------------------------------------------------------------
ModuleHandlePosix::~ModuleHandlePosix()
{
	if (_moduleHandle != nullptr)
	{
		dlclose(_moduleHandle);
	}
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
IModuleHandle::unique_ptr ModuleHandlePosix::Create(void* moduleHandle, const std::string& modulePath)
{
	return IModuleHandle::unique_ptr(new ModuleHandlePosix(moduleHandle, modulePath));
}

//----------------------------------------------------------------------------------------
IModuleHandle::unique_ptr ModuleHandlePosix::Clone() const
{
	auto duplicateModuleHandle = dlopen(_modulePath.c_str(), RTLD_NOW);
	if (duplicateModuleHandle == nullptr)
	{
		return nullptr;
	}
	return IModuleHandle::unique_ptr(new ModuleHandlePosix(duplicateModuleHandle, _modulePath));
}

//----------------------------------------------------------------------------------------
void ModuleHandlePosix::Delete()
{
	delete this;
}

}} // namespace cobalt::graphics
