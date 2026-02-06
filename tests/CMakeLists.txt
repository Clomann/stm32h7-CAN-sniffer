cmake_minimum_required(VERSION 3.16)
project(UnitTests C)

set(FETCHCONTENT_BASE_DIR "${CMAKE_BINARY_DIR}/_deps")  # persistent folder

# Resolve repo root one level up from tests/
set(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")
message(STATUS "Repo root: ${REPO_ROOT}")

add_compile_options(-O0 -g3 -Wall -Wextra)

# Fetch Unity only if we are building tests
include(FetchContent)
FetchContent_Declare(
    Unity
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(Unity)

enable_testing()
set(CTEST_OUTPUT_ON_FAILURE ON)

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
    ${REPO_ROOT}/CM7/Inc
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
    ${REPO_ROOT}/CM7/Inc
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

# --- Register tests with CTest ---
add_test(NAME FDCAN_Test COMMAND test_fdcan)
add_test(NAME BUFFER_Test COMMAND test_buffer)
add_test(NAME SPI_Test COMMAND test_spi)
add_test(NAME APP_Test COMMAND test_app)

# --- Create a custom "test_all" build target ---
add_custom_target(tests DEPENDS 
    test_fdcan 
    test_buffer
    test_spi
    test_app
)
