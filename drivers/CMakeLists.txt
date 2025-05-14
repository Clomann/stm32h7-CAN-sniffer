set(DRIVERS_BASE ${CMAKE_CURRENT_LIST_DIR})

# Sources
list(APPEND sources_SRCS
    ${CMAKE_CURRENT_LIST_DIR}/spi/Spi_Cmds.c
    ${CMAKE_CURRENT_LIST_DIR}/CanAbs/CanAbs.c 
    ${CMAKE_CURRENT_LIST_DIR}/comm/CommHandle/CommHandle.c 
    ${CMAKE_CURRENT_LIST_DIR}/comm/CommManager/CommManager.c 
    ${CMAKE_CURRENT_LIST_DIR}/fdcan/fdcan.c
    ${CMAKE_CURRENT_LIST_DIR}/fdcan/fdcan_utils.c
)

# Include directories
list(APPEND include_path_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/
    ${CMAKE_CURRENT_LIST_DIR}/spi
    ${CMAKE_CURRENT_LIST_DIR}/CanAbs
    ${CMAKE_CURRENT_LIST_DIR}/comm/CommHandle
    ${CMAKE_CURRENT_LIST_DIR}/comm/CommManager
    ${CMAKE_CURRENT_LIST_DIR}/fdcan
)

# Send the final list to the parent scope
set(sources_SRCS "${sources_SRCS}" PARENT_SCOPE)
set(include_path_DIRS "${include_path_DIRS}" PARENT_SCOPE)