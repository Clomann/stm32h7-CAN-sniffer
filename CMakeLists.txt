cmake_minimum_required(VERSION 3.22)

# Setup compiler settings
set(CMAKE_C_STANDARD                11)
set(CMAKE_C_STANDARD_REQUIRED       ON)
set(CMAKE_C_EXTENSIONS              ON)
set(CMAKE_CXX_STANDARD              20)
set(CMAKE_CXX_STANDARD_REQUIRED     ON)
set(CMAKE_CXX_EXTENSIONS            ON)
set(PROJ_PATH                       ${CMAKE_CURRENT_SOURCE_DIR})
set(STM32_HAL_PATH                  ${CMAKE_CURRENT_SOURCE_DIR}/libs/STM32_HAL/STM32H7)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJ_PATH}/_bin)

message("Build type: "              ${CMAKE_BUILD_TYPE})

#
# Core project settings
#
project(SPI_FullDuplex_ComDMA_CM7)          # Modified
enable_language(C CXX ASM)


#
# Core MCU flags, CPU, instruction set and FPU setup
# Needs to be set properly for your MCU
#
set(CPU_PARAMETERS
    -mthumb

    # This needs attention to properly set for used MCU
    -mcpu=cortex-m7                 # Modified
    -mfpu=fpv5-d16                  # Modified
    -mfloat-abi=hard                # Modified
)

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
set(linker_script_SRC               ${PROJ_PATH}/STM32CubeIDE/CM7/STM32H745ZITX_FLASH.ld) # Modified
set(EXECUTABLE                      ${CMAKE_PROJECT_NAME})

#
# List of source files to compile
#
set(sources_SRCS                    # Modified
    ${PROJ_PATH}/CM7/Src/main.c
    ${PROJ_PATH}/CM7/Src/stm32h7xx_hal_msp.c
    ${PROJ_PATH}/CM7/Src/stm32h7xx_it.c

    ${PROJ_PATH}/Common/Src/system_stm32h7xx.c

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
)

add_subdirectory(adapters)
add_subdirectory(app)
add_subdirectory(drivers)
add_subdirectory(libs)
add_subdirectory(middleware)

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
    -Og -g3 -ggdb
)

# Linker options
target_link_options(${EXECUTABLE} PRIVATE
    -T${linker_script_SRC}
    ${CPU_PARAMETERS}
    -Wl,-Map=${CMAKE_PROJECT_NAME}.map
    --specs=nosys.specs
    -u _printf_float                # STDIO float formatting support
    -Wl,--start-group
    -lc
    -lm
    -lstdc++
    -lsupc++
    -Wl,--end-group
    -Wl,--print-memory-usage
    -Wl,--warn-unresolved-symbols
    -Wl,--trace
)



# Execute post-build to print size
add_custom_command(TARGET ${EXECUTABLE} POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${EXECUTABLE}>
)

# Convert output to hex and binary
add_custom_command(TARGET ${EXECUTABLE} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${EXECUTABLE}> ${EXECUTABLE}.hex
)

# Convert to bin file -> add conditional check?
add_custom_command(TARGET ${EXECUTABLE} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${EXECUTABLE}> ${EXECUTABLE}.bin
)