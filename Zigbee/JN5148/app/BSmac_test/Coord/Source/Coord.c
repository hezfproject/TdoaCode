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
#include <jendefs.h>
#include <AppHardwareApi.h>
#include <AppQueueApi.h>
#include <mac_sap.h>
#include <mac_pib.h>
#include <Utilities.h>

#include "config.h"
#include "JN5148_util.h"
#include "bsmac.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define EVENT_SEND_PACKET 0x0001

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
typedef enum
{
    E_STATE_IDLE,
    E_STATE_ENERGY_SCANNING,
    E_STATE_COORDINATOR_STARTED,
}teState;

/* Data type for storing data related to all end devices that have associated */
typedef struct
{
    uint16 u16ShortAdr;
    uint32 u32ExtAdrL;
    uint32 u32ExtAdrH;
}tsEndDeviceData;

typedef struct
{
    /* Data related to associated end devices */
    uint16          u16NbrEndDevices;
    tsEndDeviceData sEndDeviceData[MAX_END_DEVICES];

    teState eState;

    uint8   u8Channel;
    uint8   u8TxPacketSeqNb;
    uint8   u8RxPacketSeqNb;

}tsCoordinatorData;

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE void vInitSystem(void);
PRIVATE void vBsmacCallBack(unsigned char *pbuf, unsigned char len);
PRIVATE void vProcessEventQueues(void);
PRIVATE void vProcessAppEvent();

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
/* Handles from the MAC */
PRIVATE tsCoordinatorData sCoordinatorData;
PRIVATE uint8 rxbuf[5][BSMAC_RX_LEN_DEFAULT];
PRIVATE volatile uint8 rxbuf_head;
PRIVATE volatile uint8 rxbuf_tail;
PRIVATE volatile uint8 write_mac_cnt;
PRIVATE uint8 test[114];


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
	#ifdef WATCHDOG_ENABLED
	vAHI_WatchdogStop();
	#endif

    vInitSystem();

    while (1)
    {
	//vProcessEventQueues();
	vProcessAppEvent();

        if(rxbuf_tail != rxbuf_head)
        {
            unsigned char rv;
            rv = Jbsmac_eWriteData(rxbuf[rxbuf_tail], BSMAC_MAX_TX_PAYLOAD_LEN);
            if(rv == JBSMAC_WRITE_SUCCESS) 
            {
                rxbuf_tail++;
                if(rxbuf_tail == 5) rxbuf_tail = 0;
                write_mac_cnt++;
            }
        }
        
    };
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
    /* Setup interface to MAC */
    (void)u32AHI_Init();
    //(void)u32AppQApiInit(NULL, NULL, NULL);

    PrintfUtil_u8Init(E_AHI_UART_0, PRINTF_BAUD_RATE_1M);
    //MemUtil_vInit();
    TimerUtil_vEnableTimer();

    /* Initialise coordinator state */
    sCoordinatorData.eState = E_STATE_IDLE;
    sCoordinatorData.u8TxPacketSeqNb  = 0;
    sCoordinatorData.u8RxPacketSeqNb  = 0;
    sCoordinatorData.u16NbrEndDevices = 0;

    //vAHI_DioSetPullup(E_AHI_DIO11_INT, E_AHI_DIO8_INT);
    PrintfUtil_vPrintf("system start\n");
    Jbsmac_u8Init(vBsmacCallBack, E_AHI_UART_1, BSMAC_UART_BAUD_DIVISOR_500k, 0x1234);
    vAHI_DioSetDirection(0, E_AHI_DIO8_INT);//DIO 8 output    //RED LED ON
    vAHI_DioSetOutput(0, E_AHI_DIO8_INT);
    
    uint8 i;
    for(i =0 ;i< 114;i++)
    {
        test[i]=i;
    }

    //TimerUtil_eSetTimer(EVENT_SEND_PACKET, 1000);
    
}

PRIVATE void vProcessAppEvent()
{
    uint32 event = EventUtil_u32ReadEvents();
    
    switch(event)
    {
        case EVENT_SEND_PACKET:
        {
            Jbsmac_eWriteData(test, 114);
            EventUtil_vUnsetEvent(EVENT_SEND_PACKET);
            break;
        }
        default:
            break;
    }
    return;
}

PRIVATE void vBsmacCallBack(unsigned char *pbuf, unsigned char len)
{
    unsigned char newhead = (rxbuf_head+1)%5;
    if(newhead != rxbuf_tail)  
    {
        memcpy((void*) rxbuf[rxbuf_head], pbuf, len);
        rxbuf_head = newhead;
    }
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
