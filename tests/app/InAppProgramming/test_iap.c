#include "unity.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "UpdateIngestPipeline.h"
#include "IapIngestAdapter.h"
#include "IapWriter.h"
#include "RamFlashSim.h"
#include "UpdateIngestRegistry.h"

#define TEST_FLASH_BASE      (0x08000000u)
#define TEST_FLASH_SIZE      (768u * 1024u)
#define TEST_SLOT_ADDR       (TEST_FLASH_BASE + 0x2000u)
#define TEST_SLOT_SIZE       (512u * 1024u)
#define TEST_ERASE_SIZE      (256u)
#define TEST_PROG_SIZE       (16u)
#define TEST_HTTP_CHUNK_SIZE (0x519u)

#ifndef IAP_TEST_IMAGE_PATH
#define IAP_TEST_IMAGE_PATH                                                    \
    "tests/app/InAppProgramming/artifacts/"                                    \
    "SPI_FullDuplex_ComDMA_CM7_app-signed-encrypted-padded.bin"
#endif

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

static uint8_t *LoadIapArtifactOrFail(size_t *image_size)
{
    FILE *f          = NULL;
    long file_size   = 0L;
    uint8_t *fixture = NULL;
    size_t read_n    = 0u;

    TEST_ASSERT_NOT_NULL(image_size);

    f = fopen(IAP_TEST_IMAGE_PATH, "rb");
    TEST_ASSERT_NOT_NULL_MESSAGE(
        f,
        "IAP test artifact not found. Ensure IAP_TEST_IMAGE_PATH points to "
        "SPI_FullDuplex_ComDMA_CM7_app-signed-encrypted-padded.bin."
    );

    TEST_ASSERT_EQUAL_MESSAGE(0, fseek(f, 0L, SEEK_END), "Failed to seek EOF.");
    file_size = ftell(f);
    TEST_ASSERT_TRUE_MESSAGE(
        file_size > 0L,
        "Artifact file must not be empty."
    );
    TEST_ASSERT_EQUAL_MESSAGE(0, fseek(f, 0L, SEEK_SET), "Failed to rewind.");

    fixture = (uint8_t *)malloc((size_t)file_size);
    TEST_ASSERT_NOT_NULL_MESSAGE(
        fixture,
        "Failed to allocate artifact buffer."
    );

    read_n = fread(fixture, 1u, (size_t)file_size, f);
    TEST_ASSERT_EQUAL_MESSAGE(
        (size_t)file_size,
        read_n,
        "Failed to read complete artifact file."
    );
    fclose(f);

    *image_size = (size_t)file_size;
    return fixture;
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
        IAP_UPDATE_INGEST_E_OK,
        IapIngestAdapter_Init(&TestIngestContext, &TestWriterContext)
    );
    TEST_ASSERT_EQUAL(
        IAP_UPDATE_INGEST_E_OK,
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
static void test_IapIngestAdapter_Negative_NonContinuousOffset(void);

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_IapPipeline_EndToEnd_MainFlow);
    RUN_TEST(test_IapPipeline_Negative_OutOfBoundsChunk);
    RUN_TEST(test_IapPipeline_Negative_IncompleteFinalize);
    RUN_TEST(test_IapIngestAdapter_Negative_NonContinuousOffset);

    return UNITY_END();
}

static void test_IapPipeline_EndToEnd_MainFlow(void)
{
    UpdateIngestPipelineContextType pipeline;
    uint8_t erased_probe[256];
    uint8_t *image    = NULL;
    uint8_t *verify   = NULL;
    size_t image_size = 0u;
    size_t pushed     = 0u;

    image = LoadIapArtifactOrFail(&image_size);
    TEST_ASSERT_TRUE_MESSAGE(
        image_size <= (size_t)UINT32_MAX,
        "Artifact exceeds max size supported by UpdateIngestPipeline_Begin."
    );
    TEST_ASSERT_TRUE_MESSAGE(
        image_size <= (size_t)TEST_SLOT_SIZE,
        "Artifact does not fit in configured test IAP slot."
    );
    TEST_ASSERT_EQUAL_UINT32(
        0u,
        (uint32_t)(image_size % (size_t)TEST_PROG_SIZE)
    );

    RamFlashSim_Fill(
        &TestFlashContext,
        TEST_SLOT_ADDR,
        0x00,
        sizeof(erased_probe)
    );

    TEST_ASSERT_EQUAL(
        UPDATE_INGEST_PIPELINE_E_OK,
        UpdateIngestPipeline_Begin(&pipeline, (uint32_t)image_size)
    );

    TEST_ASSERT_EQUAL_UINT32(
        (uint32_t)image_size,
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

    while (pushed < image_size)
    {
        size_t chunk_len = image_size - pushed;
        if (chunk_len > TEST_HTTP_CHUNK_SIZE)
        {
            chunk_len = TEST_HTTP_CHUNK_SIZE;
        }

        TEST_ASSERT_EQUAL(
            UPDATE_INGEST_PIPELINE_E_OK,
            UpdateIngestPipeline_Push(&pipeline, &image[pushed], chunk_len)
        );
        pushed += chunk_len;
    }

    TEST_ASSERT_EQUAL(
        UPDATE_INGEST_PIPELINE_E_OK,
        UpdateIngestPipeline_Finish(&pipeline)
    );

    verify = (uint8_t *)malloc(image_size);
    TEST_ASSERT_NOT_NULL_MESSAGE(
        verify,
        "Failed to allocate verification buffer for IAP artifact."
    );

    TEST_ASSERT_EQUAL(
        IAP_WRITER_STORAGE_E_OK,
        RamFlashSim_CopyOut(
            &TestFlashContext,
            TEST_SLOT_ADDR,
            verify,
            image_size
        )
    );
    TEST_ASSERT_EQUAL_UINT8_ARRAY(image, verify, image_size);

    free(verify);
    free(image);
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

static void test_IapIngestAdapter_Negative_NonContinuousOffset(void)
{
    uint8_t chunk[TEST_HTTP_CHUNK_SIZE];
    const UpdateIngestVTableType *vtable = IapIngestAdapter_GetVTable();
    const uint32_t wrong_offset =
        (uint32_t)TEST_HTTP_CHUNK_SIZE + TEST_PROG_SIZE;

    memset(chunk, 0x5Au, sizeof(chunk));

    TEST_ASSERT_NOT_NULL(vtable);
    TEST_ASSERT_EQUAL(
        IAP_UPDATE_INGEST_E_OK,
        vtable->begin(&TestIngestContext, 4096u)
    );
    TEST_ASSERT_EQUAL(
        IAP_UPDATE_INGEST_E_OK,
        vtable->write_chunk(&TestIngestContext, 0u, chunk, sizeof(chunk))
    );
    TEST_ASSERT_EQUAL(
        IAP_UPDATE_INGEST_E_SEQUENCE,
        vtable->write_chunk(
            &TestIngestContext,
            wrong_offset,
            chunk,
            sizeof(chunk)
        )
    );
    TEST_ASSERT_EQUAL(
        IAP_UPDATE_INGEST_E_OK,
        vtable->abort(&TestIngestContext)
    );
}
