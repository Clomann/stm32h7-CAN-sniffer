# Bootloader sources and includes (no target created here yet)

if (CORE STREQUAL "CM7")
list(APPEND boot_sources_SRCS
    ${PROJ_PATH}/bootloader/startup/startup_stm32h745zitx_boot.s
    ${PROJ_PATH}/bootloader/bootloader.c
    ${PROJ_PATH}/bootloader/hal/bootloader_flash.c
    ${PROJ_PATH}/bootloader/hal/bootloader_uart.c
    ${PROJ_PATH}/bootloader/hal/bootloader_hsm.c
    ${PROJ_PATH}/bootloader/mcuboot_port.c
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/boot_record.c
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/bootutil_area.c
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/bootutil_area.h
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/bootutil_find_key.c
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/bootutil_img_hash.c
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/bootutil_img_security_cnt.c
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/bootutil_loader.c
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/bootutil_loader.h
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/bootutil_misc.c
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/bootutil_misc.h
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/bootutil_priv.h
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/bootutil_public.c
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/caps.c
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/ed25519_psa.c
    # ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/encrypted_psa.c
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/encrypted.c
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/fault_injection_hardening_delay_rng_mbedtls.c
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/fault_injection_hardening.c
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/image_ecdsa.c
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/image_ed25519.c
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/image_rsa.c
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/image_validate.c
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/loader.c
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/ram_load.c
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/swap_misc.c
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/swap_move.c
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/swap_offset.c
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/swap_priv.h
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/swap_scratch.c
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/src/tlv.c
)

list(APPEND boot_include_path_DIRS
    ${PROJ_PATH}/bootloader
    ${PROJ_PATH}/bootloader/flash_map_backend
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/include
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/include/bootutil
    ${PROJ_PATH}/libs/mcuboot/boot/bootutil/include/bootutil/crypto
)
endif()

set(boot_sources_SRCS "${boot_sources_SRCS}" PARENT_SCOPE)
set(boot_include_path_DIRS "${boot_include_path_DIRS}" PARENT_SCOPE)
