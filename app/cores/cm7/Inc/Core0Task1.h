#pragma once

#include "main.h"

typedef struct CanLogControlDataType CanLogControlDataType;

void Core0Task1Init(void);
void Core0Task1_SetCanLogHandle(CanLogControlDataType *handle);
