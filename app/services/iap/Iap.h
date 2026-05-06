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
#define IAP_E_OK ((IapErrorType)0)

/** @brief IAP service operation failed. */
#define IAP_E_NOT_OK ((IapErrorType)1)

typedef uint8_t IapVerifyStateType;

#define IAP_VERIFY_STATE_IDLE      ((IapVerifyStateType)0u)
#define IAP_VERIFY_STATE_PENDING   ((IapVerifyStateType)1u)
#define IAP_VERIFY_STATE_VERIFYING ((IapVerifyStateType)2u)
#define IAP_VERIFY_STATE_VERIFIED  ((IapVerifyStateType)3u)
#define IAP_VERIFY_STATE_ERROR     ((IapVerifyStateType)4u)

typedef uint8_t IapPrepareStatusType;

#define IAP_PREPARE_E_OK      ((IapPrepareStatusType)0u)
#define IAP_PREPARE_E_PARAM   ((IapPrepareStatusType)1u)
#define IAP_PREPARE_E_STATE   ((IapPrepareStatusType)2u)
#define IAP_PREPARE_E_BACKEND ((IapPrepareStatusType)3u)

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

/**
 * @brief Reset firmware verification state.
 */
void Iap_VerifyReset(void);

/**
 * @brief Mark current uploaded image for asynchronous verification.
 */
void Iap_VerifyRequest(void);

/**
 * @brief Poll one verification step.
 */
void Iap_VerifyPoll(void);

/**
 * @brief Get current verification status and progress.
 */
void Iap_GetVerifyStatus(
    IapVerifyStateType *state,
    uint32_t *processed,
    uint32_t *total
);

/**
 * @brief Check if uploaded image verification completed successfully.
 */
uint8_t Iap_IsVerified(void);

/**
 * @brief Erase the whole secondary slot to prepare for a subsequent upload.
 *
 * @return IAP_PREPARE_E_OK on success, error code otherwise.
 */
IapPrepareStatusType Iap_PrepareUploadSlot(void);
