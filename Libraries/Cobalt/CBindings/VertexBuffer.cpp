// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "VertexBuffer.h"
#include <Cobalt/RendererInterface/RendererInterface.pkg>

using namespace cobalt::graphics;

//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_VertexBuffer_AllocateMemory(Cobalt_VertexBuffer vertexBuffer)
{
	auto _this = reinterpret_cast<IVertexBuffer*>(vertexBuffer);

	return _this->AllocateMemory() ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_VertexBuffer_AllocateMemoryWithAlias(Cobalt_VertexBuffer vertexBuffer, Cobalt_TexelArray texelArray)
{
	auto _this = reinterpret_cast<IVertexBuffer*>(vertexBuffer);

	return _this->AllocateMemoryWithAlias(reinterpret_cast<ITexelArray*>(texelArray)) ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
// Binding methods
//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_VertexBuffer_BindVertexAttribute(Cobalt_VertexBuffer vertexBuffer, Cobalt_VertexAttribute attribute)
{
	auto _this = reinterpret_cast<IVertexBuffer*>(vertexBuffer);
	auto _attribute = reinterpret_cast<IVertexAttribute*>(attribute);

	return _this->BindVertexAttribute(*_attribute) ? COBALT_SUCCESS : COBALT_FAILURE;
}
//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_VertexBuffer_BindVertexAttributeManualLayout(Cobalt_VertexBuffer vertexBuffer, Cobalt_VertexAttribute attribute, size_t bufferOffsetInBytes, size_t bufferStrideInBytes)
{
	auto _this = reinterpret_cast<IVertexBuffer*>(vertexBuffer);
	auto _attribute = reinterpret_cast<IVertexAttribute*>(attribute);

	return _this->BindVertexAttributeManualLayout(*_attribute, bufferOffsetInBytes, bufferStrideInBytes) ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
// Data methods
//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_VertexBuffer_SetRawInitialData(Cobalt_VertexBuffer vertexBuffer, const uint8_t* data, size_t dataSizeInBytes)
{
	auto _this = reinterpret_cast<IVertexBuffer*>(vertexBuffer);

	return _this->SetRawInitialData(data, dataSizeInBytes) ? COBALT_SUCCESS : COBALT_FAILURE;
}
//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_VertexBuffer_QueueRawDataUpdate(Cobalt_VertexBuffer vertexBuffer, const uint8_t* data, size_t dataSizeInBytes, size_t bufferOffsetInBytes, Cobalt_TransferBatch transferBatch)
{
	auto _this = reinterpret_cast<IVertexBuffer*>(vertexBuffer);

	return _this->QueueRawDataUpdate(data, dataSizeInBytes, bufferOffsetInBytes, reinterpret_cast<ITransferBatch*>(transferBatch)) ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
void Cobalt_VertexBuffer_Delete(Cobalt_VertexBuffer vertexBuffer)
{
	auto _this = reinterpret_cast<IVertexBuffer*>(vertexBuffer);

	_this->Delete();
}
