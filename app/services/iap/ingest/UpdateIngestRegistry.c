#include "UpdateIngestRegistry.h"

#include <string.h>

static UpdateIngestBindingType UpdateIngestRegistry_ActiveBinding;
static uint8_t UpdateIngestRegistry_HasBinding = 0u;

UpdateIngestStatusType
UpdateIngestRegistry_Register(const UpdateIngestVTableType *vtable, void *ctx)
{
    if (vtable == NULL || vtable->begin == NULL || vtable->write_chunk == NULL
        || vtable->finalize == NULL || vtable->abort == NULL)
    {
        return IAP_UPDATE_INGEST_E_PARAM;
    }

    UpdateIngestRegistry_ActiveBinding.vtable = vtable;
    UpdateIngestRegistry_ActiveBinding.ctx    = ctx;
    UpdateIngestRegistry_HasBinding           = 1u;

    return IAP_UPDATE_INGEST_E_OK;
}

UpdateIngestStatusType UpdateIngestRegistry_Get(UpdateIngestBindingType *binding
)
{
    if (binding == NULL)
    {
        return IAP_UPDATE_INGEST_E_PARAM;
    }

    if (UpdateIngestRegistry_HasBinding == 0u)
    {
        return IAP_UPDATE_INGEST_E_UNAVAILABLE;
    }

    *binding = UpdateIngestRegistry_ActiveBinding;
    return IAP_UPDATE_INGEST_E_OK;
}

void UpdateIngestRegistry_Clear(void)
{
    (void)memset(
        &UpdateIngestRegistry_ActiveBinding,
        0,
        sizeof(UpdateIngestRegistry_ActiveBinding)
    );
    UpdateIngestRegistry_HasBinding = 0u;
}
