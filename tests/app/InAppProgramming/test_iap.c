#include "unity.h"

#include <string.h>

#include "UpdateIngestPipeline.h"
#include "IapIngestAdapter.h"
#include "IapWriter.h"
#include "RamFlashSim.h"
#include "UpdateIngestRegistry.h"

#define TEST_FLASH_BASE (0x08000000u)
#define TEST_FLASH_SIZE (64u * 1024u)
#define TEST_SLOT_ADDR  (TEST_FLASH_BASE + 0x2000u)
#define TEST_SLOT_SIZE  (16u * 1024u)
#define TEST_ERASE_SIZE (256u)
#define TEST_PROG_SIZE  (16u)

static uint8_t TestFlashStorage[TEST_FLASH_SIZE];
static RamFlashSimContextType TestFlashContext;
static IapWriterContextType TestWriterContext;
static IapIngestAdapterContextType TestIngestContext;

static void TestAssertAllEq(const uint8_t *buf, size_t len, uint8_t expected)
{
    for (size_t i = 0; i < len; i++)
    {
        TEST_ASSERT_EQUAL_HEX8(expected, buf[i]);
    }
}

void setUp(void)
{
    const IapWriterStorageOpsType ops =
        RamFlashSim_GetStorageOps(&TestFlashContext);
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

    TEST_ASSERT_EQUAL(
        IAP_WRITER_E_OK,
        IapWriter_Init(&TestWriterContext, &config, &ops)
    );

    TEST_ASSERT_EQUAL(
        UPDATE_INGEST_E_OK,
        IapIngestAdapter_Init(&TestIngestContext, &TestWriterContext)
    );
    TEST_ASSERT_EQUAL(
        UPDATE_INGEST_E_OK,
        UpdateIngestRegistry_Register(
            IapIngestAdapter_GetVTable(),
            &TestIngestContext
        )
    );
}

void tearDown(void)
{
    UpdateIngestRegistry_Clear();
}

static void test_IapPipeline_EndToEnd_MainFlow(void);
static void test_IapPipeline_Negative_OutOfBoundsChunk(void);
static void test_IapPipeline_Negative_IncompleteFinalize(void);

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_IapPipeline_EndToEnd_MainFlow);
    RUN_TEST(test_IapPipeline_Negative_OutOfBoundsChunk);
    RUN_TEST(test_IapPipeline_Negative_IncompleteFinalize);

    return UNITY_END();
}

static void test_IapPipeline_EndToEnd_MainFlow(void)
{
    UpdateIngestPipelineContextType pipeline;
    uint8_t erased_probe[256];
    uint8_t verify[32];
    uint8_t expected[32];
    const uint8_t chunk0[16] = {
        0x31,
        0x32,
        0x33,
        0x34,
        0x35,
        0x36,
        0x37,
        0x38,
        0x39,
        0x3A,
        0x3B,
        0x3C,
        0x3D,
        0x3E,
        0x3F,
        0x40,
    };
    const uint8_t chunk1[16] = {
        0x41,
        0x42,
        0x43,
        0x44,
        0x45,
        0x46,
        0x47,
        0x48,
        0x49,
        0x4A,
        0x4B,
        0x4C,
        0x4D,
        0x4E,
        0x4F,
        0x50,
    };

    RamFlashSim_Fill(
        &TestFlashContext,
        TEST_SLOT_ADDR,
        0x00,
        sizeof(erased_probe)
    );

    TEST_ASSERT_EQUAL(
        UPDATE_INGEST_PIPELINE_E_OK,
        UpdateIngestPipeline_Begin(&pipeline, (uint32_t)sizeof(verify))
    );

    TEST_ASSERT_EQUAL_UINT32(
        (uint32_t)sizeof(verify),
        IapWriter_GetExpectedSize(&TestWriterContext)
    );
    TEST_ASSERT_EQUAL_UINT32(0u, IapWriter_GetReceivedSize(&TestWriterContext));

    TEST_ASSERT_EQUAL(
        IAP_WRITER_STORAGE_E_OK,
        RamFlashSim_CopyOut(
            &TestFlashContext,
            TEST_SLOT_ADDR,
            erased_probe,
            sizeof(erased_probe)
        )
    );
    TestAssertAllEq(erased_probe, sizeof(erased_probe), 0xFF);

    TEST_ASSERT_EQUAL(
        UPDATE_INGEST_PIPELINE_E_OK,
        UpdateIngestPipeline_Push(&pipeline, chunk0, sizeof(chunk0))
    );
    TEST_ASSERT_EQUAL(
        UPDATE_INGEST_PIPELINE_E_OK,
        UpdateIngestPipeline_Push(&pipeline, chunk1, sizeof(chunk1))
    );
    TEST_ASSERT_EQUAL(
        UPDATE_INGEST_PIPELINE_E_OK,
        UpdateIngestPipeline_Finish(&pipeline)
    );

    memcpy(&expected[0], chunk0, sizeof(chunk0));
    memcpy(&expected[16], chunk1, sizeof(chunk1));

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

static void test_IapPipeline_Negative_OutOfBoundsChunk(void)
{
    UpdateIngestPipelineContextType pipeline;
    uint8_t data[32];

    memset(data, 0xA5, sizeof(data));

    TEST_ASSERT_EQUAL(
        UPDATE_INGEST_PIPELINE_E_OK,
        UpdateIngestPipeline_Begin(&pipeline, 16u)
    );
    TEST_ASSERT_EQUAL(
        UPDATE_INGEST_PIPELINE_E_RANGE,
        UpdateIngestPipeline_Push(&pipeline, data, sizeof(data))
    );
    TEST_ASSERT_EQUAL(
        UPDATE_INGEST_PIPELINE_E_STATE,
        UpdateIngestPipeline_Finish(&pipeline)
    );
}

static void test_IapPipeline_Negative_IncompleteFinalize(void)
{
    UpdateIngestPipelineContextType pipeline;
    uint8_t chunk[16];

    memset(chunk, 0x3C, sizeof(chunk));

    TEST_ASSERT_EQUAL(
        UPDATE_INGEST_PIPELINE_E_OK,
        UpdateIngestPipeline_Begin(&pipeline, 32u)
    );
    TEST_ASSERT_EQUAL(
        UPDATE_INGEST_PIPELINE_E_OK,
        UpdateIngestPipeline_Push(&pipeline, chunk, sizeof(chunk))
    );
    TEST_ASSERT_EQUAL(
        UPDATE_INGEST_PIPELINE_E_STATE,
        UpdateIngestPipeline_Finish(&pipeline)
    );
}
