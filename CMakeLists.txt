cmake_minimum_required(VERSION 3.22)

set(FETCHCONTENT_BASE_DIR "${CMAKE_BINARY_DIR}/_deps")

# Let callers override this: -DCPU_PARAMETERS=";-mthumb;-mcpu=cortex-m7;..."
set(CPU_PARAMETERS "" CACHE STRING "Target CPU/ABI flags (list)")
set(DUAL_CORE "" CACHE STRING "Configure two target cofiguration (true/false).")
set(BUILD_TARGET "app" CACHE STRING "Build target: app, bootloader, or both")
set_property(CACHE BUILD_TARGET PROPERTY STRINGS app bootloader both)
set(IS_APP_BUILD FALSE)
set(IS_BOOTLOADER_BUILD FALSE)
set(IS_BOTH_BUILD FALSE)
if(BUILD_TARGET STREQUAL "app")
    set(IS_APP_BUILD TRUE)
elseif(BUILD_TARGET STREQUAL "bootloader")
    set(IS_BOOTLOADER_BUILD TRUE)
elseif(BUILD_TARGET STREQUAL "both")
    set(IS_APP_BUILD TRUE)
    set(IS_BOOTLOADER_BUILD TRUE)
    set(IS_BOTH_BUILD TRUE)
else()
    message(FATAL_ERROR "Unknown BUILD_TARGET='${BUILD_TARGET}'. Use app, bootloader, or both.")
endif()

set(IS_DUAL_CORE FALSE)
if(DUAL_CORE_LOWER STREQUAL "true" OR DUAL_CORE_LOWER STREQUAL "on" OR DUAL_CORE_LOWER STREQUAL "1")
    set(IS_DUAL_CORE TRUE)
endif()

if(NOT DEFINED CORE)
  message(FATAL_ERROR "Unknown CORE='${CORE}'. Use CM7 or CM4.")
endif()

# Setup compiler settings
set(CMAKE_C_STANDARD                11)
set(CMAKE_C_STANDARD_REQUIRED       ON)
set(CMAKE_C_EXTENSIONS              ON)
set(CMAKE_CXX_STANDARD              20)
set(CMAKE_CXX_STANDARD_REQUIRED     ON)
set(CMAKE_CXX_EXTENSIONS            ON)
set(PROJ_PATH                       ${CMAKE_CURRENT_SOURCE_DIR})
set(STM32_HAL_PATH                  ${CMAKE_CURRENT_SOURCE_DIR}/libs/STM32_HAL/STM32H7)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJ_PATH}/_bin/${CMAKE_BUILD_TYPE})

set(LWIP_HEAP_SIZE 0x1ffe8)

message("Build type: "              ${CMAKE_BUILD_TYPE})

#
# Core project settings
#
project(SPI_FullDuplex_ComDMA_${CORE})          # Modified
enable_language(C CXX ASM)
include(CMakeParseArguments)

#
# Core MCU flags, CPU, instruction set and FPU setup
# Needs to be set properly for your MCU
#
if(CORE STREQUAL "CM7" AND NOT CPU_PARAMETERS)
    set(CPU_PARAMETERS
        -mthumb

        # This needs attention to properly set for used MCU
        -mcpu=cortex-m7                 # Modified
        -mfpu=fpv5-d16                  # Modified
        -mfloat-abi=hard                # Modified
    )

    #
    # Symbols definition
    #
    set(symbols_SYMB                    # Modified
        "STM32H745xx"
        "USE_HAL_DRIVER"
        "CORE_CM7"
        "NO_SYS=1"
        "USE_STM32H7XX_NUCLEO_144_MB1363"
        "FDCAN_MODE_DEFAULT=FDCAN_MODE_NORMAL"
        "TARGET_STM32H7"
        "LWIP_HEAP_SIZE=${LWIP_HEAP_SIZE}"
        "MBEDTLS_PLATFORM_MS_TIME_ALT"
        "MBEDTLS_CONFIG_FILE=\"mbedtls_config.h\""
    )

elseif(CORE STREQUAL "CM4" AND NOT CPU_PARAMETERS)
    set(CPU_PARAMETERS
        -mthumb

        # This needs attention to properly set for used MCU
        -mcpu=cortex-m4                 # Modified
        -mfpu=fpv5-d16                  # Modified
        -mfloat-abi=hard                # Modified
    )

    #
    # Symbols definition
    #
    set(symbols_SYMB                    # Modified
        "STM32H745xx"
        "USE_HAL_DRIVER"
        "CORE_CM4"
        "NO_SYS=1"
        "USE_STM32H7XX_NUCLEO_144_MB1363"
        "FDCAN_MODE_DEFAULT=FDCAN_MODE_NORMAL"
        "TARGET_STM32H7"
        "MBEDTLS_PLATFORM_MS_TIME_ALT"
        "MBEDTLS_CONFIG_FILE=\"mbedtls_config.h\""
    )
