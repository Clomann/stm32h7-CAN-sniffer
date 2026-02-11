/**
  ******************************************************************************
  * @file    LwIP/LwIP_HTTP_Server_Raw/Inc/http_cgi_ssi.h 
  * @author  MCD Application Team
  * @brief   Header for http_cgi_ssi.c module
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __HTTP_CGI_SSI_H
#define __HTTP_CGI_SSI_H

#ifdef __cplusplus
extern "C" {
#endif

// #ifndef APP_UPDATE_SETTING_DEFINED
// #error "You must implement app_update_setting() to use HTTP settings update!"
// #endif

/* Includes ------------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void http_server_init(void);

/* Shim functions ------------------------------------------------------- */
void http_app_set_setting(
    int iIndex,
    int iNumParams,
    char *pcParam[],
    char *pcValue[]
);
int http_app_get_setting(int iIndex, char *pcInsert, int iInsertLen);

#ifdef __cplusplus
}
#endif

#endif /* __HTTP_CGI_SSI_H */
