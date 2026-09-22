#pragma once

/* Private to the fs_custom*.c endpoint files; not part of the http service API
 * (that is inc/fs_custom.h). */

#include <stdbool.h>
#include <stddef.h>

#define JSON_RESPONSE_HEADER                                                   \
    "HTTP/1.1 200 OK\r\n"                                                      \
    "Content-Type: application/json\r\n"                                       \
    "Connection: close\r\n"                                                    \
    "\r\n"
#define JSON_RESPONSE_HEADER_SIZE (sizeof(JSON_RESPONSE_HEADER) - 1U)

/* Split a 64-bit counter into two %lu halves (JSON *_hi / *_lo fields). */
#define U64_HI(value) ((unsigned long)(((value) >> 32) & 0xFFFFFFFFULL))
#define U64_LO(value) ((unsigned long)((value) & 0xFFFFFFFFULL))

/* Append printf-formatted text at buffer[*offset] and advance *offset.
 * Returns false (offset unchanged) if the text does not fit completely. */
bool FsCustom_AppendJson(
    char *buffer,
    size_t buffer_size,
    size_t *offset,
    const char *format,
    ...
) __attribute__((format(printf, 4, 5)));

struct fs_file;

/* Endpoint handlers, one section per fs_custom_<scope>.c file. Open handlers
 * set up the lwIP file themselves; render handlers write their payload into
 * buffer and report its length (see FsCustom_OpenRendered). */

/* fs_custom_api.c: /api/... */
int FsCustomApi_OpenVersion(struct fs_file *file, const char *name);
bool FsCustomApi_RenderCard(
    char *buffer,
    size_t buffer_size,
    size_t *payload_size
);
