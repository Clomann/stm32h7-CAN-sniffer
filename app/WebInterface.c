#include <string.h>

#include "WebInterface.h"

void appCtrlCgiHandler(
    int iIndex,
    int iNumParams,
    char *pcParam[],
    char *pcValue[]
)
{
    uint32_t i  = 0;
    char *param = NULL;
    char *value = NULL;

    if (iIndex == 0)
    {
        /* Check cgi parameter */
        for (i = 0; i < (uint32_t)iNumParams; i++)
        {
            param = pcParam[i];
            value = pcValue[i];

            if (strcmp(param, "action") == 0)
            {
                if (strcmp(value, "Stop") == 0)
                {
                    WebInterface_GetActionHook(0);
                }
                else if (strcmp(value, "Start") == 0)
                {
                    WebInterface_GetActionHook(1);
                }
            }
        }
    }
}
