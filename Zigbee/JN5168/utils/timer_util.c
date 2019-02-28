
/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include "JN5148_util.h"
#include "MicroSpecific.h"

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#define MAX_TIMER_TICK    0x0FFFFFFF
#define MAX_TIMER_MS    16777        // max timer (ms)  16777 * 16000 = 0x0FFFF280
#define EX_INTERVAL        0x0E4E1C00    // 15 seconds = 15000*16000 ticks = 0x0E4E1C00
#define EX_MS            15000

#if (defined DEBUG_TIMERUTIL)
#define DBG(x) do{x}while(0);
#else
#define DBG(x)
#endif

/****************************************************************************/
#include "jendefs.h"
#include "JN5148_util.h"

#define MAX_TIMER_NUM 32

typedef struct
{
    bool_t bValid;
    bool_t bCircle;
    uint32 u32EventID;
    uint32 u32TimeLeftUS;
    uint32 u32TimerPeriodUS;
}tsTimerNode;

PRIVATE tsTimerNode TimerList[MAX_TIMER_NUM];

PRIVATE uint8 u8TimerNum = 0;

PRIVATE uint32 u32SystemUS = 0;
PRIVATE uint32 u32LastUpdateTick = 0;

PRIVATE bool_t bPrint = FALSE;

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
PRIVATE uint8 findTimer(uint32 u32EventID);
PRIVATE uint8 getNewTimer(void);
PRIVATE void setTimer(uint8 u8Index, uint32 u32EventID, uint32 u32ms, bool_t bCircle);
PRIVATE void deleteTimer(tsTimerNode *pTimer);


/****************************************************************************/

PUBLIC void TimerUtil_vSetDebugPrint(bool_t b)
{
    bPrint = b;
}

PUBLIC void TimerUtil_vInit()
{
    vAHI_TickTimerConfigure(E_AHI_TICK_TIMER_DISABLE);
    vAHI_TickTimerWrite(0);
    vAHI_TickTimerConfigure(E_AHI_TICK_TIMER_CONT);
    vAHI_TickTimerIntEnable(FALSE);

    uint8 index;
    u8TimerNum = 0;
    u32LastUpdateTick = 0;

    for(index = 0; index < MAX_TIMER_NUM; ++index)
    {
        TimerList[index].bValid = 0;
        TimerList[index].bCircle = 0;
        TimerList[index].u32TimeLeftUS = 0;
        TimerList[index].u32EventID = 0;
        TimerList[index].u32TimerPeriodUS = 0;
    }
}

PUBLIC void TimerUtil_vUpdate()
{
    uint32 u32Int;
    MICRO_DISABLE_AND_SAVE_INTERRUPTS(u32Int);

    uint32 ticks = (u32AHI_TickTimerRead() - u32LastUpdateTick);
    uint32 us = ticks >> 4; /* 16MHz system tick timer */
    uint8 index;
    tsTimerNode *pTimer;

    if(us > 0)
    {
        u32SystemUS += us;
        u32LastUpdateTick += us << 4; // *16
        DBG(if(bPrint) PrintfUtil_vPrintf("us %d %d\n", us, u8TimerNum);)
    }

    for(index=0; index<MAX_TIMER_NUM; index++)
    {
        pTimer = &TimerList[index];

        if(!pTimer->bValid) continue;

        if(us < pTimer->u32TimeLeftUS)
        {
            pTimer->u32TimeLeftUS -= us;

            DBG(if(bPrint)PrintfUtil_vPrintf("TU: %X\n", pTimer->u32EventID);)
        }
        else
        {
            EventUtil_vSetEvent(pTimer->u32EventID);
            if(!pTimer->bCircle)
            {
                deleteTimer(pTimer);
                DBG(if(bPrint)PrintfUtil_vPrintf("TR: %X\n", pTimer->u32EventID);)
            }
            else
            {
                /*****
                  (us%pTimer->u32TimerPeriodUS) is for some slow processing even longer than u32TimerPeriodUS
                 ****/
                pTimer->u32TimeLeftUS += pTimer->u32TimerPeriodUS - (us%pTimer->u32TimerPeriodUS);
                DBG(if(bPrint)PrintfUtil_vPrintf("TRC: %X\n", pTimer->u32EventID);)
            }
        }
    }

    MICRO_RESTORE_INTERRUPTS(u32Int);
}


/*Do not exceed 2mins*/
PUBLIC eTimerUtilStatus TimerUtil_eSetTimer(uint32 u32EventID, uint32 u32Ms)
{
    uint32 u32Int;
    uint8 timerIndex;
    MICRO_DISABLE_AND_SAVE_INTERRUPTS(u32Int);

    //update first to get accurate time;
    TimerUtil_vUpdate();

    timerIndex = findTimer(u32EventID);

    // can not assign new timer
    if(timerIndex == 0xFF && u8TimerNum >= MAX_TIMER_NUM)
    {
        MICRO_RESTORE_INTERRUPTS(u32Int);
        return E_TIMERUTIL_SET_FAIL;
    }

    //Not found
    if(timerIndex == 0xFF)
    {
        timerIndex = getNewTimer();
        u8TimerNum ++;
    }

    setTimer(timerIndex, u32EventID,  u32Ms, FALSE);

    MICRO_RESTORE_INTERRUPTS(u32Int);
    return E_TIMERUTIL_SET_SUCCESS;
}