else()
  message(FATAL_ERROR "Unknown CORE='${CORE}'. Use CM7 or CM4.")
endif()

# push flags to all targets
add_compile_options(${CPU_PARAMETERS})
add_link_options(${CPU_PARAMETERS})

# thrid party libs

# Low weight ring buffer (lwrb) fetch
include(FetchContent)

FetchContent_Declare(
    lwrb
    GIT_REPOSITORY https://github.com/MaJerle/lwrb.git
    GIT_TAG        main
)

FetchContent_MakeAvailable(lwrb)

# FreeRTOS LTS fetch
add_library(freertos_config INTERFACE)
target_include_directories(freertos_config INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/middleware/freertos)

FetchContent_Declare(
    freertos_kernel
    GIT_REPOSITORY https://github.com/FreeRTOS/FreeRTOS-LTS.git
    GIT_TAG        202406-LTS
    SOURCE_SUBDIR  FreeRTOS/FreeRTOS-Kernel
    GIT_SUBMODULES "FreeRTOS/FreeRTOS-Kernel"               
)

set(FREERTOS_PORT GCC_ARM_CM7 CACHE STRING "" FORCE)
set(FREERTOS_HEAP 4            CACHE STRING "" FORCE) 

FetchContent_MakeAvailable(freertos_kernel)

# Set linker script
if(CORE STREQUAL "CM7")
    set(linker_script_app               ${PROJ_PATH}/STM32CubeIDE/CM7/STM32H745ZITX_FLASH.ld)
    set(linker_script_boot              ${PROJ_PATH}/STM32CubeIDE/CM7/STM32H745ZITX_FLASH_boot.ld)
elseif(CORE STREQUAL "CM4")
    set(linker_script_app               ${PROJ_PATH}/STM32CubeIDE/CM4/STM32H745ZITX_FLASH.ld)
    set(linker_script_boot              ${linker_script_app})
endif()
set(EXECUTABLE                      ${CMAKE_PROJECT_NAME})

if(CORE STREQUAL "CM7")
    #
    # List of source files to compile
    #
    set(sources_SRCS                    # Modified
        ${PROJ_PATH}/platform/Common/Src/system_stm32h7xx.c
    )

    #
    # Include directories
    #
    set(include_path_DIRS               # Modified
        ${PROJ_PATH}/platform/board/stm32h745/cm7/Inc
        ${PROJ_PATH}/platform/board/stm32h745/cm7/Src
        ${PROJ_PATH}/platform/Common/Inc
    )

    set(common_sources_SRCS ${sources_SRCS})
    set(common_include_path_DIRS ${include_path_DIRS})

    if(IS_APP_BUILD)
        include(${PROJ_PATH}/cmake/app_deps.cmake)
    endif()

    if(IS_BOOTLOADER_BUILD)
        include(${PROJ_PATH}/cmake/boot_deps.cmake)
    endif()

elseif(CORE STREQUAL "CM4")
    #
    # List of source files to compile
    #
    set(sources_SRCS                    # Modified
        ${PROJ_PATH}/platform/board/stm32h745/cm4/Src/main.c
        ${PROJ_PATH}/platform/board/stm32h745/cm4/Src/stm32h7xx_hal_msp.c
        ${PROJ_PATH}/platform/board/stm32h745/cm4/Src/stm32h7xx_it.c

        ${PROJ_PATH}/platform/Common/Src/system_stm32h7xx.c

        ${PROJ_PATH}/STM32CubeIDE/CM4/Example/User/CM4/syscalls.c
        ${PROJ_PATH}/STM32CubeIDE/CM4/Example/User/CM4/sysmem.c
        ${PROJ_PATH}/STM32CubeIDE/CM4/Example/User/startup/startup_stm32h745zitx.s
    )

    #
    # Include directories
    #
    set(include_path_DIRS               # Modified
        ${PROJ_PATH}/platform/board/stm32h745/cm4/Inc
        ${PROJ_PATH}/platform/Common/Inc
    )
    
    add_subdirectory(libs)

    if(IS_APP_BUILD)
        set(app_sources_SRCS ${sources_SRCS})
        set(app_include_path_DIRS ${include_path_DIRS})
        set(app_link_libs lwrb freertos_kernel freertos_config third_party_interface)
    endif()
