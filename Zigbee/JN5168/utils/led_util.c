/***********************************************************
* This file contains all functions related to LED controls.
* Pull_down means light ON; pull_up means light OFF
***********************************************************/

#include "jendefs.h"
#include "JN5148_util.h"

PRIVATE uint32 LEDregistedMap = 0;

PUBLIC bool LedUtil_bRegister(uint32 u32LedDIO)
{
    if(u32LedDIO < E_AHI_DIO0_INT || u32LedDIO > E_AHI_DIO19_INT )
    {
        return FALSE;
    }

    LEDregistedMap |= u32LedDIO;
    
    vAHI_DioSetDirection(0, u32LedDIO);

    //Defaule off
    vAHI_DioSetOutput(u32LedDIO, 0); 

    return TRUE;
}

PUBLIC void LedUtil_vOn(uint32 u32LedDIO)
{
    if(u32LedDIO < E_AHI_DIO0_INT || u32LedDIO > E_AHI_DIO19_INT || !(u32LedDIO & LEDregistedMap))
    {
        return;
    }
    
    vAHI_DioSetOutput(0, u32LedDIO); 
}

PUBLIC void LedUtil_vOff(uint32 u32LedDIO)
{
    if(u32LedDIO < E_AHI_DIO0_INT || u32LedDIO > E_AHI_DIO19_INT  || !(u32LedDIO & LEDregistedMap))
    {
        return;
    }
    
    vAHI_DioSetOutput(u32LedDIO, 0); 
}

PUBLIC void LedUtil_vToggle(uint32 u32LedDIO)
{
    if(u32LedDIO < E_AHI_DIO0_INT || u32LedDIO > E_AHI_DIO19_INT  || !(u32LedDIO & LEDregistedMap))
    {
        return;
    }

    
    if(u32LedDIO & u32AHI_DioReadInput())
    {
        vAHI_DioSetOutput(0, u32LedDIO); 
    }
    else
    {
        vAHI_DioSetOutput(u32LedDIO, 0);
    }
}

PUBLIC void LedUtil_vFlash(uint32 u32LedDIO, uint16 u16PeriodMs, uint16 u16Times)
{
    uint8 i;

    LedUtil_vOff(u32LedDIO);

    for(i=0;i<u16Times*2;i++)
    {
        TimerUtil_vDelay(u16PeriodMs/2, E_TIMER_UNIT_MILLISECOND);
        LedUtil_vToggle(u32LedDIO);
    }
}

PUBLIC void LedUtil_vFlashAll(uint16 u16PeriodMs, uint16 u16Times)
{
    uint8 i, j;

    for(i=0;i<21;i++)
    {
        if(LEDregistedMap & (1U<<i)) 
            LedUtil_vOff((1U<<i));
    }

    for(i=0;i<u16Times*2;i++)
    {
        TimerUtil_vDelay(u16PeriodMs/2, E_TIMER_UNIT_MILLISECOND);

        for(j=0;j<21;j++)
        {
            if(LEDregistedMap & (1U<<j))
                LedUtil_vToggle((1U<<j));
        }
    }
}
