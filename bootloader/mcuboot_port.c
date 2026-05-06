#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "bootloader.h"
#include "flash.h"
#include "mcuboot_config/mcuboot_config.h"
#include "bootutil/bootutil.h"
#include "bootutil/bootutil_public.h"
#include "bootutil/image.h"
#include "flash_map_backend/flash_map_backend.h"
#include "mbedtls/platform_time.h"
#include "sysflash/sysflash.h"
#include "mcuboot_config/mcuboot_logging.h"
#include "FwUpdateHandoff.h"

#if defined(__GNUC__)
#define WEAK __attribute__((weak))
#else
#define WEAK
#endif

#define ARRAY_SIZE(arr) sizeof(arr) / sizeof(arr[0])

#define FLASH_H745_SECTOR_SIZE (128U * 1024U)
#define FLASH_SECTOR_SIZE      FLASH_H745_SECTOR_SIZE

#if defined(__arm__) || defined(__ARM_ARCH)
#define MCUBOOT_FLASH_LAYOUT_FROM_LINKER 1
#else
#define MCUBOOT_FLASH_LAYOUT_FROM_LINKER 0
#endif

#if MCUBOOT_FLASH_LAYOUT_FROM_LINKER
extern uint8_t __boot_start__;
extern uint8_t __boot_end__;
extern uint8_t __boot_size__;
extern uint8_t __boot_off__;
extern uint8_t __flash_base__;
extern uint8_t __app_primary_start__;
extern uint8_t __app_primary_size__;
extern uint8_t __app_primary_off__;
extern uint8_t __app_secondary_start__;
extern uint8_t __app_secondary_size__;
extern uint8_t __app_secondary_off__;
extern uint8_t __scratch_start__;
extern uint8_t __scratch_end__;
extern uint8_t __scratch_size__;

#define BOOTLOADER_START_ADDRESS ((uint32_t)(uintptr_t) & __boot_start__)
#define BOOTLOADER_SIZE          ((uint32_t)(uintptr_t) & __boot_size__)
#define BOOTLOADER_OFFSET        ((uint32_t)(uintptr_t) & __boot_off__)
#define FLASH_DEVICE_BASE_ADDR   ((uint32_t)(uintptr_t) & __flash_base__)
#define APPLICATION_PRIMARY_START_ADDRESS                                      \
    ((uint32_t)(uintptr_t) & __app_primary_start__)
#define APPLICATION_SIZE           ((uint32_t)(uintptr_t) & __app_primary_size__)
#define APPLICATION_PRIMARY_OFFSET ((uint32_t)(uintptr_t) & __app_primary_off__)
#define APPLICATION_SECONDARY_START_ADDRESS                                    \
    ((uint32_t)(uintptr_t) & __app_secondary_start__)
#define APPLICATION_SECONDARY_SIZE                                             \
    ((uint32_t)(uintptr_t) & __app_secondary_size__)
#define APPLICATION_SECONDARY_OFFSET                                           \
    ((uint32_t)(uintptr_t) & __app_secondary_off__)
#define SCRATCH_START_ADDRESS ((uint32_t)(uintptr_t) & __scratch_start__)
#define SCRATCH_SIZE          ((uint32_t)(uintptr_t) & __scratch_size__)
#else
#define BOOTLOADER_START_ADDRESS          0x0
#define BOOTLOADER_SIZE                   (1U * FLASH_SECTOR_SIZE)
#define BOOTLOADER_OFFSET                 BOOTLOADER_START_ADDRESS
#define APPLICATION_PRIMARY_START_ADDRESS (1U * FLASH_SECTOR_SIZE)
#define APPLICATION_SIZE                  (3U * FLASH_SECTOR_SIZE)
#define APPLICATION_PRIMARY_OFFSET        APPLICATION_PRIMARY_START_ADDRESS
#define APPLICATION_SECONDARY_START_ADDRESS                                    \
    (APPLICATION_PRIMARY_START_ADDRESS + APPLICATION_SIZE)
#define APPLICATION_SECONDARY_SIZE   APPLICATION_SIZE
#define APPLICATION_SECONDARY_OFFSET APPLICATION_SECONDARY_START_ADDRESS
#define SCRATCH_START_ADDRESS                                                  \
    (APPLICATION_SECONDARY_START_ADDRESS + APPLICATION_SIZE)
