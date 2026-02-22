#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "flash_map_backend/flash_map_backend.h"
#include "sysflash/sysflash.h"

#define TEST_FLASH_DEVICE_ID 0
#define TEST_AREA_SIZE       (16u * 1024u)
#define TEST_SECTOR_SIZE     4096u

static struct flash_area test_area;

uint8_t flash_area_get_id(const struct flash_area *fa)
{
    return fa->fa_id;
}

uint8_t flash_area_get_device_id(const struct flash_area *fa)
{
    return fa->fa_device_id;
}

uint32_t flash_area_get_off(const struct flash_area *fa)
{
    return fa->fa_off;
}

uint32_t flash_area_get_size(const struct flash_area *fa)
{
    return fa->fa_size;
}

int flash_area_open(uint8_t id, const struct flash_area **area_outp)
{
    test_area.fa_id        = id;
    test_area.fa_device_id = TEST_FLASH_DEVICE_ID;
    test_area.fa_off       = 0;
    test_area.fa_size      = TEST_AREA_SIZE;

    if (area_outp != NULL)
    {
        *area_outp = &test_area;
    }
    return 0;
}

void flash_area_close(const struct flash_area *fa)
{
    (void)fa;
}

int flash_area_read(
    const struct flash_area *fa,
    uint32_t off,
    void *dst,
    uint32_t len
)
{
    (void)fa;
    (void)off;
    if (dst == NULL)
    {
        return -1;
    }
    memset(dst, 0xff, len);
    return 0;
}

int flash_area_write(
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

int flash_area_erase(const struct flash_area *fa, uint32_t off, uint32_t len)
{
    (void)fa;
    (void)off;
    (void)len;
    return 0;
}

uint32_t flash_area_align(const struct flash_area *area)
{
    (void)area;
    return 1;
}

uint8_t flash_area_erased_val(const struct flash_area *area)
{
    (void)area;
    return 0xff;
}

int flash_area_get_sectors(
    int fa_id,
    uint32_t *count,
    struct flash_sector *sectors
)
{
    (void)fa_id;
    if (count == NULL)
    {
        return -1;
    }

    if (sectors == NULL)
    {
        *count = 1;
        return 0;
    }
    if (*count < 1)
    {
        return -1;
    }

    sectors[0].fs_off  = 0;
    sectors[0].fs_size = TEST_SECTOR_SIZE;
    *count             = 1;
    return 0;
}

int flash_area_to_sectors(int fa_id, int *count, struct flash_area *sectors)
{
    (void)fa_id;
    if (count == NULL)
    {
        return -1;
    }

    if (sectors == NULL)
    {
        *count = 1;
        return 0;
    }
    if (*count < 1)
    {
        return -1;
    }

    sectors[0].fa_id        = test_area.fa_id;
    sectors[0].fa_device_id = test_area.fa_device_id;
    sectors[0].fa_off       = test_area.fa_off;
    sectors[0].fa_size      = TEST_SECTOR_SIZE;
    *count                  = 1;
    return 0;
}

int flash_area_get_sector(
    const struct flash_area *fa,
    off_t off,
    struct flash_sector *fs
)
{
    (void)off;
    if (fa == NULL || fs == NULL)
    {
        return -1;
    }

    fs->fs_off  = 0;
    fs->fs_size = TEST_SECTOR_SIZE;
    return 0;
}

int flash_area_id_from_multi_image_slot(int image_index, int slot)
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

int flash_area_id_from_image_slot(int slot)
{
    return flash_area_id_from_multi_image_slot(0, slot);
}

int flash_area_id_to_multi_image_slot(int image_index, int area_id)
{
    if (area_id == FLASH_AREA_IMAGE_PRIMARY(image_index))
    {
        return 0;
    }
    if (area_id == FLASH_AREA_IMAGE_SECONDARY(image_index))
    {
        return 1;
    }
    return -1;
}

int flash_area_id_from_image_offset(uint32_t offset)
{
    (void)offset;
    return -1;
}
