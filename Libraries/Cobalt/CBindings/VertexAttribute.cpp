// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "VertexAttribute.h"
#include <Cobalt/RendererInterface/RendererInterface.pkg>

using namespace cobalt::graphics;

//----------------------------------------------------------------------------------------
// // Type methods
//----------------------------------------------------------------------------------------
Cobalt_VertexAttributeType Cobalt_VertexAttribute_GetDataType(Cobalt_VertexAttribute attribute)
{
	auto _this = reinterpret_cast<RawVertexAttribute*>(attribute);

	return (Cobalt_VertexAttributeType)_this->GetDataType();
}

//----------------------------------------------------------------------------------------
size_t Cobalt_VertexAttribute_GetVertexCount(Cobalt_VertexAttribute attribute)
{
	auto _this = reinterpret_cast<RawVertexAttribute*>(attribute);

	return _this->GetVertexCount();
}

//----------------------------------------------------------------------------------------
size_t Cobalt_VertexAttribute_GetAttributeElementCount(Cobalt_VertexAttribute attribute)
{
	auto _this = reinterpret_cast<RawVertexAttribute*>(attribute);

	return _this->GetAttributeElementCount();
}

//----------------------------------------------------------------------------------------
// Usage methods
//----------------------------------------------------------------------------------------
Cobalt_VertexPerformanceHint Cobalt_VertexAttribute_GetPerformanceHintCpu(Cobalt_VertexAttribute attribute)
{
	auto _this = reinterpret_cast<RawVertexAttribute*>(attribute);

	return (Cobalt_VertexPerformanceHint)_this->GetPerformanceHintCpu();
}

//----------------------------------------------------------------------------------------
Cobalt_VertexPerformanceHint Cobalt_VertexAttribute_GetPerformanceHintGpu(Cobalt_VertexAttribute attribute)
{
	auto _this = reinterpret_cast<RawVertexAttribute*>(attribute);

	return (Cobalt_VertexPerformanceHint)_this->GetPerformanceHintGpu();
}

//----------------------------------------------------------------------------------------
Cobalt_VertexDataPersistenceFlags Cobalt_VertexAttribute_GetDataPersistenceFlags(Cobalt_VertexAttribute attribute)
{
	auto _this = reinterpret_cast<RawVertexAttribute*>(attribute);

	return (Cobalt_VertexDataPersistenceFlags)_this->GetDataPersistenceFlags();
}

//----------------------------------------------------------------------------------------
// Binding methods
//----------------------------------------------------------------------------------------
char Cobalt_VertexAttribute_IsBoundToBuffer(Cobalt_VertexAttribute attribute)
{
	auto _this = reinterpret_cast<RawVertexAttribute*>(attribute);

	return _this->IsBoundToBuffer() ? 1 : 0;
}

//----------------------------------------------------------------------------------------
// Data methods
//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_VertexAttribute_SetInitialData(Cobalt_VertexAttribute attribute, const uint8_t* data, size_t entryCount, size_t entryStrideInBytes)
{
	auto _this = reinterpret_cast<RawVertexAttribute*>(attribute);

	return _this->SetInitialData(data, entryCount, entryStrideInBytes) ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_VertexAttribute_QueueDataUpdate(Cobalt_VertexAttribute attribute, const uint8_t* data, size_t entryCount, size_t initialVertexNo, size_t entryStrideInBytes, Cobalt_TransferBatch transferBatch)
{
	auto _this = reinterpret_cast<RawVertexAttribute*>(attribute);

	return _this->QueueDataUpdate(data, entryCount, initialVertexNo, entryStrideInBytes, reinterpret_cast<ITransferBatch*>(transferBatch)) ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
void Cobalt_VertexAttribute_Delete(Cobalt_VertexAttribute attribute)
{
	auto _this = reinterpret_cast<RawVertexAttribute*>(attribute);

	delete _this;
}
