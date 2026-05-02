#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "httpd_post.h"
#include "UpdateIngestPipeline.h"

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

/**
 * @brief URL-encoded key/value POST mode (`/can.cgi` configuration path).
 */
#define HTTPD_POST_MODE_FORM_KV ((u8_t)0u)

/**
 * @brief Binary IAP upload POST mode (`/iap/upload` path).
 */
#define HTTPD_POST_MODE_IAP_BIN ((u8_t)1u)

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

static void *current_connection;
static void *valid_connection;

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
            /* Binary mode requires a known positive content size and ingest begin. */
            if (content_len <= 0
                || UpdateIngestPipeline_Begin(
                       &ps->iap_pipeline,
                       (u32_t)content_len
                   ) != UPDATE_INGEST_PIPELINE_E_OK)
            {
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
                /* IAP path: forward raw bytes to ingest pipeline. */
                if (UpdateIngestPipeline_Push(
                        &ps->iap_pipeline,
                        (const uint8_t *)q->payload,
                        q->len
                    )
                    != UPDATE_INGEST_PIPELINE_E_OK)
                {
                    (void)UpdateIngestPipeline_Abort(&ps->iap_pipeline);
                    ret = ERR_VAL;
                    break;
                }
            }
        }

#if LWIP_HTTPD_POST_MANUAL_WND
        httpd_post_data_recved(connection, recved);
#endif
        if (ret != ERR_VAL)
        {
            /* not returning ERR_OK aborts the connection, so return ERR_OK unless the
                connection is unknown */
            ret = ERR_OK;
        }
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
            /* IAP path: finalize upload stream after full body received. */
            else if (UpdateIngestPipeline_Finish(&ps->iap_pipeline)
                     != UPDATE_INGEST_PIPELINE_E_OK)
            {
                map_free(connection);
                current_connection = NULL;
                valid_connection   = NULL;
                return;
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
