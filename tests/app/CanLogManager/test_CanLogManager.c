#include "test_CanLogManager.h"

#include "unity.h"

#include "test_definitions.h"
#include "CanLogManager.h"
#include "CanLogBuffer.h"

#include "fs_custom.h"
#include "CanAbs.h"
#include "fdcan_msg_port.h"

static CanLogControlDataType *testCtrlData;
static uint8_t mockMountRes;
static bool mockRunCanTracer;
static FDCAN_ClassicFrame testFrame;

void reset_fs_stubs(void);
void reset_can_stubs(void);
void reset_fdcan_stubs(void);
void reset_filehandler_stubs(void);

void set_file_size(uint32_t size);
void set_file_open_result(FRESULT result);
void set_frames_available(int count);
void set_can_start_result(comm_status_t result);
void set_tracer_running(bool running);

bool get_file_closed(void);
bool get_file_open_failed(void);
bool get_can_started(void);
bool get_can_stopped(void);
bool get_read_called(void);

void test_CanLogManager_setUp(void)
{
    reset_fs_stubs();
    reset_can_stubs();
    reset_fdcan_stubs();
    reset_filehandler_stubs();

    mockMountRes     = RES_OK;
    mockRunCanTracer = false;

    testFrame.id        = 0x123;
    testFrame.dlc       = 8;
    testFrame.channel   = 1;
    testFrame.timestamp = 1000;
    memset(testFrame.data, 0xAA, sizeof(testFrame.data));

    CanLogBuffer_Init();
    testCtrlData = CanLogHandler_Init(&mockMountRes, &mockRunCanTracer);
}

void test_CanLogManager_tearDown(void)
{
    if (testCtrlData)
    {
        appCanLogHandlerDeInit(testCtrlData);
    }
}

/**
 * @brief Tests CanLogManager initialization and fs_custom interface functions
 * @details Verifies control data structure creation, tracer running state query, 
 *          and log capacity retrieval
 */
void test_CanLogHandler_Init(void)
{
    TEST_ASSERT_NOT_NULL(testCtrlData);

    uint8_t running;
    uint8_t result = FsCustom_IsTracerRunning(&running);
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL(mockRunCanTracer, running);

    uint32_t capacity;
    FsCustom_GetCanLogCapacity(&capacity);
    TEST_ASSERT_EQUAL(MAX_LOG_INDEX, capacity);
}

/**
 * @brief Tests adding a single CAN entry to the buffer
 * @details Verifies CanLogBuffer_AddClassicCanEntry successfully stores a CAN message
 */
void test_CanLogBuffer_AddEntry(void)
{
    CanLogClassicCanEntryType entry;
    memset(&entry, 0, sizeof(entry));
    entry.header.header_len = sizeof(entry.header);
    entry.header.type       = CANLOG_CLASSIC_TYPE;
    entry.header.total_len  = sizeof(CanLogClassicCanEntryType);
    entry.timestamp         = 12345;
    entry.channel           = 1;
    entry.dlc               = 8;
    entry.can_id            = 0x123;
    memset(entry.data, 0xBB, sizeof(entry.data));

    uint8_t result = CanLogBuffer_AddClassicCanEntry(&entry);
    TEST_ASSERT_EQUAL(0, result);
}

/**
 * @brief Tests buffer block ready detection
 * @details Fills buffer with enough entries to trigger block ready state
 */
void test_CanLogBuffer_BlockReady(void)
{
    CanLogClassicCanEntryType entry;
    memset(&entry, 0, sizeof(entry));
    entry.header.header_len = sizeof(entry.header);
    entry.header.type       = CANLOG_CLASSIC_TYPE;
    entry.header.total_len  = sizeof(CanLogClassicCanEntryType);
    entry.dlc               = 8;

    int entriesNeeded = (BLOCK_SIZE / sizeof(CanLogClassicCanEntryType)) + 1;
    for (int i = 0; i < entriesNeeded; i++)
    {
        entry.timestamp = 1000 + i;
        entry.can_id    = 0x200 + i;
        entry.channel   = (i % 2) + 1;

        uint8_t result = CanLogBuffer_AddClassicCanEntry(&entry);
        TEST_ASSERT_EQUAL(0, result);
    }

    uint8_t blockReady;
    CanLogBuffer_IsBlockReady(&blockReady);
    TEST_ASSERT_NOT_EQUAL(0, blockReady);
}

/**
 * @brief Tests reading a complete block from the buffer
 * @details Verifies block extraction and header metadata correctness
 */
