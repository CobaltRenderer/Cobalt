// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "ResourceArray.h"
#include <Cobalt/RendererInterface/RendererInterface.pkg>

using namespace cobalt::graphics;

//----------------------------------------------------------------------------------------
// Usage methods
//----------------------------------------------------------------------------------------
void Cobalt_ResourceArray_SetPerformanceHints(Cobalt_ResourceArray resourceArray, Cobalt_ResourceArrayPerformanceHint performanceHintCpu, Cobalt_ResourceArrayPerformanceHint performanceHintGpu)
{
	auto _this = reinterpret_cast<IResourceArray*>(resourceArray);

	_this->SetPerformanceHints((IResourceArray::PerformanceHint)performanceHintCpu, (IResourceArray::PerformanceHint)performanceHintGpu);
}

//----------------------------------------------------------------------------------------
void Cobalt_ResourceArray_SetDataPersistenceFlags(Cobalt_ResourceArray resourceArray, Cobalt_ResourceArrayDataPersistenceFlags dataPersistenceFlags)
{
	auto _this = reinterpret_cast<IResourceArray*>(resourceArray);

	_this->SetDataPersistenceFlags((IResourceArray::DataPersistenceFlags)dataPersistenceFlags);
}
