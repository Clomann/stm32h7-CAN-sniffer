#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "httpd_post.h"
#include "UpdateIngestPipeline.h"
#include "WebInterface.h"

#include "lwip/tcp.h"
#include "lwip/apps/httpd.h"

typedef struct
{
    char key[16];
    char val[32];
    u16_t klen, vlen;
    u32_t received;
    u32_t content_len;
    u8_t phase; /* 0 = key, 1 = value               */
    u8_t mode;
    u8_t iap_failed;
    u8_t iap_fail_status;
    UpdateIngestPipelineContextType iap_pipeline;
} HttpdPostStateType;

/* very small fixed-size map, because HTTPD limits the number of open
   connections anyway (MEMP_NUM_PARALLEL_HTTPD_CONNS)                  */
typedef struct
{
    void *conn;
    HttpdPostStateType ps;
} HttpdPostStateMapType;

static HttpdPostStateMapType conn_map[MEMP_NUM_PARALLEL_HTTPD_CONNS];
static void *current_connection;
static void *valid_connection;

typedef struct
{
    uint8_t state;
    uint32_t received;
    uint32_t total;
    uint8_t error_reason;
    uint8_t ingest_status;
} HttpdIapUploadStatusType;

static HttpdIapUploadStatusType HttpdIapUploadStatus = {
    .state         = HTTPD_IAP_UPLOAD_STATE_IDLE,
    .received      = 0u,
    .total         = 0u,
    .error_reason  = HTTPD_IAP_UPLOAD_ERROR_NONE,
    .ingest_status = UPDATE_INGEST_PIPELINE_E_OK,
};

/**
 * @brief URL-encoded key/value POST mode (`/can.cgi` configuration path).
 */
#define HTTPD_POST_MODE_FORM_KV ((u8_t)0u)

/**
 * @brief Binary IAP upload POST mode (`/iap/upload` path).
 */
#define HTTPD_POST_MODE_IAP_BIN ((u8_t)1u)

static void
HttpdPost_SetIapUploadStatus(uint8_t state, uint32_t received, uint32_t total)
{
    HttpdIapUploadStatus.state    = state;
    HttpdIapUploadStatus.received = received;
    HttpdIapUploadStatus.total    = total;
}

static void HttpdPost_SetIapUploadError(uint8_t reason, uint8_t ingest_status)
{
    HttpdIapUploadStatus.error_reason  = reason;
    HttpdIapUploadStatus.ingest_status = ingest_status;
}

static void HttpdPost_ClearSingleIapSession(HttpdPostStateMapType *entry)
{
    if (entry == NULL || entry->conn == NULL)
    {
        return;
    }

    if (entry->ps.mode == HTTPD_POST_MODE_IAP_BIN)
    {
        if (entry->ps.iap_failed == 0u)
        {
            (void)UpdateIngestPipeline_Abort(&entry->ps.iap_pipeline);
        }
        if (current_connection == entry->conn)
        {
            current_connection = NULL;
        }
        if (valid_connection == entry->conn)
        {
            valid_connection = NULL;
        }
        entry->conn = NULL;
        (void)memset(&entry->ps, 0, sizeof(entry->ps));
    }
}

static HttpdPostStateType *map_lookup(void *conn)
{
    for (uint32_t i = 0; i < LWIP_ARRAYSIZE(conn_map); i++)
    {
        if (conn_map[i].conn == conn)
        {
            return &conn_map[i].ps;
        }
    }
    return NULL;
}
static HttpdPostStateType *map_alloc(void *conn, u32_t content_len, u8_t mode)
{
    for (uint32_t i = 0; i < LWIP_ARRAYSIZE(conn_map); i++)
    {
        if (conn_map[i].conn == NULL)
        {
            conn_map[i].conn = conn;
            memset(&conn_map[i].ps, 0, sizeof(conn_map[i].ps));
            conn_map[i].ps.content_len = content_len;
            conn_map[i].ps.mode        = mode;
            return &conn_map[i].ps;
        }
    }
    return NULL;
}
static void map_free(void *conn)
{
    for (uint32_t i = 0; i < LWIP_ARRAYSIZE(conn_map); i++)
    {
        if (conn_map[i].conn == conn)
        {
            conn_map[i].conn = NULL;
            return;
        }
    }
}

