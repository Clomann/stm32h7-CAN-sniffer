set(DRIVERS_BASE ${CMAKE_CURRENT_LIST_DIR})

# Sources
set(DRIVERS_SRCS
    ${CMAKE_CURRENT_LIST_DIR}/spi/Spi_Cmds.c
    ${CMAKE_CURRENT_LIST_DIR}/CanAbs/CanAbs.c 
    ${CMAKE_CURRENT_LIST_DIR}/comm/CommHandle/CommHandle.c 
    ${CMAKE_CURRENT_LIST_DIR}/comm/CommManager/CommManager.c 
    ${CMAKE_CURRENT_LIST_DIR}/fdcan/fdcan.c
    ${CMAKE_CURRENT_LIST_DIR}/fdcan/fdcan_utils.c
)

# Include directories
set(DRIVERS_INCS
    ${CMAKE_CURRENT_LIST_DIR}/
    ${CMAKE_CURRENT_LIST_DIR}/spi
    ${CMAKE_CURRENT_LIST_DIR}/CanAbs
    ${CMAKE_CURRENT_LIST_DIR}/comm/CommHandle
    ${CMAKE_CURRENT_LIST_DIR}/comm/CommManager
    ${CMAKE_CURRENT_LIST_DIR}/fdcan
)
