#include "lwip/apps/fs.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "fs_custom.h"
#include "fs_custom_internal.h"
#include "httpd_post.h"
#include "WebInterface.h"

#ifndef APP_VERSION
#define APP_VERSION "0.0.0-unknown"
#endif

#if LWIP_HTTPD_CUSTOM_FILES

#define IAP_VERIFY_STATE_IDLE      ((uint8_t)0u)
#define IAP_VERIFY_STATE_PENDING   ((uint8_t)1u)
#define IAP_VERIFY_STATE_VERIFYING ((uint8_t)2u)
#define IAP_VERIFY_STATE_VERIFIED  ((uint8_t)3u)
#define IAP_VERIFY_STATE_ERROR     ((uint8_t)4u)

#define IAP_STATUS_STRING                                                      \
    "{\"app_version\":\"%s\",\"logger_active\":%s,\"upload_ready\":%s,"        \
    "\"upload_state\":\"%s\","                                                 \
    "\"upload_received\":%lu,\"upload_total\":%lu,\"received\":%lu,"           \
    "\"total\":%lu,\"verify_state\":\"%s\","                                   \
    "\"verify_processed\":%lu,\"verify_total\":%lu,\"verified\":%s,"           \
    "\"apply_ready\":%s,\"error_reason\":\"%s\",\"ingest_status\":%lu}"

#define IAP_APPLY_RESULT_STRING   "{\"ok\":%s,\"reason\":\"%s\"}"
#define IAP_PREPARE_RESULT_STRING "{\"ok\":%s,\"reason\":\"%s\"}"

bool FsCustomIap_RenderStatus(
    char *buffer,
    size_t buffer_size,
    size_t *payload_size
)
{
    uint8_t is_logger_active      = 1u;
    uint8_t upload_state          = HTTPD_IAP_UPLOAD_STATE_IDLE;
    uint8_t upload_ready          = 0u;
    uint8_t verify_state          = IAP_VERIFY_STATE_IDLE;
    uint8_t is_verified           = 0u;
    uint8_t apply_ready           = 0u;
    uint8_t upload_error          = HTTPD_IAP_UPLOAD_ERROR_NONE;
    uint8_t ingest_status         = 0u;
    uint32_t upload_received      = 0u;
    uint32_t upload_total         = 0u;
    uint32_t verify_processed     = 0u;
    uint32_t verify_total         = 0u;
    const char *upload_state_name = "idle";
    const char *verify_state_name = "idle";
    const char *error_reason_name = "none";
    int n                         = 0;

    if (0 != FsCustom_IsTracerRunning(&is_logger_active))
    {
        is_logger_active = 1u;
    }

    HttpdPost_GetIapUploadState(&upload_state, &upload_received, &upload_total);
    HttpdPost_GetIapUploadError(&upload_error, &ingest_status);
    upload_ready = HttpdPost_IsIapUploadReady();
    WebInterface_GetFirmwareVerifyStatusHook(
        &verify_state,
        &verify_processed,
        &verify_total
    );
    is_verified = WebInterface_IsFirmwareVerifiedHook();
    apply_ready = (is_logger_active == 0u && is_verified != 0u) ? 1u : 0u;

    switch (upload_state)
    {
    case HTTPD_IAP_UPLOAD_STATE_IN_PROGRESS:
        upload_state_name = "in_progress";
        break;
    case HTTPD_IAP_UPLOAD_STATE_READY:
        upload_state_name = "ready";
        break;
    case HTTPD_IAP_UPLOAD_STATE_ERROR:
        upload_state_name = "error";
        break;
    case HTTPD_IAP_UPLOAD_STATE_IDLE:
    default:
        upload_state_name = "idle";
        break;
    }

    switch (verify_state)
    {
    case IAP_VERIFY_STATE_PENDING:
        verify_state_name = "pending";
        break;
    case IAP_VERIFY_STATE_VERIFYING:
        verify_state_name = "in_progress";
        break;
    case IAP_VERIFY_STATE_VERIFIED:
        verify_state_name = "verified";
        break;
    case IAP_VERIFY_STATE_ERROR:
        verify_state_name = "error";
        break;
    case IAP_VERIFY_STATE_IDLE:
    default:
        verify_state_name = "idle";
        break;
    }

    switch (upload_error)
    {
    case HTTPD_IAP_UPLOAD_ERROR_BEGIN:
        error_reason_name = "begin";
        break;
    case HTTPD_IAP_UPLOAD_ERROR_PUSH:
        error_reason_name = "push";
        break;
    case HTTPD_IAP_UPLOAD_ERROR_INCOMPLETE:
        error_reason_name = "incomplete";
        break;
    case HTTPD_IAP_UPLOAD_ERROR_FINISH:
        error_reason_name = "finish";
        break;
    case HTTPD_IAP_UPLOAD_ERROR_NONE:
    default:
        error_reason_name = "none";
        break;
    }

    n = snprintf(
        buffer,
        buffer_size,
        IAP_STATUS_STRING,
        APP_VERSION,
        is_logger_active ? "true" : "false",
        upload_ready ? "true" : "false",
        upload_state_name,
        (unsigned long)upload_received,
        (unsigned long)upload_total,
        (unsigned long)upload_received,
        (unsigned long)upload_total,
        verify_state_name,
        (unsigned long)verify_processed,
        (unsigned long)verify_total,
        is_verified ? "true" : "false",
        apply_ready ? "true" : "false",
        error_reason_name,
        (unsigned long)ingest_status
    );
    if (n < 0 || (size_t)n >= buffer_size)
    {
        n = snprintf(
            buffer,
            buffer_size,
            "{"
            "\"logger_active\":%s,"
            "\"upload_ready\":%s,"
            "\"upload_state\":\"error\","
            "\"upload_received\":%lu,"
            "\"upload_total\":%lu,"
            "\"received\":%lu,"
            "\"total\":%lu,"
            "\"verify_state\":\"error\","
            "\"verify_processed\":%lu,"
            "\"verify_total\":%lu,"
            "\"verified\":%s,"
            "\"apply_ready\":%s,"
            "\"error_reason\":\"status_payload_too_large\","
            "\"ingest_status\":%lu"
            "}",
            is_logger_active ? "true" : "false",
            upload_ready ? "true" : "false",
            (unsigned long)upload_received,
            (unsigned long)upload_total,
            (unsigned long)upload_received,
            (unsigned long)upload_total,
            (unsigned long)verify_processed,
            (unsigned long)verify_total,
            is_verified ? "true" : "false",
            apply_ready ? "true" : "false",
            (unsigned long)ingest_status
        );
        if (n < 0 || (size_t)n >= buffer_size)
        {
            return false;
        }
    }

    *payload_size = (size_t)n;
    return true;
}

