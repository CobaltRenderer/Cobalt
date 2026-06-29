// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Macros.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Cobalt_TransferBatch_Internal* Cobalt_TransferBatch;

// Enumerations
typedef enum
{
	Cobalt_StartTiming_AfterCurrentFrame,
	Cobalt_StartTiming_Immediately,
} Cobalt_StartTiming;

typedef enum
{
	Cobalt_EndTiming_BeforeNextFrame,
	Cobalt_EndTiming_AnyFrame,
} Cobalt_EndTiming;

// Submission methods
COBALT_FUNCTION_EXPORT char Cobalt_TransferBatch_SubmitBatch(Cobalt_TransferBatch batch);
COBALT_FUNCTION_EXPORT char Cobalt_TransferBatch_IsSubmitted(Cobalt_TransferBatch batch);
COBALT_FUNCTION_EXPORT char Cobalt_TransferBatch_IsComplete(Cobalt_TransferBatch batch);
COBALT_FUNCTION_EXPORT void Cobalt_TransferBatch_WaitForComplete(Cobalt_TransferBatch batch);

// Initialization methods
COBALT_FUNCTION_EXPORT void Cobalt_TransferBatch_Delete(Cobalt_TransferBatch batch);

#ifdef __cplusplus
}
#endif
