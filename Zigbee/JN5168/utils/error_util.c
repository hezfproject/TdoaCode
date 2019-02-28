#include "JN5148_util.h"

PRIVATE uint32 LED0=0;
PRIVATE uint32 LED1=0;
PRIVATE uint32 LED2=0;

bool_t ErrorUtil_vRegisterLed0(uint32 u32LedDIO0)
{
    if(u32LedDIO0 < E_AHI_DIO0_INT || u32LedDIO0 > E_AHI_DIO19_INT )
    {
        return FALSE;
    }

    LED0 = u32LedDIO0;
    return TRUE;
}

bool_t ErrorUtil_vRegisterLed1(uint32 u32LedDIO1)
{
    if(u32LedDIO1 < E_AHI_DIO0_INT || u32LedDIO1 > E_AHI_DIO19_INT )
    {
        return FALSE;
    }

    LED1 = u32LedDIO1;
    return TRUE;
}

bool_t ErrorUtil_vRegisterLed2(uint32 u32LedDIO2)
{
    if(u32LedDIO2 < E_AHI_DIO0_INT || u32LedDIO2 > E_AHI_DIO19_INT )
    {
        return FALSE;
    }

    LED2= u32LedDIO2;
    return TRUE;
}

/**************************************************
* Function:  vFatalHalt1
* used for those have only 1 LED
* only support 8 types of errorCodes
***************************************************/
void ErrorUtil_vFatalHalt1(uint8 errorCode)
{
    while(1)
    {
        LedUtil_vFlashAll(500, 10);
    
        LedUtil_vFlash(LED0, 2000, errorCode);
        TimerUtil_vDelay(10000, E_TIMER_UNIT_MILLISECOND);
    };
}


/**************************************************
* Function:  vFatalHalt2
* used for those have only 2 LEDs
* only support 16 types of errorCodes
***************************************************/
void ErrorUtil_vFatalHalt2(uint8 errorCode)
{
    while(1)
    {
        LedUtil_vFlashAll(500, 10);

        // flashing 1 time means zero
        LedUtil_vFlash(LED0, 2000, (errorCode & 0x3) +1);

        TimerUtil_vDelay(2000, E_TIMER_UNIT_MILLISECOND);
        
        // flashing 1 time means zero
        LedUtil_vFlash(LED1, 2000, (errorCode & 0xC) +1);
        
        TimerUtil_vDelay(10000, E_TIMER_UNIT_MILLISECOND);
    };
}


/**************************************************
* Function:  vFatalHalt3
* used for those have 3 LEDs
* only support 64 types of errorCodes
***************************************************/
void ErrorUtil_vFatalHalt3(uint8 errorCode)
{
    while(1)
    {
        LedUtil_vFlashAll(500, 10);

        // flashing 1 time means zero
        LedUtil_vFlash(LED0, 2000, (errorCode & 0x3) +1);

        TimerUtil_vDelay(2000, E_TIMER_UNIT_MILLISECOND);
        
        // flashing 1 time means zero
        LedUtil_vFlash(LED1, 2000, ((errorCode & 0xC)>>2) +1);
        
        TimerUtil_vDelay(2000, E_TIMER_UNIT_MILLISECOND);
        
        // flashing 1 time means zero
        LedUtil_vFlash(LED2, 2000, ((errorCode & 0x30)>>4) +1);

        TimerUtil_vDelay(10000, E_TIMER_UNIT_MILLISECOND);
    };
}


