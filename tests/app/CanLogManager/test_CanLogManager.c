#include "test_CanLogManager.h"

#include "unity.h"
#include <string.h>
#include <stdio.h>

#include "test_definitions.h"
#include "CanLogManager.h"
#include "CanLogBuffer.h"
#include "RuntimeChecks.h"

#include "fs_custom.h"
#include "CanAbs.h"
#include "fdcan_msg_port.h"
#include "test_filehandler_state.h"
#include "test_ff_state.h"

static CanLogControlDataType *testCtrlData;
static uint8_t mockMountRes;
static bool mockRunCanTracer;
static bool mockCommit;
static FDCAN_ClassicFrameType testFrame;

void reset_fs_stubs(void);
void reset_can_stubs(void);
void reset_fdcan_stubs(void);
void reset_filehandler_stubs(void);
void test_ff_reset(void);

void set_file_size(uint32_t size);
void set_file_open_result(FRESULT result);
void set_frames_available(int count);
void set_can_start_result(comm_status_t result);
void set_tracer_running(bool running);

bool get_file_closed(void);
bool get_file_open_failed(void);
uint64_t get_total_bytes_written(void);
uint64_t get_total_frames_written(void);
uint32_t get_last_block_frame_count(void);
uint32_t get_write_call_count(void);
bool get_can_started(void);
bool get_can_stopped(void);
bool get_read_called(void);

static void RunAllTests(void);

