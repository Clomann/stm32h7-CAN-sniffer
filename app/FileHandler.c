#include <stdlib.h>
#include <string.h>

#include "FileHandler.h"
#include "core_json.h"

static int FileHandler_GetValue(
    char *buff, 
    uint32_t buffLen, 
    char const *key, 
    uint32_t keyLen, 
    char **value, 
    size_t *valLen
)
{
    JSONStatus_t result;

    *valLen = 0U;
    
    result = JSON_Search( buff, buffLen, key, keyLen,
        value, valLen );
        
    if( result == JSONSuccess )
    {
        // The pointer "value" will point to a location in the "buffer".
        char save = (*value)[ *valLen ];
        // After saving the character, set it to a null byte for printing.
        (*value)[ *valLen ] = '\0';
        
        // // Restore the original character.
        (*value)[ *valLen ] = save;  
    }

    return result;
}

static int m_ConvertToInteger(char *data, uint32_t *val, uint8_t base)
{
    char *endptr;
    long value = strtol(data, &endptr, base);

    // Check if the conversion was successful
    if (*endptr != '\0') {
        // Handle conversion error: non-numeric characters were encountered
    } 
    else
    {
        *val = (uint32_t) value;
    }

    return 0U;
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

            m_ConvertToInteger(element, &ValTmp, 10U);  
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

    m_ConvertToInteger(element, &ValTmp, 10U);
    if (ValTmp > 255) return ERROR_INVALID_FORMAT; // Check last segment

    arr[ElemCount++] = (uint8_t) ValTmp;
    
    return (ElemCount == 4U) ? SUCCESS : ERROR_INVALID_FORMAT; // Ensure exactly 4 octets
}

int FileHandler_LoadConfig(char *buffer, uint32_t len, AppConfigType *cfg)
{
    // Variables used in this example.
    JSONStatus_t result;
    size_t bufferLength = len;
    char queryKey1[] = "CAN1.Baudrate";
    size_t queryKeyLength1 = sizeof( queryKey1 ) - 1;
    char queryKey2[] = "HTTP.IP";
    size_t queryKeyLength2 = sizeof( queryKey2 ) - 1;
    
    char TmpBuf[64];

    char * value;
    size_t valueLength;

    uint32_t Baudrate;
    uint8_t IP[4U];

    // Calling JSON_Validate() is not necessary if the document is guaranteed to be valid.
    result = JSON_Validate( buffer, bufferLength );    

    if( result == JSONSuccess )
    {
        
        if( JSONSuccess == (result = FileHandler_GetValue( buffer, bufferLength, queryKey1, queryKeyLength1, &value, &valueLength ) ) )
        {
            strncpy(TmpBuf, value, valueLength);
            TmpBuf[valueLength] = '\0';
            if ( 0U == m_ConvertToInteger(TmpBuf, &Baudrate, 10U) )
                cfg->baudrate = Baudrate;
        }
        if( JSONSuccess == (result = FileHandler_GetValue( buffer, bufferLength, queryKey2, queryKeyLength2, &value, &valueLength ) ) ) 
        {
            if (0U == IpStringToIntArray(value, valueLength, IP))
                memcpy(cfg->ip, IP, 4U);
        }
    }

    return result;
}