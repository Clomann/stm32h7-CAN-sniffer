#include "./fdcan_msg_port.h"

#include <string.h>
#include <stdbool.h>
#include <stdint.h>

int mock_frames_available = 0;
bool mock_read_called     = false;

static FDCAN_ClassicFrameType mock_frames[16];
static int frames_remaining   = 0;

void reset_fdcan_stubs(void)
{
    mock_frames_available = 0;
    mock_read_called      = false;
    frames_remaining      = 0;

    memset(mock_frames, 0, sizeof(mock_frames));
    mock_frames[0].id           = 0x100;
    mock_frames[0].dlc_dl_flags = 8;
    mock_frames[0].channel      = 1;
    mock_frames[0].timestamp    = 2000;
    memset(mock_frames[0].data, 0xDD, sizeof(mock_frames[0].data));
}

void set_frames_available(int count)
{
    if (count > (int)(sizeof(mock_frames) / sizeof(mock_frames[0])))
    {
        count = (int)(sizeof(mock_frames) / sizeof(mock_frames[0]));
    }

    mock_frames_available = count;
    frames_remaining      = count;

    /* Seed remaining frames with incrementing id/timestamp for variety */
    for (int i = 0; i < count; i++)
    {
        mock_frames[i]           = mock_frames[0];
        mock_frames[i].id       += (uint32_t)i;
        mock_frames[i].timestamp += (uint32_t)(10 * i);
    }
}

bool get_read_called(void)
{
    return mock_read_called;
}

void fdcan_msg_port_init(void)
{
    ;
}

void fdcan_msg_port_flush(void)
{
    /* no-op */
}

size_t fdcan_msg_port_read(FDCAN_ClassicFrameType **frame, uint32_t timeout)
{
    (void)timeout;

    mock_read_called = true;

    if ((frames_remaining > 0) && (frame != NULL))
    {
        int index = mock_frames_available - frames_remaining;
        *frame    = &mock_frames[index];
        frames_remaining--;

        return sizeof(FDCAN_ClassicFrameType);
    }

    if (frame != NULL)
    {
        *frame = NULL;
    }
    return 0;
}

uint8_t fdcan_msg_port_get_highwater_bytes(uint32_t *bytes)
{
    if (bytes)
    {
        *bytes = 0U;
    }
    return 0U;
}

uint32_t fdcan_msg_port_get_capacity_bytes(void)
{
    return 0U;
}
