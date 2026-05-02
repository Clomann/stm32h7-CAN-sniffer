#include "unity.h"

#include <string.h>

#include "IapWriter.h"
#include "RamFlashSim.h"

#define TEST_FLASH_BASE (0x08000000u)
#define TEST_FLASH_SIZE (64u * 1024u)
#define TEST_SLOT_ADDR  (TEST_FLASH_BASE + 0x2000u)
#define TEST_SLOT_SIZE  (16u * 1024u)
#define TEST_ERASE_SIZE (256u)
#define TEST_PROG_SIZE  (16u)

static uint8_t TestFlashStorage[TEST_FLASH_SIZE];
static RamFlashSimContextType TestFlashContext;
static IapWriterContextType TestWriterContext;

typedef struct
{
    uint32_t image_size;
    const uint8_t *chunk0;
    size_t chunk0_len;
    const uint8_t *chunk1;
    size_t chunk1_len;
    uint8_t corrupt_before_finalize;
    uint32_t corrupt_offset;
    uint8_t corrupt_value;
    IapWriterStatusType expected_write0_status;
    IapWriterStatusType expected_write1_status;
    IapWriterStatusType expected_finalize_status;
} IapUploadScenarioType;

static void TestAssertAllEq(const uint8_t *buf, size_t len, uint8_t expected)
{
    for (size_t i = 0; i < len; i++)
    {
        TEST_ASSERT_EQUAL_HEX8(expected, buf[i]);
    }
}

static void TestRunUploadScenario(const IapUploadScenarioType *scenario)
{
    IapWriterStatusType write_status0 = IAP_WRITER_E_PARAM;
    IapWriterStatusType write_status1 = IAP_WRITER_E_PARAM;
    IapWriterStatusType finalize_status = IAP_WRITER_E_PARAM;
    uint32_t offset = 0u;

    TEST_ASSERT_NOT_NULL(scenario);
    TEST_ASSERT_EQUAL(
        IAP_WRITER_E_OK,
        IapWriter_Begin(&TestWriterContext, scenario->image_size)
    );

    if (scenario->chunk0 != NULL && scenario->chunk0_len > 0u)
    {
        write_status0 = IapWriter_WriteChunk(
            &TestWriterContext,
            0u,
            scenario->chunk0,
            scenario->chunk0_len
        );
        TEST_ASSERT_EQUAL(scenario->expected_write0_status, write_status0);
        if (write_status0 == IAP_WRITER_E_OK)
        {
            offset += (uint32_t)scenario->chunk0_len;
        }
    }

    if (scenario->chunk1 != NULL && scenario->chunk1_len > 0u)
    {
        write_status1 = IapWriter_WriteChunk(
            &TestWriterContext,
            offset,
            scenario->chunk1,
            scenario->chunk1_len
        );
        TEST_ASSERT_EQUAL(scenario->expected_write1_status, write_status1);
    }

    if (scenario->corrupt_before_finalize != 0u)
    {
        RamFlashSim_CorruptByte(
            &TestFlashContext,
            TEST_SLOT_ADDR + scenario->corrupt_offset,
            scenario->corrupt_value
        );
    }

    finalize_status = IapWriter_FinalizeAndVerify(&TestWriterContext);
    TEST_ASSERT_EQUAL(scenario->expected_finalize_status, finalize_status);
}

void setUp(void)
{
    const IapWriterConfigType config = {
        .slot_addr  = TEST_SLOT_ADDR,
        .slot_size  = TEST_SLOT_SIZE,
        .erase_size = 0u,
        .prog_size  = 0u,
    };

    RamFlashSim_Init(
        &TestFlashContext,
        TestFlashStorage,
        sizeof(TestFlashStorage),
        TEST_FLASH_BASE,
        TEST_ERASE_SIZE,
        TEST_PROG_SIZE
    );

    const IapWriterStorageOpsType ops =
        RamFlashSim_GetStorageOps(&TestFlashContext);
    TEST_ASSERT_EQUAL(
        IAP_WRITER_E_OK,
        IapWriter_Init(&TestWriterContext, &config, &ops)
    );
}

void tearDown(void)
{
}

static void test_IapWriter_BeginErasesRequestedRange(void);
static void test_IapWriter_InitRejectsMissingPropertyProvider(void);
static void test_IapWriter_EndToEnd_HappyPath(void);
static void test_IapWriter_EndToEnd_WriteOutOfBoundsRejected(void);
static void test_IapWriter_EndToEnd_IncompleteImageRejectedOnFinalize(void);
static void test_IapWriter_EndToEnd_ReadbackCorruptionDetected(void);

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_IapWriter_BeginErasesRequestedRange);
    RUN_TEST(test_IapWriter_InitRejectsMissingPropertyProvider);
    RUN_TEST(test_IapWriter_EndToEnd_HappyPath);
    RUN_TEST(test_IapWriter_EndToEnd_WriteOutOfBoundsRejected);
    RUN_TEST(test_IapWriter_EndToEnd_IncompleteImageRejectedOnFinalize);
    RUN_TEST(test_IapWriter_EndToEnd_ReadbackCorruptionDetected);

    return UNITY_END();
}

