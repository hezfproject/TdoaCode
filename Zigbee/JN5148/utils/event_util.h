#ifndef EVENT_UTIL_H
#define EVENT_UTIL_H

#include "jendefs.h"

#define APP_MAX_EVENT 32

typedef enum
{
    E_EU_SUCCESS = 0,
    E_EU_QUEUE_FULL = 1,        
}EventUtil_eStatus;


PUBLIC void EventUtil_vSetEvent(uint32 u32Event);
PUBLIC void EventUtil_vUnsetEvent(uint32 u32Event);
PUBLIC bool EventUtil_bTestEvent(uint32 u32Event);
PUBLIC void EventUtil_vResetAllEvents(void);
PUBLIC uint32 EventUtil_u32ReadEvents(void);

#if 0
typedef struct
{
    uint8 u8Event;
    uint8 *pu8Data;    
}EventUtil_Item_s;

#endif

#endif

