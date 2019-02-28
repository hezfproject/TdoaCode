
/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include "jendefs.h"
#include "event_util.h"


/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/


/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
static uint32 u32EventBitMap = 0;

/****************************************************************************
 *
 * NAME:       EventUtil_vSetEvent
 *
 * DESCRIPTION:
 * Set an evnet
 *
 * PARAMETERS:      Name            RW  Usage                  
 *
 * RETURNS:
 * none
 *
 * NOTE:
 * one event occupies one bit. 
 * the smaller, the higher priority
 ****************************************************************************/
PUBLIC void EventUtil_vSetEvent(uint32 u32Event)
{
    u32EventBitMap |= u32Event;
}


/****************************************************************************
 *
 * NAME:       EventUtil_vUnsetEvent
 *
 * DESCRIPTION:
 * Unset an evnet
 *
 * PARAMETERS:      Name            RW  Usage                  
 *
 * RETURNS:
 * none
 *
 * NOTE:
 * one event occupies one bit
 ****************************************************************************/
PUBLIC void EventUtil_vUnsetEvent(uint32 u32Event)
{
	if (u32Event)
		u32EventBitMap &= (~u32Event);
}

/****************************************************************************
 *
 * NAME:       EventUtil_vTestEvent
 *
 * DESCRIPTION:
 * Test if an evnet set
 *
 * PARAMETERS:      Name            RW  Usage                  
 *
 * RETURNS:
 * TURE if having been set
 *
 * NOTE:
 * one event occupies one bit
 ****************************************************************************/

PUBLIC bool EventUtil_bTestEvent(uint32 u32Event)
{
    return u32EventBitMap & u32Event ? TRUE: FALSE;
}

/****************************************************************************
 *
 * NAME:       EventUtil_vResetAllEvents
 *
 * DESCRIPTION:
 * Reset/Remove all events
 *
 * PARAMETERS:      Name            RW  Usage                  
 *
 * RETURNS:
 * none
 ****************************************************************************/
PUBLIC void EventUtil_vResetAllEvents(void)
{
    u32EventBitMap = 0;
}


/****************************************************************************
 *
 * NAME:       EventUtil_u32ReadEvents
 *
 * DESCRIPTION:
 * Read an event. Always return the highest priority event;
 *
 * PARAMETERS:      Name            RW  Usage                  
 *
 * RETURNS:
 * none

 * NOTE:
 * the smaller, the higher priority
 ****************************************************************************/
PUBLIC uint32 EventUtil_u32ReadEvents(void)
{
    static uint32 pos =0x1;
    uint32 event;
    uint8 counter;

    if(u32EventBitMap == 0) return 0;
    
    for(counter = 0;counter<APP_MAX_EVENT;counter++)
    {
        if(pos == 0) pos = 0x1;
        event = u32EventBitMap & pos;
        pos <<= 1;
        if(event) {            
            break;
        }
    }

    return event;
}

#if 0

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/


/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
/* Buffer entries */
EventUtil_Item_s asAppEventBuffer[APP_MAX_EVENT];

/* Queues for spare buffer entries */
tsSFqueue sAppEventBufferQueue;
void     *apvAppEventBufferData[APP_MAX_EVENT + 1];

/* Queues for used buffer entries */
tsSFqueue sAppEventQueue;
void     *apvAppEventData[APP_MAX_EVENT + 1];


/****************************************************************************
 *
 * NAME:       EventUtil_vApiInit
 *
 * DESCRIPTION:
 * Set an evnet
 *
 * PARAMETERS:      Name            RW  Usage                  
 *
 * RETURNS:
 * none
 *
 ****************************************************************************/
PUBLIC void EventUtil_vApiInit()
{
    int i;

    /* Initialise 'spare buffers' queues and fill with spare buffers */
    vFifoInit(&sAppEventBufferQueue, apvAppEventBufferData, APP_MAX_EVENT);
    for (i = 0; i < APP_MAX_EVENT; i++)
    {
        bFifoPush(&sAppEventBufferQueue, (void *)&asAppEventBuffer[i]);
    }

    /* Initialise 'used buffers' queues */
    vFifoInit(&sAppEventQueue, apvAppEventData, APP_MAX_EVENT);
    
}

/****************************************************************************
 *
 * NAME:       EventUtil_eSetEvent
 *
 * DESCRIPTION:
 * Set an evnet
 *
 * PARAMETERS:      Name            RW  Usage                  
 *
 * RETURNS:
 * E_EU_SUCCESS on success 
 *
 ****************************************************************************/

/* do not support psEventItem->data for now, need dynamic memory management, but keep the interface for future fix*/
PUBLIC  EventUtil_eStatus EventUtil_eSetEvent(void* pvParam, EventUtil_Item_s *psEventItem)
{
    /* Fetch buffer */
    EventUtil_Item_s* psBuf = pvFifoPull(&sAppEventBufferQueue);

    if(psBuf == NULL) return E_EU_QUEUE_FULL;
    
    psBuf->u8Event = psEventItem->u8Event;
    bFifoPush(&sAppEventQueue, (void *)psEventItem);

    return E_EU_SUCCESS;
}

PUBLIC EventUtil_Item_s* EventUtil_psReadEvent(void)
{
    return (EventUtil_Item_s *)pvFifoPull(&sAppEventQueue);
}


/****************************************************************************
 *
 * NAME:       EventUtil_vReturnBuffer
 *
 * DESCRIPTION:
 * Returns a buffer to the MLME spare buffers queue
 *
 * PARAMETERS:      Name            RW  Usage
 *                  psBuffer        R   Buffer to return
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void EventUtil_vReturnBuffer(EventUtil_Item_s *psBuffer)
{
    bFifoPush(&sAppEventBufferQueue, (void *)psBuffer);
}
#endif