endif()

function(configure_firmware_target target_name)
    set(options)
    set(oneValueArgs LINKER_SCRIPT USE_PRINTF_FLOAT)
    set(multiValueArgs SOURCES INCLUDES LIBS COMPILE_DEFS)
    cmake_parse_arguments(FW "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT FW_SOURCES)
        message(FATAL_ERROR "No sources provided for target '${target_name}'.")
    endif()

    add_executable(${target_name} ${FW_SOURCES})

    if(FW_LIBS)
        target_link_libraries(${target_name} PRIVATE ${FW_LIBS})
    endif()

    # Include paths
    target_include_directories(${target_name} PRIVATE ${FW_INCLUDES})

    # Project symbols
    target_compile_definitions(${target_name} PRIVATE ${symbols_SYMB})

    if(FW_COMPILE_DEFS)
        target_compile_definitions(${target_name} PRIVATE ${FW_COMPILE_DEFS})
    endif()

    # Compiler options
    target_compile_options(${target_name} PRIVATE
        ${CPU_PARAMETERS}
        -Wall
        -Wextra
        -Wpedantic
        -Werror=implicit-function-declaration
        # Full debug configuration
        $<$<CONFIG:Release>:-O1;-g3;-ggdb>
        $<$<CONFIG:Debug>:-Og;-g3;-ggdb>
        -fstack-usage
        -ffunction-sections
        -fdata-sections
    )

    # Linker options
    target_link_options(${target_name} PRIVATE
        -T${FW_LINKER_SCRIPT}
        ${CPU_PARAMETERS}
        -Wl,--defsym=LWIP_HEAP_SIZE=${LWIP_HEAP_SIZE}
        -Wl,-Map=$<TARGET_FILE_DIR:${target_name}>/${target_name}.map
        --specs=nosys.specs
        -u _printf_float
        -Wl,--start-group
        -lc
        -lm
        -lstdc++
        -lsupc++
        -Wl,--end-group
        -Wl,--print-memory-usage
        -Wl,--error-unresolved-symbols
        -Wl,--fatal-warnings
        -Wl,--trace
        -Wl,--no-undefined
        # -Wl,--gc-sections,--print-gc-sections
    )



    # Execute post-build to print size
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${target_name}>
    )

    # Convert output to hex and binary
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${target_name}>
        COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${target_name}> $<TARGET_FILE_DIR:${target_name}>/${target_name}.hex
    )

    # Convert to bin file -> add conditional check?
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${target_name}>
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${target_name}> $<TARGET_FILE_DIR:${target_name}>/${target_name}.bin
    )
endfunction()

if(IS_BOTH_BUILD)
    set(APP_EXECUTABLE ${EXECUTABLE}_app)
    set(BOOT_EXECUTABLE ${EXECUTABLE}_boot)

    configure_firmware_target(${APP_EXECUTABLE}
        LINKER_SCRIPT ${linker_script_app}
        SOURCES ${app_sources_SRCS}
        INCLUDES ${app_include_path_DIRS}
        LIBS ${app_link_libs}
        USE_PRINTF_FLOAT ${FW_USE_PRINTF_FLOAT}
    )

    configure_firmware_target(${BOOT_EXECUTABLE}
        LINKER_SCRIPT ${linker_script_boot}
        SOURCES ${boot_target_sources_SRCS}
        INCLUDES ${boot_target_include_path_DIRS}
        COMPILE_DEFS ${boot_target_compile_defs}
        LIBS ${boot_link_libs}
        USE_PRINTF_FLOAT OFF
    )
elseif(IS_APP_BUILD)
    set(APP_EXECUTABLE ${EXECUTABLE}_app)

    configure_firmware_target(${APP_EXECUTABLE}
        LINKER_SCRIPT ${linker_script_app}
        SOURCES ${app_sources_SRCS}
        INCLUDES ${app_include_path_DIRS}
        LIBS ${app_link_libs}
        USE_PRINTF_FLOAT ${FW_USE_PRINTF_FLOAT}
    )
elseif(IS_BOOTLOADER_BUILD)
    set(BOOT_EXECUTABLE ${EXECUTABLE}_boot)

    configure_firmware_target(${BOOT_EXECUTABLE}
        LINKER_SCRIPT ${linker_script_boot}
        SOURCES ${boot_target_sources_SRCS}
        INCLUDES ${boot_target_include_path_DIRS}
        COMPILE_DEFS ${boot_target_compile_defs}
        LIBS ${boot_link_libs}
        USE_PRINTF_FLOAT OFF
    )
endif()
