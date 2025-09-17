#pragma once

#include <stdint.h>

enum SPIABS_DEVICE {
    SPIABS_DEVICE_1,
    SPIABS_DEVICE_2,
    SPIABS_DEVICEn
};

extern uint8_t aRxSpiDummy[1024];

uint8_t SpiAbs_Init_Spi1(void);
void * SpiAbs_GetHandle_Spi1(void);

void SpiAbs_ErrorHandler(void);

uint8_t SpiAbs_PwrOn(enum SPIABS_DEVICE dev);
uint8_t SpiAbs_PwrOff(enum SPIABS_DEVICE dev);

uint8_t SpiAbs_CsEnable(enum SPIABS_DEVICE dev);
uint8_t SpiAbs_CsDisable(enum SPIABS_DEVICE dev);

uint8_t SpiAbs_readByte(enum SPIABS_DEVICE dev, uint8_t * resp);
uint8_t SpiAbs_writByte(enum SPIABS_DEVICE dev, const uint8_t *data);
uint8_t SpiAbs_SendReceiveMsg(
    enum SPIABS_DEVICE dev, 
    const uint8_t * pTxBuffer, 
    uint8_t * pRxBuffer, 
    uint16_t TxBytes);
uint8_t SpiAbs_PollForResponse(enum SPIABS_DEVICE dev, uint8_t * pResponse);
uint8_t SpiAbs_Send_Spi1_Task0(
    const uint8_t * data,
    uint16_t bytes);

uint8_t SpiAbs_Receive_Spi1_Task0(
    uint8_t * data,
    uint16_t bytes);

uint8_t SpiAbs_GoHighSpeed(enum SPIABS_DEVICE dev);

uint8_t SpiAbs_Poll(void);

void SpiAbs_Task(void *parameters);

void __attribute__((weak)) SpiAbs_TaskControlCallback(uint32_t timeout);

void __attribute__((weak)) SpiAbs_TaskSendReceiveCallback(void);


void SpiAbs_Send_Spi1_CompleteCallback_Task0(
    void * context, 
    uint32_t status, 
    const uint8_t *data, 
    uint32_t len
);
