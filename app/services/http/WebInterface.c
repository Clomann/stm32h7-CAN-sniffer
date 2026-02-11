#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "WebInterface.h"
#include "CanLogManager.h"

static uint32_t appParseUint32(const char *value)
{
    char *end            = NULL;
    unsigned long parsed = strtoul(value, &end, 10);

    if (value == end)
    {
        return 0U;
    }

    if (parsed > UINT32_MAX)
    {
        return UINT32_MAX;
    }

    return (uint32_t)parsed;
}

void appCtrlCgiHandler(
    int iIndex,
    int iNumParams,
    char *pcParam[],
    char *pcValue[]
)
{
    uint32_t i            = 0;
    char *param           = NULL;
    char *value           = NULL;
    uint32_t cluster_kb   = 0U;
    uint32_t file_size_mb = 0U;
    uint32_t file_count   = 0U;
    bool format_requested = false;

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
                else if (strcmp(value, "Format") == 0)
                {
                    format_requested = true;
                }
            }
            else if (strcmp(param, "cluster_kb") == 0)
            {
                cluster_kb = appParseUint32(value);
            }
            else if (strcmp(param, "file_size_mb") == 0)
            {
                file_size_mb = appParseUint32(value);
            }
            else if (strcmp(param, "file_count") == 0)
            {
                file_count = appParseUint32(value);
            }
        }

        if (format_requested)
        {
            uint32_t cluster_size   = CLUSTER_SIZE;
            uint32_t log_file_size  = MAX_LOG_FILE_SIZE;
            uint32_t log_file_count = MAX_LOG_FILE_COUNT;

            if (cluster_kb > 0U && cluster_kb <= (UINT32_MAX / 1024U))
            {
                cluster_size = cluster_kb * 1024U;
            }

            if (file_size_mb > 0U
                && file_size_mb <= (UINT32_MAX / (1024U * 1024U)))
            {
                log_file_size = file_size_mb * 1024U * 1024U;
            }

            if (file_count > 0U)
            {
                log_file_count = file_count;
            }

            WebInterface_RequestFormattingHook(
                cluster_size,
                log_file_size,
                log_file_count
            );
        }
    }
}
