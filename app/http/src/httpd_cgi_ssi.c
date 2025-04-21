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
// #include "lwip/debug.h"
#include "lwip/tcp.h"
#include "lwip/apps/httpd.h"
#include "http_cgi_ssi.h"


#include <string.h>
#include <stdlib.h>

tSSIHandler ADC_Page_SSI_Handler;
uint32_t ADC_not_configured=1;

/* we will use character "t" as tag for CGI */
char const* TAGCHAR[] = {
  "baudrate",  // index=0
  "mode",  // index=1
};

char const** TAGS = (char const**)&TAGCHAR;

u16_t Handler(int iIndex, char *pcInsert, int iInsertLen);

/* CGI handler for LED control */
const char * LEDS_CGI_Handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
const char * CAN_config_CGI_Handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
void httpd_ssi_init(void);
void httpd_cgi_init(void);

/* Html request for "/leds.cgi" will start LEDS_CGI_Handler */
const tCGI LEDS_CGI={"/leds.cgi", LEDS_CGI_Handler};
const tCGI CAN_CFG_CGI={"/can.cgi", CAN_config_CGI_Handler};

/* Cgi call table, only one CGI used */
tCGI CGI_TAB[1];

/**
  * @brief  ADC_Handler : SSI handler for ADC page
  */
u16_t Handler(int iIndex, char *pcInsert, int iInsertLen)
{
  return http_app_get_setting(iIndex, pcInsert, iInsertLen);
}

const char * CAN_config_CGI_Handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
  http_app_set_setting(iIndex, iNumParams, pcParam, pcValue);
  
  /* uri to send after cgi call*/
  return "/can.shtml";
}

/**
  * @brief  CGI handler for LEDs control
  */
const char * LEDS_CGI_Handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
  uint32_t i=0;

  /* We have only one SSI handler iIndex = 0 */
  if (iIndex==0)
  {
    /* All LEDs off */
    // BSP_LED_Off(LED1);
    // BSP_LED_Off(LED2);
    // BSP_LED_Off(LED3);
    // BSP_LED_Off(LED4);

    /* Check cgi parameter : application GET /leds.cgi?led=2&led=4 */
    for (i=0; i<(uint32_t)iNumParams; i++)
    {
      /* check parameter "led" */
      if (strcmp(pcParam[i] , "led")==0)
      {
        /* Switch LED1 ON if 1 */
        if(strcmp(pcValue[i], "1") ==0)
        {
          
        }  // BSP_LED_On(LED1);

        /* Switch LED2 ON if 2 */
        else if(strcmp(pcValue[i], "2") ==0)
        {  // BSP_LED_On(LED2);
        }

        /* Switch LED3 ON if 3 */
        else if(strcmp(pcValue[i], "3") ==0)
        {

        } // BSP_LED_On(LED3);

        /* Switch LED4 ON if 4 */
        else if(strcmp(pcValue[i], "4") ==0)
        {

        }  // BSP_LED_On(LED4);
      }
    }
  }
  /* uri to send after cgi call*/
  return "/can.shtml";
}

/**
  * @brief  Http webserver Init
  */
void http_server_init(void)
{
  /* Httpd Init */
  httpd_init();

  /* configure SSI handlers */
  http_set_ssi_handler(Handler, (char const **)TAGS, 4);

  /* configure CGI handlers */
  CGI_TAB[0] = LEDS_CGI;
  CGI_TAB[1] = CAN_CFG_CGI;
  http_set_cgi_handlers(CGI_TAB, 2);
}
