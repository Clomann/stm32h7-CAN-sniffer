#include <string.h>
#include <stdlib.h>
#include <stdio.h>


#include "SettingsHandler.h"
#include "http_cgi_ssi.h"
#include "httpd_post.h"
#include "core_json.h"
#include "fdcan.h"


#define SETTINGS_HANDLER_JSON_BUFFER_SIZE 256U

static AppConfigType * AppSettings;

static void consume_param_values(char *param, char *value)
{
    AppFcdanConfigType *pCanConfig;
    const char Baudrate[] = "baud";
    const char Mode[] = "mode";
    
    /* check parameter "baud" */
    if (strncmp(param, Baudrate, sizeof(Baudrate)-1) == 0)
    {
        uint32_t NewBaudrate = 0;
        uint32_t OldBaudrate = 0;

        if (strcmp(param , "baud1") == 0)
        {
            pCanConfig = &AppSettings->can1;
            
        }
        else if (strcmp(param , "baud2") == 0)
        {
            pCanConfig = &AppSettings->can2;
        }
        else
        {
            return;
        }

        OldBaudrate = pCanConfig->baudrate;

        if(strcmp(value, "250000") == 0)
        {
            NewBaudrate = 250000;
        }
        else if(strcmp(value, "500000") == 0)
        {
            NewBaudrate = 500000;
        }
        else if(strcmp(value, "1000000") == 0)
        {
            NewBaudrate = 1000000;
        }
        else
        {
            NewBaudrate = OldBaudrate;
        }

        if (NewBaudrate != OldBaudrate)
        {
            AppSettings->updated = 1U;
        }

        pCanConfig->baudrate = NewBaudrate;
    }
    else if (strncmp(param, Mode, sizeof(Mode)-1)==0)
    {
        uint32_t NewMode = 0;
        uint32_t OldMode = 0;

        if (strcmp(param , "mode1") == 0)
        {
            pCanConfig = &AppSettings->can1;
            
        }
        else if (strcmp(param , "mode2") == 0)
        {
            pCanConfig = &AppSettings->can2;
        }
        else
        {
            return;
        }

        OldMode = pCanConfig->mode;
            
        if(strcmp(value, "1") ==0)
        {
            NewMode = FDCAN_MODE_1;
        }
        else if(strcmp(value, "2") ==0)
        {
            NewMode = FDCAN_MODE_2;
        }
        else if(strcmp(value, "3") ==0)
        {
            NewMode = FDCAN_MODE_3;
        }
        else
        {
            NewMode = OldMode;
        }

        if (NewMode != OldMode)
        {
            AppSettings->updated = 1U;
        }

        pCanConfig->mode = NewMode;
    }
    else if (strcmp(param , "action") == 0)
    {
        if(strcmp(value, "apply") ==0)
        {
            SettingsHandler_ApplyRequestCallback();
        }
        else if(strcmp(value, "save") ==0)
        {
        }
    }
}

void httpd_post_cb(char *key, char *val)
{
    consume_param_values(key, val);
}

void http_app_set_setting(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    uint32_t i = 0;
    char * param = NULL;
    char * value = NULL;

    if (iIndex==1)
    {
      /* Check cgi parameter */
      for (i = 0; i<(uint32_t)iNumParams; i++)
      {
        param = pcParam[i];
        value = pcValue[i];

        consume_param_values(param, value);        
      }
    }
}

