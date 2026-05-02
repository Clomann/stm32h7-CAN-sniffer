cmake_minimum_required(VERSION 3.16)
project(UnitTests C)

set(FETCHCONTENT_BASE_DIR "${CMAKE_BINARY_DIR}/_deps")  # persistent folder

# Resolve repo root one level up from tests/
get_filename_component(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
message(STATUS "Repo root: ${REPO_ROOT}")

include(${REPO_ROOT}/cmake/mcuboot_signing.cmake)
include(${REPO_ROOT}/cmake/mcuboot_keys.cmake)
mcuboot_keys_init(ROOT_DIR ${REPO_ROOT})

add_compile_options(-O0 -g3 -Wall -Wextra)

# Fetch Unity only if we are building tests
include(FetchContent)
FetchContent_Declare(
    Unity
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(Unity)

# Build mbedTLS for MCUboot unit tests (no programs/tests needed)
set(BOOTLOADER_MBEDTLS_CONFIG "${REPO_ROOT}/bootloader/mbedtls_config.h")
set(MBEDTLS_CONFIG_FILE "${BOOTLOADER_MBEDTLS_CONFIG}"
    CACHE FILEPATH "Mbed TLS config file" FORCE)
set(BOOTLOADER_MBEDTLS_DEFS
    MBEDTLS_PLATFORM_MS_TIME_ALT
    MBEDTLS_CONFIG_FILE=\"${MBEDTLS_CONFIG_FILE}\"
)

set(ENABLE_TESTING OFF CACHE BOOL "Disable mbedTLS tests" FORCE)
set(ENABLE_PROGRAMS OFF CACHE BOOL "Disable mbedTLS programs" FORCE)
add_subdirectory(${REPO_ROOT}/libs/mbedtls ${CMAKE_BINARY_DIR}/mbedtls)

foreach(_mbedtls_target mbedcrypto mbedx509 mbedtls)
    if(TARGET ${_mbedtls_target})
        target_compile_definitions(${_mbedtls_target} PUBLIC ${BOOTLOADER_MBEDTLS_DEFS})
        target_include_directories(${_mbedtls_target} PUBLIC ${REPO_ROOT}/bootloader)
    endif()
endforeach()

enable_testing()
set(CTEST_OUTPUT_ON_FAILURE ON)

add_library(test_flash_memory STATIC
    ${REPO_ROOT}/tests/common/flash_sim/TestFlashMemory.c
)
target_include_directories(test_flash_memory PUBLIC
    ${REPO_ROOT}/tests/common/flash_sim
)

# ===== fdcan =====

add_library(fdcan STATIC
    ${REPO_ROOT}/platform/drivers/fdcan/fdcan_utils.c
)

target_include_directories(fdcan PUBLIC
    ${REPO_ROOT}/platform/drivers
    ${REPO_ROOT}/platform/drivers/fdcan
)

add_executable(test_fdcan
    fdcan/test_fdcan.c
)
target_link_libraries(test_fdcan PRIVATE fdcan unity)

target_include_directories(test_fdcan PRIVATE
    ${unity_SOURCE_DIR}/src
    ${REPO_ROOT}/platform/drivers
    ${REPO_ROOT}/platform/drivers/fdcan
)

# ===== buffer =====
add_executable(test_buffer
    buffer/test_buffer.c
)

target_include_directories(test_buffer PRIVATE
    ${unity_SOURCE_DIR}/src
    ${REPO_ROOT}/platform/drivers
)

target_link_libraries(test_buffer PRIVATE unity)

# ===== SPI =====

set(symbols_SPI                    # Modified
        "STM32H745xx"
        "USE_HAL_DRIVER"
        "CORE_CM7"
        "NO_SYS=1"
        "USE_STM32H7XX_NUCLEO_144_MB1363"
        "FDCAN_MODE_DEFAULT=FDCAN_MODE_NORMAL"
        "UNIT_TEST"
    )

add_library(spi STATIC
    ${REPO_ROOT}/tests/spi/stubs/SpiAbs.c
    ${REPO_ROOT}/platform/drivers/spi/spi.c
    ${REPO_ROOT}/tests/spi/stubs/Spi_Cmds.c
)

target_include_directories(spi PUBLIC
    ${REPO_ROOT}/app/cores/cm7/Inc
    ${REPO_ROOT}/platform/board/stm32h745/cm7/Inc
    ${REPO_ROOT}/platform/Common/Inc
    ${REPO_ROOT}/tests/spi/stubs
    ${REPO_ROOT}/platform/drivers
    ${REPO_ROOT}/platform/drivers/spi
    ${REPO_ROOT}/platform/drivers/comm/CommManager
    ${REPO_ROOT}/platform/drivers/comm/CommHandle
)

target_include_directories(spi SYSTEM PUBLIC
    ${REPO_ROOT}/libs/STM32_HAL/STM32H7/Drivers/STM32H7xx_HAL_Driver/Inc
    ${REPO_ROOT}/libs/STM32_HAL/STM32H7/Drivers/BSP/STM32H7xx_Nucleo
    ${REPO_ROOT}/libs/STM32_HAL/STM32H7/Drivers/CMSIS/Device/ST/STM32H7xx/Include/
    ${REPO_ROOT}/libs/STM32_HAL/STM32H7/Drivers/CMSIS/Core/Include/
)

target_compile_definitions(spi PUBLIC ${symbols_SPI})

add_executable(test_spi
    spi/test_spi.c
)
target_link_libraries(test_spi PRIVATE spi unity)

target_include_directories(test_spi PRIVATE
    ${unity_SOURCE_DIR}/src
    ${REPO_ROOT}/platform/drivers
    ${REPO_ROOT}/platform/drivers/spi
)

# ===== app =====

# Low weight ring buffer (lwrb) fetch
include(FetchContent)

FetchContent_Declare(
    lwrb
    GIT_REPOSITORY https://github.com/MaJerle/lwrb.git
    GIT_TAG        main
)

FetchContent_MakeAvailable(lwrb)

set(symbols_APP
        "STM32H745xx"
        "USE_HAL_DRIVER"
        "CORE_CM7"
        "NO_SYS=1"
        "USE_STM32H7XX_NUCLEO_144_MB1363"
        "FDCAN_MODE_DEFAULT=FDCAN_MODE_NORMAL"
        "UNIT_TEST"
    )

add_library(app STATIC
    ${REPO_ROOT}/tests/app/CanLogManager/test_CanLogManager.c
    ${REPO_ROOT}/tests/app/CanLogManager/stubs/CanAbs.c
    ${REPO_ROOT}/tests/app/CanLogManager/stubs/fs_custom.c
    ${REPO_ROOT}/tests/app/CanLogManager/stubs/fdcan_msg_port.c
    ${REPO_ROOT}/tests/app/CanLogManager/stubs/ff.c
    ${REPO_ROOT}/tests/app/CanLogManager/stubs/FileHandler.c
    ${REPO_ROOT}/tests/app/CanLogManager/stubs/instrumentation.c
    ${REPO_ROOT}/app/services/logging/CanLogBuffer.c
    ${REPO_ROOT}/app/services/logging/CanLogManager.c
    ${REPO_ROOT}/app/services/logging/RuntimeChecks.c
    ${REPO_ROOT}/libs/coreJSON/source/core_json.c
)

target_include_directories(app PUBLIC
    ${REPO_ROOT}/tests/app/CanLogManager/stubs/
    ${REPO_ROOT}/tests/app/CanLogManager
    ${REPO_ROOT}/tests/app
    ${REPO_ROOT}/app/services/config
    ${REPO_ROOT}/app/services/logging
    ${REPO_ROOT}/app/services/storage
    ${REPO_ROOT}/platform/Common/Inc
    ${REPO_ROOT}/app/cores/cm7/Inc
    ${REPO_ROOT}/platform/board/stm32h745/cm7/Inc
    ${REPO_ROOT}/libs/ff15a/source
    ${REPO_ROOT}/platform/drivers
    ${REPO_ROOT}/platform/drivers/CanAbs
    ${REPO_ROOT}/platform/drivers/comm/CommManager
    ${REPO_ROOT}/platform/drivers/comm/CommHandle
    ${REPO_ROOT}/platform/ports
    ${REPO_ROOT}/app/services/http/inc
    ${REPO_ROOT}/libs/coreJSON/source/include
)

target_link_libraries(app PRIVATE unity lwrb)

target_compile_definitions(app PUBLIC ${symbols_APP})

add_executable(test_app
    app/test_app.c
)
target_link_libraries(test_app PRIVATE app unity)

target_include_directories(test_app PRIVATE
    ${unity_SOURCE_DIR}/src
)

# ===== iap =====
add_executable(test_iap
    ${REPO_ROOT}/tests/app/InAppProgramming/test_iap.c
    ${REPO_ROOT}/tests/app/InAppProgramming/stubs/RamFlashSim.c
    ${REPO_ROOT}/app/services/iap/IapIngestAdapter.c
    ${REPO_ROOT}/app/services/iap/IapWriter.c
    ${REPO_ROOT}/app/services/update/UpdateIngestRegistry.c
)

target_include_directories(test_iap PRIVATE
    ${unity_SOURCE_DIR}/src
    ${REPO_ROOT}/tests/app/InAppProgramming/stubs
    ${REPO_ROOT}/tests/common/flash_sim
    ${REPO_ROOT}/app/services/iap
    ${REPO_ROOT}/app/services/update
    ${REPO_ROOT}/platform/drivers/flash
)

target_link_libraries(test_iap PRIVATE unity test_flash_memory)

# ===== bootloader =====
set(mcuboot_bootutil_SRCS
    ${REPO_ROOT}/libs/mcuboot/boot/bootutil/src/boot_record.c
    ${REPO_ROOT}/libs/mcuboot/boot/bootutil/src/bootutil_area.c
    ${REPO_ROOT}/libs/mcuboot/boot/bootutil/src/bootutil_find_key.c
    ${REPO_ROOT}/libs/mcuboot/boot/bootutil/src/bootutil_img_hash.c
    ${REPO_ROOT}/libs/mcuboot/boot/bootutil/src/bootutil_img_security_cnt.c
    ${REPO_ROOT}/libs/mcuboot/boot/bootutil/src/bootutil_loader.c
    ${REPO_ROOT}/libs/mcuboot/boot/bootutil/src/bootutil_misc.c
    ${REPO_ROOT}/libs/mcuboot/boot/bootutil/src/bootutil_public.c
    ${REPO_ROOT}/libs/mcuboot/boot/bootutil/src/caps.c
    ${REPO_ROOT}/libs/mcuboot/boot/bootutil/src/ed25519_psa.c
    ${REPO_ROOT}/libs/mcuboot/boot/bootutil/src/encrypted.c
    ${REPO_ROOT}/libs/mcuboot/boot/bootutil/src/fault_injection_hardening_delay_rng_mbedtls.c
    ${REPO_ROOT}/libs/mcuboot/boot/bootutil/src/fault_injection_hardening.c
    ${REPO_ROOT}/libs/mcuboot/boot/bootutil/src/image_ecdsa.c
    ${REPO_ROOT}/libs/mcuboot/boot/bootutil/src/image_ed25519.c
    ${REPO_ROOT}/libs/mcuboot/boot/bootutil/src/image_rsa.c
    ${REPO_ROOT}/libs/mcuboot/boot/bootutil/src/image_validate.c
    ${REPO_ROOT}/libs/mcuboot/boot/bootutil/src/loader.c
    ${REPO_ROOT}/libs/mcuboot/boot/bootutil/src/ram_load.c
    ${REPO_ROOT}/libs/mcuboot/boot/bootutil/src/swap_misc.c
    ${REPO_ROOT}/libs/mcuboot/boot/bootutil/src/swap_move.c
    ${REPO_ROOT}/libs/mcuboot/boot/bootutil/src/swap_offset.c
    ${REPO_ROOT}/libs/mcuboot/boot/bootutil/src/swap_scratch.c
    ${REPO_ROOT}/libs/mcuboot/boot/bootutil/src/tlv.c
)

add_library(mcuboot_bootutil STATIC
    ${mcuboot_bootutil_SRCS}
)

target_include_directories(mcuboot_bootutil PUBLIC
    ${REPO_ROOT}/tests/bootloader
    ${REPO_ROOT}/bootloader
    ${REPO_ROOT}/bootloader/flash_map_backend
    ${REPO_ROOT}/libs/mcuboot/boot/bootutil/include
    ${REPO_ROOT}/libs/mcuboot/boot/bootutil/include/bootutil
    ${REPO_ROOT}/libs/mcuboot/boot/bootutil/include/bootutil/crypto
    ${REPO_ROOT}/libs/mcuboot/boot/bootutil/src
)

target_link_libraries(mcuboot_bootutil PUBLIC mbedcrypto)
set(BOOTLOADER_TEST_COMPILE_DEFS ${BOOTLOADER_MBEDTLS_DEFS})
mcuboot_keys_append_compile_defs(BOOTLOADER_TEST_COMPILE_DEFS)
target_compile_definitions(mcuboot_bootutil PUBLIC ${BOOTLOADER_TEST_COMPILE_DEFS})

add_executable(test_bootloader
    ${REPO_ROOT}/tests/bootloader/test_bootloader.c
    ${REPO_ROOT}/tests/bootloader/logging_stub.c
    ${REPO_ROOT}/tests/bootloader/boot_platform_stub.c
    ${REPO_ROOT}/tests/bootloader/bootloader_test_stubs.c
    ${REPO_ROOT}/tests/bootloader/hal_flash_stubs.c
    ${REPO_ROOT}/tests/bootloader/flash_write_wrapper.c
    ${REPO_ROOT}/bootloader/hal/bootloader_flash.c
    ${REPO_ROOT}/platform/drivers/flash/flash.c
    ${REPO_ROOT}/bootloader/bootloader.c
    ${REPO_ROOT}/bootloader/mcuboot_port.c
)

set(BOOTLOADER_TEST_ELF "${REPO_ROOT}/_bin/Release/SPI_FullDuplex_ComDMA_CM7_app.elf")
set(BOOTLOADER_TEST_BIN "${CMAKE_BINARY_DIR}/SPI_FullDuplex_ComDMA_CM7_app.bin")
set(BOOTLOADER_TEST_SIGNED_BIN "${CMAKE_BINARY_DIR}/SPI_FullDuplex_ComDMA_CM7_app-signed.bin")
set(BOOTLOADER_TEST_SIGNED_ENCRYPTED_BIN "${CMAKE_BINARY_DIR}/SPI_FullDuplex_ComDMA_CM7_app-signed-encrypted.bin")
set(BOOTLOADER_TEST_MARKERS_H "${CMAKE_BINARY_DIR}/image_test_markers.h")
set(BOOTLOADER_TEST_IMGTOOL "${REPO_ROOT}/libs/mcuboot/scripts/imgtool.py")
set(BOOTLOADER_TEST_LOG_PATH "${REPO_ROOT}/tests/build/bootloader_test.log")
set(BOOTLOADER_TEST_GEN_KEYS_DIR "${CMAKE_BINARY_DIR}/generated/keys")
set(BOOTLOADER_TEST_SIGN_KEY_C "${BOOTLOADER_TEST_GEN_KEYS_DIR}/autogen_sign_pub.c")
set(BOOTLOADER_TEST_ENC_KEY_C "${BOOTLOADER_TEST_GEN_KEYS_DIR}/autogen_enc_priv.c")

find_program(BOOTLOADER_TEST_PYTHON_EXE NAMES python3 python py)
mcuboot_keys_generate_sources(
    GENERATE_DIR ${BOOTLOADER_TEST_GEN_KEYS_DIR}
    SIGN_OUT ${BOOTLOADER_TEST_SIGN_KEY_C}
    ENC_OUT ${BOOTLOADER_TEST_ENC_KEY_C}
    PYTHON ${BOOTLOADER_TEST_PYTHON_EXE}
    IMGTOOL ${BOOTLOADER_TEST_IMGTOOL}
)

set(_bootloader_key_sources
    ${REPO_ROOT}/bootloader/mcuboot_config/keys.c
)
mcuboot_keys_append_generated_sources(
    _bootloader_key_sources
    ${BOOTLOADER_TEST_SIGN_KEY_C}
    ${BOOTLOADER_TEST_ENC_KEY_C}
)
target_sources(test_bootloader PRIVATE ${_bootloader_key_sources})

add_custom_command(
    OUTPUT ${BOOTLOADER_TEST_MARKERS_H}
    COMMAND ${CMAKE_COMMAND}
            -DELF=${BOOTLOADER_TEST_ELF}
            -DOUT=${BOOTLOADER_TEST_MARKERS_H}
            -P ${REPO_ROOT}/tests/cmake/gen_image_markers.cmake
    DEPENDS ${BOOTLOADER_TEST_ELF}
    COMMENT "Generating image markers header from ${BOOTLOADER_TEST_ELF}"
    VERBATIM
)

add_custom_command(
    OUTPUT ${BOOTLOADER_TEST_BIN}
    COMMAND ${CMAKE_OBJCOPY} -O binary ${BOOTLOADER_TEST_ELF} ${BOOTLOADER_TEST_BIN}
    DEPENDS ${BOOTLOADER_TEST_ELF}
    COMMENT "Generating bootloader test app bin from ${BOOTLOADER_TEST_ELF}"
    VERBATIM
)

set(_bootloader_test_sign_common_args
    INPUT ${BOOTLOADER_TEST_BIN}
    IMGTOOL ${BOOTLOADER_TEST_IMGTOOL}
    HEADER_SIZE 0x400
    ALIGN 4
    SLOT_SIZE 0x60000
    VERSION 0.4.0
    OVERWRITE_ONLY
    REQUIRE_CLICK
    DEPENDS ${BOOTLOADER_TEST_BIN} ${BOOTLOADER_TEST_IMGTOOL}
    COMMENT "Signing bootloader test image ${BOOTLOADER_TEST_BIN}"
)

set(_bootloader_test_sign_plain_args
    ${_bootloader_test_sign_common_args}
    OUTPUT ${BOOTLOADER_TEST_SIGNED_BIN}
)
mcuboot_keys_append_signing_key_arg(_bootloader_test_sign_plain_args)
mcuboot_add_imgtool_sign_command(${_bootloader_test_sign_plain_args})

set(_bootloader_test_sign_encrypted_args
    ${_bootloader_test_sign_common_args}
    OUTPUT ${BOOTLOADER_TEST_SIGNED_ENCRYPTED_BIN}
)
mcuboot_keys_append_signing_key_arg(_bootloader_test_sign_encrypted_args)
mcuboot_keys_append_encryption_pubkey_arg(_bootloader_test_sign_encrypted_args)
mcuboot_add_imgtool_sign_command(${_bootloader_test_sign_encrypted_args})

add_custom_target(
    bootloader_test_image_artifacts
    DEPENDS
        ${BOOTLOADER_TEST_MARKERS_H}
        ${BOOTLOADER_TEST_SIGNED_BIN}
        ${BOOTLOADER_TEST_SIGNED_ENCRYPTED_BIN}
)

add_dependencies(test_bootloader bootloader_test_image_artifacts)

target_include_directories(test_bootloader PRIVATE
    ${unity_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}
    ${REPO_ROOT}/tests/common/flash_sim
    ${REPO_ROOT}/tests/bootloader
    ${REPO_ROOT}/bootloader/mcuboot_config
    ${REPO_ROOT}/bootloader
    ${REPO_ROOT}/bootloader/flash_map_backend
)

target_link_libraries(test_bootloader PRIVATE mcuboot_bootutil unity test_flash_memory)
target_compile_definitions(
    test_bootloader
    PRIVATE
        ${BOOTLOADER_TEST_COMPILE_DEFS}
        IMAGE_TEST_SIGNED_BIN_PATH="${BOOTLOADER_TEST_SIGNED_BIN}"
        IMAGE_TEST_SIGNED_ENCRYPTED_BIN_PATH="${BOOTLOADER_TEST_SIGNED_ENCRYPTED_BIN}"
        IMAGE_TEST_LOG_PATH="${BOOTLOADER_TEST_LOG_PATH}"
)

set_source_files_properties(
    ${REPO_ROOT}/platform/drivers/flash/flash.c
    PROPERTIES
    COMPILE_DEFINITIONS "Flash_Write=Flash_Write_impl"
    COMPILE_OPTIONS "-include;${REPO_ROOT}/tests/bootloader/memcpy_override.h"
)

# --- Register tests with CTest ---
add_test(NAME FDCAN_Test COMMAND test_fdcan)
add_test(NAME BUFFER_Test COMMAND test_buffer)
add_test(NAME SPI_Test COMMAND test_spi)
add_test(NAME APP_Test COMMAND test_app)
add_test(NAME IAP_Test COMMAND test_iap)
add_test(NAME BOOTLOADER_Test COMMAND test_bootloader)

# --- Create a custom "test_all" build target ---
add_custom_target(tests DEPENDS 
    test_fdcan 
    test_buffer
    test_spi
    test_app
    test_iap
    test_bootloader
)
