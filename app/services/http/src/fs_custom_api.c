#include "lwip/apps/fs.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "fs_custom_internal.h"
#include "MmcAdapter.h"
#include "instrumentation.h"
#include "build_manifest.h"

#if LWIP_HTTPD_CUSTOM_FILES

#define APP_CARD_STRING                                                        \
    "{\"present\":%s,\"manufacturer_id\":%u,\"oem_id\":\"%c%c\","              \
    "\"product_name\":\"%c%c%c%c%c\",\"revision\":\"%u.%u\","                  \
    "\"serial\":%lu,\"manufactured\":\"%04u-%02u\",\"capacity_mb\":%lu}"

/* Version endpoint payload: the generated manifest fragment plus the effective
 * instrumentation selection, closing the JSON. */
static const char AppVersionData[] = BUILD_MANIFEST_JSON_HEAD
    ",\"component_trace\":\"" COMPONENT_TRACE_NAME "\"}}";

/* CID identity fields are raw register bytes; off-brand cards leave some
 * non-ASCII. Keep the JSON printable. */
static char FsCardAscii(char c)
{
    unsigned char u = (unsigned char)c;
    return (u >= 0x20U && u < 0x7FU) ? (char)u : '?';
}

int FsCustomApi_OpenVersion(struct fs_file *file, const char *name)
{
    (void)name;

    file->data           = AppVersionData;
    file->len            = (int)(sizeof(AppVersionData) - 1U);
    file->index          = 0;
    file->is_custom_file = 0;
    return 1;
}

bool FsCustomApi_RenderCard(
    char *buffer,
    size_t buffer_size,
    size_t *payload_size
)
{
    SdCidRegisterType cid = {0};
    uint32_t sector_count = 0U;
    uint8_t have_cid      = (SD_E_OK == MMCAdapter_GetCardId(&cid));
    int n;

    (void)MMCAdapter_GetSectorCount(&sector_count);

    n = snprintf(
        buffer,
        buffer_size,
        APP_CARD_STRING,
        have_cid ? "true" : "false",
        (unsigned)cid.manufacturerId,
        FsCardAscii(cid.oemId[0]),
        FsCardAscii(cid.oemId[1]),
        FsCardAscii(cid.productName[0]),
        FsCardAscii(cid.productName[1]),
        FsCardAscii(cid.productName[2]),
        FsCardAscii(cid.productName[3]),
        FsCardAscii(cid.productName[4]),
        (unsigned)cid.revisionMajor,
        (unsigned)cid.revisionMinor,
        (unsigned long)cid.serialNumber,
        (unsigned)cid.manufacturingYear,
        (unsigned)cid.manufacturingMonth,
        (unsigned long)((uint64_t)sector_count / 2048U)
    );

    if (n < 0 || (size_t)n >= buffer_size)
    {
        return false;
    }

    *payload_size = (size_t)n;
    return true;
}

#endif
