// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "IndexBuffer.h"
#include <Cobalt/RendererInterface/RendererInterface.pkg>

using namespace cobalt::graphics;

//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_IndexBuffer_AllocateMemory(Cobalt_IndexBuffer indexBuffer)
{
	auto _this = reinterpret_cast<IIndexBuffer*>(indexBuffer);

	return _this->AllocateMemory() ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_IndexBuffer_AllocateMemoryWithAlias(Cobalt_IndexBuffer indexBuffer, Cobalt_TexelArray texelArray)
{
	auto _this = reinterpret_cast<IIndexBuffer*>(indexBuffer);

	return _this->AllocateMemoryWithAlias(reinterpret_cast<ITexelArray*>(texelArray)) ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
// Binding methods
//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_IndexBuffer_BindIndexAttribute(Cobalt_IndexBuffer indexBuffer, Cobalt_IndexAttribute attribute)
{
	auto _this = reinterpret_cast<IIndexBuffer*>(indexBuffer);
	auto _attribute = reinterpret_cast<IIndexAttribute*>(attribute);

	return _this->BindIndexAttribute(*_attribute) ? COBALT_SUCCESS : COBALT_FAILURE;
}
//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_IndexBuffer_BindIndexAttributeManualLayout(Cobalt_IndexBuffer indexBuffer, Cobalt_IndexAttribute attribute, size_t bufferOffsetInBytes, size_t bufferStrideInBytes)
{
	auto _this = reinterpret_cast<IIndexBuffer*>(indexBuffer);
	auto _attribute = reinterpret_cast<IIndexAttribute*>(attribute);

	return _this->BindIndexAttributeManualLayout(*_attribute, bufferOffsetInBytes, bufferStrideInBytes) ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
// Data methods
//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_IndexBuffer_SetRawInitialData(Cobalt_IndexBuffer indexBuffer, const uint8_t* data, size_t dataSizeInBytes)
{
	auto _this = reinterpret_cast<IIndexBuffer*>(indexBuffer);

	return _this->SetRawInitialData(data, dataSizeInBytes) ? COBALT_SUCCESS : COBALT_FAILURE;
}
//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_IndexBuffer_QueueRawDataUpdate(Cobalt_IndexBuffer indexBuffer, const uint8_t* data, size_t dataSizeInBytes, size_t bufferOffsetInBytes, Cobalt_TransferBatch transferBatch)
{
	auto _this = reinterpret_cast<IIndexBuffer*>(indexBuffer);

	return _this->QueueRawDataUpdate(data, dataSizeInBytes, bufferOffsetInBytes, reinterpret_cast<ITransferBatch*>(transferBatch)) ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
void Cobalt_IndexBuffer_Delete(Cobalt_IndexBuffer indexBuffer)
{
	auto _this = reinterpret_cast<IIndexBuffer*>(indexBuffer);

	_this->Delete();
}
