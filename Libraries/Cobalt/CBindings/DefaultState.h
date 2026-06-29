// Copyright (c) 2026 Maptek Pty Ltd
// Licensed under the MIT License
#pragma once
#include "Macros.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Cobalt_DefaultState_Internal* Cobalt_DefaultState;

COBALT_FUNCTION_EXPORT void Cobalt_DefaultState_Delete(Cobalt_DefaultState defaultState);

#ifdef __cplusplus
}
#endif