#define SCRATCH_SIZE           (1U * FLASH_SECTOR_SIZE)
#define FLASH_DEVICE_BASE_ADDR 0u
#endif

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#define STATIC_ASSERT(cond, msg) _Static_assert((cond), #msg)
#else
#define STATIC_ASSERT(cond, msg)                                               \
    typedef char static_assertion_##msg[(cond) ? 1 : -1]
#endif

#if !MCUBOOT_FLASH_LAYOUT_FROM_LINKER
STATIC_ASSERT(
    (BOOTLOADER_START_ADDRESS % FLASH_SECTOR_SIZE) == 0,
    bootloader_start_must_align_to_flash_sector_size
);
STATIC_ASSERT(
    (APPLICATION_PRIMARY_START_ADDRESS % FLASH_SECTOR_SIZE) == 0,
    primary_start_must_align_to_flash_sector_size
);
STATIC_ASSERT(
    (APPLICATION_SECONDARY_START_ADDRESS % FLASH_SECTOR_SIZE) == 0,
    secondary_start_must_align_to_flash_sector_size
);
STATIC_ASSERT(
    (BOOTLOADER_SIZE % FLASH_SECTOR_SIZE) == 0,
    bootloader_size_must_be_multiple_of_flash_sector_size
);
STATIC_ASSERT(
    (APPLICATION_SIZE % FLASH_SECTOR_SIZE) == 0,
    application_size_must_be_multiple_of_flash_sector_size
);
STATIC_ASSERT(
    (SCRATCH_START_ADDRESS % FLASH_SECTOR_SIZE) == 0,
    scratch_start_must_align_to_flash_sector_size
);
STATIC_ASSERT(
    (SCRATCH_SIZE % FLASH_SECTOR_SIZE) == 0,
    scratch_size_must_be_multiple_of_flash_sector_size
);
#endif

static const struct flash_area bootloader = {
    .fa_id        = FLASH_AREA_BOOTLOADER,
    .fa_device_id = FLASH_DEVICE_INTERNAL_FLASH,
    .fa_off       = BOOTLOADER_OFFSET,
    .fa_size      = BOOTLOADER_SIZE,
};

static const struct flash_area primary_img0 = {
    .fa_id        = FLASH_AREA_IMAGE_PRIMARY(0),
    .fa_device_id = FLASH_DEVICE_INTERNAL_FLASH,
    .fa_off       = APPLICATION_PRIMARY_OFFSET,
    .fa_size      = APPLICATION_SIZE,
};

static const struct flash_area secondary_img0 = {
    .fa_id        = FLASH_AREA_IMAGE_SECONDARY(0),
    .fa_device_id = FLASH_DEVICE_INTERNAL_FLASH,
    .fa_off       = APPLICATION_SECONDARY_OFFSET,
    .fa_size      = APPLICATION_SECONDARY_SIZE,
};

#if !defined(MCUBOOT_OVERWRITE_ONLY)
static const struct flash_area scratch = {
    .fa_id        = FLASH_AREA_IMAGE_SCRATCH,
    .fa_device_id = FLASH_DEVICE_INTERNAL_FLASH,
    .fa_off       = SCRATCH_START_ADDRESS,
    .fa_size      = SCRATCH_SIZE,
};
#endif

static const struct flash_area *s_flash_areas[] = {
    &bootloader,
    &primary_img0,
    &secondary_img0,
#if !defined(MCUBOOT_OVERWRITE_ONLY)
    &scratch,
#endif
};

struct boot_loader_state;
struct boot_status;
struct image_header;
struct boot_swap_state;

uint8_t FwUpdateHandoff_GetMarkerAddress_Hook(uint32_t *address)
{
    if (NULL == address)
    {
        return FW_UPDATE_HANDOFF_E_PARAM;
    }

    *address = (uint32_t)(SCRATCH_START_ADDRESS + SCRATCH_SIZE);

    return FW_UPDATE_HANDOFF_E_OK;
}

static void Error_Handler(int code)
{
    (void)code;
}

static const struct flash_area *prv_lookup_flash_area(uint8_t id)
{
    for (size_t i = 0; i < ARRAY_SIZE(s_flash_areas); i++)
    {
        const struct flash_area *area = s_flash_areas[i];
        if (id == area->fa_id)
        {
            return area;
        }
    }
    return NULL;
}

WEAK int flash_area_open(uint8_t id, const struct flash_area **area_outp)
{
    const struct flash_area *area = prv_lookup_flash_area(id);
    if (area_outp != NULL)
    {
        *area_outp = area;
    }
    return area ? 0 : -1;
}

