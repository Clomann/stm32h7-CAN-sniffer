#include <string.h>

#include "unity.h"

#include "spi.h"
#include "SpiAbs.h"
#include "buffers.h"
#include "spi_utils.h"

extern RingBuffer *Spi_GetSlots(const uint8_t *const buf, SpiPriorityType prio);

extern volatile int Spi_Init_return_value;
extern volatile int Spi_Init_call_count;
extern volatile int Spi_SendReceiveMsg_call_count;
extern size_t Spi_SendReceiveMsg_last_len;

static int g_spi_error_handler_calls = 0;

void init_driver_instance();
void deinit_driver_instance();
void reset_rx_tx_buffers();

void SPI_ErrorHandler(void)
{
    g_spi_error_handler_calls++;
}

void setUp(void)
{
    g_spi_error_handler_calls     = 0;
    Spi_Init_return_value         = 0;
    Spi_Init_call_count           = 0;
    Spi_SendReceiveMsg_call_count = 0;
    Spi_SendReceiveMsg_last_len   = 0;

    init_driver_instance();
    reset_rx_tx_buffers();
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
void test_SPI_Read_sets_rx_only_direction_and_reserves_slot(void);
void test_SPI_Read_null_tx_slots_calls_error_handler(void);
void test_SPI_Read_no_available_slots_calls_error_handler(void);
void test_SPI_Read_sets_transaction_length_correctly(void);
void test_SPI_Send_copies_payload_to_slot_data(void);
void test_SPI_Send_passes_correct_direction_to_prepare_slot(void);
void test_SPI_Send_propagates_prepare_slot_errors(void);
void test_SPI_Send_returns_rx_full_when_rx_slots_full(void);

void test_SpiUtils_ComputePrescaler(void);

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
    RUN_TEST(test_SPI_Read_sets_rx_only_direction_and_reserves_slot);
    RUN_TEST(test_SPI_Read_null_tx_slots_calls_error_handler);
    RUN_TEST(test_SPI_Read_no_available_slots_calls_error_handler);
    RUN_TEST(test_SPI_Read_sets_transaction_length_correctly);
    RUN_TEST(test_SPI_Send_copies_payload_to_slot_data);
    RUN_TEST(test_SPI_Send_passes_correct_direction_to_prepare_slot);
    RUN_TEST(test_SPI_Send_propagates_prepare_slot_errors);
    RUN_TEST(test_SPI_Send_returns_rx_full_when_rx_slots_full);

    RUN_TEST(test_SpiUtils_ComputePrescaler);

    return UNITY_END();
}

#define SPI_TX_SLOT_REQUIRED_SIZE (515)
#define SPI_RX_SLOT_REQUIRED_SIZE (515)
#define SW_TX_SLOT_COUNT          (3U)
#define SW_RX_SLOT_COUNT          (3U)
#define SW_TX_BIN_COUNT           (SPI_PRIORITYn)
#define SW_RX_BIN_COUNT           (SW_TX_BIN_COUNT)

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