static void consume_byte(HttpdPostStateType *ps, char c)
{
    switch (c)
    {
    case '=':
        ps->key[ps->klen] = 0;
        ps->phase         = 1;
        ps->vlen          = 0;
        break;
    case '&':
        ps->val[ps->vlen] = 0;
        httpd_post_cb(ps->key, ps->val);
        ps->klen = ps->vlen = ps->phase = 0;
        break;
    default:
        if (ps->phase == 0 && ps->klen < sizeof(ps->key) - 1)
        {
            ps->key[ps->klen++] = c;
        }
        else if (ps->phase == 1 && ps->vlen < sizeof(ps->val) - 1)
        {
            ps->val[ps->vlen++] = c;
        }
    }
}

#if !LWIP_HTTPD_SUPPORT_POST
#error This needs LWIP_HTTPD_SUPPORT_POST
#else
#define USER_PASS_BUFSIZE 16

err_t httpd_post_begin(
    void *connection,
    const char *uri,
    const char *http_request,
    u16_t http_request_len,
    int content_len,
    char *response_uri,
    u16_t response_uri_len,
    u8_t *post_auto_wnd
)
{
    LWIP_UNUSED_ARG(http_request);
    LWIP_UNUSED_ARG(http_request_len);
    LWIP_UNUSED_ARG(post_auto_wnd);
    HttpdPostStateType *ps;
    u8_t mode;
    const char cfg_page[]    = "/can.cgi";
    const char upload_page[] = "/iap/upload";

    /* Route `/can.cgi` to form key/value parser. */
    if (0 == memcmp(uri, cfg_page, strnlen(cfg_page, sizeof(cfg_page))))
    {
        mode = HTTPD_POST_MODE_FORM_KV;
    }
    /* Route `/iap/upload` to binary IAP stream pipeline. */
    else if (0
             == memcmp(
                 uri,
                 upload_page,
                 strnlen(upload_page, sizeof(upload_page))
             ))
    {
        mode = HTTPD_POST_MODE_IAP_BIN;
    }
    else
    {
        return ERR_VAL;
    }

    if (current_connection != connection)
    {
        current_connection = connection;
        valid_connection   = NULL;

        ps = map_alloc(connection, (u32_t)content_len, mode);
        if (ps == NULL)
        {
            current_connection = NULL;
            return ERR_VAL;
        }

        if (mode == HTTPD_POST_MODE_IAP_BIN)
        {
            UpdateIngestPipelineStatusType ingest_begin_status =
                UPDATE_INGEST_PIPELINE_E_OK;

            /* Binary mode requires a known positive content size and ingest begin. */
            WebInterface_ResetFirmwareVerifyHook();
            HttpdPost_SetIapUploadStatus(
                HTTPD_IAP_UPLOAD_STATE_IN_PROGRESS,
                0u,
                (content_len > 0) ? (uint32_t)content_len : 0u
            );
            HttpdPost_SetIapUploadError(
                HTTPD_IAP_UPLOAD_ERROR_NONE,
                UPDATE_INGEST_PIPELINE_E_OK
            );

            if (content_len > 0)
            {
                ingest_begin_status = UpdateIngestPipeline_Begin(
                    &ps->iap_pipeline,
                    (u32_t)content_len
                );
            }

            if (content_len <= 0
                || ingest_begin_status != UPDATE_INGEST_PIPELINE_E_OK)
            {
                HttpdPost_SetIapUploadStatus(
                    HTTPD_IAP_UPLOAD_STATE_ERROR,
                    0u,
                    (content_len > 0) ? (uint32_t)content_len : 0u
                );
                HttpdPost_SetIapUploadError(
                    HTTPD_IAP_UPLOAD_ERROR_BEGIN,
                    (uint8_t)ingest_begin_status
                );
                map_free(connection);
                current_connection = NULL;
                return ERR_VAL;
            }
        }

        /* default page */
        snprintf(response_uri, response_uri_len, "/404.html");

        valid_connection = connection;

        /* e.g. for large uploads to slow flash over a fast connection, you should
            manually update the rx window. That way, a sender can only send a full
            tcp window at a time. If this is required, set 'post_aut_wnd' to 0.
            We do not need to throttle upload speed here, so: */
#if LWIP_HTTPD_POST_MANUAL_WND
        *post_auto_wnd = 0;
#else
        *post_auto_wnd = 1;
#endif
        return ERR_OK;
    }

    return ERR_VAL;
}

