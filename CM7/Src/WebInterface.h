#pragma once

#include <stdint.h>

void appCtrlCgiHandler(
    int iIndex,
    int iNumParams,
    char *pcParam[],
    char *pcValue[]
);

void WebInterface_GetActionHook(uint8_t action);
