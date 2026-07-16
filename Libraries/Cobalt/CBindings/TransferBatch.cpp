// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#include "TransferBatch.h"
#include <Cobalt/RendererInterface/RendererInterface.pkg>

using namespace cobalt::graphics;

//----------------------------------------------------------------------------------------
// Submission methods
//----------------------------------------------------------------------------------------
Cobalt_Result Cobalt_TransferBatch_SubmitBatch(Cobalt_TransferBatch batch)
{
	auto _this = reinterpret_cast<ITransferBatch*>(batch);

	return _this->SubmitBatch() ? COBALT_SUCCESS : COBALT_FAILURE;
}

//----------------------------------------------------------------------------------------
char Cobalt_TransferBatch_IsSubmitted(Cobalt_TransferBatch batch)
{
	auto _this = reinterpret_cast<ITransferBatch*>(batch);

	return _this->IsSubmitted() ? 1 : 0;
}

//----------------------------------------------------------------------------------------
char Cobalt_TransferBatch_IsComplete(Cobalt_TransferBatch batch)
{
	auto _this = reinterpret_cast<ITransferBatch*>(batch);

	return _this->IsComplete() ? 1 : 0;
}

//----------------------------------------------------------------------------------------
void Cobalt_TransferBatch_WaitForComplete(Cobalt_TransferBatch batch)
{
	auto _this = reinterpret_cast<ITransferBatch*>(batch);

	_this->WaitForComplete();
}

//----------------------------------------------------------------------------------------
// Initialization methods
//----------------------------------------------------------------------------------------
void Cobalt_TransferBatch_Delete(Cobalt_TransferBatch batch)
{
	auto _this = reinterpret_cast<ITransferBatch*>(batch);

	_this->Delete();
}