WEAK void flash_area_close(const struct flash_area *area)
{
    (void)area;

    Error_Handler(0);

}

WEAK int flash_area_read(
    const struct flash_area *fa,
    uint32_t off,
    void *dst,
    uint32_t len
)
{
    if (fa == NULL || dst == NULL)
    {
        return -1;
    }
    if (fa->fa_device_id != FLASH_DEVICE_INTERNAL_FLASH)
    {
        return -1;
    }

    if (off > fa->fa_size || len > (fa->fa_size - off))
    {
        MCUBOOT_LOG_ERR(
            "%s: Out of Bounds (0x%x vs 0x%x)",
            __func__,
            (int)(off + len),
            (int)fa->fa_size
        );
        return -1;
    }

    const uint32_t addr = FLASH_DEVICE_BASE_ADDR + fa->fa_off + off;
    if (boot_internal_flash_read(addr, dst, len) != 0)
    {
        MCUBOOT_LOG_ERR(
            "%s: Read Failed (0x%x vs 0x%x)",
            __func__,
            (int)(off + len),
            (int)fa->fa_size
        );
        return -1;
    }

    return 0;
}

WEAK int flash_area_write(
    const struct flash_area *fa,
    uint32_t off,
    const void *src,
    uint32_t len
)
{
    if (fa == NULL || src == NULL)
    {
        return -1;
    }
    if (fa->fa_device_id != FLASH_DEVICE_INTERNAL_FLASH)
    {
        return -1;
    }

    if (off > fa->fa_size || len > (fa->fa_size - off))
    {
        MCUBOOT_LOG_ERR(
            "%s: Out of Bounds (0x%x vs 0x%x)",
            __func__,
            (int)(off + len),
            (int)fa->fa_size
        );
        return -1;
    }

    const uint32_t addr = FLASH_DEVICE_BASE_ADDR + fa->fa_off + off;
    MCUBOOT_LOG_DBG(
        "%s: Addr: 0x%08x Length: %d",
        __func__,
        (int)addr,
        (int)len
    );
    boot_internal_flash_write(addr, src, len);

#if VALIDATE_PROGRAM_OP
    if (memcmp((void *)addr, src, len) != 0)
    {
        MCUBOOT_LOG_ERR("%s: Program Failed", __func__);
        assert(0);
    }
#endif

    return 0;
}

WEAK int
flash_area_erase(const struct flash_area *fa, uint32_t off, uint32_t len)
{
    if (fa == NULL)
    {
        return -1;
    }
    if (fa->fa_device_id != FLASH_DEVICE_INTERNAL_FLASH)
    {
        return -1;
    }

    if ((len % FLASH_SECTOR_SIZE) != 0 || (off % FLASH_SECTOR_SIZE) != 0)
    {
        MCUBOOT_LOG_ERR(
            "%s: Not aligned on sector Offset: 0x%x Length: 0x%x",
            __func__,
            (int)off,
            (int)len
        );
        return -1;
    }

    if (off > UINT32_MAX - len || off + len > fa->fa_size
        || (off % FLASH_SECTOR_SIZE) != 0)
    {
        MCUBOOT_LOG_ERR(
            "%s: Eraseing outside of sector: 0x%x Length: 0x%x",
            __func__,
            (int)off,
            (int)len
        );
        return -1;
    }

    const uint32_t start_addr = FLASH_DEVICE_BASE_ADDR + fa->fa_off + off;
    MCUBOOT_LOG_DBG(
        "%s: Addr: 0x%08x Length: %d",
        __func__,
        (int)start_addr,
        (int)len
    );

    for (size_t i = 0; i < len; i += FLASH_SECTOR_SIZE)
    {
        const uint32_t addr = start_addr + i;
        boot_internal_flash_erase_sector(addr);
    }

#if VALIDATE_PROGRAM_OP
    for (size_t i = 0; i < len; i++)
    {
        uint8_t *val = (void *)(start_addr + i);
        if (*val != 0xff)
        {
            MCUBOOT_LOG_ERR("%s: Erase at 0x%x Failed", __func__, (int)val);
            assert(0);
        }
    }
#endif

    return 0;
}

WEAK uint32_t flash_area_align(const struct flash_area *area)
{
    FlashStatusType res = 0;
    FlashInfoType info;

    (void)area;
    // the smallest unit a flash write can occur along.
    // Note: Image trailers will be scaled by this size
    res = Flash_GetInfo(&info);

    if (FLASH_E_OK != res)
    {
        return -1;
    }
    else
    {
        return info.write_alignment;
    }
}