bool FsCustomIap_RenderPrepare(
    char *buffer,
    size_t buffer_size,
    size_t *payload_size
)
{
    uint8_t is_logger_active = 1u;
    const char *reason       = "internal_error";
    const char *ok_text      = "false";
    int n                    = 0;

    if (0 != FsCustom_IsTracerRunning(&is_logger_active))
    {
        is_logger_active = 1u;
    }

    if (0u != is_logger_active)
    {
        reason = "logger_running";
    }
    else
    {
        HttpdPost_ResetIapUploadSession();

        if (0u == WebInterface_PrepareFirmwareUploadHook())
        {
            reason = "prepare_failed";
        }
        else
        {
            ok_text = "true";
            reason  = "slot_erased";
        }
    }

    n = snprintf(
        buffer,
        buffer_size,
        IAP_PREPARE_RESULT_STRING,
        ok_text,
        reason
    );
    if (n < 0 || (size_t)n >= buffer_size)
    {
        return false;
    }

    *payload_size = (size_t)n;
    return true;
}

bool FsCustomIap_RenderApply(
    char *buffer,
    size_t buffer_size,
    size_t *payload_size
)
{
    uint8_t is_logger_active = 1u;
    uint8_t is_verified      = 0u;
    const char *reason       = "internal_error";
    const char *ok_text      = "false";
    int n                    = 0;

    if (0 != FsCustom_IsTracerRunning(&is_logger_active))
    {
        is_logger_active = 1u;
    }

    is_verified = WebInterface_IsFirmwareVerifiedHook();

    if (0u != is_logger_active)
    {
        reason = "logger_running";
    }
    else if (0u == is_verified)
    {
        reason = "image_not_verified";
    }
    else
    {
        WebInterface_RequestFirmwareApplyHook();
        HttpdPost_ClearIapUploadReady();
        ok_text = "true";
        reason  = "reboot_requested";
    }

    n = snprintf(buffer, buffer_size, IAP_APPLY_RESULT_STRING, ok_text, reason);
    if (n < 0 || (size_t)n >= buffer_size)
    {
        return false;
    }

    *payload_size = (size_t)n;
    return true;
}

#endif
