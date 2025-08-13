#include <string.h>

#include "unity.h"

#include "spi.h"
#include "SpiAbs.h"

static int g_spi_error_handler_calls = 0;
void SPI_ErrorHandler(void)
{
    g_spi_error_handler_calls++;
}

extern volatile int Spi_Init_return_value;
extern volatile int Spi_Init_call_count;
extern volatile int Spi_SendReceiveMsg_call_count;
extern size_t Spi_SendReceiveMsg_last_len;

void init_driver_instance();
void deinit_driver_instance();

void setUp(void)
{
    g_spi_error_handler_calls     = 0;
    Spi_Init_return_value         = 0;
    Spi_Init_call_count           = 0;
    Spi_SendReceiveMsg_call_count = 0;
    Spi_SendReceiveMsg_last_len   = 0;

    init_driver_instance();
}

void tearDown(void)
{
    deinit_driver_instance();
}

void test_SPI_CreateDriver_sets_interface_and_core_fields(void);
void test_SPI_CreateDriver_bad_cfg_size_calls_error_handler(void);
void test_SPI_CreateDriver_two_instances_unique_and_share_interface_ptr(void);
void test_SPI_Init_calls_hal_and_sets_state(void);
void test_SPI_Send_calls_abs_with_msg_length(void);
void test_SPI_CreateDriver_invalid_device_number(void);
void test_SPI_CreateDriver_all_instances_used(void);
void test_SPI_Init_hal_failure(void);

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_SPI_CreateDriver_sets_interface_and_core_fields);
    RUN_TEST(test_SPI_CreateDriver_bad_cfg_size_calls_error_handler);
    RUN_TEST(
        test_SPI_CreateDriver_two_instances_unique_and_share_interface_ptr
    );
    RUN_TEST(test_SPI_Init_calls_hal_and_sets_state);
    RUN_TEST(test_SPI_Send_calls_abs_with_msg_length);
    RUN_TEST(test_SPI_CreateDriver_invalid_device_number);
    RUN_TEST(test_SPI_CreateDriver_all_instances_used);
    RUN_TEST(test_SPI_Init_hal_failure);

    return UNITY_END();
}

#define SPI_RX_SLOT_REQUIRED_SIZE (515)
#define SPI_TX_SLOT_REQUIRED_SIZE (515)
#define SW_RX_SLOT_COUNT          (3U)
#define SW_TX_SLOT_COUNT          (3U)
#define SW_RX_BIN_COUNT           (2U)
#define SW_TX_BIN_COUNT           (2U)

/* SPI 1 */

#define SPI_TX_SLOT_SIZE                                                       \
    DRV_BUFFER_ALIGNED_SIZE(SPI_SLOT_META_BYTES + SPI_TX_SLOT_REQUIRED_SIZE)
#define SPI_RX_SLOT_SIZE                                                       \
    DRV_BUFFER_ALIGNED_SIZE(SPI_SLOT_META_BYTES + SPI_RX_SLOT_REQUIRED_SIZE)

ALIGN_32BYTES(
    uint8_t Spi1RxBins[SW_RX_BIN_COUNT][SW_RX_SLOT_COUNT][SPI_RX_SLOT_SIZE]
);
ALIGN_32BYTES(
    uint8_t Spi1TxBins[SW_TX_BIN_COUNT][SW_TX_SLOT_COUNT][SPI_TX_SLOT_SIZE]
);

SPI_ASSERT_SIZE(Spi1TxBins[0][0]);
SPI_ASSERT_ALIGNMENT(Spi1TxBins);

SPI_ASSERT_SIZE(Spi1RxBins[0][0]);
SPI_ASSERT_ALIGNMENT(Spi1RxBins);

static SpiConfigType Spi1Cfg = {
    .rx_bin_cnt     = SW_RX_BIN_COUNT,
    .rx_slots       = &Spi1RxBins[0][0][0],
    .rx_slots_cnt   = SW_RX_SLOT_COUNT,
    .rx_slot_stride = SPI_RX_SLOT_SIZE,
    .tx_bin_cnt     = SW_TX_BIN_COUNT,
    .tx_slots       = &Spi1TxBins[0][0][0],
    .tx_slots_cnt   = SW_TX_SLOT_COUNT,
    .tx_slot_stride = SPI_TX_SLOT_SIZE
};

