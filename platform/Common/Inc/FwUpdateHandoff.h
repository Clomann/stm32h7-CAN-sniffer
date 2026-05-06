#pragma once

#include <stdint.h>

typedef uint8_t FwUpdateHandoffStatusType;

#define FW_UPDATE_HANDOFF_E_OK    ((FwUpdateHandoffStatusType)0u)
#define FW_UPDATE_HANDOFF_E_PARAM ((FwUpdateHandoffStatusType)1u)
#define FW_UPDATE_HANDOFF_E_FLASH ((FwUpdateHandoffStatusType)2u)
#define FW_UPDATE_HANDOFF_E_STATE ((FwUpdateHandoffStatusType)3u)

/**
 * @brief Persistently request firmware apply on next boot.
 *
 * Writes a validated handoff record into the reserved metadata sector.
 *
 * @return FW_UPDATE_HANDOFF_E_OK on success, error code otherwise.
 */
FwUpdateHandoffStatusType FwUpdateHandoff_RequestApply(void);

/**
 * @brief Clear any previously stored firmware apply request.
 *
 * Erases the metadata sector used for handoff.
 *
 * @return FW_UPDATE_HANDOFF_E_OK on success, error code otherwise.
 */
FwUpdateHandoffStatusType FwUpdateHandoff_ClearApplyRequest(void);

/**
 * @brief Check whether a valid firmware apply request is present.
 *
 * @return 1 if a valid apply request is present, otherwise 0.
 */
uint8_t FwUpdateHandoff_IsApplyRequested(void);

uint8_t FwUpdateHandoff_GetMarkerAddress_Hook(uint32_t *address);
