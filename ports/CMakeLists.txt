set(DRIVERS_BASE ${CMAKE_CURRENT_LIST_DIR})

# Sources
list(APPEND sources_SRCS
${CMAKE_CURRENT_LIST_DIR}/fdcan_msg_port.c
${CMAKE_CURRENT_LIST_DIR}/spi/spi_port_freertos.c
)

# Include directories
list(APPEND include_path_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/
    ${CMAKE_CURRENT_LIST_DIR}/spi
)

# Send the final list to the parent scope
set(sources_SRCS "${sources_SRCS}" PARENT_SCOPE)
set(include_path_DIRS "${include_path_DIRS}" PARENT_SCOPE)