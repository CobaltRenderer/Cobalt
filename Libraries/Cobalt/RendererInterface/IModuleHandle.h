// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Deleter.h"
#include <memory>
namespace cobalt { namespace graphics {

class IModuleHandle
{
public:
	// Typedefs
	typedef std::unique_ptr<IModuleHandle, Deleter<IModuleHandle>> unique_ptr;

public:
	// Initialization methods
	virtual IModuleHandle::unique_ptr Clone() const = 0;
	virtual void Delete() = 0;

protected:
	// Constructors
	~IModuleHandle() = default;
};

}} // namespace cobalt::graphics
