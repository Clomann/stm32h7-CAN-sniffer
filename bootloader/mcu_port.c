#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "bootutil/bootutil.h"
#include "flash_map_backend/flash_map_backend.h"
#include "mbedtls/platform_time.h"
#include "sysflash/sysflash.h"

#if defined(__GNUC__)
#define WEAK __attribute__((weak))
#else
#define WEAK
#endif

struct boot_loader_state;
struct boot_status;
struct image_header;
struct boot_swap_state;

static struct flash_area dummy_area;

WEAK int flash_area_open(uint8_t id, const struct flash_area **area_outp)
{
    dummy_area.fa_id        = id;
    dummy_area.fa_device_id = 0;
    dummy_area.fa_off       = 0;
    dummy_area.fa_size      = 0;

    if (area_outp != NULL)
    {
        *area_outp = &dummy_area;
    }
    return 0;
}

WEAK void flash_area_close(const struct flash_area *area)
{
    (void)area;
}

WEAK int flash_area_read(
    const struct flash_area *fa,
    uint32_t off,
    void *dst,
    uint32_t len
)
{
    (void)fa;
    (void)off;
    (void)dst;
    (void)len;
    return -1;
}

WEAK int flash_area_write(
    const struct flash_area *fa,
    uint32_t off,
    const void *src,
    uint32_t len
)
{
    (void)fa;
    (void)off;
    (void)src;
    (void)len;
    return 0;
}

WEAK int
flash_area_erase(const struct flash_area *fa, uint32_t off, uint32_t len)
{
    (void)fa;
    (void)off;
    (void)len;
    return 0;
}

WEAK uint32_t flash_area_align(const struct flash_area *area)
{
    (void)area;
    return 1;
}

WEAK uint8_t flash_area_erased_val(const struct flash_area *area)
{
    (void)area;
    return 0xff;
}

WEAK uint8_t flash_area_get_device_id(const struct flash_area *fa)
{
    return fa ? fa->fa_device_id : 0u;
}

WEAK uint8_t flash_area_get_id(const struct flash_area *fa)
{
    return fa ? fa->fa_id : 0u;
}

WEAK uint32_t flash_area_get_size(const struct flash_area *fa)
{
    return fa ? fa->fa_size : 0u;
}

WEAK uint32_t flash_area_get_off(const struct flash_area *fa)
{
    return fa ? fa->fa_off : 0u;
}

WEAK int
flash_area_get_sectors(int fa_id, uint32_t *count, struct flash_sector *sectors)
{
    (void)fa_id;
    (void)count;
    (void)sectors;
    return -1;
}

WEAK int
flash_area_to_sectors(int fa_id, int *count, struct flash_area *sectors)
{
    (void)fa_id;
    (void)count;
    (void)sectors;
    return -1;
}

WEAK int flash_area_get_sector(
    const struct flash_area *fa,
    off_t off,
    struct flash_sector *fs
)
{
    (void)fa;
    (void)off;
    (void)fs;
    return -1;
}

WEAK int flash_area_id_from_multi_image_slot(int image_index, int slot)
{
    if (slot == 0)
    {
        return FLASH_AREA_IMAGE_PRIMARY(image_index);
    }
    if (slot == 1)
    {
        return FLASH_AREA_IMAGE_SECONDARY(image_index);
    }
    return -1;
}

WEAK int flash_area_id_from_image_slot(int slot)
{
    return flash_area_id_from_multi_image_slot(0, slot);
}

WEAK int flash_area_id_to_multi_image_slot(int image_index, int area_id)
{
    (void)image_index;
    (void)area_id;
    return -1;
}

WEAK int flash_area_id_from_image_offset(uint32_t offset)
{
    (void)offset;
    return -1;
}

WEAK void example_assert_handler(const char *file, int line)
{
    (void)file;
    (void)line;
}

WEAK int boot_read_image_header(
    struct boot_loader_state *state,
    int slot,
    struct image_header *out_hdr,
    struct boot_status *bs
)
{
    (void)state;
    (void)slot;
    (void)out_hdr;
    (void)bs;
    return -1;
}

WEAK size_t boot_img_sector_size(
    const struct boot_loader_state *state,
    size_t slot,
    size_t sector
)
{
    (void)state;
    (void)slot;
    (void)sector;
    return 0u;
}

WEAK void boot_swap_sectors(
    int idx,
    uint32_t sz,
    struct boot_loader_state *state,
    struct boot_status *bs,
    const struct flash_area *fap_pri,
    const struct flash_area *fap_sec
)
{
    (void)idx;
    (void)sz;
    (void)state;
    (void)bs;
    (void)fap_pri;
    (void)fap_sec;
}

#if defined(MCUBOOT_SWAP_USING_OFFSET) && defined(MCUBOOT_ENC_IMAGES)
WEAK int boot_copy_region(
    struct boot_loader_state *state,
    const struct flash_area *fap_src,
    const struct flash_area *fap_dst,
    uint32_t off_src,
    uint32_t off_dst,
    uint32_t sz,
    uint32_t sector_off
)
{
    (void)state;
    (void)fap_src;
    (void)fap_dst;
    (void)off_src;
    (void)off_dst;
    (void)sz;
    (void)sector_off;
    return 0;
}
#else
WEAK int boot_copy_region(
    struct boot_loader_state *state,
    const struct flash_area *fap_src,
    const struct flash_area *fap_dst,
    uint32_t off_src,
    uint32_t off_dst,
    uint32_t sz
)
{
    (void)state;
    (void)fap_src;
    (void)fap_dst;
    (void)off_src;
    (void)off_dst;
    (void)sz;
    return 0;
}
#endif

WEAK int
boot_write_status(const struct boot_loader_state *state, struct boot_status *bs)
{
    (void)state;
    (void)bs;
    return 0;
}

WEAK int boot_write_trailer(
    const struct flash_area *fap,
    uint32_t off,
    const uint8_t *inbuf,
    uint8_t inlen
)
{
    (void)fap;
    (void)off;
    (void)inbuf;
    (void)inlen;
    return 0;
}

WEAK int swap_set_image_ok(uint8_t image_index)
{
    (void)image_index;
    return 0;
}

WEAK int swap_set_copy_done(uint8_t image_index)
{
    (void)image_index;
    return 0;
}

WEAK int
boot_read_swap_state_by_id(int flash_area_id, struct boot_swap_state *state)
{
    (void)flash_area_id;
    if (state != NULL)
    {
        memset(state, 0, sizeof(*state));
    }
    return 0;
}

WEAK void fill_rsp(struct boot_loader_state *state, struct boot_rsp *rsp)
{
    (void)state;
    if (rsp != NULL)
    {
        rsp->br_flash_dev_id = 0;
        rsp->br_image_off    = 0;
        rsp->br_hdr          = NULL;
    }
}

WEAK mbedtls_ms_time_t mbedtls_ms_time(void)
{
    return 0;
}
