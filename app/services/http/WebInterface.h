#pragma once

#include <stdint.h>

void appCtrlCgiHandler(
    int iIndex,
    int iNumParams,
    char *pcParam[],
    char *pcValue[]
);

void WebInterface_GetActionHook(uint8_t action);

/**
 * @brief Hook function for handling SD card formatting requests from the web interface.
 *
 * This function serves as a glue layer between the web interface (lwip) and the
 * application's SD card formatting logic. It is called when a "Format" command 
 * is received via the web interface.
 *
 * @note This function must be implemented in the main application to define the
 *       formatting behavior.
 */
void WebInterface_RequestFormattingHook(
    uint32_t cluster_size,
    uint32_t log_file_size,
    uint32_t log_file_count
);
