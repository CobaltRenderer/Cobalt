// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Deleter.h"
#include "IStateContainer.h"
#include <memory>
namespace cobalt { namespace graphics {

class IDefaultState : public IStateContainer
{
public:
	// Typedefs
	typedef std::unique_ptr<IDefaultState, Deleter<IDefaultState>> unique_ptr;

public:
	// Initialization methods
	virtual void Delete() = 0;

protected:
	// Constructors
	~IDefaultState() = default;
};

}} // namespace cobalt::graphics