void test_CanLogBuffer_ReadBlock(void)
{
    CanLogClassicCanEntryType entry;
    memset(&entry, 0, sizeof(entry));
    entry.header.header_len = sizeof(entry.header);
    entry.header.type       = CANLOG_CLASSIC_TYPE;
    entry.header.total_len  = sizeof(CanLogClassicCanEntryType);
    entry.timestamp         = 5000;
    entry.channel           = 2;
    entry.dlc               = 8;
    entry.can_id            = 0x456;
    memset(entry.data, 0xCC, sizeof(entry.data));

    int entriesNeeded = (BLOCK_SIZE / sizeof(CanLogClassicCanEntryType)) + 1;
    for (int i = 0; i < entriesNeeded; i++)
    {
        entry.timestamp = 5000 + i;
        entry.can_id    = 0x456 + i;
        CanLogBuffer_AddClassicCanEntry(&entry);
    }

    uint8_t blockData[BLOCK_SIZE];
    uint32_t blockLength;
    uint8_t readResult = CanLogBuffer_ReadNextBlock(blockData, &blockLength);

    TEST_ASSERT_EQUAL(CANLOG_E_OK, readResult);
    TEST_ASSERT_EQUAL(BLOCK_SIZE, blockLength);

    CanLogBlockHeaderType *header = (CanLogBlockHeaderType *)blockData;
    TEST_ASSERT_EQUAL(CANLOG_VERSION, header->version);
    TEST_ASSERT_EQUAL(BLOCK_SIZE, header->block_size);
}

/**
 * @brief Tests CanLogManager handler initialization
 * @details Verifies appCanLogHandlerInit returns success status
 */
void test_appCanLogHandlerInit(void)
{
    FRESULT result = appCanLogHandlerInit(testCtrlData);
    TEST_ASSERT_EQUAL(FR_OK, result);
}

/**
 * @brief Tests end-to-end frame processing from CAN port to buffer
 * @details Verifies frames are read from fdcan_msg_port when tracer is active
 */
void test_Integration_FrameToBuffer(void)
{
    appCanLogHandlerInit(testCtrlData);

    mockRunCanTracer = true;
    set_frames_available(3);

    appCanLogHandlerPoll(testCtrlData);

    TEST_ASSERT_TRUE(get_read_called());
}

/**
 * @brief Tests CAN tracer start and stop functionality
 * @details Verifies CAN controllers are activated/deactivated with tracer state changes
 */
void test_TracerStartStop(void)
{
    appCanLogHandlerInit(testCtrlData);

    mockRunCanTracer = true;
    appCanLogHandlerPoll(testCtrlData);

    TEST_ASSERT_TRUE(get_can_started());

    mockRunCanTracer = false;
    appCanLogHandlerPoll(testCtrlData);

    TEST_ASSERT_TRUE(get_can_stopped());
}

/**
 * @brief Tests log file rotation trigger
 * @details Verifies file closure when maximum file size is exceeded
 */
void test_FileRotation(void)
{
    appCanLogHandlerInit(testCtrlData);

    set_file_size(MAX_LOG_FILE_SIZE + 1000);

    mockRunCanTracer = true;
    set_frames_available(1);

    appCanLogHandlerPoll(testCtrlData);

    TEST_ASSERT_TRUE(get_file_closed());
}

/**
 * @brief Tests error handling for file operation failures
 * @details Verifies graceful handling of file open failures during initialization
 */
void test_ErrorHandling_FileOpenFailure(void)
{
    set_file_open_result(FR_DENIED);

    FRESULT result = appCanLogHandlerInit(testCtrlData);

    TEST_ASSERT_EQUAL(FR_OK, result);

    TEST_ASSERT_TRUE(get_file_open_failed());
}

void RunAllTests(void)
{
    RUN_TEST(test_CanLogHandler_Init);
    RUN_TEST(test_CanLogBuffer_AddEntry);
    RUN_TEST(test_CanLogBuffer_BlockReady);
    RUN_TEST(test_CanLogBuffer_ReadBlock);
    RUN_TEST(test_appCanLogHandlerInit);
    RUN_TEST(test_Integration_FrameToBuffer);
    RUN_TEST(test_TracerStartStop);
    RUN_TEST(test_FileRotation);
    RUN_TEST(test_ErrorHandling_FileOpenFailure);
}

void test_CanLogBuffer()
{
    RunAllTests();
}
