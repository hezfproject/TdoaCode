/****************************************************************************
 *
 * MODULE:             coordinator.c
 *
 * COMPONENT:          coordinator.c,v
 *
 * VERSION:
 *
 * REVISION:           1.3
 *
 * DATED:              2006/11/06 10:03:22
 *
 * STATUS:             Exp
 *
 * AUTHOR:
 *
 * DESCRIPTION:
 *
 * LAST MODIFIED BY:
 *                     $Modtime: $
 *
 ****************************************************************************
 *
 * This software is owned by Jennic and/or its supplier and is protected
 * under applicable copyright laws. All rights are reserved. We grant You,
 * and any third parties, a license to use this software solely and
 * exclusively on Jennic products. You, and any third parties must reproduce
 * the copyright and warranty notice and any other legend of ownership on each
 * copy or partial copy of the software.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS". JENNIC MAKES NO WARRANTIES, WHETHER
 * EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE,
 * ACCURACY OR LACK OF NEGLIGENCE. JENNIC SHALL NOT, IN ANY CIRCUMSTANCES,
 * BE LIABLE FOR ANY DAMAGES, INCLUDING, BUT NOT LIMITED TO, SPECIAL,
 * INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON WHATSOEVER.
 *
 * Copyright Jennic Ltd 2009. All rights reserved
 *
 ****************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <AppQueueApi.h>
#include <AppHardwareApi.h>
#include <AppHardwareApi_JN514x.h>
#include "JN5148_util.h"

#include "MainBroad.h"
#include "ADS1100.h"
#include "TCA6416A.h"
#include "OD2101.h"
#include "Pulse.h"
#include "RS485.h"



/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define EVENT_ADS1100_TEST (0x0001)
#define EVENT_TCA6416A_TEST (0x0002)
#define EVENT_RELAY_TEST (0x0004)
#define EVENT_CONTER_TEST (0x0008)
#define EVENT_INIT_IIC    (0x0010)

#define DIO_INPUT_INIT (0x00003000)      //Enable input DIO_12,DIO_13
#define DIO_OUTPUT_INIT (0x00040A01)     //Enable output DIO_0,DIO_9,DIO_11,DIO_18
#define DIO_UP_INIT (0x00043201)         //WakePull Up
#define Max_address_num (32254)
#define Min_address_num (32001)
#define TEN_MINUTES     (600)
#define FIVE_SECONDS   (50)

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE void vInitSystem(void);
PRIVATE void vProcessAppEvent();

PRIVATE uint16 time_count = 0;
PRIVATE uint16 InitTCA6416A_Count = 0;


/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
PUBLIC uint8  gCurrent_Channel = 0 ;
PUBLIC uint8  gCurrent_Channel_old = 1;
PUBLIC uint16 gflash_bit = 0;

PUBLIC uint32 DIO_status,*p;
PUBLIC uint8  DIO_flag;

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: AppColdStart
 *
 * DESCRIPTION:
 * Entry point for application from boot loader. Initialises system and runs
 * main loop.
 *
 * RETURNS:
 * Never returns.
 *
 ****************************************************************************/
PUBLIC void AppColdStart(void)
{
    /* Disable watchdog if enabled by default */

    vInitSystem();
    while (1)
    {
     vAHI_WatchdogRestart();
     vProcessAppEvent();
     if(TX_flag)
     {
        vAHI_DioSetOutput(0x0800,0);     //DIO_11 set 1,占用总线
        Send_Pack (fram_control);
        vAHI_DioSetOutput(0,0x0800);          //DIO_11 Clear 0 ,释放总线
        TX_flag = 0;
     }
    }
}

/****************************************************************************
 *
 * NAME: AppWarmStart
 *
 * DESCRIPTION:
 * Entry point for application from boot loader. Simply jumps to AppColdStart
 * as, in this instance, application will never warm start.
 *
 * RETURNS:
 * Never returns.
 *
 ****************************************************************************/