static RingBuffer Spi1TxSlotBuffer[SW_TX_BIN_COUNT] = {
    {.startAddress = &Spi1TxBins[0][0][0],
     .head         = 0,
     .tail         = 0,
     .bufferLength = SW_TX_SLOT_COUNT,
     .elementSize  = sizeof(SpiSlotType),
     .stride       = SPI_TX_SLOT_SIZE,
     .elementCount = 0,
     .isFull       = false},
    {.startAddress = &Spi1TxBins[1][0][0],
     .head         = 0,
     .tail         = 0,
     .bufferLength = SW_TX_SLOT_COUNT,
     .elementSize  = sizeof(SpiSlotType),
     .stride       = SPI_TX_SLOT_SIZE,
     .elementCount = 0,
     .isFull       = false}
};

static SpiBinType Spi1TxBinsRingBuffer[SW_TX_BIN_COUNT] = {
    {.slot_cnt = SW_TX_SLOT_COUNT, .slots = &Spi1TxSlotBuffer[0]},
    {.slot_cnt = SW_TX_SLOT_COUNT, .slots = &Spi1TxSlotBuffer[1]}
};

static RingBuffer Spi1RxSlotBuffer[SW_RX_BIN_COUNT] = {
    {.startAddress = &Spi1RxBins[0][0][0],
     .head         = 0,
     .tail         = 0,
     .bufferLength = SW_RX_SLOT_COUNT,
     .elementSize  = sizeof(SpiSlotType),
     .stride       = SPI_RX_SLOT_SIZE,
     .elementCount = 0,
     .isFull       = false},
    {.startAddress = &Spi1RxBins[1][0][0],
     .head         = 0,
     .tail         = 0,
     .bufferLength = SW_RX_SLOT_COUNT,
     .elementSize  = sizeof(SpiSlotType),
     .stride       = SPI_RX_SLOT_SIZE,
     .elementCount = 0,
     .isFull       = false}
};

static SpiBinType Spi1RxBinsRingBuffer[SW_RX_BIN_COUNT] = {
    {.slot_cnt = SW_RX_SLOT_COUNT, .slots = &Spi1RxSlotBuffer[0]},
    {.slot_cnt = SW_RX_SLOT_COUNT, .slots = &Spi1RxSlotBuffer[1]}
};

static CommDriver Spi1Driver;
static CommDriverConfigType Spi1Config = {
    .config = DRIVER_CFG2,
    .devNbr = COMM_DEVICE_NUMBER_1,
    .driver = (void *)&Spi1Cfg
};

static CommDriver Spi2Driver;

void init_driver_instance()
{
    memset(&Spi1Driver, 0, sizeof(Spi1Driver));
    memset(&Spi2Driver, 0, sizeof(Spi2Driver));
}

void deinit_driver_instance()
{
    if (NULL != Spi1Driver.instance)
    {
        SPI_DestroyDriver(&Spi1Driver);
    }

    if (NULL != Spi2Driver.instance)
    {
        SPI_DestroyDriver(&Spi2Driver);
    }
}

void test_SPI_CreateDriver_sets_interface_and_core_fields(void)
{
    comm_status_t rc;
    SpiInstanceType *inst;

    Spi1Driver.protocol = DRIVER_SPI;

    rc = SPI_CreateDriver(
        &Spi1Driver,
        &Spi1Config,
        sizeof(Spi1Config),
        (uint8_t *)Spi1TxBinsRingBuffer,
        (uint8_t *)Spi1RxBinsRingBuffer
    );

    TEST_ASSERT_EQUAL(COMM_SUCCESS, rc);

    TEST_ASSERT_NOT_NULL(Spi1Driver.interface);
    TEST_ASSERT_EQUAL_PTR(SPI_Init, Spi1Driver.interface->init);
    TEST_ASSERT_EQUAL_PTR(SPI_Send, Spi1Driver.interface->send);
    TEST_ASSERT_EQUAL_PTR(SPI_Read, Spi1Driver.interface->read);
    TEST_ASSERT_EQUAL_PTR(SPI_Ioctl, Spi1Driver.interface->ioctl);

    TEST_ASSERT_EQUAL_PTR(Spi1TxBinsRingBuffer, Spi1Driver.TxFrameBuffer);
    TEST_ASSERT_EQUAL_PTR(Spi1RxBinsRingBuffer, Spi1Driver.RxFrameBuffer);
    TEST_ASSERT_NOT_NULL(Spi1Driver.instance);
    TEST_ASSERT_EQUAL(DRIVER_SPI, Spi1Driver.protocol);
    TEST_ASSERT_EQUAL(DRIVER_STATE_INITIALIZED, Spi1Driver.state);

    inst = (SpiInstanceType *)Spi1Driver.instance;
    TEST_ASSERT_EQUAL_PTR(SPI_1, inst->spi);
}

