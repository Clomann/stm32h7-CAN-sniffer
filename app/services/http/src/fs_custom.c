#include "lwip/apps/fs.h"
#include "lwip/def.h"

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "fs_custom.h"
#include "fs_custom_internal.h"

static const char redirect_reply[] = "HTTP/1.1 303 See Other\r\n"
                                     "Location: /can.shtml\r\n"
                                     "Connection: close\r\n"
                                     "Content-Length: 0\r\n"
                                     "\r\n";

#if LWIP_HTTPD_CUSTOM_FILES

#if LWIP_HTTPD_DYNAMIC_FILE_READ != 1
#warning "LWIP_HTTPD_DYNAMIC_FILE_READ is NOT enabled!"
#endif

#define CANLOG_MAX_META_DATA_SIZE 128U
#define CANLOG_MAX_STATUS_SIZE    5120U
#define CANLOG_MAX_DEBUG_SIZE     4096U
#define CANLOG_MAX_CONFIG_SIZE    96U
#define IAP_STATUS_MAX_SIZE       640U
#define CANLOG_FILE_PATH          "/logs/CAN.LOG"
#define CANLOG_META_DATA_PATH     "/logs/meta"
#define CANLOG_CONFIG_PATH        "/logs/config"
#define CANLOG_STATUS_PATH        "/logger/status"
#define CANLOG_DEBUG_PATH         "/logger/debug"
#define ERRORRECORDER_PATH        "/logger/errors"
#define CANLOG_POST_REDIRECT_PATH "/postredir"
#define IAP_STATUS_PATH           "/iap/status"
#define IAP_PREPARE_PATH          "/iap/prepare"
#define IAP_APPLY_PATH            "/iap/apply"
#define APP_VERSION_PATH          "/api/version"
#define APP_CARD_PATH             "/api/card"

/* WARNING: Not thread-safe. MetaData/StatusData shared across requests.
 * Assumes single-threaded or serialized HTTP request processing. */
static char MetaData[CANLOG_MAX_META_DATA_SIZE];
static char StatusData[CANLOG_MAX_STATUS_SIZE];
static char DebugData[JSON_RESPONSE_HEADER_SIZE + CANLOG_MAX_DEBUG_SIZE];
static char ConfigData[CANLOG_MAX_CONFIG_SIZE];
static char IapStatusData[IAP_STATUS_MAX_SIZE];
static char IapPrepareResultData[96U];
static char IapApplyResultData[96U];

static int FsCustom_OpenPostRedirect(struct fs_file *file, const char *name)
{
    (void)name;

    file->data = redirect_reply;
    file->len  = sizeof(redirect_reply) - 1;
    file->flags =
        FS_FILE_FLAGS_HEADER_INCLUDED | FS_FILE_FLAGS_HEADER_PERSISTENT;
    file->index = 0;
#if LWIP_HTTPD_DYNAMIC_HEADERS
    // file->http_header_included = 1;
#endif
#if LWIP_HTTPD_CUSTOM_FILES
    file->is_custom_file = 1;
#endif
    return 1;
}

typedef int (*FsCustomOpenFn)(struct fs_file *file, const char *name);
typedef bool (*FsCustomRenderFn)(
    char *buffer,
    size_t buffer_size,
    size_t *payload_size
);

typedef struct
{
    const char *path;
    size_t path_length;
    FsCustomOpenFn open; /* stream / const data; NULL for rendered routes */
    FsCustomRenderFn render; /* fills buffer; used when open == NULL */
    char *buffer;
    size_t buffer_size;
    uint8_t flags; /* FS_FILE_FLAGS_HEADER_INCLUDED: JSON header is prepended */
} FsCustomRouteType;

#define FS_CUSTOM_ROUTE(path, handler)                                         \
    {                                                                          \
        (path), sizeof(path) - 1U, (handler), NULL, NULL, 0U, 0U               \
    }
#define FS_CUSTOM_RENDER_ROUTE(path, render, buffer, flags)                    \
    {                                                                          \
        (path), sizeof(path) - 1U, NULL, (render), (buffer), sizeof(buffer),   \
            (flags)                                                            \
    }