err_t httpd_post_receive_data(void *connection, struct pbuf *p)
{
    err_t ret = ERR_OK;
#if LWIP_HTTPD_POST_MANUAL_WND
    u16_t recved = p->tot_len;
#endif
    HttpdPostStateType *ps;
    char *c;

    LWIP_ASSERT("NULL pbuf", p != NULL);

    if (current_connection == connection)
    {
        ps = map_lookup(connection);

        if (!ps)
        {
            pbuf_free(p);
            return ERR_VAL;
        }

        for (struct pbuf *q = p; q; q = q->next)
        {
            if (ps->mode == HTTPD_POST_MODE_FORM_KV)
            {
                /* Configuration path: decode urlencoded key/value stream. */
                c = q->payload;

                for (u16_t i = 0; i < q->len; i++)
                {
                    consume_byte(ps, c[i]);
                }

                ps->received += q->len;
            }
            else
            {
                UpdateIngestPipelineStatusType ingest_push_status =
                    UPDATE_INGEST_PIPELINE_E_OK;
                uint32_t remaining = 0u;
                u16_t push_len     = q->len;

                if (ps->iap_failed != 0u)
                {
                    continue;
                }

                /* IAP path: forward raw bytes to ingest pipeline. */
                if (ps->received < ps->content_len)
                {
                    remaining = ps->content_len - ps->received;
                }

                if (remaining == 0u)
                {
                    continue;
                }

                if ((uint32_t)push_len > remaining)
                {
                    push_len = (u16_t)remaining;
                }

                ingest_push_status = UpdateIngestPipeline_Push(
                    &ps->iap_pipeline,
                    (const uint8_t *)q->payload,
                    push_len
                );
                if (ingest_push_status != UPDATE_INGEST_PIPELINE_E_OK)
                {
                    ps->iap_failed      = 1u;
                    ps->iap_fail_status = (uint8_t)ingest_push_status;
                    HttpdPost_SetIapUploadStatus(
                        HTTPD_IAP_UPLOAD_STATE_ERROR,
                        ps->received,
                        ps->content_len
                    );
                    HttpdPost_SetIapUploadError(
                        HTTPD_IAP_UPLOAD_ERROR_PUSH,
                        (uint8_t)ingest_push_status
                    );
                    (void)UpdateIngestPipeline_Abort(&ps->iap_pipeline);
                    continue;
                }

                ps->received += push_len;
                HttpdPost_SetIapUploadStatus(
                    HTTPD_IAP_UPLOAD_STATE_IN_PROGRESS,
                    ps->received,
                    ps->content_len
                );
            }
        }

#if LWIP_HTTPD_POST_MANUAL_WND
        httpd_post_data_recved(connection, recved);
#endif
        /* Always keep the POST socket alive and let finished() close flow cleanly. */
        ret = ERR_OK;
    }
    else
    {
        ret = ERR_VAL;
    }

    /* this function must ALWAYS free the pbuf it is passed or it will leak memory */
    pbuf_free(p);

    return ret;
}

#define LWIP_SUPPORTS_POST_RESPONSE 0U