PUBLIC void AppWarmStart(void)
{
    AppColdStart();
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

PRIVATE void Flash_LED (void)
{
    if((u16StationPanId < Min_address_num) || (u16StationPanId > Max_address_num))
    {
        gflash_bit |= (PORT_RUN_LED);
    }
    else
    {
        gflash_bit &= (~PORT_RUN_LED);
    }
    if(u16Big_To_Little(mbus_hdr_slv.mbus_tlv.mbus_sensor_packed.mbus_sensor [0].value) >= 1310) gflash_bit &= (~PORT_CURRENT1_LED);else gflash_bit |= PORT_CURRENT1_LED;
    if(u16Big_To_Little(mbus_hdr_slv.mbus_tlv.mbus_sensor_packed.mbus_sensor [1].value) >= 1310) gflash_bit &= (~PORT_CURRENT2_LED);else gflash_bit |= PORT_CURRENT2_LED;
    if(u16Big_To_Little(mbus_hdr_slv.mbus_tlv.mbus_sensor_packed.mbus_sensor [2].value) >= 1310) gflash_bit &= (~PORT_CURRENT3_LED);else gflash_bit |= PORT_CURRENT3_LED ;
    if(u16Big_To_Little(mbus_hdr_slv.mbus_tlv.mbus_sensor_packed.mbus_sensor [3].value) >= 1310) gflash_bit &= (~PORT_CURRENT4_LED);else gflash_bit |= PORT_CURRENT4_LED;
    if(u16Big_To_Little(mbus_hdr_slv.mbus_tlv.mbus_sensor_packed.mbus_sensor [4].value) >= 1310) gflash_bit &= (~PORT_CURRENT5_LED);else gflash_bit |= PORT_CURRENT5_LED;
    if(u16Big_To_Little(mbus_hdr_slv.mbus_tlv.mbus_sensor_packed.mbus_sensor [5].value) >= 1310) gflash_bit &= (~PORT_CURRENT6_LED);else gflash_bit |= PORT_CURRENT6_LED;
    if(u16Big_To_Little(mbus_hdr_slv.mbus_tlv.mbus_sensor_packed.mbus_sensor [6].value) >= 1310) gflash_bit &= (~PORT_CURRENT7_LED);else gflash_bit |= PORT_CURRENT7_LED;
    if(u16Big_To_Little(mbus_hdr_slv.mbus_tlv.mbus_sensor_packed.mbus_sensor [7].value) >= 1310) gflash_bit &= (~PORT_CURRENT8_LED);else gflash_bit |= PORT_CURRENT8_LED;
    if(u16Big_To_Little(mbus_hdr_slv.mbus_tlv.mbus_sensor_packed.mbus_sensor [8].value) >= 180) gflash_bit &= (~PORT_PLUS1_LED);else gflash_bit |= PORT_PLUS1_LED;
    if(u16Big_To_Little(mbus_hdr_slv.mbus_tlv.mbus_sensor_packed.mbus_sensor [9].value) >= 180) gflash_bit &= (~PORT_PLUS2_LED);else gflash_bit |= PORT_PLUS2_LED;
    if(u16Big_To_Little(mbus_hdr_slv.mbus_tlv.mbus_sensor_packed.mbus_sensor [10].value) >= 0xffff) gflash_bit &= (~PORT_RELAY1_LED);else gflash_bit |= PORT_RELAY1_LED;
    if(u16Big_To_Little(mbus_hdr_slv.mbus_tlv.mbus_sensor_packed.mbus_sensor [11].value) >= 0xffff) gflash_bit &= (~PORT_RELAY2_LED);else gflash_bit |= PORT_RELAY2_LED;
    TCA6416A_Write_Port (gflash_bit);
    if((gflash_bit & PORT_LINK_LED) == 0) gflash_bit |= PORT_LINK_LED;
    if((gflash_bit & PORT_485_LED) == 0)  gflash_bit |= PORT_485_LED;
}

#define DIO_12 (0x00001000)
#define DIO_13 (0x00002000)

PRIVATE void Relay_State (void)
{
  vAHI_DioSetDirection(DIO_INPUT_INIT,0);   //Enable input DIO_12,DIO_13
    if(u32AHI_DioReadInput()&DIO_12) mbus_hdr_slv.mbus_tlv.mbus_sensor_packed.mbus_sensor [10].value = u16Big_To_Little(0xFFFF);
    else                             mbus_hdr_slv.mbus_tlv.mbus_sensor_packed.mbus_sensor [10].value = u16Big_To_Little(0x0000);
    if(u32AHI_DioReadInput()&DIO_13) mbus_hdr_slv.mbus_tlv.mbus_sensor_packed.mbus_sensor [11].value = u16Big_To_Little(0xFFFF);
    else                             mbus_hdr_slv.mbus_tlv.mbus_sensor_packed.mbus_sensor [11].value = u16Big_To_Little(0x0000);
}


/****************************************************************************
 *
 * NAME: vInitSystem
 *
 * DESCRIPTION:
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vInitSystem(void)
{
    gCurrent_Channel = 0;
    (void)u32AppQApiInit(NULL, NULL, NULL);

    u32AHI_Init();
    //vAHI_WatchdogStop();
    vAHI_WatchdogStart(12);           //Timeout Period =(2^(12-1)+1)X8 ms = 16.4s
    vAHI_DioSetDirection(DIO_INPUT_INIT,DIO_OUTPUT_INIT);
    vAHI_DioSetPullup(DIO_UP_INIT,0x00000000);      //WakePull Up
    PrintfUtil_u8Init(E_AHI_UART_0, PRINTF_BAUD_RATE_115200);
    vAHI_SiMasterConfigure( FALSE,  //1.Enable pulse suppression filter
    					    FALSE,  //2.Disable Serial Interface interruptEnable
    						0x07 ); //3.200Kps BraudRate Operating frequency = 16/[(PreScaler + 1) x 5] MHz
    MacUtil_vReadExtAddress(&psMacAddr);
    u16StationPanId = psMacAddr.u32L & 0xFFFF;
    if(u16StationPanId >=Min_address_num)
    {
        MbusSlvSensorAddr = u16StationPanId - Min_address_num + 1;
    }
    else
    {
        MbusSlvSensorAddr = 0;
    }
    TCA6416A_Init ();
    //PrintfUtil_vPrintf("u16StationPanId:  \n\r");
    //PrintfUtil_vPrintMem((uint8*)&psMacAddr, sizeof(psMacAddr));
    //PrintfUtil_vPrintf("u16StationPanId: %d AD1: %d \n\r", u16StationPanId,u16StationPanId);
    ADS1100_Init();
    OD2101_Init();
    Counter_init();
    RS485_Init ();
    TimerUtil_vInit();
    TimerUtil_eSetTimer(EVENT_INIT_IIC, 1000);
    TimerUtil_eSetTimer(EVENT_ADS1100_TEST, 2000);
    TimerUtil_eSetTimer(EVENT_TCA6416A_TEST, 1000);
    TimerUtil_eSetTimer(EVENT_RELAY_TEST, 1000);
    TimerUtil_eSetTimer(EVENT_CONTER_TEST, 1000);
}

PRIVATE void vProcessAppEvent()
{
	uint32 event = EventUtil_u32ReadEvents();
    TimerUtil_vUpdate();
    switch(event)
    {
        case EVENT_ADS1100_TEST:       //获取AD值
        {
            gCurrent_Channel = ADS1100_ReadBlock (gCurrent_Channel);
			TimerUtil_eSetTimer(EVENT_ADS1100_TEST, 150);
			EventUtil_vUnsetEvent(event);
        	break;
        }
        case EVENT_TCA6416A_TEST:      //状态灯指示
        {
            if(InitTCA6416A_Count++ > FIVE_SECONDS)
            {
                TCA6416A_Init ();
                InitTCA6416A_Count = 0;
            }
        	Flash_LED ();
			TimerUtil_eSetTimer(EVENT_TCA6416A_TEST, 100);
			EventUtil_vUnsetEvent(event);
        	break;
        }
        case EVENT_RELAY_TEST:         //继电器
        {
        	Relay_State ();
        	TimerUtil_eSetTimer(EVENT_RELAY_TEST, 100);
        	EventUtil_vUnsetEvent(event);
        	break;
        }
        case EVENT_CONTER_TEST:       //频率
        {
        	Read_Counter();
        	TimerUtil_eSetTimer(EVENT_CONTER_TEST, 1000);
            if(time_count++ > TEN_MINUTES)              //计时5分钟
            {
                time_count = 0;
                TimerUtil_eSetTimer(EVENT_INIT_IIC, 1000);
            }
        	EventUtil_vUnsetEvent(event);
        	break;
        }
        case EVENT_INIT_IIC:       //定时init IIC BUS
        {
            vAHI_SiMasterDisable();
            vAHI_SiMasterConfigure( FALSE,  //1.Enable pulse suppression filter
    					    FALSE,  //2.Disable Serial Interface interruptEnable
    						0x07 ); //
        	EventUtil_vUnsetEvent(event);
        	break;
        }
        default:
            EventUtil_vUnsetEvent(event);
            break;
    }

}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
