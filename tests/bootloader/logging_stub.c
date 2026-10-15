#include "logging_stub.h"

#include <stdarg.h>

static FILE *s_log_file;

void test_log_set_file(FILE *file)
{
    s_log_file = file;
}

void test_log_write(const char *fmt, ...)
{
    FILE *out = s_log_file ? s_log_file : stderr;
    va_list args;

    va_start(args, fmt);
    vfprintf(out, fmt, args);
    va_end(args);

    fputc('\n', out);
    fflush(out);
}