void test_SPI_CreateDriver_bad_cfg_size_calls_error_handler(void)
{
    Spi1Driver.protocol = DRIVER_SPI;

    (void)SPI_CreateDriver(
        &Spi1Driver,
        &Spi1Config,
        sizeof(Spi1Config) + 1,
        (uint8_t *)Spi1TxBinsRingBuffer,
        (uint8_t *)Spi1RxBinsRingBuffer
    );

    TEST_ASSERT_TRUE_MESSAGE(
        g_spi_error_handler_calls >= 1,
        "SPI_ErrorHandler should be called on cfg size mismatch"
    );
}

void test_SPI_CreateDriver_two_instances_unique_and_share_interface_ptr(void)
{
    SpiInstanceType *inst1_ptr;
    SpiInstanceType *inst2_ptr;

    Spi1Driver.protocol = DRIVER_SPI;

    (void)SPI_CreateDriver(
        &Spi1Driver,
        &Spi1Config,
        sizeof(Spi1Config),
        (uint8_t *)Spi1TxBinsRingBuffer,
        (uint8_t *)Spi1RxBinsRingBuffer
    );
    void *inst1 = Spi1Driver.instance;
    void *ifc1  = Spi1Driver.interface;

    CommDriverConfigType Spi2Config = Spi1Config;
    Spi2Config.devNbr               = COMM_DEVICE_NUMBER_2;

    Spi2Driver.protocol = DRIVER_SPI;
    (void)SPI_CreateDriver(
        &Spi2Driver,
        &Spi2Config,
        sizeof(Spi2Config),
        (uint8_t *)Spi1TxBinsRingBuffer,
        (uint8_t *)Spi1RxBinsRingBuffer
    );

    TEST_ASSERT_NOT_EQUAL(inst1, Spi2Driver.instance);
    TEST_ASSERT_EQUAL_PTR(ifc1, Spi2Driver.interface);

    inst1_ptr = (SpiInstanceType *)inst1;
    inst2_ptr = (SpiInstanceType *)Spi2Driver.instance;

    TEST_ASSERT_EQUAL_PTR(SPI_1, inst1_ptr->spi);
    TEST_ASSERT_EQUAL_PTR(SPI_2, inst2_ptr->spi);
}

void test_SPI_Init_calls_hal_and_sets_state(void)
{
    comm_status_t rc;

    Spi1Driver.protocol = DRIVER_SPI;

    (void)SPI_CreateDriver(
        &Spi1Driver,
        &Spi1Config,
        sizeof(Spi1Config),
        (uint8_t *)Spi1TxBinsRingBuffer,
        (uint8_t *)Spi1RxBinsRingBuffer
    );

    Spi1Driver.state = DRIVER_STATE_UNINITIALIZED;

    rc = Spi1Driver.interface->init(&Spi1Driver);

    TEST_ASSERT_EQUAL(COMM_SUCCESS, rc);
    TEST_ASSERT_EQUAL(1, Spi_Init_call_count);
    TEST_ASSERT_EQUAL(DRIVER_STATE_INITIALIZED, Spi1Driver.state);
}

