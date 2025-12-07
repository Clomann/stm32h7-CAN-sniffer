/*
 * buffers.h
 *
 *  Created on: Oct 24, 2024
 *      Author: cbromann
 */

#ifndef CM7_INC_BUFFERS_H_
#define CM7_INC_BUFFERS_H_

#include <string.h>
#include <stdint.h>

typedef enum
{
    RB_E_OK,
    RB_E_NOT_OK,
    RB_E_FULL,
} RingBufferErrorType;

typedef struct
{
    void *startAddress;
    uint32_t head;
    uint32_t tail;
    uint32_t bufferLength;
    uint32_t elementSize;
    uint32_t stride;
    uint32_t elementCount;
    _Bool isFull;
} RingBuffer;

static inline uint32_t ring_buffer_wrap_index(uint32_t index, uint32_t length)
{
    return index % length;
}

static inline void *ring_buffer_peek(RingBuffer *pBuffer)
{
    uint32_t SwBufferIndex;

    SwBufferIndex = pBuffer->head * pBuffer->stride;

    return (void *)((uint8_t *)pBuffer->startAddress + SwBufferIndex);
}

/* Return count of elements currently in the ring */
static inline uint32_t ring_buffer_count(const RingBuffer *rb)
{
    if (rb->head == rb->tail)
    {
        return rb->isFull ? rb->bufferLength : 0;
    }
    if (rb->head > rb->tail)
    {
        return rb->head - rb->tail;
    }
    return rb->bufferLength - (rb->tail - rb->head);
}

/* Read-only access to element at logical offset ‘pos’
   (0 = oldest, 1 = next, …).  Returns NULL if pos ≥ used.           */
static inline void *ring_buffer_peek_at(const RingBuffer *rb, uint32_t pos)
{
    uint32_t used;
    uint32_t phys;

    used = ring_buffer_count(rb);

    if (pos >= used)
    {
        return NULL;
    }

    if (0 == rb->stride)
    {
        return NULL;
    }

    phys = (rb->tail + pos) % rb->bufferLength;

    return (uint8_t *)rb->startAddress + phys * rb->stride;
}

/**
* @brief Reserve the next available slot in the ring buffer for writing
* 
* Atomically reserves the next available slot in the ring buffer and advances
* the head pointer. The caller can then safely write directly to the returned
* memory location without additional synchronization. This avoids the memcpy
* overhead of ring_buffer_put() while maintaining thread safety.
* 
* @param[in,out] pBuffer Pointer to the ring buffer structure
* 
* @return void* Pointer to the reserved slot memory, or NULL if buffer is full
* 
* @note The returned pointer is valid until the slot is consumed via ring_buffer_pop()
* @note This function advances the ring buffer state immediately - the slot is
*       considered "used" even if the caller hasn't written to it yet
* @note For thread safety, only this function call needs to be in a critical section,
*       not the subsequent writes to the returned memory location
* 
* @warning The caller must check for NULL return value before using the pointer
* @warning Do not call this function from interrupt context if the same buffer
*          is accessed from thread context without proper synchronization
* 
* @see ring_buffer_put() for copy-based insertion
* @see ring_buffer_peek() for read-only access to head slot
* 
* Example usage:
* @code
* SpiSlotType *slot = (SpiSlotType *)ring_buffer_reserve(buffer);
* if (slot != NULL) {
*     slot->data = my_data;
*     slot->length = data_len;
* }
* @endcode
*/
static inline void *ring_buffer_reserve(RingBuffer *pBuffer)
{
    void *element;

    if (pBuffer->isFull)
    {
        return NULL;
    }

    // Get the current slot
    element = ring_buffer_peek(pBuffer);

    // Atomically advance the head
    pBuffer->head =
        ring_buffer_wrap_index(pBuffer->head + 1, pBuffer->bufferLength);
    pBuffer->elementCount++;

    if (pBuffer->elementCount == pBuffer->bufferLength)
    {
        pBuffer->isFull = 1;
    }

    return element;
}

void CanAbs_InstrumentationIsrStartHook(void);
void CanAbs_InstrumentationIsrEndHook(void);

static inline RingBufferErrorType ring_buffer_put(RingBuffer *pBuffer, const void *pElement)
{
    RingBufferErrorType res = RB_E_OK;
    void *FreeElement;

    CanAbs_InstrumentationIsrStartHook();
    CanAbs_InstrumentationIsrEndHook();

    if (pBuffer->isFull)
    {
        res = RB_E_FULL;
        return res;
    }

    FreeElement = ring_buffer_peek(pBuffer);

    memcpy(FreeElement, pElement, pBuffer->elementSize);

    pBuffer->head++;
    pBuffer->elementCount++;

    pBuffer->head =
        ring_buffer_wrap_index(pBuffer->head, pBuffer->bufferLength);

    if (pBuffer->elementCount == pBuffer->bufferLength)
    {
        pBuffer->isFull = 1;
    }

    return res;
}

static inline void *ring_buffer_pop_ptr(RingBuffer *rb)
{
    uint32_t idx;
    void *elem;

    if (rb->elementCount == 0)
    {
        return NULL;
    }

    if (0 == rb->stride)
    {
        return NULL;
    }

    idx  = rb->tail * rb->stride;
    elem = (uint8_t *)rb->startAddress + idx;

    rb->tail++;
    rb->elementCount--;
    rb->tail = ring_buffer_wrap_index(rb->tail, rb->bufferLength);

    rb->isFull = 0;

    return elem;
}

static inline uint32_t ring_buffer_pop(RingBuffer *pBuffer, void *pElement)
{
    uint32_t SwBufferIndex;
    
    if (pBuffer->elementCount == 0)
    {
        return 1;
    }

    if (0 == pBuffer->stride)
    {
        pElement = NULL;
        return RB_E_NOT_OK;
    }

    SwBufferIndex = pBuffer->tail * pBuffer->stride;

    memcpy(
        pElement,
        (uint8_t *)pBuffer->startAddress + SwBufferIndex,
        pBuffer->elementSize
    );

    pBuffer->tail++;
    pBuffer->elementCount--;

    pBuffer->tail =
    ring_buffer_wrap_index(pBuffer->tail, pBuffer->bufferLength);

    pBuffer->isFull = 0;
    
    return RB_E_OK;
}

#endif /* CM7_INC_BUFFERS_H_ */
