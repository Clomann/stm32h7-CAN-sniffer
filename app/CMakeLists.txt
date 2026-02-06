# Sources
list(APPEND sources_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/services/storage/FileHandler.c
    ${CMAKE_CURRENT_SOURCE_DIR}/services/http/HttpAbs.c
    ${CMAKE_CURRENT_SOURCE_DIR}/services/logging/CanLogBuffer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/services/config/SettingsHandler.c
    ${CMAKE_CURRENT_SOURCE_DIR}/services/logging/CanLogManager.c
    ${CMAKE_CURRENT_SOURCE_DIR}/services/config/ConfigManager.c
    ${CMAKE_CURRENT_SOURCE_DIR}/services/http/WebInterface.c
    ${CMAKE_CURRENT_SOURCE_DIR}/services/logging/RuntimeChecks.c
    ${CMAKE_CURRENT_SOURCE_DIR}/services/http/src/app_ethernet.c
    ${CMAKE_CURRENT_SOURCE_DIR}/services/http/src/app_mdns.c
    ${CMAKE_CURRENT_SOURCE_DIR}/services/http/src/ethernetif.c
    ${CMAKE_CURRENT_SOURCE_DIR}/services/http/src/fs_custom.c
    ${CMAKE_CURRENT_SOURCE_DIR}/services/http/src/httpd_cgi_ssi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/services/http/src/httpd_post.c
    ${CMAKE_CURRENT_SOURCE_DIR}/services/http/src/lwip.c
    ${CMAKE_CURRENT_SOURCE_DIR}/services/http/src/tcp_echoserver.c
)

# Include directories
list(APPEND include_path_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/services/config
    ${CMAKE_CURRENT_SOURCE_DIR}/services/logging
    ${CMAKE_CURRENT_SOURCE_DIR}/services/storage
    ${CMAKE_CURRENT_SOURCE_DIR}/services/http
    ${CMAKE_CURRENT_SOURCE_DIR}/services/http/fs
    ${CMAKE_CURRENT_SOURCE_DIR}/services/http/inc
)


# Send the final list to the parent scope
set(sources_SRCS "${sources_SRCS}" PARENT_SCOPE)
set(include_path_DIRS "${include_path_DIRS}" PARENT_SCOPE)
