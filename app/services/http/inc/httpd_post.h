#pragma once

#include <stdint.h>

/* Shim functions */
/**
 * @brief Handle parsed POST key-value pairs from the HTTP server shim.
 *
 * @param key POST parameter key.
 * @param val POST parameter value.
 */
void httpd_post_cb(char *key, char *val);

#define HTTPD_IAP_UPLOAD_STATE_IDLE        ((uint8_t)0u)
#define HTTPD_IAP_UPLOAD_STATE_IN_PROGRESS ((uint8_t)1u)
#define HTTPD_IAP_UPLOAD_STATE_READY       ((uint8_t)2u)
#define HTTPD_IAP_UPLOAD_STATE_ERROR       ((uint8_t)3u)

#define HTTPD_IAP_UPLOAD_ERROR_NONE       ((uint8_t)0u)
#define HTTPD_IAP_UPLOAD_ERROR_BEGIN      ((uint8_t)1u)
#define HTTPD_IAP_UPLOAD_ERROR_PUSH       ((uint8_t)2u)
#define HTTPD_IAP_UPLOAD_ERROR_INCOMPLETE ((uint8_t)3u)
#define HTTPD_IAP_UPLOAD_ERROR_FINISH     ((uint8_t)4u)

/**
 * @brief Get current IAP upload state and progress counters.
 *
 * @param[out] state Current upload state.
 * @param[out] received Number of bytes received so far.
 * @param[out] total Expected total upload size in bytes.
 */
void HttpdPost_GetIapUploadState(
    uint8_t *state,
    uint32_t *received,
    uint32_t *total
);

/**
 * @brief Get the latest IAP upload error details.
 *
 * @param[out] reason High-level upload error reason.
 * @param[out] ingest_status Underlying ingest status code.
 */
void HttpdPost_GetIapUploadError(uint8_t *reason, uint8_t *ingest_status);

/**
 * @brief Check whether an uploaded image is ready for follow-up actions.
 *
 * @retval 1 Upload completed and ready.
 * @retval 0 Upload is not ready.
 */
uint8_t HttpdPost_IsIapUploadReady(void);

/**
 * @brief Clear the upload-ready flag after it has been consumed.
 */
void HttpdPost_ClearIapUploadReady(void);
