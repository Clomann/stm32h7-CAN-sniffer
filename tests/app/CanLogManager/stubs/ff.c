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

FRESULT f_opendir(DIR *dp, const char *path)
{
    (void)dp;
    (void)path;
    return FR_OK;
}

FRESULT f_readdir(DIR *dp, FILINFO *fno)
{
    (void)dp;
    if (fno)
    {
        fno->fname[0] = 0; /* signal end of directory */
        fno->fsize    = 0;
        fno->fattrib  = 0;
    }
    return FR_OK;
}

FRESULT f_closedir(DIR *dp)
{
    (void)dp;
    return FR_OK;
}

FRESULT f_open(FIL *fp, const char *path, BYTE mode)
{
    (void)fp;
    (void)path;
    (void)mode;
    return FR_OK;
}

FRESULT f_close(FIL *fp)
{
    (void)fp;
    return FR_OK;
}

FRESULT f_lseek(FIL *fp, FSIZE_t ofs)
{
    (void)fp;
    (void)ofs;
    return FR_OK;
}

FRESULT f_read(FIL *fp, void *buff, UINT btr, UINT *br)
{
    (void)fp;
    (void)buff;
    if (br)
    {
        *br = btr;
    }
    return FR_OK;
}

FRESULT f_write(FIL *fp, const void *buff, UINT btw, UINT *bw)
{
    (void)fp;
    (void)buff;
    if (bw)
    {
        *bw = btw;
    }
    return FR_OK;
}

FRESULT f_expand(FIL *fp, FSIZE_t fsz, BYTE opt)
{
    (void)fp;
    (void)fsz;
    (void)opt;
    return FR_OK;
}
