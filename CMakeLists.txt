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

# Set linker script
set(linker_script_SRC               ${PROJ_PATH}/STM32CubeIDE/CM7/STM32H745ZITX_FLASH.ld) # Modified
set(EXECUTABLE                      ${CMAKE_PROJECT_NAME})

#
# List of source files to compile
#
set(sources_SRCS                    # Modified
    ${PROJ_PATH}/CM7/Src/main.c
    ${PROJ_PATH}/CM7/Src/Spi_Cmds.c
    ${PROJ_PATH}/CM7/Src/stm32h7xx_hal_msp.c
    ${PROJ_PATH}/CM7/Src/stm32h7xx_it.c

    ${PROJ_PATH}/CM7/Inc/DiskIoFacade.c
    ${PROJ_PATH}/CM7/Inc/MmcAdapter.c
    ${PROJ_PATH}/CM7/Inc/Sd.c
    
    ${PROJ_PATH}/STM32CubeIDE/CM7/Example/User/CM7/syscalls.c
    ${PROJ_PATH}/STM32CubeIDE/CM7/Example/User/CM7/sysmem.c

    ${PROJ_PATH}/libs/ff15a/source/diskio.c
    ${PROJ_PATH}/libs/ff15a/source/ff.c
    ${PROJ_PATH}/libs/ff15a/source/ffsystem.c
    ${PROJ_PATH}/libs/ff15a/source/ffunicode.c

    #${PROJ_PATH}/STM32CubeIDE/CM7/Example/User/CM7/Src/stm32h7xx_it.c
    
    
    #${PROJ_PATH}/Core/Src/system_stm32h7xx.c
#    ${PROJ_PATH}/Core/Startup/startup_stm32h735igkx.s
#    ${PROJ_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_exti.c
#    ${PROJ_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_gpio.c
#    ${PROJ_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_pwr.c
#    ${PROJ_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_rcc.c
#    ${PROJ_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_utils.c
)

#
# Include directories
#
set(include_path_DIRS               # Modified
    ${PROJ_PATH}/CM7/Inc
    ${PROJ_PATH}/Common/Inc

    ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Inc
    ${STM32_HAL_PATH}/Drivers/CMSIS/Device/ST/STM32H7xx/Include
    ${STM32_HAL_PATH}/Drivers/CMSIS/Include
    ${STM32_HAL_PATH}/Drivers/BSP/STM32H7xx_Nucleo
    ${STM32_HAL_PATH}/Drivers/BSP/Components/Common
    ${STM32_HAL_PATH}/Middlewares/ST/STM32_Audio/Addons/PDM/Inc
    ${STM32_HAL_PATH}/Utilities/CPU
    ${STM32_HAL_PATH}/Utilities/Fonts

    ${PROJ_PATH}/libs/ff15a/source/
)

#
# Symbols definition
#
set(symbols_SYMB                    # Modified
    "STM32H745xx"
    "USE_HAL_DRIVER"
    "CORE_CM7"
    "USE_STM32H7XX_NUCLEO_144_MB1363"
)

# Executable files
add_executable(${EXECUTABLE} ${sources_SRCS})

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