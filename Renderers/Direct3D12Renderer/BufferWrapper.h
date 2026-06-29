// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Direct3DHeaders.h"
namespace cobalt::graphics {

struct BufferWrapper
{
	ID3D12Resource* buffer = nullptr;
	D3D12_RESOURCE_STATES lastResourceState = {};
	D3D12_RESOURCE_STATES defaultResourceState = {};
};

} // namespace cobalt::graphics
