#include "test_CanLogManager.h"

#include "unity.h"

#include "test_definitions.h"
#include "CanLogManager.h"
#include "CanLogBuffer.h"
#include "RuntimeChecks.h"

#include "fs_custom.h"
#include "CanAbs.h"
#include "fdcan_msg_port.h"

static CanLogControlDataType *testCtrlData;
static uint8_t mockMountRes;
static bool mockRunCanTracer;
static bool mockCommit;
static FDCAN_ClassicFrameType testFrame;

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
    mockCommit = false;

    testFrame.id           = 0x123;
    testFrame.dlc_dl_flags = 8;
    testFrame.channel      = 1;
    testFrame.timestamp    = 1000;
    memset(testFrame.data, 0xAA, sizeof(testFrame.data));

    CanLogBuffer_Init();
    testCtrlData = CanLogHandler_Init(&mockMountRes, &mockRunCanTracer, &mockCommit);
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
    TEST_ASSERT_EQUAL(MAX_LOG_INDEX + 1U, capacity);
}

/**
 * @brief Tests adding a single CAN entry to the buffer
 * @details Verifies CanLogBuffer_AddEntry successfully stores a CAN message
 */
void test_CanLogBuffer_AddEntry(void)
{
    CanLogEntryStackBufferType entryBuf = {0};
    CanLogEntryType *entry              = (CanLogEntryType *)entryBuf.raw;
    const uint8_t payload_len           = 8;

    entry->header.header_len = sizeof(entry->header);
    entry->header.type       = CLB_ENTRY_TYPE_FRAME;
    entry->data_len          = payload_len;
    entry->header.total_len  = sizeof(CanLogEntryType) + entry->data_len;
    entry->timestamp         = 12345;
    entry->channel           = 1;
    entry->can_id            = 0x123;
    entry->dlc_flags         = MAKE_DLC_FLAGS(payload_len, 0);
    memset(entry->data, 0xBB, payload_len);

    uint8_t result = CanLogBuffer_AddEntry(entry, entry->header.total_len);
    TEST_ASSERT_EQUAL(0, result);
}

/**
 * @brief Tests reading a complete block from the buffer
 * @details Verifies block extraction and header metadata correctness
 */
void test_CanLogBuffer_ReadBlock(void)
{
    CanLogEntryStackBufferType entryBuf = {0};
    CanLogEntryType *entry              = (CanLogEntryType *)entryBuf.raw;
    const uint8_t payload_len           = 8;

    entry->header.header_len = sizeof(entry->header);
    entry->header.type       = CLB_ENTRY_TYPE_FRAME;
    entry->data_len          = payload_len;
    entry->header.total_len  = sizeof(CanLogEntryType) + entry->data_len;
    entry->timestamp         = 5000;
    entry->channel           = 2;
    entry->can_id            = 0x456;
    entry->dlc_flags         = MAKE_DLC_FLAGS(payload_len, 0);
    memset(entry->data, 0xCC, payload_len);

    int entriesNeeded = (BLOCK_SIZE / entry->header.total_len) + 1;
    for (int i = 0; i < entriesNeeded; i++)
    {
        entry->timestamp = 5000 + i;
        entry->can_id    = 0x456 + i;
        CanLogBuffer_AddEntry(entry, entry->header.total_len);
    }

    uint8_t *blockData;
    uint32_t blockLength;
    uint32_t frameCount;
    uint8_t readResult = CanLogBuffer_ReadNextBlock(&blockData, &blockLength, &frameCount);

    TEST_ASSERT_EQUAL(CANLOG_E_OK, readResult);
    TEST_ASSERT_EQUAL(BLOCK_SIZE, blockLength);

    CanLogBlockHeaderType *header = (CanLogBlockHeaderType *)blockData;
    TEST_ASSERT_EQUAL(CANLOG_VERSION, header->version);
    TEST_ASSERT_EQUAL(BLOCK_SIZE, header->block_size);

    CanLogBuffer_Consume(blockLength, frameCount);
}

/**
 * @brief Ensures ReadNextBlock only succeeds when a full block is available
 */