void httpd_post_finished(
    void *connection,
    char *response_uri,
    u16_t response_uri_len
)
{
    const char page[] = "/postredir";
    /* default page */
    snprintf(response_uri, response_uri_len, "/404.html");

    if (current_connection == connection)
    {
        if (valid_connection == connection)
        {
            HttpdPostStateType *ps = map_lookup(connection);

            if (!ps)
            {
                return;
            }

            if (ps->mode == HTTPD_POST_MODE_FORM_KV)
            {
                /* Configuration path: flush trailing key/value pair. */
                /* flush last pair if body ended without '&' */
                if (ps->phase == 1 && ps->vlen)
                {
                    ps->val[ps->vlen] = 0;
                    httpd_post_cb(ps->key, ps->val);
                }
            }
            else if (ps->iap_failed != 0u)
            {
                HttpdPost_SetIapUploadStatus(
                    HTTPD_IAP_UPLOAD_STATE_ERROR,
                    ps->received,
                    ps->content_len
                );
                HttpdPost_SetIapUploadError(
                    HTTPD_IAP_UPLOAD_ERROR_PUSH,
                    ps->iap_fail_status
                );
            }
            /* IAP path: finalize only when full body was forwarded. */
            else if (ps->received != ps->content_len)
            {
                (void)UpdateIngestPipeline_Abort(&ps->iap_pipeline);
                HttpdPost_SetIapUploadStatus(
                    HTTPD_IAP_UPLOAD_STATE_ERROR,
                    ps->received,
                    ps->content_len
                );
                HttpdPost_SetIapUploadError(
                    HTTPD_IAP_UPLOAD_ERROR_INCOMPLETE,
                    UPDATE_INGEST_PIPELINE_E_STATE
                );
            }
            else
            {
                UpdateIngestPipelineStatusType ingest_finish_status =
                    UpdateIngestPipeline_Finish(&ps->iap_pipeline);

                if (ingest_finish_status != UPDATE_INGEST_PIPELINE_E_OK)
                {
                    HttpdPost_SetIapUploadStatus(
                        HTTPD_IAP_UPLOAD_STATE_ERROR,
                        ps->received,
                        ps->content_len
                    );
                    HttpdPost_SetIapUploadError(
                        HTTPD_IAP_UPLOAD_ERROR_FINISH,
                        (uint8_t)ingest_finish_status
                    );
                }
                else
                {
                    WebInterface_RequestFirmwareVerifyHook();
                    HttpdPost_SetIapUploadStatus(
                        HTTPD_IAP_UPLOAD_STATE_READY,
                        ps->content_len,
                        ps->content_len
                    );
                    HttpdPost_SetIapUploadError(
                        HTTPD_IAP_UPLOAD_ERROR_NONE,
                        UPDATE_INGEST_PIPELINE_E_OK
                    );
                }
            }

            map_free(connection);

#if LWIP_SUPPORTS_POST_RESPONSE
            struct http_state *hs = (struct http_state *)connection;
            snprintf(response_uri, response_uri_len, "/can.shtml");
            response_uri[response_uri_len - 1] = '\0';

            hs->post_response_uri = response_uri; // tell lwIP to redirect
#else
            /* send the virtual file defined above */
            snprintf(response_uri, response_uri_len, page);
#endif
        }
        current_connection = NULL;
        valid_connection   = NULL;
    }
}
#endif

void HttpdPost_GetIapUploadState(
    uint8_t *state,
    uint32_t *received,
    uint32_t *total
)
{
    if (state != NULL)
    {
        *state = HttpdIapUploadStatus.state;
    }
    if (received != NULL)
    {
        *received = HttpdIapUploadStatus.received;
    }
    if (total != NULL)
    {
        *total = HttpdIapUploadStatus.total;
    }
}

void HttpdPost_GetIapUploadError(uint8_t *reason, uint8_t *ingest_status)
{
    if (reason != NULL)
    {
        *reason = HttpdIapUploadStatus.error_reason;
    }
    if (ingest_status != NULL)
    {
        *ingest_status = HttpdIapUploadStatus.ingest_status;
    }
}

uint8_t HttpdPost_IsIapUploadReady(void)
{
    return (HttpdIapUploadStatus.state == HTTPD_IAP_UPLOAD_STATE_READY) ? 1u
                                                                        : 0u;
}

void HttpdPost_ClearIapUploadReady(void)
{
    if (HttpdIapUploadStatus.state == HTTPD_IAP_UPLOAD_STATE_READY)
    {
        HttpdPost_SetIapUploadStatus(HTTPD_IAP_UPLOAD_STATE_IDLE, 0u, 0u);
        HttpdPost_SetIapUploadError(
            HTTPD_IAP_UPLOAD_ERROR_NONE,
            UPDATE_INGEST_PIPELINE_E_OK
        );
    }
}

void HttpdPost_ResetIapUploadSession(void)
{
    for (uint32_t i = 0; i < LWIP_ARRAYSIZE(conn_map); i++)
    {
        HttpdPost_ClearSingleIapSession(&conn_map[i]);
    }

    HttpdPost_SetIapUploadStatus(HTTPD_IAP_UPLOAD_STATE_IDLE, 0u, 0u);
    HttpdPost_SetIapUploadError(
        HTTPD_IAP_UPLOAD_ERROR_NONE,
        UPDATE_INGEST_PIPELINE_E_OK
    );
}
