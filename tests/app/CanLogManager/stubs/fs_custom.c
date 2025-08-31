#include "fs_custom.h"
#include "CanLogManager.h"

#include <stdbool.h>

static uint32_t mock_head_index = 0;
static uint32_t mock_tail_index = 0;
static bool mock_tracer_running = false;

void reset_fs_stubs(void)
{
    mock_head_index     = 0;
    mock_tail_index     = 0;
    mock_tracer_running = false;
}

void set_tracer_running(bool running)
{
    mock_tracer_running = running;
}