void test_SPI_Send_calls_abs_with_msg_length(void)
{
    SPI_Message msg       = {0};
    uint8_t test_data1[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    uint8_t test_data2[8] = {11, 12, 13, 14, 15, 16, 17, 18};
    uint8_t test_data3[7] = {21, 22, 23, 24, 25, 26, 27};

    Spi1Driver.protocol = DRIVER_SPI;
    (void)SPI_CreateDriver(
        &Spi1Driver,
        &Spi1Config,
        sizeof(Spi1Config),
        (uint8_t *)Spi1TxBinsRingBuffer,
        (uint8_t *)Spi1RxBinsRingBuffer
    );

    msg.msgBase.length  = sizeof(test_data1);
    msg.msgBase.payload = test_data1;
    (void)Spi1Driver.interface->send(&Spi1Driver, &msg);
    msg.msgBase.length  = 0;
    msg.msgBase.payload = NULL;

    msg.msgBase.length  = sizeof(test_data2);
    msg.msgBase.payload = test_data2;
    (void)Spi1Driver.interface->send(&Spi1Driver, &msg);
    msg.msgBase.length  = 0;
    msg.msgBase.payload = NULL;

    msg.msgBase.length  = sizeof(test_data3);
    msg.msgBase.payload = test_data3;
    (void)Spi1Driver.interface->send(&Spi1Driver, &msg);
    msg.msgBase.length  = 0;
    msg.msgBase.payload = NULL;

    SPI_Poll(&Spi1Driver);

    TEST_ASSERT_EQUAL(1, Spi_SendReceiveMsg_call_count);
    TEST_ASSERT_EQUAL(9, Spi_SendReceiveMsg_last_len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(
        test_data1,
        Spi_SendReceiveMsg_last_tx,
        sizeof(test_data1)
    );

    SPI_Poll(&Spi1Driver);

    TEST_ASSERT_EQUAL(2, Spi_SendReceiveMsg_call_count);
    TEST_ASSERT_EQUAL(8, Spi_SendReceiveMsg_last_len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(
        test_data2,
        Spi_SendReceiveMsg_last_tx,
        sizeof(test_data2)
    );

    SPI_Poll(&Spi1Driver);

    TEST_ASSERT_EQUAL(3, Spi_SendReceiveMsg_call_count);
    TEST_ASSERT_EQUAL(7, Spi_SendReceiveMsg_last_len);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(
        test_data3,
        Spi_SendReceiveMsg_last_tx,
        sizeof(test_data3)
    );
}

void test_SPI_CreateDriver_invalid_device_number(void)
{
    comm_status_t rc;
    CommDriver driver;
    CommDriverConfigType config;
    const uint8_t invalid_device_nmb = 99;

    memset((uint8_t *)&driver, 0x0, sizeof(driver));

    config = Spi1Config;

    config.devNbr = invalid_device_nmb;

    driver.protocol = DRIVER_SPI;

    rc = SPI_CreateDriver(
        &driver,
        &config,
        sizeof(config),
        (uint8_t *)Spi1TxBinsRingBuffer,
        (uint8_t *)Spi1RxBinsRingBuffer
    );

    TEST_ASSERT_EQUAL(COMM_INVALID_PARAMETER, rc);
    TEST_ASSERT_TRUE_MESSAGE(
        g_spi_error_handler_calls >= 1,
        "SPI_ErrorHandler should be called for invalid device number"
    );
}

void test_SPI_CreateDriver_all_instances_used(void)
{
    comm_status_t rc1;
    comm_status_t rc2;
    comm_status_t rc3;
    CommDriverConfigType config1;
    CommDriverConfigType config2;
    CommDriver driver3 = {0};
#if 4 <= (SPI_DRIVER_NUMBERn + 1U)
#    error "SPI_DRIVER_NUMBERn exceeds number of testen drivers!"
#endif

    config1        = Spi1Config;
    config1.devNbr = COMM_DEVICE_NUMBER_1;

    config2        = Spi1Config;
    config2.devNbr = COMM_DEVICE_NUMBER_2;

    Spi1Driver.protocol = DRIVER_SPI;
    Spi1Driver.protocol = DRIVER_SPI;
    driver3.protocol    = DRIVER_SPI;

    // Create first two instances (assuming SPI_MAX_INSTANCES is 2)
    rc1 = SPI_CreateDriver(
        &Spi1Driver,
        &config1,
        sizeof(config1),
        (uint8_t *)Spi1TxBinsRingBuffer,
        (uint8_t *)Spi1RxBinsRingBuffer
    );

    rc2 = SPI_CreateDriver(
        &Spi2Driver,
        &config2,
        sizeof(config2),
        (uint8_t *)Spi1TxBinsRingBuffer,
        (uint8_t *)Spi1RxBinsRingBuffer
    );

    // Try to create third instance - should fail
    g_spi_error_handler_calls = 0;

    rc3 = SPI_CreateDriver(
        &driver3,
        &config1,
        sizeof(config1),
        (uint8_t *)Spi1TxBinsRingBuffer,
        (uint8_t *)Spi1RxBinsRingBuffer
    );

    TEST_ASSERT_TRUE_MESSAGE(
        g_spi_error_handler_calls >= 1,
        "SPI_ErrorHandler should be called when all instances are used"
    );
    TEST_ASSERT_EQUAL(rc1, 0);
    TEST_ASSERT_EQUAL(rc2, 0);
    TEST_ASSERT_EQUAL(rc3, COMM_NO_RESSOURCES);
}

void test_SPI_Init_hal_failure(void)
{
    comm_status_t rc;

    Spi1Driver.protocol = DRIVER_SPI;

    (void)SPI_CreateDriver(
        &Spi1Driver,
        &Spi1Config,
        sizeof(Spi1Config),
        (uint8_t *)Spi1TxBinsRingBuffer,
        (uint8_t *)Spi1RxBinsRingBuffer
    );

    Spi1Driver.state = DRIVER_STATE_UNINITIALIZED;

    Spi_Init_return_value = -1;

    rc = Spi1Driver.interface->init(&Spi1Driver);

    TEST_ASSERT_NOT_EQUAL(COMM_SUCCESS, rc);
    TEST_ASSERT_EQUAL(DRIVER_STATE_UNINITIALIZED, Spi1Driver.state);

    Spi_Init_return_value = 0; // Reset for other tests
}
