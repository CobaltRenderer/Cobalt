// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "DefaultState.h"
#include <Cobalt/RendererInterface/RendererInterface.pkg>

using namespace cobalt::graphics;

//----------------------------------------------------------------------------------------
void Cobalt_DefaultState_Delete(Cobalt_DefaultState defaultState)
{
	auto _this = reinterpret_cast<IDefaultState*>(defaultState);

	_this->Delete();
}
