#include "FIFO_util.h"

/****************************************************************************
 *
 * NAME: vFifoInit
 *
 * DESCRIPTION:
 * Initialises a FIFO.
 *
 * PARAMETERS: Name         RW Usage
 *             psQueue      W  FIFO queue to initialise
 *             ppvDataStart R  Array of pointers used as queue contents
 *             u8Entries    R  Number of entries in array, and hence queue
 *
 * RETURNS:
 * void
 *
 * NOTES:
 *
 ****************************************************************************/
PUBLIC void vFifoInit(tsSFqueue *psQueue, void **ppvDataStart, uint8 u8Entries)
{
    psQueue->ppvReadPtr = &ppvDataStart[0];
    psQueue->ppvWritePtr = &ppvDataStart[0];
    psQueue->ppvQueueStart = &ppvDataStart[0];
    psQueue->ppvQueueEnd = &ppvDataStart[u8Entries];
}

/****************************************************************************
 *
 * NAME: pvFifoPull
 *
 * DESCRIPTION:
 * Takes the next entry from the FIFO, or NULL if the FIFO is empty.
 *
 * PARAMETERS:      Name            RW  Usage
 *                  psQueue         R   FIFO from which to get entry
 *
 * RETURNS:
 * void * pointer to FIFO entry, or NULL if FIFO empty
 *
 ****************************************************************************/
PUBLIC void *pvFifoPull(tsSFqueue *psQueue)
{
    void *pvRetVal;

    if (psQueue->ppvReadPtr == psQueue->ppvWritePtr)
    {
        return (void *)NULL;
    }

    pvRetVal = *(psQueue->ppvReadPtr);

    if (psQueue->ppvReadPtr == psQueue->ppvQueueEnd)
    {
        psQueue->ppvReadPtr = psQueue->ppvQueueStart;
    }
    else
    {
        psQueue->ppvReadPtr++;
    }

    return pvRetVal;
}
/****************************************************************************
 *
 * NAME: bFifoPush
 *
 * DESCRIPTION:
 * Pushes an item onto the FIFO, if there is room
 *
 * PARAMETERS:      Name    RW  Usage
 *                  psQueue R   Queue to put item onto
 *                  pvData  R   Item to add
 *
 * RETURNS:
 * TRUE if item added successfully
 * FALSE if FIFO full
 *
 ****************************************************************************/
PUBLIC bool_t bFifoPush(tsSFqueue *psQueue, void *pvData)
{
    if (psQueue->ppvWritePtr == psQueue->ppvQueueEnd)
    {
        if (psQueue->ppvReadPtr == psQueue->ppvQueueStart)
        {
            return FALSE;
        }
    }
    else
    {
        if (psQueue->ppvReadPtr == (psQueue->ppvWritePtr + 1))
        {
            return FALSE;
        }
    }

    *(psQueue->ppvWritePtr) = pvData;

    if (psQueue->ppvWritePtr == psQueue->ppvQueueEnd)
    {
        psQueue->ppvWritePtr = psQueue->ppvQueueStart;
    }
    else
    {
        psQueue->ppvWritePtr++;
    }

    return TRUE;
}

