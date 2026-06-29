// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "IndexAttribute.h"
#include <Cobalt/RendererInterface/RendererInterface.pkg>

using namespace cobalt::graphics;

//----------------------------------------------------------------------------------------
// // Type methods
//----------------------------------------------------------------------------------------
Cobalt_IndexAttributeType Cobalt_IndexAttribute_GetDataType(Cobalt_IndexAttribute attribute)
{
	auto _this = reinterpret_cast<RawIndexAttribute*>(attribute);

	return (Cobalt_IndexAttributeType)_this->GetDataType();
}

//----------------------------------------------------------------------------------------
size_t Cobalt_IndexAttribute_GetIndexCount(Cobalt_IndexAttribute attribute)
{
	auto _this = reinterpret_cast<RawIndexAttribute*>(attribute);

	return _this->GetIndexCount();
}

//----------------------------------------------------------------------------------------
// Usage methods
//----------------------------------------------------------------------------------------
Cobalt_IndexPerformanceHint Cobalt_IndexAttribute_GetPerformanceHintCpu(Cobalt_IndexAttribute attribute)
{
	auto _this = reinterpret_cast<RawIndexAttribute*>(attribute);

	return (Cobalt_IndexPerformanceHint)_this->GetPerformanceHintCpu();
}

//----------------------------------------------------------------------------------------
Cobalt_IndexPerformanceHint Cobalt_IndexAttribute_GetPerformanceHintGpu(Cobalt_IndexAttribute attribute)
{
	auto _this = reinterpret_cast<RawIndexAttribute*>(attribute);

	return (Cobalt_IndexPerformanceHint)_this->GetPerformanceHintGpu();
}

//----------------------------------------------------------------------------------------
Cobalt_IndexDataPersistenceFlags Cobalt_IndexAttribute_GetDataPersistenceFlags(Cobalt_IndexAttribute attribute)
{
	auto _this = reinterpret_cast<RawIndexAttribute*>(attribute);

	return (Cobalt_IndexDataPersistenceFlags)_this->GetDataPersistenceFlags();
}

//----------------------------------------------------------------------------------------
// Binding methods
//----------------------------------------------------------------------------------------
char Cobalt_IndexAttribute_IsBoundToBuffer(Cobalt_IndexAttribute attribute)
{
	auto _this = reinterpret_cast<RawIndexAttribute*>(attribute);

	return _this->IsBoundToBuffer() ? 1 : 0;
}

//----------------------------------------------------------------------------------------
// Data methods
//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_IndexAttribute_SetInitialData(Cobalt_IndexAttribute attribute, const uint8_t* data, size_t entryCount, size_t entryStrideInBytes)
{
	auto _this = reinterpret_cast<RawIndexAttribute*>(attribute);

	return _this->SetInitialData(data, entryCount, entryStrideInBytes) ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_IndexAttribute_QueueDataUpdate(Cobalt_IndexAttribute attribute, const uint8_t* data, size_t entryCount, size_t initialIndexNo, size_t entryStrideInBytes, Cobalt_TransferBatch transferBatch)
{
	auto _this = reinterpret_cast<RawIndexAttribute*>(attribute);

	return _this->QueueDataUpdate(data, entryCount, initialIndexNo, entryStrideInBytes, reinterpret_cast<ITransferBatch*>(transferBatch)) ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
void Cobalt_IndexAttribute_Delete(Cobalt_IndexAttribute attribute)
{
	auto _this = reinterpret_cast<RawIndexAttribute*>(attribute);

	delete _this;
}