PUBLIC eTimerUtilStatus TimerUtil_eSetCircleTimer(uint32 u32EventID, uint32 u32Ms)
{
    uint8 timerIndex;
    timerIndex = findTimer(u32EventID);

    // can not assign new timer
    if(timerIndex == 0xFF && u8TimerNum >= MAX_TIMER_NUM) return E_TIMERUTIL_SET_FAIL;

    //Not found
    if(timerIndex == 0xFF)
    {
        timerIndex = getNewTimer();
        u8TimerNum ++;
    }

    setTimer(timerIndex, u32EventID,  u32Ms, TRUE);

    return E_TIMERUTIL_SET_SUCCESS;
}

PUBLIC eTimerUtilStatus TimerUtil_eStopTimer(uint32 u32EventID)
{
    uint32 u32Int;
    MICRO_DISABLE_AND_SAVE_INTERRUPTS(u32Int);


    uint8 timerIndex;
    timerIndex = findTimer(u32EventID);

    if(timerIndex == 0xFF)
    {
        MICRO_RESTORE_INTERRUPTS(u32Int);
        return E_TIMERUTIL_STOP_NOT_FOUND;
    }

    deleteTimer(&TimerList[timerIndex]);

    MICRO_RESTORE_INTERRUPTS(u32Int);
    return E_TIMERUTIL_STOP_SUCCESS;
}

PUBLIC void TimerUtil_vStopAllTimer()
{
    uint8 index;

    for(index=0;index<MAX_TIMER_NUM;index++)
    {
        deleteTimer(&TimerList[index]);
    }
}

PRIVATE uint8 findTimer(uint32 u32EventID)
{
    uint8 i;
    for (i=0; i < MAX_TIMER_NUM; i++)
    {
        if(TimerList[i].u32EventID == u32EventID && TimerList[i].bValid) return i;
    }

    return 0xFF;
}

PRIVATE uint8 getNewTimer(void)
{
    uint8 i;
    for (i=0; i < MAX_TIMER_NUM; i++)
    {
        if(!TimerList[i].bValid) return i;
    }

    return 0xFF;
}

PRIVATE void setTimer(uint8 u8Index, uint32 u32EventID, uint32 u32ms, bool_t bCircle)
{
    TimerList[u8Index].u32EventID = u32EventID;
    TimerList[u8Index].u32TimerPeriodUS = u32ms*1000;
    TimerList[u8Index].u32TimeLeftUS = TimerList[u8Index].u32TimerPeriodUS;
    TimerList[u8Index].bValid = TRUE;
    TimerList[u8Index].bCircle = bCircle;

    DBG(if(bPrint)PrintfUtil_vPrintf("TS: %d %X %d %d\n", u8Index, u32EventID, u32ms, bCircle);)
}

PRIVATE void deleteTimer(tsTimerNode *pTimer)
{
    if(!pTimer->bValid)
    {
        return;
    }

    pTimer->bValid = FALSE;
    pTimer->bCircle = FALSE;
    pTimer->u32EventID = 0;
    pTimer->u32TimeLeftUS = 0;
    pTimer->u32TimerPeriodUS = 0;

    u8TimerNum --;
}

#if (defined SUPPORT_HARD_WATCHDOG)
PUBLIC void vFeedHardwareWatchDog(void)
{
    uint32 dio;
    dio = u32AHI_DioReadInput();
    if(0 == (dio & E_AHI_DIO12_INT))
    {
        vAHI_DioSetOutput(E_AHI_DIO12_INT,0);
    }
    else
    {
        vAHI_DioSetOutput(0,E_AHI_DIO12_INT);
    }
}
#endif


PUBLIC void TimerUtil_vDelay(uint16 u16Delay, eTimerUtilDelay eTimeUnit)
{
    uint16 u16Scale;
    uint32 u32BaseTick = u32AHI_TickTimerRead();

    if(E_TIMER_UNIT_MICROSECOND == eTimeUnit)
        u16Scale = 16;

    else if(E_TIMER_UNIT_MILLISECOND == eTimeUnit)
        u16Scale = 16000;
    else
        return;

    while(u32AHI_TickTimerRead() - u32BaseTick < (uint32)(u16Scale*u16Delay))
    {
        if(u16AHI_WatchdogReadValue() > 16)
        {
            vAHI_WatchdogRestart();

            #if (defined SUPPORT_HARD_WATCHDOG)
            vFeedHardwareWatchDog();
            #endif
        }
    };    // delay
}

PUBLIC uint32 TimerUtil_GetSystemTimer(void)
{
    return u32SystemUS;
}