/* Prefix match, first hit wins: keep the order. */
static const FsCustomRouteType FsCustomRoutes[] = {
    FS_CUSTOM_ROUTE(CANLOG_FILE_PATH, FsCustomLogs_OpenFile),
    FS_CUSTOM_RENDER_ROUTE(
        CANLOG_META_DATA_PATH,
        FsCustomLogs_RenderMeta,
        MetaData,
        0U
    ),
    FS_CUSTOM_RENDER_ROUTE(
        CANLOG_CONFIG_PATH,
        FsCustomLogs_RenderConfig,
        ConfigData,
        0U
    ),
    FS_CUSTOM_RENDER_ROUTE(
        CANLOG_STATUS_PATH,
        FsCustomLogger_RenderStatus,
        StatusData,
        0U
    ),
    FS_CUSTOM_RENDER_ROUTE(
        CANLOG_DEBUG_PATH,
        FsCustomLogger_RenderDebug,
        DebugData,
        FS_FILE_FLAGS_HEADER_INCLUDED | FS_FILE_FLAGS_HEADER_PERSISTENT
    ),
    FS_CUSTOM_RENDER_ROUTE(
        ERRORRECORDER_PATH,
        FsCustomLogger_RenderErrors,
        DebugData,
        FS_FILE_FLAGS_HEADER_INCLUDED | FS_FILE_FLAGS_HEADER_PERSISTENT
    ),
    FS_CUSTOM_RENDER_ROUTE(
        IAP_STATUS_PATH,
        FsCustomIap_RenderStatus,
        IapStatusData,
        0U
    ),
    FS_CUSTOM_ROUTE(APP_VERSION_PATH, FsCustomApi_OpenVersion),
    FS_CUSTOM_RENDER_ROUTE(
        APP_CARD_PATH,
        FsCustomApi_RenderCard,
        StatusData,
        0U
    ),
    FS_CUSTOM_RENDER_ROUTE(
        IAP_PREPARE_PATH,
        FsCustomIap_RenderPrepare,
        IapPrepareResultData,
        0U
    ),
    FS_CUSTOM_RENDER_ROUTE(
        IAP_APPLY_PATH,
        FsCustomIap_RenderApply,
        IapApplyResultData,
        0U
    ),
    FS_CUSTOM_ROUTE(CANLOG_POST_REDIRECT_PATH, FsCustom_OpenPostRedirect),
};

/* Render into the route's buffer and publish it. For routes with
 * FS_FILE_FLAGS_HEADER_INCLUDED the JSON response header goes first and the
 * payload follows it. */
static int
FsCustom_OpenRendered(struct fs_file *file, const FsCustomRouteType *route)
{
    size_t HeaderSize  = 0U;
    size_t PayloadSize = 0U;

    if (0U != (route->flags & FS_FILE_FLAGS_HEADER_INCLUDED))
    {
        HeaderSize = JSON_RESPONSE_HEADER_SIZE;
    }

    if ((route->buffer_size <= HeaderSize)
        || !route->render(
            &route->buffer[HeaderSize],
            route->buffer_size - HeaderSize,
            &PayloadSize
        ))
    {
        return 0;
    }

    memcpy(route->buffer, JSON_RESPONSE_HEADER, HeaderSize);

    file->data           = route->buffer;
    file->len            = (int)(HeaderSize + PayloadSize);
    file->index          = 0;
    file->flags          = route->flags;
    file->is_custom_file = 0; /* httpd sends static buffer */
    return 1;
}

int fs_open_custom(struct fs_file *file, const char *name)
{
    memset(file, 0, sizeof(*file));

    for (uint32_t i = 0; i < LWIP_ARRAYSIZE(FsCustomRoutes); i++)
    {
        const FsCustomRouteType *route = &FsCustomRoutes[i];

        if (0 == strncmp(name, route->path, route->path_length))
        {
            return (NULL != route->open) ? route->open(file, name)
                                         : FsCustom_OpenRendered(file, route);
        }
    }

    return 0; // Fallback to default file system
}

int fs_read_custom(struct fs_file *file, char *buffer, int count)
{
    return FsCustomLogs_Read(file, buffer, count);
}

void fs_close_custom(struct fs_file *file)
{
    FsCustomLogs_Close(file);
}

#endif