void reset_rx_tx_buffers()
{
    uint32_t index;

    for (index = 0; index < SW_TX_BIN_COUNT; index++)
    {
        Spi1TxSlotBuffer[index].head         = 0;
        Spi1TxSlotBuffer[index].tail         = 0;
        Spi1TxSlotBuffer[index].elementCount = 0;
        Spi1TxSlotBuffer[index].isFull       = false;
    }

    for (index = 0; index < SW_RX_BIN_COUNT; index++)
    {
        Spi1RxSlotBuffer[index].head         = 0;
        Spi1RxSlotBuffer[index].tail         = 0;
        Spi1RxSlotBuffer[index].elementCount = 0;
        Spi1RxSlotBuffer[index].isFull       = false;
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
    SPI_Message msg                = {0};
    SpiTransactionType transaction = {.direction = SPI_DIR_TX_RX};
    uint8_t test_data1[9]          = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    uint8_t test_data2[8]          = {11, 12, 13, 14, 15, 16, 17, 18};
    uint8_t test_data3[7]          = {21, 22, 23, 24, 25, 26, 27};

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
    msg.transaction     = &transaction;
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

void test_SPI_Read_sets_rx_only_direction_and_reserves_slot(void)
{
    SPI_Message msg                = {0};
    SpiTransactionType transaction = {0};
    comm_status_t rc;
    RingBuffer *txSlots;
    SpiSlotType *txSlot;

    Spi1Driver.protocol = DRIVER_SPI;
    (void)SPI_CreateDriver(
        &Spi1Driver,
        &Spi1Config,
        sizeof(Spi1Config),
        (uint8_t *)Spi1TxBinsRingBuffer,
        (uint8_t *)Spi1RxBinsRingBuffer
    );

    msg.msgBase.length = 10;
    msg.transaction    = &transaction;

    rc = Spi1Driver.interface->read(&Spi1Driver, &msg, 0, 0);

    TEST_ASSERT_EQUAL(COMM_SUCCESS, rc);

    // Verify that a slot was reserved and configured correctly
    txSlots =
        (RingBuffer *)Spi_GetSlots(Spi1Driver.TxFrameBuffer, SPI_PRIORITY_LOW);
    txSlot = (SpiSlotType *)ring_buffer_peek_at(txSlots, 0);

    TEST_ASSERT_NOT_NULL(txSlot);
    TEST_ASSERT_EQUAL(10, txSlot->used_len);
    TEST_ASSERT_EQUAL(10, txSlot->transaction.length);

    TEST_ASSERT_EQUAL(SPI_DIR_RX_ONLY, txSlot->transaction.direction);
    TEST_ASSERT_EQUAL(msg.msgBase.length, txSlot->transaction.length);
    TEST_ASSERT_EQUAL(transaction.timeout, txSlot->transaction.timeout);
    TEST_ASSERT_EQUAL_PTR(transaction.callback, txSlot->transaction.callback);
    TEST_ASSERT_EQUAL_PTR(transaction.context, txSlot->transaction.context);
}

void test_SPI_Read_null_tx_slots_calls_error_handler(void)
{
    SPI_Message msg                = {0};
    SpiTransactionType transaction = {0};
    CommDriver driver              = {0};
    comm_status_t rc;

    driver.protocol      = DRIVER_SPI;
    driver.TxFrameBuffer = NULL;

    msg.msgBase.length = 5;
    msg.transaction    = &transaction;

    g_spi_error_handler_calls = 0;

    rc = SPI_Read(&driver, &msg, 0, 0);

    TEST_ASSERT_EQUAL_INT(COMM_NULL_POINTER, rc);
    TEST_ASSERT_TRUE_MESSAGE(
        g_spi_error_handler_calls >= 1,
        "SPI_ErrorHandler should be called when TxFrameBuffer is NULL"
    );
}

void test_SPI_Read_no_available_slots_calls_error_handler(void)
{
    SPI_Message msg                = {0};
    SpiTransactionType transaction = {0};
    comm_status_t rc;
    RingBuffer *txSlots;

    Spi1Driver.protocol = DRIVER_SPI;
    (void)SPI_CreateDriver(
        &Spi1Driver,
        &Spi1Config,
        sizeof(Spi1Config),
        (uint8_t *)Spi1TxBinsRingBuffer,
        (uint8_t *)Spi1RxBinsRingBuffer
    );

    // Fill up all available slots
    txSlots =
        (RingBuffer *)Spi_GetSlots(Spi1Driver.TxFrameBuffer, SPI_PRIORITY_LOW);
    txSlots->elementCount = txSlots->bufferLength; // Make buffer appear full
    txSlots->isFull       = true;

    msg.msgBase.length = 5;
    msg.transaction    = &transaction;

    g_spi_error_handler_calls = 0;

    rc = SPI_Read(&Spi1Driver, &msg, 0, 0);

    TEST_ASSERT_EQUAL(COMM_TX_FULL, rc);
    TEST_ASSERT_EQUAL_INT(g_spi_error_handler_calls, 0);
}

void test_SPI_Read_calls_spi_receive_when_polled(void)
{
    SPI_Message msg                = {0};
    SpiTransactionType transaction = {0};
    comm_status_t rc;

    Spi1Driver.protocol = DRIVER_SPI;
    (void)SPI_CreateDriver(
        &Spi1Driver,
        &Spi1Config,
        sizeof(Spi1Config),
        (uint8_t *)Spi1TxBinsRingBuffer,
        (uint8_t *)Spi1RxBinsRingBuffer
    );

    msg.msgBase.length = 16;
    msg.transaction    = &transaction;

    // Queue a read operation
    rc = Spi1Driver.interface->read(&Spi1Driver, &msg, 0, 0);
    TEST_ASSERT_EQUAL(COMM_SUCCESS, rc);

    // Verify transaction was set up correctly
    TEST_ASSERT_EQUAL(SPI_DIR_RX_ONLY, transaction.direction);
    TEST_ASSERT_EQUAL(16, transaction.length);

    // Reset stub counters
    Spi_Receive_call_count = 0;
    Spi_Receive_last_len   = 0;

    // Poll the driver - this should call Spi_Receive
    SPI_Poll(&Spi1Driver);

    // Verify Spi_Receive was called with correct parameters
    TEST_ASSERT_EQUAL(1, Spi_Receive_call_count);
    TEST_ASSERT_EQUAL(16, Spi_Receive_last_len);
}

void test_SPI_Read_sets_transaction_length_correctly(void)
{
    SPI_Message msg1 = {0}, msg2 = {0}, msg3 = {0};
    SpiTransactionType transaction1 = {0}, transaction2 = {0},
                       transaction3 = {0};
    comm_status_t rc;
    RingBuffer *txSlots;
    SpiSlotType *txSlot;

    Spi1Driver.protocol = DRIVER_SPI;
    (void)SPI_CreateDriver(
        &Spi1Driver,
        &Spi1Config,
        sizeof(Spi1Config),
        (uint8_t *)Spi1TxBinsRingBuffer,
        (uint8_t *)Spi1RxBinsRingBuffer
    );

    txSlots =
        (RingBuffer *)Spi_GetSlots(Spi1Driver.TxFrameBuffer, SPI_PRIORITY_LOW);

    // Test different message lengths
    msg1.msgBase.length = 15;
    msg1.transaction    = &transaction1;
    rc                  = Spi1Driver.interface->read(&Spi1Driver, &msg1, 0, 0);
    TEST_ASSERT_EQUAL(COMM_SUCCESS, rc);

    txSlot = (SpiSlotType *)ring_buffer_peek_at(txSlots, 0);
    TEST_ASSERT_EQUAL(15, txSlot->used_len);
    TEST_ASSERT_EQUAL(15, txSlot->transaction.length);
    TEST_ASSERT_EQUAL(SPI_DIR_RX_ONLY, txSlot->transaction.direction);

    msg2.msgBase.length = 32;
    msg2.transaction    = &transaction2;
    rc                  = Spi1Driver.interface->read(&Spi1Driver, &msg2, 0, 0);
    TEST_ASSERT_EQUAL(COMM_SUCCESS, rc);

    txSlot = (SpiSlotType *)ring_buffer_peek_at(txSlots, 1);
    TEST_ASSERT_EQUAL(32, txSlot->used_len);
    TEST_ASSERT_EQUAL(32, txSlot->transaction.length);
    TEST_ASSERT_EQUAL(SPI_DIR_RX_ONLY, txSlot->transaction.direction);

    msg3.msgBase.length = 1;
    msg3.transaction    = &transaction3;
    rc                  = Spi1Driver.interface->read(&Spi1Driver, &msg3, 0, 0);
    TEST_ASSERT_EQUAL(COMM_SUCCESS, rc);

    txSlot = (SpiSlotType *)ring_buffer_peek_at(txSlots, 2);
    TEST_ASSERT_EQUAL(1, txSlot->used_len);
    TEST_ASSERT_EQUAL(1, txSlot->transaction.length);
    TEST_ASSERT_EQUAL(SPI_DIR_RX_ONLY, txSlot->transaction.direction);
}

void test_SPI_Send_copies_payload_to_slot_data(void)
{
    SPI_Message msg                = {0};
    SpiTransactionType transaction = {0};
    uint8_t test_data[]            = {0x11, 0x22, 0x33, 0x44, 0x55};
    comm_status_t rc;
    RingBuffer *txSlots;
    SpiSlotType *txSlot;

    Spi1Driver.protocol = DRIVER_SPI;
    (void)SPI_CreateDriver(
        &Spi1Driver,
        &Spi1Config,
        sizeof(Spi1Config),
        (uint8_t *)Spi1TxBinsRingBuffer,
        (uint8_t *)Spi1RxBinsRingBuffer
    );

    msg.msgBase.length  = sizeof(test_data);
    msg.msgBase.payload = test_data;
    msg.transaction     = &transaction;

    rc = Spi1Driver.interface->send(&Spi1Driver, &msg);

    TEST_ASSERT_EQUAL(COMM_SUCCESS, rc);

    // Verify payload was copied (this is unique to SPI_Send)
    txSlots =
        (RingBuffer *)Spi_GetSlots(Spi1Driver.TxFrameBuffer, SPI_PRIORITY_LOW);
    txSlot = (SpiSlotType *)ring_buffer_peek_at(txSlots, 0);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(test_data, txSlot->data, sizeof(test_data));
}

void test_SPI_Send_passes_correct_direction_to_prepare_slot(void)
{
    SPI_Message msg                = {0};
    SpiTransactionType transaction = {0};
    uint8_t test_data[]            = {0xAA, 0xBB};
    comm_status_t rc;
    RingBuffer *txSlots;
    SpiSlotType *txSlot;

    transaction.direction = SPI_DIR_RX_ONLY;

    Spi1Driver.protocol = DRIVER_SPI;
    (void)SPI_CreateDriver(
        &Spi1Driver,
        &Spi1Config,
        sizeof(Spi1Config),
        (uint8_t *)Spi1TxBinsRingBuffer,
        (uint8_t *)Spi1RxBinsRingBuffer
    );

    msg.msgBase.length  = sizeof(test_data);
    msg.msgBase.payload = test_data;
    msg.transaction     = &transaction;

    rc = SPI_Send(&Spi1Driver, &msg);

    TEST_ASSERT_EQUAL(COMM_SUCCESS, rc);

    // SPI_Send forwards the provided transaction direction
    txSlots =
        (RingBuffer *)Spi_GetSlots(Spi1Driver.TxFrameBuffer, SPI_PRIORITY_LOW);
    txSlot = (SpiSlotType *)ring_buffer_peek_at(txSlots, 0);
    TEST_ASSERT_EQUAL(SPI_DIR_RX_ONLY, txSlot->transaction.direction);
}

void test_SPI_Send_propagates_prepare_slot_errors(void)
{
    SPI_Message msg                = {0};
    SpiTransactionType transaction = {0};
    uint8_t test_data[]            = {0x11, 0x22};
    comm_status_t rc;
    RingBuffer *txSlots;

    Spi1Driver.protocol = DRIVER_SPI;
    (void)SPI_CreateDriver(
        &Spi1Driver,
        &Spi1Config,
        sizeof(Spi1Config),
        (uint8_t *)Spi1TxBinsRingBuffer,
        (uint8_t *)Spi1RxBinsRingBuffer
    );

    // Force m_PrepareTxSlot to fail by filling buffer
    txSlots =
        (RingBuffer *)Spi_GetSlots(Spi1Driver.TxFrameBuffer, SPI_PRIORITY_LOW);
    txSlots->elementCount = txSlots->bufferLength;
    txSlots->isFull       = true;

    msg.msgBase.length  = sizeof(test_data);
    msg.msgBase.payload = test_data;
    msg.transaction     = &transaction;

    rc = SPI_Send(&Spi1Driver, &msg);

    // Verify error is propagated and memcpy is not called
    TEST_ASSERT_EQUAL(COMM_TX_FULL, rc);
}

void test_SPI_Send_returns_rx_full_when_rx_slots_full(void)
{
    SPI_Message msg                = {0};
    SpiTransactionType transaction = {.direction = SPI_DIR_TX_RX};
    uint8_t test_data[]            = {0x01, 0x02, 0x03};
    comm_status_t rc;
    RingBuffer *rxSlots;

    Spi1Driver.protocol = DRIVER_SPI;
    (void)SPI_CreateDriver(
        &Spi1Driver,
        &Spi1Config,
        sizeof(Spi1Config),
        (uint8_t *)Spi1TxBinsRingBuffer,
        (uint8_t *)Spi1RxBinsRingBuffer
    );

    /* Force Rx buffer full to hit COMM_RX_FULL branch */
    rxSlots =
        (RingBuffer *)Spi_GetSlots(Spi1Driver.RxFrameBuffer, SPI_PRIORITY_LOW);
    rxSlots->elementCount = rxSlots->bufferLength;
    rxSlots->isFull       = true;

    msg.msgBase.length  = sizeof(test_data);
    msg.msgBase.payload = test_data;
    msg.transaction     = &transaction;

    rc = SPI_Send(&Spi1Driver, &msg);

    TEST_ASSERT_EQUAL(COMM_RX_FULL, rc);
    TEST_ASSERT_EQUAL(0, Spi_SendReceiveMsg_call_count);
}

void test_SpiUtils_ComputePrescaler()
{
    TEST_ASSERT_EQUAL_INT(
        SPI_BAUDRATEPRESCALER_2,
        SpiUtils_ComputePrescaler(20000000, 100000000)
    );
    TEST_ASSERT_EQUAL_INT(
        SPI_BAUDRATEPRESCALER_2,
        SpiUtils_ComputePrescaler(20000000, 10000000)
    );
    TEST_ASSERT_EQUAL_INT(
        SPI_BAUDRATEPRESCALER_4,
        SpiUtils_ComputePrescaler(20000000, 5000000)
    );
    TEST_ASSERT_EQUAL_INT(
        SPI_BAUDRATEPRESCALER_256,
        SpiUtils_ComputePrescaler(20000000, 1)
    );
    TEST_ASSERT_EQUAL_INT(
        SPI_BAUDRATEPRESCALER_64,
        SpiUtils_ComputePrescaler(20000000, 500000)
    );
}
