# Sources
set(APP_SRCS
    ${PROJ_PATH}/app/FileHandler.c
    ${PROJ_PATH}/app/HttpAbs.c
    ${PROJ_PATH}/app/CanLogBuffer.c
    ${PROJ_PATH}/app/SettingsHandler.c
    ${PROJ_PATH}/app/http/src/app_ethernet.c
    ${PROJ_PATH}/app/http/src/ethernetif.c
    ${PROJ_PATH}/app/http/src/fs_custom.c
    ${PROJ_PATH}/app/http/src/httpd_cgi_ssi.c
    ${PROJ_PATH}/app/http/src/lwip.c
    ${PROJ_PATH}/app/http/src/tcp_echoserver.c
)

# Include directories
set(APP_INCS
    ${PROJ_PATH}/app
    ${PROJ_PATH}/app/fs
    ${PROJ_PATH}/app/http/inc
)
