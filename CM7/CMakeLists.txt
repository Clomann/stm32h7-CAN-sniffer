# Sources
list(APPEND sources_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32h7xx_hal_msp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/stm32h7xx_it.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/Core0Task0.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/Core0Task1.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/SdBridgeTask.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/CanBridgeTask.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/CanSendTask.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/CanCtrl.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/SpiTask.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/utils_mpu.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/profiling.c
)

# Include directories
list(APPEND include_path_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/Src
)


# Send the final list to the parent scope
set(sources_SRCS "${sources_SRCS}" PARENT_SCOPE)
set(include_path_DIRS "${include_path_DIRS}" PARENT_SCOPE)