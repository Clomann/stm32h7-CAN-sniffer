#include "ff.h"
#include "test_filehandler_state.h"
#include "test_ff_state.h"
#include <string.h>
#include <stdbool.h>

typedef struct
{
    char path[96];
    uint32_t size;
    bool exists;
} TestFileEntry;

typedef struct
{
    FIL *fil;
    int file_index;
    uint32_t pos;
    bool in_use;
} TestOpenHandle;

static TestFileEntry test_files[256];
static TestOpenHandle open_handles[16];
static uint32_t test_file_count   = 0;
static uint32_t test_open_count   = 0;
static uint32_t test_expand_count = 0;

static int test_find_file_index(const char *path)
{
    for (uint32_t i = 0; i < test_file_count; i++)
    {
        if (test_files[i].exists && (strcmp(test_files[i].path, path) == 0))
        {
            return (int)i;
        }
    }
    return -1;
}

static int test_create_file_entry(const char *path)
{
    if (test_file_count >= (sizeof(test_files) / sizeof(test_files[0])))
    {
        return -1;
    }

    strncpy(
        test_files[test_file_count].path,
        path,
        sizeof(test_files[test_file_count].path) - 1U
    );
    test_files[test_file_count]
        .path[sizeof(test_files[test_file_count].path) - 1U] = '\0';
    test_files[test_file_count].size                         = 0U;
    test_files[test_file_count].exists                       = true;
    return (int)test_file_count++;
}

static TestOpenHandle *test_open_handle_for(FIL *fp)
{
    for (uint32_t i = 0; i < (sizeof(open_handles) / sizeof(open_handles[0]));
         i++)
    {
        if (open_handles[i].in_use && open_handles[i].fil == fp)
        {
            return &open_handles[i];
        }
    }
    return NULL;
}

static TestOpenHandle *test_alloc_handle(FIL *fp, int file_index)
{
    for (uint32_t i = 0; i < (sizeof(open_handles) / sizeof(open_handles[0]));
         i++)
    {
        if (!open_handles[i].in_use)
        {
            open_handles[i].in_use     = true;
            open_handles[i].fil        = fp;
            open_handles[i].file_index = file_index;
            open_handles[i].pos        = 0U;
            test_open_count++;
            return &open_handles[i];
        }
    }
    return NULL;
}

void test_ff_reset(void)
{
    memset(test_files, 0, sizeof(test_files));
    memset(open_handles, 0, sizeof(open_handles));
    test_file_count   = 0;
    test_open_count   = 0;
    test_expand_count = 0;
}

bool test_ff_file_exists(const char *path)
{
    return (test_find_file_index(path) >= 0);
}

uint32_t test_ff_get_file_size(const char *path)
{
    int idx = test_find_file_index(path);
    if (idx < 0)
    {
        return 0U;
    }
    return test_files[idx].size;
}

uint32_t test_ff_get_open_count(void)
{
    return test_open_count;
}

uint32_t test_ff_get_expand_count(void)
{
    return test_expand_count;
}

FRESULT f_stat(const char *path, FILINFO *fno)
{
    if (path && (strcmp(path, "/logs") == 0))
    {
        if (fno)
        {
            fno->fattrib = AM_DIR;
            fno->fsize   = 0;
            strcpy(fno->fname, "logs");
        }
        return FR_OK;
    }

    if (path && (strcmp(path, "/logs_meta.json") == 0))
    {
        if (!test_filehandler_meta_exists())
        {
            return FR_NO_FILE;
        }

        if (fno)
        {
            fno->fattrib = 0;
            fno->fsize   = test_filehandler_meta_size();
            strcpy(fno->fname, "logs_meta.json");
        }
        return FR_OK;
    }

    if (path)
    {
        int idx = test_find_file_index(path);
        if (idx >= 0)
        {
            if (fno)
            {
                fno->fattrib = 0;
                fno->fsize   = test_files[idx].size;
                strncpy(
                    fno->fname,
                    test_files[idx].path,
                    sizeof(fno->fname) - 1U
                );
                fno->fname[sizeof(fno->fname) - 1U] = '\0';
            }
            return FR_OK;
        }
    }

    return FR_NO_FILE;
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
    if (fp == NULL || path == NULL)
    {
        return FR_INVALID_PARAMETER;
    }

    int idx               = test_find_file_index(path);
    const bool create_new = (mode & FA_CREATE_NEW) != 0U;
    const bool write      = (mode & FA_WRITE) != 0U;

    if (idx < 0)
    {
        if (create_new || write)
        {
            idx = test_create_file_entry(path);
            if (idx < 0)
            {
                return FR_NOT_ENOUGH_CORE;
            }
        }
        else
        {
            return FR_NO_FILE;
        }
    }
    else if (create_new)
    {
        return FR_EXIST;
    }

    if (test_alloc_handle(fp, idx) == NULL)
    {
        return FR_TOO_MANY_OPEN_FILES;
    }

    return FR_OK;
}

FRESULT f_close(FIL *fp)
{
    if (fp == NULL)
    {
        return FR_INVALID_PARAMETER;
    }

    TestOpenHandle *handle = test_open_handle_for(fp);
    if (handle)
    {
        handle->in_use = false;
    }
    return FR_OK;
}

FRESULT f_lseek(FIL *fp, FSIZE_t ofs)
{
    TestOpenHandle *handle = test_open_handle_for(fp);
    if (handle == NULL)
    {
        return FR_INVALID_OBJECT;
    }

    uint32_t size = test_files[handle->file_index].size;
    if (ofs > size)
    {
        return FR_INVALID_PARAMETER;
    }

    handle->pos = (uint32_t)ofs;
    return FR_OK;
}

FRESULT f_read(FIL *fp, void *buff, UINT btr, UINT *br)
{
    TestOpenHandle *handle = test_open_handle_for(fp);
    if (handle == NULL)
    {
        return FR_INVALID_OBJECT;
    }

    uint32_t size      = test_files[handle->file_index].size;
    uint32_t available = (handle->pos < size) ? (size - handle->pos) : 0U;
    uint32_t to_read   = (btr < available) ? btr : available;

    if (buff && to_read > 0U)
    {
        memset(buff, 0, to_read);
    }

    if (br)
    {
        *br = to_read;
    }

    handle->pos += to_read;
    return FR_OK;
}

FRESULT f_write(FIL *fp, const void *buff, UINT btw, UINT *bw)
{
    (void)buff;
    TestOpenHandle *handle = test_open_handle_for(fp);
    if (handle == NULL)
    {
        return FR_INVALID_OBJECT;
    }

    uint32_t new_end = handle->pos + btw;
    if (new_end > test_files[handle->file_index].size)
    {
        test_files[handle->file_index].size = new_end;
    }

    handle->pos = new_end;

    if (bw)
    {
        *bw = btw;
    }
    return FR_OK;
}

FRESULT f_expand(FIL *fp, FSIZE_t fsz, BYTE opt)
{
    (void)opt;
    TestOpenHandle *handle = test_open_handle_for(fp);
    if (handle == NULL)
    {
        return FR_INVALID_OBJECT;
    }

    test_files[handle->file_index].size = (uint32_t)fsz;
    if (handle->pos > test_files[handle->file_index].size)
    {
        handle->pos = test_files[handle->file_index].size;
    }
    test_expand_count++;
    return FR_OK;
}
