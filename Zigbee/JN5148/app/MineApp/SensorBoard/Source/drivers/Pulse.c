/*
 * PULSE.c
 *
 * Created on: 2011-4-2
 * Author: Dong Biwen
 */
#include <AppHardwareApi.h>
#include "MainBroad.h"
#include "Pulse.h"
#include "RS485.h"

//#include <string.h>
//#include "config.h"
//#include "JN5148_util.h"

PUBLIC void Counter_init(void)
{
    //config PluseCounter_one
    bAHI_PulseCounterConfigure(E_AHI_PC_0,
    						  0,                  //上升沿触发Rising edge
    						  2,				  //4次采样消抖
    						  FALSE,              //Disable combined counter
    						  FALSE); 			  //Disable interrrupts
    //config PluseCounter_two
    bAHI_PulseCounterConfigure(E_AHI_PC_1,
    						  0,                  //上升沿触发
    						  2,				  //4次采样消抖
    						  FALSE,              //Disable combined counter
    						  FALSE); 			  //Disable interrrupts
    bAHI_Clear16BitPulseCounter(E_AHI_PC_0);
    bAHI_Clear16BitPulseCounter(E_AHI_PC_1);
}

PUBLIC void Read_Counter(void)
{
    uint16 plus_one,plus_tow;
    bAHI_StopPulseCounter(E_AHI_PC_0);
    bAHI_StopPulseCounter(E_AHI_PC_1);

    bAHI_Read16BitCounter(E_AHI_PC_0, &plus_one); //Read Counter Date
    bAHI_Read16BitCounter(E_AHI_PC_1, &plus_tow); //Read Counter Date

    mbus_hdr_slv.mbus_tlv.mbus_sensor_packed.mbus_sensor [9].value = u16Big_To_Little(plus_one);
    mbus_hdr_slv.mbus_tlv.mbus_sensor_packed.mbus_sensor [8].value = u16Big_To_Little(plus_tow);

    bAHI_Clear16BitPulseCounter(E_AHI_PC_0);
    bAHI_Clear16BitPulseCounter(E_AHI_PC_1);

    bAHI_StartPulseCounter(E_AHI_PC_0);
    bAHI_StartPulseCounter(E_AHI_PC_1);
}
