/**
  ******************************************************************************
  * @file    LwIP/LwIP_HTTP_Server_Raw/Src/httpd_cg_ssi.c
  * @author  MCD Application Team
  * @brief   Webserver SSI and CGI handlers
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "lwip/tcp.h"
#include "lwip/apps/httpd.h"
#include "http_cgi_ssi.h"

#include <string.h>
#include <stdlib.h>

/**
 * \note If LWIP_HTTPD_MAX_TAG_NAME_LEN is not set explicitly, it will default to 8!
 */
char const *CFG_TAGCHAR[] = {
    "baud1", // index=0
    "mode1",
    "baud2", // index=0
    "mode2",
};

char const **TAGS = (char const **)&CFG_TAGCHAR;

u16_t Handler(int iIndex, char *pcInsert, int iInsertLen);

/* CGI handler for LED control */
const char *app_control_cgi_handler(
    int iIndex,
    int iNumParams,
    char *pcParam[],
    char *pcValue[]
);
const char *CAN_config_CGI_Handler(
    int iIndex,
    int iNumParams,
    char *pcParam[],
    char *pcValue[]
);
void httpd_ssi_init(void);
void httpd_cgi_init(void);

/* Html request for "/leds.cgi" will start LEDS_CGI_Handler */
const tCGI CAN_CTL_CGI = {"/cancontrol.cgi", app_control_cgi_handler};
const tCGI CAN_CFG_CGI = {"/can.cgi", CAN_config_CGI_Handler};

/* Cgi call table, only one CGI used */
tCGI CGI_TAB[] = {CAN_CTL_CGI, CAN_CFG_CGI};

/**
  * @brief  ADC_Handler : SSI handler for ADC page
  */
u16_t Handler(int iIndex, char *pcInsert, int iInsertLen)
{
    return http_app_get_setting(iIndex, pcInsert, iInsertLen);
}

const char *CAN_config_CGI_Handler(
    int iIndex,
    int iNumParams,
    char *pcParam[],
    char *pcValue[]
)
{
    http_app_set_setting(iIndex, iNumParams, pcParam, pcValue);

    /* uri to send after cgi call*/
    return "/can.shtml";
}

/**
  * @brief  CGI handler for CAN control
  */
const char *app_control_cgi_handler(
    int iIndex,
    int iNumParams,
    char *pcParam[],
    char *pcValue[]
)
{
    appCtrlCgiHandler(iIndex, iNumParams, pcParam, pcValue);

    /* uri to send after cgi call*/
    return "/can.shtml";
}

/**
  * @brief  Http webserver Init
  */
void http_server_init(void)
{
    uint8_t HandlerCount;

    /* Httpd Init */
    httpd_init();

    /* configure SSI handlers */
    http_set_ssi_handler(Handler, (char const **)TAGS, 4);

    /* configure CGI handlers */
    HandlerCount = sizeof(CGI_TAB) / sizeof(tCGI);
    http_set_cgi_handlers(CGI_TAB, HandlerCount);
}