WEAK uint8_t flash_area_erased_val(const struct flash_area *area)
{
    (void)area;

    Error_Handler(0);

    // the value a byte reads when erased on storage.
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
    const struct flash_area *fa = prv_lookup_flash_area(fa_id);
    if (fa == NULL || count == NULL)
    {
        return -1;
    }
    if (fa->fa_device_id != FLASH_DEVICE_INTERNAL_FLASH)
    {
        return -1;
    }

    const size_t sector_size = FLASH_SECTOR_SIZE;
    if (sector_size == 0)
    {
        return -1;
    }

    const uint32_t total_count = (uint32_t)(fa->fa_size / sector_size);
    if (sectors == NULL)
    {
        *count = total_count;
        return 0;
    }
    if (*count < total_count)
    {
        return -1;
    }

    for (uint32_t i = 0; i < total_count; i++)
    {
        // Note: Offset here is relative to flash area, not device
        sectors[i].fs_off  = (uint32_t)(i * sector_size);
        sectors[i].fs_size = (uint32_t)sector_size;
    }

    *count = total_count;
    return 0;
}

WEAK int
flash_area_to_sectors(int fa_id, int *count, struct flash_area *sectors)
{
    (void)fa_id;
    (void)count;
    (void)sectors;

    Error_Handler(0);

    return -1;
}

WEAK int flash_area_get_sector(
    const struct flash_area *fa,
    off_t off,
    struct flash_sector *fs
)
{
    int res                    = -1;
    FlashSectorInfoType sector = {0};
    uint32_t sector_abs_addr   = 0;

    if (fa == NULL || fs == NULL || off < 0)
    {
        return -1;
    }

    if ((uint32_t)off >= fa->fa_size)
    {
        return -1;
    }

    sector_abs_addr = FLASH_DEVICE_BASE_ADDR + fa->fa_off + off;

    if (FLASH_E_OK == Flash_GetSectorByAddr(sector_abs_addr, &sector))
    {
        // Note: Offset here is relative to flash area, not device
        fs->fs_off  = sector.off - fa->fa_off;
        fs->fs_size = sector.size;

        res = 0;
    }
    else
    {
        res = -1;
    }

    return res;
}

WEAK int flash_area_id_from_multi_image_slot(int image_index, int slot)
{
    switch (slot)
    {
    case 0:
        return FLASH_AREA_IMAGE_PRIMARY(image_index);
    case 1:
        return FLASH_AREA_IMAGE_SECONDARY(image_index);
    }

    MCUBOOT_LOG_ERR(
        "Unexpected Request: image_index=%d, slot=%d",
        image_index,
        slot
    );
    return -1; /* flash_area_open will fail on that */
}

WEAK int flash_area_id_from_image_slot(int slot)
{
    return flash_area_id_from_multi_image_slot(0, slot);
}

WEAK int flash_area_id_to_multi_image_slot(int image_index, int area_id)
{
    if (area_id == FLASH_AREA_IMAGE_PRIMARY(image_index))
    {
        return BOOT_SLOT_PRIMARY;
    }
    if (area_id == FLASH_AREA_IMAGE_SECONDARY(image_index))
    {
        return BOOT_SLOT_SECONDARY;
    }
    return -1;
}

WEAK int flash_area_id_from_image_offset(uint32_t offset)
{
    (void)offset;

    Error_Handler(0);

    return -1;
}

WEAK void example_assert_handler(const char *file, int line)
{
    (void)file;
    (void)line;
    EXAMPLE_LOG("ASSERT: File: %s Line: %d", file, line);
    __builtin_trap();
}

WEAK mbedtls_ms_time_t mbedtls_ms_time(void)
{
    Error_Handler(0);

    return 0;
}

void boot_activate_pending_if_image_present(void)
{
    int res;
    const struct flash_area *fap;
    uint32_t magic = 0u;
    uint8_t apply_requested;

    apply_requested = FwUpdateHandoff_IsApplyRequested();
    if (apply_requested == 0u)
    {
        return;
    }

    (void)FwUpdateHandoff_ClearApplyRequest();

    if (flash_area_open(FLASH_AREA_IMAGE_SECONDARY(0), &fap) != 0)
    {
        return;
    }

    res = flash_area_read(fap, 0u, &magic, sizeof(magic));
    flash_area_close(fap);

    if (magic == IMAGE_MAGIC)
    {
        res = boot_set_pending_multi(0, 1);
    }

    (void)res;
}
