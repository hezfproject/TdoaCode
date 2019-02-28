#ifndef FIFO_UTIL_H
#define FIFO_UTIL_H

#include "jendefs.h"
/* Each FIFO queue has a pointer to the current read position and write
   position, and the start and end of the queue */
typedef struct
{
    void **ppvReadPtr;
    void **ppvWritePtr;
    void **ppvQueueStart;
    void **ppvQueueEnd;
} tsSFqueue;

PUBLIC void vFifoInit(tsSFqueue *psQueue, void **ppvDataStart, uint8 u8Entries);
PUBLIC void *pvFifoPull(tsSFqueue *psQueue);
PUBLIC bool_t bFifoPush(tsSFqueue *psQueue, void *pvData);

#endif