void test_CanLogBuffer_BlockBoundaryPadding(void)
{
    CanLogEntryStackBufferType entryBuf = {0};
    CanLogEntryType *entry              = (CanLogEntryType *)entryBuf.raw;
    const uint8_t payload_len           = 12;

    entry->header.header_len = sizeof(entry->header);
    entry->header.type       = CLB_ENTRY_TYPE_FRAME;
    entry->data_len          = payload_len;
    entry->header.total_len  = sizeof(CanLogEntryType) + entry->data_len;
    entry->dlc_flags         = MAKE_DLC_FLAGS(payload_len, 0);

    /* Fill until a full block is ready */
    uint8_t blockReady = 0;
    uint32_t i         = 0;
    while (blockReady == 0)
    {
        entry->timestamp = 7000 + i;
        entry->can_id    = 0x600 + i;
        entry->channel   = 1;
        memset(entry->data, 0xA0 + (uint8_t)i, payload_len);

        TEST_ASSERT_EQUAL_UINT8(
            CANLOG_E_OK,
            CanLogBuffer_AddEntry(entry, entry->header.total_len)
        );

        CanLogBuffer_IsBlockReady(&blockReady);
        i++;
    }

    uint8_t *blockData;
    uint32_t blockLength;
    uint32_t frameCount;
    TEST_ASSERT_EQUAL_UINT8(
        CANLOG_E_OK,
        CanLogBuffer_ReadNextBlock(&blockData, &blockLength, &frameCount)
    );

    CanLogBlockHeaderType *header = (CanLogBlockHeaderType *)blockData;
    TEST_ASSERT_EQUAL(BLOCK_SIZE, blockLength);
    TEST_ASSERT_NOT_EQUAL(0U, frameCount);
    TEST_ASSERT_GREATER_OR_EQUAL(sizeof(CanLogBlockHeaderType), header->header_size);
    TEST_ASSERT_LESS_OR_EQUAL_UINT(header->block_size, BLOCK_SIZE);

    CanLogBuffer_Consume(blockLength, frameCount);

    /* After consuming, no full block should be ready yet */
    CanLogBuffer_IsBlockReady(&blockReady);
    TEST_ASSERT_EQUAL(0, blockReady);

    /* Fill again until another full block is ready */
    while (blockReady == 0)
    {
        entry->timestamp = 9000 + i;
        entry->can_id    = 0x700 + i;
        entry->channel   = 2;
        memset(entry->data, 0xB0 + (uint8_t)i, payload_len);

        TEST_ASSERT_EQUAL_UINT8(
            CANLOG_E_OK,
            CanLogBuffer_AddEntry(entry, entry->header.total_len)
        );

        CanLogBuffer_IsBlockReady(&blockReady);
        i++;
    }

    TEST_ASSERT_EQUAL_UINT8(
        CANLOG_E_OK,
        CanLogBuffer_ReadNextBlock(&blockData, &blockLength, &frameCount)
    );
    TEST_ASSERT_EQUAL(BLOCK_SIZE, blockLength);
    TEST_ASSERT_NOT_EQUAL(0U, frameCount);
    CanLogBuffer_Consume(blockLength, frameCount);
}

/**
 * @brief Verifies consume updates frame counters coherently
 */
void test_CanLogBuffer_ConsumeUpdatesCounters(void)
{
    CanLogEntryStackBufferType entryBuf = {0};
    CanLogEntryType *entry              = (CanLogEntryType *)entryBuf.raw;
    const uint8_t payload_len           = 8;

    entry->header.header_len = sizeof(entry->header);
    entry->header.type       = CLB_ENTRY_TYPE_FRAME;
    entry->data_len          = payload_len;
    entry->header.total_len  = sizeof(CanLogEntryType) + entry->data_len;
    entry->dlc_flags         = MAKE_DLC_FLAGS(payload_len, 0);

    const uint32_t entriesNeeded =
        (BLOCK_SIZE / entry->header.total_len) + 1U;

    for (uint32_t i = 0; i < entriesNeeded; i++)
    {
        entry->timestamp = 8000 + i;
        entry->can_id    = 0x700 + i;
        entry->channel   = 2;
        memset(entry->data, 0xB0 + (uint8_t)i, payload_len);

        TEST_ASSERT_EQUAL_UINT8(
            CANLOG_E_OK,
            CanLogBuffer_AddEntry(entry, entry->header.total_len)
        );
    }

    /* Verify write-side counter */
    TEST_ASSERT_EQUAL_UINT64(entriesNeeded, CanLogBuffer_FrameCount1);

    uint8_t *blockData;
    uint32_t blockLength;
    uint32_t frameCount;
    TEST_ASSERT_EQUAL_UINT8(
        CANLOG_E_OK,
        CanLogBuffer_ReadNextBlock(&blockData, &blockLength, &frameCount)
    );

    CanLogBuffer_Consume(blockLength, frameCount);

    TEST_ASSERT_EQUAL_UINT64(frameCount, CanLogBuffer_FrameCount2);
    TEST_ASSERT_EQUAL_UINT64(0U, CanLogBuffer_FrameDropCount);
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
    RUN_TEST(test_CanLogBuffer_ReadBlock);
    RUN_TEST(test_CanLogBuffer_BlockBoundaryPadding);
    RUN_TEST(test_CanLogBuffer_ConsumeUpdatesCounters);
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
