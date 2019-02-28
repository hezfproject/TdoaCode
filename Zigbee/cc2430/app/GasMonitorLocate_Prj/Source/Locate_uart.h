/**************************************************************************************************
Filename:       Locate_uart.h
Revised:        $Date: 2011/07/22 18:49:55 $
Revision:       $Revision: 1.5 $

**************************************************************************************************/
#ifndef _LOCATE_UART_H
#define _LOCATE_UART_H

#include "hal_types.h"
#include "Hal_uart.h"
#include "string.h"
#include "Locate_protocol.h"
/* UART*/
/***************************************************/
//static uint_8 uartPort = HAL_UART_PORT_0;

#define GASMONITOR_FRAME_TIMEOUT   1// millisecond

extern bool    Locate_isvalid;
extern uint16  LocateDistance;
extern uint16 save_real_locatedistance;
extern uint16  Target_Name;
extern uint16 uartWantReadLen;
extern uint8   Rate;
extern int8    RSSI;
extern uint8  receive_uart_buff[128];
extern bool sync_flag;

#define  RESET_JN5148_BV           BV(4)
#define  RESET_JN5148_SBIT         P0_4
#define  RESET_JN5148_DDR          P0DIR
#define  RESET_JN5148_POLARITY     ACTIVE_LOW

#define  RESET_JN5148_BIT         4

#define  INIT_RESET_JN5148()   {    \
	  	P1SEL &= ~(0x1<<RESET_JN5148_BIT);			/*  Select GPIO*/  \
  		P1DIR |= (0x1<<RESET_JN5148_BIT);			/*  Set to output*/  \
  		P1INP &= ~(0x1<<RESET_JN5148_BIT);                 /* set to Pull-up*/     \
}
//extern  void  LocateUart_init(void);
extern  void  GasMonitor_stationID_fromdensity(uint8* pNum10000,uint8* pNum1000,uint8* pNum100, uint8* pNum10,uint8* pNum1,uint16 density);
extern void	 GasMonitor_stationRSSI(uint8* pNum10000,uint8* pNum1000,uint8* pNum100, uint8* pNum10,uint8* pNum1,int8 bRSSI);
extern  uint8  Locate_Uart_BuildandSendDataPAN(uint8 *p, uint8 len, uint16 DstAddr, uint16 DstPan);
extern   void GasMonitor_UartWrite(uint8* p, uint8 len, uint8 cmdtype);
extern  uint16 GasMonitor_Get_SAVE_LocateDistance(void);
extern  uint16 GasMonitor_GetLocateDistance(void);
extern uint16 GasMonitor_GetLocateTargetName(void);
extern int8 GasMonitor_Get_RSSI(void);
extern uint16  GasMonitor_Get_Rate(void);

#endif
/***************************************************/
