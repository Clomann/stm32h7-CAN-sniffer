#pragma once

static inline int mbedtls_printf(const char *format, ...)
{
    (void)format;

    return 0;
}
