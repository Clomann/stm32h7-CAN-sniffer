#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "fs_custom_internal.h"

bool FsCustom_AppendJson(
    char *buffer,
    size_t buffer_size,
    size_t *offset,
    const char *format,
    ...
)
{
    va_list args;
    int n;

    if ((NULL == buffer) || (NULL == offset) || (NULL == format)
        || (*offset >= buffer_size))
    {
        return false;
    }

    va_start(args, format);
    n = vsnprintf(&buffer[*offset], buffer_size - *offset, format, args);
    va_end(args);

    if ((n < 0) || ((size_t)n >= (buffer_size - *offset)))
    {
        return false;
    }

    *offset += (size_t)n;
    return true;
}
