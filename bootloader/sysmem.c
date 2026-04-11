/*
 * Minimal sysmem for bootloader (newlib _sbrk).
 */

#include <errno.h>
#include <sys/types.h>

extern char _end; /* Provided by the linker */
extern char _estack; /* Provided by the linker */

static char *heap_end;

void *_sbrk(ptrdiff_t incr)
{
    if (heap_end == 0)
    {
        heap_end = &_end;
    }

    char *prev_heap_end = heap_end;
    char *new_heap_end  = heap_end + incr;

    if (new_heap_end < &_end || new_heap_end > &_estack)
    {
        errno = ENOMEM;
        return (void *)-1;
    }

    heap_end = new_heap_end;
    return (void *)prev_heap_end;
}
