#include "CommFactory.h"

/* These are “magic” symbols the linker emits automatically */
extern const CommFactoryEntry __start_comm_factory[];
extern const CommFactoryEntry __stop_comm_factory[];

/* Helper the Comm core will call */
const CommFactoryEntry *CommFactory_Find(CommProtocolType key)
{
    const CommFactoryEntry *p = __start_comm_factory;
    for (; p < __stop_comm_factory; ++p)
    {
        if (p->protocol == key)
        {
            return p;
        }
    }
    return NULL; /* nothing registered      */
}
