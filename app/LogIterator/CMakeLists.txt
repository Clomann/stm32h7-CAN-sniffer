# Sources
list(APPEND sources_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/LogIterator.c
)

# Append new include directories
list(APPEND include_path_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Send the final list to the parent scope
set(sources_SRCS "${sources_SRCS}" PARENT_SCOPE)
set(include_path_DIRS "${include_path_DIRS}" PARENT_SCOPE)