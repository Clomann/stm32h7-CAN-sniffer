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

/**
 * @brief Reset firmware verification state in the application layer.
 */
void WebInterface_ResetFirmwareVerifyHook(void);

/**
 * @brief Request firmware image verification for the currently staged update.
 */
void WebInterface_RequestFirmwareVerifyHook(void);

/**
 * @brief Query current firmware verification progress.
 *
 * @param[out] state Current verification state.
 * @param[out] processed Number of bytes already verified.
 * @param[out] total Total number of bytes to verify.
 */
void WebInterface_GetFirmwareVerifyStatusHook(
    uint8_t *state,
    uint32_t *processed,
    uint32_t *total
);

/**
 * @brief Check whether the staged firmware image is verified.
 *
 * @retval 1 Firmware image is verified.
 * @retval 0 Firmware image is not verified.
 */
uint8_t WebInterface_IsFirmwareVerifiedHook(void);

/**
 * @brief Prepare the firmware upload target before receiving data.
 *
 * @retval 1 Upload target is ready.
 * @retval 0 Preparation failed.
 */
uint8_t WebInterface_PrepareFirmwareUploadHook(void);

/**
 * @brief Request applying the verified firmware image on next reboot.
 */
void WebInterface_RequestFirmwareApplyHook(void);
