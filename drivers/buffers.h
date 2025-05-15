/*
 * buffers.h
 *
 *  Created on: Oct 24, 2024
 *      Author: cbromann
 */

#ifndef CM7_INC_BUFFERS_H_
#define CM7_INC_BUFFERS_H_

#include <string.h>

typedef struct {
	void * startAddress;
	uint32_t head;
	uint32_t tail;
	uint32_t bufferLength;
	uint32_t elementSize;
	uint32_t elementCount;
	bool isFull;
} RingBuffer;

static inline void ring_buffer_put(RingBuffer *pBuffer, void *pElement)
{
	uint32_t SwBufferIndex;

	if (pBuffer->head > pBuffer->bufferLength - 1U)
	{
		pBuffer->head = 0;
	}

	SwBufferIndex = pBuffer->head * pBuffer->elementSize;

	memcpy((uint8_t *)pBuffer->startAddress + SwBufferIndex, pElement, pBuffer->elementSize);

	pBuffer->head++;
	pBuffer->elementCount++;
}

static inline unsigned int ring_buffer_pop(RingBuffer *pBuffer, void *pElement)
{
	uint32_t SwBufferIndex;

    if (pBuffer->elementCount <= 0)
    {
        return 1;
    }

	if (pBuffer->bufferLength <= pBuffer->tail)
	{
		pBuffer->tail = 0;
	}

	SwBufferIndex = pBuffer->tail * pBuffer->elementSize;

	memcpy(pElement, (uint8_t *)pBuffer->startAddress + SwBufferIndex, pBuffer->elementSize);

	pBuffer->tail++;
	pBuffer->elementCount--;

    return 0;
}

#endif /* CM7_INC_BUFFERS_H_ */
