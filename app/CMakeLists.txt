# Sources
list(APPEND sources_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/FileHandler.c
    ${CMAKE_CURRENT_SOURCE_DIR}/HttpAbs.c
    ${CMAKE_CURRENT_SOURCE_DIR}/CanLogBuffer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/SettingsHandler.c
    ${CMAKE_CURRENT_SOURCE_DIR}/CanLogManager.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ConfigManager.c
    ${CMAKE_CURRENT_SOURCE_DIR}/WebInterface.c
    ${CMAKE_CURRENT_SOURCE_DIR}/RuntimeChecks.c
    ${CMAKE_CURRENT_SOURCE_DIR}/http/src/app_ethernet.c
    ${CMAKE_CURRENT_SOURCE_DIR}/http/src/app_mdns.c
    ${CMAKE_CURRENT_SOURCE_DIR}/http/src/ethernetif.c
    ${CMAKE_CURRENT_SOURCE_DIR}/http/src/fs_custom.c
    ${CMAKE_CURRENT_SOURCE_DIR}/http/src/httpd_cgi_ssi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/http/src/httpd_post.c
    ${CMAKE_CURRENT_SOURCE_DIR}/http/src/lwip.c
    ${CMAKE_CURRENT_SOURCE_DIR}/http/src/tcp_echoserver.c
)

# Include directories
list(APPEND include_path_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/fs
    ${CMAKE_CURRENT_SOURCE_DIR}/http/inc
)


# Send the final list to the parent scope
set(sources_SRCS "${sources_SRCS}" PARENT_SCOPE)
set(include_path_DIRS "${include_path_DIRS}" PARENT_SCOPE)