int http_app_get_setting(int iIndex, char *pcInsert, int iInsertLen)
{
    //   iIndex=0 => "opt250"
  //   iIndex=1 => "opt500"
  //   storedBaudRate is the previously selected baud
  switch (iIndex) {
    case 0: // "naud1"
        if (AppSettings->can1.baudrate == 250000) {
            snprintf(pcInsert, iInsertLen, "250 kbit/s");
        } else if (AppSettings->can1.baudrate == 500000) {
            snprintf(pcInsert, iInsertLen, "500 kbit/s");
        } else if (AppSettings->can1.baudrate == 1000000) {
            snprintf(pcInsert, iInsertLen, "1 Mbit/s");
        } else {
            snprintf(pcInsert, iInsertLen, "n/a");
        }
        return (uint16_t)strlen(pcInsert);
    case 1: // "mode1"
        if (AppSettings->can1.mode == 1) {
            snprintf(pcInsert, iInsertLen, "normal");
        } else if (AppSettings->can1.mode == 2) {
            snprintf(pcInsert, iInsertLen, "listen only");
        } else if (AppSettings->can1.mode == 3) {
            snprintf(pcInsert, iInsertLen, "off");
        } else {
            snprintf(pcInsert, iInsertLen, "n/a");
        }
        return (uint16_t)strlen(pcInsert);    
    case 2: // "baud2"
        if (AppSettings->can2.baudrate == 250000) {
            snprintf(pcInsert, iInsertLen, "250 kbit/s");
        } else if (AppSettings->can2.baudrate == 500000) {
            snprintf(pcInsert, iInsertLen, "500 kbit/s");
        } else if (AppSettings->can2.baudrate == 1000000) {
            snprintf(pcInsert, iInsertLen, "1 Mbit/s");
        } else {
            snprintf(pcInsert, iInsertLen, "n/a");
        }
        return (uint16_t)strlen(pcInsert);
    case 3: // "mode2"
        if (AppSettings->can2.mode == 1) {
            snprintf(pcInsert, iInsertLen, "normal");
        } else if (AppSettings->can2.mode == 2) {
            snprintf(pcInsert, iInsertLen, "listen only");
        } else if (AppSettings->can2.mode == 3) {
            snprintf(pcInsert, iInsertLen, "off");
        } else {
            snprintf(pcInsert, iInsertLen, "n/a");
        }
        return (uint16_t)strlen(pcInsert);      
    default:
        break;
  }

  return 0;
}

#define ERROR_INVALID_FORMAT   -1
#define ERROR_TOO_MANY_SEGMENTS -2
#define SUCCESS 0

static int IpStringToIntArray(const char *ip, uint8_t len, uint8_t *arr)
{
    char element[4U] = {0};  // Buffer for each segment (max 3 digits + null terminator)
    uint32_t ValTmp;
    uint8_t CharCount = 0U;
    uint8_t ElemCount = 0U;

    for (uint8_t index = 0U; index < len; index++)
    {
        if (ip[index] >= '0' && ip[index] <= '9')  // Valid digit
        {
            if (CharCount < 3U)  
            {
                element[CharCount++] = ip[index];
                element[CharCount] = '\0';  // Ensure it's null-terminated
            }
            else  
            {
                return ERROR_INVALID_FORMAT; // Too many digits in a segment
            }
        }
        else if (ip[index] == '.')  // Delimiter
        {
            if (CharCount == 0U) return ERROR_INVALID_FORMAT; // Prevent ".." or leading/trailing dots

            FileHandler_ConvertToInteger(element, &ValTmp, 10U);  
            if (ValTmp > 255) return ERROR_INVALID_FORMAT; // Invalid octet value

            arr[ElemCount++] = (uint8_t) ValTmp;

            if (ElemCount > 3U) return ERROR_TOO_MANY_SEGMENTS; // Too many octets

            CharCount = 0U;
        }
        else  
        {
            return ERROR_INVALID_FORMAT; // Invalid character in input
        }
    }

    if (CharCount == 0U) return ERROR_INVALID_FORMAT; // Handle trailing dot case

    FileHandler_ConvertToInteger(element, &ValTmp, 10U);
    if (ValTmp > 255) return ERROR_INVALID_FORMAT; // Check last segment

    arr[ElemCount++] = (uint8_t) ValTmp;
    
    return (ElemCount == 4U) ? SUCCESS : ERROR_INVALID_FORMAT; // Ensure exactly 4 octets
}

