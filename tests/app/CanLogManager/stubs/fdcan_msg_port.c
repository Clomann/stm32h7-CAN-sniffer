#include "./fdcan_msg_port.h"

#include <string.h>
#include <stdbool.h>
#include <stdint.h>

int mock_frames_available = 0;
bool mock_read_called     = false;

static FDCAN_ClassicFrame mock_test_frame;
static int frames_remaining = 0;

void reset_fdcan_stubs(void)
{
    mock_frames_available = 0;
    mock_read_called      = false;
    frames_remaining      = 0;

    mock_test_frame.id        = 0x100;
    mock_test_frame.dlc       = 8;
    mock_test_frame.channel   = 1;
    mock_test_frame.timestamp = 2000;
    memset(mock_test_frame.data, 0xDD, sizeof(mock_test_frame.data));
}

void set_frames_available(int count)
{
    mock_frames_available = count;
    frames_remaining      = count;
}

bool get_read_called(void)
{
    return mock_read_called;
}

void fdcan_msg_port_init(void)
{
    ;
}

size_t fdcan_msg_port_read(FDCAN_ClassicFrame *frame, uint32_t timeout)
{
    (void)timeout;

    mock_read_called = true;

    if (frames_remaining > 0 && frame)
    {
        *frame = mock_test_frame;
        mock_test_frame.timestamp += 10;
        mock_test_frame.id++;
        frames_remaining--;
        return 1;
    }

    return 0;
}