void test_CanLogManager_setUp(void)
{
    reset_fs_stubs();
    reset_can_stubs();
    reset_fdcan_stubs();
    reset_filehandler_stubs();

    mockMountRes     = RES_OK;
    mockRunCanTracer = false;
    mockCommit       = false;

    testFrame.id           = 0x123;
    testFrame.dlc_dl_flags = 8;
    testFrame.channel      = 1;
    testFrame.timestamp    = 1000;
    memset(testFrame.data, 0xAA, sizeof(testFrame.data));

    CanLogBuffer_Init();
    testCtrlData =
        CanLogHandler_Init(&mockMountRes, &mockRunCanTracer, &mockCommit);
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
    uint8_t readResult =
        CanLogBuffer_ReadNextBlock(&blockData, &blockLength, &frameCount);

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
    TEST_ASSERT_GREATER_OR_EQUAL(
        sizeof(CanLogBlockHeaderType),
        header->header_size
    );
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

    const uint32_t entriesNeeded = (BLOCK_SIZE / entry->header.total_len) + 1U;

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
void test_FileRotation_ClosesFileOnSizeExceeded(void)
{
    appCanLogHandlerInit(testCtrlData);

    set_file_size(MAX_LOG_FILE_SIZE + 1000);

    mockRunCanTracer = true;
    set_frames_available(1);

    appCanLogHandlerPoll(testCtrlData);

    TEST_ASSERT_TRUE(get_file_closed());
}

/**
 * @brief Tests end-to-end store pipeline to file
 * @details Verifies a padded block is flushed and counted in the file handler stub
 */
void test_StorePipeline_WritesFramesToFile(void)
{
    appCanLogHandlerInit(testCtrlData);

    /* Clear metadata write side effects from init. */
    reset_filehandler_stubs();

    /* Start tracer once to avoid sync entry in the measured batch. */
    mockRunCanTracer = true;
    set_frames_available(0);
    appCanLogHandlerPoll(testCtrlData);

    /* Clear the sync entry emitted on start. */
    CanLogBuffer_Init();

    set_frames_available(3);
    appCanLogHandlerPoll(testCtrlData);

    /* Stop tracer to force commit/padding flush. */
    mockRunCanTracer = false;
    appCanLogHandlerPoll(testCtrlData);

    TEST_ASSERT_GREATER_THAN_UINT32(0U, get_write_call_count());
    TEST_ASSERT_GREATER_THAN_UINT64(0U, get_total_bytes_written());
    TEST_ASSERT_EQUAL_UINT32(3U, get_last_block_frame_count());
    TEST_ASSERT_EQUAL_UINT64(
        get_last_block_frame_count(),
        get_total_frames_written()
    );
}

/**
 * @brief Tests commit/flush-on-stop path
 * @details Stops tracer to pad buffer and flush a block to storage
 */
void test_CommitFlushOnStop_PadsAndFlushes(void)
{
    appCanLogHandlerInit(testCtrlData);

    /* Clear metadata/prealloc write side effects from init. */
    reset_filehandler_stubs();

    /* Start tracer once to enable stop/commit path. */
    mockRunCanTracer = true;
    set_frames_available(0);
    appCanLogHandlerPoll(testCtrlData);

    /* Clear any sync entry emitted on start. */
    CanLogBuffer_Init();

    /* Inject exactly 54 frames into the buffer. */
    CanLogEntryStackBufferType entryBuf = {0};
    CanLogEntryType *entry              = (CanLogEntryType *)entryBuf.raw;

    entry->header.header_len = sizeof(entry->header);
    entry->header.type       = CLB_ENTRY_TYPE_FRAME;
    entry->data_len          = 0;
    entry->header.total_len  = sizeof(CanLogEntryType);
    entry->dlc_flags         = MAKE_DLC_FLAGS(0, 0);

    for (uint32_t i = 0; i < 54U; i++)
    {
        entry->timestamp = 1000U + i;
        entry->channel   = 1;
        entry->can_id    = 0x100U + i;
        TEST_ASSERT_EQUAL_UINT8(
            CANLOG_E_OK,
            CanLogBuffer_AddEntry(entry, entry->header.total_len)
        );
    }

    mockRunCanTracer = false;
    appCanLogHandlerPoll(testCtrlData);

    TEST_ASSERT_GREATER_THAN_UINT32(0U, get_write_call_count());
    TEST_ASSERT_EQUAL_UINT64(BLOCK_SIZE, get_total_bytes_written());
    TEST_ASSERT_EQUAL_UINT32(54U, get_last_block_frame_count());
    TEST_ASSERT_EQUAL_UINT64(54U, get_total_frames_written());
}

/**
 * @brief Tests metadata load/store JSON round-trip
 * @details Verifies epoch increment and value persistence across init
 */
void test_MetaDataPersistence_RoundTrip(void)
{
    const char input_json[] = "{\n"
                              "    \"epoch\":5,\n"
                              "    \"index\":7,\n"
                              "    \"offset\":123,\n"
                              "    \"crc\":42\n"
                              "}\n";

    test_filehandler_set_meta_content(input_json, (uint32_t)strlen(input_json));
    appCanLogHandlerInit(testCtrlData);

    uint32_t stored_len = 0;
    const char *stored  = test_filehandler_get_meta_content(&stored_len);
    TEST_ASSERT_NOT_NULL(stored);
    TEST_ASSERT_GREATER_THAN_UINT32(0U, stored_len);

    TEST_ASSERT_NOT_NULL(strstr(stored, "\"epoch\":6"));
    TEST_ASSERT_NOT_NULL(strstr(stored, "\"index\":7"));
    TEST_ASSERT_NOT_NULL(strstr(stored, "\"offset\":123"));
    TEST_ASSERT_NOT_NULL(strstr(stored, "\"crc\":42"));
}

/**
 * @brief Tests preallocation creates and sizes log files
 * @details Verifies preallocation runs and sizes log files to configured size
 */
void test_Preallocation_CreatesSizedFiles(void)
{
    const uint32_t file_count = 3U;
    const uint32_t file_size  = 2048U;

    test_ff_reset();
    appCanLogSetFileConfig(file_size, file_count);
    appCanLogHandlerInit(testCtrlData);

    for (uint32_t i = 0; i < file_count; i++)
    {
        char path[64];
        snprintf(path, sizeof(path), "/logs/CAN.LOG%u", i);
        TEST_ASSERT_TRUE(test_ff_file_exists(path));
        TEST_ASSERT_EQUAL_UINT32(file_size, test_ff_get_file_size(path));
    }

    uint8_t prealloc_error = 1U;
    FsCustom_GetPreallocErrorFlag(&prealloc_error);
    TEST_ASSERT_EQUAL_UINT8(0U, prealloc_error);

    appCanLogSetFileConfig(0U, 0U);
}

/**
 * @brief Tests file rotation head/tail wrap behavior
 * @details Forces rotation across 100 files (10 MB each => ~1 GB total capacity)
 */
void test_FileRotation_WrapsHeadTailAtLimit(void)
{
    const uint32_t file_count = 100U;

    /* Limit test to 100 files; size defaults to MAX_LOG_FILE_SIZE (10 MB). */
    appCanLogSetFileConfig(0U, file_count);
    appCanLogHandlerInit(testCtrlData);

    uint32_t capacity = 0;
    FsCustom_GetCanLogCapacity(&capacity);
    TEST_ASSERT_EQUAL_UINT32(file_count, capacity);

    uint32_t head = 0;
    uint32_t tail = 0;

    FsCustom_GetCanLogHeadIndex(&head);
    FsCustom_GetCanLogTailIndex(&tail);
    TEST_ASSERT_EQUAL_UINT32(0U, head);
    TEST_ASSERT_EQUAL_UINT32(0U, tail);

    /* Rollover now occurs after a block flush, not on an idle poll. */
    set_file_size(appCanLogGetLogFileSize());
    mockRunCanTracer = true;
    set_frames_available(1);
    appCanLogHandlerPoll(testCtrlData);
    mockRunCanTracer = false;
    appCanLogHandlerPoll(testCtrlData);
    FsCustom_GetCanLogHeadIndex(&head);
    FsCustom_GetCanLogTailIndex(&tail);
    TEST_ASSERT_EQUAL_UINT32(1U, head);
    TEST_ASSERT_EQUAL_UINT32(0U, tail);

    /* Rotate through remaining slots to force wrap. */
    for (uint32_t i = 1U; i < file_count; i++)
    {
        set_file_size(appCanLogGetLogFileSize());
        mockRunCanTracer = true;
        set_frames_available(1);
        appCanLogHandlerPoll(testCtrlData);
        mockRunCanTracer = false;
        appCanLogHandlerPoll(testCtrlData);
    }

    FsCustom_GetCanLogHeadIndex(&head);
    FsCustom_GetCanLogTailIndex(&tail);
    TEST_ASSERT_EQUAL_UINT32(0U, head);
    TEST_ASSERT_EQUAL_UINT32(1U, tail);

    /* Restore defaults for other tests. */
    appCanLogSetFileConfig(0U, 0U);
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
    RUN_TEST(test_FileRotation_ClosesFileOnSizeExceeded);
    RUN_TEST(test_StorePipeline_WritesFramesToFile);
    RUN_TEST(test_CommitFlushOnStop_PadsAndFlushes);
    RUN_TEST(test_MetaDataPersistence_RoundTrip);
    RUN_TEST(test_Preallocation_CreatesSizedFiles);
    RUN_TEST(test_FileRotation_WrapsHeadTailAtLimit);
    RUN_TEST(test_ErrorHandling_FileOpenFailure);
}

void test_CanLogBuffer()
{
    RunAllTests();
}