static void test_IapWriter_BeginErasesRequestedRange(void)
{
    uint8_t verify[0x300];

    RamFlashSim_Fill(&TestFlashContext, TEST_SLOT_ADDR, 0x00, sizeof(verify));

    TEST_ASSERT_EQUAL(
        IAP_WRITER_E_OK,
        IapWriter_Begin(&TestWriterContext, 0x280u)
    );
    TEST_ASSERT_EQUAL_UINT32(
        0x280u,
        IapWriter_GetExpectedSize(&TestWriterContext)
    );
    TEST_ASSERT_EQUAL_UINT32(0u, IapWriter_GetReceivedSize(&TestWriterContext));

    TEST_ASSERT_EQUAL(
        IAP_WRITER_STORAGE_E_OK,
        RamFlashSim_CopyOut(
            &TestFlashContext,
            TEST_SLOT_ADDR,
            verify,
            sizeof(verify)
        )
    );
    TestAssertAllEq(verify, sizeof(verify), 0xFF);
}

static void test_IapWriter_InitRejectsMissingPropertyProvider(void)
{
    IapWriterContextType context;
    IapWriterStorageOpsType ops = RamFlashSim_GetStorageOps(&TestFlashContext);
    const IapWriterConfigType config = {
        .slot_addr  = TEST_SLOT_ADDR,
        .slot_size  = TEST_SLOT_SIZE,
        .erase_size = 0u,
        .prog_size  = 0u,
    };

    ops.get_property = NULL;

    TEST_ASSERT_EQUAL(
        IAP_WRITER_E_PARAM,
        IapWriter_Init(&context, &config, &ops)
    );
}

static void test_IapWriter_EndToEnd_HappyPath(void)
{
    static const uint8_t image_chunk0[16] = {
        0x10,
        0x11,
        0x12,
        0x13,
        0x14,
        0x15,
        0x16,
        0x17,
        0x18,
        0x19,
        0x1A,
        0x1B,
        0x1C,
        0x1D,
        0x1E,
        0x1F,
    };
    static const uint8_t image_chunk1[16] = {
        0x20,
        0x21,
        0x22,
        0x23,
        0x24,
        0x25,
        0x26,
        0x27,
        0x28,
        0x29,
        0x2A,
        0x2B,
        0x2C,
        0x2D,
        0x2E,
        0x2F,
    };
    uint8_t verify[32];
    uint8_t expected[32];
    const IapUploadScenarioType scenario = {
        .image_size                = 32u,
        .chunk0                    = image_chunk0,
        .chunk0_len                = sizeof(image_chunk0),
        .chunk1                    = image_chunk1,
        .chunk1_len                = sizeof(image_chunk1),
        .corrupt_before_finalize   = 0u,
        .corrupt_offset            = 0u,
        .corrupt_value             = 0u,
        .expected_write0_status    = IAP_WRITER_E_OK,
        .expected_write1_status    = IAP_WRITER_E_OK,
        .expected_finalize_status  = IAP_WRITER_E_OK,
    };

    memcpy(&expected[0], image_chunk0, sizeof(image_chunk0));
    memcpy(&expected[16], image_chunk1, sizeof(image_chunk1));
    TestRunUploadScenario(&scenario);

    TEST_ASSERT_EQUAL(
        IAP_WRITER_STORAGE_E_OK,
        RamFlashSim_CopyOut(
            &TestFlashContext,
            TEST_SLOT_ADDR,
            verify,
            sizeof(verify)
        )
    );
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, verify, sizeof(verify));
}

static void test_IapWriter_EndToEnd_WriteOutOfBoundsRejected(void)
{
    uint8_t data[32];
    const IapUploadScenarioType scenario = {
        .image_size                = 16u,
        .chunk0                    = data,
        .chunk0_len                = sizeof(data),
        .chunk1                    = NULL,
        .chunk1_len                = 0u,
        .corrupt_before_finalize   = 0u,
        .corrupt_offset            = 0u,
        .corrupt_value             = 0u,
        .expected_write0_status    = IAP_WRITER_E_RANGE,
        .expected_write1_status    = IAP_WRITER_E_PARAM,
        .expected_finalize_status  = IAP_WRITER_E_STATE,
    };

    memset(data, 0xA5, sizeof(data));
    TestRunUploadScenario(&scenario);
}

static void test_IapWriter_EndToEnd_IncompleteImageRejectedOnFinalize(void)
{
    uint8_t chunk[16];
    const IapUploadScenarioType scenario = {
        .image_size                = 32u,
        .chunk0                    = chunk,
        .chunk0_len                = sizeof(chunk),
        .chunk1                    = NULL,
        .chunk1_len                = 0u,
        .corrupt_before_finalize   = 0u,
        .corrupt_offset            = 0u,
        .corrupt_value             = 0u,
        .expected_write0_status    = IAP_WRITER_E_OK,
        .expected_write1_status    = IAP_WRITER_E_PARAM,
        .expected_finalize_status  = IAP_WRITER_E_STATE,
    };

    memset(chunk, 0x3C, sizeof(chunk));
    TestRunUploadScenario(&scenario);
}

static void test_IapWriter_EndToEnd_ReadbackCorruptionDetected(void)
{
    uint8_t data[32];
    const IapUploadScenarioType scenario = {
        .image_size                = 32u,
        .chunk0                    = data,
        .chunk0_len                = 16u,
        .chunk1                    = &data[16],
        .chunk1_len                = 16u,
        .corrupt_before_finalize   = 1u,
        .corrupt_offset            = 5u,
        .corrupt_value             = 0x00u,
        .expected_write0_status    = IAP_WRITER_E_OK,
        .expected_write1_status    = IAP_WRITER_E_OK,
        .expected_finalize_status  = IAP_WRITER_E_VERIFY,
    };

    for (size_t i = 0; i < sizeof(data); i++)
    {
        data[i] = (uint8_t)(i + 1u);
    }
    TestRunUploadScenario(&scenario);
}
