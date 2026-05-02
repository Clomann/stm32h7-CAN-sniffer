/**
 * @file Iap.h
 * @brief Public initialization interface for IAP service wiring.
 */
#pragma once

#include <stdint.h>

/**
 * @brief IAP service error code type.
 */
typedef uint8_t IapErrorType;

/** @brief IAP service operation succeeded. */
#define IAP_E_OK     ((IapErrorType)0)
/** @brief IAP service operation failed. */
#define IAP_E_NOT_OK ((IapErrorType)1)

/**
 * @brief Initialize IAP writer, adapter, and ingest registry wiring.
 *
 * @return IAP_E_OK on success, otherwise IAP_E_NOT_OK.
 */
IapErrorType Iap_Init(void);

/**
 * @brief Deinitialize IAP ingress wiring.
 *
 * @return IAP_E_OK on success, otherwise IAP_E_NOT_OK.
 */
IapErrorType Iap_DeInit(void);
