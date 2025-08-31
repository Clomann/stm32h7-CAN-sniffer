#include "ff.h"
#include <string.h>

FRESULT f_stat(const char *path, FILINFO *fno)
{
    (void)path;

    if (fno)
    {
        fno->fattrib = AM_DIR;
        fno->fsize   = 1000;
        strcpy(fno->fname, "test.log");
    }
    return FR_OK;
}

FRESULT f_mkdir(const char *path)
{
    (void)path;

    return FR_OK;
}

FRESULT f_unlink(const char *path)
{
    (void)path;

    return FR_OK;
}
