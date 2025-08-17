cmake_minimum_required(VERSION 3.16)
project(UnitTests C)

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
    ${REPO_ROOT}/drivers/fdcan/fdcan_utils.c
)

target_include_directories(fdcan PUBLIC
    ${REPO_ROOT}/drivers
    ${REPO_ROOT}/drivers/fdcan
)

add_executable(test_fdcan
    fdcan/test_fdcan.c
)
target_link_libraries(test_fdcan PRIVATE fdcan unity)

target_include_directories(test_fdcan PRIVATE
    ${unity_SOURCE_DIR}/src
    ${REPO_ROOT}/drivers
    ${REPO_ROOT}/drivers/fdcan
)

# ===== buffer =====
add_executable(test_buffer
    buffer/test_buffer.c
)

target_include_directories(test_buffer PRIVATE
    ${unity_SOURCE_DIR}/src
    ${REPO_ROOT}/drivers
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
    ${REPO_ROOT}/drivers/spi/spi.c
    ${REPO_ROOT}/tests/spi/stubs/Spi_Cmds.c
)

target_include_directories(spi PUBLIC
    ${REPO_ROOT}/CM7/Inc
    ${REPO_ROOT}/Common/Inc
    ${REPO_ROOT}/tests/spi/stubs
    ${REPO_ROOT}/drivers
    ${REPO_ROOT}/drivers/spi
    ${REPO_ROOT}/drivers/comm/CommManager
    ${REPO_ROOT}/drivers/comm/CommHandle
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
    ${REPO_ROOT}/drivers
    ${REPO_ROOT}/drivers/spi
)

# --- Register tests with CTest ---
add_test(NAME FDCAN_Test COMMAND test_fdcan)
add_test(NAME BUFFER_Test COMMAND test_buffer)
add_test(NAME SPI_Test COMMAND test_spi)

# --- Create a custom "test_all" build target ---
add_custom_target(tests DEPENDS 
    test_fdcan 
    test_buffer
    test_spi
)
