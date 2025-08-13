#pragma once

#include <stdint.h>

enum SPIABS_DEVICE {
    SPIABS_DEVICE_1,
    SPIABS_DEVICE_2,
    SPIABS_DEVICEn
};

uint8_t SpiAbs_Init_Spi1(void);
void * SpiAbs_GetHandle_Spi1(void);

void SpiAbs_ErrorHandler(void);

uint8_t SpiAbs_PwrOn(enum SPIABS_DEVICE dev);
uint8_t SpiAbs_PwrOff(enum SPIABS_DEVICE dev);

uint8_t SpiAbs_CsEnable(enum SPIABS_DEVICE dev);
uint8_t SpiAbs_CsDisable(enum SPIABS_DEVICE dev);

uint8_t SpiAbs_readByte(enum SPIABS_DEVICE dev, uint8_t * resp);
uint8_t SpiAbs_writByte(enum SPIABS_DEVICE dev, const uint8_t *data);
uint8_t SpiAbs_SendReceiveMsg(enum SPIABS_DEVICE dev, const uint8_t * pTxBuffer, uint8_t * pRxBuffer, uint8_t TxBytes);
uint8_t SpiAbs_PollForResponse(enum SPIABS_DEVICE dev, uint8_t * pResponse);

void SpiAbs_Task(void *parameters);
void SpiAbs_TaskControlCallback(uint32_t timeout);
void SpiAbs_TaskSendReceiveCallback(void);

// task specific helper functions:
void SpiAbs_Send_Spi1_Task0(const uint8_t * pTxBuffer, uint8_t * pRxBuffer, uint8_t TxBytes);
void SpiAbs_Send_Spi1_CompleteCallback_Task0(void * context, uint32_t status, const uint8_t *data, uint32_t len);