int SettingsHandler_ParseConfig(char *buffer, uint32_t len, AppConfigType *cfg)
{
    // Variables used in this example.
    JSONStatus_t result;
    size_t bufferLength = len;
    const char queryKey1[] = "CAN1.Baudrate";
    const size_t queryKeyLength1 = sizeof( queryKey1 ) - 1;
    const char queryKey2[] = "CAN1.Mode";
    const size_t queryKeyLength2 = sizeof( queryKey2 ) - 1;
    const char queryKey3[] = "CAN2.Baudrate";
    const size_t queryKeyLength3 = sizeof( queryKey3 ) - 1;
    const char queryKey4[] = "CAN2.Mode";
    const size_t queryKeyLength4 = sizeof( queryKey4 ) - 1;
    const char queryKey5[] = "HTTP.IP";
    const size_t queryKeyLength5 = sizeof( queryKey5 ) - 1;

    char TmpBuf[64];

    char * value;
    size_t valueLength;

    uint32_t Baudrate;
    uint32_t Mode;
    uint8_t IP[4U];

    // Calling JSON_Validate() is not necessary if the document is guaranteed to be valid.
    result = JSON_Validate( buffer, bufferLength );    

    if( result == JSONSuccess )
    {
        result = FileHandler_GetValue( buffer, bufferLength, queryKey1, queryKeyLength1, &value, &valueLength);
        if( JSONSuccess ==  result )
        {
            strncpy(TmpBuf, value, valueLength);
            TmpBuf[valueLength] = '\0';
            if ( 0U == FileHandler_ConvertToInteger(TmpBuf, &Baudrate, 10U) )
                cfg->can1.baudrate = Baudrate;
        }
        
        result = FileHandler_GetValue( buffer, bufferLength, queryKey2, queryKeyLength2, &value, &valueLength );
        if( JSONSuccess == result ) 
        {
            strncpy(TmpBuf, value, valueLength);
            TmpBuf[valueLength] = '\0';
            if (0U == FileHandler_ConvertToInteger(TmpBuf, &Mode, 10U))
                cfg->can1.mode = Mode;
        }

        result = FileHandler_GetValue( buffer, bufferLength, queryKey3, queryKeyLength3, &value, &valueLength);
        if( JSONSuccess ==  result )
        {
            strncpy(TmpBuf, value, valueLength);
            TmpBuf[valueLength] = '\0';
            if ( 0U == FileHandler_ConvertToInteger(TmpBuf, &Baudrate, 10U) )
                cfg->can2.baudrate = Baudrate;
        }
        
        result = FileHandler_GetValue( buffer, bufferLength, queryKey4, queryKeyLength4, &value, &valueLength );
        if( JSONSuccess == result ) 
        {
            strncpy(TmpBuf, value, valueLength);
            TmpBuf[valueLength] = '\0';
            if (0U == FileHandler_ConvertToInteger(TmpBuf, &Mode, 10U))
                cfg->can2.mode = Mode;
        }

        result = FileHandler_GetValue( buffer, bufferLength, queryKey5, queryKeyLength5, &value, &valueLength );
        if( JSONSuccess == result ) 
        {
            if (0U == IpStringToIntArray(value, valueLength, IP))
                memcpy(cfg->ip, IP, 4U);
        }
    }

    if (JSONSuccess == result)
    {
        return SETTINGS_OK;
    }
    else
    {
        return SETTINGS_NOT_OK;
    }
}

int FatFS_SD_LoadConfig(FatFsDeviceType *dev, char *data, uint32_t *len)
{
    FRESULT fr;
    FATFS FatFs;
    UINT BytesRead = 0U;
    uint32_t FileSize = 0U;

    fr = f_mount(&FatFs, SETTINGSHANDLER_PARTITION_NO, 0U);		/* Give a work area to the default drive */

    if (fr == FR_OK)
        fr = f_open(&dev->file, SETTINGSHANDLER_PARTITION_NO "conf.txt", FA_READ | FA_OPEN_EXISTING);	/* Create a file */

    if (fr == FR_OK) {
        FileSize = f_size(&dev->file);
        f_read(&dev->file, data, FileSize, &BytesRead);
        fr = f_close(&dev->file);							/* Close the file */
    }

    *len = BytesRead;

    return fr;
}

uint8_t SettingsHandler_Init(AppConfigType *cfg)
{
    AppSettings = cfg;
    return 0U;
}

static uint8_t m_CreateJSonString(AppConfigType *cfg, char *json, uint32_t maxLength, uint32_t *len)
{
    // Create the JSON string
    snprintf(json, maxLength,
    "{\n"
    "    \"CAN1\":{\n"
    "        \"Baudrate\":%ld,\n"
    "        \"Mode\":%d\n"
    "    },\n"
    "    \"CAN2\":{\n"
    "        \"Baudrate\":%ld,\n"
    "        \"Mode\":%d\n"
    "    },\n"
    "    \"HTTP\":{\n"
    "        \"IP\":\"%d.%d.%d.%d\"\n"
    "    }\n"
    "}\n",
    cfg->can1.baudrate,
    cfg->can1.mode,
    cfg->can2.baudrate,
    cfg->can2.mode,
    cfg->ip[0U],
    cfg->ip[1U],
    cfg->ip[2U],
    cfg->ip[3U]);

    *len = strnlen(json, maxLength);
    return 0U;
}

int SettingsHandler_CreateJsonString(AppConfigType* config, char* buffer, uint32_t maxLength, uint32_t* length) {
    return m_CreateJSonString(config, buffer, maxLength, length);
}

uint8_t SettingsHandler_Poll(AppConfigType *cfg) 
{
    if (cfg->updated)
    {
        return 1;
    }
    else 
    {
        return 0;
    }
}
