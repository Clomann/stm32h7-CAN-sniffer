cmake_minimum_required(VERSION 3.22)

# Let callers override this: -DCPU_PARAMETERS=";-mthumb;-mcpu=cortex-m7;..."
set(CPU_PARAMETERS "" CACHE STRING "Target CPU/ABI flags (list)")
set(DUAL_CORE "" CACHE STRING "Configure two target cofiguration (true/false).")

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

message("Build type: "              ${CMAKE_BUILD_TYPE})

#
# Core project settings
#
project(SPI_FullDuplex_ComDMA_${CORE})          # Modified
enable_language(C CXX ASM)

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
    set(linker_script_SRC               ${PROJ_PATH}/STM32CubeIDE/CM7/STM32H745ZITX_FLASH.ld)
elseif(CORE STREQUAL "CM4")
    set(linker_script_SRC               ${PROJ_PATH}/STM32CubeIDE/CM4/STM32H745ZITX_FLASH.ld)
endif()

set(EXECUTABLE                      ${CMAKE_PROJECT_NAME})

if(CORE STREQUAL "CM7")
    #
    # List of source files to compile
    #
    set(sources_SRCS                    # Modified
        ${PROJ_PATH}/CM7/Src/main.c
        ${PROJ_PATH}/CM7/Src/stm32h7xx_hal_msp.c
        ${PROJ_PATH}/CM7/Src/stm32h7xx_it.c
        ${PROJ_PATH}/CM7/Src/Core0Task0.c
        ${PROJ_PATH}/CM7/Src/Core0Task1.c
        ${PROJ_PATH}/CM7/Src/CanBridgeTask.c
        ${PROJ_PATH}/CM7/Src/CanSendTask.c
        ${PROJ_PATH}/CM7/Src/CanLogManager.c
        ${PROJ_PATH}/CM7/Src/ConfigManager.c
        ${PROJ_PATH}/CM7/Src/CanCtrl.c
        ${PROJ_PATH}/CM7/Src/SpiTask.c
        ${PROJ_PATH}/CM7/Src/utils_mpu.c

        ${PROJ_PATH}/Common/Src/system_stm32h7xx.c

        ${PROJ_PATH}/middleware/freertos/freertos_interface.c

        ${PROJ_PATH}/STM32CubeIDE/CM7/Example/User/CM7/syscalls.c
        ${PROJ_PATH}/STM32CubeIDE/CM7/Example/User/CM7/sysmem.c
        ${PROJ_PATH}/STM32CubeIDE/CM7/Example/User/startup/startup_stm32h745zitx.s
    )

    #
    # Include directories
    #
    set(include_path_DIRS               # Modified
        ${PROJ_PATH}/CM7/Inc
        ${PROJ_PATH}/Common/Inc
        ${PROJ_PATH}/middleware/freertos
    )

    add_subdirectory(adapters)
    add_subdirectory(app)
    add_subdirectory(drivers)
    add_subdirectory(libs)
    add_subdirectory(middleware)
    add_subdirectory(ports)

elseif(CORE STREQUAL "CM4")
    #
    # List of source files to compile
    #
    set(sources_SRCS                    # Modified
        ${PROJ_PATH}/CM4/Src/main.c
        ${PROJ_PATH}/CM4/Src/stm32h7xx_hal_msp.c
        ${PROJ_PATH}/CM4/Src/stm32h7xx_it.c

        ${PROJ_PATH}/Common/Src/system_stm32h7xx.c

        ${PROJ_PATH}/STM32CubeIDE/CM4/Example/User/CM4/syscalls.c
        ${PROJ_PATH}/STM32CubeIDE/CM4/Example/User/CM4/sysmem.c
        ${PROJ_PATH}/STM32CubeIDE/CM4/Example/User/startup/startup_stm32h745zitx.s
    )

    #
    # Include directories
    #
    set(include_path_DIRS               # Modified
        ${PROJ_PATH}/CM4/Inc
        ${PROJ_PATH}/Common/Inc
    )
    
    add_subdirectory(libs)
endif()

# Executable files
add_executable(${EXECUTABLE} ${sources_SRCS})

target_link_libraries(${EXECUTABLE} 
    PRIVATE 
        lwrb 
        freertos_kernel
        freertos_config)

# Include paths
target_include_directories(${EXECUTABLE} PRIVATE ${include_path_DIRS})

# Project symbols
target_compile_definitions(${EXECUTABLE} PRIVATE ${symbols_SYMB})

# Compiler options
target_compile_options(${EXECUTABLE} PRIVATE
    ${CPU_PARAMETERS}
    -Wall
    -Wextra
    -Wpedantic
    -Wno-unused-parameter
    -Werror=implicit-function-declaration
    # Full debug configuration
    $<$<CONFIG:Release>:-O2>
    $<$<CONFIG:Debug>:-Og;-g3;-ggdb>
    -fstack-usage
    -ffunction-sections
    -fdata-sections
)

# Linker options
target_link_options(${EXECUTABLE} PRIVATE
    -T${linker_script_SRC}
    ${CPU_PARAMETERS}
    -Wl,-Map=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_PROJECT_NAME}.map
    --specs=nosys.specs
    -u _printf_float                # STDIO float formatting support
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
add_custom_command(TARGET ${EXECUTABLE} POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${EXECUTABLE}>
)

# Convert output to hex and binary
add_custom_command(TARGET ${EXECUTABLE} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${EXECUTABLE}> ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${EXECUTABLE}.hex
)

# Convert to bin file -> add conditional check?
add_custom_command(TARGET ${EXECUTABLE} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${EXECUTABLE}> ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${EXECUTABLE}.bin
)