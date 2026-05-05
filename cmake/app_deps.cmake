# Application dependency aggregation

set(sources_SRCS ${common_sources_SRCS})
set(include_path_DIRS ${common_include_path_DIRS})

list(APPEND sources_SRCS
    ${PROJ_PATH}/platform/board/stm32h745/cm7/Src/stm32h7xx_hal_msp.c
    ${PROJ_PATH}/platform/board/stm32h745/cm7/Src/stm32h7xx_it.c
    ${PROJ_PATH}/platform/board/stm32h745/cm7/Src/main.c
    ${PROJ_PATH}/platform/board/stm32h745/cm7/Src/utils_mpu.c
    ${PROJ_PATH}/platform/Common/Src/instrumentation.c
    ${PROJ_PATH}/platform/Common/Src/FwUpdateHandoff.c
    ${PROJ_PATH}/middleware/freertos/freertos_interface.c
    ${PROJ_PATH}/STM32CubeIDE/CM7/Example/User/CM7/syscalls.c
    ${PROJ_PATH}/STM32CubeIDE/CM7/Example/User/startup/startup_stm32h745zitx.s
)

if(FW_USE_MALLOC)
    list(APPEND sources_SRCS
        ${PROJ_PATH}/STM32CubeIDE/CM7/Example/User/CM7/sysmem.c
    )
endif()

list(APPEND include_path_DIRS
    ${PROJ_PATH}/middleware/freertos
)

add_subdirectory(app/cores/cm7)
add_subdirectory(adapters)
add_subdirectory(app)
add_subdirectory(platform/drivers)
add_subdirectory(libs)
add_subdirectory(middleware)
add_subdirectory(platform/ports)

set(app_sources_SRCS "${sources_SRCS}")
set(app_include_path_DIRS "${include_path_DIRS}")
set(app_link_libs lwrb freertos_kernel freertos_config third_party_interface)
